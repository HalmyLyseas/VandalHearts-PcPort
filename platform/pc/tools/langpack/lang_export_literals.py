#!/usr/bin/env python3
"""lang_export_literals.py -- export the text HARDCODED IN THE CODE (exchange/80, step 1.2).

The fifth text source, and the one a call-site sweep of tables misses entirely: strings passed as C
literals straight to DrawText / DrawSjisText / StringToGlyphs. Small in count, large on screen -- the
Skill/Spell/Items panel, the battle action menu, the world menu, the options menus and "Battle
results" all live here.

IDENTITY IS THE CONTENT. Each entry is keyed by an FNV-1a hash of the literal's own bytes, the same
trick the HD pack uses for images. Consequences: identical literals at different call sites collapse
to one entry (translate once, stays consistent), no id table has to be maintained by hand, and the
planned `PC_LANGSTR("...")` hook needs no explicit id in the C either.

WHAT IS SKIPPED, and why:
  * `#N`-only templates ("#63\\n#64\\n#65") -- pure menu LAYOUT. The words come from gStringTable,
    which is already exported; there is nothing here to translate. Counted, not emitted.
  * debug_menu.c -- reachable only in debug mode, not player-facing.

LIMITS COME FROM THE CALL SITE: the 3rd argument of DrawText is the column budget, read per site. A
literal drawn at several sites takes the tightest one.

Usage: ./lang_export_literals.py <path to vh/src> <workdir>
"""
import glob, os, re, sys, unicodedata
from lang_io import write_json

DRAW = re.compile(r'\b(DrawText_Internal|DrawText|DrawSjisText|StringToGlyphs)\s*\(([^;]*?)\)\s*;', re.S)
TEMPLATE = re.compile(r'(?:"(?:#\d+|\\n)*"\s*)+$')
SKIP_FILES = {"debug_menu.c"}

# Literals proven UNREACHABLE, excluded so they cannot confuse a translator. Keyed by content hash
# (line numbers churn, content does not). Each needs a proof, not a hunch -- a merely *suspicious*
# string stays in the working set, which is why the japanese_leftovers bucket still exists below.
DEAD = {
    "literal:d47bbbda49d83b01":
        "the Japanese main menu (始めから / ロード) in main.c's Objf582_MainMenu_Jpn. "
        "OBJF_MAIN_MENU_JPN = 582 is declared in object.h and sits in the dispatch table at "
        "obj_function_pointers.c:1166, but NOTHING in src/ ever assigns it to obj->functionIndex -- "
        "so the function is never entered in the US build.",
    "literal:40feee2af0415efa":
        "sPartyNames[0], the katakana 'dummy' slot. Index 0 is the party list's NULL TERMINATOR, "
        "not a character: supplies.c writes gCurrentParty[j] = 0 to end the list and every consumer "
        "loops `while (gCurrentParty[i] != 0)`, so slot 0 is never dereferenced as a name.",
}

# Hand annotations for sites whose behaviour a translator cannot infer from the string alone. Keyed
# by the entry's own `literal:<fnv1a>` key (same identity the export uses everywhere), so the retail
# text is not spelled out here -- the annotation describes the site, it does not quote it.
NOTES = {
    # the item-pickup line's leading phrase
    "literal:f22b155f50c44ff6":
        ("⚠ SENTENCE ASSEMBLED BY PIXEL POSITION, not concatenation: battle_0201b8.c draws this "
         "leading phrase at x=16, the item name at x=80 and the trailing mark at x=80+len*8. A "
         "longer translation will overlap the item name -- this site needs its x positions moved, "
         "not just its text replaced."),
    # the trailing mark of the item-pickup line
    "literal:af639c4c86017fcc":
        ("⚠ the tail of the item-pickup line -- see the leading-phrase entry; the three pieces are "
         "positioned independently."),
    # the after-battle experience popup's leading phrase
    "literal:a06b1d432c61c1d6":
        ("⚠ the PREFIX of the after-battle experience popup: ShowExpDialog (battle_0190dc.c) draws "
         "this phrase, then the number, then the second line. Keep it short -- it shares a "
         "20-column line with the number, which is placed right after whatever you write here."),
}

