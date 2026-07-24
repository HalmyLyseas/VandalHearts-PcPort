#!/bin/sh
# Runs the AddressSanitizer build (make asan32) with the options this port specifically needs.
#
# Why each option is here -- none of these are cargo-culted, the build genuinely misbehaves
# without them:
#
#   handle_segv=0
#     pc_bootstrap.c installs its OWN SIGSEGV handler (sigaction + SA_SIGINFO). It is load-bearing,
#     not diagnostic: the game writes to string literals (PSX RAM is all writable), and the handler
#     mprotects the faulting .rodata page RW and retries the store. ASAN installs its own SIGSEGV
#     handler by default and would intercept those faults, turning every legitimate literal write
#     into a bogus "SEGV on unknown address" and killing the run. Letting our handler keep the
#     signal costs nothing: ASAN's real findings come from its instrumentation (redzones), not from
#     SIGSEGV, so out-of-bounds detection is unaffected.
#
#   halt_on_error=0
#     Requires -fsanitize-recover=address (the asan target passes it). Report and continue, so one
#     playthrough surfaces every distinct site instead of stopping at the first.
#
#   detect_leaks=0
#     LeakSanitizer is on by default in ASAN builds. This port never frees anything by design (it
#     emulates a fixed PSX memory map), so a leak report at exit would be thousands of lines of
#     expected noise. Memory growth is a known separate watch-item -- track it deliberately, not as
#     an ASAN exit dump.
#
#   log_path=asan.log
#     Reports go to asan.log.<pid> instead of stderr, so they survive the SDL window closing and
#     don't interleave with the game's own logging.
#
# This is the 32-BIT ASAN build, deliberately. A 64-bit one does not work: ASAN's shadow memory
# starts at 0x7fff8000 on x86-64 and swallows the fixed PSX RAM arena pc_bootstrap.c reserves at
# 0x80000000, so the mmap fails and the hardcoded PSX scratch-buffer literals in decompiled source
# write into ASAN's shadow (the game dies in the first CdRead). The 32-bit shadow is at
# 0x20000000, leaving 0x80000000 free. See the Makefile's asan/asan32 targets. No coverage is
# lost: out-of-bounds bugs are width-independent. The width-DEPENDENT class ASAN cannot see is
# covered separately by diffing sizeof() between build/ and build32/ under gdb.
#
# The binary must run from platform/pc/ -- it resolves the disc image and saves/ relative to cwd.

cd "$(dirname "$0")" || exit 1

if [ ! -x build_asan32/vandalhearts_pc ]; then
    echo "build_asan32/vandalhearts_pc not found -- run 'make asan32' first." >&2
    exit 1
fi

# ARCHIVE the previous run's reports, never delete them. This used to be `rm -f asan.log.*`, which
# threw away a real run's findings the moment the script was started again for any reason
# (including a short unrelated smoke test). A sweep's whole value is the accumulated report set.
if ls asan.log.* >/dev/null 2>&1; then
    mkdir -p asan_runs
    stamp=$(date +%Y%m%d-%H%M%S)
    for f in asan.log.*; do mv "$f" "asan_runs/${stamp}-${f}"; done
    echo "archived previous reports to asan_runs/${stamp}-*"
fi

# VH_SCALE is display-only: pc_gpu_window.c multiplies the window size and nearest-neighbour
# upscales the blit. The game still renders its native 320x240 framebuffer and no game logic or
# memory layout changes, so it cannot affect what ASAN reports. Defaulted to 4 here (1280x960) and
# still overridable from the shell: `VH_SCALE=2 ./run_asan.sh`.
# It must be part of this env prefix -- a bare `VH_SCALE=4` on its own line would set an
# unexported shell variable the game never sees.
# VH_RCNT1_NORMALIZE is REQUIRED for this build to get through a battle, and it is not a hack to
# work around a game bug -- it compensates for one the sanitizer itself creates. src/graphics.c
# gates its incremental sprite decoder on `GetRCnt(RCntCNT1) <= 470` during enemy turns. 470 HBlank
# ticks is ~30 ms, which on hardware (16.7 ms frames) can never be reached. Under ASAN a battle
# scene runs at ~12 FPS (83 ms frames), so the gate trips every frame, gDecodingSprites never
# clears, and the enemy turn never starts -- the camera pans forever with no unit activating.
# This makes RCnt1 frame-relative so the budget survives a slow host. Off by default in the normal
# build, so the calibrated AI timing is untouched there. See libkernel.c ResetRCnt.
ASAN_OPTIONS="handle_segv=0:halt_on_error=0:detect_leaks=0:log_path=asan.log:print_stacktrace=1:strict_string_checks=0" \
VH_SCALE="${VH_SCALE:-4}" \
VH_RCNT1_NORMALIZE="${VH_RCNT1_NORMALIZE:-1}" \
    ./build_asan32/vandalhearts_pc "$@"
status=$?

echo
if ls asan.log.* >/dev/null 2>&1; then
    echo "=== ASAN findings ==="
    grep -h "^==.*ERROR: AddressSanitizer" asan.log.* | sort | uniq -c | sort -rn
    echo
    echo "Full reports: $(ls asan.log.*)"
else
    echo "No ASAN findings."
fi
exit $status
