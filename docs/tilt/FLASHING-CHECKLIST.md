# TILT-FM — flashing checklist for the real FM-1

You flash; nothing here was run against hardware. Sources: Felucca `BUILDING.md` / `README.md` /
`tools/fm1_install.py` (1.6.1, 5738617), aroum/fm1-custom-fw `REVERSE_ENGINEERING.md`,
Quixotic7/MvaveFM1Unbricker `README.md` (all read 2026-10-16).

## 0. Before you start (once)

- [ ] **Download official V15** (`FM-1.fwsc`) from <https://www.m-vave.com/product?id=fm-1> and keep it
      somewhere safe. Check it:
      `Get-FileHash FM-1.fwsc` → `DB1642B2B6FA5C2CCCB11FFD13878068BB28601678D3644049F99DC40E7EDB8A`.
      This is your way back; don't start without it.
- [ ] Python on Windows: `py -m pip install mido python-rtmidi`.
- [ ] Copy `tools/fm1_install.py` out of the TILT-FM tree (or run it from `\\wsl$\Ubuntu\home\<you>\TILT-FM`).
- [ ] Build is a **dev build** (identity `FM-1_900`) with `FELUCCA_OTA=1` (default). **Never flash a build
      with `FELUCCA_OTA=0` or `FELUCCA_FLASH=0`**: the FM-1 has no recovery button, so firmware without the
      update entry can only be undone through UBOOT mode.

## 1. Each flash

- [ ] **Battery charged AND USB-C plugged into the PC the whole time.** A flat battery mid-write is the
      known way to a *hard* brick (no UBOOT disk; needs the RP2040 Transporter).
- [ ] USB **data** cable, plugged **directly** into the PC (no hub). Close any other MIDI app
      (DAW, M-VAVE updater, browser tabs with Web MIDI).
- [ ] **Back up**:
  - On stock V15: there is no read-back from a running unit — write down / photograph any sounds you
    care about. (A full 1 MiB flash dump is only possible from UBOOT mode; see §3.)
  - Already on Felucca/TILT-FM: open the web installer/editor (<https://hugelton.github.io/Felucca/>)
    in Chrome/Edge and use its backup to save projects, presets and settings to a file.
- [ ] Identify the unit: `py fm1_install.py --info` → shows `FM-1_015` (stock) or `FM-1_9xx` (Felucca).
- [ ] Flash: `py fm1_install.py tilt-fm.fwsc` — confirm, then **do not touch cable or power**
      until it reports done and the FM-1 has restarted. The installer checks the identity after.
- [ ] Look for the amber dot-matrix **TILT** on boot. Credits line underneath: "TILT-FM / Felucca v1.6.1".

## 2. Back to official V15

- [ ] `py fm1_install.py FM-1.fwsc` (the unmodified V15 is allowed without `--force`; it checks the hash), **or**
- [ ] Felucca web installer → **Return to official V15**, **or** M-VAVE's own updater.
- [ ] If TILT-FM runs but won't accept the update: **while it is running**, hold **OCT− + OCT+** for 5 s
      ("UPDATE MODE IN" counts down from 2 s; letting go cancels). It reboots into WL82 UBOOT → use §3.
      (Held at *power-on* the same pair opens Felucca's panel setup instead; `main.c`.)

## 3. Unbrick: black screen, PC sees a disk "WL82 UBOOT1.00"

That is the chip's mask-ROM boot mode (USB `VID_4C4A PID_8057`) — recoverable with a plain cable.

- [ ] **If Windows offers to format a disk: Cancel.**
- [ ] Confirm: `Get-CimInstance Win32_DiskDrive | Format-Table Index, Model, InterfaceType`
      shows `WL82 UBOOT1.00 USB Device`.
- [ ] Get <https://github.com/Quixotic7/MvaveFM1Unbricker>, put `FM-1.fwsc` (V15) in its folder.
- [ ] **Administrator** PowerShell in that folder:
      ```
      py fm1_unbrick.py setup
      py fm1_unbrick.py restore FM-1.fwsc --dry-run
      py fm1_unbrick.py restore FM-1.fwsc --verify-v15
      ```
      It double-reads a full-flash backup into `backups/…/full-flash-before.bin` first (keep it),
      requires chip key `0x980F` / flash ID `0x856014`, writes only `0x4000–0x92FFF`, verifies.
      Type `WRITE` when asked.
- [ ] Exit code 3 / verify failure: **do not power off** — run `restore` again.
- [ ] Done → unplug USB, switch off, wait 5 s, switch on.
- [ ] Rule: **nothing is ever written or erased below `0x4000`** (boot head). If you ever go manual with
      `jl-uboot-tool`, the app goes at `write 0x4000 …`, never `0x0`.

### Not showing up as WL82 UBOOT1.00?

- [ ] Leave it on **2 minutes** (Felucca's boot-loop guard drops to ROM boot after two crash loops), or
- [ ] if it boots to the UI: **OCT− + OCT+** held 5 s (Felucca's UBOOT countdown), or
- [ ] **Three quick restarts**: on 10 s, off; on 10 s, off; on.
- [ ] Still nothing on USB at all → hard brick: FM-1 Transporter (XIAO RP2040 on the USB data lines;
      <https://github.com/kurogedelic/FM-1-transporter>, MvaveFM1Unbricker's `TRANSPORTER-GUIDE.md`).

## 4. The 8 scenes x 3 songs build (round 2): the flash map changed

The three user sample slots (`0xA0000..0xDBFFF`) are now the scenes' flash (`docs/FLASH-MAP.md`). TFM has no samples.

- [ ] **Back up first in the editor** (the web editor's backup: scenes, songs, presets, settings). The backup of a build
      before round 2 holds your four scenes as "slots 1-4" and any samples in the three sample slots; the new editor
      restores the four scenes into song 1 (A..D) and ignores the samples (they are wiped at the first boot).
- [ ] The first boot after the flash takes a few seconds longer: it wipes the old sample region (every sector that is
      not blank) and writes the format tag. Do not power off during that boot (a cut is safe: the old scenes stay and
      the next boot starts again; `tests/persistence_test.c` cuts power at every one of its 66 flash operations).
- [ ] After it: SCENES shows eight tiles (your A..D, E..H empty), SONGS three slots (song 1 = what you had, 2 and 3 empty).
- [ ] The migration runs once. If the region ever loses its tag (another firmware wrote there), it runs again and wipes
      whatever is in the region: scenes E..H and songs 2 and 3 are gone, A..D and song 1's globals come back from A..D.
