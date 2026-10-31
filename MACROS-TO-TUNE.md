# MACROS TO TUNE (TFM instruments)

Generated from `tools/tfm_ins_spec.py` by `tools/gen_macros_doc.py`: every macro, its targets (`lo..hi`) and what each is at 0 / 25 / 50 / 75 / 100 % of the turn. Check each by ear and tick it, or change `lo` / `hi` in `tools/tfm_ins_spec.py` and `firmware/src/tfm_instr.c`. Rules (tfm-spec/TFM-INSTRUMENTS.md): a big musical change across the turn, no dead zone, 50 sounds like the instrument, no jumps (a SWITCH steps), level steady while a macro adds drive, no LEVEL macro.

## THUMP LAB

| # | Macro | Targets | 0 | 25 | 50 | 75 | 100 | OK? |
|---|---|---|---|---|---|---|---|---|
| 1 | MORPH (JAM) | from the sound as set to: DECY -> 110; REV -> 90; IA -> 110; IMIX -> 100; DRV -> 90; DLY -> 25; LN3 -> 127; LEVEL -9 (level, half dB) | as set | | | | the contrast |  |
| 2 | PUNCH | LN0 96..127; DRV 0..80; ACC 40..120; LEVEL 108..102 | 96/0/40/108 | 104/20/60/106 | 112/40/80/105 | 119/60/100/104 | 127/80/120/102 |  |
| 3 | SMACK | SNAP 10..120; LN1 80..115; LN2 80..115; LEVEL 106..100 | 10/80/80/106 | 38/89/89/104 | 65/98/98/103 | 92/106/106/102 | 120/115/115/100 |  |
| 4 | TIME | DECY 8..120 EXP; LEVEL 104..94 | 8/104 | 15/102 | 36/99 | 71/96 | 120/94 |  |
| 5 | ROOM | REV 0..90; DLY 0..25 >70% | 0/0 | 22/0 | 45/0 | 68/4 | 90/25 |  |
| 6 | TUNE | TUNE 16..112 | 16 | 40 | 64 | 88 | 112 |  |
| 7 | DIRT | IA 0..110; IMIX 20..100; LEVEL 106..104 | 0/20/106 | 28/40/106 | 55/60/105 | 82/80/104 | 110/100/104 |  |
| 8 | HATS | LN3 80..127; LN4 80..127 | 80/80 | 92/92 | 104/104 | 115/115 | 127/127 |  |

Presets: WAREHOUSE, BOOM BAP, DISCO, TRASH CAN, ARCADE.

## SQUELCH

| # | Macro | Targets | 0 | 25 | 50 | 75 | 100 | OK? |
|---|---|---|---|---|---|---|---|---|
| 1 | MORPH (JAM) | from the sound as set to: CUT -> 100; RES -> 118; ED>FLT -> 60; DRV -> 100; DEC -> 55; IMIX -> 60; LEVEL -3 (level, half dB) | as set | | | | the contrast |  |
| 2 | CUTOFF | CUT 18..112 EXP | 18 | 24 | 42 | 71 | 112 |  |
| 3 | RESO | RES 10..118; LEVEL 108..102 | 10/108 | 37/106 | 64/105 | 91/104 | 118/102 |  |
| 4 | ENV MOD | ED>FLT 0..60; DEC 30..55; LEVEL 108..102 | 0/30/108 | 15/36/106 | 30/42/105 | 45/49/104 | 60/55/102 |  |
| 5 | DECAY | DEC 20..100; REL 10..50 | 20/10 | 40/20 | 60/30 | 80/40 | 100/50 |  |
| 6 | BITE | DRV 0..120; LEVEL 108..99; DIST 0..70 | 0/108/0 | 30/106/18 | 60/104/35 | 90/101/52 | 120/99/70 |  |
| 7 | WAVE (SWITCH) | WAVE: SAW SQR PWM SYNC SUB | SAW | SQR | PWM | SYNC | SUB |  |
| 8 | SLIDE | GLIDE 0..110 EXP | 0 | 7 | 28 | 62 | 110 |  |

