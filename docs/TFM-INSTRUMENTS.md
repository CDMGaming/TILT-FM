# TFM instruments v1 (2026-10-28)

One sheet per instrument. This replaces the `TFM_INS` table in `firmware/src/tfm_instr.c`
and `MACROS-TO-TUNE.md`. The faces are drawn in `screens/Sound*.dc.html` (2x scale).

## Rules for every instrument

**An instrument is more than a preset.** Each one has three parts:
1. **Voicing:** fixed settings that make it this instrument. These are the engine, voice
   mode, filter type, LFO, mod-matrix routes, the INSERT effect and its sends. The player
   never sees them, except in DEEP.
2. **Eight macros:** K1–K4 on MACROS 1 and K1–K4 on MACROS 2.
3. **Presets:** each preset is a voicing tweak plus the eight macro positions.

**Macros**
- Each macro moves 1–3 targets. Each target has its own `lo..hi` range and a curve: LIN,
  EXP, or SWITCH (stepped, with names shown).
- Any macro that adds drive, resonance or distortion also lowers LEVEL, so the volume
  stays steady.
- **No LEVEL macros.** Track level belongs to MIX. Every slot that used to be LEVEL now
  does something musical.
- At 50 the instrument should sound like itself. 0 and 100 can be extreme. Everything in
  between has to be usable, with no dead zone and no jumps (except on SWITCH macros).
- The ranges below are starting points. Tune them by ear and keep the sheet updated.
  Where a target name isn't an exact match for a parameter, map it to the closest one in
  the engine and say so in the report.
- A macro writes absolute values to its targets. If a target is changed in DEEP, the next
  move of that macro overrides it. DEEP shows `M` next to any parameter a macro owns.
- Macro moves are recorded in MOTION, like any other knob.

**JAM knob (fix round 1).** JAM's knob turns one macro of each instrument, never a switch:
THUMP PUNCH · SQUELCH CUTOFF · WOBBLER WOB · CHORDETTE WARM · RAZOR CUT · HAZE BLOOM ·
PIXEL SWEEP · TINE BARK · GLINT METAL · PLUCK BRIGHT · VOX VOWEL · WHEELHOUSE DRIVE.
(In `firmware/src/tfm_instr.c` this is `TFM_JAMM`; `MACROS-TO-TUNE.md` marks it "JAM".)

**Macro feel (fix round 1).** Each track keeps its eight macro positions (0..127) in RAM;
a turn moves the stored position and every target follows it, so one detent is exactly
one step from 0 to 127. A SWITCH macro steps once per two detents. When a parameter is
changed elsewhere (DEEP, a load, undo, MIDI) the position is read back from the first
target once and kept again.

**Effects**
- A track has **one INSERT**: SOFT, HARD, FOLD, FUZZ, CRUSH, PHASR, FLANG or CHOR.
- It also has sends: DIST, CHOR, DLY and REV.
- Delay time and reverb type/size are shared by all tracks (they live on SPACE). So
  instruments use REV and DLY sends, but never change the global reverb or delay.
- An instrument chooses its INSERT. Its macros may only use that insert and the sends.

**Face** (template = `SoundSquelch.dc.html`)
- Header: track badge, TIMER, `MACROS 1 · 2 · DEEP`.
- Then the toy, the name in Silkscreen in the instrument's accent colour, the one-line
  description, the preset name and `PRESETS ◂ n of m ▸`.
- Then four dials in the K1–K4 colours. On MACROS 2 the dials show macros 5–8.
- Colours per instrument: BG (background), SURF (toy panel and empty dial track), ACC
  (accent), MUTE (secondary text), INK (main text).
- **Toys:** each toy redraws only its own box, at most once per frame. Each one says
  exactly what it reacts to.

**Presets:** ship the 5 named presets per instrument listed below. The existing preset
banks can stay behind them, renamed only if a name clashes.

---

## 1. THUMP LAB · drums · DRUM engine
`BG #150A08 SURF #1F0E0B ACC #FF7A3D MUTE #B5948A INK #F8ECE6` · drawn: `SoundThump.dc.html`
- **Description:** "drum kit · bumpers pop on hits"
- **Voicing:** KIT STD, KICK PUNCH. Lane levels: KICK 110, SNARE 100, CLAP 92, HATS 84,
  the rest 80. INSERT SOFT, MIX 20. REV send 10.

| # | Macro | Targets | 0 | 50 | 100 |
|---|---|---|---|---|---|
| 1 | PUNCH | kick TUNE 70..34 EXP, DRV 0..80, ACC 40..120 (LEVEL 108..94) | soft round thud | tight house kick | overdriven boom with a click |
| 2 | SMACK | SNAP 10..120, TONE 30..115, LN1/LN2 (snare, clap) 80..115 | brushed, dull | crisp | cracking, bright |
| 3 | TIME | DECY 8..120 EXP (LEVEL 104..96) | choked, staccato | natural | long, boomy tails |
| 4 | ROOM | REV 0..90, DLY 0..25 (above 70 only) | dry and close | small room | big room with a slapback |
| 5 | TUNE | TUNE 16..112 (whole kit) | low, heavy | as recorded | high, toy-like |
| 6 | DIRT | INSERT SOFT→HARD at 60, IA 0..110, IMIX 20..100 (LEVEL 106..90) | clean | warm tape | smashed |
| 7 | HATS | LN3/LN4 50..120, TONE +0..+15 | hats tucked away | balanced | hats up front, sizzling |
| 8 | ECHO | DLY 0..80 | none | dub tails | washed-out echoes |

- **Presets:** WAREHOUSE, BOOM BAP, DISCO, TRASH CAN, ARCADE
- **Toy:** pop bumpers (KICK big, SNARE, CLAP), a HATS spinner and the 8 lane chips.
  - Each bumper flashes on its hit, with brightness set by velocity.
  - The hat spinner advances one notch per hat hit.
  - A lane chip lights for one beat when its lane hits.

## 2. SQUELCH · bass · ANALOG
`BG #0E1206 SURF #1E2A0A ACC #C8F25A MUTE #9BA889 INK #F2F6E8` · drawn: `SoundSquelch.dc.html`
- **Description:** "acid bass · spins with the cutoff"
- **Voicing:** MONO, LEGATO glide, LP filter, saw. ENV>FLT on and fast. INSERT FUZZ,
  MIX 0. Accented steps open the filter by 20 (mod matrix: VELO>CUT 20).

| # | Macro | Targets | 0 | 50 | 100 |
|---|---|---|---|---|---|
| 1 | CUTOFF | CUT 18..112 EXP | closed, rubbery | classic squelch | open, buzzing |
| 2 | RESO | RES 10..118 (LEVEL 108..96) | smooth | singing | screaming, near self-oscillation |
| 3 | ENV MOD | ENV>FLT 0..60, DEC 30..55 | flat, no zap | squelchy | big zaps on every note |
| 4 | DECAY | DEC 20..100, REL 10..50 | short blips | bouncy | long, wah-like sweeps |
| 5 | BITE | DRV 0..120 (LEVEL 108..88) | clean | fat | crunchy |
| 6 | GNAW | INSERT FUZZ IMIX 0..100 (LEVEL 108..92) | none | grit | fuzzed out |
| 7 | WAVE | MIX 0..127 (saw→square) | saw | blend | hollow square |
| 8 | SLIDE | GLIDE 0..80 EXP | no slides | 303-style slides | long portamento |

