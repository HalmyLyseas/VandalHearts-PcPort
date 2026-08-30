#!/usr/bin/env python3
"""Generates platform/pc/build/generated_data.c: real C definitions for
every global variable that's `extern`-declared and used by src/*.c but
never given a defining declaration anywhere (the matching-decomp data
segment relies entirely on the linker script placing splat-extracted raw
bytes at fixed addresses).

Pipeline (all in this one script so the whole thing is reproducible from a
single `make` invocation, not a one-off interactive exploration):
  1. Link what's already built; collect "undefined reference" symbol names.
  2. Find each symbol's own `extern` declaration (project headers + src/*.c
     for file-local ones) and classify it as pointer-typed (or a struct/
     union with a pointer member) vs plain value -- pointer bytes can't be
     blindly copied from the 32-bit MIPS build onto a 64-bit host.
  3. For plain-value symbols, get a real sizeof() via a compiled probe
     using the actual project headers.
  4. Extract real bytes from the byte-exact build/SLUS_004.47.elf at each
     symbol's VRAM address (mapped to a file offset via the ELF's own
     PROGBITS section headers) and emit a real typed definition,
     initialized via a constructor-run memcpy.
  5. Pointer-typed/pointer-containing symbols get a plain zero-initialized
     tentative definition instead, flagged with a comment.

Run from platform/pc/ after `make default` has produced build/src/*.o and
the backend build/*.o files.
"""
import glob
import json
import os
import re
import shlex
import shutil
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))  # platform/pc
os.chdir(ROOT)

# VH_GAME_ROOT is the game tree this build compiles (US repo root by default, jp/ for
# REGION=jp); VH_PSX_BASENAME names its byte-exact executable.
PROJECT_ROOT = os.environ.get('VH_GAME_ROOT', os.path.join(ROOT, '..', '..'))
PSX_BASENAME = os.environ.get('VH_PSX_BASENAME', 'SLUS_004.47')
ELF = os.path.join(PROJECT_ROOT, 'build', PSX_BASENAME + '.elf')
PSX_EXE = os.environ.get('VH_PSX_EXE', os.path.join(PROJECT_ROOT, PSX_BASENAME))
SYMBOL_ADDRS = os.path.join(PROJECT_ROOT, 'symbol_addrs.txt')
# BUILD_DIR is settable so 32- and 64-bit trees can be generated side by side.
BUILD_DIR = os.environ.get('VH_BUILD_DIR', 'build')
STAGE_DIR = f'{BUILD_DIR}/include_stage'
WORK_DIR = f'{BUILD_DIR}/data_segment_work'
OUT_C = f'{BUILD_DIR}/generated_data.c'

KNOWN_SAFE_TYPES = {'RECT', 'SVECTOR', 'VECTOR', 'CVECTOR', 'MATRIX', 'CdlLOC', 'CdlFILTER',
                     'POLY_F4', 'SPRT', 'TILE', 'DR_MODE', 's8', 's16', 's32', 'u8', 'u16', 'u32'}
KNOWN_POINTER_TYPES = {'DIRENTRY'}  # struct DIRENTRY.next -- real PsyQ header, not in our project globs

