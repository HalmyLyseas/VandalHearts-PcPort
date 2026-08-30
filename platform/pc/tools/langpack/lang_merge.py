#!/usr/bin/env python3
"""lang_merge.py -- fold the translator's edits (translate/) back into the build layer (strings/).

Closes the gap that made `translate/` a read-only view: `lang_build` consumes `strings/`, and until
this tool a translator editing the grouped files changed nothing. The pipeline is now:

    edit translate/*.json  (+ strings/dialogue/*.json directly)
        -> lang_merge.py <workdir>
        -> lang_validate.py <disc> <workdir>
        -> lang_build.py <disc> <workdir> <outdir>

Direction is ONE-WAY, translate/ -> strings/: the grouped view is the translator-facing layer, so where
both carry a (different) translation, translate/ wins and the conflict is REPORTED, never silent.
Dialogue has no grouped view (its files are already entity-shaped) and is untouched here.

The grouped view ROUND-TRIPS: lang_group carries the existing strings/ translation into translate/,
so re-grouping does not blank a translator's work, and a field the translator CLEARS is a deliberate
revert to English -- reported by default (never silently kept), applied with --revert-cleared.

Mappings mirror lang_group.py exactly:
  items.json        id -> gItemNamesSjis / gItemNames / gItemDescriptions / gItemDescriptions2
  spells.json       id -> gSpellNames / gSpellDescriptions
  characters.json   id -> gCharacterNames
  classes.json      grouped BY NAME -> every key in its appears_in list (status panel + dojo stay
                    consistent by construction)
  menus.json        key -> gStringTable[i]
  terrain.json      key -> terrainText[i]
  literals.json     key (content hash) -> strings/literals.json; option lists rejoin with \\n
  tactical sub-objects -> strings/tactical.json (same key namespace)

Usage: ./lang_merge.py <workdir>
"""
import os, sys
from lang_io import load_json, write_json


def load(p):
    return load_json(p)


def save(p, d):
    write_json(d, p)


FIELDS = {
    "items": [("name", "gItemNamesSjis"), ("equip_name", "gItemNames"),
              ("desc", "gItemDescriptions"), ("shop_desc", "gItemDescriptions2")],
    "spells": [("name", "gSpellNames"), ("desc", "gSpellDescriptions")],
    "characters": [("name", "gCharacterNames")],
}


def merge(work, revert_cleared=False):
    tp, sp = os.path.join(work, "translate"), os.path.join(work, "strings")
    tables = load(os.path.join(sp, "tables.json"))
    T = tables["tables"]
    stats, conflicts, reverts = {}, [], []

    def put(table, idx, text, src):
        # translate/ is the translator's working copy and round-trips (lang_group carries the
        # existing translation into the view); an empty field where strings/ has text is a
        # deliberate clear to English. Report it always; apply it only with --revert-cleared.
        ents = T[table]["entries"]
        if idx >= len(ents):
            return                       # this group row has no slot in this table (they differ in
                                         # length -- gItemNames runs 139, gItemNamesSjis only 101)
        e = ents[idx]
        old = e.get("text") or ""
        if old == text:
            return
        if text == "":
            reverts.append(f"{table}[{idx}]: strings/ has {old!r}, {src} cleared it")
            if revert_cleared:
                e["text"] = ""
                stats[f"{src} (revert)"] = stats.get(f"{src} (revert)", 0) + 1
            return
        if old:
            conflicts.append(f"{table}[{idx}]: strings/ had {old!r}, {src} says {text!r}")
        e["text"] = text
        stats[src] = stats.get(src, 0) + 1

    tac_edits = {}                                  # key -> (text, src); text "" = cleared

    for kind, fields in FIELDS.items():
        p = os.path.join(tp, f"{kind}.json")
        if not os.path.exists(p):
            continue
        for e in load(p)["entries"]:
            i = e["id"]
            for fname, table in fields:
                t = (e["fields"].get(fname) or {}).get("text") or ""
                put(table, i, t, f"{kind}.json")
            for fname, sub in (e.get("tactical") or {}).items():
                t = sub.get("text") or ""
                for fn2, table in fields:
                    if fn2 == fname:
                        tac_edits[f"{table}[{i}]"] = (t, f"{kind}.json(tactical)")

    p = os.path.join(tp, "classes.json")
    if os.path.exists(p):
        for e in load(p)["entries"]:
            t = e.get("text") or ""
            for key in e["appears_in"]:
                table, idx = key[:-1].split("[")
                put(table, int(idx), t, "classes.json")

    for kind in ("menus", "terrain"):
        p = os.path.join(tp, f"{kind}.json")
        if os.path.exists(p):
            for e in load(p)["entries"]:
                t = e.get("text") or ""
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
                    txt = ("\n".join(o.get("text") or o["en"] for o in e["options"])
                           if any(o.get("text") for o in e["options"]) else "")
                else:
                    txt = e.get("text") or ""
                tgt = by_key.get(e["key"])
                if tgt is None:
                    continue
                old = tgt.get("text") or ""
                if old == txt:
                    continue
                if txt == "":
                    reverts.append(f"literal {e['key']}: strings/ has a translation, cleared")
                    if revert_cleared:
                        tgt["text"] = ""; nlit += 1
                    continue
                tgt["text"] = txt; nlit += 1
        save(lp_s, sdoc)

    # tactical layer: same table[idx] key namespace as the retail tables
    ntac = 0
    tacp = os.path.join(sp, "tactical.json")
    if tac_edits and os.path.exists(tacp):
        tdoc = load(tacp)
        for e in tdoc["entries"]:
            hit = tac_edits.get(e["key"])
            if not hit:
                continue
            new, src = hit
            old = e.get("text") or ""
            if old == new:
                continue
            if new == "":
                reverts.append(f"{e['key']} (tactical): strings/ has {old!r}, {src} cleared it")
                if revert_cleared:
                    e["text"] = ""; ntac += 1
                continue
            e["text"] = new; ntac += 1
        save(tacp, tdoc)

    return stats, conflicts, reverts, nlit, ntac


if __name__ == "__main__":
    if len(sys.argv) < 2:
        raise SystemExit(__doc__)
    revert_cleared = "--revert-cleared" in sys.argv
    work = next(a for a in sys.argv[1:] if not a.startswith("-"))
    stats, conflicts, reverts, nlit, ntac = merge(work, revert_cleared)
    tot = sum(stats.values())
    for src, n in sorted(stats.items()):
        print(f"  {src:24} {n:>4} field(s) merged")
    if nlit:
        print(f"  {'literals.json':24} {nlit:>4} entry(ies) merged")
    if ntac:
        print(f"  {'tactical layer':24} {ntac:>4} entry(ies) merged")
    if conflicts:
        print(f"\n  {len(conflicts)} conflict(s) -- translate/ won, review these:")
        for c in conflicts[:10]:
            print(f"    {c}")
    if reverts:
        if revert_cleared:
            print(f"\n  {len(reverts)} field(s) reverted to English (--revert-cleared).")
        else:
            print(f"\n  {len(reverts)} field(s) CLEARED in translate/ but still set in strings/ -- "
                  f"LEFT AS-IS.\n  Re-run with --revert-cleared to remove them (revert to English), "
                  f"or restore the text in translate/:")
            for r in reverts[:10]:
                print(f"    {r}")
            if len(reverts) > 10:
                print(f"    ... and {len(reverts) - 10} more")
    if not tot and not nlit and not ntac and not reverts:
        print("  nothing to merge (no text fields changed in translate/)")
