#!/usr/bin/env bash
# raster_check.sh [path-to-exe] -- golden-image regression test for the software rasterizer.
#
# First run: boots the game headless (smoke mode, movies auto-skipped) while RECORDING a GPU trace
# of the whole boot-to-title sequence -- every VRAM upload and every primitive the rasterizer drew
# -- then REPLAYS that trace deterministically and stores its VRAM signature as the reference.
# Every later run replays the same trace through the current code and compares signatures:
# any change to rasterization behavior (coverage, UV stepping, dithering, blending, clipping,
# texture windows...) changes the hash and FAILS the check. Byte-for-byte, no tolerance.
#
# The trace contains game-derived texture data, so it lives under build/regress/ (gitignored) and
# is recorded from YOUR disc on first use. After INTENTIONAL rasterizer changes, re-baseline with:
#   rm build/regress/boot.vht*    (then re-run this script)
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PC_DIR="$(cd "$HERE/../.." && pwd)"
EXE="${1:-$PC_DIR/build/vandalhearts_pc}"
RD="$PC_DIR/build/regress"
TRACE="$RD/boot.vht"; REF="$RD/boot.vht.ref"
[ -x "$EXE" ] || { echo "raster: no executable at $EXE (build first: make link OPT=1)" >&2; exit 2; }
mkdir -p "$RD"

replay_hash() {
    # Pin the raster environment: the ini->env loader runs before replay, so a user's
    # VH_INTERNAL_SCALE/VH_RASTER_THREADS in vandalhearts.ini would otherwise leak in.
    # VH_LANG/VH_LANGPACK too (empty = no pack): a persisted language pack legitimately patches
    # the F_WD glyph-sheet uploads at LoadImage, which changes the replayed VRAM signature --
    # exactly the false FAIL this pinning exists to prevent (bit us 2026-08-09 with ru-gen).
    env VH_GPU_REPLAY="$TRACE" VH_INTERNAL_SCALE=1 VH_RASTER_THREADS=1 \
        VH_LANG= VH_LANGPACK= VH_HDPACK=0 \
        SDL_VIDEODRIVER=dummy ALSOFT_DRIVERS=null "$EXE" 2>/dev/null \
        | grep -E "^REPLAY " || { echo "raster: replay produced no signature" >&2; exit 2; }
}

if [ ! -f "$TRACE" ]; then
    echo "raster: no trace yet -- recording the boot sequence (headless, ~10s)..."
    env VH_SMOKE=1 VH_SMOKE_LINGER=600 VH_GPU_RECORD="$TRACE" VH_GPU_RECORD_FRAMES=400 \
        SDL_VIDEODRIVER=dummy ALSOFT_DRIVERS=null VH_FULLSCREEN=0 VH_HDPACK=0 \
        VH_LANG= VH_LANGPACK= \
        "$EXE" > "$RD/record.log" 2>&1 || { echo "raster: recording boot FAILED:" >&2; tail -5 "$RD/record.log" >&2; exit 2; }
    [ -s "$TRACE" ] || { echo "raster: no trace was written (see $RD/record.log)" >&2; exit 2; }
    replay_hash > "$REF"
    echo "raster: reference created: $(cat "$REF")"
    echo "raster: BASELINE OK ($(du -h "$TRACE" | cut -f1) trace) -- future runs verify against it"
    exit 0
fi

NOW="$(replay_hash)"
if [ "$NOW" = "$(cat "$REF")" ]; then
    echo "raster: PASS  $NOW"
else
    echo "raster: FAIL -- rasterizer output changed!" >&2
    echo "  reference: $(cat "$REF")" >&2
    echo "  current:   $NOW" >&2
    echo "  (VH_GPU_REPLAY_VERBOSE=1 VH_GPU_REPLAY=$TRACE $EXE  shows per-frame hashes to localize;" >&2
    echo "   if the change is INTENDED, re-baseline: rm $TRACE* and re-run)" >&2
    exit 1
fi
