#!/usr/bin/env python3
"""Guard for the region shared-source scheme.

The port compiles the game TUs listed in platform/pc/shared_tus.txt from
../../src for BOTH regions (US and JP), because those files are byte-identical
after stripping the PC gate conditionals and comments -- so every gated fix or
feature lands once and serves both regions. Same for the shared headers below.

This tool re-proves that identity. If a listed file diverges (a genuine game
edit on one side), it must be REMOVED from shared_tus.txt and compiled
per-region -- silently compiling the US copy for JP would ship wrong code.

Run from platform/pc/ (make check-shared). Exit 0 = all identical.
"""
import re
import sys
import os

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))  # platform/pc
US = os.path.join(ROOT, '..', '..')
JP = os.path.join(US, 'jp')
SHARED_LIST = os.path.join(ROOT, 'shared_tus.txt')

# Headers staged from the US tree for the JP region (all stripped-identical).
# NOT shared (region-specific): audio.h card.h units.h (JP as-is),
# field.h (hand-merged variant at include/region-jp/field.h).
SHARED_HEADERS = [
    'battle.h', 'cd_files.h', 'common.h', 'glyphs.h', 'graphics.h',
    'include_asm.h', 'inline_gte.h', 'object.h', 'state.h', 'types.h',
    'window.h',
]

# The matching builds define none of these; the port defines a region-dependent
# subset. Stripping them yields each file's "pure game code" view.
GATE = re.compile(r'\b(PERMUTER|PC_PORT(_LP64)?|PC_FEAT|PC_DEBUG_[A-Z_0-9]+)\b')


def strip_comments(text):
    out, i, n = [], 0, len(text)
    while i < n:
        c = text[i]
        if c == '/' and i + 1 < n and text[i + 1] == '*':
            j = text.find('*/', i + 2)
            i = n if j < 0 else j + 2
            out.append(' ')
        elif c == '/' and i + 1 < n and text[i + 1] == '/':
            j = text.find('\n', i)
            i = n if j < 0 else j
        elif c in '"\'':
            q, j = c, i + 1
            while j < n:
                if text[j] == '\\':
                    j += 2
                elif text[j] == q:
                    j += 1
                    break
                else:
                    j += 1
            out.append(text[i:j])
            i = j
        else:
            out.append(c)
            i += 1
    return ''.join(out)


def eval_gate_cond(directive, cond):
    """True/False if the condition is fully determined by gate macros
    (all undefined); None if it involves unknown macros (kept opaque)."""
    idents = set(re.findall(r'\b[A-Za-z_][A-Za-z_0-9]*\b', cond)) - {'defined'}
    if not idents or not any(GATE.fullmatch(x) for x in idents):
        return None
    if directive == 'ifdef':
        return False if all(GATE.fullmatch(x) for x in idents) else None
    if directive == 'ifndef':
        return True if all(GATE.fullmatch(x) for x in idents) else None

    # #if/#elif: gates are 0; try non-gate defined()/macros as both 0 and 1 --
    # if the result agrees either way, the expression is determined.
    def build(unknown_val):
        e = re.sub(r'defined\s*\(\s*(\w+)\s*\)',
                   lambda m: '0' if GATE.fullmatch(m.group(1)) else unknown_val, cond)
        e = re.sub(r'defined\s+(\w+)',
                   lambda m: '0' if GATE.fullmatch(m.group(1)) else unknown_val, e)
        e = GATE.sub('0', e)
        e = re.sub(r'\b[A-Za-z_][A-Za-z_0-9]*\b', unknown_val, e)
        e = e.replace('&&', ' and ').replace('||', ' or ').replace('!', ' not ')
        e = e.replace(' not =', ' !=')
        return e
    try:
        r0 = bool(eval(build('0'), {'__builtins__': {}}))
        r1 = bool(eval(build('1'), {'__builtins__': {}}))
        return r0 if r0 == r1 else None
    except Exception:
        return None


def stripped(path):
    text = strip_comments(open(path, encoding='utf-8', errors='replace').read())
    out = []
    stack = []  # entries: 'opaque' or [taken_any, currently_active]

    def emitting():
        return all(s == 'opaque' or s[1] for s in stack)

    for line in text.split('\n'):
        m = re.match(r'\s*#\s*(ifdef|ifndef|if|elif|else|endif)\b(.*)', line)
        if not m:
            if emitting():
                out.append(line.rstrip())
            continue
        d, rest = m.group(1), m.group(2).strip()
        if d in ('ifdef', 'ifndef', 'if'):
            if not emitting():
                stack.append([True, False])  # whole block inside skipped region
                continue
            v = eval_gate_cond(d, rest)
            if v is None:
                stack.append('opaque')
                out.append(line.rstrip())
            else:
                stack.append([v, v])
        elif d == 'elif':
            if not stack:
                continue
            s = stack[-1]
            if s == 'opaque':
                out.append(line.rstrip())
            elif s[0]:
                s[1] = False
            else:
                v = eval_gate_cond('if', rest)
                if v is None:
                    s[0] = s[1] = True
                    out.append(line.rstrip())
                else:
                    s[0] = s[1] = v
        elif d == 'else':
            if not stack:
                continue
            s = stack[-1]
            if s == 'opaque':
                out.append(line.rstrip())
            else:
                s[1] = not s[0]
                s[0] = True
        elif d == 'endif':
            if not stack:
                continue
            if stack.pop() == 'opaque':
                out.append(line.rstrip())
    # Drop blank lines, bare PC_PORT_* macro-invocation lines, and empty-body
    # #defines of PC_PORT_* macros (a gate block's #else fallback) -- all expand
    # to nothing in the matching view (e.g. object.h's PC_PORT_COORDS_ALIAS_PAD8).
    def invisible(l):
        return (re.fullmatch(r'\s*PC_PORT_[A-Z_0-9]+\s*;?\s*', l)
                or re.fullmatch(r'\s*#\s*define\s+PC_PORT_[A-Z_0-9]+\s*', l))
    return '\n'.join(l for l in out if l.strip() and not invisible(l))


def main():
    pairs = []
    with open(SHARED_LIST) as f:
        for line in f:
            line = line.strip()
            if line and not line.startswith('#'):
                pairs.append((os.path.join(US, 'src', line),
                              os.path.join(JP, 'src', line), 'src/' + line))
    for h in SHARED_HEADERS:
        pairs.append((os.path.join(US, 'include', h),
                      os.path.join(JP, 'include', h), 'include/' + h))

    bad = []
    for us_path, jp_path, name in pairs:
        if not os.path.exists(jp_path):
            bad.append((name, 'missing in jp/'))
            continue
        if stripped(us_path) != stripped(jp_path):
            bad.append((name, 'DIVERGED after gate-stripping'))
    if bad:
        print('check-shared FAILED -- these shared files are no longer identical:')
        for name, why in bad:
            print(f'  {name}: {why}')
        print('A diverged TU must move out of shared_tus.txt and compile per-region.')
        return 1
    print(f'check-shared OK: {len(pairs) - len(SHARED_HEADERS)} TUs + '
          f'{len(SHARED_HEADERS)} headers identical after gate-stripping.')
    return 0


if __name__ == '__main__':
    sys.exit(main())
