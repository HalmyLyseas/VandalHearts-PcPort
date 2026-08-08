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

Usage: ./lang_validate.py <disc.bin> <workdir> [--strict] [--packart <dir>]
       --packart validates a NON-LATIN working set in script mode (pass the same sheets you build
       with); without it a Cyrillic/Greek set is checked as if it were Latin and mis-reports.
"""
import glob, io, os, re, shutil, sys, tempfile
from lang_io import load_json
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


def validate(disc, work, strict=False, packart=None):
    errors, warns = [], []

    # --- layer 1: dry-run build ----------------------------------------------------------------
    # Pass --packart so a NON-LATIN working set is validated in the mode it will actually build in
    # (script mode / 1-byte codes); without it the builder would reject every Cyrillic string as a
    # bad Latin one. allow_incomplete keeps an unfinished script pack from hard-failing here, so the
    # other errors (glyph capacity, collisions, budgets) still surface; completeness is reported
    # separately below.
    tmp = tempfile.mkdtemp(prefix="lang_validate_")
    buf = io.StringIO()
    built = True
    try:
        with redirect_stdout(buf), redirect_stderr(buf):
            # The dry-run name must PASS check_pack_name, or build() bails at the name check before
            # running a single hard-rule test -- which silently made this whole layer a no-op while
            # the name was "validate". "zz-validate" satisfies the <tag>-<desc> convention.
            lang_build.build(disc, work, tmp, "zz-validate", packart=packart, allow_incomplete=True)
    except SystemExit:
        built = False
    if not built:
        for ln in buf.getvalue().splitlines():
            ln = ln.strip()
            # Keep the hard-rule errors (listed under "BUILD FAILED:"); drop the builder's own
            # progress/notes ("[lang] ...": font loaded, untranslated note, incomplete warning) --
            # they are stderr chatter, not validation findings.
            if ln and ln != "BUILD FAILED:" and not ln.startswith("[lang]"):
                errors.append(f"[build] {ln}")
    if packart:                                    # script mode: incompleteness renders as nonsense
        n = lang_build.count_untranslated(work)
        if n:
            (errors if strict else warns).append(
                f"{n} string(s) untranslated -- a non-Latin pack renders each as nonsense; the final "
                f"build refuses an incomplete pack (this is a warning so mid-work validation is usable)")

    # --- layer 2: render budgets ---------------------------------------------------------------
    tables = load_json(os.path.join(work, "strings", "tables.json"))["tables"]
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
        doc = load_json(f)
        stem = doc["file"]
        # Read the budget the EXPORTER recorded rather than restating the rule here -- it knows the
        # render path per file, and per ENTRY where they differ (the battle condition panel is drawn
        # by DrawText, not the message box). Falling back keeps older working sets valid.
        fileb = doc.get("render") or ({"max_cols": SHOP_T_COLS, "wraps": True} if stem == "SHOP_T"
                                      else {"max_cols": MSGBOX_COLS, "wraps": False})
        for e in doc["entries"]:
            eb = e.get("render") or fileb
            budget, hard = eb["max_cols"], not eb.get("wraps", False)
            exempt = set(e.get("render_exempt", []))      # proven not drawn by this path
            for li, t in enumerate(e.get("text", [])):
                if not t or li in exempt:
                    continue
                c = cols(t, st_entries)
                if c > budget:
                    msg = (f"{e['key']} line {li}: {c} cols > {budget} -- "
                           + ("HARD CLIP, tail lost on screen" if hard else "wraps"))
                    (errors if hard else warns).append(msg)

    # Code literals: the exporter records a per-call-site column budget on each one (`limit.max_cols`,
    # the tightest DrawText site). The builder does not police it, so a literal that overflows its
    # menu slot sailed through unchecked -- e.g. the world menu and the option panels at 10 cols.
    lp = os.path.join(work, "strings", "literals.json")
    if os.path.exists(lp):
        for e in load_json(lp)["entries"]:
            t = e.get("text") or ""
            lim = e.get("limit")
            if not t or not lim:
                continue                          # untranslated, or a site with no recorded budget
            budget = lim["max_cols"]
            for line in t.split("\n"):            # a multi-line menu literal: each line has the budget
                c = cols(line, st_entries)
                if c > budget:
                    warns.append(f"literal {e['key']}: {c} cols > {budget} -- wraps "
                                 f"(overflow costs a row; {lim.get('note', '')})".rstrip(" ;"))

    # Tactical layer: its strings target the retail tables, so they carry those tables' budgets --
    # gSpellNames a 20-char HARD record, the two description tables a wrapping column count. Neither
    # was checked (only the builder's 20-char gSpellNames rule fired, via the dry-run).
    tp = os.path.join(work, "strings", "tactical.json")
    if os.path.exists(tp):
        for e in load_json(tp)["entries"]:
            t = e.get("text") or ""
            if not t:
                continue
            key = e["key"]
            table = key.split("[")[0]
            if table in FIXED_CHARS:              # gSpellNames: hard record width in characters
                w = FIXED_CHARS[table]
                if len(t) > w:
                    errors.append(f"tactical {key}: {len(t)} chars > {w} -- record truncates")
            elif table in COLS:                   # descriptions: wrapping column budget
                budget = COLS[table][0]
                c = max(cols(line, st_entries) for line in t.split("\n"))
                if c > budget:
                    warns.append(f"tactical {key}: {c} cols > {budget} -- wraps to an extra row")

    shutil.rmtree(tmp, ignore_errors=True)         # the dry-run pack held translated content -- drop it

    if strict:
        errors, warns = errors + warns, []
    return errors, warns


if __name__ == "__main__":
    if len(sys.argv) < 3:
        raise SystemExit(__doc__)
    strict = "--strict" in sys.argv
    packart = sys.argv[sys.argv.index("--packart") + 1] if "--packart" in sys.argv else None
    pos = [a for a in sys.argv[1:] if not a.startswith("-") and a != packart]
    errors, warns = validate(pos[0], pos[1], strict, packart)
    for w in warns:
        print(f"  warn : {w}")
    for e in errors:
        print(f"  ERROR: {e}")
    print(f"\n  {len(errors)} error(s), {len(warns)} warning(s)")
    if not errors and not warns:
        print("  working set is clean")
    sys.exit(1 if errors else 0)
