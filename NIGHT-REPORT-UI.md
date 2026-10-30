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

## Round 3 (Felix's emulator notes)

Branch `tfm-ui`; one commit and push per item: `08de033` (1), `c2d85ec` (2), `b1b285c` (3 and 4, together: they share SETUP and DICE), then the commit with this section (5). Package: `tilt-fm-ui-r3-<sha>.fwsc`. Nothing was run on an FM-1.

1. **NOTES.** K1 steps through every step (empty ones too, the bar follows); SELECT scrolls the roll by pitch rows; K2 PITCH, K3 LENGTH, K4 VELO act on the note at the cursor and are dim on an empty step; the info line reads "STEP 7 · EMPTY" or "STEP 7 · D3 · 1/16". A tap on OCT+ grabs the note (outlined white, header MOVING), K1 moves it in time with its ties (not past another note or the pattern ends), K2 its pitch, a second tap drops it; a hold is still the step sheet (a press longer than 0.45 s is never a grab). Footer K1 STEP · OCT+ GRAB · EDIT DELETE; DRUMS footer K1 STEP. Side effects: on NOTES the OCT buttons no longer shift the octave; the root and scale text left the NOTES header (the REC mode took its place). Test: every step reached, grab / move / drop keeps length, velocity and the number of notes and ties.
2. **REC decides the keys** on STEP, NOTES and DRUMS: REC off the keys only play (stopped or playing), REC on and stopped they write at the cursor, REC on and playing they record live. The header says PLAY ONLY / WRITE STEPS / REC LIVE (red when on). The round 0 decision is reversed. The DRUMS header lost its "BAR n of m" (no room beside the mode). Tests for all three modes on all three screens.
3. **New song.** PLAY on DICE makes a fresh song in the first empty slot: scene A the rolled sounds with empty patterns, B..H and the chain empty, scene A stored; the song it leaves is stored first and stays. All three slots used: SONGS opens in "REPLACE WHICH?" (SELECT picks, OCT+ replaces, OCT- or HOME cancels back to DICE). SETUP's NEW SONG row (FRESH default, KEEP SCENES) took the info-only TIME'S UP row's place; KEEP SCENES re-rolls the sounds only and keeps patterns, scenes and chain. The old "PLAY AGAIN: CLEARS ALL" step is gone in TFM (nothing is cleared). Tests: fresh song empty A..H, the old song untouched, the picker, KEEP SCENES.
4. **Timer.** 10:00 by default: the stored byte 0 now means 10 (older settings get it), 1..4 are 5, 10, 15, 20 as before and 5 is OFF (SETUP still selects OFF). DICE shows it and K4 there sets this song's timer (OFF 5 10 15 20) without touching the default; a new song starts the countdown, which JAM's header shows. Tests included.
5. **In key.** Done in the tables (`firmware/src/tfm_instr.c`, `tools/tfm_ins_spec.py`, `MACROS-TO-TUNE.md` regenerated, rule in `docs/TFM-INSTRUMENTS.md`): a. CHORDETTE CHORD = AUTO, AUTO 7, MAJ MIN 7 MAJ7 MIN7 SUS4 POWER, voicing on AUTO (the extra presets HOUSE ORGAN, DUB CHORD moved to AUTO / AUTO 7); the chord's real name is not shown. b. PIXEL: SWEEP range 0..25 and 0 in every preset but WARP ZONE; COIN replaced by DUTY; JUMP OFF / OCT / 5TH; JAM macro TONE. c. RAZOR: LD_PIT voicing 0, VIB 0..2 steps. d. HAZE: DRIFT 0..2 steps, gated (0 below 70 %) (one LD_PIT step is about 19 cents, so 50 stays at 0 and 100 is 2 steps, about 38 cents; DARK DRIFT and TAPE PAD lost their LD_PIT). e. GLINT: FM6 slot 1 as its base, RATIO -4..+4. g. skipped (THUMP LAB TUNE not mapped to a pitch). **The measurement** (`test_r3_pitch`, tests/ui_test_r3.h): a host render of root (C4) and fifth, one beat at 120 BPM, autocorrelation after the first 1/16, every preset and every continuous macro at 0 and 50. **What it shows, honestly:** SQUELCH, CHORDETTE and TINE pass all five presets; WOBBLER, RAZOR, WHEELHOUSE 4 of 5; HAZE and PIXEL 3 of 5 (so those eight instruments meet "at least 3 presets", and are asserted); **GLINT (1 of 5), PLUCK (2) and VOX (0) do not**, and many continuous macros miss at 0 or 50 (the full list is printed by the test: `tests/ui_test` stderr lines `PITCH ...`). Several misses are ~500 or ~275 cents, which looks like a measurement problem (octave/harmonic confusion, or a sound with no single fundamental) as much as an instrument one; I did not have the budget to tell them apart. GLINT, PLUCK and VOX are reported, not asserted, so the suite is green but item 5 is **not** done: the next step is to look at the printed list sound by sound, with a better estimator (YIN) and by ear.

