#!/usr/bin/env python3
"""lang_probe.py -- build a LABELLED probe pack (text import, in-game verification).

One probe per text source, each tagged L1..L8, so a play session answers two questions at once:

  1. does an edited JSON reach the screen?  (the step-1 goal)
  2. WHICH glyph store serves WHICH window?  -- still unmapped. The exe's ASCII->glyph map case-folds
     and reaches only 51 of the font's 110 non-blank cells, so some windows must be drawn from the
     on-disc F_WD.DAT sheet instead. Several probes are deliberately MixedCase: wherever a probe
     renders with its lowercase intact, that window is NOT on the case-folding ASCII path.

Nothing here is authored text -- these are markers, chosen to be impossible to mistake for content.

Usage: ./lang_probe.py <disc.bin> <workdir> <outdir>
       -> <outdir>/langpacks/en-probe/ (manifest name "Probes L1-L17" for the overlay picklist)
"""
import os, shutil, subprocess, sys, tempfile
from lang_io import load_json, write_json

import lang_build

#  tag   table                 index  replacement                        where to look in-game
PROBES = [
    # Path 1b (sheet): strip-path table -- the accent needs BOTH the bitmap and its F_WD sheet cell.
    ("L1", "gCharacterNames",      1, "L1-\u00e0SH",
     "party/status screen -- accented a through the charmap + sheet stamp"),
    # Path 2 (krom): the accent rides a pack 2-byte code -> 16x15 glyph via Krom2RawAdd.
    ("L2", "gItemNamesSjis",       1, "L2H\u00e9rb",
     "inventory/shop item lists -- accented e at 16x15 through the krom extension"),
    # Increment 3 (D2 charmap): accents in a FIXED table via 1-byte pack codes -- e-acute and
    # e-grave cost ONE byte each, so the 20-char budget is untouched.
    ("L3", "gSpellNames",          1, "L3 Fa\u00e9rie Lit\u00e8",
     "the spell menu in battle -- accents through the charmap, 1 byte each"),
    # Accented so the L13 #N-insertion probe has a multi-byte payload to splice.
    ("L4", "gStringTable",        33, "L4 Vallée",
     "world map destination panel; ALSO the payload the L13 dialogue probe splices via #33"),
    ("L5", "gSpellDescriptions",   1, "L5 spell desc MixedCase",
     "the description bar under the spell menu"),
    # UTF-8 increment (D1, exchange/80): the e-acute renders through the pack's font engine.
    ("L6", "gItemDescriptions",    1, "L6 accent: cafe = caf\u00e9",
     "the 288x36 description bar -- the accented e proves the UTF-8 path"),
    ("L7", "gItemDescriptions2",   1, "L7 shop desc MixedCase",
     "the shop / transfer window (312x90)"),
    # Round 2: the StringToGlyphs tables the first probe run uncovered.
    # Deliberately index 13 (L.sword), not Herb: it is EQUIPPED, so it shows on the status panel
    # immediately -- the same line that read "L.SWORD" in the L1 screenshot. Its shop counterpart
    # gItemNamesSjis[13] is left untouched, so the shop still says "L. sword": one item, two tables.
    ("L9",  "gItemNames",            13, "L9 L.sw\u00f6rd",
     "status panel, the EQUIPPED weapon line (shop still shows 'L. sword' -- that is L2's table)"),
    ("L10", "gUnitTypeNames",         1, "L10 Hero",
     "the class name beside a character's name in the status panel"),
    # Retargeted from Champion (index 1) to the two classes a MAGE is actually offered, so the probe
    # can be reached from a save parked in front of Zohar's promotion. Both lines change at once.
    ("L11",  "gClassAdvancementNames", 11, "L11 Sorcerer",
     "the dojo's class-advancement list, first line for a Mage"),
    ("L11b", "gClassAdvancementNames", 13, "L11b Monk",
     "the dojo's class-advancement list, second line for a Mage"),
    # Also carries a charmap accent (i-circumflex) THROUGH the deferred battle hook.
    ("L12", "terrainText",            0, "L12 Pla\u00eens",
     "battle: the terrain info box, bottom-left -- accent via charmap + deferred hook"),
]
# Increment 2 (msgbox UTF-8): the second line carries accents through the message-box parser.
# Final combination round (L13/L14): the two validated-pieces-but-untested PAIRINGS --
#   L13: a #N insertion whose referenced menu string is ACCENTED (parser resume bookkeeping around
#        a multi-byte inserted string; L4's probe text is made accented for this)
#   L14: accented SUBSTITUTED dialogue rendered by the DrawText route (SHOP_T wraps at 30 cols,
#        never sees the message box) -- the shop clerk's greeting
# Increment 5 (PC_LANGSTR literals): replacements matched by the entry's own `literal:<hash>` key,
# so the retail English is not embedded here (it lives only on the player's disc, hashed at export).
LITERALS = [
    ("L15", "literal:ea126cddb07855e5", "L15 R\u00e9sultats",
     "the battle-results banner after a battle ends -- a replaced CODE literal, with an accent"),
    ("L16", "literal:5d75bd4732d1eb30", "L16\u00e9t\nSorts\nObjets",
     "the party menu's skill/spell/items panel -- the most-seen literal in the game"),
]