- **Presets:** RUBBER, ACID RAIN, LOW RIDER, ZAPPER, GNARLY
- **Toy:** a spinner.
  - Its speed follows the current filter frequency, including the envelope, so it whips
    round on every zap.
  - It flashes white on accented notes.

## 3. WOBBLER · bass · TRIO
`BG #0A0B16 SURF #161A30 ACC #8C7BFF MUTE #8E8AB0 INK #EEEBFF`
- **Description:** "wobble bass · the magnet shakes with the LFO"
- **Voicing:** MONO. LFO synced, TRIG NOTE, sine, routed to filter. INSERT FOLD, MIX 0.
  Oscillators slightly detuned.

| # | Macro | Targets | 0 | 50 | 100 |
|---|---|---|---|---|---|
| 1 | WOB | LFO>FLT 0..63, RES 10..60 | still | wobbling | deep yoy-yoy |
| 2 | RATE | LFO SYNC SWITCH: 1/1 1/2 1/4 1/4T 1/8 1/8T 1/16 1/32 | slow sweeps | 1/8 | buzzing |
| 3 | GROWL | INSERT FOLD IMIX 0..100, IA 20..90 (LEVEL 108..92) | round | growl | metallic snarl |
| 4 | CUT | CUT 15..110 EXP | dark sub | thick | bright |
| 5 | SPREAD | DTN 0..40, SPRD 0..100 | narrow, mono | wide | huge detuned wall |
| 6 | DIRT | DIST send 0..90 (LEVEL 106..92) | clean | dirty | wrecked |
| 7 | DROP | TRANS SWITCH 0 / −12 / −24 (shown as 0, −1 OCT, −2 OCT) | normal | an octave down | sub-floor |
| 8 | SLIDE | GLIDE 0..80 | none | slides | long glides |

- **Presets:** HALF STEP, YOY, TRASH FLOOR, LASER WOB, SUBWAY
- **Toy:** a magnet target (concentric rings with a ball held in the middle).
  - The ball is pushed off-centre by LFO phase × WOB depth.
  - The rings light up on each note.

## 4. CHORDETTE · chords · ANALOG (poly + chord keys)
`BG #120A14 SURF #24142A ACC #FF8FD0 MUTE #B898AC INK #FFEEF7`
- **Description:** "one key, whole chords · targets drop per note"
- **Voicing:** POLY, chord keys on, LP filter, saw + square. CHOR send 20. INSERT CRUSH,
  MIX 0. LFO 1/4 synced, routed to AMP at 0.

| # | Macro | Targets | 0 | 50 | 100 |
|---|---|---|---|---|---|
| 1 | CHORD | CHORD SWITCH: MAJ MIN 7 MAJ7 MIN7 SUS2 SUS4 ADD9 POWER | | | |
| 2 | VOICE | VOICING SWITCH: CLOSE OPEN DROP2 SPREAD STACK | | | |
| 3 | WARM | CUT 30..110, RES 40..10 | muffled felt | warm | bright, glassy |
| 4 | SWELL | ATK 0..90 EXP, REL 10..110 | stabs | soft | slow swells |
| 5 | SHIMMER | CHOR 0..60, DTN 0..20 | dry | lush | seasick |
| 6 | SPACE | REV 0..90, DLY 0..40 (above 60 only) | dry | room | cathedral |
| 7 | PUMP | LFO>AMP 0..80 (1/4) | steady | breathing | sidechain pump |
| 8 | AGE | INSERT CRUSH IMIX 0..70, IA 16..6 bits | new | dusty | old cassette |

- **Presets:** HOUSE STAB, SUNDAY, VELVET, PUMPER, MIXTAPE
- **Toy:** a bank of 4 drop targets, one for each chord note.
  - A target drops while its note sounds and pops back up on release.
  - All four flash when every target is down.

## 5. RAZOR · lead · ANALOG (unison)
`BG #0B1014 SURF #142028 ACC #4FD4FF MUTE #87A2B0 INK #E8F8FF`
- **Description:** "unison lead · the ball rides the ramp"
- **Voicing:** UNISON 3 voices, saw, LP filter. LFO slow, routed to pitch, with a delayed
  fade. DLY send 0. INSERT HARD, MIX 0.

| # | Macro | Targets | 0 | 50 | 100 |
|---|---|---|---|---|---|
| 1 | SCREAM | RES 10..110, DRV 0..70 (LEVEL 108..92) | smooth | edgy | screaming |
| 2 | SWEEP | ENV>FLT −20..60, DEC 20..70 | dark fall | plucky | big rise on every note |
| 3 | CUT | CUT 30..120 EXP | muffled | cutting | razor bright |
| 4 | GLIDE | GLIDE 0..80 | none | portamento | long swoops |
| 5 | WIDTH | DTN 0..80, SPRD 0..127 | one thin saw | supersaw | massive |
| 6 | ATTACK | ATK 0..80 | instant | soft | slow fade-in |
| 7 | VIB | LFO>PIT 0..20, LFADE 60 | none | expressive vibrato | wide warble |
| 8 | ECHO | DLY 0..90 | dry | lead echoes | drenched |

- **Presets:** HERO, TRANCE GATE, ARENA, SINE RIDER, HOT WIRE
- **Toy:** a ramp with a ball and chase lights.
  - The ball's height follows the pitch.
  - The chase lights speed up with the filter envelope.

## 6. HAZE · pad · TRIO
`BG #0E0C14 SURF #1C1828 ACC #B9A6FF MUTE #9A92B4 INK #F2EEFF`
- **Description:** "slow pad · the lights glow and drift"
- **Voicing:** POLY, slow attack and release. LFO slow, free, routed to pitch at 0.
  REV send 60 with reverb set to HALL. INSERT CRUSH, MIX 0.

| # | Macro | Targets | 0 | 50 | 100 |
|---|---|---|---|---|---|
| 1 | DRIFT | LFO>PIT 0..14, DTN 5..30 | still | tape drift | seasick |
| 2 | DUST | INSERT CRUSH IMIX 0..100 | clean | dusty | broken radio |
| 3 | SPACE | REV 10..127 | close | wide | endless |
| 4 | BLOOM | ATK 10..120 EXP, REL 30..120 | quick | slow bloom | glacial |
| 5 | TONE | CUT 10..127 EXP | underwater | soft | airy |
| 6 | SHINE | CHOR 0..110 | flat | lush | shimmering |
| 7 | MOVE | LFO>FLT 0..60 | static | breathing | sweeping |
| 8 | HOLD | SUS 60..127, REL +0..+30 | fades | sustains | drones |

- **Presets:** FOG, MORNING, VHS, AURORA, DEEP END
- **Toy:** a row of general-illumination bulbs along a soft glow.
  - Their brightness follows the amp envelope.
  - The glow's colour drifts with the LFO.

## 7. PIXEL · lead · LOFI
`BG #08100C SURF #102018 ACC #5CFF8F MUTE #86A892 INK #EAFFF0`
- **Description:** "chip lead · the sprite jumps on notes"
- **Voicing:** MONO, pulse wave, short decay. Pitch envelope routed in, at 0.

