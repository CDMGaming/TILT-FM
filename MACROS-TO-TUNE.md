# MACROS TO TUNE (TFM instruments)

Generated from `tools/tfm_ins_spec.py` by `tools/gen_macros_doc.py`: every macro, its targets (`lo..hi`) and what each is at 0 / 25 / 50 / 75 / 100 % of the turn. Check each by ear and tick it, or change `lo` / `hi` in `tools/tfm_ins_spec.py` and `firmware/src/tfm_instr.c`. Rules (tfm-spec/TFM-INSTRUMENTS.md): a big musical change across the turn, no dead zone, 50 sounds like the instrument, no jumps (a SWITCH steps), level steady while a macro adds drive, no LEVEL macro.

## THUMP LAB

| # | Macro | Targets | 0 | 25 | 50 | 75 | 100 | OK? |
|---|---|---|---|---|---|---|---|---|
| 1 | PUNCH (JAM) | TUNE 70..34 EXP; DRV 0..80; ACC 40..120; LEVEL 108..94 | 70/0/40/108 | 68/20/60/104 | 61/40/80/101 | 50/60/100/98 | 34/80/120/94 |  |
| 2 | SMACK | SNAP 10..120; TONE 30..115; LN1 80..115; LN2 80..115 | 10/30/80/80 | 38/51/89/89 | 65/72/98/98 | 92/94/106/106 | 120/115/115/115 |  |
| 3 | TIME | DECY 8..120 EXP; LEVEL 104..96 | 8/104 | 15/102 | 36/100 | 71/98 | 120/96 |  |
| 4 | ROOM | REV 0..90; DLY 0..25 >70% | 0/0 | 22/0 | 45/0 | 68/4 | 90/25 |  |
| 5 | TUNE | TUNE 16..112 | 16 | 40 | 64 | 88 | 112 |  |
| 6 | DIRT | IA 0..110; IMIX 20..100; LEVEL 106..90 | 0/20/106 | 28/40/102 | 55/60/98 | 82/80/94 | 110/100/90 |  |
| 7 | HATS | LN3 50..120; LN4 50..120 | 50/50 | 68/68 | 85/85 | 102/102 | 120/120 |  |
| 8 | ECHO | DLY 0..80 | 0 | 20 | 40 | 60 | 80 |  |

Presets: WAREHOUSE, BOOM BAP, DISCO, TRASH CAN, ARCADE.

## SQUELCH

| # | Macro | Targets | 0 | 25 | 50 | 75 | 100 | OK? |
|---|---|---|---|---|---|---|---|---|
| 1 | CUTOFF (JAM) | CUT 18..112 EXP | 18 | 24 | 42 | 71 | 112 |  |
| 2 | RESO | RES 10..118; LEVEL 108..96 | 10/108 | 37/105 | 64/102 | 91/99 | 118/96 |  |
| 3 | ENV MOD | ED>FLT 0..60; DEC 30..55 | 0/30 | 15/36 | 30/42 | 45/49 | 60/55 |  |
| 4 | DECAY | DEC 20..100; REL 10..50 | 20/10 | 40/20 | 60/30 | 80/40 | 100/50 |  |
| 5 | BITE | DRV 0..120; LEVEL 108..88 | 0/108 | 30/103 | 60/98 | 90/93 | 120/88 |  |
| 6 | GNAW | IMIX 0..100; LEVEL 108..92 | 0/108 | 25/104 | 50/100 | 75/96 | 100/92 |  |
| 7 | WAVE | MIX 0..127 | 0 | 32 | 64 | 95 | 127 |  |
| 8 | SLIDE | GLIDE 0..80 EXP | 0 | 5 | 20 | 45 | 80 |  |

Presets: RUBBER, ACID RAIN, LOW RIDER, ZAPPER, GNARLY.

## WOBBLER

