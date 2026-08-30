#!/usr/bin/env bash
# build-manual.sh [version] [outfile] -- renders docs/manual/manual.md to a PDF: pandoc to
# standalone HTML (images embedded as data URIs), then headless Chromium print-to-pdf --
# no LaTeX/weasyprint/typst needed. Output is a release asset, never committed.
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="$(cd "$HERE/../../.." && pwd)"
VER="${1:-development build}"
OUT="${2:-$REPO/platform/pc/build/VandalHearts-Manual.pdf}"
SRC="$REPO/docs/manual/manual.md"
CSS="$REPO/docs/manual/manual.css"
TMPHTML="$(mktemp --suffix=.html)"
trap 'rm -f "$TMPHTML"' EXIT

command -v pandoc >/dev/null || { echo "manual: pandoc not found" >&2; exit 2; }
CHROME="$(command -v chromium || command -v chromium-browser || command -v google-chrome || true)"
[ -n "$CHROME" ] || { echo "manual: no chromium/chrome for PDF rendering" >&2; exit 2; }

sed "s/@VERSION@/$VER/" "$SRC" | pandoc --standalone --embed-resources \
    --css "$CSS" \
    --resource-path "$REPO/docs/manual:$REPO/docs:$REPO" \
    --from markdown -o "$TMPHTML"

mkdir -p "$(dirname "$OUT")"
"$CHROME" --headless --disable-gpu --no-sandbox --no-pdf-header-footer \
    --print-to-pdf="$OUT" "$TMPHTML" 2>/dev/null
[ -s "$OUT" ] || { echo "manual: chromium produced no PDF" >&2; exit 2; }
echo "manual: $OUT ($(du -h "$OUT" | cut -f1))"
