#!/usr/bin/env python3
"""Regenerate platform/pc/src/pc_sprite_box_quads.c -- the reconstructed gSpriteBoxQuads[19]
sprite-box Quad pointer table.

WHY: build_data_segment.py flags gSpriteBoxQuads as pointer-typed and zero-fills it (raw MIPS
pointer values are meaningless in our process), so every entry was NULL. The real table's 19
pointers all land in one small contiguous Quad span directly preceding the table itself; we
embed that span as a blob and emit the table with each entry relocated into the blob.

THE feedback-25 SUBTLETY (frozen-live-global class, see the hand-written original's comment):
entries whose original address IS a named live gQuad_* global must point at THE LIVE GLOBAL,
not the frozen blob copy -- on hardware they are the same object, and per-frame by-name writes
(the arrow rotation, fx quads, flyer shadows) must reach what the renderer reads. The named
gQuad_* symbols exist in both regions' symbol maps, so the aliasing is resolved per region.

Region parameterisation: VH_REGION ('us' default | 'jp'), VH_GAME_ROOT,
VH_PSX_EXE, VH_GENERATED_OUT -- same contract as gen_evt_entities.py.
"""
import os
import re
import struct

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))  # vh/
GAME_ROOT = os.environ.get("VH_GAME_ROOT", ROOT)
REGION = os.environ.get("VH_REGION", "us")
BASENAME = {"us": "SLUS_004.47", "jp": "SLPM_860.07"}[REGION]
EXE = os.environ.get("VH_PSX_EXE", os.path.join(GAME_ROOT, "build", BASENAME))
SYMS = os.path.join(GAME_ROOT, "symbol_addrs.txt")
OUT = os.environ.get("VH_GENERATED_OUT",
                     os.path.join(ROOT, "platform/pc/src/pc_sprite_box_quads.c"))

COUNT = 19
QUAD_BYTES = 32  # typedef SVECTOR Quad[4] -- 4 vectors of 4 shorts


def main():
    d = open(EXE, "rb").read()
    load = struct.unpack_from("<I", d, 0x18)[0]

    def foff(a):
        return a - load + 0x800

    addrs = {}
    for line in open(SYMS):
        m = re.match(r'(\w+)\s*=\s*(0x[0-9a-fA-F]+);', line)
        if m:
            addrs[m.group(1)] = int(m.group(2), 16)
    table = addrs["gSpriteBoxQuads"]
    live = {addrs[k]: k for k in addrs if k.startswith("gQuad_")}

    ptrs = struct.unpack_from("<%dI" % COUNT, d, foff(table))
    lo = min(ptrs)
    # The span runs from the lowest target up to the table itself (verified layout in both
    # regions: the Quad data sits immediately before gSpriteBoxQuads).
    assert all(lo <= p < table and (p - lo) % 4 == 0 for p in ptrs), \
        [hex(p) for p in ptrs]
    span = table - lo
    assert span <= 0x400, hex(span)  # runaway guard; real span is 0x220 in both regions
    blob = d[foff(lo):foff(lo) + span]

    L = ["/* gSpriteBoxQuads[%d] (include/graphics.h), reconstructed from the byte-exact binary: every" % COUNT,
         " * entry targets a Quad inside one 512-byte span. See docs/pc-port/data-segment.md,",
         ' * "pc_sprite_box_quads.c". */',
         '#include "graphics.h"', "",
         "/* Not const: RenderUnitSprite rotates a box's Y coordinates in place and other units swap",
         " * gSpriteBoxQuads[N] between Quad sources, so a rodata blob is a write-protection SIGSEGV. */",
         "static u8 sSpriteBoxQuadBlob[%d] = {" % len(blob)]
    for i in range(0, len(blob), 20):
        L.append("    " + ", ".join("0x%02x" % b for b in blob[i:i + 20]) + ",")
    L.append("};")
    L.append("")
    L.append("Quad *gSpriteBoxQuads[%d] = {" % COUNT)
    for i, p in enumerate(ptrs):
        if p in live:
            L.append("    &%s, /* box %d, orig 0x%08x (live global) */" % (live[p], i, p))
        else:
            L.append("    (Quad *)&sSpriteBoxQuadBlob[%d], /* box %d, orig 0x%08x */"
                     % (p - lo, i, p))
    L.append("};")
    open(OUT, "w").write("\n".join(L) + "\n")
    nlive = sum(1 for p in ptrs if p in live)
    print("wrote %s (span %d bytes, %d live-global entries)" % (OUT, span, nlive))


if __name__ == "__main__":
    main()
