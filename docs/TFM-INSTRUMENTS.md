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