# Types the compiled probe can't see because they are typedef'd locally in one .c file.
LOCAL_TYPEDEFS = '''\
typedef struct MenuMem1 { u8 ofs; u8 top; } MenuMem1;
typedef struct MenuMem2 { s16 ofs; s16 top; } MenuMem2;
typedef struct EvtEntityProperties {
   u8 **altAnims;
   u8 **baseAnims;
   u8 stripIdxA, stripIdxB;
   u16 padding;
} EvtEntityProperties;
'''
# Buffers with no authoritative size (absent from symbol_addrs.txt, size 0 in the ELF symbol
# table), sized from the largest `name + 0xNNNN` offset used in src/ plus a margin. The same
# values hold for both regions. See docs/pc-port/data-segment.md, "Size overrides".
SIZE_OVERRIDES = {
    'gMenuMem_TransferFrom': 2, 'gMenuMem_TransferTo': 2,
    'gMenuMem_SellingFromDepot': 12, 'gMenuMem_ShopOrDepot': 12,
    'gText': 0x2ab0,
    # additional_VRAM_END gives only 0x12970, but SwapOutCodeToVram's last StoreImage writes
    # 17280 bytes at &additional_VRAM[0x10380]; a shorter buffer lets it overrun gClutIds.
    'additional_VRAM': 0x14700,
    'gScratch1_801317c0': 0x10000,  # real usage up to +0xa000
    'gScratch3_80180210': 0x80000,  # real usage up to +0x3a300 (gUnitDataPtr et al)
    'gSeqData': 0xb800,             # no offset usage found; kept at gap-to-next-symbol bound
}
UNCERTAIN_SIZE = {'gScratch1_801317c0', 'gScratch3_80180210', 'gSeqData'}

# Flagged (pointer-typed) symbols with a real definition elsewhere in platform/pc/: emitting
# even a zero-init tentative definition here would collide at link time.
# See docs/pc-port/data-segment.md, "Manually defined symbols".
MANUALLY_DEFINED = {'gBattleEnemyUnitInitialStates', 'gBattlePartyUnitInitialStates', 'gUnitAnimSets',
                     'gSpriteBoxQuads',
                     # cutscene/event unit animation-set pointer tables (pc_event_anim_data.c)
                     'gAnimSet_800f2db4', 'gAnimSet_80103f8c', 'gAnimSet_80103fd0',
                     'gAnimSet_80104034', 'gAnimSet_801042a8', 'gAnimSet_8010488c', 'gAnimSet_80104f18',
                     'gAnimSet_801053cc', 'gAnimSet_80105b10', 'gAnimSet_80105d00', 'gAnimSet_801063f4',
                     # event entity property tables (pc_evt_entities.c)
                     'gEvtEntities',
                     # battle spell/item info strings (pc_spell_descriptions.c)
                     'gSpellDescriptions',
                     # item/equipment info strings (pc_item_descriptions.c). gTextPointers
                     # (0x8012be9c) is runtime-filled .bss, all-NULL in ROM, and stays out of this set.
                     'gItemDescriptions', 'gItemDescriptions2',
                     # Tactical Mode flag defined in pc_balance.c and referenced by gated src/ hooks;
                     # not a ROM data symbol.
                     'gTacticalMode'}


def sh(cmd, **kw):
    return subprocess.run(cmd, capture_output=True, text=True, **kw)


def strip_comments(text):
    text = re.sub(r'/\*.*?\*/', lambda m: '\n' * m.group(0).count('\n'), text, flags=re.S)
    return re.sub(r'//[^\n]*', '', text)


# Target width. The sizeof() probe must run at the width the real build uses: every struct with a
# pointer member changes size between -m32 and -m64, and a wrong-width probe corrupts silently.
# See docs/pc-port/data-segment.md, "The width and cross-compile traps".
_march = os.environ.get('VH_TARGET_MARCH', '')
# An empty value means native width and must become [] rather than [''] (an empty string is a
# bogus input filename). Parsed as an argument string so `-arch x86_64` works too.
M32 = shlex.split(_march)
_TARGET_IS_32 = (M32 == ['-m32'])

# Build-system-agnostic hooks, all optional (VH_CC, VH_OBJ_FILES, VH_LINK_LIBS, VH_EXTRA_CFLAGS,
# VH_HOST_CC). The sizeof probe is compiled AND RUN, so under cross-compilation VH_HOST_CC must
# target the build host. See docs/pc-port/data-segment.md, "Driver hooks and their pitfalls".
CC_CMD = os.environ.get('VH_CC', 'gcc').split()
OBJ_FILES_ENV = os.environ.get('VH_OBJ_FILES', '').split()
LINK_LIBS_ENV = os.environ.get('VH_LINK_LIBS', '').split()
EXTRA_CFLAGS = os.environ.get('VH_EXTRA_CFLAGS', '').split()
HOST_CC = os.environ.get('VH_HOST_CC', '').split() or CC_CMD

