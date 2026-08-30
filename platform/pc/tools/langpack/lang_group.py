#!/usr/bin/env python3
"""lang_group.py -- turn the raw table mirror into an ENTITY-GROUPED translator view.

Why this exists: the game stores an item's text across three unrelated tables (short name, inventory
description, shop description) and a spell's across two, with Tactical Mode overriding some of them.
Handing a translator those tables raw guarantees inconsistency -- they would translate "MstrRobe" in
one file and "Master's robe" in another with no way to see they are the same object.

So there are two views, deliberately:
  strings/    faithful mirror of the disc (what the BUILDER consumes; keeps empty/unused slots)
  translate/  grouped by entity, only what a human should edit (what the TRANSLATOR opens)

Grouping is applied wherever the game has an entity: items, spells, characters. Menu strings have no
entity, so they stay a flat list.

Usage: ./lang_group.py <workdir>          (expects strings/tables.json [+ strings/tactical.json])
"""
import os, sys
from lang_io import load_json, write_json

HARD = lambda n: {"max_chars": n, "hard": True,
                  "note": f"fixed record -- text past {n} characters is LOST"}
def WRAP(cols, rows, box):
    """Wrapping field. DrawText_Internal has NO row limit: it advances rowY forever, so text that
    wraps past the window's row capacity is drawn OUTSIDE the box, over the scene behind it. So the
    real constraint is cols x rows, and exceeding rows is a HARD failure (visual corruption)."""
    return {"max_cols": cols, "max_rows": rows, "hard": False,
            "box": box,
            "note": (f"wraps at {cols} columns; the box fits {rows} row(s) "
                     f"({box}) -- a {rows+1}th row is drawn OUTSIDE the window, over the scene")}

# entity -> (table, field name, limit).  Order defines what the translator sees first.
GROUPS = {
    # 139, not 101: gItemNames runs past the shop/field table with enemy-only equipment ("Thief
    # Sword", "Pirate Suit") that has no shop entry, hence no SJIS name and no description.
    "items": {"count": 139, "fields": [
        ("name",      "gItemNamesSjis",     HARD(8)),
        ("equip_name","gItemNames",         HARD(12)),
        ("desc",      "gItemDescriptions",  WRAP(35, 1, "288x36 battle/inventory bar, text at y=10, 17px rows")),
        ("shop_desc", "gItemDescriptions2", WRAP(29, 4, "312x90 shop/transfer window, text at y=20, 17px rows"))]},
    "spells": {"count": 72, "fields": [
        ("name", "gSpellNames",        HARD(20)),
        ("desc", "gSpellDescriptions", WRAP(35, 1, "288x36 spell bar, text at y=10, 17px rows"))]},
    "characters": {"count": 35, "fields": [
        ("name", "gCharacterNames", HARD(6))]},
}

def load(work):
    tables = load_json(os.path.join(work, "strings", "tables.json"))["tables"]
    tac = {}
    p = os.path.join(work, "strings", "tactical.json")
    if os.path.exists(p):
        for e in load_json(p)["entries"]:
            tac[e["key"]] = e                       # keep the whole entry: en AND any existing text
    return tables, tac

