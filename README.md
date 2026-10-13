# Felucca

[![License: GPL-3.0-only](https://img.shields.io/badge/license-GPL--3.0--only-blue.svg)](LICENSE)

![Felucca 1.0](docs/felucca-1.0.png)

**TL;DR:** Felucca 1.5.6 — conditional steps (1:2 … 4:4, FILL), EUCLID, DOUBLE / HALF, a MIDI channel per track, PHASE FOLD, three more drum kits and humanize in the MOD matrix, on top of 1.5.1's SEQ > DETAIL, MIDI Program Change, dotted delay times and REV / ECHO THROW, and 1.5's INSERT effects and MIDI LEARN. Connect your FM-1 to a computer by USB,
open the [web installer](https://hugelton.github.io/Felucca/) in Chrome or Edge, and press Install;
no extra hardware is needed. Installing is at your own risk: M-VAVE's updater or the installer's
**Return to official V15** takes you back. Coming from 1.1.5.x? Read [⚠ Coming from 1.1.5](https://github.com/hugelton/Felucca/wiki/Upgrading-from-1.1.5) first:
projects saved by 1.4 or later do not open on 1.1.5.x. Going back from 1.5.6 to 1.5 or 1.5.1? Read
[⚠ New in 1.5.6](https://github.com/hugelton/Felucca/wiki/Release-Notes#new-in-156) first: a project with conditional steps does not open there. Going back from 1.5.x to
1.4.1? Read [⚠ New in 1.5](https://github.com/hugelton/Felucca/wiki/Release-Notes#new-in-15) first: projects saved by 1.5 do not open on 1.2 to 1.4.1. Want to look around first?
[Try it in your browser](https://hugelton.github.io/Felucca/webapp/try/), no FM-1 needed.

Multi-engine synthesizer firmware for the M-VAVE FM-1. Please report what you find in
[Issues](https://github.com/hugelton/Felucca/issues).

- Install: [web installer](https://hugelton.github.io/Felucca/) (Chrome or Edge, USB), or `tools/fm1_install.py` from a terminal
- Try: [Felucca in your browser](https://hugelton.github.io/Felucca/webapp/try/): the same firmware compiled to
  WebAssembly, with the panel on screen (mouse, touch, computer keyboard, Web MIDI in)
- Editor: [web editor](https://hugelton.github.io/Felucca/webapp/editor/); its development has moved to
  [Felucca-WebApp](https://github.com/hugelton/Felucca-WebApp)
- Build: [BUILDING.md](BUILDING.md)

<a href="https://hugelton.itch.io/felucca"><img src="https://static.itch.io/images/badge-color.svg" alt="Available on itch.io" width="74"></a>

## Features

- **Thirteen engines**, each with factory presets browsable by category
- **Four tracks**, 8 voices shared, one engine and sound per track
- **Sequencer:** 64 steps per track with chords, ties, accent, slide, chance, conditions, velocity, ratchets, nudge,
  parameter locks, automation, live/step recording
- **SEQ TOOLS:** RANDOM, COOK, EUCLID, DOUBLE / HALF and more, each one undo
- **Songs** of up to 16 sections, metronome and count-in, chord keys
- **Effects, MOD matrix, MIDI** (USB and TRS, per-track channels, MIDI LEARN, Program Change), USB audio

Full feature list: [Features](https://github.com/hugelton/Felucca/wiki/Features) in the wiki.

## New in 1.5.6

- **Conditional steps** (1:2 … 4:4, FILL / !FILL) on SEQ > DETAIL
- **EUCLID, DOUBLE and HALF** in SEQ TOOLS
- **A MIDI channel per track**
- PHASE FOLD, three more drum kits, humanize in the MOD matrix
- ⚠ A project with conditional steps does not open on 1.5 or 1.5.1

All changes and downgrade notes: [Release notes](https://github.com/hugelton/Felucca/wiki/Release-Notes). Coming from 1.1.5.x: [Upgrading from 1.1.5](https://github.com/hugelton/Felucca/wiki/Upgrading-from-1.1.5).

## Documentation

The manual lives in the [wiki](https://github.com/hugelton/Felucca/wiki/Home):
[Installation](https://github.com/hugelton/Felucca/wiki/Installation) · [Controls](https://github.com/hugelton/Felucca/wiki/Controls) · [Menu](https://github.com/hugelton/Felucca/wiki/Menu) · [Engines](https://github.com/hugelton/Felucca/wiki/Engines) ·
[Scale keyboard](https://github.com/hugelton/Felucca/wiki/Scale-Keyboard) · [Layout](https://github.com/hugelton/Felucca/wiki/Layout)

## If the FM-1 does not start

See [Troubleshooting](https://github.com/hugelton/Felucca/wiki/Troubleshooting).

## Support

If Felucca is useful to you, [sponsoring on GitHub](https://github.com/sponsors/hugelton) or a donation
on [itch.io](https://hugelton.itch.io/felucca) helps keep its development going.

Issues are for reproducible bugs (one per issue). Ideas and requests go to
[Discussions](https://github.com/hugelton/Felucca/discussions), and feature requests posted as issues will be
moved there. Pull requests are welcome: see [CONTRIBUTING.md](CONTRIBUTING.md).

## AI disclaimer

Felucca is developed with the assistance of AI coding agents. These tools are used for coding, testing, documentation, translation, and maintenance.
The instrument's design, features, sound design, and overall direction are determined by the maintainer or community.
No generative AI is used to create music, icons, or visual artwork for this project.
For more details, see [On AI-Assisted Development and Responsibility](https://github.com/hugelton/Felucca/discussions/166).

## Credits

- **[Hügelton Instruments](https://hugelton.com)** (Leo Kuroshita, [@kurogedelic](https://github.com/kurogedelic)):
  Felucca itself; the PHASE engine's waveforms (a C port of the oscillator of
  [CrispyZebra](https://github.com/hugelton/CrispyZebra), GPL-3.0); the DRUM voices and kits; the Hügelton Sample
  Pack (the drum samples, GPL-3.0-only, not CC0); the [Fukiai](https://github.com/hugelton/Fukiai) icon
  font ([MIT](LICENSES/MIT-Fukiai.txt))
- Fonts: [Inter Tight](https://github.com/rsms/inter-tight) by The Inter Project Authors, [SIL OFL 1.1](LICENSES/OFL-InterTight.txt);
  the browser emulator's labels: [DotGothic16](https://github.com/fontworks-fonts/DotGothic16) by The DotGothic16 Project Authors, [SIL OFL 1.1](LICENSES/OFL-DotGothic16.txt)
- Samples: [Versilian Studios](https://versilian-studios.com/) [VSCO-2 Community Edition](https://github.com/sgossner/VSCO-2-CE) and [VCSL](https://github.com/sgossner/VCSL), CC0 1.0: the SAMPLE sets, also SLICE's PIANO and PIANO HD ([attribution](assets/samples-cc0/ATTRIBUTION.txt))
- VOICE engine: after [klattsch](https://github.com/tgies/klattsch) by Tony Gies (MIT); formant data from Klatt (1980) and Hillenbrand et al. (1995)
- PHYS engine: models ported from [DaisySP](https://github.com/electro-smith/DaisySP) by Electrosmith and Emilie Gillet ([MIT](LICENSES/MIT-DaisySP.txt)) and from Emilie Gillet's [eurorack](https://github.com/pichenettes/eurorack) code ([MIT](LICENSES/MIT-Rings.txt))
- FM6 engine: msfa from [Dexed](https://github.com/asb2m10/dexed) by Google Inc. and Pascal Gauthier ([Apache-2.0](LICENSES/Apache-2.0-msfa.txt))
- Browser emulator: after [X0X](https://github.com/charlesvestal/fm1-x0x) by [charlesvestal](https://github.com/charlesvestal) (GPL-3.0), a Felucca fork whose browser build showed the way
- Package format and boot files: [JieLi AC79 SDK](https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK) ([Apache-2.0](LICENSES/Apache-2.0.txt); three of its files are in every package, none in this tree)
- Contributions: [keremimo](https://github.com/keremimo) (white-key scales, #2), [ChanceTheMaker](https://github.com/ChanceTheMaker)
  (TRS MIDI, bend, sustain and clock, palettes, favourites, editor display settings: #8, #10, #11, #12),
  [andreahaku](https://github.com/andreahaku) (sample recording and trim, #29; SLICE manual slices and tests, #27, #22),
  [spinkham](https://github.com/spinkham) (the boot fix for 0.9 projects, #111),
  [zednaked](https://github.com/zednaked) (ratchets, #100),
  [jasonpersinger](https://github.com/jasonpersinger) (the ROOM reverb click fix, #121),
  [renebohne](https://github.com/renebohne) (MIDI Program Change, #179)

## Licence

Free software: [GPL-3.0-only](LICENSE), the Hügelton Sample Pack included. The bundled fonts and the
ported DSP keep their own licences ([LICENSES/](LICENSES/)); details in [LICENSING.md](LICENSING.md).

M-VAVE and FM-1 are trademarks of their respective owners. Felucca is not affiliated with or endorsed by them.

Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
