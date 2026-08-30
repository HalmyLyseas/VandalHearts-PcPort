#!/usr/bin/env python3
"""lang_template.py -- work out which letters a finished translation needs DRAWN, and hand them to
the pack-art step.

A non-Latin (or Nordic/Polish) translation needs glyph art for every letter the game has never drawn
-- but "which letters?" was, until now, answered only at build time, when lang_build.py errors on the
first one it can't synthesise. This reads the translation you've already written (the working set's
`strings/`, exactly what the builder reads) and tells you the full list up front, split into:

  * letters the game draws on its own          -- ASCII: nothing to do
  * letters the builder SYNTHESISES from Latin  -- accented Latin (e' n~ ...): nothing to draw
  * letters that need drawn art                 -- everything else: Cyrillic, Greek, Nordic ...

The last group is what a `--packart` sheet must supply. With --out, it goes straight on to
gen_packart.py and writes a ready-to-tweak starting sheet covering exactly those letters -- so the
loop is: translate -> lang_template -> (draw/tweak the sheet) -> lang_build --packart.

No disc image and no KROMDAT are needed: the "can the game draw this?" test is a pure property of the
letter (its Unicode decomposition and the builder's mark set), imported from lang_build so the two can
never disagree.

Usage: ./lang_template.py <workdir>              report only (+ the gen_packart command to run)
       ./lang_template.py <workdir> --out <dir>  also rasterise a starting sheet into <dir>
       ./lang_template.py <workdir> --out <dir> --font other.bdf
"""
import os, sys, unicodedata

from lang_build import MARKS, KROM_MARKS, RETAIL_MAP, drawn_chars, count_untranslated
from lang_io import load_json


def glyphless_ascii(cp):
    """A printable ASCII byte the base game draws no glyph for (RETAIL_MAP==0), excluding the two
    control-code bytes '#' '$'. A pack that USES such a character -- Greek's ';' question mark, say --
    must supply a glyph for it, exactly like a non-ASCII letter, so the template must report it."""
    return 0x21 <= cp <= 0x7E and cp not in (0x23, 0x24) and RETAIL_MAP[cp] == 0

# The two glyph surfaces and which text sources land on each -- see
# platform/pc/tools/langpack/README.md, "The two fonts". The split lets the report name a
# letter a Latin pack needs on only one surface (e.g. an uppercase accent in an item name).
LARGE_TABLES = {"gItemNamesSjis"}


def synth_small(cp):
    """True if lang_build's synth_one would draw this codepoint without art: lowercase Latin base
    plus marks it knows. Mirrors synth_one's None-decision exactly (which never touches the disc)."""
    d = unicodedata.normalize("NFD", chr(cp))
    if len(d) < 2 or not ("a" <= d[0] <= "z"):
        return False
    return all(ord(m) in MARKS for m in d[1:])


def synth_large(cp):
    """True if krom_synth would draw it: any-case Latin base + known marks, at most one above-mark.
    (The rare geometry refusal -- a base too tall for the mark -- is left to the builder; a template
    that is slightly optimistic there costs one drawn cell nobody needed, never a missing letter.)"""
    d = unicodedata.normalize("NFD", chr(cp))
    if len(d) < 2 or not ("a" <= d[0] <= "z" or "A" <= d[0] <= "Z"):
        return False
    marks = [ord(m) for m in d[1:]]
    if any(m not in KROM_MARKS for m in marks):
        return False
    return len([m for m in marks if m != 0x327]) <= 1     # >1 above-mark: krom_synth refuses


