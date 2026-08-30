#!/usr/bin/env bash
# Builds a self-contained VandalHearts-x86_64.AppImage from an already-built binary.
#   ./build-appimage.sh [path/to/vandalhearts_pc]
# Default: platform/pc/build/vandalhearts_pc. See docs/cross-platform.md, "Building a release".

# This bundles nothing from glibc, so the AppImage requires the host's glibc to be at least
# the build machine's. See docs/cross-platform.md, "Runtime requirements (the glibc floor)".
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PC_DIR="$(cd "$HERE/../.." && pwd)"                    # platform/pc
BIN="${1:-$PC_DIR/build/vandalhearts_pc}"
OUT_DIR="$PC_DIR/dist"
APPDIR="$OUT_DIR/AppDir"

export APPIMAGE_EXTRACT_AND_RUN=1
export ARCH=x86_64
# linuxdeploy ships an ancient `strip` that chokes on the modern `.relr.dyn` relocation section
# that recent toolchains (e.g. Arch binutils) emit ("unknown type [0x13]"), aborting the run.
# Stripping is only a size optimization, so disable it. (appimagetool compresses anyway.)
export NO_STRIP=1

log() { printf '\033[1;36m==>\033[0m %s\n' "$*"; }
die() { printf '\033[1;31mERROR:\033[0m %s\n' "$*" >&2; exit 1; }

command -v linuxdeploy  >/dev/null || die "linuxdeploy not found (paru -S linuxdeploy-appimage)"
command -v appimagetool >/dev/null || die "appimagetool not found (paru -S appimagetool-bin)"
[ -x "$BIN" ] || die "binary not found/executable: $BIN  (build it first: make link)"

log "Binary:  $BIN"
file "$BIN" | sed 's/^/         /'

# Fresh AppDir every run.
rm -rf "$APPDIR"
mkdir -p "$OUT_DIR"

# Rasterize the original placeholder icon (SVG -> 256x256 PNG) if a rasterizer is present;
# otherwise expect a pre-made vandalhearts.png beside the .desktop.
ICON_PNG="$HERE/vandalhearts.png"
if [ ! -f "$ICON_PNG" ]; then
    if command -v rsvg-convert >/dev/null; then
        log "Rendering icon (rsvg-convert)"
        rsvg-convert -w 256 -h 256 "$HERE/vandalhearts.svg" -o "$ICON_PNG"
    elif command -v magick >/dev/null; then
        log "Rendering icon (magick)"
        magick -background none "$HERE/vandalhearts.svg" -resize 256x256 "$ICON_PNG"
    else
        die "no rsvg-convert/magick to rasterize the icon, and no prebuilt vandalhearts.png"
    fi
fi

log "Populating AppDir with linuxdeploy (bundling SDL2/OpenAL + private deps)"
linuxdeploy \
    --appdir "$APPDIR" \
    --executable "$BIN" \
    --desktop-file "$HERE/vandalhearts.desktop" \
    --icon-file "$ICON_PNG"

log "Bundled libraries:"
ls -1 "$APPDIR/usr/lib/" 2>/dev/null | sed 's/^/         /' || true

log "Packing AppImage with appimagetool"
APPIMAGE_OUT="$OUT_DIR/VandalHearts-x86_64.AppImage"
appimagetool "$APPDIR" "$APPIMAGE_OUT"

chmod +x "$APPIMAGE_OUT"

# Ship the same vandalhearts.ini template Windows ships, NEXT TO the .AppImage. The AppImage runs
# from a read-only mount, so the editable config can't live inside it -- it goes beside it, where
# PC_GetDeployDir() (via $APPIMAGE) looks. Identical file + keys as the Windows build = parity.
INI_SRC="$PC_DIR/vandalhearts.ini"
if [ -f "$INI_SRC" ]; then
    cp -f "$INI_SRC" "$OUT_DIR/vandalhearts.ini"
    log "Config template: $OUT_DIR/vandalhearts.ini (edit VH_SCALE etc.; ships beside the .AppImage)"
fi

log "Done: $APPIMAGE_OUT  ($(du -h "$APPIMAGE_OUT" | cut -f1))"
echo
echo "  A release = these two files together (plus the user's own disc):"
echo "     VandalHearts-x86_64.AppImage      vandalhearts.ini"
echo "  To run:   put your disc image (game/*.bin, or a *.bin) NEXT TO the .AppImage, then:"
echo "              ./VandalHearts-x86_64.AppImage"
echo "  Config:   edit vandalhearts.ini beside the .AppImage (uncomment VH_SCALE=2, etc.)."
