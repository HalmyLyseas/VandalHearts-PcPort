#!/usr/bin/env bash
#
# make-release.sh <version-tag> [--windows-only|--linux-only] [--no-publish] [--hdpack=<dir>]
#
#   ./make-release.sh v1.0.0
#   ./make-release.sh v1.6.0 --hdpack=/path/to/assembled/hdpacks   # + optional HD pack asset
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
                                              # (e.g. "Unchanged since v1.6.1 — keep yours.")
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

# The AppImage stage runs `rm -rf build-uni*` (clean builds are a hard release rule). A live test
# DEPLOYMENT (disc images, hdpacks/langpacks, saves) parked inside build-uni/ would be deleted
# with it -- which happened once (2026-08-22, v2.0.0 staging: the dev deployment was wiped and had
# to be restored from external/ + work-dir copies). Refuse to run while user data sits there.
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
    # Release binaries are optimized (-O2, matching the validated `build_opt`). The default CMake/Make
    # build is -O0 -g for debugging; the internal-resolution rasterizer (1.5) needs -O2 to hold 30 fps,
    # so the release MUST override it. -DCMAKE_C_FLAGS=-O2 adds -O2 on top of the default -g.
    # VH_MINGW_FFMPEG points the toolchain's CMAKE_FIND_ROOT_PATH at the static libav prefix (a host
    # CMAKE_PREFIX_PATH is ignored under MinGW's find-root mode ONLY).
    # Release builds are CLEAN builds. Plain Make does not track compiler-flag/include-path changes,
    # so an incremental build can silently link objects compiled against one library era with
    # archives from another -- exactly the 1.6.1 AppImage crash: a build_deb pc_hdvideo.o compiled
    # against the container's shared libav-59 headers got linked into the static libav-61 binary
    # (mismatched struct offsets -> SEGV in avcodec_parameters_to_context). Never ship incremental.
    # P5 (exchange/104): the shipped Windows exe is the UNIFIED binary (both regions, runtime
    # disc selection). build-unified-win.sh does the three clean CMake stages (us core, jp core,
    # final link) with these exact toolchain conventions.
    VH_MINGW_FFMPEG="$FFPREFIX" "$PC_DIR/packaging/build-unified-win.sh" >/dev/null
    WIN_EXE="$PC_DIR/build_win_uni/vandalhearts_pc.exe"
    [ -f "$WIN_EXE" ] || die "Windows build produced no .exe"
    WZIP_DIR="$STAGE/win"; mkdir -p "$WZIP_DIR"
    cp "$WIN_EXE" "$WZIP_DIR/"
    # Ship the exe stripped: debug info has no runtime use on Windows (no backtrace machinery)
    # and needlessly embeds local build metadata; stripping also cuts the download size. The
    # unstripped exe stays in build_win/ for local debugging. The guard keeps the shipped copy
    # free of local paths permanently.
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
    # 1.6 libav: link a minimal STATIC libav (same as Windows) instead of the distro's shared ffmpeg.
    # A shared libav drags its full codec closure into the AppImage (100+ .so, ~65MB vs ~20MB) for a
    # 15fps movie decode. Built once inside the container (its gcc sets the ABI floor) from a source
    # tree cloned on the HOST (the container has no git); cached at ffmpeg-linux-static/.
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
        # P5 (exchange/104): ship the UNIFIED binary (both regions). Clean all three stages.
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
# videos/<sector>.mp4 + manifest.json. The pack is upscaled DERIVATIVE art (see NOTICE); it is NOT
# built here -- you supply the finished, metadata-stripped folder. Named VandalHearts-$TAG-hdpack.zip
# so the checksum + upload globs below include it automatically.
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
# (2.0: HD packs are standing per-game release assets -- the rows above are unconditional. The
# legacy --hdpack single-zip row is retired; HDPACK_DONE still gates the validation/zip flow.)
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