# PC_LANGSTR-wrapped literals that are NOT a draw-call argument -- a prefix composed into a buffer
# before drawing (ShowExpDialog builds "You got <N>"; Objf425 builds "TURN <n>"), so the number can
# follow a translated prefix of any length. The draw-call sweep below cannot see these -- the exact
# blind spot that let the experience popup go untranslated through a whole UAT. Keyed per file by the
# {content hash: what it is} of the composed literal, so a removed or changed wrap fails the export
# PRECISELY (the hash is the literal's identity, so no retail bytes are spelled out here).
WRAPPED_LITERAL_FILES = {
    "battle_0190dc.c": {"a06b1d432c61c1d6": "the after-battle experience-popup prefix ('You got ')"},
    "battle_0201b8.c": {"a06466fe43a3714c": "the battle turn-counter prefix ('TURN')"},
    # the save-slot caption labels, recomposed at display time with the numbers (main_menu.c
    # TranslateCaption; the caption is stored English in the save and translated only for display)
    "main_menu.c": {"66eb39dcbe1a877b": "save-slot caption: the chapter label ('Chap.')",
                    "21c52824b66774d1": "save-slot caption: the section label ('Sct.')",
                    "af64014c86022b6b": "save-slot caption: the level label ('L')"},
}
PC_LANGSTR_RX = re.compile(r'PC_LANGSTR\s*\(\s*("(?:[^"\\]|\\.)*")\s*\)')


def scan_wrapped(srcdir):
    """-> [(file, raw bytes)] for the single-string PC_LANGSTR literals in the curated files. Verifies
    each EXPECTED composed literal (by hash) is present, so one silently disappearing fails the export
    instead of dropping a string. (Returns every match; the caller keeps only the not-already-found
    ones, i.e. the composed literals -- the draw-call ones are handled by the sweep above.)"""
    out = []
    for base, expected in WRAPPED_LITERAL_FILES.items():
        path = os.path.join(srcdir, base)
        if not os.path.exists(path):
            continue
        text = re.sub(r"/\*.*?\*/|//[^\n]*", "", open(path, encoding="latin1").read(), flags=re.S)
        raws = [unescape(lit) for lit in PC_LANGSTR_RX.findall(text)]
        have = {f"{fnv1a(r):016x}" for r in raws}
        for h, desc in expected.items():
            if h not in have:
                raise SystemExit(f"{base}: expected PC_LANGSTR literal {h} ({desc}) not found -- a "
                                 f"wrap was removed or changed? update WRAPPED_LITERAL_FILES")
        out += [(base, r) for r in raws]
    return out


# A single string held in a char ARRAY (not a pointer array), reached only through a pointer to it:
# sEmptyFileCaption, the "Empty" placeholder for an unused save/load slot. The draw-call sweep sees a
# variable, and scan_arrays matches pointer arrays -- so neither finds it. Its draws are wrapped
# (DrawTextWindow, and the load-menu DrawText calls), so a pack can replace it once it is exported.
# Curated by (file, symbol); a rename or removal fails the self-check below.
CHAR_ARRAY_LITERALS = {
    "main_menu.c": ["sEmptyFileCaption"],
}


def scan_char_arrays(srcdir):
    """-> [(symbol, raw bytes)] for the curated `TYPE name[] = "literal";` definitions."""
    out = []
    for base, names in CHAR_ARRAY_LITERALS.items():
        path = os.path.join(srcdir, base)
        if not os.path.exists(path):
            continue
        text = re.sub(r"/\*.*?\*/|//[^\n]*", "", open(path, encoding="latin1").read(), flags=re.S)
        for name in names:
            m = re.search(r'\b(?:static\s+)?(?:const\s+)?(?:u8|s8|char)\s+' + re.escape(name) +
                          r'\s*\[\s*\]\s*=\s*("(?:[^"\\]|\\.)*")\s*;', text)
            if not m:
                raise SystemExit(f"{base}: char-array literal {name} not found -- renamed or removed? "
                                 f"update CHAR_ARRAY_LITERALS")
            out.append((name, unescape(m.group(1))))
    return out


def unescape(c_literal):
    """Concatenated C string literals -> raw bytes."""
    out = bytearray()
    for chunk in re.findall(r'"((?:[^"\\]|\\.)*)"', c_literal, re.S):
        i = 0
        while i < len(chunk):
            ch = chunk[i]
            if ch != "\\":
                out.append(ord(ch)); i += 1; continue
            nxt = chunk[i + 1]
            if nxt == "x":
                out.append(int(chunk[i + 2:i + 4], 16)); i += 4
            elif nxt == "n":
                out.append(0x0A); i += 2
            elif nxt == "t":
                out.append(0x09); i += 2
            else:
                out.append(ord(nxt)); i += 2
    return bytes(out)


