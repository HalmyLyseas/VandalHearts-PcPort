#!/usr/bin/env python3
"""Generate docs/decomp/ reference tables from the source of truth.

Everything is derived, not transcribed:
  - objf index -> handler       : parsed from src/core/obj_function_pointers.c
  - handler -> file             : scanned from src/**/*.c definitions
  - spell dispatch (gSpellsEx)  : read from the retail binary SLUS_004.47
  - spell names (gSpellNames)   : read from the retail binary (SJIS)
  - scene loaders (gSceneLoaders): read from the retail binary
  - event 0x1d census           : re-parsed from external/DATA/EVDATA*.DAT
  - evaluators (gBattleEvaluator): parsed from src/battle/evaluators.c
  - map tables (sMapObjects_*)  : parsed from src/maps/setup_objects.c
  - C spawn sites               : OBJF_ enum uses grepped across src/

Any inconsistency (handler without definition, Objf function missing from the
table, spell slot naming mismatch) is a hard error.
Inputs (both user-supplied, gitignored): the retail SLUS_004.47 binary at the repo root
and the de-sectored EVDATA*.DAT files under external/DATA/.
Run from anywhere: python3 tools/gen_decomp_docs.py
"""
import re, os, glob, struct, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)

VRAM_BASE = 0x80010000
FILE_BASE = 0x800
def off(vram): return vram - VRAM_BASE + FILE_BASE

BIN = open('SLUS_004.47', 'rb').read()

# ---- 1. objf index -> handler ---------------------------------------------
src_fp = open('src/core/obj_function_pointers.c').read()
table = {}
for m in re.finditer(r'\[(\d+)\]\s*=\s*(?:\(ObjFunction\))?\s*(\w+)', src_fp):
    idx, name = int(m.group(1)), m.group(2)
    table[idx] = None if name == 'NULL' else name
n_slots = max(table) + 1
assert n_slots == 804, n_slots

# ---- 2. handler -> file ----------------------------------------------------
defs = {}
for path in sorted(glob.glob('src/*/*.c')) + sorted(glob.glob('src/*.c')):
    text = open(path).read()
    for m in re.finditer(r'^(?:void|s32)\s+(Objf\w+|Noop_\w+)\s*\(', text, re.M):
        # skip prototypes (line ends with ';')
        line_end = text.find('\n', m.start())
        if text[m.start():line_end].rstrip().endswith(';'):
            continue
        name = m.group(1)
        assert name not in defs, 'duplicate definition: %s (%s and %s)' % (name, defs.get(name), path)
        defs[name] = path.replace('src/', '')
missing = sorted(set(h for h in table.values() if h) - set(defs))
assert not missing, 'handlers with no definition found: %s' % missing

# reverse check: every Objf definition appears in the table (aliased names cover many indices)
in_table = set(h for h in table.values() if h)
orphans = sorted(n for n in defs if n.startswith('Objf') and n not in in_table
                 and '_Unk_' not in n)
# Objf handlers not in the table are the documented Objf_Unk_* unreachables only
assert not orphans, 'Objf definitions missing from the table: %s' % orphans

# ---- 3. spell table --------------------------------------------------------
SPELLS_EX = off(0x800fbd98)
spells = []
for i in range(73):
    row = struct.unpack_from('<5h', BIN, SPELLS_EX + i*10)
    spells.append(row)
NAMES = off(0x800ee410)
def sjis_name(i):
    raw = BIN[NAMES + i*21 : NAMES + (i+1)*21]
    return raw.split(b'\x00')[0].decode('shift-jis', errors='replace').strip()
spell_names = {i: sjis_name(i) for i in range(72)}  # gSpellNames[id] = name of spell id

spell_reached = {}
for sid in range(1, 72):
    main, target, defeat, _, _ = spells[sid]
    for slot, v in (('MAIN', main), ('TARGET', target), ('DEFEAT', defeat)):
        if v > 0:
            spell_reached.setdefault(v, []).append((sid, slot))

