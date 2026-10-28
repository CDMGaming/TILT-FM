#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""TILT-FM's web installer site (GitHub Pages, cdmgaming.github.io/TILT-FM): Felucca's make_site.py, then TILT-FM's
name on what the page shows (Felucca's credit, its licences and the checks stay as they are).

  web/make_tfm_site.py PKG VERSION OUT [--preview]

OUT gets make_site.py's layout (index.html, firmware/, webapp/installer/, webapp/editor/), the package named
tilt-fm-VERSION.fwsc. --preview: a page for a build not yet tested on an FM-1 (said in red on the page). The
installer code itself is Felucca's: identity check (the package's FM-1_9xx, FM-1_984 for TILT-FM), backup, return
to official V15."""
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import make_site  # noqa: E402

SRC = "https://github.com/CDMGaming/TILT-FM"
BETA_EN = ("TILT-FM {v} is a pre-release; please report problems on GitHub. The author accepts no responsibility if "
           "your FM-1 is damaged or misbehaves because of this custom firmware. Use it at your own risk.")
BETA_JA = ("TILT-FM {v} はプレリリース版です。不具合は GitHub で知らせてください。このカスタムファームウェアを使ったことで "
           "FM-1 が壊れたり不具合が起きたりしても、作者は一切の責任を負いません。自己責任でお使いください。")
PREVIEW = ("<p class=\"small\" style=\"color:#e5484d\"><b>Preview build {v}, not yet tested on a real FM-1.</b> "
           "For trying new features; the tested release is <a href=\"../../../webapp/installer/\" "
           "style=\"color:inherit\">here</a>.</p>")


def one(s, old, new):
    if s.count(old) != 1:
        raise SystemExit(f"installer page: expected {old!r} once; update make_tfm_site.py")
    return s.replace(old, new)


def main(pkg, version, out, preview):
    out = Path(out)
    make_site.main(pkg, version, out)
    fw = out / "firmware"
    old = fw / f"felucca-{version}.fwsc"
    new = fw / f"tilt-fm-{version}.fwsc"
    old.rename(new)
    inst = out / "webapp" / "installer" / "index.html"
    s = inst.read_text(encoding="utf-8")
    s = one(s, f'"pkg": "../../firmware/{old.name}"', f'"pkg": "../../firmware/{new.name}"')
    s = one(s, "<title>Felucca</title>", "<title>TILT-FM</title>")
    s = one(s, "<h1>Felucca</h1>", f"<h1>TILT-FM {version}</h1>" + (PREVIEW.format(v=version) if preview else ""))
    s = one(s, "say(`Felucca ${meta.version}`)", "say(`TILT-FM ${meta.version}`)")
    s = re.sub(r'beta: "Felucca 1\.0 は[^"]*"', 'beta: "' + BETA_JA.format(v=version) + '"', s, count=1)
    s = re.sub(r'beta: "Felucca 1\.0 is[^"]*"', 'beta: "' + BETA_EN.format(v=version) + '"', s, count=1)
    s = one(s, '<p class="small">Felucca by Leo Kuroshita',
            f'<p class="small">TILT-FM by CDM Gaming, a fork of Felucca: source <a href="{SRC}" target="_blank" '
            f'rel="noopener" style="color:inherit">github.com/CDMGaming/TILT-FM</a>.<br>\n    Felucca by Leo Kuroshita')
    if not preview:
        s = one(s, '<p><a href="../editor/" style="color:inherit" data-t="editor"></a></p>',
                '<p><a href="../editor/" style="color:inherit" data-t="editor"></a></p>\n'
                '  <p class="small"><a href="../../next/webapp/installer/" style="color:inherit">Preview of the next '
                'version (untested)</a></p>')
    inst.write_text(s, encoding="utf-8")
    (out / "index.html").write_text(
        '<!doctype html><meta charset="utf-8"><title>TILT-FM</title>'
        '<meta http-equiv="refresh" content="0; url=webapp/installer/">'
        '<a href="webapp/installer/">TILT-FM installer</a>\n', encoding="utf-8")
    print(f"tfm site: {out}: TILT-FM {version}{' (preview)' if preview else ''}, firmware/{new.name}")


if __name__ == "__main__":
    a = [x for x in sys.argv[1:] if x != "--preview"]
    if len(a) != 3:
        sys.exit(__doc__)
    main(*a, preview="--preview" in sys.argv[1:])
