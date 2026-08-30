#!/usr/bin/env python3
"""Regenerate platform/pc/src/pc_unit_anim_data.c -- the reconstructed gUnitAnimSets[489]
unit-animation table (blob + per-unit anim-pointer arrays).

WHY: the PC data-gen (build_data_segment.py) flags gUnitAnimSets as a pointer-containing global
and leaves it NULL -- raw pointer VALUES from the original binary are MIPS virtual addresses,
meaningless in our own process. The real definition is a THREE-level structure that must be
relocated to local symbols:

  gUnitAnimSets[144 in hardware, 489 on PC -- see include/graphics.h for the widening rationale]
    : u8** per unit (143 of 144 non-null; many units share one deduplicated sub-array)
    -> u8*[24|26|30]  : one pointer per AnimIdx state; a couple of slots are genuinely NULL
      -> u8[]         : self-terminating animation byte strips (frame/delay pairs, parsed by
                        src/core/object.c UpdateUnitSpriteAnimation)

Every level is resolved directly against the region's byte-exact executable, with NO hardcoded
region window: the 144 pointers are read from gUnitAnimSets' own address, the unique level-2
sub-array addresses are packed contiguously right below gUnitAnimSets (so consecutive unique
addresses ARE the element-count boundaries; the last array ends exactly AT gUnitAnimSets), and
the level-3 strips live in one contiguous blob right below the level-2 region. The blob is
anchored at min(level-3 address) rounded down to 8 -- for the US binary that reproduces the
original hand-extracted anchor 0x800e45c8 (the SLUS_004.47.yaml subsegment boundary), keeping
the output byte-comparable with the previous hand-written file; the <=7 prefix bytes are
unreferenced padding either way.

Run from the repo root (vh/): python3 platform/pc/tools/gen_unit_anim_data.py
Region selection (same conventions as gen_evt_entities.py):
  VH_REGION=us|jp (default us), VH_GAME_ROOT=<game tree root> (vh/ or vh/jp/),
  VH_PSX_EXE=<exe override>, VH_GENERATED_OUT=<output override>.
Needs: <GAME_ROOT>/build/<SLUS_004.47|SLPM_860.07> (byte-exact) + <GAME_ROOT>/symbol_addrs.txt.
"""
import struct, re, os

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))  # vh/ (tools is 4 deep)
GAME_ROOT = os.environ.get("VH_GAME_ROOT", ROOT)
REGION = os.environ.get("VH_REGION", "us")
BASENAME = {"us": "SLUS_004.47", "jp": "SLPM_860.07"}[REGION]
EXE  = os.environ.get("VH_PSX_EXE", os.path.join(GAME_ROOT, "build", BASENAME))
SYMS = os.path.join(GAME_ROOT, "symbol_addrs.txt")
OUT  = os.environ.get("VH_GENERATED_OUT", os.path.join(ROOT, "platform/pc/src/pc_unit_anim_data.c"))

d = open(EXE, "rb").read()
t = struct.unpack_from("<I", d, 0x18)[0]          # PS-X EXE load addr (0x80010000)
def off(a): return (a - t) + 0x800                # file offset for a PSX address
def u32(a): return struct.unpack_from("<I", d, off(a))[0]

def load_syms(path):
    addrs, sizes = {}, {}
    for line in open(path):
        m = re.match(r'(\w+)\s*=\s*(0x[0-9a-fA-F]+);(?:\s*//\s*size:(0x[0-9a-fA-F]+))?', line)
        if m:
            addrs[m.group(1)] = int(m.group(2), 16)
            sizes[m.group(1)] = int(m.group(3), 16) if m.group(3) else 0
    return addrs, sizes

