#!/usr/bin/env python3
"""Regenerate platform/pc/src/pc_evt_entities.c -- the reconstructed gEvtEntities[95] event-entity
property tables (+ the unnamed anim-set arrays they reference).

WHY: the PC data-gen (build_data_segment.py) flags gEvtEntities as a pointer-containing global and
leaves it NULL ("needs manual review"); nothing populates it at runtime. Result: LoadEvent() gives
every cutscene event entity a NULL animSet -> animData=NULL -> gfxIdx=0 -> INVISIBLE cutscene units
(Magnus, knights, altar...). This script extracts the real gEvtEntities from the byte-exact
SLUS_004.47 and emits a C definition with all anim-set pointers relocated to the PC symbols.

Run from the repo root (vh/): python3 platform/pc/tools/gen_evt_entities.py
Needs: build/SLUS_004.47 (byte-exact matching build) + symbol_addrs.txt.

See memory milestone_cutscene_units_fixed for the full story (found via BizHawk probe exchange/50).
"""
import struct, re, sys, os

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))  # vh/ (tools is 4 deep)
# Region parameterisation (exchange/102 P1): the game tree provides the symbol map and the
# byte-exact executable; symbol NAMES are shared between the trees (JP kept US naming).
GAME_ROOT = os.environ.get("VH_GAME_ROOT", ROOT)
REGION = os.environ.get("VH_REGION", "us")
BASENAME = {"us": "SLUS_004.47", "jp": "SLPM_860.07"}[REGION]
EXE  = os.environ.get("VH_PSX_EXE", os.path.join(GAME_ROOT, "build", BASENAME))
SYMS = os.path.join(GAME_ROOT, "symbol_addrs.txt")
OUT  = os.environ.get("VH_GENERATED_OUT", os.path.join(ROOT, "platform/pc/src/pc_evt_entities.c"))

d = open(EXE, "rb").read()
t = struct.unpack_from("<I", d, 0x18)[0]          # PS-X EXE load addr (0x80010000)
def off(a): return (a - t) + 0x800                # file offset for a PSX address
def u32(a):
    o = off(a); return struct.unpack_from("<I", d, o)[0] if 0 <= o <= len(d)-4 else 0

def load_syms(path):
    addrs, sizes = {}, {}
    for line in open(path):
        m = re.match(r'(\w+)\s*=\s*(0x[0-9a-fA-F]+);(?:\s*//\s*size:(0x[0-9a-fA-F]+))?', line)
        if m:
            addrs[m.group(1)] = int(m.group(2), 16)
            sizes[m.group(1)] = int(m.group(3), 16) if m.group(3) else 0
    return addrs, sizes

addrs, sizes = load_syms(SYMS)

# The JP map carries no `// size:` annotations, and gap-to-next-symbol is unreliable here
# (labels are sparse: gaps overshoot the true gAnimSet_800f2db4 size 0xd0 as 0x24c0).
# The anim tables are structurally shared between the regions (same names, same event
# system), so missing sizes fall back to the US map's annotation for the same name --
# clamped by the region's own gap-to-next, which is a hard upper bound.
if any(sizes.get(k, 0) == 0 for k in addrs if k.startswith('gAnimSet_')) or not sizes.get('gUnitAnimSets'):
    us_addrs, us_sizes = load_syms(os.path.join(ROOT, 'symbol_addrs.txt'))
    all_sorted = sorted(addrs.values())
    import bisect
    def gap(a):
        i = bisect.bisect_right(all_sorted, a)
        return (all_sorted[i] - a) if i < len(all_sorted) else 0x10000
    for k in list(addrs):
        if sizes.get(k, 0) == 0 and us_sizes.get(k, 0):
            sizes[k] = min(us_sizes[k], gap(addrs[k]))

