# Building

There are two independent builds, matching the two layers in [architecture.md](architecture.md):

- **The matching decompilation** — rebuilds the original `SLUS_004.47` and proves byte-identity. Uses
  a period-correct MIPS toolchain; you only need this to verify the decomp or work on `src/`.
- **The native PC port** — builds the playable desktop binary. Uses a normal host compiler + SDL2 /
  OpenAL and Metal (macOS) or OpenGL (other targets). This is what most contributors build.

Both builds reconstruct a few game/BIOS-derived files locally from your own copy of the game; nothing
copyrighted is committed (see [NOTICE](../NOTICE) and [pc-port/data-segment.md](pc-port/data-segment.md)).

---

## The matching decompilation

### What you need

| Piece | What it is |
|---|---|
| [splat](https://github.com/ethteck/splat) | disassembly / asset extraction driver |
| [maspsx](https://github.com/mkst/maspsx) | post-processes GCC 2.x asm for the PSX assembler quirks |
| old GCC 2.x frontends (`cc1_v263`, `cc1_v257`) | period compiler, prebuilt by [decompals/old-gcc](https://github.com/decompals/old-gcc) |
| a MIPS binutils (`as`/`ld`/`objcopy`) | any `mipsel-linux-gnu-*` works — the vendor triple isn't load-bearing |
| Python 3 | drives splat and the generators |
| PsyQ v3.3 SDK headers → `include/PsyQ/` | proprietary Sony headers, **gitignored, sourced locally** — never committed |
| `SLUS_004.47` (your own dump) | the original executable, the extraction input and the MD5 reference |

The exact, machine-specific setup recipe (where each tool was sourced, the `PATH`/`CROSS`/`PYTHON`
overrides this environment needs) lives in the `decomp-build` skill (`.claude/skills/decomp-build/`).
That skill is the authoritative setup reference; this page is the overview.

On macOS, Homebrew supplies native `mipsel-linux-gnu-binutils`, but the exact decompals GCC 2.5.7
and 2.6.3 PSX frontends are 32-bit i386 **Linux ELF** executables. Rosetta runs Intel Mach-O, not
Linux ELF, so the byte-matching build still needs Linux (a VM/container or a Linux build host).
Do not substitute the available GCC 2.7/2.8 macOS builds: compiler version and code generation are
part of the byte-identity check.

### Build and verify

```sh
# from the repo root
make extract        # splat: disassemble the original into asm/ + stub .c
make check          # rebuild SLUS_004.47 and md5-compare it to the original
```

`make check` is the **only correctness signal**: it rebuilds the executable and compares its MD5 to
the original (`596bb082a2de5f1fe977dd3d7e160b03`). A match means every decompiled function still
reproduces the original bytes. **Run it after any change to `src/` or `include/`** — an ungated edit
that shifts a struct or array silently breaks the match (see the gating conventions in
[architecture.md](architecture.md#how-the-port-avoids-breaking-the-match)).

A handful of files need per-file compiler overrides (e.g. `core/audio.c` builds with `cc1_v257 -O2 -G0`);
these are already in the `Makefile`. Follow that pattern rather than changing global flags.

---

## The native PC port

Everything below runs from `platform/pc/`.

### What you need

- A host C compiler (GCC or Clang).
- **SDL2**, **OpenAL** (e.g. OpenAL Soft), **OpenGL** on Linux/Windows, **libwebp**, and **libav / ffmpeg** (`libavformat`,
  `libavcodec`, `libavutil`, `libswscale`) development libraries. libwebp + libav power the 1.6 HD pack
  (`.webp` backgrounds, `.mp4` movies); build without them via `make link NO_WEBP=1 NO_HDVIDEO=1` /
  `cmake -DVH_WEBP=OFF -DVH_HDVIDEO=OFF`, which drops HD-pack support (native `.hdi`/MDEC fallbacks).
- **Python 3** (runs the data generators during the build).
- Your own `SLUS_004.47` and the game disc `.bin` (the generators read the former; the game reads the
  latter at runtime — see [configuration.md](configuration.md)).
- Optional: a 32-bit toolchain, only for the `-m32` reference build.

### Two interchangeable build systems

Both produce the same binary. The Makefile is the original; CMake was added for cross-platform work
and is what the Windows cross-compile uses.

```sh
# --- Makefile ---
make link                              # -> build/vandalhearts_pc          (64-bit, default)
make link M32=-m32 BUILD_DIR=build32   # -> build32/vandalhearts_pc        (32-bit reference build)

# --- CMake ---
cmake -S . -B build_cmake
cmake --build build_cmake              # -> build_cmake/vandalhearts_pc
```

**64-bit is the default.** The port was deliberately 32-bit for most of its life as a debugging
baseline — see [memory-safety.md](memory-safety.md) for why, and why the 32-bit build is still kept
as an A/B reference. Both build systems run the mid-build data-segment generator; see
[pc-port/data-segment.md](pc-port/data-segment.md).

**Optimization.** Both build systems default to `-O0 -g` (unoptimized, for debugging). The internal-
resolution rasterizer (1.5) needs optimization to hold the frame cap, so build with `-O2` for anything
perf-sensitive: `make link CC="cc -O2"` or `cmake … -DCMAKE_C_FLAGS=-O2`. The **release packaging
(`make-release.sh`) always builds `-O2`** on both platforms, so shipped binaries are optimized.
This split is deliberate policy, not drift: dev builds stay `-O0` for exact breakpoints and readable
crash backtraces (most maintenance work starts from a bug report), while every shipped artifact is
`-O2` from a clean tree — so use an explicit `-O2` build for any performance measurement, never the
dev default.

Run it, and point it at your disc:

```sh
cd build
# put your disc .bin in a "game" folder next to the binary, or set the path explicitly:
VH_DISC_IMAGE="/path/to/Vandal Hearts (USA).bin" ./vandalhearts_pc
```

See [configuration.md](configuration.md) for the disc auto-detect rules, the `vandalhearts.ini` config
file, and all runtime options.

### Sanitizer builds

The Stage 2.3 memory-safety work leans on these (see [memory-safety.md](memory-safety.md)):

```sh
make asan32                                        # 32-bit AddressSanitizer build (run ./run_asan.sh)
make ubsan                                         # UBSan build (works at 64-bit)
cmake -S . -B build_ubsan -DVH_SANITIZE="-fsanitize=bounds -fno-omit-frame-pointer"
```

AddressSanitizer must be **32-bit** here: at 64-bit its shadow memory collides with the `0x80000000`
PSX-RAM arena the port reserves. UBSan has no shadow, so it works at 64-bit.

---

## Cross-compiling for Windows

A 64-bit Windows `.exe` is cross-compiled from Linux or macOS with the MinGW-w64 toolchain — no
Windows machine is needed to build (only to run). Full detail in
[cross-platform.md](cross-platform.md).

```sh
# toolchain: mingw-w64-gcc (pulls binutils/crt/headers/winpthreads)
#            + SDL2 and OpenAL built for the w64-mingw32 target (e.g. AUR mingw-w64-sdl2 / -openal)
cd platform/pc
cmake -S . -B build_win -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-mingw-w64.cmake
cmake --build build_win
```

On macOS, install Homebrew `mingw-w64`, cross-build SDL2/OpenAL into an external target prefix, and
pass that prefix as `-DVH_MINGW_PREFIX=/path/to/prefix`. The toolchain obtains Homebrew's versioned
compiler sysroot from `x86_64-w64-mingw32-gcc -print-sysroot`; no Homebrew path is committed.

The result is a self-contained `build_win/` folder: `vandalhearts_pc.exe`, `vandalhearts.ini`, and the
runtime DLLs (SDL2, OpenAL, and the MinGW runtime) copied in beside it — ready to zip and run on a
stock Windows 10/11 machine. For the native Apple Silicon build, see
[cross-platform.md](cross-platform.md#macos-apple-silicon--native-build).