def fnv1a(b):
    h = 14695981039346656037
    for x in b:
        h = ((h ^ x) * 1099511628211) & 0xFFFFFFFFFFFFFFFF
    return h


def split_args(s):
    out, depth, cur, q, i = [], 0, "", False, 0
    while i < len(s):
        ch = s[i]
        if ch == '"' and (i == 0 or s[i - 1] != "\\"):
            q = not q
        if not q:
            if ch in "([":
                depth += 1
            if ch in ")]":
                depth -= 1
            if ch == "," and depth == 0:
                out.append(cur.strip()); cur = ""; i += 1; continue
        cur += ch; i += 1
    out.append(cur.strip())
    return out


def decode(raw):
    if any(0x81 <= b <= 0x9F or 0xE0 <= b <= 0xFC for b in raw):
        try:
            return unicodedata.normalize("NFKC", raw.decode("cp932")), "sjis"
        except UnicodeDecodeError:
            pass
    return raw.decode("latin1"), "ascii"


# Text held in ARRAYS rather than written at the draw call. The sweep above matches literals in the
# draw call's argument list, so it cannot see these -- which is how the entire title screen, every
# memory-card prompt and the party list stayed untranslated through a whole UAT pass.
#
# They are reachable anyway, because the code that CONSUMES the array can be wrapped: PC_LangStr
# hashes whatever string it is handed at run time, so one wrap where the array is read covers every
# entry in it. Curated rather than swept, because a blind scan for "array of char*" also matches
# data tables; the self-check below fails the export if one of these disappears or grows.
ARRAY_SOURCES = {
    # file            array prefix or exact name      why it is reachable
    "main_menu.c":   ("sText_",
                      "every one is passed to DrawTextWindow, whose two DrawText calls are wrapped"),
    "supplies.c":    ("sPartyNames",
                      "read into the party-list buffer at supplies.c:590, which is wrapped"),
}
# Arrays that are defined but never drawn -- excluded WITH a reason, never on a hunch.
ARRAY_DEAD = {
    "sText_InBattleSaveOrBattleStart": "marked '// Unused' in the source",
    "sUnused_80102364": "name says it; no reference anywhere in src/",
    "sText_FileSaveCaptions": "caption BUFFERS, filled at run time -- the literals are empty",
    # sText_FileLoadCaptions is NOT dead: slots 0-2 are empty run-time buffers (skipped by the
    # non-empty filter in scan_arrays), but slot 3 is a real constant, "In-battle save", drawn in
    # the load menu. Kept translatable; the empties still contribute nothing.
    "sText_LanguageOptions": "retail language menu; the port never reaches it",
}

ARRAY_RX = re.compile(
    r'\b(?:static\s+)?(?:const\s+)?(?:u8|s8|char)\s*\*\s*(\w+)\s*\[[^\]]*\]\s*=\s*\{(.*?)\n\};',
    re.S)


def scan_arrays(srcdir):
    """-> [(array, index, raw bytes)] for every drawable string held in an array."""
    out = []
    for base, (prefix, _why) in ARRAY_SOURCES.items():
        path = os.path.join(srcdir, base)
        if not os.path.exists(path):
            continue
        text = re.sub(r"/\*.*?\*/|//[^\n]*", "", open(path, encoding="latin1").read(), flags=re.S)
        for m in ARRAY_RX.finditer(text):
            name, body = m.group(1), m.group(2)
            if not name.startswith(prefix) or name in ARRAY_DEAD:
                continue
            for i, lit in enumerate(re.findall(r'"((?:[^"\\]|\\.)*)"', body)):
                raw = unescape(f'"{lit}"')
                if raw.strip():
                    out.append((name, i, raw))
    return out