Presets: RUBBER, ACID RAIN, LOW RIDER, ZAPPER, GNARLY.

## WOBBLER

| # | Macro | Targets | 0 | 25 | 50 | 75 | 100 | OK? |
|---|---|---|---|---|---|---|---|---|
| 1 | MORPH (JAM) | from the sound as set to: LD>FLT -> 63; RES -> 60; CUT -> 100; IMIX -> 90; IA -> 80; DIST -> 60; LEVEL +0 (level, half dB) | as set | | | | the contrast |  |
| 2 | WOB | LD>FLT 0..63; RES 10..60 | 0/10 | 16/22 | 32/35 | 47/48 | 63/60 |  |
| 3 | RATE (SWITCH) | LSYNC: 1/1 1/2 1/4 1/8 8T 1/16 16T 1/32 | 1/1 | 1/2 | 1/8 | 1/16 | 1/32 |  |
| 4 | GROWL | IMIX 0..100; IA 20..90 | 0/20 | 25/38 | 50/55 | 75/72 | 100/90 |  |
| 5 | CUT | CUT 15..110 EXP | 15 | 21 | 39 | 68 | 110 |  |
| 6 | DIRT | DIST 0..90; LEVEL 106..98 | 0/106 | 22/104 | 45/102 | 68/100 | 90/98 |  |
| 7 | DROP (SWITCH) | TRANS: 0 -1 OCT -2 OCT | 0 | 0 | -1 OCT | -2 OCT | -2 OCT |  |
| 8 | SLIDE | GLIDE 0..110 | 0 | 28 | 55 | 82 | 110 |  |

Presets: HALF STEP, YOY, TRASH FLOOR, LASER WOB, SUBWAY.

## CHORDETTE

| # | Macro | Targets | 0 | 25 | 50 | 75 | 100 | OK? |
|---|---|---|---|---|---|---|---|---|
| 1 | MORPH (JAM) | from the sound as set to: CUT -> 110; RES -> 10; CHOR -> 60; LD>AMP -> 70; REV -> 70; DLY -> 30; LEVEL +0 (level, half dB) | as set | | | | the contrast |  |
| 2 | CHORD (SWITCH) | CHRD: AUTO AUTO 7 MAJ MIN 7 MAJ7 MIN7 SUS4 POWER | AUTO | MAJ | 7 | MIN7 | POWER |  |
| 3 | VOICE (SWITCH) | VOIC: CLOSE OPEN DROP2 SPREAD STACK | CLOSE | OPEN | DROP2 | SPREAD | STACK |  |
| 4 | WARM | CUT 30..110; RES 40..10; LEVEL 110..100 | 30/40/110 | 50/32/108 | 70/25/105 | 90/18/102 | 110/10/100 |  |
| 5 | SWELL | ATK 0..90 EXP; REL 10..110 | 0/10 | 6/35 | 22/60 | 51/85 | 90/110 |  |
| 6 | SHIMMER | CHOR 0..60; DTN 0..20 | 0/0 | 15/5 | 30/10 | 45/15 | 60/20 |  |
| 7 | SPACE | REV 0..127; DLY 0..70 >70% | 0/0 | 32/0 | 64/0 | 95/12 | 127/70 |  |
| 8 | PUMP | LD>AMP 0..80 | 0 | 20 | 40 | 60 | 80 |  |

Presets: HOUSE STAB, SUNDAY, VELVET, PUMPER, MIXTAPE.

## RAZOR

