# TILT-FM

**TILT-FM is a modified version of Felucca** (by Leo Kuroshita, Hügelton Instruments), made by CDM Gaming in 2026.
It adds TILT's quick-start ideas inside Felucca's UI: the TILT layer (hold ENV), NEW SONG (key, scale, tempo and four
role-aware sounds, HOLD and NUDGE, START), STUCK (RANDOM or COOK on the current track), a TIMER with a TIME ending and
a timed take, and the amber TILT look. Like Felucca it is GPL-3.0-only; everything below is Felucca's own README,
unchanged. Changes: `git log` from `86655d0` on the `tilt` branch; flashing: [docs/tilt/FLASHING-CHECKLIST.md](docs/tilt/FLASHING-CHECKLIST.md).

TILT-FM's identity is **FM-1_984** (its own: Felucca's is FM-1_900, its releases FM-1_9XY), and every build has it. A
release: set `TILT_VERSION` in `firmware/src/ui.c`, then `./build.sh --release X.Y.Z`; `build/release-X.Y.Z/` holds
`tilt-fm-X.Y.Z.fwsc`, the app, `SHA256SUMS` and the licences. The build itself is Felucca's: see
[BUILDING.md](BUILDING.md). It keeps Felucca's update loader, so it can be left again to any firmware or to the
official V15.

---

# Felucca

[![License: GPL-3.0-only](https://img.shields.io/badge/license-GPL--3.0--only-blue.svg)](LICENSE)

## Maintenance Notice

What began as a personal experiment grew far beyond my expectations. I’m grateful to everyone who contributed, supported the project, and made music with Felucca.

I’ve decided to step back from active development to focus on creating new instruments and music.

Felucca will remain available with occasional maintenance, but new features, issue responses, and pull request reviews are not guaranteed.

Thank you for everything. I hope you’ll look forward to my next creations.

## Overview

Multi-engine synthesizer firmware for the M-VAVE FM-1, version 1.6.1. Connect the FM-1 to a computer by USB, open the
[web installer](https://hugelton.github.io/Felucca/) in Chrome or Edge and press Install; no extra hardware is needed.
Installing is at your own risk: M-VAVE's updater or the installer's **Return to official V15** takes you back.
[Try it in your browser](https://hugelton.github.io/Felucca/webapp/try/), no FM-1 needed.

- Install: [web installer](https://hugelton.github.io/Felucca/)
- Edit: [web editor](https://hugelton.github.io/Felucca/webapp/editor/) ([Felucca-WebApp](https://github.com/hugelton/Felucca-WebApp))
- Read: the manual and the release notes are in the [wiki](https://github.com/hugelton/Felucca/wiki/Home)
- Build: [BUILDING.md](BUILDING.md)
- Related: [Felucca-Engines](https://github.com/hugelton/Felucca-Engines) (the ANALOG, LOFI, TRIO, WHEEL and NOISE engines, MIT; still maintained) ·
  [felucca-bsp](https://github.com/hugelton/felucca-bsp) (the FM-1 hardware layer)

## AI disclaimer

Felucca is developed with the assistance of AI coding agents (code, tests, documentation, translation, maintenance). The
design, features, sound design and direction are the maintainer's or the community's, and no generative AI is used for music,
icons or artwork. See [On AI-Assisted Development and Responsibility](https://github.com/hugelton/Felucca/discussions/166).

## Credits and licence

Free software: [GPL-3.0-only](LICENSE); the Hügelton Sample Pack is CC0 1.0; the engines in `firmware/engines` are MIT
([Felucca-Engines](https://github.com/hugelton/Felucca-Engines)); the bundled fonts and ported DSP keep their own licences
([LICENSES/](LICENSES/), [LICENSING.md](LICENSING.md)). Thanks to everyone whose work and contributions are in it: [CREDITS.md](CREDITS.md).

M-VAVE and FM-1 are trademarks of their respective owners. Felucca is not affiliated with or endorsed by them.

Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
