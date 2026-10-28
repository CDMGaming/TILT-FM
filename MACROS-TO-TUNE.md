# MACROS TO TUNE (TFM instruments)

Generated from `firmware/src/tfm_instr.c` (TFM_INS). Each instrument has 8 macros: KNOB 1-4 on EDIT > MACROS, 5-8 on MACROS 2. A macro sets its targets in a straight line in the parameter's own units (CUT, the envelope times and the LFO rate are already exponential there). Check each by ear at 0 / 25 / 50 / 75 / 100 % and tick it, or note the range to change (edit `lo`, `hi` in the table).

Rules: a big musical change across the whole turn, no dead zone, noon sounds good, no jumps (a stepped one is marked SWITCH), level steady while driving or opening the filter, extremes may be wild.

## THUMP LAB (DRUM)

| # | Macro | Targets (lo..hi) | 0 | 25 | 50 | 75 | 100 | OK? |
|---|---|---|---|---|---|---|---|---|
| 1 | TUNE | TUNE 16..112 | 16 | 40 | 64 | 88 | 112 |  |
| 2 | DECAY | DECY 8..120 | 8 | 36 | 64 | 92 | 120 |  |
| 3 | SNAP | SNAP 8..120 | 8 | 36 | 64 | 92 | 120 |  |
| 4 | TONE | TONE 10..120 | 10 | 38 | 65 | 92 | 120 |  |
| 5 | DRIVE | DRV 0..110; LEVEL 108..90 | 0/108 | 28/104 | 55/99 | 82/94 | 110/90 |  |
| 6 | ACCNT | ACC 30..127 | 30 | 54 | 78 | 103 | 127 |  |
| 7 | ROOM | REVERB send 0..90 | 0 | 22 | 45 | 68 | 90 |  |
| 8 | LEVEL | LEVEL 70..120 | 70 | 82 | 95 | 108 | 120 |  |

## SQUELCH (ANALOG)

| # | Macro | Targets (lo..hi) | 0 | 25 | 50 | 75 | 100 | OK? |
|---|---|---|---|---|---|---|---|---|
| 1 | CUT | CUT 18..112 | 18 | 42 | 65 | 88 | 112 |  |
| 2 | RESO | RES 10..118 | 10 | 37 | 64 | 91 | 118 |  |
| 3 | ENV | ENV>FLT 0..60 | 0 | 15 | 30 | 45 | 60 |  |
| 4 | DECAY | DEC 20..100 | 20 | 40 | 60 | 80 | 100 |  |
| 5 | BITE | DRV 0..120; LEVEL 108..88 | 0/108 | 30/103 | 60/98 | 90/93 | 120/88 |  |
| 6 | GNAW | DIST send 0..100; LEVEL 108..92 | 0/108 | 25/104 | 50/100 | 75/96 | 100/92 |  |
| 7 | MIX | MIX 0..127 | 0 | 32 | 64 | 95 | 127 |  |
| 8 | GLIDE | GLIDE 0..80 | 0 | 20 | 40 | 60 | 80 |  |

## WOBBLER (TRIO)

| # | Macro | Targets (lo..hi) | 0 | 25 | 50 | 75 | 100 | OK? |
|---|---|---|---|---|---|---|---|---|
| 1 | WOB | LFO>FLT 0..63 | 0 | 16 | 32 | 47 | 63 |  |
| 2 | RATE (SWITCH) | LFO SYNC 3..10 | 3 | 5 | 6 | 8 | 10 |  |
| 3 | GROWL | RES 10..120 | 10 | 38 | 65 | 92 | 120 |  |
| 4 | CRUSH | INSERT MIX 0..110 | 0 | 28 | 55 | 82 | 110 |  |
| 5 | CUT | CUT 15..110 | 15 | 39 | 62 | 86 | 110 |  |
| 6 | SPRD | DTN 0..40 | 0 | 10 | 20 | 30 | 40 |  |
| 7 | GLIDE | GLIDE 0..80 | 0 | 20 | 40 | 60 | 80 |  |
| 8 | LEVEL | LEVEL 70..120 | 70 | 82 | 95 | 108 | 120 |  |

## CHORDETTE (ANALOG)