| # | Macro | Targets | 0 | 50 | 100 |
|---|---|---|---|---|---|
| 1 | JUMP | ARP SWITCH: OFF OCT 5TH TRIAD | | | |
| 2 | WAVE | WAVE SWITCH: 12% 25% 50% TRI NOISE | | | |
| 3 | SWEEP | SWP 0..110 | flat | swoops | laser |
| 4 | CRUSH | CRSH 0..120 | clean 8-bit | crunchy | broken console |
| 5 | VIB | VIB 0..110 | none | game vibrato | dizzy |
| 6 | DECAY | DEC 15..100 | blips | notes | held tones |
| 7 | TONE | TONE 20..127 | muffled | bright | piercing |
| 8 | COIN | ED_PIT 0..60, DEC −0..−20 | none | blip | coin pickup |

- **Presets:** 1UP, BOSS LEVEL, WARP ZONE, HIGH SCORE, GLITCH
- **Toy:** an original pixel sprite on a DMD (dot-matrix display) grid.
  - It jumps on each note, with height set by velocity.
  - A coin pops out when COIN is above 30 or on an accent.

## 8. TINE · keys · FM6
`BG #14100A SURF #261E12 ACC #FFC56B MUTE #B4A486 INK #FFF6E6`
- **Description:** "electric piano · notes light the rollovers"
- **Voicing:** a two-operator tine algorithm. LFO 5 Hz routed to AMP at 0. CHOR send 20.
  INSERT SOFT, MIX 0.

| # | Macro | Targets | 0 | 50 | 100 |
|---|---|---|---|---|---|
| 1 | BARK | MLVL −40..40 | mellow | classic | growling |
| 2 | TINE | MEG −40..40 | soft | bell | glassy, clangy |
| 3 | TREM | LFO>AMP 0..90 | steady | gentle tremolo | choppy |
| 4 | CHORUS | CHOR 0..110 | dry | classic chorus | wobbly |
| 5 | DECAY | DEC 30..110 | short | piano | long ring |
| 6 | WARMTH | INSERT SOFT IMIX 0..80 (LEVEL 106..96) | clean | amp | broken-up |
| 7 | SPACE | REV 0..100 | dry | room | hall |
| 8 | REL | REL 10..100 | damped | natural | sustain pedal |

- **Presets:** SUITCASE, LOUNGE, BARK BARK, DREAMY, CHURCH BASEMENT
- **Toy:** 5 rollover lanes.
  - Each note lights the lane for its pitch class (note mod 5).
  - Each light fades with the decay.

## 9. GLINT · bells · FM6
`BG #0A1214 SURF #142428 ACC #7FF0E6 MUTE #86AAA6 INK #EAFFFC`
- **Description:** "FM bells · stars sparkle on notes"
- **Voicing:** FM6 with a non-integer ratio. REV send 40. INSERT CHOR, MIX 0.

| # | Macro | Targets | 0 | 50 | 100 |
|---|---|---|---|---|---|
| 1 | METAL | MLVL −40..50 | pure | bell | gong |
| 2 | RATIO | MRAT SWITCH −8..8 (shown as ratio) | | | |
| 3 | BLOOM | MEG −40..50 | static | shimmer | wild sweep |
| 4 | DECAY | DEC 30..120 | plink | bell | long ring |
| 5 | BITE | FB −5..7 | smooth | edgy | noisy |
| 6 | ATTACK | ATK 0..80 | strike | soft | bowed |
| 7 | SPACE | REV 0..110 | dry | wide | huge |
| 8 | SHIMMER | INSERT CHOR IMIX 0..90, DLY 0..40 | still | moving | sparkling |

- **Presets:** MUSIC BOX, TEMPLE, ICE, CHIMES, ROBOT BELLS
- **Toy:** 5 star rollovers.
  - The number of stars that light depends on velocity.
  - The twinkle rate follows METAL.

## 10. PLUCK · strings · PHYS
`BG #120E08 SURF #241C10 ACC #E8B07A MUTE #A89880 INK #FFF4E6`
- **Description:** "plucked strings · flippers kick on notes"
- **Voicing:** a plucked string model. REV send 20. INSERT SOFT, MIX 0.

| # | Macro | Targets | 0 | 50 | 100 |
|---|---|---|---|---|---|
| 1 | BRIGHT | BRIT 20..127, POS 40..20 | nylon | steel | harp-bright |
| 2 | DAMP | DAMP 20..127 | muted, palm | natural | long ring |
| 3 | BODY | STRC 10..120 | thin | guitar | big box |
| 4 | PICK | POS 0..110 | at the bridge | middle | at the neck |
| 5 | BOW | BOW 0..110 | plucked | mixed | bowed |
| 6 | ACCENT | ACC 30..127 | even | dynamic | snappy |
| 7 | SPACE | REV 0..100 | dry | room | hall |
| 8 | ECHO | DLY 0..80 | none | slap | dub echoes |

- **Presets:** KOTO, NYLON, MUTED, HARP, BANJO-ISH
- **Toy:** 4 flippers.
  - Each note kicks the next flipper, and harder notes kick it higher.
  - The flippers settle back with the damping.

## 11. VOX · voice · FORMANT
`BG #140A0E SURF #28141C ACC #FF6F6F MUTE #B89098 INK #FFEEF0`
- **Description:** "talking synth · the mouth opens with the sound"
- **Voicing:** formant voice. LFO slow, routed to vowel at 0. CHOR send 10. INSERT FLANG,
  MIX 0.

| # | Macro | Targets | 0 | 50 | 100 |
|---|---|---|---|---|---|
| 1 | VOWEL | VOWL 0..127 (A E I O U) | aah | eeh | ooh |
| 2 | TALK | TALK 0..110 | held vowel | chatty | babbling |
| 3 | BUZZ | BUZZ 10..127 | breathy | voice | buzzing robot |
| 4 | BREATH | BRTH 0..110 | none | airy | whisper |
| 5 | SHIFT | SHIFT SWITCH −12..+12 (shown in semitones) | giant | as is | chipmunk |
| 6 | Q | Q 10..127 | blurry | clear | whistling |
| 7 | CHOIR | CHOR 0..90, SPRD 0..120, INSERT FLANG IMIX 0..40 | one voice | duet | choir |
| 8 | SPACE | REV 0..100 | dry | room | church |

- **Presets:** ROBOT, CHOIR BOY, WHISPER, YAMMER, MONK
- **Toy:** a DMD mouth.
  - How far it opens follows the amp envelope.
  - Its shape follows the vowel (A E I O U).

## 12. WHEELHOUSE · organ · WHEEL
`BG #120C06 SURF #24180C ACC #FFAA3C MUTE #B09C80 INK #FFF2E0`
- **Description:** "tonewheel organ · the disc spins with the rotor"
- **Voicing:** a tonewheel engine. The rotor has its own spin-up/spin-down inertia.
  INSERT SOFT, MIX 0.

