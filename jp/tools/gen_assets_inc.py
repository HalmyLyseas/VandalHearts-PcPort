#!/usr/bin/env python3
"""Regenerate the hand-authored `assets/*.inc` initializer includes from SLPM_860.07.

Three matched TUs `#include` a C initializer body that splat does NOT produce (splat only carves
raw `assets/*.bin` blobs). Those `.inc` files are disc-derived data, so per the project's policy
(.gitignore: "the target binary and any disc-derived data NEVER get committed") they are
REGENERATED here rather than committed -- the same treatment splat gives `assets/*.bin`.

Run it alongside `splat split` (see the README pipeline); without it, a fresh clone fails at
    src/maps/map_32.c:30:10: fatal error: assets/801009bc.inc: No such file or directory

`--check` compares the generated output against whatever is already on disk instead of
writing, so the files can be verified without touching them.

    python3 tools/gen_assets_inc.py [--check]
"""
import argparse
import os
import struct
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TARGET = os.path.join(ROOT, 'SLPM_860.07')
ASSETS = os.path.join(ROOT, 'assets')

BASE = 0x8000f800  # vram -> file offset: off = vram - BASE

# name             kind          JP vram      n   included by
JOBS = [
    ('801009bc', 'tilemodel', 0x80102b94, 4),  # src/maps/map_32.c  -- rail tile models
    ('80100e9c', 'tilemodel', 0x80103074, 1),  # src/maps/map_35.c
    ('8010102c', 'strtable',  0x80103530, 100),  # src/core/text.c -- gStringTable[100]
]

# MapTileModel (include/graphics.h): SVECTOR vertices[22]; s16 gfx[18]; u8 faces[18][4];
#                                    u8 shades[18]; u8 faceCt; u8 height;   => 304 bytes
TILEMODEL_SIZE = 22 * 8 + 18 * 2 + 18 * 4 + 18 + 1 + 1


def s16(b, o):
    return struct.unpack_from('<h', b, o)[0]


def u32(b, o):
    return struct.unpack_from('<I', b, o)[0]


def tilemodel(blob, off, wrap):
    """One MapTileModel initializer as the 5 lines the .inc files use.

    `wrap` is True for an array element (the element's own braces live in the .inc) and False for a
    single struct (the enclosing braces live in the .c file).
    """
    verts = []
    for i in range(22):
        o = off + i * 8
        verts.append('{%d, %d, %d, %d}' % tuple(s16(blob, o + k * 2) for k in range(4)))
    o = off + 176
    gfx = ', '.join(str(s16(blob, o + i * 2)) for i in range(18))
    o = off + 212
    faces = ', '.join('{%d, %d, %d, %d}' % tuple(blob[o + i * 4 + k] for k in range(4))
                      for i in range(18))
    o = off + 284
    shades = ', '.join(str(blob[o + i]) for i in range(18))
    face_ct, height = blob[off + 302], blob[off + 303]

    open_brace = '{' if wrap else ''
    return [
        '%s{%s},' % (open_brace, ', '.join(verts)),
        ' {%s},' % gfx,
        ' {%s},' % faces,
        ' {%s},' % shades,
        ' %d, %d' % (face_ct, height),
    ]


def build(kind, vram, n, blob):
    off = vram - BASE
    if kind == 'strtable':
        # gStringTable: absolute JP pointers, emitted as casts; every entry keeps a trailing comma.
        return ''.join('(u8 *)%s,\n' % hex(u32(blob, off + i * 4)) for i in range(n))
    if kind == 'tilemodel':
        if n == 1:
            return '\n'.join(tilemodel(blob, off, wrap=False)) + '\n'
        lines = []
        for i in range(n):
            body = tilemodel(blob, off + i * TILEMODEL_SIZE, wrap=True)
            # close the element; every element but the last is comma-separated
            body[-1] += '}' + (',' if i != n - 1 else '')
            lines += body
        return '\n'.join(lines) + '\n'
    raise SystemExit('unknown kind ' + kind)


def ensure_include_symlink():
    """`#include "assets/X.inc"` is resolved through the `-Iinclude` search path, so `include/assets`
    must point at `assets/`. Both are generated/ignored, so the link is recreated here rather than
    committed -- without it the build dies with `fatal error: assets/801009bc.inc`."""
    link = os.path.join(ROOT, 'include', 'assets')
    if os.path.islink(link):
        if os.readlink(link) == '../assets':
            return
        os.unlink(link)
    elif os.path.exists(link):
        return  # a real directory someone made on purpose; leave it alone
    os.symlink('../assets', link)
    print('%-14s -> ../assets (created)' % 'include/assets')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--check', action='store_true',
                    help='compare against the files on disk instead of writing')
    args = ap.parse_args()

    if not os.path.exists(TARGET):
        sys.exit('gen_assets_inc: %s not found (supply the target binary)' % TARGET)
    blob = open(TARGET, 'rb').read()
    os.makedirs(ASSETS, exist_ok=True)
    if not args.check:
        ensure_include_symlink()

    bad = 0
    for name, kind, vram, n in JOBS:
        text = build(kind, vram, n, blob)
        path = os.path.join(ASSETS, name + '.inc')
        if args.check:
            have = open(path).read() if os.path.exists(path) else None
            ok = have == text
            bad += not ok
            print('%-14s %s' % (name + '.inc', 'OK' if ok else
                                ('MISSING' if have is None else 'DIFFERS')))
        else:
            open(path, 'w').write(text)
            print('%-14s wrote %d bytes (from %#x)' % (name + '.inc', len(text), vram))
    if args.check and bad:
        sys.exit('gen_assets_inc: %d file(s) differ' % bad)


if __name__ == '__main__':
    main()
