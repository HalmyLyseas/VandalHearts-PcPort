#!/usr/bin/env python3
"""Regenerate platform/pc/src/pc_event_anim_data.c -- the reconstructed cutscene/event unit
ANIMATION-SET pointer tables (gAnimSet_*) + the verbatim anim-byte window they index.

WHY: the PC data-gen (build_data_segment.py) leaves these pointer-containing globals NULL, flagged
for manual reconstruction; cutscene unit sprites then resolve animData=NULL -> gfxIdx=0 ->
INVISIBLE units (battle path worked via gUnitAnimSets). This script extracts every gAnimSet_* table
from the region's byte-exact executable and re-bases each PSX pointer onto a single verbatim blob
(sEventAnimBlob) spanning [lowest strip target .. end of the last table]. The window is dumped
verbatim -- it interleaves strip bytes with the pointer tables themselves; the embedded raw table
bytes are never indexed and keep the offsets trivial. gAnimSet_80101fc0 is intentionally EXCLUDED:
it is already real-defined in src/maps/unpack.c.

Region parameterisation (same scheme as gen_evt_entities.py): symbol NAMES are shared between the
US and JP trees (JP kept US naming at JP addresses), so one script serves both.
  VH_REGION=us|jp          (default us)
  VH_GAME_ROOT=<tree>      (default: this repo root; jp tree lives at <root>/jp)
  VH_PSX_EXE=<exe>         (default: GAME_ROOT/build/<SLUS_004.47|SLPM_860.07>)
  VH_GENERATED_OUT=<path>  (default: <root>/platform/pc/src/pc_event_anim_data.c)

Run from the repo root (vh/): python3 platform/pc/tools/gen_event_anim_data.py
Needs: the region's byte-exact matching build + its symbol_addrs.txt.
"""
import struct, re, sys, os, bisect

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))  # vh/ (tools is 4 deep)
GAME_ROOT = os.environ.get("VH_GAME_ROOT", ROOT)
REGION = os.environ.get("VH_REGION", "us")
BASENAME = {"us": "SLUS_004.47", "jp": "SLPM_860.07"}[REGION]
EXE  = os.environ.get("VH_PSX_EXE", os.path.join(GAME_ROOT, "build", BASENAME))
SYMS = os.path.join(GAME_ROOT, "symbol_addrs.txt")
OUT  = os.environ.get("VH_GENERATED_OUT", os.path.join(ROOT, "platform/pc/src/pc_event_anim_data.c"))

# Real-defined elsewhere in the port build -- never emit (would multiply-define at link).
EXCLUDE = {"gAnimSet_80101fc0"}   # defined in src/maps/unpack.c

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

# The JP map carries no `// size:` annotations, so missing sizes fall back to the US map's
# annotation for the same name, clamped by this region's own gap-to-next-symbol (a hard upper
# bound). Same approach as gen_evt_entities.py.
if any(sizes.get(k, 0) == 0 for k in addrs if k.startswith('gAnimSet_')):
    us_addrs, us_sizes = load_syms(os.path.join(ROOT, 'symbol_addrs.txt'))
    all_sorted = sorted(addrs.values())
    def gap(a):
        i = bisect.bisect_right(all_sorted, a)
        return (all_sorted[i] - a) if i < len(all_sorted) else 0x10000
    for k in list(addrs):
        if sizes.get(k, 0) == 0 and us_sizes.get(k, 0):
            sizes[k] = min(us_sizes[k], gap(addrs[k]))

tables = sorted((addrs[k], k) for k in addrs
                if k.startswith("gAnimSet_") and k not in EXCLUDE)
if not tables:
    sys.exit(f"no gAnimSet_* symbols in {SYMS}")

INEXE = lambda p: t <= p < t + len(d) - 0x800
entries = {}                                       # name -> [ptr values]
anomalies = []
for a, name in tables:
    sz = sizes.get(name, 0)
    if sz == 0 or sz % 4:
        sys.exit(f"{name}: unusable size {sz:#x} (no annotation and no US fallback?)")
    ents = [u32(a + i*4) for i in range(sz // 4)]
    for i, p in enumerate(ents):
        if p != 0 and not INEXE(p):
            anomalies.append(f"{name}[{i}] = {p:#x} not a pointer into the exe")
    entries[name] = ents
if anomalies:
    sys.exit("table entries fail pointer sanity (size overshoot?):\n  " + "\n  ".join(anomalies))

allptrs = [p for es in entries.values() for p in es if p]
blo = min(allptrs)                                 # lowest strip target
bhi = max(max(a + sizes[n] for a, n in tables),    # end of the last table...
          max(allptrs) + 4)                        # ...and never below the last pointer
if bhi - blo > 0x100000:
    sys.exit(f"blob window [{blo:#x},{bhi:#x}) implausibly large ({bhi-blo} bytes) -- bad size data?")
blob = d[off(blo):off(bhi)]

def ref(p):
    if p == 0: return "NULL"
    if not (blo <= p < bhi): sys.exit(f"pointer {p:#x} escaped blob window [{blo:#x},{bhi:#x})")
    return f"(u8 *)(sEventAnimBlob+{p - blo})"

L = [f"/* GENERATED by platform/pc/tools/gen_event_anim_data.py (region '{REGION}', from {BASENAME}): the",
     " * cutscene gAnimSet_* tables rebased onto a verbatim blob (gAnimSet_80101fc0 lives in",
     ' * src/maps/unpack.c). Do not hand-edit. See docs/pc-port/data-segment.md, "pc_event_anim_data.c". */',
     '#include "common.h"', "",
     f"static const unsigned char sEventAnimBlob[{len(blob)}] = {{"]
for i in range(0, len(blob), 24):
    L.append("  " + ",".join(str(b) for b in blob[i:i+24]) + ",")
L.append("};")
L.append("")
for a, name in tables:
    es = entries[name]
    L.append(f"u8 *{name}[{len(es)}] = {{ " + ", ".join(ref(p) for p in es) + " };")
open(OUT, "w").write("\n".join(L) + "\n")
print(f"wrote {OUT}: {len(tables)} tables ({sum(len(e) for e in entries.values())} strip ptrs, "
      f"{sum(1 for es in entries.values() for p in es if p == 0)} NULL), blob {len(blob)}B "
      f"[{blo:#x},{bhi:#x})")
for a, name in tables:
    print(f"  {name} @ {a:#x} n={len(entries[name])}")
