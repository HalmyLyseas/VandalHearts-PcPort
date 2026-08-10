#!/usr/bin/env python3
"""lang_export_cues.py -- install the movie-subtitle cue templates into a working set.

The fifth export. Unlike the other four it does not extract from your disc: the cue timings,
cover rects, and English reference lines are maintainer-authored artifacts (per-frame validated
against the retail videos) that ship with this tooling in cues/.

    lang_export_cues.py <work>

Copies cues/*.json -> <work>/strings/cues/. Each cue carries:
    "en"   the burned-in English reference -- always visible while you translate, never ships
    "text" YOUR translation. EMPTY = that cue is not emitted, and the line keeps its burned-in
           English (a cue set is a per-line DIFF, like every langpack section -- leaving END2's
           credit slides untranslated is a deliberate example, not an error)

Frame ranges and rects are already validated; translators normally edit ONLY "text".
Existing files in <work>/strings/cues/ are left untouched (your translation is never clobbered);
delete a file to re-install its fresh template.
"""
import os
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, "cues")


def main():
    if len(sys.argv) != 2:
        raise SystemExit(__doc__)
    dst = os.path.join(sys.argv[1], "strings", "cues")
    os.makedirs(dst, exist_ok=True)
    installed = kept = 0
    for fn in sorted(os.listdir(SRC)):
        if not fn.endswith(".json"):
            continue
        target = os.path.join(dst, fn)
        if os.path.exists(target):
            kept += 1
            continue
        shutil.copyfile(os.path.join(SRC, fn), target)
        installed += 1
    print(f"cues: {installed} template(s) installed"
          + (f", {kept} existing file(s) kept" if kept else "")
          + f" -> {dst}")


if __name__ == "__main__":
    main()
