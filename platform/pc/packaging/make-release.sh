#!/usr/bin/env bash
#
# make-release.sh <version-tag> [--windows-only|--linux-only] [--no-publish]
#
#   ./make-release.sh v1.0.0
#
# Builds the Windows + Linux release artifacts and publishes a GitHub release.
#
# WHY THIS IS A LOCAL SCRIPT, NOT A GITHUB ACTIONS BUILD: the data-segment
# generator needs the byte-exact SLUS_004.47(.elf) + the PsyQ KROMDAT.BIN at
# build time to reconstruct the embedded game data. Those are copyrighted and
# cannot live on GitHub's runners, so the binaries MUST be built on a machine
# that has the user's own disc/BIOS (i.e. here). Automation covers packaging +
# upload only. See docs/cross-platform.md and NOTICE (release binaries embed a
# portion of game-derived data).
#
# Build environments (each artifact in the one that gives the right result):
#   * Windows .exe  -> host MinGW-w64 cross-compile (CMake toolchain file)
#   * Linux AppImage -> the pinned Debian 12 distrobox container 'vh-deb12'
#                       (sets the glibc floor low; a host-built AppImage would
#                        only run on distros as new as this host)
set -euo pipefail

# ---- args -------------------------------------------------------------------
TAG="${1:-}"
[ -n "$TAG" ] || { echo "usage: $0 <version-tag> [--windows-only|--linux-only] [--no-publish]" >&2; exit 2; }
shift || true
DO_WIN=1 DO_LINUX=1 PUBLISH=1 CONTAINER="vh-deb12"
for a in "$@"; do case "$a" in
  --windows-only) DO_LINUX=0 ;;
  --linux-only)   DO_WIN=0 ;;
  --no-publish)   PUBLISH=0 ;;
  --container=*)  CONTAINER="${a#*=}" ;;
  *) echo "unknown flag: $a" >&2; exit 2 ;;
esac; done

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"   # platform/pc/packaging
PC_DIR="$(cd "$HERE/.." && pwd)"                       # platform/pc
REPO="$(cd "$PC_DIR/../.." && pwd)"                    # vh/
STAGE="$PC_DIR/dist/release/$TAG"
INI="$PC_DIR/vandalhearts.ini"

log()  { printf '\033[1;36m==>\033[0m %s\n' "$*"; }
die()  { printf '\033[1;31mERROR:\033[0m %s\n' "$*" >&2; exit 1; }

rm -rf "$STAGE"; mkdir -p "$STAGE"
[ -f "$INI" ] || die "missing $INI"