def collect(work):
    """-> {cp: set_of_surfaces} for every non-ASCII character the translation actually DRAWS.
    Surfaces are 'small' / 'large'. Reads only the merged `strings/`, the builder's own input."""
    sdir = os.path.join(work, "strings")
    used = {}

    def add(text, surface):
        for ch in drawn_chars(text or ""):
            cp = ord(ch)
            if cp > 0x7F or glyphless_ascii(cp):
                used.setdefault(cp, set()).add(surface)

    tables = load_json(os.path.join(sdir, "tables.json"))["tables"]
    for name, t in tables.items():
        surface = "large" if name in LARGE_TABLES else "small"
        for e in t["entries"]:
            add(e.get("text"), surface)

    lit = os.path.join(sdir, "literals.json")
    if os.path.exists(lit):
        for e in load_json(lit)["entries"]:
            add(e.get("text"), "large" if e.get("encoding") == "sjis" else "small")

    tac = os.path.join(sdir, "tactical.json")
    if os.path.exists(tac):
        for e in load_json(tac)["entries"]:
            add(e.get("text"), "small")               # spell names + descriptions both draw 8x9

    dd = os.path.join(sdir, "dialogue")
    if os.path.isdir(dd):
        for fn in os.listdir(dd):
            for e in load_json(os.path.join(dd, fn))["entries"]:
                for line in (e.get("text") or []):
                    add(line, "small")

    # Movie subtitles render in the large (16x15) font, so cues join the scan on the large
    # surface and mirror the builder's own letter-casing rule. See
    # platform/pc/tools/langpack/README.md, "Movie subtitles".
    cdir = os.path.join(sdir, "cues")
    if os.path.isdir(cdir):
        for fn in os.listdir(cdir):
            if fn.endswith(".json"):
                for c in load_json(os.path.join(cdir, fn)).get("cues", []):
                    for ch in (c.get("text") or ""):
                        if ord(ch) <= 0x7F:
                            add(ch, "large")
                        elif synth_large(ord(ch)):
                            add(ch, "large")
                        else:
                            add(ch.upper(), "large")
    return used


def classify(used):
    """Split collected codepoints into (needs_art{cp:surfaces}, synthesised[cp])."""
    needs, synthd = {}, []
    for cp, surfaces in used.items():
        art = set()
        if "small" in surfaces and not synth_small(cp):
            art.add("small")
        if "large" in surfaces and not synth_large(cp):
            art.add("large")
        if art:
            needs[cp] = art
        else:
            synthd.append(cp)
    return needs, sorted(synthd)


def name_of(cp):
    try:
        return unicodedata.name(chr(cp))
    except ValueError:
        return "?"


def main():
    if len(sys.argv) < 2 or sys.argv[1].startswith("-"):
        raise SystemExit(__doc__)
    work = sys.argv[1]
    out = sys.argv[sys.argv.index("--out") + 1] if "--out" in sys.argv else None
    font = sys.argv[sys.argv.index("--font") + 1] if "--font" in sys.argv else None

    used = collect(work)
    needs, synthd = classify(used)
    cps = sorted(needs)

    if synthd:
        print(f"{len(synthd)} accented Latin letter(s) synthesised automatically (no art needed): "
              + " ".join(chr(c) for c in synthd))

    if not cps:
        print("\nThis translation needs NO drawn glyph art -- it builds as a Latin pack "
              "(lang_build.py without --packart).")
        return

    print(f"\n{len(cps)} letter(s) need drawn art (supply them with --packart):\n")
    for cp in cps:
        where = needs[cp]
        tag = ("small+large (8x9 and 16x15)" if where == {"small", "large"}
               else "large only (16x15, item names)" if where == {"large"}
               else "small only (8x9)")
        print(f"  U+{cp:04X}  {chr(cp)}   {name_of(cp):<40} {tag}")

    untr = count_untranslated(work)
    if untr:
        print(f"\n[!] {untr} string(s) are still untranslated. A non-Latin pack renders each "
              f"untranslated\n    string as NONSENSE, so finish the translation before the "
              f"real build -- and re-run\n    this tool then, in case the remaining text needs "
              f"more letters than are listed above.")

    cps_arg = ",".join(f"U+{cp:04X}" for cp in cps)
    if out:
        from gen_packart import generate, DEFAULT_FONT
        for name, n, missing in generate(out, cps, font or DEFAULT_FONT):
            print(f"\n{name}: {n}/{len(cps)} glyph(s)"
                  + (f"  -- NOT in the font: {[hex(c) for c in missing]} (draw these cells by hand)"
                     if missing else ""))
        print(f"\nwrote {out}/ -- review proof_8x9.png / proof_16x15.png, tweak any cell you like, "
              f"then:\n    lang_build.py <disc> {work} <outdir> --lang <name> --packart {out}")
    else:
        print(f"\nGenerate a starting sheet for exactly these letters with:\n"
              f"    gen_packart.py <dir> --cps {cps_arg}\n"
              f"or let this tool do it: re-run with  --out <dir>")


if __name__ == "__main__":
    main()
