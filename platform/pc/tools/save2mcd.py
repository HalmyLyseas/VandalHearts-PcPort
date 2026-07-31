#!/usr/bin/env python3
"""Wrap a Vandal Hearts PC-port single-save into a standard 128 KB PSX memory-card image (.mcd/.mcr).

The port writes each save as a raw single-save export (`saves*/BASLUS-00447VH`) -- the save-block
content beginning with the 'SC' header (title + icon frames + game data). Emulators (DuckStation,
ePSXe, mednafen, ...) want a full 128 KB card image instead. This tool builds that card with a valid
directory so the save shows up in the emulator's in-game load menu.

  usage:  save2mcd.py <port-save> <out.mcd> [card-filename]
  e.g.:   save2mcd.py build_ubsan/saves_tactical/BASLUS-00447VH VandalHearts.mcd

The card-filename (default "BASLUS-00447VH") is the on-card directory name the *game* looks for, so
leave it as the default unless you know otherwise.

Note: the Tactical-mode rebalance is applied at runtime by the PC port, not baked into the save, so a
card made from a Tactical save loads as a plain/vanilla save on real hardware or a stock emulator.

Card layout (nocash psx-spx "Memory Card" spec):
  Block 0 (8 KB = 64 x 128B frames):
    frame 0        header 'MC' + XOR checksum
    frames 1..15   directory entries (one per data block 1..15)
    frames 16..35  broken-sector list (0xFFFFFFFF = none broken)
    frames 36..62  replacement data / unused (zero)
    frame 63       write-test (copy of header)
  Blocks 1..15 (8 KB each): save data; the save's 'SC' frame sits at the start of its first block.
"""
import sys
import struct


def cksum(frame127):
    c = 0
    for b in frame127:
        c ^= b
    return c


def build_card(save, name):
    if save[:2] != b"SC":
        raise ValueError("not a PSX save (missing 'SC' magic at offset 0)")
    nblk = save[3] if save[3] else (len(save) + 8191) // 8192   # block count (byte 3), fallback = ceil
    fsize = nblk * 8192
    card = bytearray(128 * 1024)

    def put(frame_idx, data):
        card[frame_idx * 128:(frame_idx + 1) * 128] = data

    def framed(fill):
        e = bytearray(128)
        fill(e)
        e[127] = cksum(e[0:127])
        return e

    # frame 0: header
    hdr = framed(lambda e: (e.__setitem__(0, ord('M')), e.__setitem__(1, ord('C'))))
    put(0, hdr)

    # directory frames 1..15 (one per data block)
    for blk in range(1, 16):
        idx = blk - 1                                   # 0-based data-block index
        if idx == 0:                                    # first block of our save
            def fill(e, fsize=fsize):
                e[0] = 0x51                             # used, first block of a linked file
                struct.pack_into("<I", e, 4, fsize)
                struct.pack_into("<H", e, 8, 1 if nblk > 1 else 0xFFFF)   # -> next data index / none
                nm = name.encode("ascii")[:20]
                e[10:10 + len(nm)] = nm
            put(blk, framed(fill))
        elif idx < nblk:                                # middle/last block(s) of our save
            last = (idx == nblk - 1)
            def fill(e, last=last, idx=idx):
                e[0] = 0x53 if last else 0x52           # last-linked / middle-linked
                struct.pack_into("<H", e, 8, 0xFFFF if last else (idx + 1))
            put(blk, framed(fill))
        else:                                           # free / formatted
            put(blk, framed(lambda e: (e.__setitem__(0, 0xA0),
                                       struct.pack_into("<H", e, 8, 0xFFFF))))

    # broken-sector list (16..35): 0xFFFFFFFF = none broken
    for f in range(16, 36):
        put(f, framed(lambda e: struct.pack_into("<I", e, 0, 0xFFFFFFFF)))
    # 36..62 stay zero; frame 63 write-test = header copy
    put(63, hdr)

    # data: the save's first byte ('S') lands at the start of card block 1 (offset 0x2000)
    card[8192:8192 + len(save)] = save
    return card, nblk


def main(argv):
    if len(argv) < 3:
        sys.exit(__doc__)
    save = open(argv[1], "rb").read()
    name = argv[3] if len(argv) > 3 else "BASLUS-00447VH"
    card, nblk = build_card(save, name)
    open(argv[2], "wb").write(card)
    print(f"save {len(save)} B, {nblk} block(s), name='{name}' -> {argv[2]} ({len(card)} B)")


if __name__ == "__main__":
    main(sys.argv)
