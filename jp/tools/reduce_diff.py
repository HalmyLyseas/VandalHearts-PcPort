#!/usr/bin/env python3
"""Reduce a verify_fn diff to REAL differences (strict reloc filter).

Usage:
    python3 tools/verify_fn.py <tu> <Fn> [jp_start [jp_end]] > /tmp/fn.diff
    python3 tools/reduce_diff.py /tmp/fn.diff

Prints only hunks with genuine differences and a REAL HUNKS count.
0 real hunks == the function matches modulo relocations (the md5 is
still the only whole-TU proof — data sections and never-flagged
functions can still differ).

RULES (the battle/field lesson, 2026-08-16): a -/+ pair may be
cancelled as relocation noise ONLY if it carries a reloc:
  - identical text, or
  - same shape AND a memory op (lw/sw/lh/sh/lb/sb/lbu/lhu/lwl/lwr/swl/swr)
    -> the displacement is a %lo/gprel addend
  - same shape AND lui                 -> %hi
  - same shape AND addiu rX,rX,imm     -> %lo add
A `li`/`ori`/`slti`/shift-immediate mismatch is ALWAYS real: those are
source literals (coordinates, widths, constants). A loose filter that
cancels same-shape li pairs hid 5 real coordinate diffs in a function
that "matched" 296/296 (Objf597) and 2 more in a never-flagged one.
"""
import re
import sys


def main(path):
    lines = open(path).read().splitlines()
    hunks = []
    cur = None
    for l in lines:
        if l.startswith("@@"):
            cur = [l]
            hunks.append(cur)
        elif cur is not None:
            cur.append(l)

    MEM = re.compile(r'^(lw|sw|lh|sh|lb|sb|lbu|lhu|lwl|lwr|swl|swr)\s')

    def relocish(m, p):
        if m == p:
            return True
        nm = re.sub(r'-?\d+', 'N', m)
        np = re.sub(r'-?\d+', 'N', p)
        if nm != np:
            return False
        if MEM.match(m):
            return True
        ma = re.match(r'addiu (\S+),(\S+),', m)
        if ma and ma.group(1) == ma.group(2):
            return True
        if m.startswith("lui"):
            return True
        return False  # li/ori/slti/shift literal mismatch = REAL

    n = 0
    for h in hunks:
        minus = [l[1:] for l in h
                 if l.startswith("-") and not re.match(r'-lui \S+,0x[01]$', l)]
        plus = [l[1:] for l in h
                if l.startswith("+") and not re.match(r'\+lui \S+,0x8[0-9a-f]{3}$', l)]
        rem = list(plus)
        real = []
        for m in minus:
            for p in rem:
                if relocish(m, p):
                    rem.remove(p)
                    break
            else:
                real.append("-" + m)
        real += ["+" + p for p in rem]
        if real:
            n += 1
            print(h[0])
            for r in real:
                print("  ", r)
    print("REAL HUNKS:", n)


if __name__ == "__main__":
    main(sys.argv[1])
