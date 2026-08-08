#!/usr/bin/env python3
"""gen_packart.py -- generate the two pack-art glyph sheets from a bitmap font.

The KROMDAT-free way to get a non-Latin pack's sheets: it reads the RAW pixels out of a BDF bitmap
font (GNU Unifont by default, bundled as unifont-subset.bdf) -- no rasterisation, just Unifont's own
hand-designed bitmaps fitted to the game's cell sizes -- and writes the pack-art format lang_build.py
consumes with --packart:

    <out>/font8x9.png  + .txt   (8x9  cells: the small font -- almost everything)
    <out>/font16x15.png + .txt  (16x15 cells: the large font -- item names)
    <out>/proof_8x9.png, proof_16x15.png   each cell x12 with its codepoint, for eyeball QA

PNG = 1-bit, cells packed left->right/top->bottom, BLACK = ink.  .txt = one U+XXXX per cell.

The result is a real starting sheet -- 16x15 is production-quality; 8x9 is legible (a team may want to
hand-tweak a few dense letters). Rendering the proof and confirming each glyph is the letter its
codepoint claims is the check the format relies on (see the README).

Usage: ./gen_packart.py <out> --script ru            (a preset alphabet)
       ./gen_packart.py <out> --cps U+0401,U+0410-U+042F   (explicit codepoints)
       ./gen_packart.py <out> --font other.bdf --cps ...   (a different BDF)
"""
import os, sys
from PIL import Image, ImageDraw, ImageOps

SMALL, LARGE = (8, 9), (16, 15)
HERE = os.path.dirname(os.path.abspath(__file__))
DEFAULT_FONT = os.path.join(HERE, "unifont-subset.bdf")

# Preset alphabets (uppercase -- the game draws most text in capitals). Extend as new scripts land.
SCRIPTS = {
    "ru": [0x0401] + list(range(0x0410, 0x0430)),                      # Russian: Ё + А..Я (33)
    "el": [c for c in range(0x0391, 0x03AA) if c != 0x03A2],           # Greek caps Α..Ω (24)
}


def parse_cps(spec):
    out = []
    for part in spec.split(","):
        part = part.strip()
        if "-" in part:
            a, b = part.split("-")
            out += range(int(a[2:], 16), int(b[2:], 16) + 1)
        else:
            out.append(int(part[2:], 16))
    return out


def parse_bdf(path):
    """-> {codepoint: (BBX tuple, [hex rows])}."""
    glyphs, cp, bbx, rows, reading = {}, None, None, None, False
    for line in open(path, encoding="latin1"):
        if line.startswith("ENCODING"):
            cp = int(line.split()[1])
        elif line.startswith("BBX"):
            _, w, h, xo, yo = line.split()
            bbx = (int(w), int(h), int(xo), int(yo))
        elif line.startswith("BITMAP"):
            reading, rows = True, []
        elif line.startswith("ENDCHAR"):
            if cp is not None and bbx:
                glyphs[cp] = (bbx, rows)
            reading, cp, bbx = False, None, None
        elif reading:
            rows.append(line.strip())
    return glyphs


def glyph_mask(bbx, rows):
    """The glyph's own pixels as an L image (255=ink), cropped to ink, or None if blank."""
    w, h, _xo, _yo = bbx
    if w == 0 or h == 0:
        return None
    nbytes = (w + 7) // 8
    img = Image.new("L", (w, h), 0)
    px = img.load()
    for y, rh in enumerate(rows[:h]):
        val = int(rh, 16) if rh else 0
        for x in range(w):
            if val & (1 << (nbytes * 8 - 1 - x)):
                px[x, y] = 255
    bb = img.getbbox()
    return img.crop(bb) if bb else None


def _ink_resize(mask, nw, nh, cover):
    """Downscale to nw x nh, ink-preserving: a target pixel is ink if >= `cover` of the source region
    it covers is ink. Pure integer math (deterministic, no resampling library); keeps thin strokes."""
    gw, gh = mask.size
    px = mask.load()
    out = Image.new("L", (nw, nh), 0)
    op = out.load()
    for ty in range(nh):
        y0, y1 = ty * gh // nh, max(ty * gh // nh + 1, (ty + 1) * gh // nh)
        for tx in range(nw):
            x0, x1 = tx * gw // nw, max(tx * gw // nw + 1, (tx + 1) * gw // nw)
            ink = tot = 0
            for sy in range(y0, y1):
                for sx in range(x0, x1):
                    tot += 1
                    ink += px[sx, sy] >= 128
            if tot and ink / tot >= cover:
                op[tx, ty] = 255
    return out


