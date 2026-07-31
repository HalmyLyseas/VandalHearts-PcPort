#!/usr/bin/env python3
"""Score our harness VRAM against a DuckStation native VRAM dump (the G1 oracle).

Phase 0b of the G1 rasterizer effort: a stable, re-runnable error metric so each accuracy
change to libgpu.c can be measured against ground truth. DuckStation's "Debug -> Dump VRAM..."
writes a native 1024x512 PNG that aligns 1:1 with the harness's /tmp/harness_vram.ppm.

  usage: vram_diff.py <duckstation_vramdump.png> [harness_vram.ppm]

Reports, over the final-frame draw area (x0-319 y16-255):
  - fraction of pixels differing at all (5-bit native), and by how many levels
  - split of differing pixels into 1-level (dither-consistent) vs >=2-level (UV/blend/coverage)
  - mean 8-bit abs-diff (headline scalar to drive down)
Writes an amplified /tmp/vram_diff.png for eyeballing where the residual lives.
"""
import sys
import numpy as np
from PIL import Image

# final-frame draw area in VRAM (see harness log: drawarea=(0,16 320x240))
X0, Y0, W, H = 0, 16, 320, 240


def read_ppm(p):
    with open(p, 'rb') as f:
        assert f.readline().strip() == b'P6', "not a P6 ppm"
        w, h = map(int, f.readline().split())
        f.readline()
        return np.frombuffer(f.read(w * h * 3), dtype=np.uint8).reshape(h, w, 3).astype(np.int16)


def main(ds_png, ours_ppm):
    ds = np.asarray(Image.open(ds_png).convert('RGB'), dtype=np.int16)
    ours = read_ppm(ours_ppm)
    d = ds[Y0:Y0 + H, X0:X0 + W]
    o = ours[Y0:Y0 + H, X0:X0 + W]

    adiff = np.abs(d - o).max(axis=2)               # 8-bit max-channel abs diff
    lvl = np.abs((d >> 3) - (o >> 3)).max(axis=2)   # 5-bit native level diff
    tot = lvl.size
    diff = lvl > 0

    print(f"draw area {W}x{H} @ ({X0},{Y0})  |  mean 8-bit abs-diff = {adiff.mean():.2f}  (HEADLINE)")
    print(f"identical (5-bit): {100*(lvl==0).mean():.2f}%   differing: {100*diff.mean():.2f}%")
    print("level histogram:", "  ".join(f"{L}:{100*(lvl==L).mean():.1f}%" for L in range(6)),
          f" >=6:{100*(lvl>=6).mean():.1f}%")
    if diff.any():
        print(f"of differing px: {100*(lvl[diff]==1).mean():.1f}% 1-level (dither) / "
              f"{100*(lvl[diff]>=2).mean():.1f}% >=2-level (UV/blend/coverage)")
    Image.fromarray(np.clip(adiff * 3, 0, 255).astype(np.uint8)).save('/tmp/vram_diff.png')
    print("wrote /tmp/vram_diff.png (3x amplified)")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    main(sys.argv[1], sys.argv[2] if len(sys.argv) > 2 else '/tmp/harness_vram.ppm')
