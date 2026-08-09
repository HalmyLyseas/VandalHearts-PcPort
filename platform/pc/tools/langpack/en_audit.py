#!/usr/bin/env python3
"""en_audit.py -- mechanical defect scan of the RETAIL ENGLISH text.

Not a spell-checker: it looks for defects the game's own data can PROVE, so every hit can be argued
from evidence rather than taste. Five checks:

  1. an item whose two name forms disagree     (shop/field vs equip/status -- a player sees both)
  2. an item whose two descriptions disagree   (beyond line-break placement)
  3. one description doing duty for several items (often deliberate -- listed, not judged)
  4. leading/trailing whitespace inside a stored record
  5. a lone lowercase initial in an otherwise capitalised table

Runs against the exported working set, so it re-runs for free after any re-export.

Usage: ./en_audit.py <workdir>
"""
import collections, os, re, sys
from lang_io import load_json


def audit(work):
    T = load_json(os.path.join(work, "strings", "tables.json"))["tables"]
    E = lambda t: T[t]["entries"]
    out = {}

    out["name_forms"] = [
        (i, E("gItemNamesSjis")[i]["en"], E("gItemNames")[i]["en"])
        for i in range(101)
        if (E("gItemNamesSjis")[i].get("en") and E("gItemNames")[i].get("en")
            and E("gItemNamesSjis")[i]["en"] != E("gItemNames")[i]["en"])]

    norm = lambda s: re.sub(r"\s+", " ", (s or "").replace("\n", " ")).strip()
    out["descriptions"] = [
        (i, norm(E("gItemDescriptions")[i].get("en")), norm(E("gItemDescriptions2")[i].get("en")))
        for i in range(101)
        if (E("gItemDescriptions")[i].get("en") and E("gItemDescriptions2")[i].get("en")
            and norm(E("gItemDescriptions")[i]["en"]) != norm(E("gItemDescriptions2")[i]["en"]))]

    g = collections.defaultdict(list)
    for i in range(101):
        s = E("gItemDescriptions")[i].get("en")
        if s:
            g[s].append((i, E("gItemNamesSjis")[i].get("en")))
    out["shared_desc"] = sorted(((s, v) for s, v in g.items() if len(v) > 1),
                                key=lambda x: -len(x[1]))

    FIXED = ("gItemNames", "gItemNamesSjis", "gSpellNames", "gCharacterNames",
             "gUnitTypeNames", "gClassAdvancementNames", "terrainText")
    out["whitespace"] = [(e["key"], e["en"]) for t in FIXED for e in E(t)
                         if (e.get("en") or "") and e["en"] != e["en"].strip()
                         and t != "terrainText"]      # terrainText pads by design
    out["lowercase"] = [(e["key"], e["en"]) for t in ("gItemNames", "gItemNamesSjis", "gSpellNames",
                                                      "gUnitTypeNames")
                        for e in E(t) if (e.get("en") or "") and e["en"][0].islower()]
    return out


if __name__ == "__main__":
    if len(sys.argv) < 2:
        raise SystemExit(__doc__)
    r = audit(sys.argv[1])
    print(f"1. name forms disagree ................ {len(r['name_forms'])}")
    for i, a, b in r["name_forms"]:
        print(f"     {i:3}  {a!r:12} vs {b!r}")
    print(f"2. descriptions disagree .............. {len(r['descriptions'])}")
    for i, a, b in r["descriptions"]:
        print(f"     {i:3}  {a!r}\n          {b!r}")
    print(f"3. shared descriptions ................ {len(r['shared_desc'])} groups")
    for s, v in r["shared_desc"]:
        print(f"     x{len(v):<2} {s!r}")
        print(f"          {[n for _, n in v]}")
    print(f"4. stray whitespace in a record ....... {len(r['whitespace'])}")
    for k, s in r["whitespace"]:
        print(f"     {k:26} {s!r}")
    print(f"5. lone lowercase initial ............. {len(r['lowercase'])}")
    for k, s in r["lowercase"]:
        print(f"     {k:26} {s!r}")
