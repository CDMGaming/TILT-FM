# TILT-FM hardware check: rounds 9 and 10 (one FM-1)

Felix runs this; nothing here was run on hardware by Claude. Battery charged **and** USB plugged in for the install.
Package: `tilt-fm-ui-r10-<sha>.fwsc`. Write the result after each line (OK / what happened).

| # | Check | Result |
|---|---|---|
| 1 | **Fresh install** from the preview page. First boot: seconds from power-on to the start screen: ____ s | |
| 2 | The new splash: dot field, TILT, -FM-, the ball and flippers, FREE PLAY, "TILT-FM r10-... . FELUCCA v1.6.1", "HÜGELTON INSTRUMENTS" all readable | |
| 3 | MENU (hold HOME): the header shows the build `r10-...` next to MENU | |
| 4 | **20-minute jam** with scenes: launch scenes by tapping, open the scene sheet (hold a scene key), DUP, x2, /2, CLEAR (asks), CHAIN (hold F#5), DJ (hold ARP, all four knobs), REC on some moves | |
| 4a | ... any audio dropouts or clicks? when? | |
| 4b | ... any stuck notes? (hold PLAY = panic is V16's, not ours: note how you stopped them) | |
| 4c | ... does the screen keep up (knobs, meters, the sheet) or lag? | |
| 4d | ... the scene sheet while playing: an action on another scene applies at once; on the playing scene, the header says AT LOOP END and it changes at the loop's end | |
| 5 | **Stop, power off, power on**: CONTINUE brings the jam back as it was at the stop (scenes, DJ values, sounds) | |
| 6 | **Pull the USB / power while playing** (battery low or unplug-and-switch-off): on power-on the last stopped state comes back, nothing damaged | |
| 7 | **Every delete, then hold SAVE**: NOTES (EDIT deletes a note), SONG (EDIT deletes a section), the scene sheet's CLEAR / DUP / x2 / /2: each comes back as it was just before | |
| 8 | **Knob speed**: count the clicks in one full turn of a knob: ____ clicks/turn | |
| 8a | MORPH on JAM, turned at a steady pace: about 1.5 turns for 0 -> 100? (____ turns) | |
| 8b | each DJ knob (hold ARP): about 1.5 turns end to end? slow turns still single steps? | |
| 9 | **Drum keys** on THUMP LAB, KITBOX and GARAGE: F3 .. F4 the lanes, G4 .. G5 the pitched lane going up; no level jump between F4 and G4, none anywhere F3 .. G5 | |
| 10 | **Melodic keys**: play F3 .. G5 slowly on a few instruments (SQUELCH, HAZE, VOX, PLUCK HARP): no key much louder or quieter than its neighbours | |
| 11 | **PRESETS**: one small brush shows the sound card and changes nothing; turning on changes the sound; OCT- while the card shows goes back to the first sound; at the end of the list the card says HOLD EDIT | |
| 12 | **Hold footers**: on JAM, MIX, SOUND, NOTES: hold each of SAVE, HOME, EDIT, ENV, FX, GLO, SCL, SEQ, REC, LFO, ARP (and OCT+ on NOTES): the footer says what it does before the hold takes over | |
| 13 | First run: on a fresh install JAM's footer says HOLD A BUTTON: SEE WHAT IT DOES until a button is held; after a restart it stays gone | |
| 14 | MIX and DRUMS: the black-key strip at the bottom matches what the keys do (MIX: T1-T4 mute; DRUMS: lanes, ACC, pages) | |
| 15 | **Way back**: editor Backup > Export saves a file; "Return to official V15" accepts your V15 FM-1.fwsc (do not run it unless you mean to) | |

Notes:
