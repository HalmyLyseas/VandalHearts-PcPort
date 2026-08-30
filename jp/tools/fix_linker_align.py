#!/usr/bin/env python3
import re
"""Post-`splat split` fixup for the generated linker script.

splat's PSX linker-script template hardcodes `. = ALIGN(., 16)` between the .text and
.data section groups. The single-blob asm baseline never hit this (everything lived in one
.text section), but once we carve a real C TU out and put the trailing data region in its own
section (a `bin` subsegment), that align pads the text->data boundary up to 16 bytes.

The real SLPM_860.07 layout has .text ending at 0x800e69f8 (8-aligned) with .data starting
immediately — no 16-byte pad. So we drop that one spurious align. This is safe for this binary
in general: the original linker inserted no padding at these boundaries, so removing the align
just lets each section follow its predecessor at the exact original offset.

Run this after every `splat split` (the .ld is only regenerated then, not on every make).
Idempotent: a no-op if the align is already gone.
"""
import sys, os
LD = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'SLPM_860.07.ld')

def main():
    t = open(LD).read()
    # splat's template inserts `. = ALIGN(., 16)` at every section-group boundary; the real
    # SLPM_860.07 packs sections contiguously with no 16-byte pad. Drop the align before every
    # RODATA/TEXT/DATA/SDATA END marker in any segment (BSS is nobits, left alone).
    n = 0
    for marker in re.findall(r'(\w+_(?:RODATA|TEXT|DATA|SDATA)_END) = \.;', t):
        needle = f"        . = ALIGN(., 16);\n        {marker} = .;"
        repl = f"        {marker} = .;"
        if needle in t:
            t = t.replace(needle, repl); n += 1
    # inter-segment: the `additional` (G2) block follows `main` contiguously in the file — drop the
    # 16-byte ROM/VMA padding that splat inserts after `main` (main PROGBITS end at 0x1d6d44).
    inter = "    __romPos = ALIGN(__romPos, 16);\n    . = ALIGN(., 16);\n    main_ROM_END = __romPos;"
    if inter in t:
        t = t.replace(inter, "    main_ROM_END = __romPos;"); n += 1
    # main's tiny nobits .bss (from decompiled C TUs) sits at VMA G2 and inflates SIZEOF(.main),
    # pushing additional_ROM_START past the true file offset of G2 (0x1d6d44). Pin it explicitly so
    # the `additional` block lands at G2's file offset regardless of main's bss.
    if "additional_ROM_START = __romPos;" in t:
        t = t.replace("additional_ROM_START = 0x1d6d44;", "additional_ROM_START = __romPos;")  # idempotent
        t = t.replace("additional_ROM_START = __romPos;", "additional_ROM_START = 0x1d6d44;"); n += 1
    # `.additional` is the last segment; its trailing (unused) .bss would be materialized as extra
    # file bytes by objcopy. End the section at DATA_END — drop the .bss inputs (they fall through to
    # /DISCARD/) so the emitted binary stops at 0x1e9800.
    m = re.search(r'(additional_BSS_START = \.;\n).*?(\n\s*additional_BSS_END = \.;)', t, re.DOTALL)
    if m and m.group(0) != m.group(1) + m.group(2).lstrip('\n'):
        t = t[:m.start()] + m.group(1).rstrip('\n') + m.group(2) + t[m.end():]; n += 1
    if n:
        open(LD, 'w').write(t)
        print(f"fix_linker_align: dropped {n} spurious ALIGN(.,16)")
    else:
        print("fix_linker_align: nothing to do (already patched or template changed)")

if __name__ == '__main__':
    main()