addrs, sizes = load_syms(SYMS)
gua = addrs["gUnitAnimSets"]
guaSz = sizes.get("gUnitAnimSets", 0)
if not guaSz:
    # The JP map carries no `// size:` annotations; the table is structurally shared between the
    # regions (same symbol names), so fall back to the US map's annotation for the same name --
    # clamped by this region's own gap-to-next-symbol, which is a hard upper bound.
    us_addrs, us_sizes = load_syms(os.path.join(ROOT, "symbol_addrs.txt"))
    gap = min((a for a in addrs.values() if a > gua), default=gua + 0x10000) - gua
    guaSz = min(us_sizes["gUnitAnimSets"], gap)
N = guaSz // 4

# Level 1: the per-unit set pointers.
ptrs = [u32(gua + i * 4) for i in range(N)]
uniq = sorted(set(p for p in ptrs if p))          # unique level-2 array addresses, ascending
assert uniq, "no non-null gUnitAnimSets entries?"
assert all(p % 4 == 0 for p in uniq), "misaligned level-2 pointer"
assert uniq[-1] < gua, "level-2 array not below gUnitAnimSets"

# Level 2: element counts from the gap to the next unique address (packed contiguously in the
# original binary); the last array is bounded by gUnitAnimSets itself. Read every entry.
bounds = uniq + [gua]
setEnts = {}                                      # l2 addr -> [l3 addrs (0 = real NULL slot)]
for a, b in zip(bounds, bounds[1:]):
    assert (b - a) % 4 == 0 and b > a
    setEnts[a] = [u32(a + k * 4) for k in range((b - a) // 4)]

# Level 3: the strip blob. All non-null entries must land in one contiguous window ending where
# the level-2 region starts. Anchor rounded down to 8 (see module docstring).
l3 = sorted(e for es in setEnts.values() for e in es if e)
blob_lo, blob_hi = min(l3) & ~7, uniq[0]
assert all(blob_lo <= e < blob_hi and e % 4 == 0 for e in l3), "level-3 pointer outside blob window"
blob = d[off(blob_lo):off(blob_hi)]
idx = {a: n for n, a in enumerate(uniq)}          # l2 addr -> sUnitAnimSet_N number

L = [f"/* GENERATED by platform/pc/tools/gen_unit_anim_data.py (region '{REGION}', from {BASENAME}): the",
     " * three-level gUnitAnimSets table, relocated onto a local blob. Do not hand-edit; rerun the",
     ' * generator. See docs/pc-port/data-segment.md, "pc_unit_anim_data.c". */',
     '#include "graphics.h"', "", ""]

L.append(f"static const u8 sUnitAnimDataBlob[{len(blob)}] = {{")
for i in range(0, len(blob), 20):
    L.append("    " + ", ".join(f"0x{b:02x}" for b in blob[i:i+20]) + ",")
L.append("};")

for a in uniq:
    L += ["", f"static const u8 *sUnitAnimSet_{idx[a]}[{len(setEnts[a])}] = {{"]
    for e in setEnts[a]:
        if e:
            L.append(f"    &sUnitAnimDataBlob[{e - blob_lo}], /* orig 0x{e:08x} */")
        else:
            L.append("    (const u8 *)0, /* orig 0x00000000 (real NULL -- no animation for this state) */")
    L.append("};")

L += ["", "u8 **gUnitAnimSets[489] = {"]
for i, p in enumerate(ptrs):
    if p:
        L.append(f"    (u8 **)sUnitAnimSet_{idx[p]}, /* unit {i}, orig 0x{p:08x} */")
    else:
        L.append(f"    (u8 **)0, /* unit {i}, NULL in the original */")
L += [f"    /* indices {N}..488: implicit NULL -- safety widening, see include/graphics.h */", "};"]

open(OUT, "w").write("\n".join(L) + "\n")
zeros = sum(es.count(0) for es in setEnts.values())
print(f"wrote {OUT}: {sum(1 for p in ptrs if p)}/{N} units, {len(uniq)} unique sets, "
      f"{len(set(l3))} unique strips, blob {len(blob)}B @0x{blob_lo:08x}, {zeros} NULL anim slots")
