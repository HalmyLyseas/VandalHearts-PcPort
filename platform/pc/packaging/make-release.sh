#!/usr/bin/env bash

# make-release.sh <version-tag> [--windows-only|--linux-only] [--no-publish] [--hdpack=<dir>]
# Builds the Windows + Linux release artifacts and publishes a GitHub release.
# Example: ./make-release.sh vX.Y.Z --hdpack=<assembled hdpacks dir>

# Runs locally, not in CI -- the data-segment generator needs the byte-exact
# game files, which cannot live on a public runner. See docs/releasing.md,
# "5. Publish".

# Build environments (each artifact in the one that gives the right result):
#   * Windows .exe   -> host MinGW-w64 cross-compile (CMake toolchain file)
#   * Linux AppImage -> the Debian 12 distrobox container 'vh-deb12'

set -euo pipefail

# ---- args -------------------------------------------------------------------
TAG="${1:-}"
[ -n "$TAG" ] || { echo "usage: $0 <version-tag> [--windows-only|--linux-only] [--no-publish] [--hdpack=<dir>]" >&2; exit 2; }
shift || true
DO_WIN=1 DO_LINUX=1 PUBLISH=1 CONTAINER="vh-deb12"
HDPACK_SRC="${VH_HDPACK_DIR:-}"      # optional assembled hdpacks/ folder -> extra release asset
HDPACK_NOTE=""
for a in "$@"; do case "$a" in
  --windows-only) DO_LINUX=0 ;;
  --linux-only)   DO_WIN=0 ;;
  --no-publish)   PUBLISH=0 ;;
  --container=*)  CONTAINER="${a#*=}" ;;
  --hdpack=*)     HDPACK_SRC="${a#*=}" ;;
  --hdpack-note=*) HDPACK_NOTE="${a#*=}" ;;   # release-specific suffix for the Downloads-table row
                                              # (e.g. "Unchanged since last release -- keep yours.")
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

# Guard: Makefile <-> CMakeLists source-list drift breaks exactly one platform's build (the 1.6
# pc_hdvideo incident). Catch it before spending minutes on either build.
"$PC_DIR/tools/check_build_parity.sh" || die "build-system parity check failed (see above)"

# The AppImage stage runs `rm -rf build-uni*` (clean builds are required); a live
# test deployment (discs, hdpacks/langpacks, saves) parked there would be deleted
# with it. See docs/releasing.md, "3. Stage-build BOTH platforms first — always".
for d in "$PC_DIR"/build-uni; do
    [ -d "$d" ] || continue
    if compgen -G "$d/*.bin" >/dev/null || [ -d "$d/saves" ] || [ -d "$d/saves_tactical" ] \
       || [ -d "$d/hdpacks" ] || [ -d "$d/langpacks" ]; then
        die "user data (discs/saves/packs) found in $d -- the release build wipes build-uni*. \
Move your test deployment elsewhere (e.g. a deploy/ folder) first."
    fi
done

