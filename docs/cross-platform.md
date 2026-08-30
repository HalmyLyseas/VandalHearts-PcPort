# Cross-platform (Windows, Linux packaging & macOS)

The port targets Linux, Windows, and macOS. Linux and Windows have full-playthrough validation. The
native Apple Silicon build has been validated through the first battle and surrounding game systems
(see the end of this page). This is Stage 2.4 of the roadmap.

## The portability model

The backends are ordinary portable C over SDL2 / OpenAL and a thin host presentation layer, so most of the code is
platform-neutral. The OS-specific surface is small and concentrated in `platform/pc/src/pc_bootstrap.c`
plus a couple of build shims. The few things that genuinely differ per OS:

| Concern | Linux | Windows | macOS |
|---|---|---|---|
| Executable's own path (`PC_GetExePath`) | `/proc/self/exe` | `GetModuleFileNameA` | `_NSGetExecutablePath` |
| Reserve the fixed PSX RAM ranges | `mmap(MAP_FIXED)` | `VirtualAlloc` | not used; host-backed work buffers |
| Make read-only data writable at startup | `dl_iterate_phdr` + `mprotect` | PE section walk + `VirtualProtect` | dyld section walk + `mprotect` |
| Fault handler (safety net) | POSIX diagnostics; low-read fixup on i386 only | not implemented; source guards + startup remap | not implemented; crashes on an unguarded low read |
| Present the software framebuffer | SDL2 + OpenGL | SDL2 + OpenGL | SDL2 Metal renderer |

The Linux i386 fault handler is a load-bearing safety net for retail behavior that still performs
low/NULL reads; source-level `PC_PORT` guards cover known high-traffic sites, and the startup remap
separately handles read-only-data writes. Windows and macOS do not implement the low-read instruction
fixup. The PC string-table constructor therefore normalizes the 12 retail NULL slots and the added
entry-100 sentinel to a stable empty string on every host. That removes the known table-specific
crash, but any other unguarded low-pointer path will still crash on macOS. See
[memory-safety.md](memory-safety.md) and [known_issues.md](known_issues.md).

The host-backed sound work RAM, 1 KB PS1 Scratchpad RAM, movie ring, and string-table changes are gated by
`PC_PORT`, not by `__APPLE__`: they therefore change native runtime behavior on Linux and Windows as
well as macOS. The matching PS1 branches remain separate and byte-identical.

## Windows (MinGW-w64)

The Windows `.exe` is **cross-compiled from Linux or macOS** — no Windows machine is needed to build,
only to run. MinGW-w64 was chosen over MSVC precisely for this: it keeps a GCC frontend (so the existing
`-fsanitize`, `__attribute__`, and GCC-isms carry over) and can produce native Windows PE binaries
from Linux. Crucially it is **not** Cygwin — the output is an ordinary Win32 binary with no POSIX
emulation DLL; its only real dependencies are our own (SDL2, OpenAL) plus the MinGW runtime.

Since 2.0 the shipped Windows exe is the **unified** (all-regions) binary:
`packaging/build-unified-win.sh` runs the three clean CMake stages (US core blob, JP core blob,
launcher link) that mirror `make unified`. The single-region CMake build below remains the dev
shape.

### Toolchain

```sh
# the cross GCC (pulls binutils, CRT, headers, winpthreads)
#   Arch/CachyOS: pacman -S mingw-w64-gcc
# SDL2 + OpenAL + libwebp for the w64-mingw32 target (OpenGL's import lib ships with the toolchain)
#   Arch/CachyOS: paru -S mingw-w64-sdl2 mingw-w64-openal mingw-w64-libwebp   (into /usr/x86_64-w64-mingw32/)
cd platform/pc
# libav (HD FMV): the distro/AUR mingw ffmpeg is a SHARED "kitchen-sink" build whose avcodec DLL
# hard-imports 35+ codec DLLs (x264/x265/aom/vpx/dav1d/...), 60-100 MB to bundle. Instead build a
# minimal STATIC libav (H.264 decode + mov demux + swscale only) that links into the exe with NO
# ffmpeg DLLs -- ~2.4 MB of exe growth:
tools/build-ffmpeg-static.sh              # -> platform/pc/ffmpeg-mingw-static/
cmake -S . -B build_win -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-mingw-w64.cmake \
      -DCMAKE_PREFIX_PATH="$PWD/ffmpeg-mingw-static"
cmake --build build_win
```