| # | Macro | Targets (lo..hi) | 0 | 25 | 50 | 75 | 100 | OK? |
|---|---|---|---|---|---|---|---|---|
| 1 | WARM | CUT 30..110 | 30 | 50 | 70 | 90 | 110 |  |
| 2 | ATK | ATK 0..90 | 0 | 22 | 45 | 68 | 90 |  |
| 3 | VOICE (SWITCH) | VOICING 0..4 | 0 | 1 | 2 | 3 | 4 |  |
| 4 | CHORD (SWITCH) | CHORD 1..9 | 1 | 3 | 5 | 7 | 9 |  |
| 5 | REL | REL 10..110 | 10 | 35 | 60 | 85 | 110 |  |
| 6 | RES | RES 0..90 | 0 | 22 | 45 | 68 | 90 |  |
| 7 | SPACE | REVERB send 0..90; CHORUS send 0..60 | 0/0 | 22/15 | 45/30 | 68/45 | 90/60 |  |
| 8 | LEVEL | LEVEL 70..120 | 70 | 82 | 95 | 108 | 120 |  |

## RAZOR (ANALOG)

| # | Macro | Targets (lo..hi) | 0 | 25 | 50 | 75 | 100 | OK? |
|---|---|---|---|---|---|---|---|---|
| 1 | SCRM | RES 10..110; DRV 0..70 | 10/0 | 35/18 | 60/35 | 85/52 | 110/70 |  |
| 2 | SWEEP | ENV>FLT -20..60 | -20 | 0 | 20 | 40 | 60 |  |
| 3 | CUT | CUT 30..120 | 30 | 52 | 75 | 98 | 120 |  |
| 4 | GLIDE | GLIDE 0..80 | 0 | 20 | 40 | 60 | 80 |  |
| 5 | DTUN | DTN 0..80 | 0 | 20 | 40 | 60 | 80 |  |
| 6 | SPRD | SPREAD 0..127 | 0 | 32 | 64 | 95 | 127 |  |
| 7 | ATK | ATK 0..80 | 0 | 20 | 40 | 60 | 80 |  |
| 8 | LEVEL | LEVEL 70..120 | 70 | 82 | 95 | 108 | 120 |  |

## HAZE (TRIO)

| # | Macro | Targets (lo..hi) | 0 | 25 | 50 | 75 | 100 | OK? |
|---|---|---|---|---|---|---|---|---|
| 1 | DRIFT | LFO>PIT 0..14 | 0 | 4 | 7 | 10 | 14 |  |
| 2 | DUST | INSERT MIX 0..100 | 0 | 25 | 50 | 75 | 100 |  |
| 3 | SPACE | REVERB send 10..127 | 10 | 39 | 68 | 98 | 127 |  |
| 4 | LOW | CUT 10..70 | 10 | 25 | 40 | 55 | 70 |  |
| 5 | HI | CUT 60..127 | 60 | 77 | 94 | 110 | 127 |  |
| 6 | SHINE | CHORUS send 0..110 | 0 | 28 | 55 | 82 | 110 |  |
| 7 | ATK | ATK 10..120 | 10 | 38 | 65 | 92 | 120 |  |
| 8 | LEVEL | LEVEL 70..120 | 70 | 82 | 95 | 108 | 120 |  |

## PIXEL (LOFI)

| # | Macro | Targets (lo..hi) | 0 | 25 | 50 | 75 | 100 | OK? |
|---|---|---|---|---|---|---|---|---|
| 1 | JUMP (SWITCH) | ARP 0..3 | 0 | 1 | 2 | 2 | 3 |  |
| 2 | SWEEP | SWP 0..110 | 0 | 28 | 55 | 82 | 110 |  |
| 3 | VIB | VIB 0..110 | 0 | 28 | 55 | 82 | 110 |  |
| 4 | CRUSH | CRSH 0..120 | 0 | 30 | 60 | 90 | 120 |  |
| 5 | WAVE (SWITCH) | WAVE 0..4 | 0 | 1 | 2 | 3 | 4 |  |
| 6 | DECAY | DEC 15..100 | 15 | 36 | 58 | 79 | 100 |  |
| 7 | TONE | TONE 20..127 | 20 | 47 | 74 | 100 | 127 |  |
| 8 | LEVEL | LEVEL 70..120 | 70 | 82 | 95 | 108 | 120 |  |

## TINE (FM6)

| # | Macro | Targets (lo..hi) | 0 | 25 | 50 | 75 | 100 | OK? |
|---|---|---|---|---|---|---|---|---|
| 1 | BARK | MLVL -40..40 | -40 | -20 | 0 | 20 | 40 |  |
| 2 | TINE | MEG -40..40 | -40 | -20 | 0 | 20 | 40 |  |
| 3 | TREM | LFO>AMP 0..90 | 0 | 22 | 45 | 68 | 90 |  |
| 4 | CHOR | CHORUS send 0..110 | 0 | 28 | 55 | 82 | 110 |  |
| 5 | DECAY | DEC 30..110 | 30 | 50 | 70 | 90 | 110 |  |
| 6 | REL | REL 10..100 | 10 | 32 | 55 | 78 | 100 |  |
| 7 | SPACE | REVERB send 0..100 | 0 | 25 | 50 | 75 | 100 |  |
| 8 | LEVEL | LEVEL 70..120 | 70 | 82 | 95 | 108 | 120 |  |

