#!/usr/bin/env python3
"""Generates platform/pc/build/generated_data.c: real C definitions for
every global variable that's `extern`-declared and used by src/*.c but
never given a defining declaration anywhere (the matching-decomp data
segment relies entirely on the linker script placing splat-extracted raw
bytes at fixed addresses -- see exchange/11-phase-c-data-segment.md).

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

PROJECT_ROOT = os.path.join(ROOT, '..', '..')
ELF = os.path.join(PROJECT_ROOT, 'build', 'SLUS_004.47.elf')
PSX_EXE = os.environ.get('VH_PSX_EXE', os.path.join(PROJECT_ROOT, 'SLUS_004.47'))
SYMBOL_ADDRS = os.path.join(PROJECT_ROOT, 'symbol_addrs.txt')
# Stage 2.3: BUILD_DIR is settable so 32- and 64-bit trees can be generated side by side
# (the safest way to A/B the -m64 flip). Defaults to 'build', i.e. unchanged behaviour.
BUILD_DIR = os.environ.get('VH_BUILD_DIR', 'build')
STAGE_DIR = f'{BUILD_DIR}/include_stage'
WORK_DIR = f'{BUILD_DIR}/data_segment_work'
OUT_C = f'{BUILD_DIR}/generated_data.c'

KNOWN_SAFE_TYPES = {'RECT', 'SVECTOR', 'VECTOR', 'CVECTOR', 'MATRIX', 'CdlLOC', 'CdlFILTER',
                     'POLY_F4', 'SPRT', 'TILE', 'DR_MODE', 's8', 's16', 's32', 'u8', 'u16', 'u32'}
KNOWN_POINTER_TYPES = {'DIRENTRY'}  # struct DIRENTRY.next -- real PsyQ header, not in our project globs

# Types/sizes the compiled probe can't resolve directly (locally-typedef'd
# in one .c file, or incomplete-array declarations) -- see the writeup for
# how each was derived.
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
SIZE_OVERRIDES = {
    'gMenuMem_TransferFrom': 2, 'gMenuMem_TransferTo': 2,
    'gMenuMem_SellingFromDepot': 12, 'gMenuMem_ShopOrDepot': 12,
    'gText': 0x2ab0,
    # Scratch/staging buffers with no authoritative size anywhere (not in
    # symbol_addrs.txt, size 0 in the ELF's own symbol table -- even the
    # original project's own tooling doesn't pin these down). Sized from
    # REAL EVIDENCE: grepped every `name + 0xNNNN` offset expression
    # against these symbols across all of src/*.c and took the largest
    # observed offset, then added a safety margin -- gap-to-next-symbol
    # (tried first) turned out unreliable, since splat emits a D_XXXXXXXX
    # label for nearly every individually-referenced byte inside these
    # regions, not just at real allocation boundaries, so "next symbol" is
    # often only 1-2 bytes away despite being the same conceptual buffer.
    # A real crash (gScratch3 sized too small at first, before this
    # evidence-based pass) confirmed a too-small guess breaks real gameplay
    # code, not just theoretically -- see exchange/12-phase-c-bootstrap.md.
    # additional_VRAM: originally sized from `additional_VRAM_END` (0x801e4690 to 0x801f7000
    # in build/SLUS_004.47.map), giving 0x12970 (76144) bytes -- but this project's OWN earlier
    # investigation (see the old version of this comment) had *already found and documented*
    # that real usage via SwapInCodeFromVram/SwapOutCodeToVram (units/load.c) extends to
    # +0x14700 (83712), a LARGER number than the linker-derived size actually applied here --
    # an oversight, not a re-derivation. Confirmed as a real, live bug via a `gdb` hardware
    # watchpoint (2026-07-11): `SwapOutCodeToVram`'s last `StoreImage` call
    # (`&additional_VRAM[0x10380]`, `gTempRect.w=60,h=144` => a 17280-byte write, ending at
    # 0x10380+0x4380=0x14700) overflows the 0x12970-byte allocation by 7568 bytes. On real
    # PS1 hardware this write safely spills past `additional_VRAM_END` into other, harmless
    # real memory -- genuine, working original game behavior, not a decomp bug. In this
    # project's own C build, the same overflow lands on whatever global our OWN compiler
    # happens to place next in memory -- confirmed via the same watchpoint to be `gClutIds`,
    # zeroing it out after it was correctly populated, which meant the sprite renderer's
    # palette lookups all faulted to black/transparent for every unit sprite. This is why
    # `additional_VRAM_END`, despite being a real linker symbol, isn't automatically safe to
    # treat as the true buffer size for a region PS1-era code writes past without an explicit
    # bounds check -- it marks a *segment* boundary in the original address space, not
    # necessarily the last byte anything actually touches. Sized to the confirmed real usage
    # instead, matching what this file's own comment history already established but never
    # applied. See exchange/12-phase-c-bootstrap.md.
    'additional_VRAM': 0x14700,
    'gScratch1_801317c0': 0x10000,  # real usage up to +0xa000
    'gScratch3_80180210': 0x80000,  # real usage up to +0x3a300 (gUnitDataPtr et al)
    'gSeqData': 0xb800,             # no offset usage found; kept at gap-to-next-symbol bound
}
UNCERTAIN_SIZE = {'gScratch1_801317c0', 'gScratch3_80180210', 'gSeqData'}

# Flagged (pointer-typed) symbols with a REAL, hand-written definition elsewhere in
# platform/pc/ (see exchange/12-phase-c-bootstrap.md Bug 11) -- skip emitting even the
# zero-init tentative definition here, or the two definitions collide at link time
# ("multiple definition"). Each entry's pointer targets were resolved against the real,
# byte-exact SLUS_004.47 binary (not guessed) and rebuilt as a real local blob + per-element
# pointer fixup; this is NOT a general mechanism for the other ~56 still-zeroed flagged
# pointers, just the two specifically root-caused and fixed so far.
MANUALLY_DEFINED = {'gBattleEnemyUnitInitialStates', 'gBattlePartyUnitInitialStates', 'gUnitAnimSets',
                     'gSpriteBoxQuads',
                     # cutscene/event unit animation-set pointer tables -- reconstructed in
                     # platform/pc/src/pc_event_anim_data.c (were NULL -> invisible cutscene units).
                     'gAnimSet_800f2db4', 'gAnimSet_80103f8c', 'gAnimSet_80103fd0',
                     'gAnimSet_80104034', 'gAnimSet_801042a8', 'gAnimSet_8010488c', 'gAnimSet_80104f18',
                     'gAnimSet_801053cc', 'gAnimSet_80105b10', 'gAnimSet_80105d00', 'gAnimSet_801063f4',
                     # event entity property tables -- reconstructed in pc_evt_entities.c (were NULL
                     # -> invisible cutscene units; see that file's header).
                     'gEvtEntities',
                     # battle spell/item info strings -- reconstructed in pc_spell_descriptions.c
                     # (were NULL -> blank spell-description window; see gen_spell_descriptions.py).
                     'gSpellDescriptions',
                     # item/equipment info strings -- reconstructed in pc_item_descriptions.c (were
                     # NULL -> blank overworld/shop item-description window; see gen_item_descriptions.py).
                     # NOTE: gTextPointers (0x8012be9c) is runtime-filled (.bss, all-NULL in ROM) -- NOT here.
                     'gItemDescriptions', 'gItemDescriptions2',
                     # Stage-3 1.3 Tactical Mode flag -- defined in platform/pc/src/pc_balance.c, referenced
                     # by the gated src/ hooks (states/game_setup.c etc.); not a ROM data symbol.
                     'gTacticalMode'}


def sh(cmd, **kw):
    return subprocess.run(cmd, capture_output=True, text=True, **kw)


def strip_comments(text):
    text = re.sub(r'/\*.*?\*/', lambda m: '\n' * m.group(0).count('\n'), text, flags=re.S)
    return re.sub(r'//[^\n]*', '', text)


# ---- TARGET WIDTH -----------------------------------------------------------------------
# ⚠️ This MUST match the width the real build uses. The sizeof() probe further down measures
# struct sizes in order to slice the data segment out of the ELF, and EVERY struct containing a
# pointer changes size between -m32 and -m64. Probing at the wrong width does not fail loudly --
# it produces a generated_data.c whose objects are the wrong size, i.e. silent data corruption.
# So the Makefile passes its own $(M32) through VH_TARGET_MARCH rather than this file guessing.
# Default '-m32' keeps today's behaviour byte-for-byte.
# (see the Makefile's own M32 comment / exchange/12-phase-c-bootstrap.md)
# VH_TARGET_MARCH: '-m32' / '-m64' / etc. An EMPTY value means "no -m flag" (native width, e.g. the
# CMake 64-bit build) -- must become an empty list, NOT [''], or the empty string is passed to the
# compiler as a bogus input filename and the probe link fails before any undefined ref is seen.
# Default is now '' (native width, no -m flag) rather than '-m32'. The Makefile always sets
# VH_TARGET_MARCH explicitly so it is unaffected; CMake omits it entirely (passing an empty
# `VH_TARGET_MARCH=` through `cmake -E env` makes it treat the next token as the command -> a
# "permission denied" that silently stopped the generator from ever running under CMake).
_march = os.environ.get('VH_TARGET_MARCH', '')
# Historically this was one flag (-m32/-m64). Cross-architecture macOS builds need the two-token
# spelling `-arch x86_64` (and Universal 2 needs it twice), so parse it as a normal argument string.
M32 = shlex.split(_march)
_TARGET_IS_32 = (M32 == ['-m32'])

# Build-system-agnostic hooks (Stage 2.4 CMake / cross-compile). All optional; when unset the
# Makefile's original glob/pkg-config/gcc behaviour is preserved exactly.
#   VH_CC        - host compiler for the probe links (default 'gcc'); MinGW/clang override it.
#   VH_OBJ_FILES - explicit newline/space-separated object list for the undefined-symbol probe.
#                  CMake passes $<TARGET_OBJECTS:...> here because its object layout
#                  (CMakeFiles/*.dir/....c.o) does not match the Makefile's build/src/*.o glob.
#   VH_LINK_LIBS - explicit link libraries for the probe (e.g. from CMake's find_package); when
#                  unset, pkg-config sdl2+openal + -lGL -lm as before.
#   VH_EXTRA_CFLAGS - extra flags for the sizeof-PROBE compile (below). Windows uses it to
#                  -include the MinGW compat shim (pc_win_compat.h) so the staged PsyQ headers
#                  compile in the probe the same way they do in the real build.
#   VH_HOST_CC   - compiler for the sizeof PROBE specifically. The probe is compiled AND RUN to read
#                  sizeof() values, so under cross-compilation it must target the build HOST, not
#                  VH_CC's target -- a Windows/ARM probe binary cannot execute on the Linux host.
#                  "safe" symbols (the only ones probed) are pointer-free, and on x86-64 host vs
#                  target differ only in `long` (mapped to int in the PC layer) -- so host sizeof ==
#                  target sizeof for them; a worst-case mismatch only over-reserves storage, never
#                  under. Defaults to VH_CC when unset, so native builds are byte-for-byte unchanged.
CC_CMD = os.environ.get('VH_CC', 'gcc').split()
OBJ_FILES_ENV = os.environ.get('VH_OBJ_FILES', '').split()
LINK_LIBS_ENV = os.environ.get('VH_LINK_LIBS', '').split()
EXTRA_CFLAGS = os.environ.get('VH_EXTRA_CFLAGS', '').split()
HOST_CC = os.environ.get('VH_HOST_CC', '').split() or CC_CMD

# Sanitizer flags, forwarded from the Makefile's $(SAN). find_undefined_symbols() below LINKS the
# real build's objects; if those were compiled with -fsanitize=... the link fails on undefined
# __asan_* symbols unless the same flag is passed here. That failure is not obvious from the
# output -- link_ok goes False and the generator falls back to its slower path -- so forward it.
# The sizeof() probe further down does NOT need it (it compiles its own standalone TU) but gets
# it anyway for consistency; sanitizers do not change struct layout.
SAN = os.environ.get('VH_SAN', '').split()

# pkg-config must resolve libraries for the same width. The 32-bit path needs the explicit
# lib32 pkgconfig dir; for any other width the system default is already correct.
PKG_CONFIG_32_ENV = ({**os.environ, 'PKG_CONFIG_LIBDIR': '/usr/lib32/pkgconfig', 'PKG_CONFIG_PATH': ''}
                     if _TARGET_IS_32 else dict(os.environ))


def find_undefined_symbols():
    # WORK_DIR must exist BEFORE the probe link below, which writes -o {WORK_DIR}/symprobe.
    # It used to be created only in the later sizeof-probe step, which meant that on a genuinely
    # fresh BUILD_DIR the link failed with "No such file or directory" -- and since that error
    # carries no "undefined reference" lines, the regex below matched nothing and the generator
    # cheerfully emitted an EMPTY generated_data.c (909 bytes), producing a link full of undefined
    # references to symbols it is supposed to define. It never surfaced because build/ and build32/
    # had long since had the directory created; `rm -rf build_asan` was the first truly clean run.
    os.makedirs(WORK_DIR, exist_ok=True)
    # BUILD_DIR-relative, NOT hardcoded 'build/'. When this was hardcoded, a
    # `make link M32=-m32 BUILD_DIR=build32` linked the 64-bit objects out of build/ with -m32
    # flags: the link failed on the arch mismatch AND gcc truncated its -o target, silently
    # deleting the working 64-bit binary. The symbol discovery still "worked" only because
    # undefined-symbol NAMES do not depend on width.
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
    # Only the undefined-symbol NAMES matter here, and a normal probe intentionally fails with those
    # names. A failure that yields no names is different: a missing library, bad output path, or
    # incompatible object can stop ld before symbol discovery. Continuing would emit an empty data
    # segment and defer the real error to a huge, misleading final-link failure.
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
    files = glob.glob('../../include/*.h') + glob.glob('../../src/*.c') + glob.glob('../../src/*/*.c')
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
    files = glob.glob('../../include/*.h') + glob.glob('../../src/*.c') + glob.glob('../../src/*/*.c')
    text_cache = {f: strip_comments(open(f, encoding='latin1').read()) for f in files}
    pointer_types = set()
    for t in type_names:
        if t in KNOWN_SAFE_TYPES:
            continue
        body = None
        for f, text in text_cache.items():
            # [^{}]*, not .* with re.S: a non-greedy DOTALL body regex doesn't
            # stop at the *nearest* closing brace, it hunts for the next
            # literal "} TypeName;" anywhere in the file -- so an earlier,
            # unrelated typedef struct (e.g. UnitStatus, which genuinely has
            # pointer fields) gets swallowed whole into the match and its
            # pointer fields get misattributed to a later, actually-flat
            # struct (e.g. UnitInfo) sharing the same header. Excluding brace
            # characters from the body class forces each candidate starting
            # position to close at its own struct's brace, so the match only
            # succeeds where the immediately-following name is the real one.
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
        r = sh([*HOST_CC, *M32, *SAN, *EXTRA_CFLAGS, '-std=gnu89', '-DPERMUTER', f'-I{STAGE_DIR}', '-Iinclude', '-I../..',
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

    # Write-if-changed: this runs on every `make link` (the rule hangs off the phony `default`
    # so the sizeof-probe always re-checks the current tree), but when the emitted text is
    # identical, rewriting would only bump the timestamp -- forcing a pointless recompile of
    # this multi-MB translation unit plus a relink on every no-op build. Content equality is a
    # complete staleness check: every input that matters (ELF bytes, target width, sanitizer
    # env, this script's own logic) changes the generated text.
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