# Sanitizer flags from the build. find_undefined_symbols() links the real build's objects, and
# objects compiled with -fsanitize=... need the same flag or the link fails on __asan_* symbols
# without any visible error. Sanitizers do not change struct layout, so the probe is unaffected.
SAN = os.environ.get('VH_SAN', '').split()

# pkg-config must resolve libraries for the same width. The 32-bit path needs the explicit
# lib32 pkgconfig dir; for any other width the system default is already correct.
PKG_CONFIG_32_ENV = ({**os.environ, 'PKG_CONFIG_LIBDIR': '/usr/lib32/pkgconfig', 'PKG_CONFIG_PATH': ''}
                     if _TARGET_IS_32 else dict(os.environ))


def find_undefined_symbols():
    # WORK_DIR must exist before the probe link writes -o {WORK_DIR}/symprobe: a "No such file"
    # failure carries no "undefined reference" lines and would yield an empty generated_data.c.
    os.makedirs(WORK_DIR, exist_ok=True)
    # Object paths are BUILD_DIR-relative: linking another tree's objects at this width fails on
    # the arch mismatch and gcc truncates its -o target, deleting that tree's working binary.
    if OBJ_FILES_ENV:
        # CMake (or any non-Makefile driver) passed the object list explicitly.
        objs = OBJ_FILES_ENV
    else:
        # Makefile layout: game objs by glob, backend objs by fixed name.
        objs = glob.glob(f'{BUILD_DIR}/src/*.o') + glob.glob(f'{BUILD_DIR}/src/*/*.o') + [
            f'{BUILD_DIR}/{o}' for o in
            ('libetc.o', 'libcd.o', 'libsnd.o', 'libspu.o', 'libkernel.o',
             'libgte.o', 'libgpu.o', 'pc_gpu_window.o', 'libsn.o')]
    if LINK_LIBS_ENV:
        libs = LINK_LIBS_ENV
    else:
        libs = (sh(['pkg-config', '--libs', 'sdl2'], env=PKG_CONFIG_32_ENV).stdout.split()
                + sh(['pkg-config', '--libs', 'openal'], env=PKG_CONFIG_32_ENV).stdout.split()
                + ['-lGL', '-lm'])
    # Only the undefined-symbol names matter; the probe link is expected to fail with them. A
    # failure yielding no names (missing library, bad output path, incompatible object) is fatal:
    # continuing would emit an empty data segment and defer the error to the final link.
    r = sh([*CC_CMD, *M32, *SAN, *objs, *libs,
            '-o', f'{WORK_DIR}/symprobe'])   # scratch target -- never the real binary
    link_output = r.stderr + '\n' + r.stdout
    names = set()
    for m in re.finditer(r"undefined reference to [`']([A-Za-z_]\w*)'", link_output):
        names.add(m.group(1))
    # Apple ld groups undefined symbols as: "_symbol", referenced from:. Mach-O's leading
    # underscore is linker decoration, not part of the C identifier.
    for m in re.finditer(r'^\s+"_([A-Za-z_]\w*)", referenced from:', link_output, re.MULTILINE):
        names.add(m.group(1))
    if r.returncode != 0 and not names:
        raise SystemExit("probe link failed before yielding undefined symbols:\n" +
                         "\n".join(link_output.splitlines()[:20]))
    return sorted(names), r.returncode == 0


