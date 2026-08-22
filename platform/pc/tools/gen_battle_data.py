#!/usr/bin/env python3
"""Regenerate platform/pc/src/pc_battle_data.c -- the reconstructed per-map battle unit
placement tables gBattleEnemyUnitInitialStates[74] and gBattlePartyUnitInitialStates[50]
(declared extern in include/battle.h).

WHY: the PC data-gen (build_data_segment.py) flags both as pointer-containing globals and
leaves them NULL (a raw pointer VALUE copied from the original binary would be a MIPS virtual
address, meaningless in our own process). SetupBattleUnits (src/states/game_setup.c)
dereferences gBattleEnemyUnitInitialStates[gState.mapNum] unconditionally, so every NULL entry
was a guaranteed crash the first time any battle state was reached.

WHAT: both arrays' pointer VALUES are read straight out of the byte-exact executable; every
one points into one small, contiguous data region directly preceding the array itself
(per table: [min pointed-to address, the array's own address)). Each per-map list is walked
by the game's own conventions -- enemy lists are 8-byte BattleEnemyUnitInitialState records
terminated by stripIdx==0, party lists are 4-byte BattlePartyUnitInitialState records
terminated by partyIdx==UNIT_INVALID (0xff); the terminator occupies 4 bytes and lists pack
contiguously. The full span is extracted as a raw byte blob (real data, not synthesized) and
each pointer is rebuilt as a real host pointer at the same relative offset into our local
copy, so terminator/overlap conventions (maps with no units share one sentinel whose
terminator byte deliberately aliases map 0's data start - 4) land exactly where the original
game intended.

Run from the repo root (vh/): python3 platform/pc/tools/gen_battle_data.py
Env (same conventions as gen_evt_entities.py):
  VH_REGION        'us' (default) | 'jp'
  VH_GAME_ROOT     tree providing symbol_addrs.txt + build/<exe> (default: this repo root;
                   jp: <root>/jp)
  VH_PSX_EXE       explicit executable path (default GAME_ROOT/build/<SLUS_004.47|SLPM_860.07>)
  VH_GENERATED_OUT output path (default platform/pc/src/pc_battle_data.c)

See memory milestone notes / exchange/12-phase-c-bootstrap.md Bug 11 for the original story.
"""
import struct, re, os, sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))  # vh/ (tools is 4 deep)
GAME_ROOT = os.environ.get("VH_GAME_ROOT", ROOT)
REGION = os.environ.get("VH_REGION", "us")
BASENAME = {"us": "SLUS_004.47", "jp": "SLPM_860.07"}[REGION]
EXE  = os.environ.get("VH_PSX_EXE", os.path.join(GAME_ROOT, "build", BASENAME))
SYMS = os.path.join(GAME_ROOT, "symbol_addrs.txt")
OUT  = os.environ.get("VH_GENERATED_OUT", os.path.join(ROOT, "platform/pc/src/pc_battle_data.c"))

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
# Entry counts come from the map's `// size:` annotation (count = size/4); the JP map carries
# no annotations, so missing counts fall back to the US map's -- the two trees share
# include/battle.h's [74]/[50] declarations (JP kept US naming/shapes for these tables).
us_sizes = None
def count_of(name):
    global us_sizes
    if sizes.get(name):
        return sizes[name] // 4
    if us_sizes is None:
        us_sizes = load_syms(os.path.join(ROOT, "symbol_addrs.txt"))[1]
    if not us_sizes.get(name):
        sys.exit(f"no size annotation for {name} in either {SYMS} or the US map")
    return us_sizes[name] // 4

# (symbol, C struct type, blob C name, record size, terminator byte value)
TABLES = [
    ("gBattleEnemyUnitInitialStates", "BattleEnemyUnitInitialState", "sBattleEnemyInitBlob", 8, 0x00),
    ("gBattlePartyUnitInitialStates", "BattlePartyUnitInitialState", "sBattlePartyInitBlob", 4, 0xff),
]