# ---- Windows: host MinGW-w64 cross-compile ----------------------------------
if [ "$DO_WIN" = 1 ]; then
    command -v x86_64-w64-mingw32-gcc >/dev/null || die "MinGW-w64 toolchain not found (pacman -S mingw-w64-gcc)"
    log "Windows: cross-compiling with MinGW-w64"
    ( cd "$PC_DIR"
      cmake -S . -B build_win -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-mingw-w64.cmake >/dev/null
      cmake --build build_win >/dev/null )
    WIN_EXE="$PC_DIR/build_win/vandalhearts_pc.exe"
    [ -f "$WIN_EXE" ] || die "Windows build produced no .exe"
    WZIP_DIR="$STAGE/win"; mkdir -p "$WZIP_DIR"
    cp "$WIN_EXE" "$WZIP_DIR/"
    # the 6 runtime DLLs the CMake post-build step stages next to the .exe
    cp "$PC_DIR"/build_win/*.dll "$WZIP_DIR/" 2>/dev/null || die "expected runtime DLLs beside the .exe"
    cp "$INI" "$WZIP_DIR/"
    ndll=$(ls -1 "$WZIP_DIR"/*.dll | wc -l)
    [ "$ndll" -eq 6 ] || echo "  WARNING: expected 6 DLLs, found $ndll"
    ( cd "$WZIP_DIR" && zip -q -r "$STAGE/VandalHearts-$TAG-windows-x64.zip" . )
    rm -rf "$WZIP_DIR"
    log "  -> VandalHearts-$TAG-windows-x64.zip ($ndll DLLs + exe + ini)"
fi

# ---- Linux: AppImage from the Debian 12 container ---------------------------
if [ "$DO_LINUX" = 1 ]; then
    command -v distrobox >/dev/null || die "distrobox not found"
    distrobox list 2>/dev/null | grep -q "$CONTAINER" || die "container '$CONTAINER' not found (see docs/cross-platform.md)"
    log "Linux: building the AppImage in container '$CONTAINER' (glibc floor)"
    distrobox enter "$CONTAINER" -- bash -lc "
        set -e; export PATH=\"\$HOME/bin:\$PATH\"
        cd '$PC_DIR'
        make link BUILD_DIR=build_deb >/dev/null
        packaging/appimage/build-appimage.sh build_deb/vandalhearts_pc >/dev/null"
    APP="$PC_DIR/dist/VandalHearts-x86_64.AppImage"
    [ -f "$APP" ] || die "container build produced no AppImage"
    cp "$APP" "$STAGE/VandalHearts-$TAG-linux-x86_64.AppImage"
    cp "$INI" "$STAGE/vandalhearts.ini"
    log "  -> VandalHearts-$TAG-linux-x86_64.AppImage + vandalhearts.ini"
fi

# ---- checksums --------------------------------------------------------------
log "Checksums"
( cd "$STAGE" && sha256sum VandalHearts-* vandalhearts.ini 2>/dev/null > SHA256SUMS.txt || true )
cat "$STAGE/SHA256SUMS.txt" | sed 's/^/    /'

# ---- release notes ----------------------------------------------------------
NOTES="$STAGE/RELEASE_NOTES.md"
cat > "$NOTES" <<NOTE
## Vandal Hearts — PC Port $TAG

A native PC port of Vandal Hearts (US, SLUS_004.47). **You must supply your own
legally-owned disc image** (\`.bin\`); the download does nothing without it.

### Downloads
| Platform | File | How to run |
|---|---|---|
| Windows 10/11 | \`VandalHearts-$TAG-windows-x64.zip\` | Unzip; put your disc in a \`game\\\` folder next to \`vandalhearts_pc.exe\`; run it. |
| Linux (glibc ≥ 2.34) | \`VandalHearts-$TAG-linux-x86_64.AppImage\` + \`vandalhearts.ini\` | Put both together; put your disc in a \`game/\` folder beside the \`.AppImage\`; \`chmod +x\` and run. Needs FUSE2. |

Config: edit \`vandalhearts.ini\` next to the executable (window scale, audio, etc.).

### Note on contents
A release binary embeds a portion of game-derived data (the executable's static
data segment, a small BIOS-derived font, reconstructed tables) so the port can
run — © Konami / © Sony, no ownership claimed. It is a fraction of the game; the
bulk loads from your own disc at runtime. See NOTICE/DISCLAIMER.

Verify downloads against \`SHA256SUMS.txt\`.
NOTE
log "Notes: $NOTES"

# ---- publish ----------------------------------------------------------------
if [ "$PUBLISH" = 1 ]; then
    command -v gh >/dev/null || die "gh CLI not found (pacman -S github-cli; then gh auth login)"
    gh auth status >/dev/null 2>&1 || die "gh not authenticated (gh auth login)"
    log "Publishing GitHub release $TAG"
    # -R pins the repo; --verify-tag would require an existing tag, so let gh create it from HEAD.
    gh release create "$TAG" \
        --repo HalmyLyseas/VandalHearts-PcPort \
        --title "Vandal Hearts PC Port $TAG" \
        --notes-file "$NOTES" \
        "$STAGE"/VandalHearts-* "$STAGE/vandalhearts.ini" "$STAGE/SHA256SUMS.txt"
    log "Done. Release $TAG published."
else
    log "Staged (not published): $STAGE"
    log "Publish later with: gh release create $TAG -R HalmyLyseas/VandalHearts-PcPort --notes-file $NOTES $STAGE/VandalHearts-* $STAGE/vandalhearts.ini $STAGE/SHA256SUMS.txt"
fi