def find_declarations(syms):
    # Headers come from the stage, which is what the build and the sizeof probe compile against;
    # for REGION=jp it substitutes gate-carrying headers whose array geometries differ from the
    # raw jp/ copies (e.g. field.h's PERMUTER-widened gTravelAscentCost[20][20] vs [14][20]).
    files = (glob.glob(f'{STAGE_DIR}/*.h') + glob.glob(f'{PROJECT_ROOT}/src/*.c')
             + glob.glob(f'{PROJECT_ROOT}/src/*/*.c'))
    text_cache = {f: strip_comments(open(f, encoding='latin1').read()) for f in files}
    simple_re_t = r'extern\s+[^;{{]*\b({sym})\b[^;{{]*;'
    anon_re_t = r'extern\s+(?:struct|union)\s*\{{[^}}]*\}}\s*({sym})\s*(\[[^\]]*\])?\s*;'
    found, missing = {}, []
    for s in syms:
        pat1 = re.compile(simple_re_t.format(sym=re.escape(s)))
        pat2 = re.compile(anon_re_t.format(sym=re.escape(s)), re.S)
        hit = None
        for f, text in text_cache.items():
            m = pat1.search(text) or pat2.search(text)
            if m:
                hit = (f, m.group(0).strip())
                break
        if hit:
            found[s] = {'file': hit[0], 'decl': hit[1]}
        else:
            missing.append(s)
    return found, missing


def classify_pointer(decl_text):
    d = re.sub(r'^extern\s+', '', decl_text.strip()).rstrip(';').strip()
    m = re.match(r'(?:struct|union)\s*\{.*\}\s*(\**)\s*[A-Za-z_]\w*\s*(\[.*\])?$', d, re.S)
    if m:
        return 'pointer' if m.group(1) else 'value'
    if re.search(r'\(\s*\*\s*[A-Za-z_]\w*\s*\)', d):
        return 'pointer'
    idents = list(re.finditer(r'[A-Za-z_]\w*', d))
    if not idents:
        return 'unknown'
    before = d[:idents[-1].start()]
    return 'pointer' if '*' in before else 'value'


def extract_base_type(decl_text):
    d = re.sub(r'^extern\s+', '', decl_text.strip()).rstrip(';').strip()
    if d.startswith('struct') or d.startswith('union'):
        m = re.match(r'((?:struct|union)\s*\{.*\})\s*[A-Za-z_]', d, re.S)
        if m:
            return 'anon', m.group(1)
    d2 = re.sub(r'^(const\s+|volatile\s+|struct\s+|union\s+)*', '', d)
    m = re.match(r'([A-Za-z_]\w*)', d2)
    return ('named', m.group(1)) if m else (None, None)


def find_struct_pointer_fields(type_names):
    # Same staged-header rule as find_declarations().
    files = (glob.glob(f'{STAGE_DIR}/*.h') + glob.glob(f'{PROJECT_ROOT}/src/*.c')
             + glob.glob(f'{PROJECT_ROOT}/src/*/*.c'))
    text_cache = {f: strip_comments(open(f, encoding='latin1').read()) for f in files}
    pointer_types = set()
    for t in type_names:
        if t in KNOWN_SAFE_TYPES:
            continue
        body = None
        for f, text in text_cache.items():
            # [^{}]* rather than a non-greedy DOTALL .*: the latter hunts for the next literal
            # "} TypeName;" anywhere in the file, swallowing an earlier unrelated struct (e.g.
            # UnitStatus) and misattributing its pointer fields to a flat one (e.g. UnitInfo).
            m = (re.search(r'typedef\s+(?:struct|union)(?:\s+\w+)?\s*\{([^{}]*)\}\s*' + re.escape(t) + r'\s*;', text, re.S)
                 or re.search(r'\b(?:struct|union)\s+' + re.escape(t) + r'\s*\{([^{}]*)\};', text, re.S))
            if m:
                body = m.group(1)
                break
        if body is None:
            continue  # alias typedef (e.g. `typedef u8 PathGridRow[65]`) or genuinely not found -- treated as safe unless known otherwise
        fields = [ln.strip() for ln in body.split(';')]
        if any('*' in ln for ln in fields):
            pointer_types.add(t)
    return pointer_types


