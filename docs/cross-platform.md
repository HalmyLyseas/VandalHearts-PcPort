# Cross-platform (Windows, Linux packaging & macOS)

The port targets Linux, Windows, and — in principle — macOS. Linux and Windows are both built and
validated; macOS is scaffolded but not pursued (see the end of this page). This is Stage 2.4 of the
roadmap.

## The portability model

The backends are ordinary portable C over SDL2 / OpenAL / OpenGL, so most of the code is
platform-neutral. The OS-specific surface is small and concentrated in `platform/pc/src/pc_bootstrap.c`
plus a couple of build shims. The three things that genuinely differ per OS:

| Concern | Linux | Windows | macOS |
|---|---|---|---|
| Executable's own path (`PC_GetExePath`) | `/proc/self/exe` | `GetModuleFileNameA` | `_NSGetExecutablePath` |
| Reserve the fixed PSX RAM ranges | `mmap(MAP_FIXED)` | `VirtualAlloc` | `mmap` (untested) |
| Make read-only data writable at startup | `dl_iterate_phdr` + `mprotect` | PE section walk + `VirtualProtect` | dyld walk — **stub** |
| Fault handler (safety net) | POSIX `sigaction` | not needed (see below) | not implemented (ARM) |

The fault handler is a *net*, not the primary mechanism: the 64-bit build absorbs transient NULL reads
with source-level `PC_PORT` guards, and the startup remap handles read-only-data writes. So Windows
needs no signal machinery at all to run — the POSIX handler is simply compiled out there. See
[memory-safety.md](memory-safety.md).

## Windows (MinGW-w64)

The Windows `.exe` is **cross-compiled from Linux** — no Windows machine is needed to build, only to
run. MinGW-w64 was chosen over MSVC precisely for this: it keeps a GCC frontend (so the existing
`-fsanitize`, `__attribute__`, and GCC-isms carry over) and can produce native Windows PE binaries
from Linux. Crucially it is **not** Cygwin — the output is an ordinary Win32 binary with no POSIX
emulation DLL; its only real dependencies are our own (SDL2, OpenAL) plus the MinGW runtime.

### Toolchain

```sh
# the cross GCC (pulls binutils, CRT, headers, winpthreads)
#   Arch/CachyOS: pacman -S mingw-w64-gcc
# SDL2 + OpenAL for the w64-mingw32 target (OpenGL's import lib ships with the toolchain)
#   Arch/CachyOS: paru -S mingw-w64-sdl2 mingw-w64-openal   (install into /usr/x86_64-w64-mingw32/)
cd platform/pc
cmake -S . -B build_win -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-mingw-w64.cmake
cmake --build build_win
```

The toolchain file (`cmake/toolchain-mingw-w64.cmake`) points CMake at the `x86_64-w64-mingw32`
compilers and the sysroot. Win64 is LLP64 (`long` is 32-bit), which is harmless here because the
64-bit port already mapped PSX `long`→`int` (see [memory-safety.md](memory-safety.md)).

### What the port needed for Windows

Six things, all guarded so the Linux build is untouched:

1. **`pc_bootstrap.c`** — Win32 branches: `VirtualAlloc` for the fixed PSX RAM ranges, a
   `VirtualProtect` PE-section walk for the read-only-data remap, and all the POSIX
   signal/backtrace/ucontext code compiled out (the 64-bit build needs no fault handler).
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
```

The three "lib*" DLLs are the MinGW GCC/pthreads/stdc++ runtime (not Windows system DLLs, so they
ship); everything else the `.exe` imports — the UCRT `api-ms-win-crt-*`, `OPENGL32`, `KERNEL32`,
`USER32` — is an OS component. The user drops their disc `.bin` in a `game\` folder next to the `.exe`
and double-clicks (see [configuration.md](configuration.md)).

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
and low.

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
     libsdl2-dev libopenal-dev libgl1-mesa-dev binutils-mipsel-linux-gnu
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

## macOS (Apple Silicon) — scaffolded, not pursued

macOS is **deliberately deprioritized**. Unlike Windows, it can't be cleanly cross-compiled from Linux:
it needs Apple's macOS SDK (licensed to Apple hardware), an `osxcross`-style toolchain built from an
extracted Xcode SDK (legally gray and fiddly), and Apple-Silicon binaries additionally require
code-signing to launch — none of which is clean off a Mac. Realistically it would be a *native* clang
build on the Mac itself, not a cross-compile.

The groundwork is nonetheless in place, at no cost to the other targets: the `__APPLE__` branches
(`_NSGetExecutablePath`, the `mmap` reservations) and guarded platform includes already exist. What
remains for anyone who wants to finish it on-device:

- Implement `PC_MakeRodataWritable` for macOS — a dyld segment walk (`_dyld_get_image_header` +
  `getsegbyname` on `__DATA`/`__DATA_CONST`) + `mprotect`. This is the one real code gap; it's marked
  as a stub in `pc_bootstrap.c`.
- The fault-handler safety net is x86-specific (it reads the x86 page-fault write bit); on ARM it would
  need its own port, but with the startup remap done it may not be needed.
- `brew install sdl2 openal-soft`, a CMake toolchain/preset for `arm64-apple-darwin`, and the usual
  per-OS validation of window/audio/paths.

Treat macOS as community/future work, not an active target.