gEE   = addrs["gEvtEntities"]                       # 0x8010308c, 95 ptrs
gua   = addrs["gUnitAnimSets"]; guaSz = sizes["gUnitAnimSets"]
anim  = sorted((addrs[k], k, sizes.get(k, 4)) for k in addrs if k.startswith("gAnimSet_"))
guaval = {u32(gua + n*4): n for n in range(guaSz // 4)}   # gUnitAnimSets[N] value -> N (may repeat)
INEXE  = lambda p: t <= p < t + len(d) - 0x800
# The unnamed anim-set/byte region, per region. Both windows span the same content:
# just below gStringTable up to just below gAnimSet_80104034 (verified by mapping the US
# bounds through the named symbols the two maps share -- exchange/102 P1).
_ANIM_WIN = {"us": (0x80101000, 0x80104000), "jp": (0x80103504, 0x80105fa4)}[REGION]
ANIM_REGION = lambda p: INEXE(p) and _ANIM_WIN[0] <= p < _ANIM_WIN[1]

used_syms = set()
unnamed_arrs = {}                                   # array addr -> [entry addrs]
def read_unnamed(A):
    if A in unnamed_arrs: return
    ents, k = [], 0
    # Cap was 32 -- which TRUNCATED real arrays: e.g. sArr_80102538 is indexed at 32 by a
    # Grog-house prop (animIdx=32), so a 32-entry array read index 32 OUT OF BOUNDS -> wrong
    # sprite (bugreport-01.md). The real end of each array is the non-anim entry (below); the
    # cap is only a runaway guard. 256 covers any real animIdx with margin; over-reading into a
    # contiguous following array is harmless (extra valid anim pointers no entity indexes).
    while k < 256:
        e = u32(A + k*4)
        if e != 0 and not ANIM_REGION(e): break     # non-anim entry => end of array
        ents.append(e); k += 1
    unnamed_arrs[A] = ents

def classify(v):
    """PSX pointer value -> C expression (relocated to a PC symbol / reconstructed array)."""
    if v == 0: return "NULL"
    for a, name, sz in anim:
        if v == a: used_syms.add(name); return name
        if a < v < a+sz and (v-a) % 4 == 0: used_syms.add(name); return f"(u8**)&{name}[{(v-a)//4}]"
    if gua <= v < gua+guaSz and (v-gua) % 4 == 0: return f"(u8**)&gUnitAnimSets[{(v-gua)//4}]"
    if v in guaval: return f"gUnitAnimSets[{guaval[v]}]"
    if ANIM_REGION(v): read_unnamed(v); return f"(u8**)sArr_{v:08x}"
    return "NULL"                                   # unnamed-but-out-of-region / garbage -> NULL

def is_valid_base(b):
    return (b == 0 or ANIM_REGION(b) or b in guaval or gua <= b < gua+guaSz
            or any(a <= b < a+sz for a, _, sz in anim))

ptrs = [u32(gEE + i*4) for i in range(95)]
events = {}
for num, ap in enumerate(ptrs):
    if not ap: continue
    ents, i = [], 0
    while i < 40:
        o = ap + i*12; alt = u32(o); base = u32(o+4)
        if base != 0 and not is_valid_base(base): break   # genuine garbage base => end of array
        ents.append((classify(alt), classify(base), d[off(o+8)], d[off(o+9)])); i += 1
    if ents: events[num] = ents

# self-contained blob for the anim bytes the unnamed arrays point at
allent = [e for es in unnamed_arrs.values() for e in es if e != 0]
blo, bhi = min(allent), max(allent) + 256
blob = d[off(blo):off(bhi)]
def blobref(e): return "NULL" if e == 0 else f"(u8*)(sEvtAltBlob+{e-blo})"

L = ["/* platform/pc/src/pc_evt_entities.c -- GENERATED by platform/pc/tools/gen_evt_entities.py.",
     " * Reconstructed gEvtEntities[95] (event-entity property tables) + the unnamed anim-set arrays",
     " * they reference. PC data-gen left gEvtEntities NULL -> event entities got NULL animSet ->",
     f" * gfxIdx=0 -> INVISIBLE cutscene units. Extracted from byte-exact {os.path.basename(EXE)}; altAnims/baseAnims",
     " * relocated to PC symbols (gAnimSet_x, gUnitAnimSets) or reconstructed unnamed arrays (sArr_x,",
     " * entries -> sEvtAltBlob anim bytes). DO NOT hand-edit; rerun the generator. Stage-2 PC data",
     " * only; not in the matching build. (NOTE: never put a slash-star-slash sequence in this comment.) */",
     '#include "common.h"', "",
     "typedef struct EvtEntityProperties { u8 **altAnims; u8 **baseAnims; u8 stripIdxA, stripIdxB; } EvtEntityProperties;",
     "", "extern u8 *gUnitAnimSets[];"]
for s in sorted(used_syms): L.append(f"extern u8 *{s}[];")
L.append("")
L.append(f"static const unsigned char sEvtAltBlob[{len(blob)}] = {{")
for i in range(0, len(blob), 24): L.append("  " + ",".join(str(b) for b in blob[i:i+24]) + ",")
L.append("};\n")
for A in sorted(unnamed_arrs):
    L.append(f"static u8 *sArr_{A:08x}[] = {{ " + ", ".join(blobref(e) for e in unnamed_arrs[A]) + " };")
L.append("")
for num in sorted(events):
    L.append(f"static EvtEntityProperties sEvt_{num}[] = {{")
    for alt, base, sA, sB in events[num]:
        L.append(f"  {{ (u8**)({alt}), (u8**)({base}), {sA}, {sB} }},")
    L.append("};")
L.append("\nEvtEntityProperties *gEvtEntities[95] = {")
row = []
for i in range(95):
    row.append(f"sEvt_{i}" if i in events else "NULL")
    if len(row) == 8: L.append("  " + ", ".join(row) + ","); row = []
if row: L.append("  " + ", ".join(row) + ",")
L.append("};")
open(OUT, "w").write("\n".join(L) + "\n")
print(f"wrote {OUT}: {len(events)} events, {len(unnamed_arrs)} unnamed arrays, blob {len(blob)}B")
