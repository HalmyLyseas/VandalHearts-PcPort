#!/usr/bin/env python3
"""Compile src/<tu>.c, disassemble one function from the .o, immediate-preserving
diff vs the JP binary function of the same name. Iteration aid for localized TUs.
Usage: verify_fn.py <tu> <FuncName> [jp_start] [jp_end]
  jp_start: explicit JP address (for no-jp functions the map lacks).
  jp_end:   explicit end bound — REQUIRED when the next map symbol is missing or
            bogus -- a phantom symbol pointing into a function's epilogue clips the
            real boundary and truncates the compare window."""
import sys, re, subprocess, os
BASE = 0x80010000
tu, fn = sys.argv[1], sys.argv[2]
def load_funcs(path):
    d = {}
    for ln in open(path):
        m = re.match(r'(\w+)\s*=\s*0x([0-9a-f]+);.*type:func', ln)
        if m: d[m.group(1)] = int(m.group(2), 16)
    return d
jpf = load_funcs('symbol_addrs.txt')
def norm(mn, op):
    op = re.sub(r'0x[0-9a-f]{5,}', 'A', op)   # mask long reloc addrs
    if mn in ('j', 'jal'): op = 'T'
    return f'{mn} {op}'.strip()
# JP side: disasm the named function's range from the target binary.
# Optional 3rd arg = explicit JP address (for no-jp functions the map lacks).
import bisect
jpv = sorted(jpf.values())
ja = int(sys.argv[3], 16) if len(sys.argv) > 3 else jpf[fn]
je = int(sys.argv[4], 16) if len(sys.argv) > 4 else jpv[bisect.bisect_right(jpv, ja)]
data = open('SLPM_860.07','rb').read()[0x800:]
open('/tmp/vf_jp.bin','wb').write(data[ja-BASE:je-BASE])
out = subprocess.run(['mipsel-linux-gnu-objdump','-D','-b','binary','-m','mips:3000','-EL',
                      f'--adjust-vma={ja}','/tmp/vf_jp.bin'],capture_output=True,text=True).stdout
jp=[]
for l in out.splitlines():
    m=re.match(r'^([0-9a-f]{8}):\t[0-9a-f ]+\t(\S+)(?:\s+(.*))?$',l)
    if m: jp.append(norm(m.group(2), m.group(3) or ''))
# OUR side: compile through the pipeline, find the fn symbol in the .o, disasm it
o = f'build/src/{tu}.c.o'
os.makedirs(os.path.dirname(o), exist_ok=True)
subprocess.run(['rm', '-f', o])
r = subprocess.run(['make', o], stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                   stdin=subprocess.DEVNULL, text=True)
if not os.path.exists(o):
    print(r.stdout[-3000:]); sys.exit(1)
# get symbol offset+size in the .o
syms = subprocess.run(['mipsel-linux-gnu-objdump','-t',o],capture_output=True,text=True).stdout
off=sz=None
for l in syms.splitlines():
    m=re.match(r'^([0-9a-f]{8})\s+.*\s+\.text\s+([0-9a-f]{8})\s+'+re.escape(fn)+r'$',l)
    if m: off=int(m.group(1),16); sz=int(m.group(2),16)
if off is None:
    print(f'{fn} not found in {o} (still static? not compiled?)'); sys.exit(1)
txt = subprocess.run(['mipsel-linux-gnu-objcopy','-O','binary','-j','.text',o,'/dev/stdout'],
                     capture_output=True).stdout[off:off+sz]
open('/tmp/vf_our.bin','wb').write(txt)
out2=subprocess.run(['mipsel-linux-gnu-objdump','-D','-b','binary','-m','mips:3000','-EL',
                     f'--adjust-vma={ja}','/tmp/vf_our.bin'],capture_output=True,text=True).stdout
our=[]
for l in out2.splitlines():
    m=re.match(r'^([0-9a-f]{8}):\t[0-9a-f ]+\t(\S+)(?:\s+(.*))?$',l)
    if m: our.append(norm(m.group(2), m.group(3) or ''))
import difflib
print(f'OUR {len(our)} insns  vs  JP {len(jp)} insns  ({fn})')
d=list(difflib.unified_diff(our, jp, 'OUR','JP', lineterm='', n=1))
if not d: print('*** IDENTICAL (immediate-preserving) ***')
for l in d: print(l)