| # | Macro | Targets | 0 | 50 | 100 |
|---|---|---|---|---|---|
| 1 | STOPS | REG SWITCH 0..15 (show names: FLUTE, JAZZ, GOSPEL, ROCK, FULL…) | | | |
| 2 | DRIVE | DRV 0..120 (LEVEL 108..92) | clean | warm | screaming |
| 3 | ROTOR | ROTR SWITCH: OFF SLOW FAST | | | |
| 4 | CLICK | CLICK 0..127 | smooth | key click | percussive |
| 5 | SUB | SUB −8..8 | no sub | balanced | heavy low end |
| 6 | BODY | BODY −8..8 | hollow | full | thick |
| 7 | TOP | TOP −8..8 | dark | sweet | shrill |
| 8 | SPACE | REV 0..90 | dry | room | hall |

- **Presets:** SMOKY, SUNDAY SERVICE, ROCK OUT, BALLPARK, FLUTES
- **Toy:** a spinning disc.
  - Its speed follows the rotor, spinning up and down with the same inertia.
  - It brightens while keys are held.

---

## Still open (not for this build)
- There is only one drum instrument, and DICE always rolls drums. Felix had wanted 3 kits
  (for example THUMP LAB, plus a vintage machine kit and a lo-fi kit).
- MACROS-TO-TUNE should be regenerated from the new table so Felix can tick each macro
  off by ear.

## In key (round 3)
Melodic notes land on the written pitch or an exact octave of it. Detune, vibrato and drift are seasoning: at macro 50 within
+-10 cents, at 100 within +-35 cents. Pitch sweeps and pitch envelopes are back on pitch within a 1/16 note. Chords follow the
scale (CHORDETTE's CHORD switch: AUTO, AUTO 7, then MAJ MIN 7 MAJ7 MIN7 SUS4 POWER). Done in round 3: CHORDETTE AUTO; PIXEL's SWEEP
default 0 (range cut), JUMP OFF / OCT / 5TH, COIN replaced by DUTY, JAM macro TONE; RAZOR no always-on LD_PIT, VIB at most 2 steps;
HAZE DRIFT 0..2 steps, gated (nothing below 70 %); GLINT on FM6 slot 1 (BELL), RATIO -4..+4. Measured by tests/ui_test_r3.h and asserted for every melodic instrument (round 4): a host render of each preset (and, listed,
every continuous macro at 0 and 50) at C4 and G4 from a clean start (the reverb, echo and chorus buffers cleared between notes,
or the tail of the first note reads as a second pitch), judged by YIN (the first dip of the normalised difference function
under 0.15, parabola refined) within 35 cents of the note or of an octave of it; a sound with no single period (confidence
under 0.55) counts as measured, not wrong. At least 3 of the 5 named presets pass on SQUELCH, WOBBLER, CHORDETTE, RAZOR, HAZE,
PIXEL, TINE, GLINT, PLUCK, VOX and WHEELHOUSE (THUMP LAB is a kit: it has no pitch to tune). What the listed macro misses are,
and why they are left: WOBBLER's wob is an amplitude wobble (measured with it at 0: the pitch is the note's); PIXEL's SWEEP is
a pitch slide by design; GLINT's bells show their inharmonic partials when the modulation is low or the decay long (RATIO is
in tune at its -2 place, 30 %, which the presets now use); PLUCK's string is a loop filter whose delay shifts the pitch with
BRIGHT and DAMP (the presets sit where it is in tune, a few macro corners are not); VOX's CHOIR and noise presets have no single
period. Detune, glide and chord layers stay seasoning at the amounts above.

## The named presets (round 6)
Audited on the host (tests/ui_test_r6.h: a phrase from a clean start; brightness and its change, attack, sustain, motion,
level): no two presets of one instrument are nearer than 0.60 in that space (the closest pair before was 0.36). One line
each:

- **THUMP LAB.** WAREHOUSE: a round kick, dry snare, closed hats, medium room. BOOM BAP: dusty, swung, a fat snare with a
  long tail. DISCO: a tight four-on-the-floor kick, bright snare and open hats, big room and an echo on the hats. TRASH CAN:
  crushed, distorted, short and loud. ARCADE: tuned-up toy drums, bright and clicky.
- **SQUELCH.** RUBBER: a round resonant acid bass. ACID RAIN: brighter, more envelope, squelchier. LOW RIDER: dark and
  sub-heavy with a slow filter. ZAPPER: short zappy plucks. GNARLY: driven, resonant and growling.
- **WOBBLER.** HALF STEP: a slow half-time wob. YOY: a talking yoy-yoy wob. TRASH FLOOR: distorted, fast and wide. LASER
  WOB: a sharp, fast laser sweep. SUBWAY: deep, sub-heavy, slow.
- **CHORDETTE.** HOUSE STAB: a short bright organ stab. SUNDAY: a warm soft chord with a slow swell. VELVET: dark pad-like
  chords. PUMPER: a sidechained pumping chord. MIXTAPE: a lo-fi wobbly tape chord.
- **RAZOR.** HERO: a bright unison lead with a slow attack. TRANCE GATE: a gated, rhythmic supersaw. ARENA: big, wide, with
  space. SINE RIDER: a soft, round, gliding lead. HOT WIRE: hot, bright and biting.
- **HAZE.** FOG: a dark, soft drifting pad. MORNING: a bright airy pad. VHS: a worn, wobbly pad. AURORA: a slowly opening,
  shimmering pad. DEEP END: a long, dark, deep swell.
- **PIXEL.** 1UP: a short bright square blip. BOSS LEVEL: a slow, dramatic square lead. WARP ZONE: a falling pitch-swept
  zap. HIGH SCORE: a long triangle lead with vibrato and a fifth jump. GLITCH: a crushed, glitchy noise lead.
- **TINE.** SUITCASE: a classic warm electric piano. LOUNGE: soft and mellow. BARK BARK: barking, bright and dirty. DREAMY:
  chorused and soft. CHURCH BASEMENT: dull, tremolo, roomy.
- **GLINT.** MUSIC BOX: a short, dry, plucked tine. TEMPLE: a long bell with a huge space. ICE: soft-attack glass with
  shimmer and bloom. CHIMES: struck, metallic, long ringing, dry. ROBOT BELLS: harsh, metallic, biting and short.
- **PLUCK.** KOTO: a bright, nasal pluck. NYLON: a soft nylon string. MUTED: a palm-muted thud. HARP: a soft, ringing harp
  with space and an echo. BANJO-ISH: twangy and very bright.
- **VOX.** ROBOT: a buzzy robot voice. CHOIR BOY: a soft choir. WHISPER: breathy. YAMMER: a talking, vowel-moving voice.
  MONK: a low, chanting voice.
- **WHEELHOUSE.** SMOKY: a mellow jazz organ. SUNDAY SERVICE: full gospel organ. ROCK OUT: driven with a fast rotor.
  BALLPARK: bright, all the drawbars. FLUTES: soft flute stops.

## Levels (round 6)

Measured on the host (`R6LOUD=1` on `tests/ui_test.c`, `tests/ui_test_r6.h`): ITU BS.1770-style loudness, the K-weighting re-derived for 44.1 kHz, 400 ms blocks every 100 ms, the absolute (-70) and relative (-10 LU) gates for the integrated loudness (I, LUFS), the loudest 3 s for the short-term one. Each preset plays 2 bars at 120 of a part in its role: THUMP LAB a beat (kick, snare, hats); SQUELCH and WOBBLER eighths in C2-C3; CHORDETTE, HAZE, TINE, PLUCK, VOX, WHEELHOUSE chords on the half bars in C3-C4 (CHORDETTE: the root, it makes the chord); RAZOR, PIXEL and GLINT quarters in C4-C6. Default LEVEL, the master at unity, from a clean start. "-1 / +1 oct": the same part an octave down / up. "pk": the part's peak before the master (dBFS).

