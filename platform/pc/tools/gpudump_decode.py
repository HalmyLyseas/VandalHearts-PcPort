#!/usr/bin/env python3
"""Decode a DuckStation .psxgpu(.zst) GPU trace (ps1dev GPUDUMP.md format) and inventory it.

Phase 0 of the G1 rasterizer harness: prove the trace is a usable per-primitive oracle, extract the
initial VRAM + the GP0 draw-command stream. Format (from external/duckstation/src/core/gpu_dump.{h,cpp}):
  - stream of packets; each: 4-byte LE header = length_words | (type<<24), then length_words * u32 body.
  - type 0x00 GPUPort0Data = ONE GP0 command (opcode = body[0]>>24) + its params.
  - type 0x02 VSyncEvent = frame boundary. 0x05 TraceBegin, 0x06 GPUVersion, 0x10-0x12 metadata.
  - initial VRAM = a GPUPort0Data 0xA0 (CPU->VRAM) upload of the whole 1024x512 (dest 0, size 0).
"""
import sys, struct, collections

PKT = {0x00:"GPUPort0Data",0x01:"GPUPort1Data",0x02:"VSyncEvent",0x03:"DiscardPort0",
       0x04:"ReadbackPort0",0x05:"TraceBegin",0x06:"GPUVersion",0x10:"GameID",
       0x11:"VideoFormat",0x12:"Comment"}
VRAM_W, VRAM_H = 1024, 512

def gp0_name(op):
    if op == 0x00: return "NOP"
    if op == 0x01: return "ClearCache"
    if op == 0x02: return "FillRect"
    if 0x20 <= op <= 0x3F: return "Polygon"
    if 0x40 <= op <= 0x5F: return "Line"
    if 0x60 <= op <= 0x7F: return "Rect"
    if 0x80 <= op <= 0x9F: return "VRAMtoVRAM"
    if 0xA0 <= op <= 0xBF: return "CPUtoVRAM"
    if 0xC0 <= op <= 0xDF: return "VRAMtoCPU"
    if 0xE1 <= op <= 0xE6: return {0xE1:"DrawMode",0xE2:"TexWindow",0xE3:"DrawAreaTL",
                                   0xE4:"DrawAreaBR",0xE5:"DrawOffset",0xE6:"MaskBit"}[op]
    return f"op{op:02x}"

def main(path):
    data = open(path, "rb").read()
    if data[:4] == b'\x28\xb5\x2f\xfd':  # zstd magic
        try: import zstandard
        except ImportError: sys.exit("need `pip install zstandard` (or pre-decompress with `zstd -d`)")
        data = zstandard.ZstdDecompressor().decompress(data, max_output_size=1<<30)
    print(f"decompressed: {len(data)} bytes")
    FILE_HEADER = b'PSXGPUDUMPv1\x00\x00'   # 14 bytes; packets follow
    if data[:len(FILE_HEADER)] != FILE_HEADER: sys.exit("bad magic (not a PSXGPUDUMPv1 file)")

    pos, pkt_hist, gp0_hist = len(FILE_HEADER), collections.Counter(), collections.Counter()
    frames, ft4_tpages, initial_vram, draw_cmds = 0, collections.Counter(), None, []
    while pos + 4 <= len(data):
        hdr = struct.unpack_from("<I", data, pos)[0]; pos += 4
        length = hdr & 0xFFFFFF; ptype = (hdr >> 24) & 0xFF
        body = data[pos:pos + length*4]; pos += length*4
        pkt_hist[PKT.get(ptype, f"?{ptype:02x}")] += 1
        if ptype == 0x02: frames += 1
        if ptype == 0x00 and length >= 1:  # one GP0 command
            w0 = struct.unpack_from("<I", body, 0)[0]; op = (w0 >> 24) & 0xFF
            nm = gp0_name(op); gp0_hist[nm] += 1
            if 0xA0 <= op <= 0xBF and length > 100000 and initial_vram is None:
                initial_vram = body[12:12 + VRAM_W*VRAM_H*2]     # skip cmd,dest,size (3 words)
            elif nm == "Polygon":
                textured = bool(op & 0x04); nverts = 4 if (op & 0x08) else 3
                gouraud = bool(op & 0x10); semi = bool(op & 0x02)
                # words: [color] v0 [uv0] v1 [uv1] ...  -- capture tpage (2nd uv word hi16) for textured
                if textured:
                    ws = struct.unpack_from(f"<{length}I", body, 0)
                    # layout: cmd, v0, uv0(clut), v1, uv1(page), ... (flat, non-gouraud)
                    if not gouraud and length >= 5:
                        tpage = (ws[4] >> 16) & 0xFFFF
                        ft4_tpages[f"tpage=0x{tpage:04x} nverts={nverts} semi={int(semi)}"] += 1
                draw_cmds.append((nm, op, length))
            else:
                draw_cmds.append((nm, op, length))
    print(f"\npackets: {dict(pkt_hist)}")
    print(f"frames (VSync): {frames}")
    print(f"\nGP0 command histogram:"); [print(f"  {n:14s} {c}") for n,c in gp0_hist.most_common()]
    print(f"\ntextured-flat-polygon tpages (find the ray quads, tpage~0x0036):")
    [print(f"  {k}: {c}") for k,c in ft4_tpages.most_common(12)]
    if initial_vram:
        out = path.rsplit(".",2)[0] + ".initvram.bin" if path.endswith(".zst") else path + ".initvram.bin"
        open(out,"wb").write(initial_vram)
        print(f"\ninitial VRAM ({len(initial_vram)} bytes = {VRAM_W}x{VRAM_H}x2) -> {out}")

if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else sys.exit("usage: gpudump_decode.py <trace.psxgpu[.zst]>"))
