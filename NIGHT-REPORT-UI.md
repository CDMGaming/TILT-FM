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
