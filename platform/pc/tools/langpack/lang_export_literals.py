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
import glob, json, os, re, sys, unicodedata

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
}

# Hand annotations for sites whose behaviour a translator cannot infer from the string alone.
NOTES = {
    "you got ": ("⚠ SENTENCE ASSEMBLED BY PIXEL POSITION, not concatenation: battle_0201b8.c draws "
                 "\"you got \" at x=16, the item name at x=80 and \"!\" at x=80+len*8. A longer "
                 "translation will overlap the item name -- this site needs its x positions moved, "
                 "not just its text replaced."),
    "!": ("⚠ the tail of the \"you got <item>!\" line -- see that entry; the three pieces are "
          "positioned independently."),
}


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
            item["limit"] = {"max_cols": min(e["cols"]), "wraps": True,
                             "note": (f"drawn with a {min(e['cols'])}-column budget"
                                      + (" (tightest of several call sites)" if len(set(e["cols"])) > 1 else "")
                                      + "; wraps rather than clipping, so overflow costs a row")}
        if len(lines) > 1:
            item["options"] = lines
            item["options_note"] = ("one menu option per line -- keep the same number of lines, the "
                                    "game indexes them by position")
        n = NOTES.get(e["en"])
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
    json.dump(doc, open(p, "w"), indent=1, ensure_ascii=False)

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
    json.dump(tdoc, open(q, "w"), indent=1, ensure_ascii=False)
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
