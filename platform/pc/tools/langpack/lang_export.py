#!/usr/bin/env python3
"""lang_export.py -- build the translator working set from a retail disc (step 1: static tables).

Implements section A of the committed contract in exchange/79: emit `strings/tables.json` where every
entry carries a stable key, the English source, an empty translation slot, and ITS OWN LIMIT, so a
translator never has to know about glyph indices, Shift-JIS, or the engine.

Two encodings appear in these tables and both are handled here:
  * plain 1-byte text (most tables) -- ASCII in the retail US build;
  * 2-byte Shift-JIS (`gItemNamesSjis`) -- decoded via cp932 and NFKC-normalised, so the translator
    sees "HERB", not full-width "ＨＥＲＢ".

Markup is PRESERVED verbatim and counted, never "cleaned":
  * `#N`  splices gStringTable entry N into the line (`##` is a literal '#')
  * `$X`  parser control code (W/F/P/S...), costs no screen columns

Usage: ./lang_export.py <disc.bin> <outdir>
"""
import json, os, re, struct, sys, unicodedata

SECTOR, DOFF, EXE_LBA, EXE_SIZE = 2352, 24, 23, 1996800
LOAD, HDR = 0x80010000, 0x800

# name, vram, kind, count, record width (fixed tables) / None (pointer tables), limit note
# ⭐ The three StringToGlyphs tables (gUnitTypeNames / gItemNames / gClassAdvancementNames) were MISSED
# by the first pass and found by the step-1 probe run (exchange/80): they are drawn through a THIRD
# mechanism -- StringToGlyphs into a sprite glyph strip -- not DrawText and not DrawSjisText, so a
# sweep of DrawText call sites alone never saw them. gItemNames is a SECOND, longer item-name table
# (139 entries vs gItemNamesSjis's 101): the equip/status panel uses it, the shop and field use the
# Shift-JIS one. All three are plain ASCII, NUL-padded.
TABLES = [
    ("gCharacterNames",    0x800eaf58, "fixed",   35,  7, "character name"),
    ("gUnitTypeNames",     0x800eb050, "fixed",   86, 11, "class name (status panel)"),
    ("gItemNames",         0x800eb404, "fixed",  139, 13, "item name, equip/status panel"),
    ("gClassAdvancementNames", 0x801f6a34, "fixed", 18, 17, "class name in the dojo"),
    # A FUNCTION-STATIC in battle_0201b8.c (Objf030_FieldInfo) -- no external linkage, so unlike every
    # other table the runtime cannot reach it without a PC_FEAT hook in that file. Exported here all
    # the same: it is real on-screen text. ⚠️ Its column alignment is literal spaces inside the string
    # ("Plains   0%" vs "Thicket 15%") so the % figures line up -- never trim or normalise it.
    ("terrainText",        0x800f29f4, "fixed",   10, 12, "battle terrain info box (bottom-left)"),
    ("gItemNamesSjis",     0x800eed20, "fixed",  101, 17, "item name, shop/field (2-byte SJIS)"),
    ("gSpellNames",        0x800ee410, "fixed",   72, 21, "spell name"),
    ("gStringTable",       0x8010102c, "ptr",    100, None, "menu / world-map / inserted string"),
    ("gSpellDescriptions", 0x800ee9f8, "ptr",     72, None, "spell description"),
    ("gItemDescriptions",  0x800ef3d8, "ptr",    101, None, "item description (line 1)"),
    ("gItemDescriptions2", 0x800ef56c, "ptr",    101, None, "item description (line 2)"),
]

def read_exe(disc):
    out = bytearray()
    with open(disc, "rb") as f:
        i = 0
        while len(out) < EXE_SIZE:
            f.seek((EXE_LBA + i) * SECTOR + DOFF); out += f.read(2048); i += 1
    exe = bytes(out[:EXE_SIZE])
    if exe[:8] != b"PS-X EXE":
        raise SystemExit(f"{disc}: not a Vandal Hearts (USA) image")
    return exe

def foff(v): return v - LOAD + HDR

def decode(raw):
    """Game bytes -> readable text. Returns (text, encoding_used).

    Deliberately NOT NFKC-normalised here: normalisation folds the full-width space (0x8140) into an
    ASCII space, and that distinction is load-bearing -- 0x8140 is record FILLER, an ASCII space is
    CONTENT. Callers strip filler first, then normalise."""
    if not raw:
        return "", "empty"
    if any(0x81 <= b <= 0x9f or 0xe0 <= b <= 0xfc for b in raw):
        try:
            return raw.decode("cp932"), "sjis"
        except UnicodeDecodeError:
            pass
    return raw.decode("latin1"), "ascii"


