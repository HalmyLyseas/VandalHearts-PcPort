#!/usr/bin/env python3
"""lang_export_tactical.py -- export the PORT-AUTHORED Tactical Mode text as its own working file.

Kept deliberately separate from `strings/tables.json` (contract amendment 2b):
  * retail content is frozen -- `tables.json` should never need re-diffing;
  * Tactical Mode is ours and evolves, so its text moves independently;
  * a translator who only wants to follow a Tactical change opens one small file.

Tactical text OVERRIDES a retail table entry at runtime when Tactical Mode is on, so entries reuse
the same `table[index]` key namespace -- the builder needs no special routing, just a second layer.

Source of truth is `platform/pc/src/pc_balance.c` itself (the strings are literals at the patch call
sites), so this cannot drift from what the game actually shows.

Usage: ./lang_export_tactical.py <path to pc_balance.c> <outdir>
"""
import json, os, re, sys

# helper( id , "text" )  ->  which table(s) the id addresses
PATTERNS = [
    (re.compile(r'\baddDescSwap\(\s*(\d+)\s*,\s*"((?:[^"\\]|\\.)*)"'),
     ["gItemDescriptions", "gItemDescriptions2"],          # one call patches BOTH tables
     "item description (Tactical flavour text)"),
    (re.compile(r'\baddSpellDescSwap\(\s*(\d+)\s*,\s*"((?:[^"\\]|\\.)*)"'),
     ["gSpellDescriptions"],
     "spell description (Tactical stat line)"),
    (re.compile(r'\baddStrPatch\(\s*gSpellNames\[\s*(\d+)\s*\]\s*,\s*"((?:[^"\\]|\\.)*)"'),
     ["gSpellNames"],
     "spell name (fixed 20-char record)"),
]
# limits mirror strings/tables.json so a translator sees the same constraint in both files
LIMITS = {"gSpellNames": {"kind": "fixed record", "max_chars": 20,
                          "padding": "full-width space (0x8140)"}}

def export(balance_c, outdir):
    src = open(balance_c, encoding="latin1").read()
    entries = []
    for rx, tables, note in PATTERNS:
        for m in rx.finditer(src):
            idx, text = int(m.group(1)), m.group(2).encode().decode("unicode_escape")
            for t in tables:
                e = {"key": f"{t}[{idx}]", "en": text, "text": "",
                     "overrides": "retail", "note": note}
                lim = LIMITS.get(t)
                if lim:
                    e["limit"] = lim
                entries.append(e)
    entries.sort(key=lambda e: (e["key"].split("[")[0], int(e["key"].split("[")[1][:-1])))
    doc = {"layer": "tactical",
           "note": ("port-authored text for Tactical Mode; replaces the retail entry with the same "
                    "key while Tactical is enabled. Retail text lives in strings/tables.json and is "
                    "unaffected."),
           "source": os.path.basename(balance_c),
           "entries": entries}
    os.makedirs(os.path.join(outdir, "strings"), exist_ok=True)
    path = os.path.join(outdir, "strings", "tactical.json")
    json.dump(doc, open(path, "w"), indent=1, ensure_ascii=False)
    return path, doc

if __name__ == "__main__":
    if len(sys.argv) < 3: raise SystemExit(__doc__)
    path, doc = export(sys.argv[1], sys.argv[2])
    by = {}
    for e in doc["entries"]:
        by.setdefault(e["key"].split("[")[0], []).append(e)
    print(f"wrote {path}\n")
    print(f"{'target table':24}{'entries':>9}{'longest':>9}")
    for t, es in sorted(by.items()):
        print(f"  {t:22}{len(es):>9}{max(len(e['en']) for e in es):>9}")
    print(f"\n  TOTAL {len(doc['entries'])} Tactical strings "
          f"({len({e['en'] for e in doc['entries']})} distinct texts)")
