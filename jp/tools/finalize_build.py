#!/usr/bin/env python3
"""Post-batch fixups so the build links: run after match_tu.py + gen_yaml.py + splat split.

Two recurring issues once TUs are carved out of the bins:
  1. Duplicate address in symbol_addrs.txt — the reorganized US source sometimes uses a second name
     for one address (e.g. DrawSjisText == DrawText). splat rejects duplicate addresses, so move the
     later alias to undefined_additional.txt (a linker script, which allows duplicates).
  2. Address-encoded danglers — a still-asm referrer into a carved section loses its `D_XXXX` /
     `func_XXXX` / `s_..._XXXX` global (the C emits a local label). Since the name encodes the
     address, define it absolutely in undefined_additional.txt. Loop the link until none remain.

Usage: tools/finalize_build.py   (dedups symbol_addrs, then loops make to drain danglers)
"""
import subprocess, re, os, sys
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)

def dedup_symbols():
    seen, keep, dups = {}, [], []
    for ln in open('symbol_addrs.txt'):
        m = re.match(r'(\w+)\s*=\s*0x([0-9a-fA-F]+);', ln)
        if not m: keep.append(ln.rstrip('\n')); continue
        name, addr = m.group(1), int(m.group(2), 16)
        if addr in seen and seen[addr] != name: dups.append((name, addr))
        else: seen[addr] = name; keep.append(ln.rstrip('\n'))
    if dups:
        open('symbol_addrs.txt', 'w').write('\n'.join(keep) + '\n')
        existing = {re.match(r'(\w+)', l).group(1) for l in open('undefined_additional.txt') if re.match(r'\w+\s*=', l)}
        with open('undefined_additional.txt', 'a') as f:
            for n, a in dups:
                if n not in existing: f.write(f'{n} = 0x{a:08x};\n')
        print(f'dedup: moved {len(dups)} duplicate-address symbols to aliases:', [n for n, _ in dups])
        return True
    return False

def drain_danglers():
    existing = {re.match(r'(\w+)', l).group(1) for l in open('undefined_additional.txt') if re.match(r'\w+\s*=', l)}
    total = 0
    for _ in range(8):
        out = subprocess.run(['make', 'PYTHON=python3', 'CROSS=mipsel-linux-gnu-', 'build/SLPM_860.07.elf'],
                             capture_output=True, text=True, stdin=subprocess.DEVNULL, env=os.environ).stderr
        # Only D_/func_ names are safe to define from the name (the encoded address is already
        # the JP address, from disassembly). Source-derived names (s_/g_..._XXXX) encode the US
        # address instead and must be recovered by match_tu -- an undefined one is a gap to fix.
        refs = set(re.findall(r"`((?:D_|func_)([0-9A-Fa-f]{8}))'", out))
        add = [f'{full} = 0x{int(a, 16):08x};' for full, a in refs
               if full not in existing and int(a, 16) >= 0x80000000]
        if not add: break
        open('undefined_additional.txt', 'a').write('\n'.join(add) + '\n')
        existing |= {x.split()[0] for x in add}; total += len(add)
        print(f'danglers: added {len(add)}', [x.split()[0] for x in add])
    if total: print(f'danglers: {total} address-encoded absolutes added total')

if __name__ == '__main__':
    if dedup_symbols():
        print('re-run `splat split` after dedup, then this tool again to drain danglers'); sys.exit(0)
    drain_danglers()
