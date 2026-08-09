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

Usage: ./lang_validate.py <disc.bin> <workdir> [--strict] [--packart <dir>] [--hdpack <pack>]
       --hdpack checks work/backgrounds/<hash>.webp (F2): valid WebP, exactly 1280x960, a 16-hex hash
       that is a real background in that HD pack. (Backgrounds render only at internal scale >= 2.)
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


def cols(s, string_table=None, _visiting=frozenset(), _cycles=None):
    """Rendered column count: $X free, #N expands, each codepoint = 1 column.
    A #N chain that revisits an index is a reference CYCLE (translator typo): expansion stops
    there -- the back-reference measures 0 -- instead of recursing forever, and the index is
    recorded in _cycles for validate() to report as an error. Nothing downstream guards a cycle
    (the builder ships #N through untouched), so this is the one place it gets caught."""
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
                if idx in _visiting:
                    if _cycles is not None:
                        _cycles.add(idx)
                elif 0 <= idx < len(string_table):
                    e = string_table[idx]
                    ins = (e.get("text") or e.get("en") or "")
                out += cols(ins, string_table, _visiting | {idx}, _cycles)
                i += len(m.group(0))
                continue
        out += 1
        i += 1
    return out


def load_hdpack_hashes(hdpack, errors):
    """The set of valid background hashes from an HD pack, for the membership check. Prefers the
    manifest.json "hashes" list; falls back to the <hash>.webp filenames under backgrounds/. Accepts
    --hdpack pointing at the pack root (hdpacks/) OR straight at a backgrounds/ folder."""
    for mp in (os.path.join(hdpack, "manifest.json"),
               os.path.join(os.path.dirname(hdpack.rstrip("/\\")), "manifest.json")):
        if os.path.exists(mp):
            try:
                m = load_json(mp)
                if isinstance(m.get("hashes"), list):
                    return set(m["hashes"])
            except SystemExit:
                pass
    hs = {os.path.splitext(os.path.basename(f))[0]
          for d in (os.path.join(hdpack, "backgrounds"), hdpack)
          for f in glob.glob(os.path.join(d, "*.webp"))}
    if not hs:
        errors.append(f"--hdpack {hdpack}: no manifest \"hashes\" and no <hash>.webp files found")
    return hs


def check_backgrounds(work, hdpack, errors, warns):
    """F2 (exchange/92): validate work/backgrounds/<hash>.webp -- a valid WebP, exactly 1280x960, a
    16-hex hash name, and (with --hdpack) a hash that is a real background in that pack. Localized
    backgrounds render ONLY at internal scale >= 2 (the hi-res pass), so warn about that."""
    files = sorted(glob.glob(os.path.join(work, "backgrounds", "*.webp")))
    if not files:
        return
    try:
        from PIL import Image
    except ImportError:
        errors.append("backgrounds need Pillow to validate (pip install pillow)")
        return
    allowed = load_hdpack_hashes(hdpack, errors) if hdpack else None
    for f in files:
        name = os.path.basename(f); stem = os.path.splitext(name)[0]
        if not re.fullmatch(r"[0-9a-f]{16}", stem):
            errors.append(f"backgrounds/{name}: name must be a 16-hex <hash>.webp (from the HD pack)")
            continue
        try:
            im = Image.open(f); im.load()
        except Exception as ex:
            errors.append(f"backgrounds/{name}: not a valid image ({ex})")
            continue
        if im.format != "WEBP":
            errors.append(f"backgrounds/{name}: is a {im.format} file with a .webp name -- must be "
                          f"real WebP (the runtime decodes WebP; a renamed PNG/JPEG will not load)")
            continue
        if im.size != (1280, 960):
            errors.append(f"backgrounds/{name}: is {im.size[0]}x{im.size[1]}, must be 1280x960")
        if allowed is not None and stem not in allowed:
            errors.append(f"backgrounds/{name}: hash not in the HD pack -- not a real game background "
                          f"(typo?), or the wrong --hdpack")
    warns.append(f"{len(files)} localized background(s): these render only at internal scale >= 2 "
                 f"(the hi-res pass); at scale 1x the player sees the original background"
                 + ("" if hdpack else " -- pass --hdpack <pack> to check the hashes are real"))


def validate(disc, work, strict=False, packart=None, hdpack=None):
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

    # Per-table record widths, honoring the working set's declared encoding: bytes_per_char 1 on
    # gItemNamesSjis is the format-2 opt-in (exchange/91) and widens the record from 8 SJIS chars
    # to 16 1-byte chars. MIRRORS lang_build.build_fixed's rule -- validate and build must agree,
    # or every format-2 pack with a >8-char name is "validate red, build green" forever.
    fixed_chars = dict(FIXED_CHARS)
    if tables.get("gItemNamesSjis", {}).get("limit", {}).get("bytes_per_char") == 1:
        fixed_chars["gItemNamesSjis"] = 16

    # #N reference cycles: every #N points into gStringTable, so any cycle is reachable from one
    # of its own member entries -- one pass over that table finds them all, and later cols() calls
    # (which expand with the same guard, silently) can no longer crash on one.
    cyc = set()
    for e in st_entries:
        cols(e.get("text") or e.get("en") or "", st_entries, _cycles=cyc)
    for idx in sorted(cyc):
        errors.append(f"gStringTable[{idx}]: #N reference cycle -- the chain returns to this "
                      f"entry; the engine cannot render it")

    for name, width in fixed_chars.items():
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
            if table in fixed_chars:              # gSpellNames: hard record width in characters
                w = fixed_chars[table]
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
    # F2: after the strict flip, so a real background defect is always an error while the "needs
    # scale >= 2" note stays a warning even under --strict (it is guidance, not a defect).
    check_backgrounds(work, hdpack, errors, warns)
    return errors, warns


if __name__ == "__main__":
    # Consume argv left-to-right, skipping each flag's value BY POSITION -- filtering positionals
    # by value equality dropped any positional that merely equalled a flag value
    # (`lang_validate.py disc.bin work --packart work` lost the workdir).
    strict, packart, hdpack, pos = False, None, None, []
    args = sys.argv[1:]
    i = 0
    while i < len(args):
        a = args[i]
        if a == "--strict":
            strict = True
        elif a in ("--packart", "--hdpack"):
            if i + 1 >= len(args):
                raise SystemExit(f"{a} needs a value\n\n{__doc__}")
            if a == "--packart":
                packart = args[i + 1]
            else:
                hdpack = args[i + 1]
            i += 1
        elif a.startswith("-"):
            raise SystemExit(f"unknown option {a}\n\n{__doc__}")
        else:
            pos.append(a)
        i += 1
    if len(pos) != 2:
        raise SystemExit(__doc__)
    errors, warns = validate(pos[0], pos[1], strict, packart, hdpack)
    for w in warns:
        print(f"  warn : {w}")
    for e in errors:
        print(f"  ERROR: {e}")
    print(f"\n  {len(errors)} error(s), {len(warns)} warning(s)")
    if not errors and not warns:
        print("  working set is clean")
    sys.exit(1 if errors else 0)
