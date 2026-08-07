#!/usr/bin/env python3
"""lang_validate.py -- validate a translator WORKING SET against the engine's real limits.

(Rewritten for the v1.7.0 engine: the original validated a third-party DISC; this validates the
thing a translator actually edits, after lang_merge and before lang_build.)

Two layers, deliberately distinct:

  HARD RULES (errors, the pack cannot build):
    a DRY-RUN of lang_build itself -- record widths, the dialogue byte-safety rule, charmap
    code/slot capacity (17 shared cells), glyph synthesisability, the gText[10928] unpack cap,
    literal/code collisions. One source of truth: whatever the builder enforces, this reports.

  RENDER BUDGETS (warnings by default, errors with --strict):
    what fits on screen. The builder does not police columns, because a wrapping overflow is
    a *layout* choice; a translator still wants it flagged:
      dialogue lines        26 cols HARD CLIP (the tail is silently lost -> always an error)
      SHOP_T lines          30 cols, wraps
      gStringTable          20 cols (all 74 call sites)
      spell/item desc       35 cols, wraps outside the 288x36 bar past 1 row
      shop desc             29 cols, wraps; 312x90 window fits 4 rows
    Columns count RENDERED GLYPHS: control codes $X cost nothing, a multi-byte UTF-8 sequence is
    ONE column, a 2-byte SJIS pair is ONE column. #N insertions are counted at the referenced
    string's own width.

Usage: ./lang_validate.py <disc.bin> <workdir> [--strict]
"""
import glob, io, json, os, re, sys, tempfile
from contextlib import redirect_stderr, redirect_stdout

import lang_build

COLS = {"gStringTable": (20, "wraps"), "gSpellDescriptions": (35, "wraps"),
        "gItemDescriptions": (35, "wraps"), "gItemDescriptions2": (29, "wraps")}
FIXED_CHARS = {"gCharacterNames": 6, "gUnitTypeNames": 10, "gItemNames": 12,
               "gClassAdvancementNames": 16, "gSpellNames": 20, "terrainText": 11,
               "gItemNamesSjis": 8}
MSGBOX_COLS = 26
SHOP_T_COLS = 30


def cols(s, string_table=None):
    """Rendered column count: $X free, #N expands, each codepoint = 1 column."""
    s = re.sub(r"\$.", "", s)
    out = 0
    i = 0
    while i < len(s):
        if s[i] == "#" and i + 1 < len(s):
            if s[i + 1] == "#":
                out += 1
                i += 2
                continue
            m = re.match(r"#(\d+)", s[i:])
            if m and string_table is not None:
                idx = int(m.group(1))
                ins = ""
                if 0 <= idx < len(string_table):
                    e = string_table[idx]
                    ins = (e.get("text") or e.get("en") or "")
                out += cols(ins, string_table)
                i += len(m.group(0))
                continue
        out += 1
        i += 1
    return out


def validate(disc, work, strict=False):
    errors, warns = [], []

    # --- layer 1: dry-run build ----------------------------------------------------------------
    tmp = tempfile.mkdtemp(prefix="lang_validate_")
    buf = io.StringIO()
    built = True
    try:
        with redirect_stdout(buf), redirect_stderr(buf):
            lang_build.build(disc, work, tmp, "validate")
    except SystemExit:
        built = False
    if not built:
        for ln in buf.getvalue().splitlines():
            ln = ln.strip()
            if ln and ln != "BUILD FAILED:":
                errors.append(f"[build] {ln}")

    # --- layer 2: render budgets ---------------------------------------------------------------
    tables = json.load(open(os.path.join(work, "strings", "tables.json")))["tables"]
    st_entries = tables["gStringTable"]["entries"]

    for name, width in FIXED_CHARS.items():
        for e in tables[name]["entries"]:
            t = e.get("text") or ""
            if t and len(t) > width:
                errors.append(f"{e['key']}: {len(t)} chars, record holds {width} -- truncated")

    for name, (budget, _) in COLS.items():
        for e in tables[name]["entries"]:
            t = e.get("text") or ""
            if t:
                # measure what RENDERS: an embedded \n is a line break, so the budget applies to
                # the longest LINE, not the stored string (retail's own shop descriptions carry \n)
                c = max(cols(line, st_entries) for line in t.split("\n"))
                if c > budget:
                    warns.append(f"{e['key']}: {c} cols > {budget} -- wraps to an extra row "
                                 f"(may draw outside the window)")

    for f in sorted(glob.glob(os.path.join(work, "strings", "dialogue", "*.json"))):
        doc = json.load(open(f))
        stem = doc["file"]
        budget = SHOP_T_COLS if stem == "SHOP_T" else MSGBOX_COLS
        hard = stem != "SHOP_T"
        for e in doc["entries"]:
            for li, t in enumerate(e.get("text", [])):
                if not t:
                    continue
                c = cols(t, st_entries)
                if c > budget:
                    msg = (f"{e['key']} line {li}: {c} cols > {budget} -- "
                           + ("HARD CLIP, tail lost on screen" if hard else "wraps"))
                    (errors if hard else warns).append(msg)

    if strict:
        errors, warns = errors + warns, []
    return errors, warns


if __name__ == "__main__":
    if len(sys.argv) < 3:
        raise SystemExit(__doc__)
    strict = "--strict" in sys.argv
    errors, warns = validate(sys.argv[1], sys.argv[2], strict)
    for w in warns:
        print(f"  warn : {w}")
    for e in errors:
        print(f"  ERROR: {e}")
    print(f"\n  {len(errors)} error(s), {len(warns)} warning(s)")
    if not errors and not warns:
        print("  working set is clean")
    sys.exit(1 if errors else 0)
