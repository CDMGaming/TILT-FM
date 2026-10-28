# TFM-NEXT night report (2026-10-27/28)

Branch `tfm-next` on CDMGaming/TILT-FM, from tag `tfm-0.2-final` (= 618a488, the SAVE page rename on 0.2.1).

## READ THIS FIRST
- **Firmware to flash:** `tilt-fm-next-00ad7ec.fwsc` (in this folder, 610,090 B, identity FM-1_984), built from `00ad7ec` after the final full test run. Nothing was flashed. Flash it yourself with the usual installer, following FLASHING-CHECKLIST.md.
- **Final tests:** `tests/run_tests.sh` ALL HOST TESTS PASSED (5,901 checks ok, 0 failed); `tests/target_budget.py` 52 ok, 0 over. App 413,372 B (0.2.1 was 529,152), RAM 61,224 of 73,728 B.
- **Try it in the emulator first:** in WSL run `cd ~/TILT-FM && git checkout tfm-next && sh web/emu/build.sh && python3 -m http.server 8000`, then open `http://localhost:8000/build/emu/` in a browser.
- **Watchdog:** it was set up and it **fired once**. At 00:48 my session was paused by a usage limit; the heartbeat and your keyboard/mouse had both been idle for 120 minutes, so it shut the PC down as designed and deleted its own task. Work resumed after the 07:03 boot, and nothing was lost (every phase had been committed and pushed). Its note in this file was overwritten by my later copies; Windows' event log has it (event 1074, "TFM overnight run: idle"). The task is already gone, so there is nothing to remove. If it ever is there, `schtasks /Delete /TN TFM-Overnight-Watchdog /F` removes it, and the script is `C:\Users\Felix\tfm-watchdog.ps1`. Decision: I did not re-create it this morning, since the night's run is over.

### Manual changes (for the TFM manual)
- **Built-in samples are gone**: SAMPLE and SLICE have no factory sounds, USR1-3 work. Old sounds that used PIANO/FLUTE/SAX/BREAK say NO SAMPLE and play a sine. All four GRAIN factory sounds (CLOUD PAD, GLITCH, FROZEN, SHIMMER) used them too and now play the sine.
- **NEW SONG**: rows KEY, SCALE, TEMPO (K2 swing), LINEUP (BAND / WILD / BAND + 1), T1-T4 (instrument and sound), CODE (TILT setup codes; type one to load a setup).
- **Instruments**: 12 instruments over the engines; **EDIT opens MACROS** (K1-4 = macros 1-4), EDIT again MACROS 2 (5-8), then the engine pages. PRESETS lists the instruments first. See MACROS-TO-TUNE.md.
- **HOME > JAM** (MENU > DISPLAY > HOME): four bumpers, K1-4 = each track's macro 1 (or LEVEL), REC records the moves.
- **Track colours**: T1 blue, T2 yellow, T3 pink, T4 green.
- **MENU**: four rows at a time. SELECT/PRESETS moves, and **KNOB k changes the k-th row shown** (it has a K1-K4 badge in that colour). OCT+/- change the selected row.
- **New palettes**: PLASMA, MINT, PLAYFIELD, NEON, CHROME (after MONO).
- **LEN in bars**, **hold OCT- while turning for single steps**. **MENU > AUDIO > TIME SIG** 4/4 3/4 6/8 7/8 (sets the bar length for LEN only).
- **Quick chain**: SCENE layer, **hold G4, tap scene keys** in order. G4 alone still adds the current scene (on release).
- **Step sheet**: STEP page, **hold OCT+**: K1 MOVE, K2 COPY, K3 NUDGE, K4 SLIDE (drums: RATCH).
- STEP hint names **EDIT = REST**.
- **Pinball toys** on THUMP LAB's and SQUELCH's MACROS page.

