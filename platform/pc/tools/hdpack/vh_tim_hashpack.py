#!/usr/bin/env python3
"""
vh_tim_hashpack.py -- build the HD BACKGROUND pack OFFLINE, from the disc assets alone.

WHY THIS WORKS (the core model, shared with portraits -- see vh_hd_derivation.md):
  The engine identifies any asset by an FNV-1a/64 hash of the exact bytes DMA'd to VRAM
  (LoadImage). jpsxdec extracts each disc file as RAW CD SECTORS: 2352 bytes each =
  24-byte header + 2048-byte data payload + 280-byte ECC (sync "00 FF*10 00" every 2352).
  So the real file content = concatenation of the 2048-byte payloads. (The "24-byte
  preamble" and the "304-byte gap" seen in earlier notes were exactly this sector framing:
  304 = 24 + 280.)

  For a background, that reconstructed payload IS a standard 8bpp TIM. Parse it (TIM header
  -> CLUT block -> image block), then FNV-1a the 320x240 image data as u16 words -> the very
  hash the game asks for. Validated: reproduces 30/30 in-game-captured background hashes.

    ./vh_tim_hashpack.py --tim ../DATA --hd ../processed-webp --out /tmp/hdpack
  (accepts .TIM files that are raw-sector dumps; also plain files if not sector-framed)

Deps: numpy
"""
import struct, numpy as np, os, glob, shutil, argparse

SEC, HDR, DATA = 2352, 24, 2048
SYNC = bytes([0] + [0xFF] * 10 + [0])
OFF, PR, MASK = 1469598103934665603, 1099511628211, (1 << 64) - 1


def sector_payload(path):
    """Reconstruct real file bytes from a jpsxdec raw-sector dump (or return as-is if not framed)."""
    d = open(path, "rb").read()
    if len(d) >= SEC and d[:12] == SYNC[:12] and len(d) % SEC == 0:
        return b"".join(d[s * SEC + HDR: s * SEC + HDR + DATA] for s in range(len(d) // SEC))
    return d  # already de-sectored


def fnv_words(b):
    b = np.frombuffer(b, np.uint8)
    w = b.reshape(-1, 2).astype(np.uint64)
    wv = w[:, 0] | (w[:, 1] << np.uint64(8))
    h = OFF
    for x in wv.tolist():
        h = ((h ^ x) * PR) & MASK
    return h


def bg_hash(path):
    """Reconstruct the background TIM, parse it, and hash its 320x240 8bpp image data."""
    p = sector_payload(path)
    off = 0
    tid, flags = struct.unpack_from("<II", p, off)
    if tid != 0x10:
        return None
    off += 8
    if flags & 8:                                   # skip CLUT block
        off += struct.unpack_from("<I", p, off)[0]
    off += 4                                         # image block length
    dx, dy, w, h = struct.unpack_from("<HHHH", p, off)
    off += 8
    if (w, h) != (160, 240):                         # 320x240 8bpp backgrounds only
        return None
    return fnv_words(p[off:off + w * h * 2])


def main():
    ap = argparse.ArgumentParser(description="Offline HD background pack from disc TIMs (CD-sector model)")
    ap.add_argument("--tim", required=True, help="folder of background .TIM files (raw-sector dumps ok)")
    ap.add_argument("--hd",  required=True, help="HD assets named '<TIM>[0]_p00.webp' (jpsxdec convention)")
    ap.add_argument("--out", required=True, help="output pack folder (<hash>.webp)")
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    n = made = skip = 0
    for p in sorted(glob.glob(os.path.join(a.tim, "*.TIM"))):
        base = os.path.basename(p); n += 1
        h = bg_hash(p)
        if h is None:
            skip += 1; continue
        cand = sorted(glob.glob(os.path.join(a.hd, base + "*.webp")))
        if cand:
            shutil.copyfile(cand[0], os.path.join(a.out, f"{h:016x}.webp")); made += 1
            print(f"  {h:016x} <- {os.path.basename(cand[0])}")
        else:
            print(f"  {h:016x}    {base}  (no HD webp)")
    print(f"\n{n} TIMs, {made} pack entries -> {a.out}  ({skip} non-320x240 skipped)")


if __name__ == "__main__":
    main()