def group(work):
    tables, tac = load(work)
    out = os.path.join(work, "translate"); os.makedirs(out, exist_ok=True)
    report = []
    for kind, spec in GROUPS.items():
        entries, skipped = [], 0
        for i in range(spec["count"]):
            fields, tactical = {}, {}
            for fname, table, limit in spec["fields"]:
                ents = tables[table]["entries"]
                en = (ents[i].get("en") or "") if i < len(ents) else ""   # tables differ in length
                txt = (ents[i].get("text") or "") if i < len(ents) else ""
                fields[fname] = {"en": en, "text": txt, "limit": limit}    # carry existing translation
                t = tac.get(f"{table}[{i}]")
                if t is not None:
                    tactical[fname] = {"en": t.get("en") or "", "text": t.get("text") or "",
                                       "note": "shown INSTEAD of the retail text in Tactical Mode"}
            if not any(f["en"].strip() for f in fields.values()):
                skipped += 1
                continue                        # unused slot: kept in strings/, hidden from the view
            e = {"id": i, "fields": fields}
            if kind == "items":
                e["shop_desc_same_as_desc"] = fields["desc"]["en"] == fields["shop_desc"]["en"]
                # The same item carries two English names -- shop/field and equip/status -- and
                # retail does not always keep them equal ("Nova P." vs "N. Prism"). Flagged so a
                # translator sees it instead of leaving one window in English.
                if fields["name"]["en"] and fields["equip_name"]["en"]:
                    e["name_forms_differ"] = fields["name"]["en"] != fields["equip_name"]["en"]
            if tactical:
                e["tactical"] = tactical
            entries.append(e)
        doc = {"kind": kind,
               "note": ("one object per entry: translate its fields together so the short name and "
                        "the full name stay consistent"),
               "hidden_unused_slots": skipped,
               "count": len(entries), "entries": entries}
        write_json(doc, os.path.join(out, f"{kind}.json"))
        report.append((kind, len(entries), skipped,
                       sum(1 for e in entries if "tactical" in e)))
    # menus: no entity to group by -- flat list, unused slots dropped
    men = [{"key": e["key"], "en": e["en"], "text": e.get("text") or "",
            "limit": WRAP(20, 2, "world-map/menu panels, 17px rows")}
           for e in tables["gStringTable"]["entries"] if (e.get("en") or "").strip()]
    write_json({"kind": "menus", "note": "menu / world-map strings; no entity grouping applies",
               "count": len(men), "entries": men}, os.path.join(out, "menus.json"))
    report.append(("menus", len(men), 0, 0))

    # classes: grouped by name, not index. gClassAdvancementNames is a strict subset of
    # gUnitTypeNames (every one of its 17 names appears there) at different indices, so one edit
    # here keeps the status panel and dojo consistent; the budget is the tighter of the two records.
    seen = {}
    for table, width in (("gUnitTypeNames", 10), ("gClassAdvancementNames", 16)):
        for e in tables[table]["entries"]:
            en = (e.get("en") or "").strip()
            if not en:
                continue
            slot = seen.setdefault(en, {"en": en, "text": "", "appears_in": [],
                                        "limit": HARD(width)})
            slot["appears_in"].append(e["key"])
            if not slot["text"] and (e.get("text") or ""):     # carry existing translation (any of the
                slot["text"] = e["text"]                       # fanned-out entries -- they agree post-merge)
            slot["limit"] = HARD(min(slot["limit"]["max_chars"], width))
    cls = sorted(seen.values(), key=lambda s: s["appears_in"][0])
    write_json({"kind": "classes",
               "note": ("class names; the same name is stored in two tables (status panel + dojo) -- "
                        "translate it once here and both stay consistent"),
               "count": len(cls), "entries": cls}, os.path.join(out, "classes.json"))
    report.append(("classes", len(cls), 0, 0))

    # terrain: its own file. No entity to pair it with, and its constraint is unlike anything else --
    # so folding it into menus.json would attach the wrong limit.
    ter = [{"key": e["key"], "en": e["en"], "text": e.get("text") or "",
            "limit": {"max_chars": 11, "hard": True,
                      "note": ("fixed record -- text past 11 characters is LOST. "
                               "⚠ ALIGNMENT IS PART OF THE STRING: the spaces before the percentage "
                               "are what line the numbers up in the box. Keep the total at 11 and "
                               "pad by hand; do not let an editor trim trailing spaces.")}}
           for e in tables["terrainText"]["entries"]]
    write_json({"kind": "terrain",
               "note": ("battle terrain info box, bottom-left of the screen, one line. The percentage "
                        "is the tile's evasion bonus and is TEXT, not computed -- it will not follow "
                        "a rule change on its own."),
               "count": len(ter), "entries": ter}, os.path.join(out, "terrain.json"))
    report.append(("terrain", len(ter), 0, 0))
    return report

if __name__ == "__main__":
    if len(sys.argv) < 2: raise SystemExit(__doc__)
    print(f"{'file':16}{'entries':>9}{'hidden':>8}{'w/ tactical':>13}")
    for k, n, s, t in group(sys.argv[1]):
        print(f"  translate/{k+'.json':16}{n:>7}{s:>8}{t:>13}")
