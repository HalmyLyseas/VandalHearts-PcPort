#!/usr/bin/env python3
"""lang_export_port_ui.py -- export the PORT'S OWN UI text (the SELECT+START options overlay).

The sixth text source, and the only one that never touches the game's text engine: the overlay and
its on-screen tags render with the port's 5x7 bitmap OSD font (pc_gpu_window.c), which is CAPS-ONLY
ASCII plus a little punctuation. The runtime therefore CAPS-FOLDS a translation before drawing
(PC_LangOsdStr): accents fold to their base letter ("PARAMETRES" from "Parametres" or
"PARAMETRES"), so translations may be written naturally, in mixed case with accents.

WHAT A TRANSLATOR NEEDS TO KNOW (also emitted as notes on the entries):
  * The overlay panel AUTO-SIZES to the widest string -- there are no hard column budgets. Keep
    labels reasonably short so the panel stays a panel, but nothing clips.
  * `$ @ ^ ~` are BUTTON ICONS (Square/Circle/Triangle/Cross), and X/B/Y/A in the Xbox legend
    variants are button names -- keep them, translate the words around them.
  * `%d` / `%s` placeholders are filled by the game (slot number, save caption) -- the translation
    must keep them, in the same order.

IDENTITY IS THE CONTENT, like literals.json: the key is an FNV-1a hash of the English string's own
bytes, matching what PC_LangOsdStr computes at run time.

The string list is CURATED (these are port-owned strings; the wrap sites and this list change
together), but self-checking: every listed string must appear verbatim in the port source, and a
sweep warns about caps-looking quoted strings in the overlay files that are NOT listed -- so a
future overlay addition can't silently go untranslatable.

Usage: ./lang_export_port_ui.py <path to vh/platform/pc/src> <workdir>
"""
import json, os, re, sys


def fnv1a(b):
    h = 14695981039346656037
    for x in b:
        h = ((h ^ x) * 1099511628211) & 0xFFFFFFFFFFFFFFFF
    return h


NOTE_BUTTONS_PS = ("$ @ ^ ~ are BUTTON ICONS (Square/Circle/Triangle/Cross) -- keep them, "
                   "translate the words")
NOTE_BUTTONS_XB = "X/B/Y/A are Xbox button names -- keep them, translate the words"
NOTE_TEMPLATE = "%d and %s are filled by the game -- keep both, in this order"
NOTE_PREFIX = ("the speed digit is appended right after this string -- keep the trailing X "
               "(or your language's multiplication sign convention)")

# (english, source file, category, note-or-None). File is where the WRAP SITE lives (the string the
# hash is computed from), which for the HD status shorts is pc_overlay.c's consumption of
# pc_hdpack.c's return values -- listed under pc_hdpack.c where the literals actually appear.
STRINGS = [
    # MAIN screen: titles + item labels
    ("OPTIONS",                      "pc_overlay.c", "titles", None),
    ("SAVE MANAGEMENT",              "pc_overlay.c", "titles", "also the menu entry opening the saves screen"),
    ("TACTICAL MODE",                "pc_overlay.c", "labels", None),
    ("HD PACK",                      "pc_overlay.c", "labels", None),
    ("INTERNAL RES",                 "pc_overlay.c", "labels", None),
    ("WINDOW SCALE",                 "pc_overlay.c", "labels", None),
    ("FULLSCREEN",                   "pc_overlay.c", "labels", None),
    ("CAMERA X-AXIS",                "pc_overlay.c", "labels", None),
    ("CAMERA Y-AXIS",                "pc_overlay.c", "labels", None),
    ("BUTTON LABELS",                "pc_overlay.c", "labels", None),
    ("LANGUAGE",                     "pc_overlay.c", "labels", None),
    ("RETURN TO TITLE",              "pc_overlay.c", "labels", "also the confirm option that executes it"),
    # value texts
    ("OFF",                          "pc_overlay.c", "values", None),
    ("ON",                           "pc_overlay.c", "values", None),
    ("NORMAL",                       "pc_overlay.c", "values", None),
    ("INVERTED",                     "pc_overlay.c", "values", None),
    ("PLAYSTATION",                  "pc_overlay.c", "values", None),
    ("XBOX",                         "pc_overlay.c", "values", None),
    # HD PACK status shorts (shown in the value column when the pack is unusable)
    ("NO PACK",                      "pc_hdpack.c",  "values", None),
    ("OUTDATED PACK",                "pc_hdpack.c",  "values", None),
    ("WRONG GAME",                   "pc_hdpack.c",  "values", None),
    # CONFIRM screen
    ("RETURN TO TITLE?",             "pc_overlay.c", "confirm", None),
    ("REPLACE CURRENT CARD?",        "pc_overlay.c", "confirm", None),
    ("DELETE THIS BACKUP?",          "pc_overlay.c", "confirm", None),
    ("UNSAVED PROGRESS LOST",        "pc_overlay.c", "confirm", "the stakes warning under RETURN TO TITLE?"),
    ("BACK UP THEN RESTORE",         "pc_overlay.c", "confirm", "the safe default option"),
    ("RESTORE ONLY",                 "pc_overlay.c", "confirm", None),
    ("CANCEL",                       "pc_overlay.c", "confirm", None),
    ("DELETE",                       "pc_overlay.c", "confirm", None),
    # SAVES screen legends (PS + Xbox variants) and list states
    ("$: BACK UP   @: RESTORE",      "pc_gpu_window.c", "legends", NOTE_BUTTONS_PS),
    ("^: DELETE   ~: BACK",          "pc_gpu_window.c", "legends", NOTE_BUTTONS_PS),
    ("X: BACK UP   B: RESTORE",      "pc_gpu_window.c", "legends", NOTE_BUTTONS_XB),
    ("Y: DELETE   A: BACK",          "pc_gpu_window.c", "legends", NOTE_BUTTONS_XB),
    ("START: INSPECT FILE CONTENT",  "pc_gpu_window.c", "legends", "START is the physical button name"),
    ("(NO BACKUPS YET)",             "pc_gpu_window.c", "legends", None),
    # DETAIL screen
    ("SLOT %d   %s",                 "pc_gpu_window.c", "detail", NOTE_TEMPLATE),
    ("EMPTY",                        "pc_gpu_window.c", "detail", "an unused save slot"),
    ("~: BACK",                      "pc_gpu_window.c", "detail", NOTE_BUTTONS_PS),
    ("A: BACK",                      "pc_gpu_window.c", "detail", NOTE_BUTTONS_XB),
    # in-game OSD tag
    ("BATTLE SPEED X",               "pc_gpu_window.c", "osd", NOTE_PREFIX),
]

