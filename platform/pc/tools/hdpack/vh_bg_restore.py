#!/usr/bin/env python3
"""
vh_bg_restore.py -- Vandal Hearts HD-background detail-restoration pipeline.

Reproduces, in pure Python (PIL + numpy, no GIMP), the validated GIMP recipe that
counters AI-upscale over-smoothing by grafting the *original's own* high-frequency
detail back onto the AI upscale, then locking hue/saturation to the source.

The recipe (from workflow/recipe.txt), reverse-engineered against the GIMP output
and matched to +/-1 per channel on the detail/colour ops (the only residual is the
bicubic kernel on the smooth baseline -- sub-perceptual, ~2 mean, visually identical):

  inputs:  ORIG  = clean jpsxdec PNG export of the background (e.g. 320x240)
           AI    = AI upscale of ORIG at exactly 4x (e.g. 1280x960)

  step 1   origUp = bicubic upscale of ORIG to AI's size          (smooth low-freq baseline)
  step 2   GE     = clamp(AI - origUp + 128)      [GIMP "Grain extract"]  -> AI's added detail
  step 3   GM     = clamp(AI + GE     - 128)      [GIMP "Grain merge"]    -> detail-boosted image
  step 4   Final  = HSL-Color(H,S from GM(8-bit clamped), L from (AI+GE-128) unclamped)
                                                  [GIMP "HSL Color": palette from GM, luminance
                                                   from the high-precision grain-merge composite]

Why it works: AI upscalers discard high-frequency texture (waxy/plastic look) and can drift
in colour. Grain-extract/merge re-injects the ORIGINAL's real per-scene detail (recover, don't
invent); the HSL-Color pass locks hue+saturation to the source so colour can't wander -- a
"sharpen without reinterpreting" guarantee, and a consistency lever across the whole pack.

Usage:
    ./vh_bg_restore.py                 # batch: originals/ + upscayl/ -> processed/ (paths below)
    ./vh_bg_restore.py ORIG.png AI.png OUT.png                     # one file
    ./vh_bg_restore.py --orig DIR --ai DIR --out DIR               # override folders

Default folders are resolved RELATIVE TO THIS SCRIPT (its parent dir), matching the layout:
    <parent>/originals/                              <- jpsxdec PNG exports
    <parent>/upscayl_png_upscayl-standard-4x_4x/     <- Upscayl 4x AI upscales
    <parent>/processed/                              <- output (same filenames)
Pairs are matched by IDENTICAL filename in the originals and upscayl folders.

Deps: pip install pillow numpy
"""
import sys, os, argparse
import numpy as np
from PIL import Image

# Default folders, relative to this script's parent directory:
_ROOT    = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ORIG_DIR = os.path.join(_ROOT, "originals")
AI_DIR   = os.path.join(_ROOT, "upscayl_png_upscayl-standard-4x_4x")
OUT_DIR  = os.path.join(_ROOT, "processed")


def _rgb2hsl(rgb):
    """rgb: HxWx3 float 0..255 -> (H[0..360), S[0..1], L[0..1]) in the HSL (lightness) model."""
    mx = rgb.max(-1) / 255.0
    mn = rgb.min(-1) / 255.0
    l = (mx + mn) / 2.0
    d = mx - mn
    s = np.where(d == 0, 0.0, d / (1.0 - np.abs(2.0 * l - 1.0) + 1e-12))
    dd = np.where(d == 0, 1.0, d)
    r, g, b = rgb[..., 0] / 255.0, rgb[..., 1] / 255.0, rgb[..., 2] / 255.0
    hr = ((g - b) / dd) % 6.0
    hg = ((b - r) / dd) + 2.0
    hb = ((r - g) / dd) + 4.0
    h = np.where(mx * 255.0 == rgb[..., 0], hr, np.where(mx * 255.0 == rgb[..., 1], hg, hb))
    h = np.where(d != 0, h * 60.0, 0.0)
    return h, s, l


