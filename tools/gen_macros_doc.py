#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 CDM Gaming (TILT-FM)
"""MACROS-TO-TUNE.md from tools/tfm_ins_spec.py (the instrument sheets of tfm-spec/TFM-INSTRUMENTS.md as data, the
source of firmware/src/tfm_instr.c's TFM_INS table): every macro, its targets and what each is at 0 / 25 / 50 / 75 /
100 %, a column to tick when checked by ear.   gen_macros_doc.py OUT.md"""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from tfm_ins_spec import INS  # noqa: E402

NAMES = {"P_E%d" % i: n for i, n in enumerate(["E0", "E1", "E2", "E3", "E4", "E5", "E6", "E7"])}
ENG = {"THUMP LAB": ["KIT", "TUNE", "TONE", "DECY", "SNAP", "ACC", "KICK", "DRV"],
       "SQUELCH": ["WAVE", "DTN", "MIX", "NOIS", "CUT", "RES", "DRV", "KTR"],
       "WOBBLER": ["WAVE", "INT2", "INT3", "DTN", "MODE", "CUT", "RES", "PW"],
       "CHORDETTE": ["WAVE", "DTN", "MIX", "NOIS", "CUT", "RES", "DRV", "KTR"],
       "RAZOR": ["WAVE", "DTN", "MIX", "NOIS", "CUT", "RES", "DRV", "KTR"],
       "HAZE": ["WAVE", "INT2", "INT3", "DTN", "MODE", "CUT", "RES", "PW"],
       "PIXEL": ["CHIP", "WAVE", "DUTY", "CRSH", "SWP", "VIB", "ARP", "TONE"],
       "TINE": ["ALG", "FB", "MLVL", "MRAT", "MEG", "VMOD", "DTUN", "SLOT"],
       "GLINT": ["ALG", "FB", "MLVL", "MRAT", "MEG", "VMOD", "DTUN", "SLOT"],
       "PLUCK": ["MODEL", "STRC", "BRIT", "DAMP", "POS", "ACC", "BOW", "EXC"],
       "VOX": ["VOWL", "VOWL2", "TALK", "SHIFT", "BUZZ", "BRTH", "Q", "RAND"],
       "WHEELHOUSE": ["REG", "SUB", "BODY", "TOP", "PERC", "CLICK", "DRV", "ROTR"]}


def pname(ins, pid):
    if pid.startswith("P_E") and pid[3:].isdigit():
        return ENG[ins][int(pid[3:])]
    return pid[2:].replace("_", ">")


def val(t, pos):
    lo, hi = t[1], t[2]
    cv = t[3] if len(t) > 3 else ""
    f = pos / 100
    if cv == "E":
        f = f * f
    elif cv == "G":
        f = 0 if pos <= 70 else (pos - 70) / 30
    return round(lo + (hi - lo) * f)


L = ["# MACROS TO TUNE (TFM instruments)", "",
     "Generated from `tools/tfm_ins_spec.py` by `tools/gen_macros_doc.py`: every macro, its targets (`lo..hi`) and what "
     "each is at 0 / 25 / 50 / 75 / 100 % of the turn. Check each by ear and tick it, or change `lo` / `hi` in "
     "`tools/tfm_ins_spec.py` and `firmware/src/tfm_instr.c`. Rules (tfm-spec/TFM-INSTRUMENTS.md): a big musical change "
     "across the turn, no dead zone, 50 sounds like the instrument, no jumps (a SWITCH steps), level steady while a "
     "macro adds drive, no LEVEL macro.", ""]
for ins in INS:
    L += ["## %s" % ins["name"], "", "| # | Macro | Targets | 0 | 25 | 50 | 75 | 100 | OK? |", "|---|---|---|---|---|---|---|---|---|"]
    for n, (mn, spec) in enumerate(ins["mac"], 1):
        if isinstance(spec, tuple):
            _, pid, vals, names = spec
            sw = names or [str(v) for v in vals]
            cells = [sw[min(len(sw) - 1, p * len(sw) // 101)] for p in (0, 25, 50, 75, 100)]
            L.append("| %d | %s (SWITCH) | %s: %s | %s |  |" % (n, mn, pname(ins["name"], pid), " ".join(sw), " | ".join(cells)))
        else:
            tg = "; ".join("%s %d..%d%s" % (pname(ins["name"], t[0]), t[1], t[2], {"E": " EXP", "G": " >70%"}.get(t[3] if len(t) > 3 else "", ""))
                           for t in spec)
            cells = ["/".join(str(val(t, p)) for t in spec) for p in (0, 25, 50, 75, 100)]
            L.append("| %d | %s | %s | %s |  |" % (n, mn, tg, " | ".join(cells)))
    L.append("")
    L.append("Presets: " + ", ".join(p[0] for p in ins["pre"]) + ".")
    L.append("")
Path(sys.argv[1]).write_text("\n".join(L), encoding="utf-8")
print(len(INS), "instruments")
