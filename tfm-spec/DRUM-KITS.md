# TILT-FM drum kits (round 13, item 2)

Fifteen new kits for the DRUM engine, one per drum preset: five each for THUMP LAB, KITBOX and GARAGE. Data only: new rows in
`drum_voice.c` DV_KIT (the same voice runs as the older kits, new numbers), appended to the KIT list (stored values 12..26, in the
order below). The older stored values 0..11 are untouched (`tests/drum_test.c` keeps the renders of the old kits as they were).
Each preset loads its own kit (`P_E0`); the instrument's older, un-named sounds keep their old kit (THUMP LAB 9 DEEP, KITBOX 5 "10",
GARAGE 8 "77").

Measured by `tests/drum_kits_test.c` (run by `tests/run_tests.sh`, which also writes the WAV files): every lane struck alone at the
preset's own settings through the real signal path (the track's insert, DRV, LEVEL and trims; the room and the echo off for the
measuring only). **Pitch**: the lane's designed pitch (for the hats, the clap and the cymbal the centre of their band or high band,
in the table marked `band`). **Decay**: milliseconds until the lane is 40 dB below its loudest 2.5 ms. **Peak**: dBFS at the output.

## Listen

`tfm-spec/built/drums/<instrument>-<preset>.wav`, one per kit (44.1 kHz, 16 bit, mono, the preset's room and echo on): the eight
lanes one by one (1 s apart, the cymbal 3 s), then one bar of a simple beat at 120 BPM.


## THUMP LAB

### WAREHOUSE (KIT 12)

A 909-like room: a punchy kick with a click on top, a bright snare, a tight four-burst clap, metallic six-square hats, a ride.

| lane | piece | pitch (Hz) | decay (ms, -40 dB) | peak (dBFS) | centroid (Hz) | what it is |
|---|---|---|---|---|---|---|
| 1 KICK (KICK) | kick (PUNCH) | 52 | 721 | -12.4 | 1000 | 52 Hz, +13 st over 22 ms, tau 105 ms |
| 2 SNARE (SNARE) | snare | 190 | 190 | -11.9 | 7389 | shell 190 Hz x1.8, noise 5.2 kHz, tau 40 ms |
| 3 CLAP (CLAP) | clap | 1500 band | 200 | -14.5 | 8101 | 4 bursts 7.5 ms apart, band 1.5 kHz, tail tau 55 ms |
| 4 HAT CL (HATCL) | closed hat | 7513 band | 95 | -17.4 | 9968 | row 0 from 205 Hz, band 7.5 kHz, tau 20 ms |
| 5 HAT OP (HATOP) | open hat | 7513 band | 521 | -17.4 | 9672 | row 0 from 205 Hz, band 7.5 kHz, tau 120 ms |
| 6 TOM (TOM) | tom | 118 | 788 | -15.8 | 1460 | 118 Hz bent 5 st, tau 120 ms |
| 7 RIM (RIM) | rim | 470 | 15 | -16.3 | 4539 | 470 Hz + +22 st, tau 2.2 ms |
| 8 BELL (CYM) | cymbal | 6204 band | 2659 | -17.2 | 6520 | row 1 from 400 Hz, bands 6.2 / 3.2 kHz, ring tau 700 ms |

### BOOM BAP (KIT 13)

A dusty 808: a long low boom with a slow pitch drop, a rimshot-style snare, short hand claps, a clave, closed hats with dust on them.

| lane | piece | pitch (Hz) | decay (ms, -40 dB) | peak (dBFS) | centroid (Hz) | what it is |
|---|---|---|---|---|---|---|
| 1 KICK (KICK) | kick (PUNCH) | 50 | 2115 | -15.0 | 1435 | 50 Hz, +5 st over 120 ms, tau 280 ms |
| 2 SNARE (SNARE) | snare | 330 | 274 | -16.9 | 3564 | shell 330 Hz x1.6, noise 3.0 kHz, tau 55 ms |
| 3 CLAP (CLAP) | clap | 1050 band | 185 | -17.5 | 5037 | 3 bursts 11 ms apart, band 1.1 kHz, tail tau 45 ms |
| 4 HAT CL (HATCL) | closed hat | 6006 band | 92 | -24.1 | 11352 | row 0 from 205 Hz, band 6.0 kHz, tau 16 ms |
| 5 HAT OP (HATOP) | open hat | 5508 band | 596 | -23.7 | 10556 | row 0 from 205 Hz, band 5.5 kHz, tau 110 ms |
| 6 TOM (TOM) | tom | 82 | 1315 | -19.0 | 1428 | 82 Hz bent 6 st, tau 180 ms |
| 7 RIM (CLAVE) | clave | 2600 | 107 | -20.7 | 6500 | 2600 Hz + +5 st, tau 22 ms |
| 8 BELL (CYM) | cymbal | 5198 band | 1773 | -21.2 | 6690 | row 1 from 400 Hz, bands 5.2 / 2.6 kHz, ring tau 450 ms |

### DISCO (KIT 14)

Four on the floor: a soft thump, a snappy clap and snare, ticky closed hats against an open hat that hisses, high tuned toms, a cowbell on lane 8.

| lane | piece | pitch (Hz) | decay (ms, -40 dB) | peak (dBFS) | centroid (Hz) | what it is |
|---|---|---|---|---|---|---|
| 1 KICK (KICK) | kick (PUNCH) | 58 | 1262 | -16.3 | 1340 | 58 Hz, +9 st over 28 ms, tau 120 ms |
| 2 SNARE (SNARE) | snare | 215 | 289 | -14.5 | 9308 | shell 215 Hz x1.6, noise 6.8 kHz, tau 40 ms |
| 3 CLAP (CLAP) | clap | 1900 band | 135 | -17.0 | 8243 | 3 bursts 6 ms apart, band 1.9 kHz, tail tau 32 ms |
| 4 HAT CL (HATCL) | closed hat | 8193 band | 70 | -20.9 | 12476 | row 0 from 205 Hz, band 8.2 kHz, tau 11 ms |
| 5 HAT OP (HATOP) | open hat | 9500 band | 1497 | -20.8 | 12635 | hiss: noise through a wide band, tau 220 ms |
| 6 TOM (TOM) | tom | 205 | 1060 | -19.5 | 1641 | 205 Hz bent 3 st, tau 110 ms |
| 7 RIM (RIM) | rim | 620 | 20 | -19.7 | 5967 | 620 Hz + +20 st, tau 2.5 ms |
| 8 BELL (BELL) | bell | 560 | 661 | -19.7 | 3052 | squares 560 / 830 Hz, tau 90 ms |

### TRASH CAN (KIT 15)

Hard and driven: a distorted short kick, a noisy snare, a crunchy six-burst clap, a clipped tom, a trash-lid cymbal.

| lane | piece | pitch (Hz) | decay (ms, -40 dB) | peak (dBFS) | centroid (Hz) | what it is |
|---|---|---|---|---|---|---|
| 1 KICK (KICK) | kick (PUNCH) | 70 | 516 | -12.2 | 2149 | 70 Hz, +11 st over 10 ms, tau 65 ms |
| 2 SNARE (SNARE) | snare | 250 | 277 | -16.1 | 6472 | shell 250 Hz x1.3, noise 3.5 kHz, tau 45 ms |
| 3 CLAP (CLAP) | clap | 900 band | 459 | -16.5 | 7428 | 6 bursts 5 ms apart, band 0.9 kHz, tail tau 85 ms |
| 4 HAT CL (HATCL) | closed hat | 7988 band | 125 | -19.6 | 10217 | row 3 from 205 Hz, band 8.0 kHz, tau 20 ms |
| 5 HAT OP (HATOP) | open hat | 7988 band | 871 | -20.1 | 10343 | row 3 from 205 Hz, band 8.0 kHz, tau 150 ms |
| 6 TOM (TOM) | tom | 92 | 708 | -16.2 | 1641 | 92 Hz bent 4 st, tau 90 ms |
| 7 RIM (RIM) | rim | 700 | 20 | -17.5 | 3578 | 700 Hz + +20 st, tau 3 ms |
| 8 BELL (CYM) | cymbal | 4293 band | 1848 | -18.2 | 6281 | row 3 from 400 Hz, bands 4.3 / 2.1 kHz, ring tau 380 ms |

### ARCADE (KIT 16)

8-bit: a blip kick, a noise-burst snare, a two-burst clap, hats as thin square clicks, a pitched blip tom, the noise channel as the crash.

| lane | piece | pitch (Hz) | decay (ms, -40 dB) | peak (dBFS) | centroid (Hz) | what it is |
|---|---|---|---|---|---|---|
| 1 KICK (KICK) | kick (PUNCH) | 80 | 519 | -15.2 | 1968 | 80 Hz, +24 st over 35 ms, tau 60 ms |
| 2 SNARE (SNARE) | snare | 300 | 289 | -15.1 | 9333 | shell 300 Hz x1, noise 6.5 kHz, tau 50 ms |
| 3 CLAP (CLAP) | clap | 2200 band | 65 | -16.1 | 6876 | 2 bursts 10 ms apart, band 2.2 kHz, tail tau 22 ms |
| 4 HAT CL (HATCL) | closed hat | 6204 band | 35 | -19.7 | 9905 | two squares a fifth apart (5.2 kHz), tau 6 ms |
| 5 HAT OP (HATOP) | open hat | 6204 band | 317 | -19.6 | 10323 | two squares, tau 55 ms |
| 6 TOM (TOM) | tom | 330 | 586 | -17.3 | 3381 | 330 Hz bent 14 st, tau 70 ms |
| 7 RIM (CLAVE) | clave | 1760 | 75 | -17.8 | 4946 | 1760 Hz + +12 st, tau 14 ms |
| 8 BELL (CYM) | cymbal | 6989 band | 1404 | -19.9 | 9430 | the noise channel: noise through two bands, ring tau 480 ms |

## KITBOX

### DENTED (KIT 17)

The classic little box: a thumpy kick, ticky four-square hats, a clave, a small tin cymbal. Everything but the cymbal is over within 130 ms.

| lane | piece | pitch (Hz) | decay (ms, -40 dB) | peak (dBFS) | centroid (Hz) | what it is |
|---|---|---|---|---|---|---|
| 1 KICK (KICK) | kick (PUNCH) | 68 | 132 | -7.1 | 784 | 68 Hz, +6 st over 10 ms, tau 16 ms |
| 2 SNARE (SNARE) | snare | 185 | 142 | -6.4 | 6671 | shell 185 Hz x1, noise 4.8 kHz, tau 17 ms |
| 3 CLAP (CLAP) | clap | 1300 band | 67 | -7.8 | 5698 | 3 bursts 7 ms apart, band 1.3 kHz, tail tau 14 ms |
| 4 HAT CL (HATCL) | closed hat | 7194 band | 52 | -14.3 | 7673 | four squares, band 7.2 kHz, tau 11 ms |
| 5 HAT OP (HATOP) | open hat | 7194 band | 105 | -13.5 | 7703 | four squares, tau 22 ms |
| 6 TOM (TOM) | tom | 150 | 142 | -6.6 | 1003 | 150 Hz bent 4 st, tau 19 ms |
| 7 RIM (CLAVE) | clave | 2400 | 97 | -10.9 | 5975 | 2400 Hz + +5 st, tau 9 ms |
| 8 BELL (CYM) | cymbal | 4201 band | 1741 | -8.3 | 4541 | row 2 from 290 Hz, bands 4.2 / 2.4 kHz, ring tau 450 ms |

### TIN ROOF (KIT 18)

Metallic and tinny: a high tuned kick and snare, narrow-band metal hats that ring for a moment, a tinny tom and rim.

| lane | piece | pitch (Hz) | decay (ms, -40 dB) | peak (dBFS) | centroid (Hz) | what it is |
|---|---|---|---|---|---|---|
| 1 KICK (KICK) | kick (PUNCH) | 105 | 130 | -7.7 | 1113 | 105 Hz, +9 st over 6 ms, tau 16 ms |
| 2 SNARE (SNARE) | snare | 300 | 132 | -7.4 | 7734 | shell 300 Hz x1.5, noise 6.5 kHz, tau 9 ms |
| 3 CLAP (CLAP) | clap | 2100 band | 115 | -6.7 | 5704 | 2 bursts 6 ms apart, band 2.1 kHz, tail tau 9 ms |
| 4 HAT CL (HATCL) | closed hat | 8999 band | 47 | -9.2 | 8411 | tinny cluster, narrow band 9 kHz, tau 10 ms |
| 5 HAT OP (HATOP) | open hat | 8999 band | 82 | -9.0 | 8631 | tinny cluster, tau 16 ms |
| 6 TOM (TOM) | tom | 230 | 102 | -6.7 | 1838 | 230 Hz bent 3 st, tau 12 ms |
| 7 RIM (RIM) | rim | 1250 | 20 | -8.2 | 6293 | 1250 Hz + +20 st, tau 3 ms |
| 8 BELL (CYM) | cymbal | 8193 band | 3198 | -7.9 | 7108 | row 7 from 640 Hz, bands 8.2 / 5.2 kHz, ring tau 700 ms |

### BUCKET (KIT 19)

Low and dull: thuds, a muted snare, soft hats, a woody rim, a dull cymbal.

| lane | piece | pitch (Hz) | decay (ms, -40 dB) | peak (dBFS) | centroid (Hz) | what it is |
|---|---|---|---|---|---|---|
| 1 KICK (KICK) | kick (PUNCH) | 52 | 145 | -7.0 | 760 | 52 Hz, +4 st over 14 ms, tau 21 ms |
| 2 SNARE (SNARE) | snare | 150 | 95 | -5.8 | 2277 | shell 150 Hz x1.5, noise 1.8 kHz, tau 19 ms |
| 3 CLAP (CLAP) | clap | 700 band | 65 | -8.7 | 3848 | 3 bursts 9 ms apart, band 0.7 kHz, tail tau 15 ms |
| 4 HAT CL (HATCL) | closed hat | 3994 band | 55 | -19.5 | 6988 | soft noise, band 4 kHz, tau 11 ms |
| 5 HAT OP (HATOP) | open hat | 3994 band | 105 | -18.7 | 6919 | soft noise, tau 22 ms |
| 6 TOM (TOM) | tom | 85 | 132 | -6.1 | 736 | 85 Hz bent 4 st, tau 19 ms |
| 7 RIM (RIM) | rim | 580 | 20 | -7.4 | 1284 | 580 Hz + +14 st, tau 3.5 ms |
| 8 BELL (CYM) | cymbal | 3597 band | 631 | -9.1 | 4008 | row 2 from 290 Hz, bands 3.6 / 1.9 kHz, ring tau 240 ms |

### STATIC (KIT 20)

Noise-forward: a click kick, a hiss-burst snare, hats and cymbal of white noise, heavily crushed.

| lane | piece | pitch (Hz) | decay (ms, -40 dB) | peak (dBFS) | centroid (Hz) | what it is |
|---|---|---|---|---|---|---|
| 1 KICK (KICK) | kick (PUNCH) | 62 | 97 | -5.2 | 2366 | 62 Hz, +3 st over 6 ms, tau 11 ms |
| 2 SNARE (SNARE) | snare | 240 | 92 | -3.5 | 8072 | shell 240 Hz x1, noise 5.5 kHz, tau 7 ms |
| 3 CLAP (CLAP) | clap | 2400 band | 90 | -4.8 | 6627 | 3 bursts 6 ms apart, band 2.4 kHz, tail tau 7 ms |
| 4 HAT CL (HATCL) | closed hat | 8494 band | 50 | -5.3 | 10027 | white noise, band 8.5 kHz, tau 8 ms |
| 5 HAT OP (HATOP) | open hat | 8494 band | 57 | -5.5 | 10173 | white noise, tau 11 ms |
| 6 TOM (TOM) | tom | 140 | 122 | -5.3 | 5590 | 140 Hz bent 2 st, tau 11 ms |
| 7 RIM (RIM) | rim | 1800 | 45 | -4.9 | 5651 | 1800 Hz + +14 st, tau 3 ms |
| 8 BELL (CYM) | cymbal | 7513 band | 2549 | -4.9 | 7365 | hiss: noise through two bands, ring tau 800 ms |

### PARKING LOT (KIT 21)

The same box played in a big space: short, crisp hits (a snare crack, two-burst claps) that the preset's own reverb carries.

| lane | piece | pitch (Hz) | decay (ms, -40 dB) | peak (dBFS) | centroid (Hz) | what it is |
|---|---|---|---|---|---|---|
| 1 KICK (KICK) | kick (PUNCH) | 60 | 145 | -6.5 | 686 | 60 Hz, +8 st over 14 ms, tau 18 ms |
| 2 SNARE (SNARE) | snare | 210 | 125 | -5.9 | 5894 | shell 210 Hz x1.6, noise 5.0 kHz, tau 17 ms |
| 3 CLAP (CLAP) | clap | 1500 band | 70 | -7.9 | 5244 | 2 bursts 8 ms apart, band 1.5 kHz, tail tau 16 ms |
| 4 HAT CL (HATCL) | closed hat | 7789 band | 55 | -13.9 | 7517 | row 0 from 205 Hz, band 7.8 kHz, tau 10 ms |
| 5 HAT OP (HATOP) | open hat | 7789 band | 122 | -12.5 | 7636 | row 0 from 205 Hz, band 7.8 kHz, tau 21 ms |
| 6 TOM (TOM) | tom | 135 | 127 | -5.8 | 947 | 135 Hz bent 4 st, tau 17 ms |
| 7 RIM (RIM) | rim | 740 | 17 | -7.3 | 4035 | 740 Hz + +22 st, tau 2.4 ms |
| 8 BELL (CYM) | cymbal | 6790 band | 1701 | -8.0 | 5957 | row 0 from 420 Hz, bands 6.8 / 3.6 kHz, ring tau 400 ms |

## GARAGE

### STUDIO (KIT 22)

A small real-sounding kit: a soft kick, a snare with body, a hand clap, a floor tom, a side stick, a ride.

| lane | piece | pitch (Hz) | decay (ms, -40 dB) | peak (dBFS) | centroid (Hz) | what it is |
|---|---|---|---|---|---|---|
| 1 KICK (KICK) | kick (PUNCH) | 62 | 526 | -12.5 | 1542 | 62 Hz, +5 st over 30 ms, tau 85 ms |
| 2 SNARE (SNARE) | snare | 195 | 187 | -9.3 | 5136 | shell 195 Hz x1.6, noise 4.3 kHz, tau 40 ms |
| 3 CLAP (CLAP) | clap | 1100 band | 247 | -12.6 | 6481 | 3 bursts 11 ms apart, band 1.1 kHz, tail tau 70 ms |
| 4 HAT CL (HATCL) | closed hat | 6989 band | 132 | -15.0 | 11375 | row 0 from 205 Hz, band 7.0 kHz, tau 30 ms |
| 5 HAT OP (HATOP) | open hat | 6989 band | 706 | -15.6 | 10605 | row 0 from 205 Hz, band 7.0 kHz, tau 160 ms |
| 6 TOM (TOM) | tom | 108 | 958 | -6.8 | 1858 | 108 Hz bent 3.5 st, tau 190 ms |
| 7 RIM (RIM) | rim | 620 | 12 | -12.6 | 2692 | 620 Hz + +21 st, tau 2.2 ms |
| 8 BELL (CYM) | cymbal | 5409 band | 3193 | -14.8 | 6008 | row 1 from 420 Hz, bands 5.4 / 2.8 kHz, ring tau 900 ms |

### BASEMENT (KIT 23)

A cajon box: a bass-box thump, a slap snare, a shaker on the hats, a conga, claves, a jingle on lane 8.

| lane | piece | pitch (Hz) | decay (ms, -40 dB) | peak (dBFS) | centroid (Hz) | what it is |
|---|---|---|---|---|---|---|
| 1 KICK (KICK) | kick (PUNCH) | 80 | 599 | -14.5 | 1330 | 80 Hz, +5 st over 18 ms, tau 95 ms |
| 2 SNARE (SNARE) | snare | 290 | 222 | -10.6 | 5888 | shell 290 Hz x1.5, noise 3.0 kHz, tau 55 ms |
| 3 CLAP (CLAP) | clap | 800 band | 142 | -14.6 | 6311 | 2 bursts 12 ms apart, band 0.8 kHz, tail tau 40 ms |
| 4 HAT CL (HATCL) | closed hat | 6502 band | 215 | -17.8 | 11680 | a shaker: noise with a slow attack, tau 45 ms |
| 5 HAT OP (HATOP) | open hat | 6502 band | 536 | -17.8 | 11646 | a long shaker, tau 120 ms |
| 6 TOM (CONGA) | conga | 210 | 377 | -9.3 | 1870 | 210 Hz bent 3 st, tau 75 ms |
| 7 RIM (CLAVE) | clave | 1850 | 62 | -14.5 | 2845 | 1850 Hz + +8 st, tau 14 ms |
| 8 BELL (CYM) | cymbal | 8806 band | 1634 | -16.7 | 8983 | row 9 from 500 Hz, bands 8.8 / 5.2 kHz, ring tau 450 ms |

### PORCH (KIT 24)

Hand drums: a low conga on the kick lane, a bongo on the snare lane, a high conga, claves, a shaker.

| lane | piece | pitch (Hz) | decay (ms, -40 dB) | peak (dBFS) | centroid (Hz) | what it is |
|---|---|---|---|---|---|---|
| 1 KICK (KICK) | kick (PUNCH) | 78 | 746 | -16.5 | 1475 | 78 Hz, +3 st over 25 ms, tau 120 ms |
| 2 SNARE (SNARE) | snare | 330 | 215 | -12.4 | 2520 | shell 330 Hz x1.5, noise 3.2 kHz, tau 45 ms |
| 3 CLAP (CLAP) | clap | 1300 band | 150 | -14.1 | 6883 | 3 bursts 9 ms apart, band 1.3 kHz, tail tau 45 ms |
| 4 HAT CL (HATCL) | closed hat | 6989 band | 165 | -19.8 | 10992 | a shaker: noise, tau 35 ms |
| 5 HAT OP (HATOP) | open hat | 6989 band | 571 | -19.8 | 10894 | a long shaker, tau 130 ms |
| 6 TOM (CONGA) | conga | 300 | 586 | -11.1 | 2413 | 300 Hz bent 4 st, tau 110 ms |
| 7 RIM (CLAVE) | clave | 2300 | 85 | -15.9 | 3641 | 2300 Hz + +5 st, tau 20 ms |
| 8 BELL (CYM) | cymbal | 6597 band | 2195 | -18.7 | 7723 | row 1 from 420 Hz, bands 6.6 / 3.4 kHz, ring tau 600 ms |

### CANTINA (KIT 25)

Latin: a soft bombo, a timbale-like crack and high tuned toms, a guiro scrape on the hats, claves, a cowbell on lane 8.

| lane | piece | pitch (Hz) | decay (ms, -40 dB) | peak (dBFS) | centroid (Hz) | what it is |
|---|---|---|---|---|---|---|
| 1 KICK (KICK) | kick (PUNCH) | 72 | 741 | -16.9 | 1481 | 72 Hz, +4 st over 25 ms, tau 120 ms |
| 2 SNARE (SNARE) | snare | 360 | 277 | -13.1 | 7263 | shell 360 Hz x1.5, noise 4.5 kHz, tau 55 ms |
| 3 CLAP (CLAP) | clap | 1400 band | 135 | -16.9 | 5584 | 4 bursts 8 ms apart, band 1.4 kHz, tail tau 38 ms |
| 4 HAT CL (HATCL) | closed hat | 4996 band | 232 | -20.2 | 10213 | a guiro scrape: noise rising over 22 ms |
| 5 HAT OP (HATOP) | open hat | 4996 band | 564 | -18.4 | 10233 | a long scrape, rising over 30 ms |
| 6 TOM (TOM) | tom | 420 | 514 | -11.7 | 1837 | 420 Hz bent 3 st, tau 95 ms |
| 7 RIM (CLAVE) | clave | 2600 | 95 | -16.6 | 3993 | 2600 Hz + +5 st, tau 22 ms |
| 8 BELL (BELL) | bell | 520 | 584 | -19.1 | 3374 | squares 520 / 770 Hz, tau 160 ms |

### TIN SHED (KIT 26)

Junk percussion: an oil-drum thump, a can for a snare, a lid and a pipe on the hats, a can tom, a metal hit, a bottle-like bell.

| lane | piece | pitch (Hz) | decay (ms, -40 dB) | peak (dBFS) | centroid (Hz) | what it is |
|---|---|---|---|---|---|---|
| 1 KICK (KICK) | kick (PUNCH) | 95 | 574 | -16.6 | 1336 | 95 Hz, +7 st over 12 ms, tau 90 ms |
| 2 SNARE (SNARE) | snare | 420 | 299 | -11.9 | 5185 | shell 420 Hz x1.4, noise 3.2 kHz, tau 75 ms |
| 3 CLAP (CLAP) | clap | 1800 band | 200 | -16.3 | 7888 | 3 bursts 14 ms apart, band 1.8 kHz, tail tau 55 ms |
| 4 HAT CL (HATCL) | closed hat | 6295 band | 67 | -19.7 | 10455 | a lid: inharmonic cluster, tau 13 ms |
| 5 HAT OP (HATOP) | open hat | 6295 band | 838 | -19.7 | 10628 | a pipe: the same cluster ringing, tau 170 ms |
| 6 TOM (TOM) | tom | 270 | 536 | -11.2 | 1494 | 270 Hz bent 5 st, tau 100 ms |
| 7 RIM (CLAVE) | clave | 2100 | 170 | -15.0 | 5914 | 2100 Hz + +12 st, tau 40 ms |
| 8 BELL (BELL) | bell | 1180 | 915 | -18.7 | 3476 | squares 1180 / 2429 Hz, tau 250 ms |

## How far apart the kits are

The round 6 feature distance (`tests/ui_test_r6.h`: brightness, its change, attack, sustain, motion and level of the audit phrase
kick, snare, closed hat, kick; two presets nearer than 0.60 sound like one) between each instrument's five presets, measured the way
the audit does it. "Old presets" are the five presets as they were before this round (one kit each: DEEP, "10", "77"; measured at
`c808a83`, constants in the test); "new kits" are the same five presets now. The last column is each new kit against the old kit
at the same preset settings (only the KIT value changed).

### THUMP LAB

```
new kits     WAREHOUSE  BOOM BAP     DISCO TRASH CAN    ARCADE   old kit
WAREHOUSE            -                                           2.25
BOOM BAP          4.33         -                                 3.51
DISCO             2.29      3.60         -                       2.28
TRASH CAN         1.66      3.77      2.86         -             1.46
ARCADE            2.32      6.28      4.17      2.98         -   3.29
old presets over the old kit (c808a83): min 0.89, mean 1.45, max 2.03; new kits: min 1.66, mean 3.43, max 6.28
```

### KITBOX

```
new kits        DENTED  TIN ROOF    BUCKET    STATIC PARKING L   old kit
DENTED               -                                           4.76
TIN ROOF          5.40         -                                 1.74
BUCKET            4.64      7.03         -                       4.72
STATIC            4.17      6.48      3.38         -             5.84
PARKING LOT       4.48      4.35      3.47      2.98         -   2.05
old presets over the old kit (c808a83): min 2.65, mean 4.12, max 6.05; new kits: min 2.98, mean 4.64, max 7.03
```

### GARAGE

```
new kits        STUDIO  BASEMENT     PORCH   CANTINA  TIN SHED   old kit
STUDIO               -                                           2.21
BASEMENT          1.18         -                                 2.42
PORCH             1.05      1.49         -                       3.08
CANTINA           2.43      1.72      3.02         -             2.61
TIN SHED          4.21      4.17      4.19      4.77         -   3.07
old presets over the old kit (c808a83): min 0.61, mean 1.41, max 1.96; new kits: min 1.05, mean 2.82, max 4.77
```

## Lane balance and decay

- THUMP LAB: the loudest lane of a kit is at most 3.9 dB over the next loudest (6 dB allowed) (no decay limit on this instrument, so not measured).
- KITBOX: the loudest lane of a kit is at most 1.3 dB over the next loudest (6 dB allowed); longest decay of a lane other than the cymbal 145 ms (KITBOX: 150 ms allowed).
- GARAGE: the loudest lane of a kit is at most 2.5 dB over the next loudest (6 dB allowed) (no decay limit on this instrument, so not measured).

## Keys

- THUMP LAB: the pitched lane up the scale (G4..G5, eight keys): the largest step between adjacent keys 0.89 LU (round 9: 1.0 at most).
- KITBOX: the pitched lane up the scale (G4..G5, eight keys): the largest step between adjacent keys 0.87 LU (round 9: 1.0 at most).
- GARAGE: the pitched lane up the scale (G4..G5, eight keys): the largest step between adjacent keys 0.71 LU (round 9: 1.0 at most).