## GLINT (FM6)

| # | Macro | Targets (lo..hi) | 0 | 25 | 50 | 75 | 100 | OK? |
|---|---|---|---|---|---|---|---|---|
| 1 | METAL | MLVL -40..50 | -40 | -18 | 5 | 28 | 50 |  |
| 2 | RATIO (SWITCH) | MRAT -8..8 | -8 | -4 | 0 | 4 | 8 |  |
| 3 | BITE | FB -5..7 | -5 | -2 | 1 | 4 | 7 |  |
| 4 | BLOOM | MEG -40..50 | -40 | -18 | 5 | 28 | 50 |  |
| 5 | DECAY | DEC 30..120 | 30 | 52 | 75 | 98 | 120 |  |
| 6 | ATK | ATK 0..80 | 0 | 20 | 40 | 60 | 80 |  |
| 7 | SPACE | REVERB send 0..110 | 0 | 28 | 55 | 82 | 110 |  |
| 8 | LEVEL | LEVEL 70..120 | 70 | 82 | 95 | 108 | 120 |  |

## PLUCK (PHYS)

| # | Macro | Targets (lo..hi) | 0 | 25 | 50 | 75 | 100 | OK? |
|---|---|---|---|---|---|---|---|---|
| 1 | BRITE | BRIT 20..127 | 20 | 47 | 74 | 100 | 127 |  |
| 2 | DAMP | DAMP 20..127 | 20 | 47 | 74 | 100 | 127 |  |
| 3 | POS | POS 0..110 | 0 | 28 | 55 | 82 | 110 |  |
| 4 | BODY | STRC 10..120 | 10 | 38 | 65 | 92 | 120 |  |
| 5 | BOW | BOW 0..110 | 0 | 28 | 55 | 82 | 110 |  |
| 6 | ACCNT | ACC 30..127 | 30 | 54 | 78 | 103 | 127 |  |
| 7 | SPACE | REVERB send 0..100 | 0 | 25 | 50 | 75 | 100 |  |
| 8 | LEVEL | LEVEL 70..120 | 70 | 82 | 95 | 108 | 120 |  |

## VOX (FORMANT)

| # | Macro | Targets (lo..hi) | 0 | 25 | 50 | 75 | 100 | OK? |
|---|---|---|---|---|---|---|---|---|
| 1 | VOWEL | VOWL 0..127 | 0 | 32 | 64 | 95 | 127 |  |
| 2 | TALK | TALK 0..110 | 0 | 28 | 55 | 82 | 110 |  |
| 3 | BUZZ | BUZZ 10..127 | 10 | 39 | 68 | 98 | 127 |  |
| 4 | BRTH | BRTH 0..110 | 0 | 28 | 55 | 82 | 110 |  |
| 5 | SHIFT (SWITCH) | SHIFT -12..12 | -12 | -6 | 0 | 6 | 12 |  |
| 6 | Q | Q 10..127 | 10 | 39 | 68 | 98 | 127 |  |
| 7 | SPACE | REVERB send 0..100 | 0 | 25 | 50 | 75 | 100 |  |
| 8 | LEVEL | LEVEL 70..120 | 70 | 82 | 95 | 108 | 120 |  |

## WHEELHOUSE (WHEEL)

| # | Macro | Targets (lo..hi) | 0 | 25 | 50 | 75 | 100 | OK? |
|---|---|---|---|---|---|---|---|---|
| 1 | REG (SWITCH) | REG 0..15 | 0 | 4 | 8 | 11 | 15 |  |
| 2 | SUB | SUB -8..8 | -8 | -4 | 0 | 4 | 8 |  |
| 3 | BODY | BODY -8..8 | -8 | -4 | 0 | 4 | 8 |  |
| 4 | TOP | TOP -8..8 | -8 | -4 | 0 | 4 | 8 |  |
| 5 | CLICK | CLICK 0..127 | 0 | 32 | 64 | 95 | 127 |  |
| 6 | DRIVE | DRV 0..120; LEVEL 108..92 | 0/108 | 30/104 | 60/100 | 90/96 | 120/92 |  |
| 7 | ROTOR (SWITCH) | ROTR 0..2 | 0 | 0 | 1 | 2 | 2 |  |
| 8 | LEVEL | LEVEL 70..120 | 70 | 82 | 95 | 108 | 120 |  |