def fit_cell(mask, W, H, cover=0.5):
    """Fit the glyph into the cell by downscaling to fill, but NEVER upscaling (per axis the target is
    min(source, cell)): a dimension larger than the cell is squished in -- the small font is monospace,
    so glyphs fill the 8px width -- while a dimension that already fits keeps Unifont's native pixels,
    centred (no blocky 2x doubling in the roomy 16x15 cell). One rule, best form for each sheet."""
    gw, gh = mask.size
    nw, nh = min(gw, W), min(gh, H)
    small = _ink_resize(mask, nw, nh, cover)
    cell = Image.new("L", (W, H), 0)
    cell.paste(small, ((W - nw) // 2, (H - nh) // 2))
    return cell


def build(glyphs, cps, W, H, cover):
    per_row = 16
    rows_n = (len(cps) + per_row - 1) // per_row
    png = Image.new("L", (per_row * W, rows_n * H), 0)
    txt, missing = [], []
    for n, cp in enumerate(cps):
        txt.append(f"U+{cp:04X}")
        g = glyphs.get(cp)
        m = glyph_mask(*g) if g else None
        if m is None:
            missing.append(cp)
            continue
        png.paste(fit_cell(m, W, H, cover), ((n % per_row) * W, (n // per_row) * H))
    return png, txt, missing


def save(out, name, png, txt, W, H, cps):
    ImageOps.invert(png).convert("1").save(os.path.join(out, f"{name}.png"))     # black = ink
    with open(os.path.join(out, f"{name}.txt"), "w") as f:
        f.write("# one U+XXXX per cell, PNG reading order (16 cells/row)\n" + "\n".join(txt) + "\n")
    Z, per_row = 12, 16
    rows_n = (len(cps) + per_row - 1) // per_row
    cw, ch = W * Z + 34, H * Z + 8
    proof = Image.new("L", (per_row * cw, rows_n * ch), 255)
    d = ImageDraw.Draw(proof)
    for n, cp in enumerate(cps):
        x0, y0 = (n % per_row) * cw, (n // per_row) * ch
        cell = png.crop(((n % per_row) * W, (n // per_row) * H,
                         (n % per_row) * W + W, (n // per_row) * H + H))
        proof.paste(ImageOps.invert(cell).resize((W * Z, H * Z), Image.NEAREST), (x0 + 2, y0 + 2))
        d.rectangle([x0 + 1, y0 + 1, x0 + 2 + W * Z, y0 + 2 + H * Z], outline=128)
        d.text((x0 + W * Z + 6, y0 + H * Z // 2 - 4), chr(cp), fill=0)
    proof.save(os.path.join(out, f"proof_{name.replace('font', '')}.png"))


def main():
    if len(sys.argv) < 2 or sys.argv[1].startswith("-"):
        raise SystemExit(__doc__)
    out = sys.argv[1]
    font = sys.argv[sys.argv.index("--font") + 1] if "--font" in sys.argv else DEFAULT_FONT
    if "--script" in sys.argv:
        key = sys.argv[sys.argv.index("--script") + 1]
        if key not in SCRIPTS:
            raise SystemExit(f"unknown --script {key!r}; known: {', '.join(SCRIPTS)} (or use --cps)")
        cps = SCRIPTS[key]
    elif "--cps" in sys.argv:
        cps = parse_cps(sys.argv[sys.argv.index("--cps") + 1])
    else:
        raise SystemExit("give --script <name> or --cps U+....")
    os.makedirs(out, exist_ok=True)
    glyphs = parse_bdf(font)
    for (W, H), name in ((SMALL, "font8x9"), (LARGE, "font16x15")):
        png, txt, missing = build(glyphs, cps, W, H, 0.5)
        save(out, name, png, txt, W, H, cps)
        print(f"{name}: {len(cps) - len(missing)}/{len(cps)} glyphs"
              + (f"  MISSING {[hex(c) for c in missing]} (not in {os.path.basename(font)})"
                 if missing else ""))
    print(f"wrote {out}/  -- review proof_8x9.png / proof_16x15.png, then build with "
          f"--packart {out}")


if __name__ == "__main__":
    main()
