# TILT-FM flash map (1 MiB NOR, FM-1)

Round 2 of the new interface (8 scenes x 3 songs) took the three user sample slots for the scenes. Nothing below
`0x4000` is ever written (the bootloader); the app writes only inside the ranges marked **app data** below
(`hal/fm1_flash.h` FL_STORE_OK / FL_RANGE_OK: any other address is refused by the driver).

| Range | Size | What | Notes |
|---|---|---|---|
| `0x000000..0x003FFF` | 16 KB | bootloader | never written |
| `0x004000..0x092FFF` | 588 KB | app (`felucca.bin`, XIP) + the loader | written only by the OTA / installer |
| `0x097000..0x09EFFF` | 32 KB | **scenes 1-4** (song 1, A..D): 4 objects x A/B sectors | unchanged since 1.0; the migration moves nothing |
| `0x09F000` | 4 KB | user presets' FM6 patches, copy A | as before |
| `0x0A0000..0x0C7FFF` | 160 KB | **scenes 5-24** (song 1 E..H, songs 2 and 3 A..H): 20 objects x A/B | was user sample slots 1-3 |
| `0x0C8000..0x0CDFFF` | 24 KB | **songs 1-3**: one record each (the song's globals, chain, MIDI channels, name), A/B | |
| `0x0CE000..0x0CFFFF` | 8 KB | **format tag** (`TMS8`), A/B | marks the region as TFM scenes: the migration runs once |
| `0x0D0000..0x0DBFFF` | 48 KB | free | was the end of the sample slots; left blank |
| `0x0DC000..0x0DFFFF` | 16 KB | user preset banks 1-2 | as before |
| `0x0E0000..0x0E4FFF` | 20 KB | OTA staging | as before |
| `0x0E5000..0x0E6FFF` | 8 KB | autosave (A/B) | as before |
| `0x0E7000` | 4 KB | FM6 voice bank | as before |
| `0x0FC000..0x0FDFFF` | 8 KB | settings (A/B) | as before; one byte of `favorites.factory[13][8]` now remembers the song and scene |
| `0x0FE000` | 4 KB | user presets' FM6 patches, copy B | as before |

Before round 2 the app data was `0x097000..0x0DFFFF` plus the settings; the user sample slots were
`0xA0000`, `0xB4000`, `0xC8000` (80 KB each). Now `0x97000..0xCFFFF` is scenes, songs and the tag.

## Objects

Every object is an A/B sector pair (storage.c): an erase of the other copy, the payload, the 32-byte commit record
(magic, type, sequence, length, CRCs) **last**. A write cut short anywhere leaves the previous copy in charge.

| Type | Object | Sectors |
|---|---|---|
| 1-4 | scene g = 0..3 (song 1, A..D) | `0x97000 + 0x2000 g` (+ `0x1000` for B) |
| 40-59 | scene g = 4..23 | `0xA0000 + 0x2000 (g - 4)` |
| 64-66 | song n = 0..2 | `0xC8000 + 0x2000 n` |
| 67 | format tag | `0xCE000` |

A scene is a project record of the same format as ever (FUN10, 3,840 B). A song record is `song_rec_t` (project.c).
The chain inside both keeps the stored encoding of every older firmware (A..D = 0..3, "-" = 4) and adds E..H as 5..8.

## First boot after the update (migration)

1. The tag is not there: the whole region `0xA0000..0xDBFFF` is wiped (blank sectors are skipped).
2. Song 1's record is written from scene A (else the first stored scene): its globals, chain, MIDI channels and name.
3. The tag is written last.

Scenes A..D are not touched. A power cut anywhere leaves them as they were and no tag; the next boot starts again.
`tests/persistence_test.c` cuts power at each of the 66 erase / program calls of it (and tears the one it cuts).

## Playing

Only the current scene is in RAM. A scene switch while playing is read from flash (read-only; no erase or program
happens) and flips on the bar line. The scene left, if edited, waits in RAM (up to three scenes) and is written when the
transport stops. A SONG that plays holds the patterns of the scenes it uses (up to eight) in the pool.

## Going back

An older firmware sees `0x97000..0x9EFFF` as its four project slots and `0xA0000 / 0xB4000 / 0xC8000` as its three user
sample slots (an `FSMP` header at each); there is none, so they read as empty. A scene whose chain uses E..H does not
load in an older build (the chain bytes 5..8 are not valid there); the data stays. An older build that uploads a sample
overwrites whatever is in that slot's range: scenes / songs there read as empty here; when the tag (`0xCE000`, the third
slot's range) goes the next boot migrates again from scenes A..D. The stock firmware does not use this region. See
`docs/tilt/FLASHING-CHECKLIST.md` 4a.