def strip_filler(txt):
    """Trailing NUL or full-width space is padding; a trailing ASCII space is content
    (e.g. gItemNames[80] == "Logo P. ")."""
    return txt.rstrip("　\x00")

def markup(text):
    return {"string_refs": len(re.findall(r"#\d+", text)),
            "control_codes": len(re.findall(r"\$.", text))}

def export(disc, outdir):
    exe = read_exe(disc)
    os.makedirs(os.path.join(outdir, "strings"), exist_ok=True)
    doc = {"language": "", "source_note": "extracted from a retail Vandal Hearts (USA) disc",
           "encoding_note": "text is written normally; the build compiles it to the engine's codes",
           "markup": {"#N": "inserts gStringTable entry N ('##' = literal #)",
                      "$X": "engine control code -- keep as-is, costs no columns"},
           "tables": {}}
    stats = []
    for name, vram, kind, count, width, note in TABLES:
        entries, encs, longest = [], set(), 0
        if kind == "fixed":
            for i in range(count):
                o = foff(vram) + i * width
                raw = exe[o:o + width].split(b"\x00")[0]
                txt, enc = decode(raw)
                # Records are PADDED, not merely NUL-terminated, with two fillers in play: NUL, and
                # the full-width space 0x8140 (all of gItemNamesSjis, plus the "empty" slot 0 of
                # gSpellNames). Strip those; keep ASCII spaces, which are content.
                txt = strip_filler(txt)
                if enc == "sjis":
                    txt = unicodedata.normalize("NFKC", txt)   # full-width \uff28\uff45\uff52\uff42 -> "Herb"
                encs.add(enc); longest = max(longest, len(txt))
                entries.append({"key": f"{name}[{i}]", "en": txt, "text": "",
                                "max_bytes": width - 1, **markup(txt)})
            two = (name == "gItemNamesSjis")
            limit = {"kind": "fixed record", "max_bytes": width - 1,
                     "bytes_per_char": 2 if two else 1,
                     "max_chars": (width - 1) // (2 if two else 1),
                     # Verified against the disc by lang_build's round-trip check: only the SJIS
                     # table pads with 0x8140. Every other table pads with NUL (their slot 0 is an
                     # all-0x8140 "empty" record, which is filler, not a padding rule).
                     "padding": "full-width space (0x8140)" if two else "NUL",
                     "note": f"{note}; hard truncation at {(width-1)//(2 if two else 1)} characters"}
        else:
            o = foff(vram)
            ptrs = struct.unpack(f"<{count}I", exe[o:o + count * 4])
            for i, p in enumerate(ptrs):
                if not (LOAD < p < LOAD + 0x200000):
                    entries.append({"key": f"{name}[{i}]", "en": None, "text": None,
                                    "unused": True}); continue
                s = foff(p); e = exe.find(b"\x00", s, s + 512)
                raw = exe[s:e if e != -1 else s + 512]
                txt, enc = decode(raw)
                if enc == "sjis":
                    txt = unicodedata.normalize("NFKC", txt)
                encs.add(enc); longest = max(longest, len(txt))
                entries.append({"key": f"{name}[{i}]", "en": txt, "text": "", **markup(txt)})
            # Column budgets are the 3rd argument of DrawText/DrawText_Internal at each call site,
            # harvested from src/. DrawText_Internal WRAPS at the budget (column resets, row++), it
            # does NOT clip -- unlike the message-box path. So exceeding it costs extra ROWS, and the
            # risk is overflowing the window vertically, not losing characters.
            COLS = {"gStringTable":       (20, "20 cols at all 74 call sites"),
                    "gSpellDescriptions": (35, "window.c:2292, a 288x36 bar"),
                    "gItemDescriptions":  (35, "window.c:2105, same bar"),
                    "gItemDescriptions2": (29, "29-35 depending on screen; 29 is the safe minimum")}
            cols, where = COLS.get(name, (None, "not yet traced"))
            limit = {"kind": "wrapping", "max_cols": cols, "wraps": True,
                     "note": f"{note}; wraps at {cols} columns ({where}) -- text is not lost, "
                             f"but each wrap costs a row and can overflow the window"}
        doc["tables"][name] = {"count": count, "limit": limit, "entries": entries}
        used = sum(1 for e in entries if not e.get("unused"))
        stats.append((name, count, used, longest, "+".join(sorted(encs))))
    path = os.path.join(outdir, "strings", "tables.json")
    json.dump(doc, open(path, "w"), indent=1, ensure_ascii=False)
    return path, doc, stats