# Increment 7 (Tactical layer): a tactical flavor string, replaced by content hash -- visible with
# TACTICAL MODE ON, item 88 (Mad Book) highlighted in the items list.
TACTICAL = [("L17", "Casts Spellbind", "L17 Sortil\u00e8ge")]

DIALOGUES = [
    ("L8", "EVENT01", 0, ["L8 DIALOGUE PROBE", "café déjà reçu éèêë"],
     "the opening scene, first message box -- line 2 is the UTF-8 probe"),
    ("L13", "EVENT01", 1, ["L13 menu ref: #33", "", ""],
     "the opening scene, SECOND box -- the #33 insert must render 'L4 Vallée' with its accent"),
    ("L14", "SHOP_T", 0, ["L14 bienvenüe éh.", ""],
     "any shop: the clerk's greeting -- accents through the DrawText dialogue route; line 2 left "
     "untranslated so it renders retail unchanged"),
]


def main(disc, work, outdir):
    tmp = tempfile.mkdtemp(prefix="lang_probe_")
    try:
        stage = os.path.join(tmp, "work")
        shutil.copytree(work, stage)

        p = os.path.join(stage, "strings", "tables.json")
        doc = load_json(p)
        for tag, table, idx, repl, _ in PROBES:
            e = doc["tables"][table]["entries"][idx]
            e["text"] = repl
        write_json(doc, p)

        p = os.path.join(stage, "strings", "tactical.json")
        doc = load_json(p)
        for tag, en, text in TACTICAL:
            for e in doc["entries"]:
                if e["en"] == en:
                    e["text"] = text
        write_json(doc, p)

        p = os.path.join(stage, "strings", "literals.json")
        doc = load_json(p)
        for tag, key, text, _ in LITERALS:
            hits = [e for e in doc["entries"] if e["key"] == key]
            assert len(hits) == 1, (tag, key)
            hits[0]["text"] = text
        write_json(doc, p)

        for tag, stem, ei, lines, _ in DIALOGUES:
            p = os.path.join(stage, "strings", "dialogue", f"{stem}.json")
            doc = load_json(p)
            doc["entries"][ei]["text"] = lines
            write_json(doc, p)

        d, stats, nf, nl, ns, ng = lang_build.build(disc, stage, outdir, "en-probe",
                                                    {"name": "Probes L1-L17", "version": "dev"})
    finally:
        shutil.rmtree(tmp, ignore_errors=True)

    print(f"probe pack: {d}  ({ns} sections)\n")
    print("run it:")
    print(f"  VH_LANGPACK={d} ./build/vandalhearts_pc\n")
    print(f"{'tag':5}{'source':22}{'shows as':26}where to look")
    for tag, table, idx, repl, where in PROBES:
        print(f"  {tag:3}{table+'['+str(idx)+']':22}{repl:26}{where}")
    for tag, stem, ei, lines, where in DIALOGUES:
        print(f"  {tag:4}{stem+f' entry {ei+1}':21}{lines[0]:26}{where}")
    for tag, key, text, where in LITERALS:
        print(f"  {tag:4}{'literal':21}{text.splitlines()[0]:26}{where}")
    for tag, en, text in TACTICAL:
        print(f"  {tag:4}{'tactical':21}{text:26}Tactical ON: Mad Book's description in the items list")
    print("\nA probe that renders with its lowercase INTACT is not on the case-folding ASCII path.")
    return d


if __name__ == "__main__":
    if len(sys.argv) < 4:
        raise SystemExit(__doc__)
    main(sys.argv[1], sys.argv[2], sys.argv[3])
