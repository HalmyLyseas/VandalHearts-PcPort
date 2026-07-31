#!/usr/bin/env python3
"""Analyze a port VH_RAYLOG capture and compare the effect quads to the hardware reference.

The casting-ray / SFX layer is, on real hardware, ~dozens of SMALL additive textured quads
(from the DuckStation trace: tpage 0x0036, abr=1, neutral colour, per-quad median ~24x13 px,
UV cell 24x64). If our port projects those same quads too LARGE or piled up, additive blending
stacks them into the solid blue blob. This groups a VH_RAYLOG dump by frame and reports, per
frame, the quad count, per-quad size distribution, screen footprint, and the screenArea/texelArea
ratio -- so an inflated frame stands out immediately against the hardware reference.

  usage:  VH_RAYLOG=1 ./vandalhearts_pc 2> raylog.txt   # (play the effect)
          analyze_raylog.py raylog.txt

Hardware reference (trace): per-quad median 24x13 px; ratio (screenArea/texelArea) ~ O(0.5-2).
Flag: frames whose median quad is much larger, or whose ratio balloons = geometry inflation.
"""
import sys, re, statistics as st
from collections import defaultdict

LINE = re.compile(
    r'\[raylog\] f=(\d+) tpage=0x([0-9a-fA-F]+) clut=0x([0-9a-fA-F]+) abr=(\d+) .*?'
    r'xy=\((-?\d+),(-?\d+)\)\((-?\d+),(-?\d+)\)\((-?\d+),(-?\d+)\)\((-?\d+),(-?\d+)\) '
    r'screenArea=([\d.]+) texelArea=([\d.]+) ratio=([\d.]+)')

def main(path):
    frames = defaultdict(list)
    for line in open(path):
        m = LINE.search(line)
        if not m:
            continue
        f = int(m.group(1))
        xs = [int(m.group(i)) for i in (5, 7, 9, 11)]
        ys = [int(m.group(i)) for i in (6, 8, 10, 12)]
        w, h = max(xs) - min(xs), max(ys) - min(ys)
        frames[f].append((w, h, float(m.group(2 + 0)) if False else int(m.group(2), 16),
                          float(m.group(15)), float(m.group(14)), float(m.group(13))))
    if not frames:
        sys.exit("no [raylog] lines found — did you run with VH_RAYLOG=1 and redirect 2> ?")
    print(f"{'frame':>6} {'quads':>5} {'wMed':>5} {'hMed':>5} {'wMax':>5} {'hMax':>5} "
          f"{'ratioMed':>8} {'ratioMax':>8}   flag")
    print(f"  [hardware ref: wMed~24 hMed~13 ratioMed~O(1)]")
    for f in sorted(frames):
        q = frames[f]
        ws = [a for a, *_ in q]; hs = [b for _, b, *_ in q]
        ratios = [r for *_, r in q]
        wmed, hmed = st.median(ws), st.median(hs)
        flag = ""
        if wmed > 60 or hmed > 45:
            flag = "<< INFLATED quads"
        elif max(ratios) > 20:
            flag = "<< high magnify"
        print(f"{f:>6} {len(q):>5} {wmed:>5.0f} {hmed:>5.0f} {max(ws):>5.0f} {max(hs):>5.0f} "
              f"{st.median(ratios):>8.1f} {max(ratios):>8.1f}   {flag}")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    main(sys.argv[1])
