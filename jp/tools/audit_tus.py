#!/usr/bin/env python3
"""Audit tools/tus.txt for overlapping TU section ranges.

Any overlap means two TUs claim the same ROM bytes -> the link inserts the
duplicate and every value after it drifts by +N (N = the overlapping section's
size). That signature (uniform +N shift, N == a section size of the newest TU)
is how a rodata overlap typically surfaces: a TU's addend-derived range absorbed a
neighbour's bytes.

This catches TU-vs-TU overlaps only; a matched TU absorbing bytes of a
STILL-UNDECOMPILED neighbor (raw bin) is invisible until that neighbor is
integrated -- diagnose that case by the drift signature above.

Usage: python3 tools/audit_tus.py   (from the repo root; exit 1 on overlap)
"""
import sys

SECS = ['rodata', 'text', 'data', 'sdata']
rows = []
for ln in open('tools/tus.txt'):
    ln = ln.strip()
    if not ln or ln.startswith('#'):
        continue
    p = ln.split()
    vals = [int(x, 16) for x in p[1:]]
    for i in range(0, len(vals), 2):
        a, sz = vals[i], vals[i + 1]
        if a and sz:
            rows.append((a, a + sz, p[0], SECS[i // 2]))
rows.sort()
overlaps = 0
for (s1, e1, n1, t1), (s2, e2, n2, t2) in zip(rows, rows[1:]):
    if s2 < e1:
        overlaps += 1
        print(f"OVERLAP: {n1}.{t1} [{s1:#x},{e1:#x}) vs {n2}.{t2} [{s2:#x},{e2:#x})")
ntus = len(set(r[2] for r in rows))
print(f"{len(rows)} section ranges across {ntus} TUs — overlaps: {overlaps}")
sys.exit(1 if overlaps else 0)