def classify_all(decls):
    results = {}
    for s, info in decls.items():
        cls = classify_pointer(info['decl'])
        kind, base = extract_base_type(info['decl'])
        results[s] = {**info, 'class': cls, 'base_type': base, 'base_type_kind': kind}

    composite_types = sorted(set(r['base_type'] for r in results.values()
                                  if r['class'] == 'value' and r['base_type_kind'] == 'named'
                                  and r['base_type'] not in KNOWN_SAFE_TYPES))
    pointer_types = find_struct_pointer_fields(composite_types) | KNOWN_POINTER_TYPES

    for s, r in results.items():
        if r['class'] == 'pointer':
            r['route'], r['reason'] = 'flagged', 'top-level pointer type'
        elif r.get('base_type') in pointer_types:
            r['route'], r['reason'] = 'flagged', f"base type {r['base_type']} contains pointer field(s)"
        else:
            r['route'], r['reason'] = 'safe', ''
    return results


def run_sizeof_probe(results):
    os.makedirs(WORK_DIR, exist_ok=True)
    safe = [s for s, r in results.items() if r['route'] == 'safe']
    skip = set()
    headers = sorted(f for f in os.listdir(STAGE_DIR) if f.endswith('.h') and f != 'pc_forward_decls.h')

    def gen_probe(path):
        syms = [s for s in safe if s not in skip]
        with open(path, 'w') as out:
            out.write('#include <stdio.h>\n')
            for h in headers:
                out.write(f'#include "{h}"\n')
            out.write(LOCAL_TYPEDEFS)
            for s in syms:
                out.write(results[s]['decl'] + '\n')
            out.write('int main(void) {\n')
            for s in syms:
                out.write(f'  printf("{s}|%zu\\n", sizeof({s}));\n')
            out.write('  return 0;\n}\n')
        return syms

    probe_c = f'{WORK_DIR}/probe.c'
    probe_bin = f'{WORK_DIR}/probe'
    for _ in range(15):
        syms = gen_probe(probe_c)
        r = sh([*HOST_CC, *M32, *SAN, *EXTRA_CFLAGS, '-std=gnu89', '-DPERMUTER', f'-I{STAGE_DIR}', '-Iinclude', f'-I{PROJECT_ROOT}',
                '-include', 'pc_forward_decls.h', probe_c, '-o', probe_bin])
        errors = [l for l in r.stderr.splitlines() if ': error:' in l]
        if not errors:
            break
        probe_lines = open(probe_c).readlines()
        newly = set()
        for e in errors:
            m = re.match(r'[^:]+:(\d+):', e)
            if not m:
                continue
            ln = int(m.group(1)) - 1
            if ln < 0 or ln >= len(probe_lines):
                continue    # error reported in an included header, not probe.c -- not a symbol line
            line = probe_lines[ln]
            sm = re.search(r'printf\("([A-Za-z_]\w*)\|', line) or re.search(r'extern\s+[^;]*?\b([A-Za-z_]\w*)\s*(\[[^\]]*\])?\s*;', line)
            if sm:
                newly.add(sm.group(1))
        if not newly:
            print("probe: could not resolve remaining errors:\n" + '\n'.join(errors[:5]), file=sys.stderr)
            break
        skip |= newly
    else:
        print("probe: did not converge", file=sys.stderr)

    sizes = {}
    out = sh([probe_bin]).stdout if os.path.exists(probe_bin) else ''
    for line in out.splitlines():
        name, size = line.split('|')
        sizes[name] = int(size)
    return sizes, skip


SECTIONS = None  # filled from ELF readelf output or the PS-X EXE header
DATA_IMAGE = ELF


def load_sections():
    global SECTIONS, DATA_IMAGE
    readelf = os.environ.get('VH_READELF', 'mipsel-linux-gnu-readelf')
    if os.path.exists(ELF) and shutil.which(readelf):
        out = sh([readelf, '-S', ELF]).stdout
        sections = []
        for m in re.finditer(r'PROGBITS\s+([0-9a-fA-F]+)\s+([0-9a-fA-F]+)\s+([0-9a-fA-F]+)', out):
            addr, off, size = (int(x, 16) for x in m.groups())
            sections.append((addr, off, size))
        if sections:
            SECTIONS = sections
            DATA_IMAGE = ELF
            return
    with open(PSX_EXE, 'rb') as f:
        hdr = f.read(0x800)
    if not hdr.startswith(b'PS-X EXE') or len(hdr) < 0x20:
        raise SystemExit(f'not a PS-X EXE: {PSX_EXE}')
    load_addr = int.from_bytes(hdr[0x18:0x1c], 'little')
    load_size = int.from_bytes(hdr[0x1c:0x20], 'little')
    SECTIONS = [(load_addr, 0x800, load_size)]
    DATA_IMAGE = PSX_EXE