**How it is matched.** The mix takes a trim with LEVEL (`firmware/src/fx.c` `trk_lvl`; `tfm_instr.c` `TFM_TRIM`: half dB, per instrument and per named preset; the older sounds take the instrument's). Pads, leads and bells sit at -18 LUFS, the basses at -16.5 / -17.5, the drums at about -18. **No loudness is bought from the limiter**: a trim stops where the part alone peaks at -2 dBFS before the master (the master limiter starts at -5.2 dBFS and caught every drum hit and bass note before: THUMP LAB peaked at +9 dBFS through it, the pads were squeezed from about -6 to -11 LUFS). That ceiling is why the drums sit level with the rest rather than above, and why PLUCK sits at -21: its pick is a sharp peak over a short note.

Before, the pads and leads were 8-12 LU louder than the drums and basses (THUMP LAB -22, SQUELCH -21, the pads -11, the leads -10) and PLUCK's NYLON / MUTED / BANJO-ISH were 25 LU under HARP.

| instrument | preset | before I | after I | after S | after pk | before -1 / +1 oct | after -1 / +1 oct |
|---|---|---|---|---|---|---|---|
| THUMP LAB | WAREHOUSE | -22.2 | -18.2 | -18.0 | -2.0 | -22.2 / -22.2 | -18.2 / -18.1 |
| THUMP LAB | BOOM BAP | -22.6 | -18.1 | -18.0 | -1.9 | -22.6 / -22.6 | -18.2 / -18.1 |
| THUMP LAB | DISCO | -19.1 | -17.9 | -17.8 | -1.8 | -19.1 / -19.1 | -17.9 / -17.9 |
| THUMP LAB | TRASH CAN | -27.6 | -18.5 | -18.2 | -2.0 | -27.5 / -27.6 | -18.5 / -18.5 |
| THUMP LAB | ARCADE | -22.1 | -17.3 | -17.1 | -3.8 | -22.1 / -22.1 | -17.3 / -17.2 |
| SQUELCH | RUBBER | -21.5 | -18.1 | -17.9 | -1.6 | -23.8 / -20.1 | -20.1 / -17.6 |
| SQUELCH | ACID RAIN | -20.2 | -17.1 | -17.1 | -4.2 | -23.2 / -18.6 | -20.0 / -16.3 |
| SQUELCH | LOW RIDER | -19.1 | -17.6 | -17.5 | -6.7 | -22.1 / -17.2 | -20.6 / -16.5 |
| SQUELCH | ZAPPER | -25.2 | -18.3 | -18.1 | -2.0 | -27.0 / -23.2 | -20.1 / -17.4 |
| SQUELCH | GNARLY | -25.8 | -17.5 | -17.4 | -6.3 | -29.2 / -24.1 | -20.9 / -16.5 |
| WOBBLER | HALF STEP | -20.9 | -16.5 | -16.4 | -6.1 | -23.7 / -19.6 | -19.3 / -15.8 |
| WOBBLER | YOY | -21.5 | -16.6 | -16.5 | -7.7 | -23.5 / -21.3 | -18.6 / -16.3 |
| WOBBLER | TRASH FLOOR | -25.6 | -16.7 | -16.6 | -7.5 | -26.3 / -24.8 | -17.4 / -16.2 |
| WOBBLER | LASER WOB | -20.3 | -16.4 | -16.3 | -7.3 | -23.2 / -19.0 | -19.3 / -15.5 |
| WOBBLER | SUBWAY | -20.3 | -16.4 | -16.3 | -7.7 | -23.0 / -18.8 | -19.1 / -15.6 |
| CHORDETTE | HOUSE STAB | -13.2 | -18.1 | -18.0 | -11.9 | -13.5 / -13.1 | -18.4 / -17.7 |
| CHORDETTE | SUNDAY | -11.9 | -17.8 | -17.6 | -12.3 | -12.5 / -11.0 | -18.8 / -15.9 |
| CHORDETTE | VELVET | -11.5 | -18.2 | -18.1 | -12.9 | -12.7 / -11.1 | -21.0 / -18.0 |
| CHORDETTE | PUMPER | -14.7 | -17.8 | -17.8 | -10.7 | -15.1 / -12.9 | -18.4 / -15.5 |
| CHORDETTE | MIXTAPE | -11.4 | -17.8 | -17.6 | -11.2 | -13.5 / -11.5 | -19.9 / -17.0 |
| RAZOR | HERO | -10.3 | -18.1 | -17.9 | -9.8 | -10.7 / -9.6 | -18.0 / -18.9 |
| RAZOR | TRANCE GATE | -9.9 | -18.1 | -18.0 | -11.1 | -9.8 / -9.1 | -17.4 / -18.0 |
| RAZOR | ARENA | -11.5 | -17.8 | -17.2 | -7.0 | -10.7 / -14.5 | -16.4 / -21.0 |
| RAZOR | SINE RIDER | -14.7 | -18.0 | -17.1 | -9.0 | -10.6 / -22.2 | -11.9 / -24.7 |
| RAZOR | HOT WIRE | -10.6 | -18.2 | -18.1 | -8.1 | -10.9 / -9.7 | -18.2 / -17.1 |
| HAZE | FOG | -11.4 | -18.0 | -17.9 | -9.4 | -11.8 / -11.3 | -21.2 / -18.4 |
| HAZE | MORNING | -11.8 | -18.0 | -17.8 | -8.9 | -13.3 / -10.7 | -20.2 / -17.1 |
| HAZE | VHS | -12.1 | -17.9 | -17.7 | -7.6 | -13.0 / -11.4 | -21.0 / -17.1 |
| HAZE | AURORA | -12.4 | -17.8 | -17.5 | -6.9 | -14.4 / -11.1 | -20.5 / -15.5 |
| HAZE | DEEP END | -11.1 | -18.2 | -18.0 | -14.1 | -11.6 / -11.2 | -21.9 / -19.5 |
| PIXEL | 1UP | -9.6 | -18.1 | -18.0 | -15.3 | -10.0 / -8.6 | -18.2 / -18.3 |
| PIXEL | BOSS LEVEL | -12.6 | -17.9 | -17.9 | -9.5 | -13.2 / -12.2 | -18.2 / -18.5 |
| PIXEL | WARP ZONE | -17.5 | -17.9 | -17.7 | -8.9 | -19.1 / -16.2 | -19.1 / -17.7 |
| PIXEL | HIGH SCORE | -11.8 | -17.9 | -17.5 | -14.8 | -14.5 / -10.1 | -20.1 / -17.3 |
| PIXEL | GLITCH | -12.7 | -17.9 | -17.6 | -13.8 | -15.3 / -11.2 | -20.1 / -17.5 |
| TINE | SUITCASE | -11.2 | -17.9 | -18.0 | -9.7 | -12.1 / -11.6 | -19.6 / -18.4 |
| TINE | LOUNGE | -11.0 | -18.0 | -18.0 | -10.5 | -12.0 / -11.3 | -19.9 / -18.7 |
| TINE | BARK BARK | -11.9 | -18.2 | -18.3 | -11.0 | -13.4 / -12.5 | -19.6 / -18.9 |
| TINE | DREAMY | -10.9 | -17.9 | -18.0 | -11.4 | -12.3 / -11.3 | -19.5 / -18.6 |
| TINE | CHURCH BASEMENT | -13.1 | -18.2 | -18.2 | -11.6 | -16.5 / -13.4 | -21.1 / -18.5 |
| GLINT | MUSIC BOX | -17.1 | -18.2 | -18.0 | -10.5 | -18.2 / -16.1 | -19.3 / -17.4 |
| GLINT | TEMPLE | -15.7 | -18.0 | -17.8 | -12.1 | -15.9 / -15.3 | -18.7 / -18.1 |
| GLINT | ICE | -17.3 | -17.8 | -17.6 | -10.4 | -18.1 / -16.7 | -18.5 / -17.5 |
| GLINT | CHIMES | -15.9 | -18.0 | -17.8 | -11.2 | -16.9 / -15.3 | -18.9 / -17.7 |
| GLINT | ROBOT BELLS | -14.7 | -18.2 | -18.2 | -12.3 | -14.7 / -14.7 | -18.2 / -18.5 |
| PLUCK | KOTO | -30.7 | -21.8 | -21.4 | -1.8 | -31.1 / -30.1 | -22.1 / -22.7 |
| PLUCK | NYLON | -36.7 | -24.8 | -25.1 | -1.8 | -36.3 / -35.2 | -23.9 / -24.6 |
| PLUCK | MUTED | -36.9 | -23.5 | -24.1 | -1.8 | -37.4 / -35.3 | -23.6 / -24.2 |
| PLUCK | HARP | -14.0 | -21.1 | -20.6 | -12.3 | -14.7 / -10.6 | -22.3 / -17.1 |
| PLUCK | BANJO-ISH | -35.5 | -21.0 | -20.9 | -13.7 | -35.4 / -34.0 | -20.9 / -19.8 |
| VOX | ROBOT | -10.8 | -17.9 | -17.6 | -8.9 | -11.7 / -10.0 | -19.9 / -15.5 |
| VOX | CHOIR BOY | -11.4 | -18.0 | -17.7 | -10.2 | -12.0 / -10.9 | -20.1 / -15.8 |
| VOX | WHISPER | -12.0 | -17.9 | -17.8 | -7.7 | -11.9 / -11.5 | -17.8 / -18.6 |
| VOX | YAMMER | -11.3 | -18.2 | -17.9 | -8.2 | -12.1 / -10.7 | -19.9 / -17.0 |
| VOX | MONK | -11.3 | -17.8 | -17.4 | -11.0 | -11.5 / -10.5 | -18.2 / -15.6 |
| WHEELHOUSE | SMOKY | -11.4 | -17.9 | -17.8 | -10.3 | -12.0 / -11.3 | -19.9 / -17.8 |
| WHEELHOUSE | SUNDAY SERVICE | -11.7 | -18.1 | -18.1 | -11.0 | -12.6 / -11.9 | -20.9 / -18.3 |
| WHEELHOUSE | ROCK OUT | -12.2 | -17.9 | -17.8 | -10.1 | -12.7 / -11.2 | -18.4 / -17.0 |
| WHEELHOUSE | BALLPARK | -11.7 | -18.2 | -18.3 | -9.4 | -12.0 / -10.4 | -18.2 / -17.1 |
| WHEELHOUSE | FLUTES | -12.0 | -18.1 | -18.0 | -7.7 | -12.0 / -10.8 | -17.8 / -17.5 |

Each instrument's reference (the median of its five) and the spread after:

| instrument | reference (LUFS) | presets from it (LU) |
|---|---|---|
| THUMP LAB | -18.1 | -0.1, +0.0, +0.2, -0.4, +0.8 |
| SQUELCH | -17.6 | -0.5, +0.5, +0.0, -0.7, +0.1 |
| WOBBLER | -16.5 | +0.0, -0.1, -0.2, +0.1, +0.1 |
| CHORDETTE | -17.8 | -0.3, +0.0, -0.4, +0.0, +0.0 |
| RAZOR | -18.1 | +0.0, +0.0, +0.3, +0.1, -0.1 |
| HAZE | -18.0 | +0.0, +0.0, +0.1, +0.2, -0.2 |
| PIXEL | -17.9 | -0.2, +0.0, +0.0, +0.0, +0.0 |
| TINE | -18.0 | +0.1, +0.0, -0.2, +0.1, -0.2 |
| GLINT | -18.0 | -0.2, +0.0, +0.2, +0.0, -0.2 |
| PLUCK | -21.8 | +0.0, -3.0, -1.7, +0.7, +0.8 |
| VOX | -17.9 | +0.0, -0.1, +0.0, -0.3, +0.1 |
| WHEELHOUSE | -18.1 | +0.2, +0.0, +0.2, -0.1, +0.0 |

PLUCK NYLON and MUTED stay 1.6-3 LU under PLUCK's reference: at the -2 dBFS ceiling already (their pick's peak), louder means the limiter. A follow-up for their sound (a softer pick at the same brightness), not for the level.

