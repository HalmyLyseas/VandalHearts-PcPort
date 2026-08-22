#!/usr/bin/env python3
"""Regenerate SLPM_860.07.yaml from tools/tus.txt (the decompiled-TU manifest).

The binary has three regions we reproduce byte-exact as raw bins, carving out the decompiled TUs:
  rodata region  [0x80010000 .. 0x80017bd4)  -> .rodata group (bin, linker_section_order: .rodata)
  G1 text region [0x80017bd4 .. 0x800e69f8)  -> .text (asm)
  data region    [0x800e69f8 .. end)         -> .data group (bin)  (G1 data + G2 rodata/text/data)

For each TU with a section in a region, we split that region's bin/asm around it and drop in the
`[off, .rodata|.data, name]` / `[off, c, name]` subsegment; splat pulls the section from the compiled
.o by name. This keeps everything else an exact raw copy. (G2-text TUs are not handled yet — they'd
need the data region's text sub-range promoted to .text; none decompiled so far.)

Run after editing tools/tus.txt, then `splat split` + tools/fix_linker_align.py + make.
"""
import os
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BASE = 0x80010000
GP  = 0x80124d8c                 # JP gp_value (= .sdata start), from the startup code
G2  = 0x801e6544                 # start of the `additional` (G2) block — its own code segment
# --- main segment (G1) ---
ROD = (0x80010000, 0x80017b9c)   # rodata region [start, end)
TXT = (0x80017b9c, 0x800e69f8)   # G1 text region — starts at `main` (core/main's first function;
                                 # was 0x80017bd4 while main's address was still unrecovered)
DAT = (0x800e69f8, GP)           # .data region (below gp)
SDAT = (GP, G2)                  # .sdata group: sdata + bss-zeros, placed after .data (ends at G2)
# --- additional segment (G2), at vram G2, mirroring the US `.additional` segment ---
A_ROD = (G2, 0x801e7244)         # G2 rodata (.rdata)
A_TXT = (0x801e7244, 0x801f8930) # G2 text
A_DAT = (0x801f8930, 0x801f9000) # G2 data .. file end

def foff(vram):
    return vram - BASE + 0x800

def load_manifest():
    tus = []
    for ln in open(os.path.join(ROOT, 'tools/tus.txt')):
        ln = ln.split('#')[0].strip()
        if not ln: continue
        p = ln.split()
        name = p[0]
        rv, rs, tv, ts, dv, ds = (int(x, 16) if x != '0' else 0 for x in p[1:7])
        # optional .sdata columns (8,9); default absent
        sv, ss = (int(p[7], 16), int(p[8], 16)) if len(p) >= 9 else (0, 0)
        tus.append(dict(name=name, rod=(rv, rs), txt=(tv, ts), dat=(dv, ds), sdat=(sv, ss)))
    return tus

def region_lines(region, tus, key, kind, section):
    """Emit subsegment lines for one region: bins between carved TU sections (sorted by addr).
    Only TU sections whose address falls WITHIN [start,end) are carved here (routes G1 vs G2)."""
    start, end = region
    carved = sorted([(t[key][0], t[key][1], t['name']) for t in tus
                     if t[key][1] > 0 and start <= t[key][0] < end])
    lines = []
    cur = start
    for vaddr, vsize, name in carved:
        if vaddr > cur:
            lines.append(bin_or_asm(cur, kind, section))
        lines.append(f'      - [0x{foff(vaddr):x}, {section}, {name}]')
        cur = vaddr + vsize
    if cur < end:
        lines.append(bin_or_asm(cur, kind, section))
    return lines

def bin_or_asm(vaddr, kind, section):
    off = foff(vaddr)
    if kind == 'asm':
        return f'      - [0x{off:x}, asm]'
    if kind == 'bintext':   # G2 text as raw bin (avoids spimdisasm splitting stray .data out of it)
        return f'      - {{ start: 0x{off:x}, type: bin, linker_section_order: .text }}'
    if section == '.rodata':
        return f'      - {{ start: 0x{off:x}, type: bin, linker_section_order: .rodata }}'
    if section == '.sdata':
        return f'      - {{ start: 0x{off:x}, type: bin, linker_section_order: .sdata }}'
    return f'      - [0x{off:x}, bin]'   # data region -> default .data group

def main():
    tus = load_manifest()
    # main segment (G1): rodata / text / data / sdata
    main_body = '\n'.join(
        region_lines(ROD, tus, 'rod', 'bin', '.rodata') +
        region_lines(TXT, tus, 'txt', 'asm', 'c') +
        region_lines(DAT, tus, 'dat', 'bin', '.data') +
        region_lines(SDAT, tus, 'sdat', 'bin', '.sdata'))
    # additional segment (G2): its own rodata / text / data (a G2 TU's rod/dat route here by address)
    add_body = '\n'.join(
        region_lines(A_ROD, tus, 'rod', 'bin', '.rodata') +
        region_lines(A_TXT, tus, 'txt', 'bintext', 'c') +
        region_lines(A_DAT, tus, 'dat', 'bin', '.data'))
    yaml = f"""name: SLPM_860.07
sha1: 136abe4839e507567f059a06094d0f8ac6165e96
options:
  basename: SLPM_860.07
  target_path: SLPM_860.07
  base_path: .
  platform: psx
  compiler: GCC
  ld_script_path: SLPM_860.07.ld
  find_file_boundaries: True
  gp_value: 0x80124d8c        # JP gp (= .sdata start), from startup code; drives %gp_rel resolution
  o_as_suffix: False
  use_legacy_include_asm: False
  section_order: [".rodata", ".text", ".data", ".sdata", ".sbss", ".bss"]
  symbol_addrs_path:
    - symbol_addrs.txt
  subalign: 4
segments:
  - name: header
    type: header
    start: 0x0
  - name: main
    type: code
    start: 0x800
    vram: 0x80010000
    bss_size: 0xc0000         # placeholder; nobits, does not affect the objcopy'd binary
    subsegments:
{main_body}
  - name: additional
    type: code
    start: 0x{foff(G2):x}
    vram: 0x{G2:08x}
    bss_size: 0x2000          # placeholder; nobits
    subsegments:
{add_body}
  - [0x1e9800]
"""
    open(os.path.join(ROOT, 'SLPM_860.07.yaml'), 'w').write(yaml)
    print(f'gen_yaml: wrote SLPM_860.07.yaml — {len(tus)} TUs')

if __name__ == '__main__':
    main()