def vram_to_file_offset(vram):
    for addr, off, size in SECTIONS:
        if addr <= vram < addr + size:
            return off + (vram - addr)
    return None


def load_vram_addrs():
    addrs = {}
    for line in open(SYMBOL_ADDRS):
        m = re.match(r'([A-Za-z_]\w*)\s*=\s*(0x[0-9a-fA-F]+)\s*;', line)
        if m:
            addrs[m.group(1)] = int(m.group(2), 16)
    nm = os.environ.get('VH_NM', 'mipsel-linux-gnu-nm')
    if os.path.exists(ELF) and shutil.which(nm):
        nm_out = sh([nm, ELF]).stdout
        for line in nm_out.splitlines():
            parts = line.split()
            if len(parts) == 3:
                addr_hex, _, name = parts
                name = name.split('.')[0]
                if name not in addrs:
                    try:
                        addrs[name] = int(addr_hex, 16)
                    except ValueError:
                        pass
    return addrs


def generate(results, sizes, unresolved_by_probe):
    vram_addrs = load_vram_addrs()
    load_sections()
    with open(DATA_IMAGE, 'rb') as f:
        elf_bytes = f.read()

    headers = sorted(f for f in os.listdir(STAGE_DIR) if f.endswith('.h') and f != 'pc_forward_decls.h')
    out = [
        '/* GENERATED by platform/pc/tools/build_data_segment.py -- see',
        ' * exchange/11-phase-c-data-segment.md. Do not hand-edit; rerun',
        ' * `make gen-data` instead. */',
        '#include <string.h>',
    ]
    out += [f'#include "{h}"' for h in headers]
    out.append('#include "PsyQ/kernel.h" /* struct DIRENTRY -- only ever included by src/core/card.c, not by any header */')
    out.append(LOCAL_TYPEDEFS)

    stats = {'safe_extracted': 0, 'safe_unresolved': 0, 'flagged': 0}
    emitted_decls = set()
    ctor_lines = []

    safe = sorted(s for s, r in results.items() if r['route'] == 'safe')
    flagged = sorted(s for s, r in results.items() if r['route'] == 'flagged')

    for s in safe:
        if s in MANUALLY_DEFINED:
            out.append(f'/* {s}: real definition in a platform/pc backend -- not emitted here */')
            out.append('')
            continue
        decl = results[s]['decl']
        size = SIZE_OVERRIDES.get(s, sizes.get(s))
        def_decl = re.sub(r'^extern\s+', '', decl)
        if size is None:
            out.append(f'/* {s}: no size resolvable (probe failed and no override) -- SKIPPED, still undefined */')
            stats['safe_unresolved'] += 1
            continue
        if re.search(r'\[\s*\]', def_decl):
            def_decl = re.sub(r'\[\s*\]', f'[{size}]', def_decl, count=1)
        if s == 'gSeqData':
            # SsSeqOpen has no length argument (retail API); this is the only place that
            # knows the size actually given to the buffer, so hand it to libsnd.c as a bound.
            out.append(f'const unsigned int PC_GenSize_gSeqData = {size};')
        if s in UNCERTAIN_SIZE:
            out.append(f'/* {s}: size is gap-to-next-known-symbol, NOT authoritative -- TODO verify at runtime */')
        vram = vram_addrs.get(s)
        off = vram_to_file_offset(vram) if vram is not None else None
        if off is None:
            if decl not in emitted_decls:
                out.append(def_decl)
                emitted_decls.add(decl)
            out.append(f'/* ^ {s}: no VRAM/file-offset resolvable -- zero-initialized, not extracted */')
            stats['safe_unresolved'] += 1
            continue
        raw = elf_bytes[off:off + size]
        if len(raw) != size:
            if decl not in emitted_decls:
                out.append(def_decl)
                emitted_decls.add(decl)
            out.append(f'/* ^ {s}: short read from ELF -- zero-initialized */')
            stats['safe_unresolved'] += 1
            continue
        if decl not in emitted_decls:
            out.append(def_decl)
            emitted_decls.add(decl)
        else:
            out.append(f'/* (declaration for {s} shares a line with another already-emitted symbol) */')
        arr = f'_init_bytes_{s}'
        out.append(f'static const unsigned char {arr}[{size}] = {{{",".join(str(b) for b in raw)}}};')
        ctor_lines.append(
            f'__attribute__((constructor)) static void _init_{s}(void) {{ '
            f'memcpy((void*)&{s}, {arr}, sizeof({arr}) < sizeof({s}) ? sizeof({arr}) : sizeof({s})); }}')
        stats['safe_extracted'] += 1
        out.append('')

    out.append('/* ---- flagged (pointer-containing) symbols: zero-init only ---- */')
    for s in flagged:
        if s in MANUALLY_DEFINED:
            out.append(f'/* {s}: real definition in platform/pc/src/pc_battle_data.c or '
                        'pc_unit_anim_data.c -- not emitted here */')
            out.append('')
            stats['flagged'] += 1
            continue
        decl = results[s]['decl']
        def_decl = re.sub(r'^extern\s+', '', decl)
        out.append(f"/* {s}: {results[s]['reason']} -- zero-initialized, needs manual review */")
        if decl not in emitted_decls:
            out.append(def_decl)
            emitted_decls.add(decl)
        else:
            out.append(f'/* (declaration for {s} shares a line with another already-emitted symbol) */')
        stats['flagged'] += 1
        out.append('')

    out.append('')
    out += ctor_lines
    out.append('')

    # Write-if-changed: this runs on every `make link`, and rewriting identical text would only
    # bump the timestamp and force a recompile of this multi-MB unit plus a relink. Content
    # equality is a complete staleness check: every input changes the emitted text.
    new_text = '\n'.join(out) + '\n'
    try:
        with open(OUT_C) as f:
            stats['unchanged'] = (f.read() == new_text)
    except OSError:
        stats['unchanged'] = False
    if not stats['unchanged']:
        with open(OUT_C, 'w') as f:
            f.write(new_text)
    return stats