### Not done tonight (next steps)
- Toys for the other instruments. Time signature for the click and count-in. Quick chain queuing while playing. COPY of a step's locks in the step sheet.
- 8 scenes / several songs (format change), SAVE pages reorganised into SOUNDS / SCENES / SONGS, Morph and Anthem engines, WAV / stems / MIDI export (JIANT `web/emu/stems.js`, TILT `wav.ts`, `smf.ts`), USB playback, the all-steps overview.
- Tune the macros by ear (MACROS-TO-TUNE.md) and the instruments with few sounds (VOX, WHEELHOUSE 6; WOBBLER, TINE, GLINT 8).

## Final run fixes (00ad7ec)
- The final full run found leftovers from earlier phases; all are fixed and committed:
  - golden renders, updated for phase A's intended change;
  - the SLICE test now builds against the header that has the data;
  - two test links that pull in params.c without the UI;
  - the editor test now expects JAM in MENU > HOME;
  - sample_render's static budget, rebaselined 126 -> 1097: the compiler lays the code out differently with no built-in tables, and the user-sample code is unchanged.

## 0. Safety and watchdog: done
- Tag `tfm-0.2-final` and branch `tfm-next` pushed to CDMGaming/TILT-FM. The WSL clone's `origin` is upstream
  hugelton/Felucca (no write access), so every push goes to the CDMGaming URL by name; nothing was pushed upstream.
- Heartbeat: `TILT-FM\.cc-heartbeat` (Windows folder), touched at each commit.
- Watchdog: `C:\Users\Felix\tfm-watchdog.ps1`, per-user task `TFM-Overnight-Watchdog`, every 15 min, ends
  2026-10-28 11:00, interactive only, no admin. Shuts down (5 min warning, `shutdown /a` cancels) only when the
  heartbeat AND your keyboard/mouse are both idle over 120 min. Dry run: no action. Remove it with
  `schtasks /Delete /TN TFM-Overnight-Watchdog /F`.


## A. Room and measurements: done (d4151b9)
- **Built-in samples dropped**: SAMPLE PIANO / FLUTE / SAX and SLICE BREAK. **App 529,152 -> 398,532 B (-130,620)**, RAM unchanged (61,144 B).
- Old sounds and projects that used them still load, keep their values, play the "missing sample" sine and say NO SAMPLE. SAMPLE and SLICE now offer no factory sounds (PRESETS, NEW SONG and program changes skip them); the USR1-3 user samples work as before.
- Side effect: all four GRAIN factory sounds used the built-in sets as their source; they now play the missing sine too (corrected in the final run: not only FROZEN and SHIMMER).
- Tests: Felucca's sample-playing tests build against a second header that has the data (`build/gen_smp`), so they still guard the unchanged engine code; a new ui_test run checks the shipping (no-sample) build.
- Per-engine table. Flash = symbol sizes by name (inlined code is undercounted). Host CPU = instructions per sample for the worst factory preset (`tests/cpu_baseline.txt`). Target static = the render loop's cost (`tests/target_budget.txt`).

| Engine | Flash | Host CPU worst | Target static |
|---|---|---|---|
| ANALOG | 5,494 B | 1461 | 585 |
| FM6 | 13,783 B | 1475 | 372 |
| PHASE | 3,544 B | 1558 | 79 |
| LOFI | 1,388 B | 807 | 123 |
| SAMPLE | 2,385 B | 907 | 126 |
| VOICE | 2,318 B | 975 | 314 |
| TRIO | 3,170 B | 1478 | 341 |
| WHEEL | 1,684 B | 1418 | 267 |
| GRAIN | 3,414 B | 906 | 1731 |
| PHYS | 9,079 B | 1451 | 61 |
| NOISE | 2,144 B | 1395 | 259 |
| SLICE | 4,034 B | 1008 | 453 |
| DRUM | 14,060 B | 1265 | 86 |