| # | Macro | Targets | 0 | 25 | 50 | 75 | 100 | OK? |
|---|---|---|---|---|---|---|---|---|
| 1 | WOB (JAM) | LD>FLT 0..63; RES 10..60 | 0/10 | 16/22 | 32/35 | 47/48 | 63/60 |  |
| 2 | RATE (SWITCH) | LSYNC: 1/1 1/2 1/4 1/8 8T 1/16 16T 1/32 | 1/1 | 1/2 | 1/8 | 1/16 | 1/32 |  |
| 3 | GROWL | IMIX 0..100; IA 20..90; LEVEL 108..92 | 0/20/108 | 25/38/104 | 50/55/100 | 75/72/96 | 100/90/92 |  |
| 4 | CUT | CUT 15..110 EXP | 15 | 21 | 39 | 68 | 110 |  |
| 5 | SPREAD | DTN 0..40; SPRD 0..100 | 0/0 | 10/25 | 20/50 | 30/75 | 40/100 |  |
| 6 | DIRT | DIST 0..90; LEVEL 106..92 | 0/106 | 22/102 | 45/99 | 68/96 | 90/92 |  |
| 7 | DROP (SWITCH) | TRANS: 0 -1 OCT -2 OCT | 0 | 0 | -1 OCT | -2 OCT | -2 OCT |  |
| 8 | SLIDE | GLIDE 0..80 | 0 | 20 | 40 | 60 | 80 |  |

Presets: HALF STEP, YOY, TRASH FLOOR, LASER WOB, SUBWAY.

## CHORDETTE

| # | Macro | Targets | 0 | 25 | 50 | 75 | 100 | OK? |
|---|---|---|---|---|---|---|---|---|
| 1 | CHORD (SWITCH) | CHRD: AUTO AUTO 7 MAJ MIN 7 MAJ7 MIN7 SUS4 POWER | AUTO | MAJ | 7 | MIN7 | POWER |  |
| 2 | VOICE (SWITCH) | VOIC: CLOSE OPEN DROP2 SPREAD STACK | CLOSE | OPEN | DROP2 | SPREAD | STACK |  |
| 3 | WARM (JAM) | CUT 30..110; RES 40..10 | 30/40 | 50/32 | 70/25 | 90/18 | 110/10 |  |
| 4 | SWELL | ATK 0..90 EXP; REL 10..110 | 0/10 | 6/35 | 22/60 | 51/85 | 90/110 |  |
| 5 | SHIMMER | CHOR 0..60; DTN 0..20 | 0/0 | 15/5 | 30/10 | 45/15 | 60/20 |  |
| 6 | SPACE | REV 0..90; DLY 0..40 >70% | 0/0 | 22/0 | 45/0 | 68/7 | 90/40 |  |
| 7 | PUMP | LD>AMP 0..80 | 0 | 20 | 40 | 60 | 80 |  |
| 8 | AGE | IMIX 0..70; IA 10..90 | 0/10 | 18/30 | 35/50 | 52/70 | 70/90 |  |

Presets: HOUSE STAB, SUNDAY, VELVET, PUMPER, MIXTAPE.

## RAZOR

| # | Macro | Targets | 0 | 25 | 50 | 75 | 100 | OK? |
|---|---|---|---|---|---|---|---|---|
| 1 | SCREAM | RES 10..110; DRV 0..70; LEVEL 108..92 | 10/0/108 | 35/18/104 | 60/35/100 | 85/52/96 | 110/70/92 |  |
| 2 | SWEEP | ED>FLT -20..60; DEC 20..70 | -20/20 | 0/32 | 20/45 | 40/58 | 60/70 |  |
| 3 | CUT (JAM) | CUT 30..120 EXP | 30 | 36 | 52 | 81 | 120 |  |
| 4 | GLIDE | GLIDE 0..80 | 0 | 20 | 40 | 60 | 80 |  |
| 5 | WIDTH | DETUNE 0..80; SPRD 0..127 | 0/0 | 20/32 | 40/64 | 60/95 | 80/127 |  |
| 6 | ATTACK | ATK 0..80 | 0 | 20 | 40 | 60 | 80 |  |
| 7 | VIB | LD>PIT 0..2 | 0 | 0 | 1 | 2 | 2 |  |
| 8 | ECHO | DLY 0..90 | 0 | 22 | 45 | 68 | 90 |  |

Presets: HERO, TRANCE GATE, ARENA, SINE RIDER, HOT WIRE.

## HAZE