# ---------------------------------------------------------------------------------------------
# Dialogue: the on-disc text files. Stored BITWISE-INVERTED with CRLF lines (DecodeLineOfText reads
# ~src[0]); LoadText then walks them as <=100 entries, a blank line toggling entry start/end and
# "END" terminating. Budgets differ by RENDER PATH, traced in src/:
#   * SHOP_T  -> DrawText(gTextPointers) in supplies.c (65 sites) -> 30 cols, WRAPS
#   * all others -> the message box -> 26 cols, HARD CLIP (tail silently lost)
import re as _re

TEXT_RX = _re.compile(r"(B_TXT\d+|EVENT\d+|SIBAI[\w]*|EVDEMO\d+|TOWN_T|SHOP_T|SAKABA_T|TENS_T)\.DAT")

# SCOPING RULE (committed 2026-08-06): the US retail disc is the universe. Anything the US build never
# reaches DOES NOT EXIST for translation purposes -- no PAL/JP consideration, no "might be used".
# These six are the only text files still carrying Japanese, and they are exactly the six absent from
# gEvtTextFiles[95] (the event -> text-file map, 0x80102c18). Their only references anywhere in src/
# are the gCdFiles[] disc table entries in cd.c, which merely record every file's LBA -- NO LoadText
# call names them. Unreachable on both counts, so they are dropped rather than handed to a translator.
DEAD_FILES = {"SIBAI5", "SIBAI7", "SIBAIA", "SIBAIE", "SIBAIF", "EVDEMO7"}
def _budget(stem):
    if stem == "SHOP_T":
        return {"max_cols": 30, "wraps": True,
                "note": "drawn by DrawText in supplies.c -- wraps, text is not lost"}
    return {"max_cols": 26, "wraps": False,
            "note": "drawn by the message box -- HARD CLIP past 26 columns, tail is lost"}


# ENTRY-LEVEL exception. Budgets are per FILE except here: entry 1 of every battle file is the
# victory/defeat condition panel, and it does NOT go through the message box at all --
# battle_0201b8.c draws it with DrawText at 40 columns (line 340) and 34 columns (line 2784), which
# WRAPS. Charging it the message box's 26-column hard clip flagged 41 lines of Konami's own shipped
# text, and the disc is the oracle: a rule that fails retail is our rule being wrong. Budget is the
# tighter of the two real call sites.
# Lines PROVEN not to render through the message box, keyed by content (line numbers churn, text
# does not). Same discipline as lang_export_literals.py's DEAD list: each needs a PROOF, never a
# hunch -- a merely suspicious line stays in the working set and stays flagged.
NOT_MSGBOX = {
    "Zohar has joined your party.":
        "join messages are drawn in their own full-width box, not the message box -- confirmed in "
        "game 2026-08-06. The phrasing actually shown is SIBAI6[3] 'Zohar joined your party.' "
        "(24 cols); this longer EVENT37 variant is an unused duplicate.",
}


def _entry_budget(stem, n):
    if stem.startswith("B_TXT") and n == 1:
        return {"max_cols": 34, "wraps": True,
                "note": "battle condition panel -- drawn by DrawText (battle_0201b8.c), wraps; "
                        "the narrower of its two call sites (34 and 40 columns)"}
    return None