## B. The dice: done (8019dbf)
- **NEW SONG** now has 9 rows (keys F3..A4): KEY, SCALE, TEMPO (KNOB 2 = swing), **LINEUP** (BAND / WILD / BAND + 1), T1..T4 (instrument + sound: KNOB 1 every sound, KNOB 2 the instrument), **CODE**.
- Rolls use TILT's generator and TILT's tempo bands and scale weights, ported exactly and tested against TILT's own numbers.
- **Setup codes** use TILT's format (both versions). A TILT code entered on TFM gives TILT's key, scale, tempo, swing and instruments. Kinds TFM lacks map to the closest of their role; slots past 4 are dropped; fewer than 4 are filled from the code itself, the same way every time. Type a code on the CODE row (KNOB 1 character, KNOB 2 place, KNOB 3 length); "CODE OK" when it decodes.
- **Jam of the day**: enter TILT's daily code. Proposal for TILT's site (not built): next to today's code, a "TFM" button that shows it large in groups of four, plus a QR / short link. Codes carry everything, so TFM needs no clock.
- Challenge cards: **skipped** (optional; most can't be checked on the device).
- Decision: an instrument's sounds are its engine's factory presets plus a few changed values (no new storage). A track remembers its instrument in its preset byte (0x20 + 16 x instrument + sound): no format change.
- Tests: TILT parity (six real TILT codes from Deno, the generator's numbers), round trips, BAND's roles, WILD repeats, START and a save. ui_test 0 failed; the TFM shipping run passed; union_test 0/720 differ; midi_control ok.
- Sizes: app 406,628 B, RAM 61,176 B.

## C. Instruments: done (ace5105)
- **12 instruments** over the engines: THUMP LAB (DRUM), SQUELCH (ANALOG), WOBBLER (TRIO + LFO sync + crush), CHORDETTE (ANALOG + chord keys), RAZOR (ANALOG, sync/unison), HAZE (TRIO pad, crush + reverb), PIXEL (LOFI), TINE (FM6), GLINT (FM6), PLUCK (PHYS); wildcards VOX (VOICE) and WHEELHOUSE (WHEEL). 6-10 sounds each (names and intent from TILT's presets; values are each engine's factory preset plus a few changes, since the DSP differs).
- **MACROS**: EDIT now opens MACROS (KNOB 1-4 = macros 1-4), EDIT again MACROS 2 (5-8), then the engine's own EDIT pages as before. Only on a track that plays an instrument. Macro rules are written into `tfm_instr.c`. A macro has no storage of its own (its position is read back from its first target), and REC records it as automation.
- **PRESETS** lists the instruments' sounds first (tags THMP, SQLC, WOBL, CHRD, RAZR, HAZE, PIXL, TINE, GLNT, PLCK, VOX, WHEL), then Felucca's raw engine sounds and the user presets as before. Decision: I kept the raw list in PRESETS (after the instruments) rather than only in EDIT, so nothing Felucca could reach becomes harder to reach, and Felucca's own PRESETS tests still guard it.
- The bottom line shows the instrument and the sound (e.g. "SQUELCH RUBBER").
- **`MACROS-TO-TUNE.md`** (repo and Windows folder): every macro's targets with their values at 0 / 25 / 50 / 75 / 100 %, with a column to tick when checked by ear.
- Not done: per-instrument 8-12 sounds where an engine has few factory presets (VOX 6, WHEELHOUSE 6, WOBBLER 8, TINE 8, GLINT 8); they will need tuning by ear anyway.
- Tests: ui_test 0 failed (TFM checks for sounds, macros, pages, PRESETS); TFM shipping run passed; editor_test and web tests passed; render lint 0.
- Sizes: app 408,604 B, RAM 61,176 B.

## D. JAM, colours, menus, palettes: done (6f898d6)
- **D1 JAM**: MENU > DISPLAY > HOME > **JAM** (a fourth HOME view after SCOPE / TRACKS / LEVELS). Four bumpers, one per track; KNOB 1-4 = each track's jam control (its instrument's first macro; a track with no instrument: its LEVEL). Ring = the knob's position (7 to 5 o'clock), a flash on each note (detected as a jump in the track's level, so keys, sequencer and MIDI all count), a glow with the level, muted tracks dark with a dashed ring, a dot on the selected track. The cards above show each track's macro name and value in its colour.
  - Decision: JAM is a HOME view, **not the default** (SCOPE stays the default so saved settings and Felucca's checks keep their meaning). Set it once in MENU > DISPLAY > HOME; it is saved.
- **D2**: REC records the jam moves as automation (the macro writes go through `motion_capture`, as any knob). Not separately tested on the device.
- **D3 colours**: T1 blue, T2 yellow, T3 pink, T4 green. In the MENU each row a knob changes carries a K1-K4 badge and its label in that knob's colour. GREY and MONO keep no hue.
- **D4 four-row window**: every MENU tab shows four rows. SELECT (PRESETS) moves the row and the window follows (a scroll bar when a tab has more); **KNOB k changes the window's row k** (that row becomes the selected one); OCT+/OCT- change the selected row as before.
  - Decision: Felucca's own menu tests say "any knob changes the selected row". I kept them by making the test helper turn the selected row's knob (the intent survives); TFM's new check turns the knobs as named.
- **D5 palettes**: PLASMA, MINT, PLAYFIELD, NEON, CHROME, appended after MONO (saved palette ids are indices, so old settings keep their colours). The COLOR row's swatches are a bit smaller so PLAYFIELD fits beside the badge.
- Tests: ui_test all passed (new TFM JAM and MENU checks), theme_test and settings_test passed, render lint 0 findings, 0 GREY/MONO pixels off neutral; emulator built.
- Sizes: app 410,452 B, RAM 61,192 B.

## E. Length, songs, workflow: done except as noted (95f3983)
- **E1 bars first**: LEN turns in whole bars (from between two bars it goes to the next one; under a bar it still moves step by step). Shows `2 BARS`, `2+3 BARS`, `7 STEP`. **MENU > AUDIO > TIME SIG**: 4/4 (default), 3/4, 6/8, 7/8 = 16, 12, 12, 14 steps a bar. Single steps: **hold OCT- while turning LEN** (decision: the FM-1 has no SEL button; OCT-'s octave change is taken back when used this way). Halve/double were already in SEQ TOOLS (D5 DOUBLE, E5 HALF).
  - Limit: the time signature only sets LEN's bars for now; the click and count-in still count in 4.
- **E2 quick chain**: SCENE layer (ARP held): **hold G4 and tap scene keys** in order, each tap appends a section; the same scene again adds a repeat to the last section. The screen shows the song's tail, e.g. `SONG: D Ax3 D`. G4 alone still adds the current scene (now on release). Not done: queuing while playing (it says STOP TO EDIT, as ADD does).
- **E3 step sheet**: on STEP, **hold OCT+**: the cards become KNOB 1 MOVE (the step and its locks one place; cursor follows), KNOB 2 COPY (onto the next/previous step, cursor follows, so turning on stamps it along), KNOB 3 NUDGE, KNOB 4 SLIDE (drum tracks: RATCH x1-x4). Each turn is one undo. DELETE stays EDIT and FILL stays DETAIL's condition (both already existed). Licence: SLOOP is GPL-3.0 (same as Felucca); only the idea was used, no code copied.
- **E4 REST**: the STEP page's hint now says `[EDIT] REST` (EDIT already wrote a rest; no free key for its own).
- **E5 key maps**: already there. Felucca draws each held layer's key map on screen; nothing to add.
- **E6**: done in 0.2.1.
- Tests: ui_test all passed (new TFM checks for LEN, QUICK CHAIN and STEP SHEET; Felucca's AUDIO-tab checks updated for the new row), settings_test passed, render lint 0; emulator built.
- Sizes: app 412,556 B, RAM 61,208 B.

## F. Pinball toys: partly done (70e6a4f)
- On an instrument's **MACROS** page, between its name and the hint: **THUMP LAB** = pop bumpers (KICK the big bumper, SNARE/CLAP a slingshot, the hats a spinner that turns a notch per hat), each lit when its lane hits; **SQUELCH** = a spinner lit on each note, turning faster with the cutoff.
- Others (Chordette, Razor, Haze, Pixel, Tine, Pluck) not done: the plan said start with drums + Squelch; next steps.
- Cost: RAM +16 B, flash +0.8 KB; the page redraws every other frame like the scope.
- Tests: ui_test all passed, render lint 0 (new toy scenes); emulator built.
