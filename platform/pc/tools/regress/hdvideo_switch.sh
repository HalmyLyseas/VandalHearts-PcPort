#!/usr/bin/env bash
# hdvideo_switch.sh -- ASan regression for the mid-stream geometry-change fix in src/pc_hdvideo.c
# (codex 1.2): a later SPS/PPS can change frame width/height/pixel format, and the sws
# context/output buffer must be recreated for it rather than scaled/written into with the
# open-time geometry.
#
# Generates a fixture mp4 with ffmpeg (present on this box): a 320x240 h264 clip concatenated
# with a 160x120 one via the concat demuxer, so the decoded stream genuinely carries two SPS
# sizes (verified below with ffprobe). Compiles pc_hdvideo.c standalone with its built-in
# -DHDVIDEO_TEST main (see the file's own header comment for the base cc line) under ASan
# against the system libav, and decodes past the switch point. Expects exit 0, no ASan report,
# and the frame at the switch point reporting the NEW geometry.
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PC_DIR="$(cd "$HERE/../.." && pwd)"
BIN="$PC_DIR/build/hdvideo_switch_test"

command -v ffmpeg >/dev/null || { echo "hdvideo_switch: ffmpeg not found -- skipping" >&2; exit 2; }
command -v ffprobe >/dev/null || { echo "hdvideo_switch: ffprobe not found -- skipping" >&2; exit 2; }
pkg-config --exists libavformat libavcodec libswscale libavutil || {
    echo "hdvideo_switch: libav pkg-config modules not found -- skipping" >&2; exit 2; }

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

echo "hdvideo_switch: generating fixture (320x240 -> 160x120, 10 fps, 1s each)..."
ffmpeg -y -hide_banner -loglevel error -f lavfi -i testsrc=size=320x240:rate=10:duration=1 \
    -pix_fmt yuv420p -c:v libx264 -profile:v baseline "$TMP/a.mp4"
ffmpeg -y -hide_banner -loglevel error -f lavfi -i testsrc=size=160x120:rate=10:duration=1 \
    -pix_fmt yuv420p -c:v libx264 -profile:v baseline "$TMP/b.mp4"
printf "file 'a.mp4'\nfile 'b.mp4'\n" > "$TMP/list.txt"
ffmpeg -y -hide_banner -loglevel error -f concat -safe 0 -i "$TMP/list.txt" -c copy "$TMP/switch.mp4"

sizes="$(ffprobe -v error -select_streams v:0 -show_entries frame=width,height -of csv "$TMP/switch.mp4" \
         | sed 's/^frame,//' | sort -u | wc -l)"
[ "$sizes" -ge 2 ] || { echo "hdvideo_switch: fixture only carries one frame size ($sizes) -- ffprobe:" >&2;
    ffprobe -v error -select_streams v:0 -show_entries frame=width,height -of csv "$TMP/switch.mp4" >&2; exit 2; }
echo "hdvideo_switch: fixture carries $sizes distinct frame sizes"

echo "hdvideo_switch: compiling pc_hdvideo.c standalone under ASan..."
cat > "$TMP/verbose_stub.c" <<'EOF'
/* PC_Verbose lives in pc_bootstrap.c in the real build; this harness links pc_hdvideo.c alone. */
int PC_Verbose(void) { return 0; }
EOF
cc -fsanitize=address -fno-omit-frame-pointer -g -O0 -DVH_HD_VIDEO -DHDVIDEO_TEST \
   -I"$PC_DIR/include" \
   "$PC_DIR/src/pc_hdvideo.c" "$TMP/verbose_stub.c" \
   $(pkg-config --cflags --libs libavformat libavcodec libswscale libavutil) \
   -o "$BIN"

echo "hdvideo_switch: decoding past the switch point..."
mkdir -p "$TMP/out" && (cd "$TMP/out" && mkdir -p /tmp/hdvid)
LOG="$(mktemp)"
rc=0
env ASAN_OPTIONS=detect_leaks=0 "$BIN" "$TMP/switch.mp4" 15 > "$LOG" 2>&1 || rc=$?
cat "$LOG"

fail=0
if [ "$rc" -ne 0 ]; then
    echo "hdvideo_switch: test binary exited $rc (ASan finding or decode failure)" >&2
    fail=1
fi
grep -q "160x120" "$LOG" || { echo "hdvideo_switch: frame past the switch did not report the new 160x120 geometry" >&2; fail=1; }

rm -f "$LOG" /tmp/hdvid/decoded.ppm
rmdir /tmp/hdvid 2>/dev/null || true
if [ "$fail" -eq 0 ]; then
    echo "hdvideo_switch: PASS"
    exit 0
else
    echo "hdvideo_switch: FAIL"
    exit 1
fi