The static libav is found through the toolchain file's `CMAKE_FIND_ROOT_PATH`: with find-root mode
`ONLY`, `find_library`/`find_path` ignore a host `CMAKE_PREFIX_PATH`, so the prefix is appended there
(`-DVH_MINGW_FFMPEG=<prefix>` or the `VH_MINGW_FFMPEG` environment variable, which `make-release.sh`
sets). At link time the four archives cross-reference each other, so they go inside a
`--start-group`/`--end-group` (static archives resolve left-to-right), plus `bcrypt` for avutil's
`BCryptGenRandom`. The generator's probe link gets the same `-L<prefix>/lib` (see
[building.md](building.md#the-data-segment-generators-width)).

The toolchain file (`cmake/toolchain-mingw-w64.cmake`) points CMake at the `x86_64-w64-mingw32`
compilers and their sysroot. Its Linux default remains `/usr/x86_64-w64-mingw32`; on Homebrew it asks
the compiler for the versioned sysroot and accepts an external dependency prefix through
`-DVH_MINGW_PREFIX=/path/to/prefix`. Win64 is LLP64 (`long` is 32-bit), which is harmless here because the
64-bit port already mapped PSX `long`→`int` (see [memory-safety.md](memory-safety.md)).

### What the port needed for Windows

Six things, all guarded so the Linux build is untouched:

1. **`pc_bootstrap.c`** — Win32 branches: `VirtualAlloc` for the fixed PSX RAM ranges, a
   `VirtualProtect` PE-section walk for the read-only-data remap, and all the POSIX
   signal/backtrace/ucontext code compiled out. Windows has no low-read emulation; known paths require
   source guards or normalized host data.
2. **`include/pc_win_compat.h`** — MinGW's `<sys/types.h>` lacks the BSD `u_char`/`u_short`/… aliases
   the clean-room PsyQ headers use (glibc supplies them); this shim defines them, force-included on
   Windows only.
3. **`libkernel.c`** — MinGW's `mkdir` takes one argument (`_mkdir`), not two.
4. **`pc_gpu_window.c`** — `SDL_SetMainReady()`, since the build defines `SDL_MAIN_HANDLED` (the entry
   point is the game's own `main`, not SDL's).
5. **The data-segment generator** — two cross-compile fixes: platform-correct link-probe library names
   (`-lopengl32`/`-lOpenAL32` vs `-lGL`/`-lopenal`), and a **host compiler for the `sizeof` probe**
   (that probe compiles *and runs* a binary to read struct sizes — a `.exe` can't run on the Linux
   build host, so it builds for the host instead; safe because the probed types are pointer-free). See
   [pc-port/data-segment.md](pc-port/data-segment.md).
6. **CMake** — drop `-rdynamic` (MinGW rejects it; no backtrace path anyway), a `-mconsole` build for
   bring-up so `stderr` is visible, and a post-build step that copies the runtime DLLs beside the
   `.exe`.

### The distributable

`build_win/` is self-contained for a stock Windows 10/11 machine — zip it and run:

```
vandalhearts_pc.exe          vandalhearts.ini
SDL2.dll  OpenAL32.dll        libwinpthread-1.dll
libgcc_s_seh-1.dll  libstdc++-6.dll  libssp-0.dll
libwebp-7.dll  libsharpyuv-0.dll
```

Eight DLLs ship. `SDL2.dll` + `OpenAL32.dll` are our own dependencies; the four `lib*` are the MinGW
runtime — `libwinpthread-1` (pthreads), `libgcc_s_seh-1` (GCC unwinder), `libstdc++-6` (C++ runtime,
pulled in by SDL2/OpenAL-soft), `libssp-0` (stack-protector); `libwebp-7` + `libsharpyuv-0` are the HD
background codec. **libav ships as zero DLLs** — it is statically linked (see the deps block above), so
the HD-video decoder is folded into the `.exe` (~2.4 MB) and the exe's only extra load-time import is
`bcrypt.dll`, a Windows OS component (avutil's RNG). None of the eight are OS DLLs, so all eight must
ship. Everything else the `.exe` imports — the UCRT `api-ms-win-crt-*`, `bcrypt`, `OPENGL32`,
`KERNEL32`, `USER32` — is an OS component. A missing `lib*` DLL fails at load with a
message box *before* any code runs and produces no log, which is the tell. The user drops their disc
`.bin` in a `game\` folder next to the `.exe` and double-clicks (see
[configuration.md](configuration.md)).

**Status: validated end-to-end** — a full demo (both intro FMVs, menu, attract-mode timeout, and a
complete battle) runs with no visual or gameplay defect, both RAM reservations succeed, and the
`.rodata` remap holds.

## Linux distribution (AppImage)

A native Linux build is just a dynamically-linked ELF: `make link` links it against the system
`libSDL2`, `libopenal`, and `libGL`. That is fine for *developers* (two ubiquitous packages), but it is
not a distributable — package names differ across distros, and the binary inherits the **glibc version
of whatever machine built it**, so a build from a bleeding-edge distro refuses to start on Debian
stable with `version 'GLIBC_2.XX' not found`.

Releases therefore ship as an **AppImage**: one self-contained file, matching the Windows-zip
experience. It is built inside a pinned **Debian 12 container** so its runtime requirements are fixed
and low. Since 2.0 the packaged binary is the **unified** (all-regions) build — the release script
runs `make unified` from clean inside the container.

### What an AppImage is (and the one code change it forced)

An AppImage is a single self-contained executable: a small runtime prepended to a squashfs image that,
when run, FUSE-mounts itself read-only at `/tmp/.mount_XXXX/` and execs the bundled binary from there.
It bundles the app's **private** shared libraries (SDL2, OpenAL and their non-system deps) while
deliberately leaving glibc, `libGL`, `libstdc++` to the host — those must match the running kernel/GPU
driver, not be frozen at build time.

Because the binary runs from a throwaway read-only mount, the usual "look for the disc next to the
executable" logic breaks — the user can't drop a `.bin` into a squashfs. The AppImage runtime exports
`$APPIMAGE` = the absolute path of the `.AppImage` file itself. So `pc_bootstrap.c` grew a
`PC_GetDeployDir()` helper: when `$APPIMAGE` is set, disc- and `vandalhearts.ini`-lookup use *its*
directory; otherwise they fall back to the executable's own directory (native Linux / Windows,
unchanged). This is the only source change AppImage required.

### Building a release

Releases are **always** built in the Debian 12 container, never on the developer's own distro. An
AppImage bundles the app's private libraries but deliberately leaves **glibc and libstdc++ to the
host**, so whatever versions the build machine has become the minimum the artifact demands. Building on
a current rolling-release distro yields an AppImage that will not start on Debian/Ubuntu stable —
typically failing on the bundled OpenAL, which is C++ and so picks up both new glibc *and* new
`GLIBCXX`/`CXXABI` symbols from a recent GCC.

A **distrobox + podman** container is used rather than a VM: it is headless, rootless, and shares
`$HOME`, so the repo *and* the data generator's inputs (`build/SLUS_004.47.elf`, `include/PsyQ/`,
KROMDAT) are visible at identical paths — nothing to copy — and the finished AppImage lands directly in
the host filesystem.

```bash
# --- on the host, once ---
sudo pacman -S --needed podman distrobox     # distrobox is only a frontend; podman is the engine
distrobox create --name vh-deb12 --image debian:12
distrobox enter vh-deb12

# --- inside the container, once ---
sudo apt update && sudo apt install -y build-essential python3 patchelf file wget \
     libsdl2-dev libopenal-dev libgl1-mesa-dev libwebp-dev binutils-mipsel-linux-gnu
# libav is NOT taken from apt: the release links a minimal STATIC libav (same as Windows) so the
# AppImage doesn't bundle the distro ffmpeg's 100+-library codec closure (~65MB -> ~14MB).
# make-release.sh clones + builds it automatically (cached at platform/pc/ffmpeg-linux-static/);
# manual: TARGET=native platform/pc/tools/build-ffmpeg-static.sh inside the container.
mkdir -p ~/bin      # Debian 12 packages neither tool; use the upstream continuous builds
wget -O ~/bin/appimagetool https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage
wget -O ~/bin/linuxdeploy  https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
chmod +x ~/bin/appimagetool ~/bin/linuxdeploy && export PATH="$HOME/bin:$PATH"

# --- build (same repo path as the host, shared home) ---
cd ~/git/vandalHearts_decomp/vh/platform/pc
make link BUILD_DIR=build_deb                                  # side-by-side; leaves the host's build/ alone
packaging/appimage/build-appimage.sh build_deb/vandalhearts_pc
```

The container can also be driven from the host without entering it interactively, which is handy for
scripted release builds:
`distrobox enter vh-deb12 -- bash -lc 'cd ~/git/.../platform/pc && make link BUILD_DIR=build_deb'`.

Notes:

- **`binutils-mipsel-linux-gnu` is required.** The data-segment generator shells out to
  `mipsel-linux-gnu-nm` to read symbol VRAM addresses from the byte-exact PSX ELF. Without it the build
  fails at generator step 5 (`FileNotFoundError: 'mipsel-linux-gnu-nm'`) *after* all native compilation
  has already succeeded — which reads like a code problem but is a missing tool.
- **`BUILD_DIR=build_deb`** keeps the container's objects in their own tree, clear of the host's (the
  Makefile forwards it to the generator as `VH_BUILD_DIR`). The game source builds clean on GCC 12 with
  no source changes.
- The script runs a two-tool pipeline, each doing one job (more robust than linuxdeploy's bundled
  `--output appimage` plugin chain): **`linuxdeploy`** populates the `AppDir` — copies the exe, bundles
  SDL2/OpenAL and their private deps, sets the binary's rpath to `$ORIGIN/../lib`, installs the
  `.desktop` + icon — then **`appimagetool`** packs it into `dist/VandalHearts-x86_64.AppImage`
  (~8.4 MB, zstd, ~40 bundled libraries).
- The script presets `NO_STRIP=1` (linuxdeploy's bundled `strip` aborts on the modern `.relr.dyn`
  relocation section) and `APPIMAGE_EXTRACT_AND_RUN=1` (so both tools run without FUSE in the
  container).
- Assets live in `platform/pc/packaging/appimage/`: `vandalhearts.desktop`, an **original** placeholder
  `vandalhearts.svg` icon (deliberately not copyrighted game art), and the build script.

### Running it

```
./VandalHearts-x86_64.AppImage
```

The user drops their disc image in a `game/` folder (or a bare `*.bin`) **next to the `.AppImage`**.
Running the AppImage needs FUSE2 on the host (`pacman -S fuse2` / `apt install libfuse2`); alternatively
`./VandalHearts-x86_64.AppImage --appimage-extract-and-run`.

**Config parity with Windows.** The build also copies the **same `vandalhearts.ini`** the Windows zip
ships into `dist/`, so a Linux release is two files (plus the user's disc): the `.AppImage` and the
`vandalhearts.ini` beside it. Because the AppImage runs from a read-only mount, the editable config
*can't* live inside it — it sits alongside, where `PC_GetDeployDir()` (via `$APPIMAGE`) looks. Same file,
same keys, same precedence (env var > ini > default) as every other platform, so e.g. uncommenting
`VH_SCALE=2` in that file resizes the window with no environment variables (verified end-to-end through
the AppImage: `PC_Config: VH_SCALE=3 (from vandalhearts.ini)` → a 960×720 window).

**Validated:** built in the Debian 12 container and played on an Arch host through the endgame — final
battle, post-battle cutscenes, FMVs, and the post-credits scene, with no rendering artifacts. The
binary's rpath resolves the bundled SDL2/OpenAL (`$ORIGIN/../lib`, no system-lib fallthrough).

### Runtime requirements (the glibc floor)

The container-built AppImage requires **glibc ≥ 2.34** and has **no `GLIBCXX`/`CXXABI` requirement at
all**. 2.34 is where glibc absorbed libpthread/libdl — the natural floor for anything built after 2021
— and it lands *below* Debian 12's own 2.36 because no bundled library reaches higher.

That covers **Debian 12 & 13, Ubuntu 22.04 & 24.04, Fedora 35+, RHEL 9, and Arch**. Only glibc-2.31-era
distros (Debian 11, Ubuntu 20.04) fall outside; swap the image for `ubuntu:20.04` if they are ever
needed.

To check the floor of any build:

```bash
./VandalHearts-x86_64.AppImage --appimage-extract
for f in squashfs-root/usr/bin/vandalhearts_pc squashfs-root/usr/lib/*.so*; do
    readelf -V "$f"; done | grep -oE '(GLIBC|GLIBCXX|CXXABI)_[0-9.]+' | sort -uV | tail -3
```

## Publishing a release

> Pre-flight first: the step-by-step checks live in **[releasing.md](releasing.md)** — this section
> is the mechanics.

`platform/pc/packaging/make-release.sh <tag>` builds both artifacts, checksums them, and publishes a
GitHub release via the `gh` CLI:

```
platform/pc/packaging/make-release.sh v1.0.0                 # build both, publish
platform/pc/packaging/make-release.sh v1.0.0 --no-publish    # stage only, print the gh command
platform/pc/packaging/make-release.sh v1.0.0 --windows-only  # (or --linux-only)
```

**Why a local script and not GitHub Actions.** The data-segment generator needs the byte-exact
`SLUS_004.47` + the PsyQ `KROMDAT.BIN` at build time to reconstruct the embedded game data. Those are
copyrighted and cannot live on GitHub's runners, so the binaries *must* be built on a machine that has
the user's own disc/BIOS. CI can't produce a runnable artifact; automation therefore covers packaging
and upload only. (And a release binary embeds a portion of game-derived data — see `NOTICE`.)

The script builds each artifact in the environment that gives the correct result:
- **Windows** — host MinGW-w64 cross-compile (`build_win`), zipped with its 8 runtime DLLs +
  `vandalhearts.ini` → `VandalHearts-<tag>-windows-x64.zip`.
- **Linux** — the AppImage from the pinned Debian 12 `vh-deb12` container (the glibc floor; a
  host-built AppImage would only run on distros as new as the host) → `VandalHearts-<tag>-linux-x86_64.AppImage`.

Both stage under `platform/pc/dist/release/<tag>/` (gitignored) with `SHA256SUMS.txt` and generated
`RELEASE_NOTES.md`. Prereqs: `mingw-w64-gcc`, the `vh-deb12` container (see *Building a release* above),
and `github-cli` authenticated (`gh auth login`).

## macOS — native and Universal 2 builds (community-supported, experimental)

macOS support is a community contribution and is validated by community members — the maintainer has
no Apple hardware, so macOS-specific issues are triaged best-effort and fixes may need a contributor
with a Mac. This repository's release assets cover Windows and Linux only; macOS builds might be
provided by community members on their own forks, outside this project's scope and not verified
here. The port now builds natively with AppleClang. Testing on Apple Silicon covers the first battle,
cutscenes, HD movies/backgrounds, world map, towns, shops, saves, Tactical Mode and 2× battle speed.
The SDL2 presentation layer explicitly selects Metal and the binary has no OpenGL framework dependency.
A dependency-minimal Universal 2 binary has passed the boot-to-title smoke test as both native arm64
and forced x86_64 under Rosetta on Apple Silicon. A source-only local `.app` recipe is implemented and
ad-hoc signing is validated for both slices. A complete Intel-hardware/full-game playthrough and Apple
notarisation have not been done.

```sh
brew install cmake sdl2 openal-soft webp ffmpeg

cd platform/pc
cmake -S . -B build-macos \
  -DCMAKE_PREFIX_PATH="$(brew --prefix sdl2);$(brew --prefix openal-soft);$(brew --prefix webp);$(brew --prefix ffmpeg)" \
  -DVH_PSX_EXE=/absolute/path/to/SLUS_004.47 \
  -DVH_KROM_SOURCE=/absolute/path/to/SCPH5500.BIN
cmake --build build-macos -j

VH_DISC_IMAGE=/absolute/path/to/Vandal_Hearts_USA.bin \
  ./build-macos/vandalhearts_pc
```

The build-time generators require a byte-exact US PS1 executable and either `KROMDAT.BIN` or a
Japanese PS1 BIOS containing the kanji ROM; supply these from legally owned copies. Apple Silicon
cannot use the Linux port's fixed low-address work buffers because arm64 Mach-O reserves the low
4 GB. The native port therefore uses host storage for those buffers while preserving PS1 offsets.
WebP backgrounds and FFmpeg HD video are enabled by default. They can be omitted from a minimal build
with `-DVH_WEBP=OFF -DVH_HDVIDEO=OFF`.

For the reproducible Universal 2 command, official SDL framework hash, external game-data layout, and
local signing recipe, see [`platform/pc/packaging/macos/README.md`](../platform/pc/packaging/macos/README.md).
The generated executable embeds data reconstructed from the user's game/BIOS inputs, so generated apps
remain local and gitignored; the recipe never uploads an app or places a disc, BIOS, HD pack, or save
inside the signed bundle.