**Not done / limits:** item 5 as above (instruments still out of tolerance; chord-name display; THUMP TUNE); no device measurements; the editor's Song panel A..H UI from round 2 is still missing.

## Round 4 (the 2x drawings: JamA, NotesV2, Bars, Sound*)

Branch `tfm-ui`; one commit and push per item: `f5f2be3` (1), `cd68df8` (2), `6923690` (3), `abe1450` (4), `091d54b` (5; `0f6b94d` just before it holds only the removal of `tfm_toys.c`, a staging slip of mine, not rewritten), `4a26523` (6), `d9295c0` (7a), `3bc4165` (7b). Package `tilt-fm-ui-r4-<sha>.fwsc`. Nothing was run on an FM-1. Where an item and a drawing disagreed the item won (said below).

**Flash and RAM per item** (the app image, RAM = .data + .bss of 67,584 B, the pool of 374,784 B): round 3 build 488,084 B / 62,088 B / 365,752 B. Item 1: 488,192 (+108) / 62,104. Item 2: 488,424 (+232) / 62,104. Item 3: 490,728 (+2,304) / 62,072. Item 4: 493,464 (+2,736) / 62,072. Item 5: 495,500 (+2,036) / 62,040. Item 6: 497,772 (+2,272) / 62,040. Item 7: 497,772 (data and tests only). The pool did not move (365,752 B). Total +9,688 B of flash, RAM 48 B less; headroom RAM 5,544 B (the build asks 4,096), pool 9,032 B (asks 8,192).

