#!/bin/sh
# Runs the UndefinedBehaviorSanitizer build (make ubsan) with the options this port needs.
#
# This is the 64-BIT sanitizer pass -- the complement to run_asan.sh. AddressSanitizer must run at
# 32-bit here (its shadow at 0x7fff8000 collides with the fixed PSX RAM arena at 0x80000000, see
# run_asan.sh), so it can only prove out-of-bounds at the narrower width. `make ubsan` uses
# `-fsanitize=bounds`, which has NO shadow memory, so it runs at the DEFAULT 64-bit width the port
# actually ships -- catching a statically-sized-array overrun at the width that matters.
#
# Why this needs far less special-casing than run_asan.sh:
#   * UBSan installs no SIGSEGV handler, so pc_bootstrap.c's own .rodata-fixup handler is untouched
#     (no handle_segv needed).
#   * UBSan has no LeakSanitizer, so there is no detect_leaks exit-dump to silence.
#   * `-fsanitize=bounds` is inline compares, not shadow lookups, so the game runs at near-normal
#     speed -- it does NOT starve src/graphics.c's sprite decoder the way the ~12 FPS ASan build did,
#     so VH_RCNT1_NORMALIZE is left OFF (matching normal gameplay + the calibrated AI timing). If an
#     enemy turn ever hangs with the camera panning forever, that slow-host symptom is back: re-run
#     with VH_RCNT1_NORMALIZE=1 (see run_asan.sh's note and libkernel.c ResetRCnt).
#
# UBSAN_OPTIONS:
#   halt_on_error=0     report and continue, so one playthrough surfaces every distinct site.
#   print_stacktrace=1  include a call stack per finding (needs a symbolizer to resolve names; the
#                       bounds error itself always names its own src file:line regardless, because it
#                       is compile-time instrumented).
#   log_path=ubsan.log  reports go to ubsan.log.<pid>, surviving the SDL window closing.
#
# Coverage note: `-fsanitize=bounds` only instruments accesses whose array size the compiler knows
# at compile time, so its silence is not proof of total coverage -- it is a complement to the ASan
# sweep, not a superset. ASan already swept ch 1/4/6 + final + credits and fixed 7 OOB bugs (gated),
# so expect this pass to mostly CONFIRM. A lighter playthrough than ASan is fine.
#
# Run from platform/pc/ -- the binary resolves the disc image and saves/ relative to cwd.

cd "$(dirname "$0")" || exit 1

if [ ! -x build_ubsan/vandalhearts_pc ]; then
    echo "build_ubsan/vandalhearts_pc not found -- run 'make ubsan' first." >&2
    exit 1
fi

# Archive the previous run's reports, never delete them (a sweep's value is the accumulated set).
if ls ubsan.log.* >/dev/null 2>&1; then
    mkdir -p ubsan_runs
    stamp=$(date +%Y%m%d-%H%M%S)
    for f in ubsan.log.*; do mv "$f" "ubsan_runs/${stamp}-${f}"; done
    echo "archived previous reports to ubsan_runs/${stamp}-*"
fi

# VH_SCALE is display-only (nearest-neighbour blit upscale); it cannot affect what UBSan reports.
UBSAN_OPTIONS="halt_on_error=0:print_stacktrace=1:log_path=ubsan.log" \
VH_SCALE="${VH_SCALE:-4}" \
    ./build_ubsan/vandalhearts_pc "$@"
status=$?

echo
if ls ubsan.log.* >/dev/null 2>&1; then
    echo "=== UBSan findings (distinct sites) ==="
    grep -h "runtime error:" ubsan.log.* | sort | uniq -c | sort -rn
    echo
    echo "Full reports: $(ls ubsan.log.*)"
else
    echo "No UBSan findings."
fi
exit $status