def _hsl2rgb(h, s, l):
    c = (1.0 - np.abs(2.0 * l - 1.0)) * s
    hp = h / 60.0
    x = c * (1.0 - np.abs(hp % 2.0 - 1.0))
    m = l - c / 2.0
    z = np.zeros_like(h)
    cds = [hp < 1, hp < 2, hp < 3, hp < 4, hp < 5, hp <= 6]
    r = np.select(cds, [c, x, z, z, x, c])
    g = np.select(cds, [x, c, c, x, z, z])
    b = np.select(cds, [z, z, x, c, c, x])
    return np.clip(np.stack([r + m, g + m, b + m], -1) * 255.0, 0, 255)


def restore(orig_path, ai_path):
    """Run the pipeline. Returns a uint8 HxWx3 array."""
    orig = Image.open(orig_path).convert("RGB")
    ai_im = Image.open(ai_path).convert("RGB")
    W, H = ai_im.size
    if (W, H) != (orig.size[0] * 4, orig.size[1] * 4):
        # not exactly 4x -> still proceed, matching AI's resolution
        print(f"  note: AI {W}x{H} is not exactly 4x of ORIG {orig.size[0]}x{orig.size[1]}", file=sys.stderr)
    AI = np.asarray(ai_im).astype(np.float64)
    origUp = np.asarray(orig.resize((W, H), Image.BICUBIC)).astype(np.float64)  # step 1
    GE  = np.clip(AI - origUp + 128.0, 0, 255)   # step 2  Grain extract
    GM  = np.clip(AI + GE     - 128.0, 0, 255)   # step 3  Grain merge (8-bit, = palette source)
    GMun = AI + GE - 128.0                        #         high-precision (L source), unclamped
    hs_h, hs_s, _ = _rgb2hsl(GM)                  # step 4  HSL Color: H,S from GM ...
    _, _, l_l     = _rgb2hsl(GMun)                #                     ... L from unclamped grain-merge
    out = _hsl2rgb(hs_h, hs_s, l_l)
    return out.astype(np.uint8)


def _run_one(orig, ai, out):
    Image.fromarray(restore(orig, ai)).save(out)
    print(f"  {os.path.basename(out)}")


def main():
    ap = argparse.ArgumentParser(description="VH HD-background detail-restoration pipeline")
    ap.add_argument("args", nargs="*", help="ORIG.png AI.png OUT.png   (one-file mode; omit for batch)")
    ap.add_argument("--orig", default=ORIG_DIR, metavar="DIR", help="originals folder (default: ./originals)")
    ap.add_argument("--ai",   default=AI_DIR,   metavar="DIR", help="AI-upscale folder")
    ap.add_argument("--out",  default=OUT_DIR,  metavar="DIR", help="output folder")
    a = ap.parse_args()

    if len(a.args) == 3:
        _run_one(*a.args); return
    if a.args:
        ap.print_help(); sys.exit(2)

    # batch: pair by identical filename present in both --orig and --ai
    if not os.path.isdir(a.orig): sys.exit(f"originals folder not found: {a.orig}")
    if not os.path.isdir(a.ai):   sys.exit(f"upscale folder not found: {a.ai}")
    os.makedirs(a.out, exist_ok=True)
    names = sorted(f for f in os.listdir(a.orig) if f.lower().endswith(".png"))
    if not names: sys.exit(f"no .png files in {a.orig}")
    print(f"originals: {a.orig}\nupscales:  {a.ai}\noutput:    {a.out}")
    n = skip = 0
    for name in names:
        ai = os.path.join(a.ai, name)
        if not os.path.exists(ai):
            print(f"  SKIP {name}: no matching upscale", file=sys.stderr); skip += 1; continue
        _run_one(os.path.join(a.orig, name), ai, os.path.join(a.out, name)); n += 1
    print(f"done: {n} processed" + (f", {skip} skipped" if skip else ""))


if __name__ == "__main__":
    main()