def export(srcdir, workdir):
    found, templates, skipped, dead = {}, 0, 0, 0
    for path in sorted(glob.glob(os.path.join(srcdir, "*.c"))):
        base = os.path.basename(path)
        text = open(path, encoding="latin1").read()
        for m in DRAW.finditer(text):
            fn, args = m.group(1), split_args(m.group(2))
            si = 1 if fn == "StringToGlyphs" else 5
            if len(args) <= si:
                continue
            arg = args[si].strip()
            # The game's literals are wrapped in PC_LANGSTR(...) so a pack can replace them at
            # run time. Unwrap before the literal test -- otherwise this exporter silently stops
            # seeing the very strings it exists to find, and a fresh working set comes out with
            # zero literals. (That regression shipped once: the wrap landed without this.)
            unwrapped = re.match(r'PC_LANGSTR\s*\((.*)\)\s*$', arg, re.S)
            if unwrapped:
                arg = unwrapped.group(1).strip()
            if not arg.startswith('"'):
                continue
            if base in SKIP_FILES:
                skipped += 1; continue
            if TEMPLATE.match(arg.replace("\n", "").strip()):
                templates += 1; continue
            raw = unescape(arg)
            en, enc = decode(raw)
            cols = None
            if fn != "StringToGlyphs":
                try:
                    cols = int(args[2])
                except (ValueError, IndexError):
                    cols = None
            line = text[:m.start()].count("\n") + 1
            key = f"literal:{fnv1a(raw):016x}"
            if key in DEAD:
                dead += 1; continue
            e = found.setdefault(key, {"key": key, "en": en, "text": "",
                                       "encoding": enc, "sites": [], "cols": []})
            e["sites"].append(f"{base}:{line}")
            if cols:
                e["cols"].append(cols)

    # Text held in arrays: same content-hash identity, same K_LITERAL delivery, reached through the
    # wrap at the array's CONSUMER rather than at each element.
    for arr, idx, raw in scan_arrays(srcdir):
        en, enc = decode(raw)
        key = f"literal:{fnv1a(raw):016x}"
        if key in DEAD:
            dead += 1; continue
        e = found.setdefault(key, {"key": key, "en": en, "text": "",
                                   "encoding": enc, "sites": [], "cols": []})
        e["sites"].append(f"{arr}[{idx}]")

    # PC_LANGSTR wraps that are not a draw-call argument (composed into a buffer first). Only add the
    # ones the draw-call sweep did not already find -- a literal seen at its draw call keeps that
    # entry (and its column budget); this fills in the ones with no draw call of their own.
    for base, raw in scan_wrapped(srcdir):
        key = f"literal:{fnv1a(raw):016x}"
        if key in DEAD:
            dead += 1; continue
        if key in found:
            continue
        en, enc = decode(raw)
        found[key] = {"key": key, "en": en, "text": "", "encoding": enc,
                      "sites": [f"{base} (composed, not a draw-call arg)"], "cols": []}

    # Single-string char arrays drawn through a pointer (sEmptyFileCaption).
    for name, raw in scan_char_arrays(srcdir):
        key = f"literal:{fnv1a(raw):016x}"
        if key in DEAD:
            dead += 1; continue
        if key in found:
            continue
        en, enc = decode(raw)
        found[key] = {"key": key, "en": en, "text": "", "encoding": enc,
                      "sites": [name], "cols": []}

    entries = []
    for e in sorted(found.values(), key=lambda x: x["sites"][0]):
        lines = e["en"].split("\n")
        item = {"key": e["key"], "en": e["en"], "text": "",
                "lines": len(lines), "sites": e["sites"]}
        if e["encoding"] == "sjis":
            item["encoding"] = "sjis"
            # Two very different things arrive here. Full-width LATIN (ＥＮＥＭＹ ＴＵＲＮ) is live US
            # text that merely happens to be stored 2-byte. Actual kana/kanji is a Japanese leftover
            # the US build may never show. Decide by what survives NFKC, not by the encoding.
            item["japanese"] = any(ord(c) > 0x7F for c in e["en"])
            item["note"] = ("Japanese text left in the US build -- confirm in game before translating; "
                            "it may be unreachable" if item["japanese"] else
                            "live text that happens to be stored as 2-byte full-width Shift-JIS")
        if e["cols"]:
            # DrawText's column argument is a generous WRAP threshold, NOT the visible box width -- the
            # max-length stress run showed two different cases:
            #   * a MULTI-LINE menu (Move/Action/Done/...) draws in a box sized to its LONGEST OPTION,
            #     far narrower than the threshold (Examine=7 in a box whose DrawText arg is 20). Filling
            #     to the threshold wraps every option and hides half of them. Budget = longest option.
            #   * a SINGLE-LINE literal draws in a box ~= the threshold, but the last column lands on the
            #     border, so the clean width is threshold - 1.
            if len(lines) > 1:
                clean = max(len(l) for l in lines)
                note = (f"menu box is sized to its longest option ({clean} cols); the DrawText argument "
                        f"({min(e['cols'])}) is a generous wrap limit, not the box width -- a longer "
                        f"option wraps and hides the next")
            else:
                clean = min(e["cols"]) - 1
                note = (f"clean width {clean} (the DrawText budget is {min(e['cols'])}, but the box is one "
                        f"column narrower, so the last column lands on the border)")
            item["limit"] = {"max_cols": clean, "wraps": True, "note": note}
        if len(lines) > 1:
            item["options"] = lines
            item["options_note"] = ("one menu option per line -- keep the same number of lines, the "
                                    "game indexes them by position")
        n = NOTES.get(e["key"])
        if n:
            item["warning"] = n
        entries.append(item)

    doc = {"source": "string literals compiled into src/*.c",
           "note": ("text passed directly to a draw call, with no table behind it. Keyed by a hash of "
                    "the literal's own bytes, so identical strings share one entry."),
           "skipped": {"menu_templates": templates,
                       "menu_templates_note": ("literals that are only #N references -- pure layout; "
                                               "their words come from gStringTable"),
                       "debug_menu": skipped,
                       "proven_unreachable": dead,
                       "proven_unreachable_note": ("excluded with a proof recorded in DEAD; a merely "
                                                   "suspicious string is NOT excluded, it goes to "
                                                   "japanese_leftovers for confirmation")},
           "count": len(entries), "entries": entries}
    p = os.path.join(workdir, "strings", "literals.json")
    os.makedirs(os.path.dirname(p), exist_ok=True)
    write_json(doc, p)

    # Translator view: same content, split into what a person recognises, options presented as lists.
    # SCOPING RULE (committed 2026-08-06): the US retail disc is the universe -- no PAL/JP
    # consideration, and anything the US build never reaches does not exist. So there is no "confirm
    # later" limbo: a Japanese literal is either PROVEN dead (listed in DEAD, with its proof) or it is
    # an unresolved question for a MAINTAINER, never something to hand a translator or drop silently.
    unresolved = [e for e in entries if e.get("japanese")]
    if unresolved:
        raise SystemExit("unresolved Japanese literal(s) -- prove reachable (keep) or unreachable "
                         "(add to DEAD with the proof):\n  " +
                         "\n  ".join(f"{e['sites'][0]}  {e['en']!r}" for e in unresolved))

    def bucket(e):
        return "menus" if e.get("options") else "messages"
    view = {"menus": [], "messages": []}
    for e in entries:
        v = {"key": e["key"], "sites": e["sites"]}
        if e.get("options"):
            v["options"] = [{"en": o, "text": ""} for o in e["options"]]
            v["note"] = e["options_note"]
        else:
            v["en"] = e["en"]; v["text"] = ""
        if e.get("limit"):
            v["limit"] = e["limit"]
        if e.get("warning"):
            v["warning"] = e["warning"]
        if e.get("note"):
            v["note"] = e["note"]
        view[bucket(e)].append(v)
    tdoc = {"kind": "literals",
            "note": ("text baked into the game's code rather than a table. Same rules as everything "
                     "else; grouped here by what it is on screen."),
            "menus": view["menus"], "messages": view["messages"]}
    q = os.path.join(workdir, "translate", "literals.json")
    os.makedirs(os.path.dirname(q), exist_ok=True)
    write_json(tdoc, q)
    return p, q, doc, view


if __name__ == "__main__":
    if len(sys.argv) < 3:
        raise SystemExit(__doc__)
    p, q, doc, view = export(sys.argv[1], sys.argv[2])
    print(f"wrote {p}\nwrote {q}\n")
    print(f"  {doc['count']} translatable literals "
          f"({len(view['menus'])} menus, {len(view['messages'])} messages, "
          f"US retail only)")
    print(f"  skipped: {doc['skipped']['menu_templates']} #N-only templates, "
          f"{doc['skipped']['debug_menu']} debug-menu literals, "
          f"{doc['skipped']['proven_unreachable']} proven unreachable")