| # | Macro | Targets | 0 | 25 | 50 | 75 | 100 | OK? |
|---|---|---|---|---|---|---|---|---|
| 1 | MORPH (JAM) | from the sound as set to: RES -> 110; DRV -> 70; CUT -> 120; ED>FLT -> 60; DETUNE -> 80; SPRD -> 127; DLY -> 60; LEVEL +0 (level, half dB) | as set | | | | the contrast |  |
| 2 | SCREAM | RES 10..110; DRV 0..70; LEVEL 108..92 | 10/0/108 | 35/18/104 | 60/35/100 | 85/52/96 | 110/70/92 |  |
| 3 | SWEEP | ED>FLT -20..60; DEC 20..70 | -20/20 | 0/32 | 20/45 | 40/58 | 60/70 |  |
| 4 | CUT | CUT 30..120 EXP; LEVEL 122..100 | 30/122 | 36/116 | 52/111 | 81/106 | 120/100 |  |
| 5 | GLIDE | GLIDE 0..110 | 0 | 28 | 55 | 82 | 110 |  |
| 6 | WIDTH | DETUNE 0..127; SPRD 0..127 | 0/0 | 32/32 | 64/64 | 95/95 | 127/127 |  |
| 7 | ATTACK | ATK 0..80 | 0 | 20 | 40 | 60 | 80 |  |
| 8 | VIB | LD>PIT 0..4 | 0 | 1 | 2 | 3 | 4 |  |

Presets: HERO, TRANCE GATE, ARENA, SINE RIDER, HOT WIRE.

## HAZE

| # | Macro | Targets | 0 | 25 | 50 | 75 | 100 | OK? |
|---|---|---|---|---|---|---|---|---|
| 1 | MORPH (JAM) | from the sound as set to: CUT -> 22; LD>FLT -> 60; CHOR -> 110; REV -> 127; IMIX -> 60; LEVEL +12 (level, half dB) | as set | | | | the contrast |  |
| 2 | DRIFT | LD>PIT 0..2 >70%; DTN 5..30 | 0/5 | 0/11 | 0/18 | 0/24 | 2/30 |  |
| 3 | DUST | IMIX 0..127; IB 120..20; IC 120..30 | 0/120/120 | 32/95/98 | 64/70/75 | 95/45/52 | 127/20/30 |  |
| 4 | SPACE | REV 10..127; LEVEL 108..99 | 10/108 | 39/106 | 68/104 | 98/101 | 127/99 |  |
| 5 | BLOOM | ATK 10..120 EXP; REL 30..120 | 10/30 | 17/52 | 38/75 | 72/98 | 120/120 |  |
| 6 | TONE | CUT 10..28 EXP | 10 | 11 | 14 | 20 | 28 |  |
| 7 | SHINE | CHOR 0..127; SPRD 0..110 | 0/0 | 32/28 | 64/55 | 95/82 | 127/110 |  |
| 8 | MOVE | LD>FLT 0..60 | 0 | 15 | 30 | 45 | 60 |  |

Presets: FOG, MORNING, VHS, AURORA, DEEP END.

## PIXEL

| # | Macro | Targets | 0 | 25 | 50 | 75 | 100 | OK? |
|---|---|---|---|---|---|---|---|---|
| 1 | MORPH (JAM) | from the sound as set to: CRSH -> 70; DUTY -> 120; DEC -> 80; TONE -> 127; REV -> 40; LEVEL -2 (level, half dB) | as set | | | | the contrast |  |
| 2 | JUMP (SWITCH) | ARP: OFF OCT 5TH | OFF | OFF | OCT | 5TH | 5TH |  |
| 3 | WAVE (SWITCH) | WAVE: PULSE TRI SAW NOISE WAVE | PULSE | TRI | SAW | NOISE | WAVE |  |
| 4 | SWEEP | SWP 0..25; LEVEL 104..110 | 0/104 | 6/106 | 12/107 | 19/108 | 25/110 |  |
| 5 | CRUSH | CRSH 0..127; TONE 127..40 | 0/127 | 32/105 | 64/84 | 95/62 | 127/40 |  |
| 6 | VIB | VIB 0..110 | 0 | 28 | 55 | 82 | 110 |  |
| 7 | DECAY | DEC 15..100; SUS 20..110 | 15/20 | 36/42 | 58/65 | 79/88 | 100/110 |  |
| 8 | DUTY | DUTY 0..120; LEVEL 110..104 | 0/110 | 30/108 | 60/107 | 90/106 | 120/104 |  |