def main():
    print("1. Linking to find undefined symbols...")
    syms, link_ok = find_undefined_symbols()
    if link_ok:
        print("Link already succeeds -- nothing to generate.")
        return
    print(f"   {len(syms)} undefined symbols")

    print("2. Finding declarations...")
    decls, missing = find_declarations(syms)
    if missing:
        print(f"   WARNING: {len(missing)} symbols have no findable extern declaration: {missing}", file=sys.stderr)

    print("3. Classifying (pointer vs value)...")
    results = classify_all(decls)
    n_safe = sum(1 for r in results.values() if r['route'] == 'safe')
    n_flagged = sum(1 for r in results.values() if r['route'] == 'flagged')
    print(f"   safe: {n_safe}  flagged: {n_flagged}")

    print("4. Probing real sizeof() for safe symbols...")
    sizes, unresolved = run_sizeof_probe(results)
    print(f"   resolved {len(sizes)} sizes via compiler probe, {len(unresolved)} need overrides/fallback")

    print(f"5. Extracting real bytes and generating {OUT_C}...")
    stats = generate(results, sizes, unresolved)
    print(f"   {stats}")
    if stats.get('unchanged'):
        print("   output identical to existing file -- timestamp preserved (no recompile)")

    with open(f'{WORK_DIR}/classified.json', 'w') as f:
        json.dump(results, f, indent=1)


if __name__ == '__main__':
    main()
