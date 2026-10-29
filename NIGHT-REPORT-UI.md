# TFM-UI report (branch `tfm-ui`, from `tfm-next`)

## READ THIS FIRST
- **Package to try:** `tilt-fm-ui-b84c337.fwsc` (this folder, 610,090 B, identity FM-1_984), built from `b84c337` after the final full test run. **Nothing was flashed.** It is an untested-on-hardware build: try it in the emulator first.
- **Emulator:** in WSL `cd ~/TILT-FM && git checkout tfm-ui && sh web/emu/build.sh && python3 -m http.server 8000`, then open `http://localhost:8000/build/emu/`. It boots on TFM's START screen.
- **Final tests:** `tests/run_tests.sh` ALL HOST TESTS PASSED (5,973 checks ok, 0 failed); `tests/target_budget.py` 52 ok, 0 over. Web tests and backup tests pass. Screens rendered to `tfm-spec/built/` (33 PNGs plus `_contact-sheet.png`).
- **Size:** app 478,808 B (tfm-next 413,372: +65 KB: the two font families about +25 KB, the 12 faces, toys and screens the rest); RAM `.data+.bss` 61,736 B of 73,728 (+512 B); pool unchanged. The app region has room (it ends at 0x97000).

## A decision only you can make: the 8 scenes x 3 songs format (NOT done)
I did not change the project format. A scene is a 3,840 B storage object kept as two sectors (A/B, so a power cut loses nothing). 24 scenes need **184 KB** of flash, 16 (8 x 2) need **123 KB**, and the flash data region is full: projects 0x97000-0x9EFFF, the three 80 KB user sample slots 0xA0000-0xDBFFF, user presets, OTA staging, autosave, settings. Making room means giving up **two or more of the three user sample slots** (or shrinking the app region's headroom, which limits future growth). Without the sample slots even 8 x 2 does not fit. Pick one:
1. Take 2 of the 3 user sample slots (160 KB): room for 8 x 2 (123 KB) or 20 scenes; all 3 slots (240 KB) would fit 8 x 3.
2. Keep 4 scenes per song and add songs only.
3. Keep as is.
Because nothing changed, `web/fm1backup.js`, the editor's backup/restore and old backups work as before (their tests pass). What is ready for it: the SCENES screen draws 8 tiles (E-H dim: "E-H: NO FLASH YET"), SONGS draws 3 slots (2 and 3 say they need flash), the SONG list and chain are generic.

## What was built, per phase (one commit + push each; branch `tfm-ui`)
1. **Shell** (`5677c02`): `tfm_ui.c`, TFM's screen system: header 22 / footer 25 px, zones redrawn by signature, the full button map (spec 2), **Barlow Condensed + Silkscreen** (OFL, TTFs in `assets/fonts`, licences in `LICENSES/`, `LICENSING.md`), palette **TFM** (TFM AMBER) as the default, JAM as home, DEEP reaches Felucca's pages. Felucca's layers keep their key maps and logic; only a button's *tap* opens a TFM screen.
2. **START, DICE, SETUP, TIME** (`0c4eaeb`): as drawn. SETUP's rules (RANDOM / TYPE / one INSTRUMENT per track, KEY, SCALE, TEMPO range, TIMER default) are saved in 16 bytes of `favorites.factory[13]` (the settings record keeps its size; 0 = defaults). LINEUP and the setup code are gone from the device UI.
3. **JAM, MIX**: bumpers (ring, flash, glow, muted dashed), scene strip, MIX faders; REC records jam/fader moves, also on an armed track that is not selected.
4. **SOUND + 12 instruments** (`d19c...` series): the whole table from `TFM-INSTRUMENTS.md` (`tools/tfm_ins_spec.py` is its data, `tools/gen_macros_doc.py` writes the new `MACROS-TO-TUNE.md`): voicing, 8 multi-target macros (EXP, gated, SWITCH), 5 named presets each, no LEVEL macros; the 12 faces and toys; hold EDIT is the instrument picker.
5. **STEP, NOTES, DRUMS**: as drawn; G5 = REST; hold OCT+ the step sheet (its chips drawn on screen).
6. **SCENES, SONG, SONGS** (`4-scene storage`, see above): SCENES (hold ARP: go / copy / hold G5 to chain), SONG (K1-K4 each track's scene per section, OCT+ add, EDIT delete, PRESETS repeats, GLO again SOUNDS SCENE/CURRENT, PLAY plays the song), SONGS.
7. **FX, SPACE, ARP, CLOCK (+page 2), MOTION**: one template (big readout + four dials).
8. **Today's jam + acceptance:** the editor's "Send today's jam" button sends TILT's code of the UTC day (an 800-day table made from TILT's `daily.ts` under Deno) with editor command 100; START shows TODAY'S JAM (A3) until played once. RAM only. The acceptance walk of spec 7 is a test (`test_tfm_ui_acceptance`): no Felucca page at any step.

## Decisions I made
- **CLASSIC mode** (`tu_off`, build flag `-DTU_OFF_DEFAULT=1`): Felucca's own UI, used only by the tests so every Felucca check still runs unchanged (the host tests, the sanitizer builds and the emulator test run on CLASSIC; `web_classic(1)` is exported to the wasm for that). There is no menu row for it (SYSTEM is full). The old NEW SONG (LINEUP, setup code) still exists inside CLASSIC.
- TFM screens use fixed TFM colours; GREY and MONO palettes take their luminance. Felucca palettes remain choosable for the menu, DEEP and layers.
- A key written in STEP/NOTES while stopped writes at the cursor, armed or not (spec); playing: it plays or records live when armed.
- ARP tap turns it on as UP; G5 is masked only while held (a standing mask made layer buttons swallow taps).
- Defaults for the dice: a band (T1 drums, T2 bass, T3 chords, T4 lead).
- Macro mapping notes (the spec's target has no exact parameter): CHORD SUS2 / ADD9 -> DIA3 / DIA7; VOICE DROP2 / SPREAD / STACK -> INV1 / INV2 / +OCT; RATE 1/4T -> 1/8, 1/8T -> 8T; PIXEL JUMP 5TH / TRIAD -> arp MAJ / MIN, WAVE a chip waveform; THUMP's INSERT SOFT -> HARD at 60 left on SOFT; TONE and REL offsets, a few LFO routes and HAZE's HALL reverb (global) are not set. Macro values are starting points: tune by ear with `MACROS-TO-TUNE.md`.
- Felucca's QUANTIZE etc. untouched. The MENU (HOME held) is Felucca's, restyled by the palette only.

## Does not match the drawings, and why
- **Layers** (hold FX punch-in, SCL root keys, GLO live, ENV TILT, REC, SEQ step tools) keep Felucca's look: not drawn in the spec. Hold EDIT (picker) and hold ARP (SCENES) are TFM screens. The **NAME screen**, dialogs, count-in and the MENU are Felucca's, recoloured by the palette.
- **Toys:** THUMP LAB is drawn as in `SoundThump`; the other eleven toys are drawn from the instrument sheets' descriptions and the toy boxes of the HTML (geometry approximated, behaviour as specified); PIXEL's and VOX's DMD artwork is my own.
- The 8 x 3 format (above). SONGS slots 2-3 and scenes E-H are not usable.
- The SONG screen's "SOUNDS" toggle is on GLO again, repeats on PRESETS (spec silent).
- Text sizes are the drawings' halved (footer hints 8 px): small; check on the panel.
- The first tap of a layer button right after another layer closes is swallowed (Felucca's quiet window; a hand cannot hit it).
- Description lines on the faces are ellipsised when too long (the ellipsis glyph is faint).

## Not tested
Nothing was run on an FM-1. Audio behaviour of the new voicings and macros, the toys at real frame rates and the screens' redraw cost on the device are unverified; the redraw is zone-based and the toys redraw only their boxes, but `ui_render` has no timing for the TFM screens.

## Round 1 fixes (from Felix's emulator test)

One commit and one push per item, branch `tfm-ui`. Package to try: `tilt-fm-ui-r1-b594d9d.fwsc` (this folder).

| # | Item | Commit |
|---|------|--------|
| 1 | NOTES and DRUMS cursor | cc36204 |
| 2 | Tempo is easy to find | ef60559 |
| 3 | No dead ends | dd1965c |
| 4 | JAM shows the selected track | 7d584a4 |
| 5 | Macro knobs smooth | df0cbab |
| 7 | TYPE rules, RANDOM kits | 42b0fee |
| 6 | Bigger text | f9042a4, b594d9d |

### What changed

1. **NOTES, DRUMS cursor.** SELECT moves the step cursor one step at a time through every step, empty or not, across
   bars (the shown bar follows). PRESETS jumps a bar and puts the cursor on the same step in it. K1 is still
   next/previous note. An empty step reads `STEP 7 · EMPTY · KEY ADDS`. Footer: `SELECT STEP · KEYS ADD · EDIT DELETE`.
   Test: for 7, 32 and 64 steps, on NOTES and DRUMS, the cursor lands on every step 1..LEN going up and going down.
   *Side effect:* PRESETS no longer changes the track's sound on DRUMS (it flips bars, as on NOTES); the sound is one
   EDIT tap away on SOUND.
2. **Tempo.** JAM's footer says `SCL TEMPO`. CLOCK opens with K1 TEMPO lit (a ring round its dial and its name in
   reverse); the knob you turned last stays lit. SELECT never touches the BPM (tested).
3. **No dead ends.** One HOME tap leaves a dialog, NAME, DICE (back to where it was opened from: START, else the screen
   under it), SETUP (back to DICE), START and TIME (keep working). OCT- still does the same in DICE/SETUP. A screen
   button (SEQ EDIT GLO SAVE FX SCL LFO ARP) pressed in DICE or SETUP closes it and opens that screen. DICE and SETUP
   footers say `HOME BACK`. A test taps HOME on every screen, modal, dialog and NAME, and on a layer after its button
   is let go: always out in one tap (JAM is the one screen HOME toggles: MIX and back).
4. **JAM selection.** The selected bumper has a bright outer ring and its name and macro label filled in its track
   colour; the header reads `T2 · SQUELCH` (then the scene and bar); turning ALGORITHM puts the track name big in the
   bottom band for about a second. `song.sel` is in the JAM signature (it was missing: the old ring never moved).
5. **Macros.**
   - Each track keeps its eight macro places (0..127) in RAM (`tfm_mpos`, 4 x 8 B, plus 4 x 8 x 2 B of check values and
     4 B of flags: about 100 B of RAM, no format change). A turn moves the stored place and applies every target
     from it; the place is never read back from the first target while it is current. When a parameter is changed
     elsewhere (DEEP, a load, undo, MIDI) the place is read from the first target once and kept again. A preset
     load sets the places from the preset, or derives them once.
   - A switch steps once per two detents (turning back restarts the count).
   - JAM's knob turns each instrument's own continuous macro: THUMP PUNCH, SQUELCH CUTOFF, WOBBLER WOB, CHORDETTE WARM,
     RAZOR CUT, HAZE BLOOM, PIXEL SWEEP, TINE BARK, GLINT METAL, PLUCK BRIGHT, VOX VOWEL, WHEELHOUSE DRIVE (the table
     `TFM_JAMM` in `tfm_instr.c`). `TFM-INSTRUMENTS.md` (here and in `docs/`) and `MACROS-TO-TUNE.md` say so.
   - Test: all 12 x 8 macros: continuous ones go 0 -> 127 -> 0 by exactly 1 a detent, every target monotone; the
     switches step per two detents; JAM turns the named one.
   - Responsiveness: JAM redraws only the zone whose picture changed: nothing when idle, at most the bumpers' band
     (240 x 104 px) on a knob turn; header, scenes and footer are not redrawn (tested by the zone signatures).
     **Not measured on a device**: the emulator and host harness have no meaningful frame time for the panel, so there is
     no worst-frame figure to give. If the knobs still feel slow on the FM-1, the next step is to split the bumpers' band
     into one zone per bumper (about a quarter of the pixels a turn).
6. **Bigger text.** Barlow Condensed 11 / 13 / 16 / 24 px (was 8 / 10 / 12 / 19), Silkscreen 13 / 16 / 21 / 40 (was
   10 / 13 / 17 / 32); nothing under 11 px in TFM's own screens. All screens re-laid out: at most 3 footer hints
   everywhere (checked on every screen); what no longer fit moved into the screens' empty bands (JAM: REC, EDIT, SEQ, GLO,
   SAVE; DICE: PLAY, ENV; SONG: SELECT, PRESETS in the header; STEP: K1, SELECT); the FX chips fit the card; TIME no longer
   leaves a black strip. **Flash cost: +6.4 KB** for the larger glyphs (481,040 -> 487,444 B), RAM unchanged. The
   layout lint finds nothing on any of the 297 screens in the 16 palettes.
   *Not changed:* Felucca's own fonts (S 12 px and up; DEEP, the menu and the layers) already were 11 px or more.
7. **TYPE rules.** The picks were right (every TYPE roll has its type's bit, tested for 6 types x 4 tracks). What was
   wrong: a rule changed in SETUP did not roll that track, so DICE showed the old instrument under the new rule (LEAD
   over HAZE). Now a rule changed in SETUP rolls that track at once (a held track keeps its instrument); a changed
   key, scale or tempo range applies to the rolled setup as well. RANDOM never rolls a second drum kit: if another track
   has one (rolled before it, held, or because its rule asks for DRUMS or THUMP LAB) RANDOM picks from the rest.

### Tests, size
- `tests/run_tests.sh`: ALL HOST TESTS PASSED, 5,973 ok lines, 0 FAIL (eight new checks, `tests/ui_test_r1.h`, run in all
  three UI builds). `tests/target_budget.py`: 52 ok, 0 over. ui_render: 0 lint findings on every screen in the 16
  palettes.
- Size: app **487,308 B** (was 478,808 at the first report: +8.5 KB, of which the larger fonts +6.4 KB, the rest the
  macro places, JAM's hints band and the new rules); RAM `.data+.bss` **61,928 B** of 73,728 (+192 B); pool 345,068 of
  368,640 (unchanged). Fits the app region with room to spare.
- The 33 AMBER renders are redone in `tfm-spec/built/` (here, and in the repo under `tfm-spec/built/`), with a new
  `_contact-sheet.png`.

### Not done, and why
- The 8 scenes x 3 songs flash decision is still Felix's (the project format is unchanged: "keep as is").
- No device frame time (item 5): no device run in this session.
- Silkscreen's four sizes are my reading of "up the same way" (about x1.3); the Barlow sizes are as asked.
- PRESETS on DRUMS flips bars now (item 1 asked for it on both screens); the SPEC's "PRESETS changes the sound
  everywhere" now has two exceptions (NOTES and DRUMS).

## Round 2: 8 x 3 format

Felix's decision: the three user sample slots (240 KB) become scene storage. Branch `tfm-ui`; commits `d77d655` (steps 1 and 2: map, storage, migration, backup objects, in one commit because the migration tests live with the storage), `44c6645` (3: screens), `7c520d8` (5: going back), then the commit with this section (4: the web side, the budget lines, renders, report). Package: `tilt-fm-ui-r2-<sha>.fwsc` (Windows folder). **Nothing was run on an FM-1; nothing was flashed.**

### Flash map, before / after (full table: `docs/FLASH-MAP.md`)
| | Before | After |
|---|---|---|
| `0x97000..0x9EFFF` | 4 project slots (A/B sectors) | scenes 1-4 = song 1 A..D (same objects, not moved) |
| `0xA0000..0xDBFFF` | 3 user sample slots, 80 KB each | `0xA0000..0xC7FFF` scenes 5-24 (20 objects x A/B); `0xC8000..0xCDFFF` the 3 song records; `0xCE000..0xCFFFF` the format tag `TMS8`; `0xD0000..0xDBFFF` blank |
| everything else | user presets, FM6 patches, OTA staging, autosave, voice bank, settings | unchanged |

24 scenes x 2 sectors = 48 sectors; the old sample region is 60, so 12 sectors stay free. A scene is a project record as ever (FUN10, 3,840 B). A song record (globals, chain, MIDI channels, name; about 490 B in RAM for the three) holds what the song owns. Chain slot bytes keep the stored encoding of every older build (A..D 0..3, "-" 4) and add E..H as 5..8; in RAM "-" is now 8.

**RAM / playing.** Only the current scene is decoded into RAM; a scene is read from flash when asked. Switching scenes while playing is read-only (tested: the erase and program counters do not move through the cue, the flip and the following passes), flips on the bar line, and the scene left, if edited, waits in RAM (up to three) for the stop. A fourth edited scene is refused ("STOP TO SAVE"). **Deviation from the brief:** a SONG that plays still prepares the patterns of all the scenes it uses before PLAY (now up to eight instead of four: `chain_src`, the extra four in the pool); it does not stream sections from flash. To make room `app.ld` moved 6 KB from RAM to the pool (RAM 72K to 66K); the build's headroom checks pass (RAM 62,088 of 67,584 B, 5,496 spare of 4,096 required; pool 365,752 of 374,784 B, 9,032 spare of 8,192 required). Flash: app 486,072 B (487,308 before: the sample code left).

**Removed:** the editor's sample commands (BEGIN/WRITE/END/ERASE are refused with no reply, INFO lists no slots), the backup of sample slots, the slice store (SLICE's slices kept in the slots), the boot scan of the sample slots, the editor's Samples tab with its recorder and trimmer. The SAMPLE / GRAIN / SLICE engine code that plays a user slot is still in the tree and is now permanently empty (the SLICE render became the sine fallback, which is why `tests/target_budget.py` lost its `slc_rev` line and `slice_render` has no loops: those two budget lines were updated for that, nothing else).

### Migration, first boot (tested in `tests/persistence_test.c`)
Tag missing: the whole region `0xA0000..0xDBFFF` is wiped (blank sectors skipped, watchdog fed), song 1's record is written from scene A (else the first stored scene), the tag is written last. Scenes A..D are never touched. Results: A..D byte for byte as before; the chain (with a silent lane) and tempo decode identically; E..H and songs 2, 3 empty; the sample region holds only our records or blank sectors; the second boot writes nothing. **Power cut:** every one of the 66 erase/program calls of the migration was cut in turn (the cut call applies half a program or half an erase): at the cut the old scenes are there, and after the next boot the migration has finished, 66 of 66. A flash that cannot erase leaves A..D usable and does not hang.

### Screens
SCENES: all 8 tiles live (C4..C5 go there; "E-H: NO FLASH YET" is gone), header "SCENES · SONG n". SONG: K1..K4 step A..H then "-"; the song plays scenes E..H in the right lane (tested). SONGS: three live slots with name, scene count and length; OCT+ loads, SAVE opens NAME and stores the song now into the slot (a dialog first over another song), GLO then pick then GLO copies, EDIT clears; every dialog has NO picked, HOME and OCT- leave it. The settings remember song and scene (one byte; older settings restore into song 1). Renders: `tfm-spec/built/ui_scenes`, `ui_song`, `ui_songs`, plus `ui_song_overwrite`, `ui_song_copy`, `ui_song_clear`, `ui_songs_copy`; 301 screens x 16 palettes, 0 lint findings, 0 alignment errors.

### Backup (web)
`web/fm1backup.js` format v2 (33 objects: live music, settings, presets, FM6 objects, 24 scenes ids 64..87, 3 songs ids 96..98). v1 archives (13, 12, 11 objects) still read; restored onto the new firmware their slots 2..5 go into song 1 A..D, song 1's record is cleared, samples are skipped and reported; onto an old firmware a v1 archive restores exactly as before. Corrupted files (bad CRC, wrong order or count, bad song magic, oversize) are refused before the first write; a v2 file on an older device fails with a message and writes nothing. `node web/test_backup.mjs`: 56 ok. `node web/test_web.mjs`: 250 ok. Device side (`tests/backup_test.c`): scenes and songs round trip, length and CRC checks, a flash error keeps the scene, the restored live music lands in the song and scene the restored settings remember.

### Going back (tested; `docs/tilt/FLASHING-CHECKLIST.md` 4a)
An older firmware finds no `FSMP` header in the three sample slots and reads A..D as its four projects (one whose chain uses E..H shows empty there; the data stays). If it uploads a sample over the region: the scenes and songs in that slot read as empty here; if the tag goes, the next boot migrates again (A..D and song 1 come back, the rest is empty) and then stays quiet; with the tag intact nothing migrates. Stock V15 does not use the region. **Back up first in the editor.**

### Tests
`tests/run_tests.sh`: 6,053 ok lines; the only failure was `target_budget` (`slc_rev` gone, `slice_render` loops gone: fixed as above, now 0 over). Every older test kept its checks; the ones that encoded the old format were ported (project slots read through the scene layer, the editor's wire for song slots, backup ids), the sample-upload checks became refusal checks.

### Not done / limits
- The editor's Song panel still edits sections over A..D only: the A..H lane protocol (ops 4..7) is implemented and tested in `web/editor.html`, but there is no UI for it yet. `web/index_pkg.html` (the installer page) still says samples are backed up (`stockBackup`).
- No device measurements: first-boot time (up to 60 sector erases, typically none to a few seconds), the cost of a flash read at a scene switch (up to 15 read windows of 256 B with IRQs off, like every flash read), nothing about audio on hardware.
- A SONG preloads its scenes (up to eight) rather than streaming them.
- The migration's song 1 record comes from scene A (the song-level values every scene stores); if the four scenes disagreed, A wins.
- The web restore sends a begin + abort of scene A as a probe before a v2 restore; on the device that stops the transport (a restore needs it stopped anyway).

### What Felix must do before flashing a real FM-1
1. Back up in the editor of the build you have now (scenes, songs, presets, settings).
2. Read `docs/tilt/FLASHING-CHECKLIST.md` sections 4 and 4a; keep V15 handy as ever.
3. The first boot takes longer and must not be interrupted by pulling power (a cut is safe, but let it finish).
4. Any sample you loaded into USR1..3 is wiped by the migration.
