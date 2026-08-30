#!/usr/bin/env bash
# check_build_parity.sh -- guard against Makefile <-> CMakeLists source-list drift: every
# platform/pc/src/*.c must compile under both build systems, or one silently loses a unit.
# See docs/releasing.md, "3. Stage-build BOTH platforms first — always".
set -euo pipefail
PC_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MK="$PC_DIR/Makefile"; CM="$PC_DIR/CMakeLists.txt"

# ---- Makefile: stems it compiles --------------------------------------------------------------
mk_stems="$(
  { sed -n 's/^BACKEND_SRCS[[:space:]]*:=[[:space:]]*//p' "$MK" | tr ' ' '\n' | sed 's/\.c$//'
    # $(BUILD_DIR)/<x>.o USAGES only (link prerequisites/recipes). A per-file RULE HEADER
    # ("$(BUILD_DIR)/x.o: src/x.c") proves a rule exists, not that anything links it -- dropping a
    # file from BACKEND_SRCS while its rule remains must still be flagged.
    grep -vE '^\$\(BUILD_DIR\)/[A-Za-z_0-9]+\.o:' "$MK" \
      | grep -oE '\$\(BUILD_DIR\)/[A-Za-z_0-9]+\.o' | sed 's#.*/##; s/\.o$//'
    # P5: the unified target compiles the shared layer outside $(BUILD_DIR)
    grep -oE 'build-uni/[A-Za-z_0-9]+\.o' "$MK" | sed 's#.*/##; s/\.o$//'
  } | sort -u)"

# ---- CMakeLists: stems it compiles ------------------------------------------------------------
cm_stems="$(
  { sed -n 's/^set(BACKEND_PLAIN[[:space:]]*\(.*\))$/\1/p'  "$CM"
    sed -n 's/^set(DATA_PERM[[:space:]]*\(.*\))$/\1/p'      "$CM"
    sed -n 's/^set(DATA_PLAIN[[:space:]]*\(.*\))$/\1/p'     "$CM"
    grep -E '^foreach\(_b ' "$CM" | sed 's/^foreach(_b //; s/)$//' | tr ' ' '\n' | grep -v '^\${'
    # P5: region-conditional backend swaps + the unified shared layer
    sed -n 's/^[[:space:]]*list(APPEND BACKEND_PLAIN[[:space:]]*\(.*\))$/\1/p' "$CM"
    sed -n 's/^[[:space:]]*list(APPEND GENERATED_DATA_C[[:space:]]*\(.*\))$/\1/p' "$CM"
    grep -oE 'src/pc_region_main\.c' "$CM" | sed 's#.*/##; s/\.c$//'
  } | tr ' ' '\n' | grep -vE '^$' | sort -u)"

# fail-closed: both parses must find a plausible number of entries
mk_n=$(printf '%s\n' "$mk_stems" | grep -c . || true)
cm_n=$(printf '%s\n' "$cm_stems" | grep -c . || true)
if [ "$mk_n" -lt 15 ] || [ "$cm_n" -lt 15 ]; then
    echo "PARITY: parser broke (Makefile=$mk_n, CMake=$cm_n stems) -- the list format changed;" >&2
    echo "update tools/check_build_parity.sh to match." >&2
    exit 2
fi

fail=0
for src in "$PC_DIR"/src/*.c; do
    name="$(basename "$src")"; stem="${name%.c}"
    case "$name" in test_*.c) continue ;; esac
    printf '%s\n' "$mk_stems" | grep -qx "$stem" || { echo "PARITY: $name not compiled by the Makefile"; fail=1; }
    printf '%s\n' "$cm_stems" | grep -qx "$stem" || { echo "PARITY: $name not compiled by CMakeLists.txt"; fail=1; }
done

if [ "$fail" -ne 0 ]; then
    echo "Build-system source lists have drifted -- add the file(s) to BOTH builds" >&2
    echo "(Makefile: BACKEND_SRCS or a data rule; CMakeLists: BACKEND_PLAIN / DATA_* lists)" >&2
    exit 1
fi
echo "build parity OK (Makefile=$mk_n, CMake=$cm_n stems; all src/*.c in both)"