| # | Macro | Targets | 0 | 25 | 50 | 75 | 100 | OK? |
|---|---|---|---|---|---|---|---|---|
| 1 | DRIFT | LD>PIT 0..2 >70%; DTN 5..30 | 0/5 | 0/11 | 0/18 | 0/24 | 2/30 |  |
| 2 | DUST | IMIX 0..100 | 0 | 25 | 50 | 75 | 100 |  |
| 3 | SPACE | REV 10..127 | 10 | 39 | 68 | 98 | 127 |  |
| 4 | BLOOM (JAM) | ATK 10..120 EXP; REL 30..120 | 10/30 | 17/52 | 38/75 | 72/98 | 120/120 |  |
| 5 | TONE | CUT 10..127 EXP | 10 | 17 | 39 | 76 | 127 |  |
| 6 | SHINE | CHOR 0..110 | 0 | 28 | 55 | 82 | 110 |  |
| 7 | MOVE | LD>FLT 0..60 | 0 | 15 | 30 | 45 | 60 |  |
| 8 | HOLD | SUS 60..127; REL 40..70 | 60/40 | 77/48 | 94/55 | 110/62 | 127/70 |  |

Presets: FOG, MORNING, VHS, AURORA, DEEP END.

## PIXEL

| # | Macro | Targets | 0 | 25 | 50 | 75 | 100 | OK? |
|---|---|---|---|---|---|---|---|---|
| 1 | JUMP (SWITCH) | ARP: OFF OCT 5TH | OFF | OFF | OCT | 5TH | 5TH |  |
| 2 | WAVE (SWITCH) | WAVE: PULSE TRI SAW NOISE WAVE | PULSE | TRI | SAW | NOISE | WAVE |  |
| 3 | SWEEP (JAM) | SWP 0..25 | 0 | 6 | 12 | 19 | 25 |  |
| 4 | CRUSH | CRSH 0..120 | 0 | 30 | 60 | 90 | 120 |  |
| 5 | VIB | VIB 0..110 | 0 | 28 | 55 | 82 | 110 |  |
| 6 | DECAY | DEC 15..100 | 15 | 36 | 58 | 79 | 100 |  |
| 7 | TONE | TONE 20..127 | 20 | 47 | 74 | 100 | 127 |  |
| 8 | DUTY | DUTY 0..120 | 0 | 30 | 60 | 90 | 120 |  |

Presets: 1UP, BOSS LEVEL, WARP ZONE, HIGH SCORE, GLITCH.

## TINE

| # | Macro | Targets | 0 | 25 | 50 | 75 | 100 | OK? |
|---|---|---|---|---|---|---|---|---|
| 1 | BARK (JAM) | MLVL -40..40 | -40 | -20 | 0 | 20 | 40 |  |
| 2 | TINE | MEG -40..40 | -40 | -20 | 0 | 20 | 40 |  |
| 3 | TREM | LD>AMP 0..90 | 0 | 22 | 45 | 68 | 90 |  |
| 4 | CHORUS | CHOR 0..110 | 0 | 28 | 55 | 82 | 110 |  |
| 5 | DECAY | DEC 30..110 | 30 | 50 | 70 | 90 | 110 |  |
| 6 | WARMTH | IMIX 0..80; LEVEL 106..96 | 0/106 | 20/104 | 40/101 | 60/98 | 80/96 |  |
| 7 | SPACE | REV 0..100 | 0 | 25 | 50 | 75 | 100 |  |
| 8 | REL | REL 10..100 | 10 | 32 | 55 | 78 | 100 |  |

Presets: SUITCASE, LOUNGE, BARK BARK, DREAMY, CHURCH BASEMENT.

## GLINT

| # | Macro | Targets | 0 | 25 | 50 | 75 | 100 | OK? |
|---|---|---|---|---|---|---|---|---|
| 1 | METAL (JAM) | MLVL -40..50 | -40 | -18 | 5 | 28 | 50 |  |
| 2 | RATIO (SWITCH) | MRAT: -4 -2 0 2 4 | -4 | -2 | 0 | 2 | 4 |  |
| 3 | BLOOM | MEG -40..50 | -40 | -18 | 5 | 28 | 50 |  |
| 4 | DECAY | DEC 30..120 | 30 | 52 | 75 | 98 | 120 |  |
| 5 | BITE | FB -5..7 | -5 | -2 | 1 | 4 | 7 |  |
| 6 | ATTACK | ATK 0..80 | 0 | 20 | 40 | 60 | 80 |  |
| 7 | SPACE | REV 0..110 | 0 | 28 | 55 | 82 | 110 |  |
| 8 | SHIMMER | IMIX 0..90; DLY 0..40 | 0/0 | 22/10 | 45/20 | 68/30 | 90/40 |  |

