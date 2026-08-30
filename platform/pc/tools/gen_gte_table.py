#!/usr/bin/env python3
"""Extract the PsyQ packed sin/cos table from a retail Vandal Hearts executable.

exchange/110 item 2c, step-1 measurement harness. The real PsyQ RotMatrix (US: 0x800d0aa8) reads a
4096-entry table at US VRAM 0x8011C6C0: entry i = (cos_i << 16) | sin_i, both 4096 fixed point.
Verified layout: hi[i] == lo[(i+1024) % 4096] for all 4096 entries (the classic rsin/rcos packing).

Writes a raw 16 KB blob for VH_GTE_EXACT=<path> to load at runtime. This is GAME DATA extracted
from your own copy -- like every other reconstruction in this project it is generated locally and
must never be committed.

  python3 tools/gen_gte_table.py ../../SLUS_004.47 /tmp/gte_table.bin
"""
import sys, struct, math

VRAM_TO_FILE = 0x8000F800          # file offset = vram - this (main segment starts at 0x800)
TABLE_VRAM   = {'us': 0x8011C6C0}  # JP address not yet located -- see exchange/110
N            = 4096

def main(exe, out, region='us'):
    data = open(exe, 'rb').read()
    off  = TABLE_VRAM[region] - VRAM_TO_FILE
    if off + N * 4 > len(data):
        sys.exit(f"table at 0x{TABLE_VRAM[region]:08X} (file 0x{off:X}) runs past EOF -- wrong exe?")
    blob = data[off:off + N * 4]

    lo = [struct.unpack_from('<h', blob, i * 4)[0]     for i in range(N)]
    hi = [struct.unpack_from('<h', blob, i * 4 + 2)[0] for i in range(N)]
    # Structural check: the packing identity must hold exactly, or we read the wrong address.
    bad = sum(1 for i in range(N) if hi[i] != lo[(i + 1024) % N])
    if bad:
        sys.exit(f"not the sin/cos table: {bad}/{N} entries fail hi[i]==lo[(i+1024)%4096]")
    # Sanity: values must track sin/cos within the table's own coarseness.
    dev = max(abs(lo[i] - math.sin(i * 2 * math.pi / N) * 4096) for i in range(N))
    if dev > 16:
        sys.exit(f"table does not track sin() (max deviation {dev:.1f}) -- wrong address?")

    open(out, 'wb').write(blob)
    print(f"wrote {out}: {len(blob)} bytes, {N} entries, max deviation from ideal sin/cos {dev:.2f}/4096")

if __name__ == '__main__':
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    main(sys.argv[1], sys.argv[2], sys.argv[3] if len(sys.argv) > 3 else 'us')