Presets: 1UP, BOSS LEVEL, WARP ZONE, HIGH SCORE, GLITCH.

## TINE

| # | Macro | Targets | 0 | 25 | 50 | 75 | 100 | OK? |
|---|---|---|---|---|---|---|---|---|
| 1 | MORPH (JAM) | from the sound as set to: MLVL -> 40; MEG -> 40; LD>AMP -> 90; CHOR -> 110; IMIX -> 80; REV -> 60; LEVEL +4 (level, half dB) | as set | | | | the contrast |  |
| 2 | BARK | MLVL -40..40 | -40 | -20 | 0 | 20 | 40 |  |
| 3 | TINE | MEG -40..40 | -40 | -20 | 0 | 20 | 40 |  |
| 4 | TREM | LD>AMP 0..90 | 0 | 22 | 45 | 68 | 90 |  |
| 5 | CHORUS | CHOR 0..110 | 0 | 28 | 55 | 82 | 110 |  |
| 6 | GRIT | DIST 0..90; LEVEL 106..100 | 0/106 | 22/104 | 45/103 | 68/102 | 90/100 |  |
| 7 | WARMTH | IMIX 0..80; LEVEL 106..110 | 0/106 | 20/107 | 40/108 | 60/109 | 80/110 |  |
| 8 | SPACE | REV 0..100 | 0 | 25 | 50 | 75 | 100 |  |

Presets: SUITCASE, LOUNGE, BARK BARK, DREAMY, CHURCH BASEMENT.

## GLINT

| # | Macro | Targets | 0 | 25 | 50 | 75 | 100 | OK? |
|---|---|---|---|---|---|---|---|---|
| 1 | MORPH (JAM) | from the sound as set to: MLVL -> 50; MEG -> 50; DEC -> 120; REV -> 110; IMIX -> 90; DLY -> 40; LEVEL +0 (level, half dB) | as set | | | | the contrast |  |
| 2 | METAL | MLVL -40..50; LEVEL 106..100 | -40/106 | -18/104 | 5/103 | 28/102 | 50/100 |  |
| 3 | RATIO (SWITCH) | MRAT: -4 -2 0 2 4 | -4 | -2 | 0 | 2 | 4 |  |
| 4 | BLOOM | MEG -40..50 | -40 | -18 | 5 | 28 | 50 |  |
| 5 | TREM | LD>AMP 0..90 | 0 | 22 | 45 | 68 | 90 |  |
| 6 | BITE | DIST 0..80; LEVEL 106..100 | 0/106 | 20/104 | 40/103 | 60/102 | 80/100 |  |
| 7 | CHORUS | CHOR 0..110 | 0 | 28 | 55 | 82 | 110 |  |
| 8 | SPACE | REV 0..127; DLY 0..30 | 0/0 | 32/8 | 64/15 | 95/22 | 127/30 |  |

Presets: MUSIC BOX, TEMPLE, ICE, CHIMES, ROBOT BELLS.

## PLUCK