def walk_end(target, recsize, term):
    """End address (exclusive) of the per-map list at `target`: N records + 4-byte terminator."""
    a, n = target, 0
    while d[off(a)] != term:
        a += recsize; n += 1
        if n > 128:
            sys.exit(f"runaway list at 0x{target:08x} (recsize {recsize}): no terminator "
                     f"0x{term:02x} within 128 records -- record layout differs in this region?")
    return a + 4

out_chunks = []
stats = []
for name, ctype, blobname, recsize, term in TABLES:
    if name not in addrs:
        sys.exit(f"{name} not in {SYMS}")
    base = addrs[name]
    count = count_of(name)
    ptrs = [u32(base + i * 4) for i in range(count)]
    live = [p for p in ptrs if p]
    if not live:
        sys.exit(f"{name}: all {count} entries NULL in {EXE}?")
    blo = min(live)
    bhi = max(walk_end(p, recsize, term) for p in live)
    if not (t <= blo and bhi <= base <= t + len(d) - 0x800):
        sys.exit(f"{name}: blob span 0x{blo:08x}-0x{bhi:08x} not sane vs array @0x{base:08x}")
    for p in live:
        if not (blo <= p < bhi) or (p - blo) % 4:
            sys.exit(f"{name}: pointer 0x{p:08x} outside/misaligned in blob "
                     f"0x{blo:08x}-0x{bhi:08x} -- points into another table?")
    blob = d[off(blo):off(bhi)]
    lists = sorted(set(live))
    stats.append((name, count, len(live), count - len(live), len(lists), blo, bhi, len(blob)))

    lines = [f"static const u8 {blobname}[{len(blob)}] = {{"]
    for i in range(0, len(blob), 20):
        lines.append("    " + ", ".join(f"0x{b:02x}" for b in blob[i:i+20]) + ",")
    lines.append("};\n")
    out_chunks.append(("\n".join(lines), None))

    lines = [f"{ctype} *{name}[{count}] = {{"]
    for i, p in enumerate(ptrs):
        if p == 0:
            lines.append(f"    NULL, /* map {i} */")
        else:
            lines.append(f"    ({ctype} *)&{blobname}[{p - blo}], /* map {i}, orig 0x{p:08x} */")
    lines.append("};")
    out_chunks.append((None, "\n".join(lines)))

hdr = f"""/* platform/pc/src/pc_battle_data.c -- GENERATED by platform/pc/tools/gen_battle_data.py
 * (region: {REGION}, from byte-exact {BASENAME}). DO NOT hand-edit; rerun the generator.
 *
 * Real, extracted per-map battle unit placement tables: gBattleEnemyUnitInitialStates and
 * gBattlePartyUnitInitialStates (extern in include/battle.h). The PC data-gen
 * (build_data_segment.py) leaves pointer-typed globals NULL, and SetupBattleUnits
 * (src/states/game_setup.c) dereferences these unconditionally -- so both tables are
 * reconstructed here: each table's pointer values all land in one small contiguous data
 * region of the original binary, extracted verbatim as a local byte blob, with every
 * pointer rebuilt at the same relative offset into our copy. List terminators
 * (enemy stripIdx==0 / party partyIdx==UNIT_INVALID 0xff) and the shared empty-map
 * sentinel (whose terminator byte aliases map 0's data start) land exactly as the
 * original game intended. Stage-2 PC data only; not part of the matching build. */
#include "battle.h"

"""
body = "\n".join(c[0] for c in out_chunks if c[0]) + "\n" + \
       "\n\n".join(c[1] for c in out_chunks if c[1]) + "\n"
open(OUT, "w").write(hdr + body)

for name, count, live, nulls, lists, blo, bhi, blen in stats:
    print(f"{name}: {count} entries ({live} set, {nulls} NULL), {lists} distinct lists, "
          f"blob 0x{blo:08x}-0x{bhi:08x} ({blen}B)")
print(f"wrote {OUT}")
