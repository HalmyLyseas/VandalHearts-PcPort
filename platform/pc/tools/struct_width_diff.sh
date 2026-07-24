#!/bin/sh
# Reports every project struct whose sizeof() differs between the 32- and 64-bit builds.
#
# This is the check neither sanitizer can do. ASAN and UBSan find accesses that are out of bounds
# in the build being run; they cannot see a struct that simply CHANGES SIZE at LP64 and thereby
# overflows a fixed byte-size buffer somewhere else. That class cost a full debugging session
# (exchange/bugs/bugreport-07.md Bug B: UnitStatus 120 -> 136 grew CardFileData_InBattleSave past
# the 0x1700 card buffer, so the checksum never matched and in-battle saves would not load).
#
# Needs both builds present:
#   make link                              # build/    (64-bit)
#   make link M32=-m32 BUILD_DIR=build32   # build32/  (32-bit)
#
# Reading the output: a size difference is NOT automatically a bug. Anything holding a pointer
# legitimately grows. It becomes a bug only where the struct meets a FIXED byte size --
# serialization to a save file, a memcpy with a literal length, or a preallocated buffer. Triage
# each hit by asking "is this ever written to a file or copied with a hardcoded size?".
# Object_* variants are union members inside Object and are expected to differ; they are runtime
# only and already handled by the union-alias padding in include/object.h.

cd "$(dirname "$0")/.." || exit 1

B64=${B64:-build/vandalhearts_pc}
B32=${B32:-build32/vandalhearts_pc}

for b in "$B64" "$B32"; do
    [ -x "$b" ] || { echo "missing $b -- see the header comment for how to build it" >&2; exit 1; }
done

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

grep -hoE "^} [A-Za-z_][A-Za-z0-9_]*;" ../../include/*.h | sed 's/^} //; s/;$//' | sort -u > "$work/types.txt"

cat > "$work/sweep.py" <<'PYEOF'
import gdb, json, os
out = {}
for name in open(os.environ['TYPES']).read().split():
    for expr in (name, 'struct ' + name):
        try:
            out[name] = int(gdb.lookup_type(expr).sizeof); break
        except Exception:
            pass
open(os.environ['OUT'], 'w').write(json.dumps(out))
gdb.execute("quit")
PYEOF

for pair in "64:$B64" "32:$B32"; do
    w=${pair%%:*}; bin=${pair#*:}
    TYPES="$work/types.txt" OUT="$work/sz$w.json" \
        gdb -q -batch -x "$work/sweep.py" "$bin" >/dev/null 2>&1
done

TDIR="$work" python3 - <<'PYEOF'
import json, os
d = os.environ['TDIR']
a = json.load(open(f'{d}/sz64.json')); b = json.load(open(f'{d}/sz32.json'))
diff = [(k, b[k], a[k]) for k in sorted(a) if k in b and a[k] != b[k]]
obj = [x for x in diff if x[0].startswith('Object')]
oth = [x for x in diff if not x[0].startswith('Object')]
print(f"{len(diff)} of {len(a)} types differ ({len(obj)} are Object_* union members, expected)\n")
print(f"{'type':<34}{'32-bit':>8}{'64-bit':>8}{'delta':>8}")
for k, s32, s64 in sorted(oth, key=lambda x: -(x[2] - x[1])):
    print(f"{k:<34}{s32:>8}{s64:>8}{s64-s32:>+8}")
if not oth:
    print("(none outside Object_*)")
PYEOF