| # | Macro | Targets | 0 | 25 | 50 | 75 | 100 | OK? |
|---|---|---|---|---|---|---|---|---|
| 1 | MORPH (JAM) | from the sound as set to: BRIT -> 80; DAMP -> 50; ACC -> 127; REV -> 80; DLY -> 30; LEVEL -6 (level, half dB) | as set | | | | the contrast |  |
| 2 | BRIGHT | BRIT 20..127; POS 40..20; LEVEL 106..100 | 20/40/106 | 47/35/104 | 74/30/103 | 100/25/102 | 127/20/100 |  |
| 3 | DAMP | DAMP 20..127 | 20 | 47 | 74 | 100 | 127 |  |
| 4 | BODY | STRC 10..120 | 10 | 38 | 65 | 92 | 120 |  |
| 5 | PICK | POS 0..110 | 0 | 28 | 55 | 82 | 110 |  |
| 6 | BOW | BOW 0..110; LEVEL 106..90 | 0/106 | 28/102 | 55/98 | 82/94 | 110/90 |  |
| 7 | ACCENT | ACC 30..127 | 30 | 54 | 78 | 103 | 127 |  |
| 8 | SPACE | REV 0..100; LEVEL 108..99 | 0/108 | 25/106 | 50/104 | 75/101 | 100/99 |  |

Presets: KOTO, NYLON, MUTED, HARP, BANJO-ISH.

## VOX

| # | Macro | Targets | 0 | 25 | 50 | 75 | 100 | OK? |
|---|---|---|---|---|---|---|---|---|
| 1 | MORPH (JAM) | from the sound as set to: BUZZ -> 127; Q -> 127; TALK -> 110; CHOR -> 90; SPRD -> 120; REV -> 80; LEVEL -7 (level, half dB) | as set | | | | the contrast |  |
| 2 | VOWEL | VOWL 0..127; VOWL2 127..0 | 0/127 | 32/95 | 64/64 | 95/32 | 127/0 |  |
| 3 | TALK | TALK 0..110 | 0 | 28 | 55 | 82 | 110 |  |
| 4 | BUZZ | BUZZ 10..127; LEVEL 108..100 | 10/108 | 39/106 | 68/104 | 98/102 | 127/100 |  |
| 5 | BREATH | BRTH 0..110 | 0 | 28 | 55 | 82 | 110 |  |
| 6 | Q | Q 10..127 | 10 | 39 | 68 | 98 | 127 |  |
| 7 | CHOIR | CHOR 0..90; SPRD 0..120; IMIX 0..40 | 0/0/0 | 22/30/10 | 45/60/20 | 68/90/30 | 90/120/40 |  |
| 8 | SPACE | REV 0..127; DLY 0..30; LEVEL 106..100 | 0/0/106 | 32/8/104 | 64/15/103 | 95/22/102 | 127/30/100 |  |

Presets: ROBOT, CHOIR BOY, WHISPER, YAMMER, MONK.

## WHEELHOUSE

| # | Macro | Targets | 0 | 25 | 50 | 75 | 100 | OK? |
|---|---|---|---|---|---|---|---|---|
| 1 | MORPH (JAM) | from the sound as set to: DRV -> 120; ROTR 2 from 50; CLICK -> 127; TOP -> 8; REV -> 60; LEVEL +0 (level, half dB) | as set | | | | the contrast |  |
| 2 | STOPS (SWITCH) | REG: 0 1 2 3 4 5 6 7 8 9 | 0 | 2 | 4 | 7 | 9 |  |
| 3 | DRIVE | DRV 0..127; LEVEL 104..110; IMIX 0..120; IA 20..127 | 0/104/0/20 | 32/106/30/47 | 64/107/60/74 | 95/108/90/100 | 127/110/120/127 |  |
| 4 | ROTOR (SWITCH) | ROTR: OFF SLOW FAST | OFF | OFF | SLOW | FAST | FAST |  |
| 5 | SUB | SUB -8..8 | -8 | -4 | 0 | 4 | 8 |  |
| 6 | BODY | BODY -8..8; LEVEL 120..102 | -8/120 | -4/116 | 0/111 | 4/106 | 8/102 |  |
| 7 | TOP | TOP -8..8 | -8 | -4 | 0 | 4 | 8 |  |
| 8 | SPACE | REV 0..90 | 0 | 22 | 45 | 68 | 90 |  |

Presets: SMOKY, SUNDAY SERVICE, ROCK OUT, BALLPARK, FLUTES.