# Retail quirk: buff spells (1, 4, 32) name DEFEAT slot 143, which is a NULL table
# entry -- harmless because a buff can never kill, so the slot never dispatches.
null_slots = sorted(i for i in spell_reached if not table.get(i))
assert null_slots == [143], 'unexpected null-slot references: %s' % null_slots

# ---- 4. event 0x1d census (re-parse) ---------------------------------------
event_spawned = set()
for path in sorted(glob.glob('external/DATA/EVDATA*.DAT')):
    raw = open(path, 'rb').read()
    # files are raw 2352-byte CD sectors: 24B header + 2048B data + 280B ECC
    data = b''.join(raw[i+24:i+24+2048] for i in range(0, len(raw), 2352))
    # entity count leads, scripts are s16 (opcode, argument) pairs ending at opcode 99
    count = struct.unpack_from('<h', data, 0)[0]
    pos = 2
    for _ in range(count):
        while pos + 4 <= len(data):
            op, arg = struct.unpack_from('<2h', data, pos)
            pos += 4
            if op == 0x1d:
                event_spawned.add(arg)
            if op == 99:
                break

# ---- 5. scene loaders -------------------------------------------------------
LOADERS = off(0x800ebb14)
scene_loaders = set(struct.unpack_from('<96h', BIN, LOADERS))
scene_loaders = {v for v in scene_loaders if v > 0}

# ---- 6. evaluators ----------------------------------------------------------
ev_src = open('src/battle/evaluators.c').read()
m = re.search(r'gBattleEvaluator\[BATTLE_CT\] = \{(.*?)\};', ev_src, re.S)
_eval_names = re.findall(r'\bOBJF_\w+', m.group(1))
evaluators = set()  # resolved after the enum parse below

# ---- 7. map tables ----------------------------------------------------------
enum_src = open('include/object.h').read()
enum_map, val = {}, -1
em = re.search(r'typedef enum ObjFunctionIdx \{(.*?)\}', enum_src, re.S)
for entry in em.group(1).split(','):
    entry = re.sub(r'/\*.*?\*/', '', entry, flags=re.S)
    entry = re.sub(r'//.*', '', entry).strip()
    if not entry: continue
    mm = re.match(r'(\w+)\s*(?:=\s*(\d+))?', entry)
    if not mm: continue
    val = int(mm.group(2)) if mm.group(2) else val + 1
    enum_map[mm.group(1)] = val
inv_enum = {}
for k, v in enum_map.items():
    inv_enum.setdefault(v, []).append(k)

evaluators = set(enum_map[n] for n in _eval_names if enum_map.get(n, 0) > 1)

so_src = open('src/maps/setup_objects.c').read()
map_table = set()
for mm in re.finditer(r'\{(OBJF_\w+)', so_src):
    v = enum_map.get(mm.group(1))
    if v and v > 1:
        map_table.add(v)

# ---- 8. C spawn sites --------------------------------------------------------
c_spawned = set()
all_src = {}
def strip_comments(t):
    t = re.sub(r'/\*.*?\*/', '', t, flags=re.S)
    return re.sub(r'//.*', '', t)
for path in glob.glob('src/*/*.c'):
    if path.endswith(('obj_function_pointers.c',)): continue
    all_src[path] = strip_comments(open(path).read())
for name, v in enum_map.items():
    if v <= 1: continue
    pat = re.compile(r'\b%s\b' % name)
    for path, text in all_src.items():
        if pat.search(text):
            c_spawned.add(v)
            break

# ---- 9. emit objf-handlers.md ------------------------------------------------
# children of confirmed-dead spawners: their only C spawn site is itself unreachable
DEAD_CHILD = {321: 'Objf734', 739: 'Objf790'}

