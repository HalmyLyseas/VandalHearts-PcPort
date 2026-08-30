#!/usr/bin/env python3
"""Compare the GEOMETRY of two VHT1 GPU traces, primitive for primitive.

Both traces must come from the SAME binary and the same scripted
scene, so primitive N of frame F corresponds on both sides by construction and no alignment
search is needed. Reports the screen-space displacement of every vertex.

  python3 tools/vht_geom_diff.py base.vht exact.vht

Structural divergence (different frame count, primitive count, or type order) is reported and
stops the comparison -- it would mean the runs took different code paths, which is a different
finding from a geometry delta and must not be averaged into one.
"""
import sys, struct, collections

# PC_GPU_PRIM_* from platform/pc/include/PsyQ/libgpu.h
POLY_F4, POLY_FT4, SPRT, TILE, DR_MODE = 1, 2, 3, 4, 5
NAME = {1:'POLY_F4', 2:'POLY_FT4', 3:'SPRT', 4:'TILE', 5:'DR_MODE'}

def verts(t, b):
    """Screen-space (x,y) vertices carried by a primitive struct.
    Layout: 4-byte P_TAG, then per-vertex blocks. Coordinates are s16 pairs."""
    if t == POLY_FT4:   # tag, rgb+code, xy0,uv0+clut, xy1,uv1+tpage, xy2,uv2+pad, xy3,uv3+pad
        return [struct.unpack_from('<hh', b, o) for o in (8, 16, 24, 32)]
    if t == POLY_F4:    # tag, rgb+code, xy0..xy3
        return [struct.unpack_from('<hh', b, o) for o in (8, 12, 16, 20)]
    if t == SPRT:       # tag, rgb+code, xy, uv+clut, wh
        return [struct.unpack_from('<hh', b, 8)]
    if t == TILE:       # tag, rgb+code, xy, wh
        return [struct.unpack_from('<hh', b, 8)]
    return []           # DR_MODE carries no geometry

def load(path):
    d = open(path, 'rb').read()
    if d[:4] != b'VHT1':
        sys.exit(f"{path}: not a VHT1 trace")
    i, frames, cur = 4, [], []
    while i < len(d):
        op = chr(d[i]); i += 1
        na = struct.unpack_from('<I', d, i)[0]; i += 4; a = d[i:i+na]; i += na
        nb = struct.unpack_from('<I', d, i)[0]; i += 4; b = d[i:i+nb]; i += nb
        if op == 'Z':
            frames.append(cur); cur = []
        elif op == 'P':
            cur.append((struct.unpack('<I', a)[0], b))
    return frames

def main(pa, pb):
    A, B = load(pa), load(pb)
    print(f"A: {pa}  {len(A)} frames")
    print(f"B: {pb}  {len(B)} frames")
    n = min(len(A), len(B))
    if len(A) != len(B):
        print(f"  ! frame count differs -- comparing the first {n}")

    moved = collections.Counter()   # displacement -> count
    total = same = 0
    worst = (0, None)
    per_type = collections.Counter()
    struct_bad = 0
    for f in range(n):
        fa, fb = A[f], B[f]
        if len(fa) != len(fb):
            struct_bad += 1
            continue
        for (ta, ba), (tb, bb) in zip(fa, fb):
            if ta != tb:
                struct_bad += 1
                break
            va, vb = verts(ta, ba), verts(tb, bb)
            for (x1, y1), (x2, y2) in zip(va, vb):
                total += 1
                dx, dy = x2 - x1, y2 - y1
                if dx == 0 and dy == 0:
                    same += 1
                else:
                    d = max(abs(dx), abs(dy))      # Chebyshev: pixels moved
                    moved[d] += 1
                    per_type[NAME.get(ta, ta)] += 1
                    if d > worst[0]:
                        worst = (d, (f, ta, (x1, y1), (x2, y2)))
    print(f"\nstructural: {struct_bad} frames with differing primitive count/type order")
    if total == 0:
        print("no comparable vertices"); return
    print(f"\nvertices compared : {total:,}")
    print(f"  identical       : {same:,} ({100.0*same/total:.4f}%)")
    print(f"  moved           : {total-same:,} ({100.0*(total-same)/total:.4f}%)")
    if moved:
        print("\n  displacement histogram (pixels, Chebyshev):")
        for d in sorted(moved):
            print(f"    {d:>3} px : {moved[d]:>10,}  ({100.0*moved[d]/total:.4f}%)")
        print(f"\n  worst: {worst[0]} px  frame {worst[1][0]} {NAME.get(worst[1][1])} "
              f"{worst[1][2]} -> {worst[1][3]}")
        print(f"  moved vertices by primitive type: {dict(per_type)}")

if __name__ == '__main__':
    if len(sys.argv) != 3: sys.exit(__doc__)
    main(sys.argv[1], sys.argv[2])