**Key tracking of level** (`TFM_KT`: the top of the home range, then half dB less per octave above it, per voice at its note: `voice.c` `voice_kgain`), set from the "+1 oct" rise: SQUELCH from C3 2.5 dB/oct, WOBBLER from C3 2, CHORDETTE from C4 1.5, RAZOR from C5 0.5, HAZE from C4 1, PIXEL from C5 1.5, TINE none (it falls), GLINT from C5 0.5, PLUCK from C4 2, VOX from C4 3, WHEELHOUSE from C4 1.5. THUMP LAB none.

**THUMP LAB's low end.** The kick's body sat at ~160 Hz, not its designed 50 Hz: SMACK drove the kit's TONE (30..115), and TONE also holds the kick's pitch sweep up (0..+3 octaves); PUNCH and TUNE both wrote the kit's TUNE. Now TONE is fixed low in the voicing (26: the kick's energy under 100 Hz 56 %, was 33 %), SMACK is the snap and the snare and clap lanes, PUNCH is the kick lane's level, the accent and the drive (TUNE alone tunes), the hats' lanes are louder (voicing 110, HATS 80..127), and WAREHOUSE and BOOM BAP have more snare and hats. Nothing in the track or master chain thinned the lows (the insert's low cut is ~27 Hz, the master DC blocker ~2 Hz; SPEAKER EQ is FLAT by default); the limiter was the other half (every kick went through it). Each piece alone, after:

| kit | before kick / snare / hats | after kick / snare / hats |
|---|---|---|
| WAREHOUSE | -21.3 / -26.7 / -35.1 | -18.1 / -20.6 / -20.7 |
| BOOM BAP | -23.4 / -23.8 / -33.0 | -19.9 / -21.2 / -19.4 |
| DISCO | -22.3 / -24.9 / -22.5 | -22.1 / -24.1 / -18.9 |
| TRASH CAN | -27.6 / -32.3 / -38.3 | -18.0 / -22.3 / -24.3 |
| ARCADE | -22.2 / -24.0 / -32.8 | -17.8 / -19.7 / -23.3 |

**Macros** (each on the instrument's first named preset at 0 / 50 / 100; a level target running against the ones that brighten, resonate, drive or add space). Changed: THUMP PUNCH, SMACK, TIME, DIRT, HATS; SQUELCH RESO, ENV MOD, BITE, GNAW; WOBBLER GROWL (no level target: it over-compensated), DIRT; CHORDETTE WARM; RAZOR CUT; HAZE SPACE, TONE (its band-pass swept off the sound: 10..28 now); PIXEL SWEEP, DUTY; TINE WARMTH; GLINT METAL; PLUCK BRIGHT, BOW, SPACE; VOX BUZZ, SPACE; WHEELHOUSE DRIVE, BODY. Those kinds of macros stay within 5 LU over the whole turn (the test). Left as they are, and why: the ones that change the note's length or movement (SQUELCH DECAY +7, PLUCK DAMP +16, THUMP TIME, HAZE BLOOM / HOLD / MOVE, TINE TREM, CHORDETTE PUMP, VOX BREATH, RAZOR WIDTH) and RAZOR SWEEP (its filter envelope; RAZOR CUT holds the level target: two macros writing LEVEL fight, the last one moved wins - an old limit of the macro design, worth its own change).

| instrument | macro | before 0 / 50 / 100 | after 0 / 50 / 100 |
|---|---|---|---|
| THUMP LAB | PUNCH | -20.3 / -21.7 / -24.5 | -20.2 / -18.5 / -17.4 |
| THUMP LAB | SMACK | -24.8 / -22.2 / -19.9 | -20.2 / -19.5 / -18.5 |
| THUMP LAB | TIME | -21.5 / -21.8 / -19.5 | -21.5 / -21.1 / -17.7 |
| THUMP LAB | ROOM | -22.4 / -22.5 / -20.9 | -18.4 / -18.2 / -17.3 |
| THUMP LAB | TUNE | -22.6 / -22.0 / -21.5 | -18.8 / -17.9 / -16.7 |
| THUMP LAB | DIRT | -19.5 / -23.0 / -26.5 | -17.5 / -18.2 / -18.2 |
| THUMP LAB | HATS | -22.3 / -22.1 / -22.2 | -18.0 / -18.2 / -18.1 |
| THUMP LAB | ECHO | -22.2 / -22.2 / -21.4 | -18.2 / -17.9 / -17.7 |
| SQUELCH | CUTOFF | -22.3 / -21.4 / -20.9 | -18.6 / -18.0 / -17.5 |
| SQUELCH | RESO | -20.7 / -22.7 / -23.7 | -18.0 / -18.2 / -17.4 |
| SQUELCH | ENV MOD | -24.3 / -23.2 / -21.4 | -18.6 / -19.3 / -19.1 |
| SQUELCH | DECAY | -24.0 / -20.5 / -13.1 | -19.3 / -18.0 / -12.0 |
| SQUELCH | BITE | -22.1 / -24.2 / -27.8 | -19.2 / -18.1 / -18.4 |
| SQUELCH | GNAW | -20.1 / -24.5 / -29.3 | -18.3 / -17.5 / -17.0 |
| SQUELCH | WAVE | -20.1 / -21.9 / -20.1 | -17.1 / -18.3 / -17.1 |
| SQUELCH | SLIDE | -21.5 / -21.5 / -21.5 | -18.1 / -18.1 / -18.1 |
| WOBBLER | WOB | -20.8 / -20.8 / -21.2 | -16.4 / -16.4 / -16.8 |
| WOBBLER | GROWL | -18.2 / -22.4 / -26.8 | -16.3 / -16.5 / -16.9 |
| WOBBLER | CUT | -22.0 / -21.1 / -20.1 | -17.6 / -16.7 / -15.7 |
| WOBBLER | SPREAD | -20.7 / -21.0 / -21.3 | -16.4 / -16.6 / -16.9 |
| WOBBLER | DIRT | -18.2 / -20.4 / -21.1 | -14.4 / -15.0 / -14.2 |
| WOBBLER | SLIDE | -20.9 / -21.0 / -21.0 | -16.5 / -16.6 / -16.6 |
| CHORDETTE | WARM | -18.1 / -13.7 / -12.7 | -20.1 / -17.8 / -19.6 |
| CHORDETTE | SWELL | -13.5 / -13.3 / -14.0 | -18.2 / -18.0 / -18.9 |
| CHORDETTE | SHIMMER | -12.4 / -12.4 / -11.7 | -17.3 / -16.7 / -16.0 |
| CHORDETTE | SPACE | -13.6 / -12.7 / -12.1 | -18.6 / -17.4 / -16.0 |
| CHORDETTE | PUMP | -12.2 / -13.8 / -15.1 | -17.0 / -18.7 / -20.1 |
| CHORDETTE | AGE | -13.3 / -13.3 / -13.5 | -18.3 / -18.3 / -18.8 |
| RAZOR | SCREAM | -10.1 / -9.8 / -10.3 | -22.4 / -22.2 / -20.9 |
| RAZOR | SWEEP | -13.7 / -9.9 / -10.0 | -24.6 / -17.3 / -16.3 |
| RAZOR | CUT | -18.0 / -10.6 / -9.8 | -23.4 / -18.5 / -20.9 |
| RAZOR | GLIDE | -10.1 / -10.4 / -10.1 | -18.4 / -18.0 / -18.0 |
| RAZOR | WIDTH | -9.4 / -10.0 / -10.9 | -15.6 / -17.5 / -19.4 |
| RAZOR | ATTACK | -10.0 / -10.1 / -10.8 | -18.3 / -18.3 / -19.9 |
| RAZOR | VIB | -9.9 / -10.0 / -9.7 | -18.5 / -18.6 / -18.1 |
| RAZOR | ECHO | -10.1 / -9.9 / -10.2 | -18.9 / -17.9 / -16.8 |
| HAZE | DRIFT | -11.6 / -11.3 / -11.4 | -18.2 / -18.0 / -18.1 |
| HAZE | DUST | -11.5 / -11.5 / -11.4 | -18.0 / -18.1 / -18.3 |
| HAZE | SPACE | -11.2 / -11.3 / -11.5 | -18.0 / -18.2 / -18.1 |
| HAZE | BLOOM | -11.5 / -11.3 / -12.4 | -17.9 / -17.9 / -30.5 |
| HAZE | TONE | -11.1 / -11.8 / -24.3 | -17.6 / -18.4 / -21.5 |
| HAZE | SHINE | -11.5 / -11.5 / -11.3 | -18.3 / -17.4 / -16.7 |
| HAZE | MOVE | -11.0 / -11.6 / -12.1 | -17.5 / -19.6 / -23.7 |
| HAZE | HOLD | -11.5 / -11.5 / -11.6 | -20.5 / -18.0 / -17.0 |
| PIXEL | SWEEP | -9.5 / -10.4 / -14.0 | -19.1 / -18.5 / -20.6 |
| PIXEL | CRUSH | -9.5 / -9.5 / -9.5 | -18.1 / -18.1 / -18.1 |
| PIXEL | VIB | -9.5 / -9.4 / -9.6 | -18.1 / -18.0 / -18.1 |
| PIXEL | DECAY | -9.5 / -9.5 / -8.2 | -18.2 / -18.0 / -16.3 |
| PIXEL | TONE | -10.8 / -9.7 / -9.4 | -19.3 / -18.2 / -17.9 |
| PIXEL | DUTY | -12.3 / -10.4 / -9.2 | -18.7 / -18.4 / -18.8 |
| TINE | BARK | -11.2 / -11.1 / -11.3 | -17.7 / -17.8 / -18.4 |
| TINE | TINE | -11.1 / -11.2 / -11.1 | -17.8 / -17.8 / -17.9 |
| TINE | TREM | -11.1 / -11.6 / -13.1 | -17.2 / -18.8 / -20.9 |
| TINE | CHORUS | -11.2 / -10.9 / -10.4 | -18.0 / -17.5 / -15.8 |
| TINE | DECAY | -11.2 / -11.1 / -11.2 | -17.7 / -17.8 / -17.8 |
| TINE | WARMTH | -10.9 / -11.6 / -14.9 | -17.6 / -17.8 / -17.9 |
| TINE | SPACE | -11.1 / -11.1 / -11.4 | -18.0 / -17.0 / -15.9 |
| TINE | REL | -11.2 / -11.1 / -11.2 | -17.7 / -17.9 / -17.7 |
| GLINT | METAL | -18.5 / -17.1 / -14.9 | -18.1 / -18.2 / -17.5 |
| GLINT | BLOOM | -18.2 / -17.2 / -16.9 | -19.2 / -18.2 / -18.0 |
| GLINT | DECAY | -17.2 / -17.1 / -17.2 | -18.2 / -18.2 / -18.2 |
| GLINT | BITE | -17.2 / -17.1 / -17.1 | -18.2 / -18.2 / -18.2 |
| GLINT | ATTACK | -17.2 / -17.1 / -17.2 | -18.2 / -18.2 / -18.2 |
| GLINT | SPACE | -17.3 / -16.6 / -14.9 | -18.4 / -17.7 / -15.9 |
| GLINT | SHIMMER | -16.9 / -18.1 / -18.9 | -18.0 / -19.2 / -20.0 |
| PLUCK | BRIGHT | -33.0 / -32.2 / -28.9 | -23.9 / -24.0 / -22.8 |
| PLUCK | DAMP | -37.0 / -29.0 / -17.4 | -28.0 / -20.4 / -11.3 |
| PLUCK | BODY | -29.8 / -31.0 / -31.1 | -21.0 / -22.3 / -22.6 |
| PLUCK | PICK | -27.0 / -29.2 / -28.9 | -18.8 / -20.4 / -20.4 |
| PLUCK | BOW | -30.6 / -25.7 / -21.8 | -21.4 / -20.2 / -20.3 |
| PLUCK | ACCENT | -32.3 / -30.9 / -29.6 | -23.4 / -22.2 / -21.0 |
| PLUCK | SPACE | -31.3 / -30.0 / -27.7 | -21.4 / -21.6 / -21.3 |
| PLUCK | ECHO | -30.9 / -30.7 / -30.0 | -22.0 / -21.7 / -21.2 |
| VOX | VOWEL | -10.8 / -10.8 / -10.8 | -17.9 / -17.9 / -17.9 |
| VOX | TALK | -11.1 / -10.8 / -11.0 | -18.5 / -17.8 / -18.7 |
| VOX | BUZZ | -11.2 / -11.2 / -10.7 | -19.6 / -19.9 / -19.6 |
| VOX | BREATH | -10.9 / -11.0 / -11.4 | -17.8 / -18.8 / -21.0 |
| VOX | Q | -11.3 / -10.8 / -10.3 | -19.1 / -17.6 / -16.6 |
| VOX | CHOIR | -10.8 / -11.2 / -11.3 | -17.4 / -19.1 / -18.6 |
| VOX | SPACE | -10.6 / -11.1 / -11.2 | -17.6 / -18.0 / -17.5 |
| WHEELHOUSE | DRIVE | -11.4 / -11.5 / -12.1 | -22.1 / -20.7 / -22.0 |
| WHEELHOUSE | CLICK | -11.4 / -11.3 / -11.3 | -18.0 / -18.1 / -18.0 |
| WHEELHOUSE | SUB | -11.2 / -11.2 / -11.8 | -17.1 / -17.1 / -18.2 |
| WHEELHOUSE | BODY | -13.9 / -11.3 / -11.3 | -20.0 / -17.1 / -20.6 |
| WHEELHOUSE | TOP | -11.4 / -11.4 / -11.6 | -17.8 / -17.7 / -18.8 |
| WHEELHOUSE | SPACE | -11.0 / -11.6 / -11.4 | -18.3 / -17.6 / -16.0 |

## MORPH (round 8)

Macro 1 of every instrument is **MORPH**, and it is JAM's knob. One turn bends the sound from the preset as it is set (0) to a
contrasting voicing of the same instrument (100), many values together (`tfm_instr.c` `TFM_MORPH`). It works from where the
sound is: the values it moves are taken when it leaves 0 and put back exactly at 0; another macro turned while MORPH is out
moves that base, and MORPH bends from there. Level within 3 LU over the turn (a level offset per instrument, measured), in key
at 50 and 100 (YIN, +-35 cents), 0 and 100 may be wild, every point between usable (straight lines between the two).

| instrument | 0 (as set) -> 100 |
|---|---|
| THUMP LAB | tight and dry -> big, crushed, a room (decay, room, the INSERT driven, drive, hats up) |
| SQUELCH | round and mellow -> screaming acid (cutoff, resonance, envelope, bite, decay, gnaw) |
| WOBBLER | a soft sway -> a heavy, growling wobble (wob depth, resonance, cutoff, growl, dirt) |
| CHORDETTE | warm and still -> bright, pumping, wide (cutoff, chorus, the pump, space) |
| RAZOR | thin and dark -> screaming, wide, echoing (resonance, drive, cutoff, envelope, detune, spread, echo) |
| HAZE | dark and still -> bright and moving (the band up, filter movement, shine, space, dust) |
| PIXEL | a clean chip -> crushed, bright, ringing (crush, duty, decay, tone, space; no pitch sweep: in key) |
| TINE | soft and dry -> barking, trembling, wet (bark, tine, tremolo, chorus, warmth, space) |
| GLINT | pure -> metallic, blooming, shimmering (metal, bloom, decay, space, shimmer) |
| PLUCK | muted and short -> bright, ringing, wide (brightness, ring, accent, space; no bow: it pulls the pitch) |
| VOX | a closed mumble -> a buzzing choir (buzz, Q, talk, choir, spread, space) |
| WHEELHOUSE | clean -> driven, the rotor fast, clicking (drive, rotor at 50, click, top, space) |

To make room, one macro per instrument went (the other seven keep their jobs, one place later): THUMP LAB ECHO, SQUELCH GNAW,
WOBBLER SPREAD, CHORDETTE AGE, RAZOR ECHO, HAZE HOLD, PIXEL TONE, TINE REL, GLINT SHIMMER, PLUCK ECHO, VOX SHIFT, WHEELHOUSE
CLICK (most of them live on inside MORPH). Every named preset keeps its sound: the value its old place gave is now one of the
preset's own changes. MACROS-TO-TUNE.md lists MORPH's targets per instrument.