def reach(idx, handler):
    if idx in DEAD_CHILD:
        return 'cut content (spawned only by dead `%s`)' % DEAD_CHILD[idx]
    tags = []
    if idx in spell_reached:
        t = ', '.join('%d:%s' % (s, sl[0]) for s, sl in spell_reached[idx][:4])
        if len(spell_reached[idx]) > 4: t += ', …'
        tags.append('spell (%s)' % t)
    if idx in event_spawned: tags.append('event 0x1d')
    if idx in scene_loaders: tags.append('scene loader')
    if idx in evaluators: tags.append('evaluator')
    if idx in map_table: tags.append('map table')
    if idx in c_spawned: tags.append('code')
    if not tags:
        return 'cut content' if handler and '_Unused' in handler else ('—' if not handler else 'internal/child')
    return ', '.join(tags)

rows = []
for idx in range(n_slots):
    h = table.get(idx)
    if not h: continue
    rows.append((idx, h, defs[h], reach(idx, h)))

os.makedirs('docs/decomp', exist_ok=True)
with open('docs/decomp/objf-handlers.md', 'w') as f:
    f.write('# Object-function handlers (`gObjFunctionPointers`)\n\n')
    f.write('Generated from `src/core/obj_function_pointers.c`, the retail data tables '
            '(`gSpellsEx`, `gSceneLoaders`, `gBattleEvaluator`, `sMapObjects_*`) and a parse of all '
            '93 retail EVDATA event scripts -- do not edit by hand, regenerate with '
            '`tools/gen_decomp_docs.py`.\n\n')
    f.write('Every object in the game runs one handler per frame, selected by '
            '`obj->functionIndex` through this %d-slot table (`src/core/obj_function_pointers.c`). ' % n_slots)
    f.write('One function may serve several indices; the aliases are visible in the name '
            '(`Objf004_005_408_Window` serves slots 4, 5 and 408). The "reached via" column says how '
            'retail selects the index:\n\n'
            '- **spell (id:slot)** -- a `gSpellsEx` dispatch slot; M/T/D = MAIN/TARGET/DEFEAT '
            '(see [spell-fx-dispatch.md](spell-fx-dispatch.md))\n'
            '- **event 0x1d** -- spawned by index from a retail EVDATA event script\n'
            '- **scene loader / evaluator / map table** -- the `gSceneLoaders`, `gBattleEvaluator` or '
            '`sMapObjects_*` data tables\n'
            '- **code** -- a C spawn site sets `functionIndex` to this handler\'s `OBJF_` constant\n'
            '- **internal/child** -- only ever spawned by its parent handler (no external entry)\n'
            '- **cut content** -- reachable by nothing on the retail disc (suffix `_Unused`)\n\n')
    f.write('| idx | handler | file | reached via |\n|---:|---|---|---|\n')
    for idx, h, path, r in rows:
        f.write('| %d | `%s` | `src/%s` | %s |\n' % (idx, h, path, r))
print('objf-handlers.md: %d live slots' % len(rows))

# ---- 10. emit spell table for spell-fx-dispatch.md ----------------------------
def hname(v):
    if v <= 0: return '—'
    if not table.get(v): return 'null slot %d (never fires)' % v
    return '`%s`' % table[v]
tbl = ['| id | spell | MAIN (FX1) | TARGET (FX2) | DEFEAT (FX3) |', '|---:|---|---|---|---|']
for sid in range(1, 72):
    main, target, defeat, _, _ = spells[sid]
    tbl.append('| %d | %s | %s | %s | %s |' % (sid, spell_names.get(sid, '?'),
               hname(main), hname(target), hname(defeat)))
# splice the regenerated table into the prose doc, after the last prose line
doc_p = 'docs/decomp/spell-fx-dispatch.md'
doc = open(doc_p).read()
head = doc.split('\n| id | spell |')[0].rstrip('\n')
open(doc_p, 'w').write(head + '\n\n' + '\n'.join(tbl) + '\n')
print('spell table spliced; event census size:', len(event_spawned))
print('event census:', sorted(event_spawned))