1. **Blank scenes play silence.** `chain_prepare` no longer refuses: a lane on a scene with nothing written (or one that will not load) becomes `CHAIN_SILENT` in the copy the song plays from (the song as written is untouched), so no "PATTERN C EMPTY" anywhere; the quick chain (hold G5 and a scene key) no longer says SCENE EMPTY. A section lasts as long as its longest track as before, a section where every lane is silent or blank is one 4/4 bar times its repeats (SONG's "x2 / 2 BARS" says so too). On SONG a blank scene's cell is dim and the row says "C EMPTY". The lane knob runs `- A B .. H` ("-" first, clamped at both ends, no wrap); the stored chains keep their meaning (the wire and the project encoding did not change). Tests: A, B, C (blank), A plays through with C silent for its length; an all-blank section is 1 bar x repeats; the knob order and its clamping; the old tests that expected the refusal now expect the silence.
2. **REC state on every header** but START, DICE and TIME: four dots in the track colours (the selected one larger), a red ring round the dot when armed, a filled red disc when recording (armed and playing). They sit at the left of the header where the track badge was (the badge went: the larger dot is the selected track), the NOTES bar tabs went (the strip of item 4 replaces them) and a few hint words were shortened to make room ("PRESETS reps", "autosaved", SCENES "S1"); no text became smaller. Test: the three states on JAM, MIX, SOUND, NOTES, DRUMS, SCENES, SONG, SONGS, FX, SPACE, ARP, CLOCK, CLOCK 2, MOTION (pixel checks), the larger selected dot, and arm on SEQ then HOME shows it on JAM.
3. **JAM v2** (JamA + JamB's meters). The display is a 5 x 7 dot matrix as TILT's timer and splash: the TIMER big (pitch 4) with the dots under it draining (red for the last minute), SCENE and BAR n/m (pitch 2) and KEY SCALE TEMPO beside it (the selected track's key); timer OFF: the song's name and the scene take its place. The band is four cabinets in each instrument's own material (`TU_MAT` in tfm_ui.c, shared with the SOUND faces): track colour strip, name, the jam macro as an arc dial with its value, macro name, T1-T4 on a chip, the REC dot, a 14-LED level meter on the left edge (up at once, down one LED a frame), a note flashes the dial, muted = dimmed with MUTED across it, the selected track outlined white (header "T3 . SQUELCH"). Scenes A-H are insert lamps (the arrow): the one playing amber and glowing (a song playing: the scene of its section), used dim, empty dark, the queued one outlined and blinking. Deviations: the footer is the only hint row now (the REC / EDIT / SEQ / GLO / SAVE band under the scenes and the big name for a second on ALGORITHM went, as the drawing has neither); gradients are bands, textures hairlines or rules. Tests: selection, mute, REC dot states, timer off, timer on with the draining dots (half and the red last minute), the queued lamp blinking.
4. **NOTES v2** (NotesV2). A strip of every bar of the pattern as thumbnails (the bar shown outlined amber), 15 rows of the song's scale notes only (about two octaves; the root rows tinted and named; a note out of the key sits on the scale note under it), a violet frame round the cursor's column, the note at the cursor outlined white with its name over it, the playhead, a held key marked at the left edge, up / down counts for notes out of the rows. K1 steps every step, K2 pitch (on an empty step it inserts the last note used, REC off too), K3 length, K4 velocity; PRESETS = ZOOM (1 bar, 2 bars, the whole pattern; no longer moves the bar there; DRUMS keeps its bar); the rows centre on the shown bar's notes whenever the view changes (K1 over a bar line, zoom, opening NOTES, track or scene change) and, when they span more rows than fit, keep the cursor's (or the nearest) note in view; a GRAB scrolls with the note. REC on and stopped, with a key held: K1 sets the new note's length and the cursor lands after it on release. Footer OCT+ GRAB, EDIT DELETE, PRESETS ZOOM. A bar is 16 steps here (as NOTES always had it). Tests: every step and note reachable and in view at all three zooms on a pattern spanning five octaves in D minor, the zoom keeps the cursor, the rows follow the key (D minor, then G major), the strip, the up count, K2 inserts, the REC-hold length, a GRAB scrolling through twelve K2 turns.
5. **SOUND faces v2** (Sound*.dc.html). Each instrument is its own hardware: THUMP LAB pads (one per lane, lit on its hits) over a panel with four small knobs; SQUELCH knobs and an ACCENT lamp (lit on accented steps); WOBBLER's LED ring = WOB (the lit ones pulse with the LFO), the RATE tiles (the lit one is the rate), GROWL and CUT as small knobs; CHORDETTE's LCD with the chord being played by its real name and notes (Dm7, D F A C: from `chord_make`, named from its intervals) and the preset list; RAZOR faders; HAZE faders (glow with the level); PIXEL's screen drawing the WAVE (50, 25, 12, triangle, noise) with its buttons; TINE's tremolo lever (OFF / MONO / STER by the TREM knob's thirds); GLINT's gold knobs and stars that twinkle and flare on notes; PLUCK's string that vibrates on each note and dies with DAMP; VOX's face (mouth opens with the sound, shaped by VOWEL; eyes glow on notes) and speaker with the vowels; WHEELHOUSE's drawbars (STOPS registration, SUB / BODY / TOP scale their groups) and the ROTOR tiles. The drawn controls are the K1-K4 macros and move live; MACROS 2 shows the other four in the same style (WOBBLER and WHEELHOUSE, whose page 1 is the face itself, show the generic row there). Footer: EDIT PAGE 2, PRESETS SOUND, HOME JAM (THUMP LAB: KEYS PLAY PARTS, EDIT PAGE 2, SEQ GRID; EDIT a third time still opens DEEP). `tfm_toys.c` is gone, and the classic MACROS page draws no toy. Deviations: Barlow and Silkscreen only (no serif, italic or the TILT display faces); gradients are bands; the drawbar registrations are a table of ten of my own; the page-1 pads are the page-0 pads (they are not macros). Tests: pad lights and goes out, the accent lamp, the WOBBLER ring and tile move, the chord name Dm7 and its notes, five different PIXEL waves, the tremolo lever, the string, the mouth and eyes, the drawbars and rotor tile, HAZE and GLINT controls, MACROS 2 differing from page 1 and moving.
6. **BARS replaces STEP.** SEQ opens NOTES (DRUMS on a drum track), SEQ again BARS, again NOTES / DRUMS; STEP and every reference to it are gone (`TU_STEP`, `tq_step_*`, the spec's button map and its sections: tfm-spec/TFM-UI-SPEC.md, in the Windows folder). One row per track (badge, instrument, "2 BARS x2"), each bar a block with a mini picture of its notes (a drum track by lane); bars past a track's length are dashed repeats with the circular arrow up to the scene's length; the selected bar outlined white. K1 TRACK, K2 LENGTH, K3 BAR, K4 SHIFT (a step, wrapping; the net shown), OCT+ tap DOUBLE, EDIT clears the bar, hold OCT+ COPY (K3 the target, OCT+ pastes, OCT- cancels); each change is one undo (SAVE held, `tl_undo_take`: the last change). **Where the item cannot be met:** the pattern holds 64 steps, so a track is at most 4 bars of 16: K2 steps 1, 2, 4 (8 appears where the bar is 8 steps or less: never today) and DOUBLE stops at 4; the drawing shows four bars too. Tests: SEQ's order on a melodic and a drum track, lengths grow by repeating and shrink keeping the first bars (steps checked), DOUBLE, K3's limits, clear, shift with its wrap, COPY by hold, paste, cancel, undo after each, K1's track, the scene length.
7. **Leftovers.** a. *In key.* `test_r3_pitch` is YIN now (first dip of the normalised difference under 0.15, parabola refined, a window of 2048). **The 500-cent misses of round 3 were the measurement:** the reverb, echo and chorus buffers rang on from the first note into the second (HAZE, CHORDETTE, RAZOR, VOX, WHEELHOUSE); they are cleared between notes now and the misses are gone. WOBBLER's remaining ~45 cents was the wob (an amplitude wobble the estimator reads as a bias): measured with it at 0 (WOB itself measured as it is). Real fixes: GLINT's RATIO is in tune at its -2 place (30 %), the presets sit there now (3 to 4 of 5 in key, was 1); PLUCK's NYLON, HARP and BANJO-ISH moved to places where the string is in tune (3 of 5, was 2). **Asserted now for all eleven melodic instruments**: at least 3 of the 5 named presets in key (SQUELCH 5, WOBBLER 5, CHORDETTE 5, RAZOR 5, HAZE 5, TINE 5, WHEELHOUSE 5, VOX 4 + 1, GLINT 3 + 1, PLUCK 3 + 1, PIXEL 3 + 2: the + n are presets with no single period, noise and crush, which count as measured). THUMP LAB is a kit: no pitch. Left, and why (docs/TFM-INSTRUMENTS.md): PIXEL's SWEEP is a pitch slide by design; GLINT's bells show inharmonic partials with low modulation or a long decay (DECAY, ATTACK, SHIMMER at 50: 100-120 cents); PLUCK's loop filter shifts the pitch with BRIGHT and DAMP in a few corners; VOX's choir and noise have no single period. The list is printed by the test (`PITCH ...` lines). MACROS-TO-TUNE.md regenerated. The by-ear check is still yours. b. *The editor's SONG panel* has the lanes A-H: a section is four menus (T1-T4: - A B .. H) and its repeats, written by SONG op 5 and read by op 4; the capability is read from INFO (`57 01 4`, parsed now); firmware without it keeps the one-slot rows; the mock device says so too, and no longer rejects a blank scene before PLAY. The installer page no longer says samples are saved. Tests in web/test_web.mjs.

**Suite:** `tests=0`, no FAIL lines, `target_budget` 0 (see below). The 2x drawings are the spec; the screens are re-rendered into `tfm-spec/built/` (and the Windows folder), lint 0 findings in 16 palettes.

**Not done / limits:** nothing on an FM-1; item 7a's by-ear pass (the printed list sound by sound); K2's "8 bars" (64 steps); the editor's panel was checked by protocol tests, not clicked in a browser.

## Round 5 (live arranging from JAM: JamFinal*, PerformFinal)

Branch `tfm-ui`: `0ae7042` (items 1-3, one commit: they share one file and one screen), `661ac80` (item 4). Package `tilt-fm-ui-r5-<sha>.fwsc`. Nothing was run on an FM-1.

**Flash and RAM:** round 4 build 497,772 B / RAM 62,040 B / pool 365,752 B. Items 1-3: 502,780 B (+5,008) / 62,344 (+304). Item 4: 502,608 B (+4,836 from round 4) / 63,128 (+1,088). RAM headroom 4,456 B (the build asks 4,096; the first cut of item 4's note log took 1,536 B and failed that check, so it is 24 notes a track at 8 bytes now). Pool unchanged.

1. **The black keys launch scenes.** F#3..A#4 = scenes A-H (the white keys play the selected track as before; the black keys play nothing on JAM: `kb_map` is silent for them there). A tap queues the scene (its lamp and key light blink); it starts when the scene playing has finished its loop (seq.c `scene_tick` now waits for the longest track's loop end, its full length, and no longer the next bar: for scenes of one bar nothing changed); the same key again cancels, several taps: the last wins, the playing scene's key keeps it looping. Sounds follow the scene (the staged scene carries its engines, presets, values and FM6 patches). OCT+ and a scene key copies the playing scene there and queues it (while playing it goes to a RAM image, flushed when stopped, as every edit of a scene); OCT- and a scene key clears it (CLEAR C? with Cancel the default, undoable by the scene undo). D#5 held = FILL; C#5 dark. The bottom row is the lamps grouped 3 2 3 2 1 with each key's name under it and the FILL / CHN pills (the playing scene lit amber with a glow, the queued outlined and blinking, used dim, empty dark); the FM-1's key lights mirror them (playing solid, queued breathing, used at the mid level, F#5 / D#5 dim). The display: the TIMER, "SCENE B > C", "C IN 2 BARS . LIVE" (the bars to the loop's end), the key. Footer BLACK SCENES, HOLD F#5 CHAIN, HOLD ARP PERFORM. **A change you will notice:** JAM takes OCT+ and OCT- for these jobs, so they no longer shift the octave on JAM (they do everywhere else).
2. **CHAIN on F#5.** Held, the scene keys tapped in order are sections (the same scene twice in a row one more repeat); the display spells the taps out (boxes, "LET GO: SET CHAIN", "OCT- UNDO A TAP"); OCT- takes the last tap; let go, the chain replaces the song's chain; OCT- alone afterwards brings the old one back (one level; again, the new one). Let go with no taps: back to the chain if there is one (it stops the live music and plays it). PLAY plays the chain after a CHAIN, the selected scene once a scene key made it live. While it plays the display shows the chain as boxes (the playing one lit, the played ones outlined, a window of six), "CHAIN 4/8 . BAR 3/4", and the lamps follow its sections; a scene key tapped meanwhile drops to live (the chain stops, the music goes on, that scene queued for the loop's end; the working pattern is the scene the chain started from). Blank scenes in a chain play silence (round 4). The SCENES quick chain (G5) stays.
3. **The jam is remembered.** Every scene launch, live or chain, is logged with the loops it played (the clock track's wraps), 64 entries, the oldest dropped. KEEP AS A SONG (TIME) turns the log into the song's chain (consecutive launches of one scene are one section, repeats = loops played, at least 1, at most 16; sections cap at 16, the oldest dropped). **Not done:** the SONGS entry "SAVE JAM AS SONG" has no button yet (`jl_keep()` is there; SONGS' own SAVE still saves the song as it is): say which button you want for it.
4. **Hold ARP = PERFORM** (tap stays the arpeggiator page; the old SCENES layer is gone, its screen is no longer reachable). White keys ROLL: the held note repeats at K1's rate (1/8, 1/8T, 1/16, 1/16T, 1/32) from the audio side (seq.c `roll_tick`), recorded as a ratchet of the step when REC is on and playing (the hits the step holds at that rate; a drum track records its part the same way). F#3 G#3 A#3 C#4 mute T1-T4 at once (lit = playing; the cabinets turn into ON / MUTED panels). D#4 held = FILL. F#4 = CAPTURE: the last 1, 2 or 4 bars (K2) you played on the selected track, REC off too, from a rolling log of 24 notes a track (live keys and rolls), written into the pattern aligned to where the loop is now (stopped: from step 1), the bars it replaces cleared first, one undo (SAVE held). G#4 = MUTATE: the pattern re-rolled in key (TILT's RANDOM, as STUCK) and / or the sound (K3: PATTERN, SOUND, BOTH), one undo. The display is violet: PERFORM, ROLL rate, CAPTURE length, MUTATE mode; the pills M1-M4, FIL, CAP, MUT sit on their keys; footer WHITE ROLL, K1 RATE, LET GO BACK. The spec's button map is updated (tfm-spec/TFM-UI-SPEC.md, Round 5 section). Limits: the roll's first hit is the key's own, the repeats follow the audio clock whether or not the transport runs.

**Tests** (tests/ui_test_r5.h): the launch waits for the loop end (a 2-bar scene: still A after one bar, B after two), cancel, last wins, tap playing, FILL, C#5, copy and clear (asks first), the key lights, black keys silent; CHAIN taps, repeat, OCT- tap, replace, undo, PLAY plays it, drop to live, blank in chain, back to the chain; the log to a chain (merging, capped, KEEP); roll rates in the right ratios and a recorded ratchet, mutes and their lights, FILL, CAPTURE of 1 / 2 / 4 bars with REC off and its undo, MUTATE in key and its undo, leaving PERFORM. Old scene-layer tests run in CLASSIC and are unchanged. Renders: ui_jam (live, B > C), ui_jam_chain, ui_jam_write, ui_perform.

**Suite:** see the numbers below. **Not done / limits:** SAVE JAM AS SONG's button; nothing on an FM-1; the roll and capture are checked on the host's audio clock, not by ear.

**Suite (round 5):** tests=0, no FAIL lines, target_budget 0 (the first run failed the audio ISR cost, +18 % over; fixed in 727aab8: a volatile read per key in the key loop was the cost, now +4 %). Final build: 502,680 B, RAM 63,128 B (headroom 4,456 B), pool 365,752 B.

## Round 6 (ROUND-6.md: START v2, NEXT BALL, scene keys, levels)

Branch `tfm-ui`, one commit per item: `1f0fb6e` (1), `ad37ad8` (2), `a9590f9` (3), `d69bb62` (4), `784dac4` (5), `228e1af` (6), `6e33938` (7), `134a6fe` (8), `943b3e3` (9), `80f90d9` (10). Package `tilt-fm-ui-r6-<sha>.fwsc`. Nothing was run on an FM-1.

**Flash and RAM** (app image / RAM .data+.bss; pool 365,752 B throughout):

| build | app | RAM |
|---|---|---|
| round 5 | 502,680 | 63,128 |
| 1 SONG plays unsaved edits | 502,748 (+68) | 63,128 |
| 2 EDIT = MACROS 1/2, DEEP in MENU | 502,948 (+200) | 63,128 |
| 3 dB meters | 502,332 (-616) | 63,144 (+16) |
| 4 preset audit | 502,332 | 63,144 |
| 5 START v2 (TODAY'S JAM gone) | 502,408 (+76) | 63,144 |
| 6 NEXT BALL | 502,396 (-12) | 63,144 |
| 7 scene keys: hold to copy | 502,548 (+152) | 63,160 (+16) |
| 8 SAVE JAM HERE | 502,776 (+228) | 63,160 |
| 9 chord order, labels | 502,876 (+100) | 63,160 |
| 10 levels | 503,268 (+392) | 63,224 (+64: a level per voice) |

1. **SONG plays the scene as you hear it.** Before the chain plays, the scene in the RAM image that has unsaved edits (or was never stored) is kept into its slot first (`scene_keep_live`, tfm_live.c, through a pointer from project.c `chain_prepare`; CLASSIC unchanged). Test: an edit made on JAM is what SONG plays.
2. **EDIT toggles MACROS 1 / 2** on SOUND (the footer says EDIT PAGE 1 / PAGE 2); **DEEP EDIT** is a MENU row (AUDIO tab, after TIME SIG: OCT+ opens DEEP for the selected track; the row shows T1..T4). HOME on DEEP leaves it.
3. **dB meters.** Every meter is -48..0 dBFS over 14 LEDs (8 * log2 of the track's peak, once a frame: fast up, slow fall, the peak dot held ~1 s): green under 10, yellow 10-12, red 13. MIX and the cabinets use it; MIX's flash fires on a jump of 3 LEDs.
4. **Preset audit.** A host render of each named preset (a phrase from a clean start, fixed tempo and random seed) and six features (brightness, its change, attack, sustain, motion, level); presets nearer than 0.60 sound like one. Before: THUMP WAREHOUSE / DISCO 0.36, GLINT ICE / CHIMES 0.41, PIXEL 1UP / HIGH SCORE 0.44, PLUCK KOTO / HARP 0.55, TINE SUITCASE / CHURCH BASEMENT 0.33. Moved DISCO, HIGH SCORE, MUSIC BOX, TEMPLE, ICE, CHIMES, ROBOT BELLS, HARP, CHURCH BASEMENT (GLINT and HARP to places that stay in key). After item 4 all >= 0.63; after item 10 all >= 0.65 (the audit hears the presets' own sound: item 10's mix trims are level alone and are off while it renders). docs/TFM-INSTRUMENTS.md "The named presets (round 6)". **Note:** CHURCH BASEMENT's new places were left out of item 4's commit by mistake; they went in with item 10 (and moved again there: see 10).
5. **START v2.** WAIT: the last game asleep (its band, name), PLAY continues it, OCT+ rolls, SAVE opens SONGS. SPIN: the reels stop one by one left to right (600 ms + 300 ms each; a held one never spins). ROLLED: F3-B3 hold / release a player (HELD chip), OCT+ re-rolls the unheld, PLAY starts the new song and the clock, OCT- back to WAIT. ENV from anywhere opens START rolled; ENV again SETUP (and SETUP's HOME back to START). **TODAY'S JAM is gone:** the START row, tilt.c's decoder and table, editor command 100 (no reply now, as any unknown command) and the web editor's button.
6. **NEXT BALL.** TIME is PLAY IT BACK · EXTRA BALL · KEEP WORKING · NEXT BALL. NEXT BALL saves the jam as the song (the jam log as its chain, as KEEP AS A SONG did) and opens START mid-roll; PLAY there makes the next song (an empty slot, or asks which to replace, Cancel the default). KEEP AS A SONG is no longer a TIME choice.
7. **Scene keys: hold to copy.** A scene key launches on its RELEASE (it waits for the loop's end anyway). Hold one and tap another: the held scene is copied there (any scene; CF_SCENE_OVR asks when the target has something, Cancel the default; the scene playing cannot be the target, as before). Hold one + OCT-: clear it (asks). **OCT+ / OCT- alone shift the octave on JAM again.** F#5 held + OCT- takes the last tap; with no taps it brings the chain before back (round 5 had it on OCT- alone); letting go of F#5 after that undo plays nothing.
8. **SAVE JAM HERE.** On SONGS, SEQ on the slot picked: the jam played becomes the song's chain, then it is saved there as SAVE does (a song there: asked first, Cancel the default; then the NAME screen). No jam played yet: says so. The hint line: SEQ SAVE JAM HERE.
9. **Small fixes.** CHORDETTE's LCD lists the chord's notes up from its root (C E G), whatever the voicing: the "C G E" was the OPEN voicing's real order (the E an octave up); an inversion now keeps its root's name too (it named the lowest note before). JAM's and PERFORM's key names: only under the first lamp of each group (F#3, C#4, F#4, C#5, F#5) — eleven names 19 px apart touched; PERFORM's FIL / CAP / MUT are the word over a colour bar (three letters overflowed a 16 px disc). Checked on the 240 px render; 0 lint findings.
10. **Levels** — the tables are in docs/TFM-INSTRUMENTS.md "Levels (round 6)". Host measurement (BS.1770 style: K-weighting re-derived for 44.1 kHz, gated integrated + short-term), a part in each role at the default level. Before: drums -22, basses -21, pads -11, leads -10 LUFS; PLUCK's NYLON / MUTED / BANJO 25 LU under HARP. **The main finding:** the master limiter (from -5.2 dBFS) was making the loudness: THUMP LAB peaked at +9 dBFS through it, the pads were squeezed. Now a trim per instrument and per named preset rides with LEVEL (`fx.c trk_lvl`, `tfm_instr.c TFM_TRIM`), stopping where the part alone peaks at -2 dBFS before the master: pads, leads, bells -18 LUFS, basses -16.5 / -17.5, drums about -18 (level with the rest, not above: they are peak-bound), every named preset within 1.5 LU of its instrument's median except **PLUCK NYLON (-2.6) and MUTED (-1.6)**, peak-bound by the pick; PLUCK as a whole sits at -21. **Key tracking of level** per voice (`TFM_KT`: 0.5-3 dB per octave above each instrument's home range, from the measured rise). **THUMP LAB's low end:** SMACK drove the kit's TONE, which also holds the kick's sweep up, so the kick's body was at ~160 Hz; TONE is now fixed low (energy under 100 Hz 56 %, was 33 %), PUNCH is the kick lane, accent and drive (it no longer fights TUNE), louder hats; on its own the kick is within 3 LU of the snare and the hats. Nothing in the track or master chain cut the lows. **Macros** re-checked at the new levels (with the limiter out of the way the real swings showed): the ones that brighten, resonate, drive or add space stay within 5 LU over the turn (27 macros changed: the list is in the doc). Left: length / movement macros, and RAZOR SWEEP. **A limit worth its own change:** several macros of one instrument write LEVEL as their compensation, and the last one moved wins (it was so before). The in-key test passes. **This changes how loud things are:** pads and leads are ~7 dB quieter than before, drums and basses ~4 dB louder; the whole band has headroom now. Worth a listen before tuning further.

**Tests:** tests/ui_test_r6.h (new; included after round 5) and round 5's file: the preset audit, START v2 (the reel order, OCT- back, SAVE, ENV from anywhere), SAVE JAM HERE, the chord order, the levels (every named preset in its range, the instruments matched, no part over 0 dBFS before the master, key tracking, the kit, the kick's low end, the macros), SONG plays unsaved edits, the meters; round 5's scene-key and CHAIN tests changed to the new keys (hold + key copies, hold + OCT- clears, OCT+ / OCT- octave on JAM, F#5 + OCT- undo). `R6AUDIT=1` / `R6LOUD=1` on the ui_test binary print the tables.

11. **Standing rule in CLAUDE.md** (`f5ef1df`, ROUND-6.md item 11, added there at 13:44): the repo now has a CLAUDE.md: every round ends with the build published to the web installer as a preview (`web/make_tfm_site.py ... --preview`, gh-pages under `next/`; the tested release at the root is never replaced), the page checked for the new .fwsc, and the reply ending with the link, the package name and the commit. Done for this round below.

**A fix the suite found** (`63e9414`, final build 503,104 B / RAM 63,224 B): item 2's DEEP EDIT row in the MENU drew its value and then fell through to an empty one; the empty one's alignment hook measured the next text drawn, and the full suite's ui_render check failed on 138 places (the quick UI test does not run it). It returns after its value now; both renders 0 off.

**Renders:** tfm-spec/built re-rendered (ui_dice is gone: START's ui_start, ui_start_rolled, ui_start_spin), the contact sheet redone; 0 lint findings.

**Suite (round 6):** tests=0 (ALL HOST TESTS PASSED, after the DEEP EDIT row fix above), no FAIL lines, target_budget 0 (the audio ISR fm1_alnk0_irq costs 8,705 against 8,285: +5 %, inside the +10 %; round 5 was +4 %: the per-part level trim and the per-voice key level). Final build `63e9414`: 503,104 B, RAM 63,224 B (headroom 4,360 B), pool 365,752 B. Published as the installer preview (`next/`), the tested release at the root unchanged.

## Round 7 (ROUND-7.md: the instruments that vanished, and three small things)

Branch `tfm-ui`: `95435e4` (1), `91ff2d2` (2 and 4: one commit, they share tfm_faces.c and the same screens), `2419f94` (3). Package `tilt-fm-ui-r7-<sha>.fwsc`. Nothing was run on an FM-1.

**Flash and RAM** (app image / RAM .data+.bss; pool 365,752 B throughout): round 6 final 503,104 / 63,224. Item 1: 503,108 (+4). Items 2 + 4: 503,132 (+24). Item 3: 503,164 (+32). RAM unchanged (63,224 B, headroom 4,360 B).

1. **Instruments vanish after a song plays: found and fixed.** Your cause was right: `project.c` `chain_prepare`, SOUNDS SCENE, stored each scene's preset as `preset % npresets`, which wraps a TFM byte (0x20 and up) into the engine's factory range; `chain_sound_put` wrote it to `t->preset`, `tfm_ins_of` said TI_NONE, and the autosave kept it. It now keeps `preset >= TFM_PRE0` as is (as the scene stage at ~1899 and the restore at ~1263 did). **Test first, against the old line:** `test_r7_instruments` (a 4-track band, two scenes with different instruments on all four tracks, a chain of the two with SOUNDS SCENE on): it fails on the old code (the playing row's tracks lose their bytes) and passes on the new. Then: stopped (the live sounds come back, each with its byte), the chain again from JAM, JAM and SOUND on the instrument (JAM stays JAM, EDIT toggles the pages, never DEEP), a power cycle done the autosave's way (capture, pack, unpack, restore; the tracks set to other sounds first), MUTATE SOUND. **The audit, every place a preset byte is copied, wrapped, clamped or defaulted** (project.c, song_chain.c, seq.c, ui.c, upreset.c, editor*.c, tilt.c, tfm_*.c, web/):
   - project.c 1499 (chain_prepare, the SOUNDS SCENE source): **the bug, fixed**.
   - project.c 1899 (scene stage) and 1263 (project restore): keep the byte already.
   - project.c 258/266 (proj_trk_from), 290, 415, 436, 563, 651, 735 (the format copies and the pack / unpack), 1040 (capture), 1476 (the live copy for the chain): copy the byte as it is.
   - project.c 301, 479, 1284-1287: PROJ_DEF_KEEP / PROJ_DEF_SOUND (the old formats' own markers, never a TFM byte), their conversions at 323, 349-350, 381-382 only touch DIGITAL and SAMPLE PERC tracks (engines that have no instrument): fine.
   - song_chain.c 73 (chain_sound_put): copies the byte it is given (it was given the wrapped one).
   - seq.c 1363 (the scene stage onto the track): copies.
   - ui.c 1149 / 1207 / 1259 (undo): save and restore the byte; 1461 (the retired DIGITAL load), 1518 (apply_preset_to, an engine's own preset chosen on purpose: PRESETS in DEEP, the editor's PRESET command): a deliberate choice of an engine preset, so it ends the instrument by design.
   - ui.c 1686 / 1848 (`% npresets` in the PRESETS list position), ui_draw.c 846 (`% npresets`, the name shown): reads only, tagged tracks are handled (`tagged`).
   - upreset.c 481 (loading a user preset sets `preset = 0`, `t->user` marks it; a TFM sound saved as a user preset is the tagged case at ui.c 1682): by design.
   - editor.c 122-180 (the byte sent to the web editor, `& 0x7F` by ed_b: a display value only, never written back), editor.c 515-532 (ED_PRESET: an engine preset on purpose).
   - tfm_instr.c 482 (tfm_load), tfm_live.c prf_mutate (calls tfm_load: keeps the instrument): fine.
   - web: fm1backup.js moves objects as bytes (no preset field read); editor.html's applyPreset is the editor's own engine preset choice.
   So after this nothing but a deliberate PRESETS / editor / user-preset action can turn an instrument's byte into a raw engine preset. **No repair on load**, as the spec says: a wrapped byte cannot be told from a real Felucca preset; projects already damaged have to pick the instrument again (hold EDIT).
2. **No instrument: a plain TFM face.** SOUND on a track with no instrument shows "NO INSTRUMENT / HOLD EDIT TO PICK ONE" (and its engine's name small); EDIT toggles its pages as everywhere (the old "EDIT: DEEP" line is gone). JAM's cabinet says NO / INSTRUMENT / HOLD EDIT in place of the dial (a muted one still says MUTED); the knob no longer drives LEVEL from there. DEEP is reached only from MENU > DEEP EDIT. Renders: `ui_snd_none`, `ui_jam_none`. Test: `test_r7_noinst`.
3. **Which build is this.** `TILT_BUILD` (ui.c, default `dev`): small on START's display under TILT; MENU > INFO's first row is now BUILD (it was FIRMWARE showing Felucca's v1.6.1; the INFO page has no room for a fourth row, so Felucca's version stays on ABOUT's FELUCCA page and ABOUT's TILT-FM line now reads "TILT-FM 0.2.1 . r7-<sha>"). `tools/build.py` passes `-DTILT_BUILD=` from `$TILT_BUILD` (this round's package: `r7-<sha>`, the package name's tag), else `dev-<git short sha>`. Test: it is drawn on START.
4. **SOUND's header** reads MACROS 1 . 2 (done in the item 2 commit).

**Tests:** tests/ui_test_r6.h (round 7 section): `test_r7_instruments`, `test_r7_noinst`, `test_r7_build`.

**Suite (round 7):** tests=0 (ALL HOST TESTS PASSED), no FAIL lines, target_budget 0 (the audio ISR 8,705 against 8,285: +5 %, unchanged from round 6). 0 lint findings, both renders 0 off. Final build 503,164 B, RAM 63,224 B, pool 365,752 B. Published as the installer preview (next/), the tested release at the root unchanged.