def _iso(disc):
    f = open(disc, "rb")
    def sec(lba, n=1):
        out = bytearray()
        for i in range(n):
            f.seek((lba + i) * SECTOR + DOFF); out += f.read(2048)
        return bytes(out)
    pvd = sec(16)
    ex = struct.unpack("<I", pvd[158:162])[0]; sz = struct.unpack("<I", pvd[166:170])[0]
    out = {}
    def walk(lba, size, pre):
        buf = sec(lba, (size + 2047)//2048); i = 0
        while i < len(buf):
            L = buf[i]
            if L == 0:
                i = (i//2048 + 1) * 2048
                if i >= len(buf): break
                continue
            r = buf[i:i+L]
            e = struct.unpack("<I", r[2:6])[0]; s = struct.unpack("<I", r[10:14])[0]
            fl = r[25]; nm = r[33:33+r[32]].decode("latin1", "replace"); i += L
            if nm in ("\x00", "\x01"): continue
            if fl & 2: walk(e, s, pre + nm + "/")
            else: out[nm.split(";")[0]] = (e, s)
    walk(ex, sz, "")
    return f, out, sec

def export_dialogue(disc, outdir):
    f, files, sec = _iso(disc)
    d = os.path.join(outdir, "strings", "dialogue"); os.makedirs(d, exist_ok=True)
    tot_files = tot_entries = tot_lines = over = dead = 0
    for nm in sorted(files):
        m = TEXT_RX.fullmatch(nm)
        if not m: continue
        stem = m.group(1)
        if stem in DEAD_FILES:
            dead += 1; continue
        ex, sz = files[nm]
        raw = sec(ex, (sz + 2047)//2048)[:sz]
        text = bytes(~b & 0xFF for b in raw)                      # the whole "obfuscation"
        lines = text.split(b"\r\n")
        budget = _budget(stem)
        entries, cur, inside, n = [], None, False, 0
        for ln in lines:
            if ln.startswith(b"END"): break
            if ln == b"":
                if not inside:
                    inside = True; n += 1; cur = {"key": f"{stem}[{n}]", "en": [], "text": []}
                else:
                    inside = False
                    if cur and cur["en"]: entries.append(cur)
                    cur = None
                continue
            if inside and cur is not None:
                s = ln.decode("latin1")
                cur["en"].append(s); cur["text"].append("")
                tot_lines += 1
                vis = _re.sub(r"\$.", "", s)                       # control codes cost no columns
                if len(vis) > budget["max_cols"]: over += 1
        if cur and cur["en"]: entries.append(cur)
        over -= sum(1 for e in entries for l in e["en"]
                    if len(_re.sub(r"\$.", "", l)) > budget["max_cols"])   # undo the file-budget tally
        for i, e in enumerate(entries, 1):
            eb = _entry_budget(stem, i)
            if eb: e["render"] = eb
            lim = (eb or budget)["max_cols"]
            exempt = [j for j, l in enumerate(e["en"]) if l in NOT_MSGBOX]
            if exempt:
                e["render_exempt"] = exempt
                e["render_exempt_why"] = NOT_MSGBOX[e["en"][exempt[0]]]
            over += sum(1 for j, l in enumerate(e["en"])
                        if j not in exempt and len(_re.sub(r"\$.", "", l)) > lim)
        doc = {"file": stem, "render": budget, "count": len(entries),
               "markup": {"#N": "inserts gStringTable entry N", "$X": "control code"},
               "entries": entries}
        json.dump(doc, open(os.path.join(d, f"{stem}.json"), "w"), indent=1, ensure_ascii=False)
        tot_files += 1; tot_entries += len(entries)
    return tot_files, tot_entries, tot_lines, over, dead


if __name__ == "__main__":
    if len(sys.argv) < 3: raise SystemExit(__doc__)
    path, doc, stats = export(sys.argv[1], sys.argv[2])
    print(f"wrote {path}\n")
    print(f"{'table':22}{'entries':>9}{'used':>7}{'longest':>9}  encoding")
    for n, c, u, l, e in stats:
        print(f"  {n:20}{c:>9}{u:>7}{l:>9}  {e}")
    tot = sum(len(t['entries']) for t in doc['tables'].values())
    used = sum(1 for t in doc['tables'].values() for e in t['entries'] if not e.get('unused'))
    refs = sum(e.get('string_refs', 0) for t in doc['tables'].values() for e in t['entries'])
    ctrl = sum(e.get('control_codes', 0) for t in doc['tables'].values() for e in t['entries'])
    print(f"\n  TOTAL {used} translatable strings ({tot} slots) | #N refs: {refs} | $X codes: {ctrl}")

    nf, ne, nl, over, dead = export_dialogue(sys.argv[1], sys.argv[2])
    print(f"\nwrote {os.path.join(sys.argv[2], 'strings', 'dialogue')}/\n")
    print(f"  {nf} dialogue file(s), {ne} entries, {nl} lines"
          + (f" | {over} line(s) over budget" if over else "")
          + (f" | {dead} skipped as unreachable" if dead else ""))
