#!/usr/bin/env python3
"""Reconstruct source initializer includes from a user-owned US PS-X EXE."""
import os
import struct
import sys

LOAD_ADDR = 0x80010000
FILE_BASE = 0x800


def file_offset(address):
    return address - LOAD_ADDR + FILE_BASE


def map_model(blob):
    off = 0
    vertices = [struct.unpack_from("<4h", blob, off + i * 8) for i in range(22)]
    off += 22 * 8
    gfx = struct.unpack_from("<18h", blob, off)
    off += 18 * 2
    faces = [tuple(blob[off + i * 4:off + i * 4 + 4]) for i in range(18)]
    off += 18 * 4
    shades = tuple(blob[off:off + 18])
    off += 18
    return "{\n  { %s },\n  { %s },\n  { %s },\n  { %s },\n  %d, %d\n}" % (
        ", ".join("{%s}" % ",".join(map(str, v)) for v in vertices),
        ", ".join(map(str, gfx)),
        ", ".join("{%s}" % ",".join(map(str, face)) for face in faces),
        ", ".join(map(str, shades)), blob[off], blob[off + 1])


def main():
    exe, output_dir = sys.argv[1:3]
    data = open(exe, "rb").read()
    if not data.startswith(b"PS-X EXE"):
        raise SystemExit("not a PS-X EXE: %s" % exe)
    os.makedirs(output_dir, exist_ok=True)
    model_size = 0x138
    models = data[file_offset(0x801009bc):file_offset(0x801009bc) + model_size * 4]
    with open(os.path.join(output_dir, "801009bc.inc"), "w") as out:
        out.write(",\n".join(map_model(models[i:i + model_size]) for i in range(0, len(models), model_size)))
        out.write("\n")
    model = data[file_offset(0x80100e9c):file_offset(0x80100e9c) + model_size]
    with open(os.path.join(output_dir, "80100e9c.inc"), "w") as out:
        rendered = map_model(model)
        out.write(rendered[2:-2] + "\n")
    glyphs = data[file_offset(0x801012e4):file_offset(0x801012e4) + 128 * 9]
    with open(os.path.join(output_dir, "801012e4.inc"), "w") as out:
        for i in range(128):
            out.write("{%s},\n" % ",".join("0x%02x" % b for b in glyphs[i * 9:i * 9 + 9]))


if __name__ == "__main__":
    main()