# ---- Windows: host MinGW-w64 cross-compile ----------------------------------
if [ "$DO_WIN" = 1 ]; then
    command -v x86_64-w64-mingw32-gcc >/dev/null || die "MinGW-w64 toolchain not found (pacman -S mingw-w64-gcc)"
    # 1.6: the HD-video decoder links a minimal STATIC libav so the Windows build ships NO ffmpeg DLLs
    # (a shared distro/MSYS2 ffmpeg would drag 35+ codec DLLs). Build it once into a cached prefix and
    # point CMake at it. Override the prefix with VH_MINGW_FFMPEG=<dir> to reuse a prebuilt one.
    FFPREFIX="${VH_MINGW_FFMPEG:-$PC_DIR/ffmpeg-mingw-static}"
    if [ ! -f "$FFPREFIX/lib/libavcodec.a" ]; then
        log "Windows: building minimal static libav (one-time) -> $FFPREFIX"
        PREFIX="$FFPREFIX" "$PC_DIR/tools/build-ffmpeg-static.sh" >/dev/null
        [ -f "$FFPREFIX/lib/libavcodec.a" ] || die "static libav build failed (see tools/build-ffmpeg-static.sh)"
    fi
    log "Windows: cross-compiling with MinGW-w64 (-O2)"
    # Release binaries add -O2 on top of the default debug build (-O0 -g); the
    # internal-resolution rasterizer needs it to hold 30 fps. See docs/building.md,
    # "Optimization".

    # VH_MINGW_FFMPEG points the toolchain's CMAKE_FIND_ROOT_PATH at the static libav
    # prefix -- a plain CMAKE_PREFIX_PATH is ignored under MinGW's find-root mode.

    # Release builds are clean builds: an incremental build can link objects compiled
    # against one library era with archives from another. See docs/releasing.md,
    # "3. Stage-build BOTH platforms first — always".

    # The shipped Windows exe is the unified binary (both regions, runtime disc
    # selection); build-unified-win.sh runs the three clean CMake stages (us core,
    # jp core, final link) with these exact toolchain conventions.
    VH_MINGW_FFMPEG="$FFPREFIX" "$PC_DIR/packaging/build-unified-win.sh" >/dev/null
    WIN_EXE="$PC_DIR/build_win_uni/vandalhearts_pc.exe"
    [ -f "$WIN_EXE" ] || die "Windows build produced no .exe"
    WZIP_DIR="$STAGE/win"; mkdir -p "$WZIP_DIR"
    cp "$WIN_EXE" "$WZIP_DIR/"
    # Ship the exe stripped (no backtrace machinery on Windows, and it embeds local
    # build paths otherwise); the unstripped copy stays in build_win_uni/ for
    # debugging. See docs/releasing.md, "3. Stage-build BOTH platforms first — always".
    x86_64-w64-mingw32-strip "$WZIP_DIR/vandalhearts_pc.exe" \
        || die "strip failed on the Windows exe"
    if strings "$WZIP_DIR/vandalhearts_pc.exe" | grep -qE "/home/|$(id -un)"; then
        die "shipped Windows exe still contains local build paths"
    fi
    # the 8 runtime DLLs the CMake post-build step stages next to the .exe (6 base + libwebp/libsharpyuv)
    cp "$PC_DIR"/build_win_uni/*.dll "$WZIP_DIR/" 2>/dev/null || die "expected runtime DLLs beside the .exe"
    cp "$INI" "$WZIP_DIR/"
    # 6 base runtime DLLs (SDL2, OpenAL32, libwinpthread, libgcc_s_seh, libstdc++, libssp) + 2 for the
    # 1.6 HD background codec (libwebp, libsharpyuv). libav is static, so it adds none. -> 8 expected.
    ndll=$(ls -1 "$WZIP_DIR"/*.dll | wc -l)
    [ "$ndll" -eq 8 ] || echo "  WARNING: expected 8 DLLs (6 base + libwebp/libsharpyuv), found $ndll: $(ls -1 "$WZIP_DIR"/*.dll | xargs -n1 basename | tr '\n' ' ')"
    ( cd "$WZIP_DIR" && zip -q -r "$STAGE/VandalHearts-$TAG-windows-x64.zip" . )
    rm -rf "$WZIP_DIR"
    log "  -> VandalHearts-$TAG-windows-x64.zip ($ndll DLLs + exe + ini)"
fi

# ---- Linux: AppImage from the Debian 12 container ---------------------------
if [ "$DO_LINUX" = 1 ]; then
    command -v distrobox >/dev/null || die "distrobox not found"
    distrobox list 2>/dev/null | grep -q "$CONTAINER" || die "container '$CONTAINER' not found (see docs/cross-platform.md)"
    # Links a minimal static libav (same as Windows) instead of the distro's shared
    # ffmpeg, built inside the container from a source tree cloned on the host (the
    # container has no git). See docs/cross-platform.md, "Building a release".
    FF_LINUX="$PC_DIR/ffmpeg-linux-static"
    FF_SRC="$PC_DIR/build/ffmpeg-src"
    if [ ! -f "$FF_LINUX/lib/libavcodec.a" ]; then
        if [ ! -f "$FF_SRC/configure" ]; then
            log "Linux: cloning FFmpeg source (host) -> $FF_SRC"
            git clone --depth 1 --branch n7.1 https://github.com/FFmpeg/FFmpeg.git "$FF_SRC" >/dev/null 2>&1 \
                || die "FFmpeg clone failed"
        fi
        log "Linux: building minimal static libav in the container (one-time) -> $FF_LINUX"
        distrobox enter "$CONTAINER" -- bash -lc \
            "TARGET=native PREFIX='$FF_LINUX' SRC='$FF_SRC' '$PC_DIR/tools/build-ffmpeg-static.sh'" >/dev/null 2>&1
        [ -f "$FF_LINUX/lib/libavcodec.a" ] || die "container static-libav build failed (rerun tools/build-ffmpeg-static.sh TARGET=native inside $CONTAINER)"
    fi
    log "Linux: building the AppImage in container '$CONTAINER' (glibc floor)"
    distrobox enter "$CONTAINER" -- bash -lc "
        set -e; export PATH=\"\$HOME/bin:\$PATH\"
        cd '$PC_DIR'
        # 1.6 HD deps: fail loudly with an install hint rather than silently building a no-HD AppImage.
        # libwebp comes from the distro (small, clean dep); libav comes from the static prefix above
        # (its pkg-config dir is prepended so the port's Makefile resolves the .a's, no ffmpeg .so).
        pkg-config --exists libwebp || { echo \"ERROR: libwebp-dev missing in container '$CONTAINER'.\"; \
              echo \"  fix: distrobox enter $CONTAINER -- sudo apt-get install -y libwebp-dev\"; exit 1; }
        export PKG_CONFIG_PATH='$FF_LINUX/lib/pkgconfig'\${PKG_CONFIG_PATH:+:\$PKG_CONFIG_PATH}
        # Ship the unified binary (both regions); clean all three build stages.
        rm -rf build-uni-us build-uni-jp build-uni
        make unified CC='cc -O2' >/dev/null
        packaging/appimage/build-appimage.sh build-uni/vandalhearts_pc >/dev/null"
    APP="$PC_DIR/dist/VandalHearts-x86_64.AppImage"
    [ -f "$APP" ] || die "container build produced no AppImage"
    cp "$APP" "$STAGE/VandalHearts-$TAG-linux-x86_64.AppImage"
    cp "$INI" "$STAGE/vandalhearts.ini"
    log "  -> VandalHearts-$TAG-linux-x86_64.AppImage + vandalhearts.ini"
fi

# ---- Player Manual PDF (release asset; source = docs/manual/, built via pandoc + chromium) ------
log "Manual: building the Player Manual PDF"
"$PC_DIR/tools/build-manual.sh" "$TAG" "$STAGE/VandalHearts-$TAG-Manual.pdf" \
    || die "manual build failed (pandoc + chromium needed -- see tools/build-manual.sh)"

# ---- optional HD pack (a SEPARATE release asset, not embedded in any binary) -------------------

# --hdpack=<dir> (or VH_HDPACK_DIR) points at an assembled hdpacks/ folder: backgrounds/*.webp +
# videos/<sector>.mp4 + manifest.json -- upscaled derivative art (see NOTICE) that you assemble
# yourself; the zip name below is what the checksum/upload steps expect.
if [ -n "$HDPACK_SRC" ]; then
    [ -d "$HDPACK_SRC" ]              || die "--hdpack: '$HDPACK_SRC' is not a directory"
    [ -f "$HDPACK_SRC/manifest.json" ] || die "--hdpack: no manifest.json in '$HDPACK_SRC' (assemble the pack first)"
    log "HD pack: packaging $HDPACK_SRC"
    nbg=$(ls -1 "$HDPACK_SRC"/backgrounds/*.webp 2>/dev/null | wc -l)
    nvid=$(ls -1 "$HDPACK_SRC"/videos/*.mp4 2>/dev/null | wc -l)
    PKGTMP="$STAGE/_hdpack"; rm -rf "$PKGTMP"; mkdir -p "$PKGTMP/hdpacks"
    cp -rL "$HDPACK_SRC"/. "$PKGTMP/hdpacks/"      # -L: dereference symlinks -> real bytes in the asset
    ( cd "$PKGTMP" && zip -q -r "$STAGE/VandalHearts-$TAG-hdpack.zip" hdpacks )
    rm -rf "$PKGTMP"
    log "  -> VandalHearts-$TAG-hdpack.zip ($nbg backgrounds + $nvid movies)"
    HDPACK_DONE=1
fi

# ---- checksums --------------------------------------------------------------
log "Checksums"
( cd "$STAGE" && sha256sum VandalHearts-* vandalhearts.ini 2>/dev/null > SHA256SUMS.txt || true )
cat "$STAGE/SHA256SUMS.txt" | sed 's/^/    /'

# ---- release notes ----------------------------------------------------------
NOTES="$STAGE/RELEASE_NOTES.md"

# Lead the release body with this version's changelog section: everything under
# "## [<ver>]" up to the next "## [" heading (the heading line itself dropped, since
# the release title already names the version). Best-effort; warns if absent.
VER="${TAG#v}"
WHATS_NEW="$(awk -v ver="$VER" '
    index($0, "## [" ver "]") == 1 { p=1; next }
    p && index($0, "## [") == 1    { exit }
    p                              { print }
' "$REPO/CHANGELOG.md" 2>/dev/null || true)"

cat > "$NOTES" <<NOTE
## Vandal Hearts — PC Port $TAG

A native PC port of Vandal Hearts — one executable for the USA (SLUS-00447), Asia
(SCPS-45183) and Japan (SLPM-86007) releases. **You must supply your own
legally-owned disc image** (\`.bin\`); the download does nothing without it.
NOTE

if printf '%s' "$WHATS_NEW" | grep -q '[^[:space:]]'; then
    printf '%s\n' "$WHATS_NEW" >> "$NOTES"
else
    log "WARN: CHANGELOG.md has no [$VER] section -- release notes omit the changelog."
fi

cat >> "$NOTES" <<NOTE

### Downloads
| Platform | File | How to run |
|---|---|---|
| Windows 10/11 | \`VandalHearts-$TAG-windows-x64.zip\` | Unzip; put your disc in a \`game\\\` folder next to \`vandalhearts_pc.exe\`; run it. |
| Linux (glibc ≥ 2.34) | \`VandalHearts-$TAG-linux-x86_64.AppImage\` + \`vandalhearts.ini\` | Put both together; put your disc in a \`game/\` folder beside the \`.AppImage\`; \`chmod +x\` and run. Needs FUSE2. |
| Any | \`VandalHearts-$TAG-Manual.pdf\` | The Player Manual: setup, controls, features, troubleshooting. |
| Optional | \`VandalHearts-$TAG-hdpack-SLUS-00447.zip\` | HD backgrounds + movies. Unzip so \`hdpacks/\` sits beside the executable. Loaded on **US/Asia discs**. |
| Optional | \`VandalHearts-$TAG-hdpack-SLPM-86007.zip\` | HD backgrounds + movies. Unzip so \`hdpacks/\` sits beside the executable. Loaded on **Japan**. |
NOTE
# HD packs are standing per-game release assets, so the rows above are unconditional;
# HDPACK_DONE still gates the validation/zip flow for a supplied --hdpack.
cat >> "$NOTES" <<NOTE

Config: edit \`vandalhearts.ini\` next to the executable (window scale, audio, etc.).

### Note on contents
A release binary embeds a portion of game-derived data (the executable's static
data segment, a small BIOS-derived font, reconstructed tables) so the port can
run — © Konami / © Sony, no ownership claimed. It is a fraction of the game; the
bulk loads from your own disc at runtime. See NOTICE/DISCLAIMER.

Verify downloads against \`SHA256SUMS.txt\`.

### Optional HD packs
Higher-resolution backgrounds + re-encoded FMV movies, one pack per game
(\`SLUS-00447\` for US/Asia, \`SLPM-86007\` for Japan). **Optional**; the game runs
identically without one. Unzip so \`hdpacks/\` sits next to the executable, then
enable **HD PACK** in the Select+Start options (see docs/hd-pack.md). Upgrading a
1.x install? Your old pack still works — or move the previous contents of
\`hdpacks/\` into a new \`hdpacks/SLUS-00447/\` folder to match the new layout.
This is upscaled derivative art (© Konami), provided for convenience and also
buildable from your own disc — see NOTICE / DISCLAIMER.
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