# Files the self-check reads; also the sweep's search space.
SOURCE_FILES = ["pc_overlay.c", "pc_gpu_window.c", "pc_hdpack.c"]

# Sweep: caps-looking quoted strings that are legitimately NOT translatable.
SWEEP_OK = re.compile(
    r"^(VH_[A-Z0-9_]*|X\d|[A-Z]|%s|%d|BASLUS.*|SLUS.*|HDI\d|SDL_.*|GL_.*|\(\*\)|\( %d / %d \))$")

COMMENTS = re.compile(r"/\*.*?\*/|//[^\n]*", re.S)


def export(srcdir, workdir):
    src = {}
    for fn in SOURCE_FILES:
        with open(os.path.join(srcdir, fn), encoding="utf-8") as f:
            src[fn] = f.read()

    # self-check 1: every listed string appears verbatim in its file
    missing = [(s, fn) for s, fn, _, _ in STRINGS if f'"{s}"' not in src[fn]]
    if missing:
        raise SystemExit("listed string(s) not found in the source -- the wrap site and this list "
                         "must change together:\n  " +
                         "\n  ".join(f'{fn}: "{s}"' for s, fn in missing))

    # self-check 2 (sweep): warn about caps-looking quoted strings not in the list
    listed = {s for s, _, _, _ in STRINGS}
    warned = 0
    for fn in SOURCE_FILES:
        for m in re.finditer(r'"([A-Z][A-Z0-9 :%ds\'!,.\-()/*?~^@$]{2,})"', COMMENTS.sub("", src[fn])):
            s = m.group(1)
            if s in listed or SWEEP_OK.match(s):
                continue
            print(f"  ⚠ {fn}: unlisted caps string {s!r} -- translatable? add it to STRINGS "
                  f"(and a PC_LangOsdStr wrap) or to SWEEP_OK", file=sys.stderr)
            warned += 1

    entries = []
    for s, fn, cat, note in STRINGS:
        item = {"key": f"portui:{fnv1a(s.encode('utf-8')):016x}",
                "en": s, "text": "", "file": fn, "category": cat}
        if note:
            item["note"] = note
        entries.append(item)

    doc = {"source": "the port's own UI (options overlay, save browser, OSD tags)",
           "note": ("rendered with the port's 5x7 caps-only OSD font: the runtime CAPS-FOLDS a "
                    "translation (accents fold to base letters), so write naturally -- mixed case "
                    "and accents are fine. The panel auto-sizes; no hard column budgets."),
           "count": len(entries), "entries": entries}
    p = os.path.join(workdir, "strings", "port_ui.json")
    os.makedirs(os.path.dirname(p), exist_ok=True)
    json.dump(doc, open(p, "w"), indent=1, ensure_ascii=False)

    # Translator view: flat, grouped by category (no option lists here).
    view = {}
    for e in entries:
        v = {"key": e["key"], "en": e["en"], "text": ""}
        if e.get("note"):
            v["note"] = e["note"]
        view.setdefault(e["category"], []).append(v)
    tdoc = {"kind": "port_ui",
            "note": doc["note"]}
    tdoc.update(view)
    q = os.path.join(workdir, "translate", "port_ui.json")
    os.makedirs(os.path.dirname(q), exist_ok=True)
    json.dump(tdoc, open(q, "w"), indent=1, ensure_ascii=False)
    return p, q, doc, warned


if __name__ == "__main__":
    if len(sys.argv) < 3:
        raise SystemExit(__doc__)
    p, q, doc, warned = export(sys.argv[1], sys.argv[2])
    print(f"wrote {p}\nwrote {q}\n  {doc['count']} port-UI strings" +
          (f", {warned} sweep warning(s)" if warned else ", sweep clean"))
