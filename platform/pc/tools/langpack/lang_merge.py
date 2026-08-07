#!/usr/bin/env python3
"""lang_merge.py -- fold the translator's edits (translate/) back into the build layer (strings/).

Closes the gap that made `translate/` a read-only view: `lang_build` consumes `strings/`, and until
this tool a translator editing the grouped files changed nothing. The pipeline is now:

    edit translate/*.json  (+ strings/dialogue/*.json directly)
        -> lang_merge.py <workdir>
        -> lang_validate.py <disc> <workdir>
        -> lang_build.py <disc> <workdir> <outdir>

Direction is ONE-WAY, translate/ -> strings/: the grouped view is the human-facing layer, so where
both carry a (different) translation, translate/ wins and the conflict is REPORTED, never silent.
Dialogue has no grouped view (its files are already entity-shaped) and is untouched here.

Mappings mirror lang_group.py exactly:
  items.json        id -> gItemNamesSjis / gItemNames / gItemDescriptions / gItemDescriptions2
  spells.json       id -> gSpellNames / gSpellDescriptions
  characters.json   id -> gCharacterNames
  classes.json      grouped BY NAME -> every key in its appears_in list (status panel + dojo stay
                    consistent by construction)
  menus.json        key -> gStringTable[i]
  terrain.json      key -> terrainText[i]
  literals.json     key (content hash) -> strings/literals.json; option lists rejoin with \\n
  port_ui.json      key (content hash) -> strings/port_ui.json (grouped by category in translate/)
  tactical sub-objects -> strings/tactical.json (same key namespace)

Usage: ./lang_merge.py <workdir>
"""
import json, os, sys


def load(p):
    return json.load(open(p))


def save(p, d):
    json.dump(d, open(p, "w"), indent=1, ensure_ascii=False)


FIELDS = {
    "items": [("name", "gItemNamesSjis"), ("equip_name", "gItemNames"),
              ("desc", "gItemDescriptions"), ("shop_desc", "gItemDescriptions2")],
    "spells": [("name", "gSpellNames"), ("desc", "gSpellDescriptions")],
    "characters": [("name", "gCharacterNames")],
}


def merge(work):
    tp, sp = os.path.join(work, "translate"), os.path.join(work, "strings")
    tables = load(os.path.join(sp, "tables.json"))
    T = tables["tables"]
    stats, conflicts = {}, []

    def put(table, idx, text, src):
        e = T[table]["entries"][idx]
        old = e.get("text") or ""
        if old and old != text:
            conflicts.append(f"{table}[{idx}]: strings/ had {old!r}, {src} says {text!r}")
        if old != text:
            e["text"] = text
            stats[src] = stats.get(src, 0) + 1

    tac_edits = {}                                  # key -> (text, src)

    for kind, fields in FIELDS.items():
        p = os.path.join(tp, f"{kind}.json")
        if not os.path.exists(p):
            continue
        for e in load(p)["entries"]:
            i = e["id"]
            for fname, table in fields:
                t = (e["fields"].get(fname) or {}).get("text") or ""
                if t:
                    put(table, i, t, f"{kind}.json")
            for fname, sub in (e.get("tactical") or {}).items():
                t = sub.get("text") or ""
                if t:
                    for fn2, table in fields:
                        if fn2 == fname:
                            tac_edits[f"{table}[{i}]"] = (t, f"{kind}.json(tactical)")

    p = os.path.join(tp, "classes.json")
    if os.path.exists(p):
        for e in load(p)["entries"]:
            t = e.get("text") or ""
            if t:
                for key in e["appears_in"]:
                    table, idx = key[:-1].split("[")
                    put(table, int(idx), t, "classes.json")

    for kind in ("menus", "terrain"):
        p = os.path.join(tp, f"{kind}.json")
        if os.path.exists(p):
            for e in load(p)["entries"]:
                t = e.get("text") or ""
                if t:
                    table, idx = e["key"][:-1].split("[")
                    put(table, int(idx), t, f"{kind}.json")

    save(os.path.join(sp, "tables.json"), tables)

    # literals: keyed by content hash; option lists rejoin positionally
    nlit = 0
    lp_t, lp_s = os.path.join(tp, "literals.json"), os.path.join(sp, "literals.json")
    if os.path.exists(lp_t) and os.path.exists(lp_s):
        tdoc, sdoc = load(lp_t), load(lp_s)
        by_key = {e["key"]: e for e in sdoc["entries"]}
        for bucket in ("menus", "messages"):
            for e in tdoc.get(bucket, []):
                if e.get("options"):
                    if not any(o.get("text") for o in e["options"]):
                        continue
                    txt = "\n".join(o.get("text") or o["en"] for o in e["options"])
                else:
                    txt = e.get("text") or ""
                    if not txt:
                        continue
                tgt = by_key.get(e["key"])
                if tgt is not None and (tgt.get("text") or "") != txt:
                    tgt["text"] = txt
                    nlit += 1
        save(lp_s, sdoc)

    # port-UI: keyed by content hash like literals; the translate view is grouped by category,
    # every group is a flat list of {key, en, text} entries
    npui = 0
    pp_t, pp_s = os.path.join(tp, "port_ui.json"), os.path.join(sp, "port_ui.json")
    if os.path.exists(pp_t) and os.path.exists(pp_s):
        tdoc, sdoc = load(pp_t), load(pp_s)
        by_key = {e["key"]: e for e in sdoc["entries"]}
        for cat, lst in tdoc.items():
            if not isinstance(lst, list):
                continue
            for e in lst:
                txt = e.get("text") or ""
                if not txt:
                    continue
                tgt = by_key.get(e["key"])
                if tgt is not None and (tgt.get("text") or "") != txt:
                    tgt["text"] = txt
                    npui += 1
        save(pp_s, sdoc)

    # tactical layer: same table[idx] key namespace as the retail tables
    ntac = 0
    tacp = os.path.join(sp, "tactical.json")
    if tac_edits and os.path.exists(tacp):
        tdoc = load(tacp)
        for e in tdoc["entries"]:
            hit = tac_edits.get(e["key"])
            if hit and (e.get("text") or "") != hit[0]:
                e["text"] = hit[0]
                ntac += 1
        save(tacp, tdoc)

    return stats, conflicts, nlit, ntac, npui


if __name__ == "__main__":
    if len(sys.argv) < 2:
        raise SystemExit(__doc__)
    stats, conflicts, nlit, ntac, npui = merge(sys.argv[1])
    tot = sum(stats.values())
    for src, n in sorted(stats.items()):
        print(f"  {src:24} {n:>4} field(s) merged")
    if nlit:
        print(f"  {'literals.json':24} {nlit:>4} entry(ies) merged")
    if ntac:
        print(f"  {'tactical layer':24} {ntac:>4} entry(ies) merged")
    if npui:
        print(f"  {'port_ui.json':24} {npui:>4} entry(ies) merged")
    if conflicts:
        print(f"\n  {len(conflicts)} conflict(s) -- translate/ won, review these:")
        for c in conflicts[:10]:
            print(f"    {c}")
    if not tot and not nlit and not ntac and not npui:
        print("  nothing to merge (no text fields set in translate/)")