Presets: MUSIC BOX, TEMPLE, ICE, CHIMES, ROBOT BELLS.

## PLUCK

| # | Macro | Targets | 0 | 25 | 50 | 75 | 100 | OK? |
|---|---|---|---|---|---|---|---|---|
| 1 | BRIGHT (JAM) | BRIT 20..127; POS 40..20 | 20/40 | 47/35 | 74/30 | 100/25 | 127/20 |  |
| 2 | DAMP | DAMP 20..127 | 20 | 47 | 74 | 100 | 127 |  |
| 3 | BODY | STRC 10..120 | 10 | 38 | 65 | 92 | 120 |  |
| 4 | PICK | POS 0..110 | 0 | 28 | 55 | 82 | 110 |  |
| 5 | BOW | BOW 0..110 | 0 | 28 | 55 | 82 | 110 |  |
| 6 | ACCENT | ACC 30..127 | 30 | 54 | 78 | 103 | 127 |  |
| 7 | SPACE | REV 0..100 | 0 | 25 | 50 | 75 | 100 |  |
| 8 | ECHO | DLY 0..80 | 0 | 20 | 40 | 60 | 80 |  |

Presets: KOTO, NYLON, MUTED, HARP, BANJO-ISH.

## VOX

| # | Macro | Targets | 0 | 25 | 50 | 75 | 100 | OK? |
|---|---|---|---|---|---|---|---|---|
| 1 | VOWEL (JAM) | VOWL 0..127 | 0 | 32 | 64 | 95 | 127 |  |
| 2 | TALK | TALK 0..110 | 0 | 28 | 55 | 82 | 110 |  |
| 3 | BUZZ | BUZZ 10..127 | 10 | 39 | 68 | 98 | 127 |  |
| 4 | BREATH | BRTH 0..110 | 0 | 28 | 55 | 82 | 110 |  |
| 5 | SHIFT (SWITCH) | SHIFT: -12 -7 -5 0 5 7 12 | -12 | -7 | 0 | 7 | 12 |  |
| 6 | Q | Q 10..127 | 10 | 39 | 68 | 98 | 127 |  |
| 7 | CHOIR | CHOR 0..90; SPRD 0..120; IMIX 0..40 | 0/0/0 | 22/30/10 | 45/60/20 | 68/90/30 | 90/120/40 |  |
| 8 | SPACE | REV 0..100 | 0 | 25 | 50 | 75 | 100 |  |

Presets: ROBOT, CHOIR BOY, WHISPER, YAMMER, MONK.

## WHEELHOUSE

| # | Macro | Targets | 0 | 25 | 50 | 75 | 100 | OK? |
|---|---|---|---|---|---|---|---|---|
| 1 | STOPS (SWITCH) | REG: 0 1 2 3 4 5 6 7 8 9 | 0 | 2 | 4 | 7 | 9 |  |
| 2 | DRIVE (JAM) | DRV 0..120; LEVEL 108..92 | 0/108 | 30/104 | 60/100 | 90/96 | 120/92 |  |
| 3 | ROTOR (SWITCH) | ROTR: OFF SLOW FAST | OFF | OFF | SLOW | FAST | FAST |  |
| 4 | CLICK | CLICK 0..127 | 0 | 32 | 64 | 95 | 127 |  |
| 5 | SUB | SUB -8..8 | -8 | -4 | 0 | 4 | 8 |  |
| 6 | BODY | BODY -8..8 | -8 | -4 | 0 | 4 | 8 |  |
| 7 | TOP | TOP -8..8 | -8 | -4 | 0 | 4 | 8 |  |
| 8 | SPACE | REV 0..90 | 0 | 22 | 45 | 68 | 90 |  |

Presets: SMOKY, SUNDAY SERVICE, ROCK OUT, BALLPARK, FLUTES.
