#!/usr/bin/env bash
# check_build_parity.sh -- guard against Makefile <-> CMakeLists source-list drift.
#
# The port has two interchangeable build systems, each with its OWN source list. Every
# platform/pc/src/*.c must be COMPILED by both, or one build silently loses a compilation unit and
# breaks only on the platform that uses that system. This exact drift caused the 1.6 Windows
# release breakage: pc_hdvideo.c was in the Makefile's BACKEND_SRCS but not CMake's BACKEND_PLAIN
# -> undefined PC_HdVideo* at the MinGW link (a nearby CMake COMMENT mentioned the file, which is
# why this script parses the actual source LISTS, not whole-file mentions).
#
# Parsed lists (fail-closed: if parsing yields implausibly few entries, the script errors so a
# format change can't silently disable the check):
#   Makefile   : the BACKEND_SRCS line + every $(BUILD_DIR)/<stem>.o reference (data files)
#   CMakeLists : set(BACKEND_PLAIN ...), set(DATA_PERM ...), set(DATA_PLAIN ...) + the literal
#                stems on the foreach() line that assembles ALL_OBJ_SRCS
# Exempt: src/test_*.c (standalone Phase-C PoC harnesses with their own main(), never in the game
# build). make-release.sh runs this before building anything.
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
