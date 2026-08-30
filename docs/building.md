# Building

There are two independent builds, matching the two layers in [architecture.md](architecture.md):

- **The matching decompilations** — rebuild the original executables and prove byte-identity: the
  US `SLUS_004.47` from the repo root, and (since 2.0) the Japanese `SLPM_860.07` from the `jp/`
  tree, each with its own `make check`. Uses a period-correct MIPS toolchain; you only need this
  to verify the decomps or work on `src/` / `jp/src/`.
- **The native PC port** — builds the playable desktop binary. Uses a normal host compiler + SDL2 /
  OpenAL and Metal (macOS) or OpenGL (other targets). This is what most contributors build. The
  release shape is the **unified binary** (`make unified`), which carries both regional builds;
  `make link` builds a single-region binary for development.

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
cd jp && make check # same for the Japanese SLPM_860.07 (its own tree, same recipe)
```

`make check` is the **only correctness signal**: it rebuilds the executable and compares its MD5 to
the original (US `596bb082a2de5f1fe977dd3d7e160b03`, JP `53849277b08184863bd45f10925995a6`). A
match means every decompiled function still reproduces the original bytes. **Run it after any
change to `src/` or `include/`** (and the `jp/` twin when a shared file changes) — an ungated edit
that shifts a struct or array silently breaks the match (see the gating conventions in
[architecture.md](architecture.md#how-the-port-avoids-breaking-the-match)). Most game files are
byte-shared between the two trees; the port build verifies the shared set stays identical
(`make check-shared`).

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
make unified                           # -> build-uni/vandalhearts_pc      (ALL regions -- the release shape)
make link                              # -> build/vandalhearts_pc          (US-only dev build, 64-bit)
make link REGION=jp                    # -> build-jp/vandalhearts_pc       (JP-only dev build)
make link M32=-m32 BUILD_DIR=build32   # -> build32/vandalhearts_pc        (32-bit reference build)

# --- CMake ---
cmake -S . -B build_cmake
cmake --build build_cmake              # -> build_cmake/vandalhearts_pc
```

`make unified` compiles both regional cores into per-region blobs and links them under one thin
launcher (disc scan → region classify → dispatch); only one core ever executes per process. The
single-region `make link` builds stay the everyday dev shape (faster iteration, direct symbols).

**64-bit is the default.** The port was deliberately 32-bit for most of its life as a debugging
baseline — see [memory-safety.md](memory-safety.md) for why, and why the 32-bit build is still kept
as an A/B reference. Both build systems run the mid-build data-segment generator; see
[pc-port/data-segment.md](pc-port/data-segment.md). The generator refuses a wrong-region or truncated
PS-X executable (size + md5 check against the known retail hashes) before it slices any data out of
it; `VH_ALLOW_UNVERIFIED_EXE=1` overrides the hash check for a deliberately modified executable.

### Optimization

Both build systems default to `-O0 -g` (unoptimized, for debugging). The internal-resolution
rasterizer needs optimization to hold the frame cap, so build with `-O2` for anything
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

### How the port build works

Both build systems implement the same recipe; the Makefile is the reference and `CMakeLists.txt`
mirrors it group by group. `tools/check_build_parity.sh` (run by the release script) proves the two
source lists still name every `platform/pc/src/*.c`; a file present in only one list breaks only the
platform that uses the other system, so add new backend files to **both**.

#### The header-staging tree

Game sources are compiled with `-I<build>/include_stage`, a tree of symlinks, instead of
`-I../../include`. GCC resolves a quoted `#include "PsyQ/x.h"` relative to the *including file's own
directory* before it consults any `-I` path, and the project headers under `include/` include the
PsyQ headers that way — so with a plain `-I` the real (gitignored, proprietary) Sony headers in
`include/PsyQ/` would always win over the clean-room ones in `platform/pc/include/PsyQ/`. The staging
tree links every project header in individually and points its `PsyQ/` entry at the clean-room set,
which has the same effect as replacing `include/PsyQ/` without touching the decomp tree. The tree is
rebuilt on every build (Makefile `stage:`; CMake at configure time — reconfigure after adding a header).
Backend files that read game structs (`libetc.c`, `libsnd.c`, `pc_balance.c`, `pc_overlay.c`, the
`pc_*_data.c` reconstructions) compile through the same staged tree.

#### Game-source compile flags

The decompiled game source is 1996 C and compiles under a fixed profile (`GAME_C_FLAGS` / the CMake
per-source properties):

| Flag | Why |
|---|---|
| `-std=gnu89` | the tolerant C89/K&R semantics of the period compiler; implicit declarations are a hard error under GCC 14's default standard |
| `-fno-builtin-csqrt` | the game's own `csqrt(int)` (a GTE symbol) collides with glibc's complex-number builtin |
| `-DPERMUTER` | makes `include/include_asm.h` turn `INCLUDE_ASM`/`INCLUDE_RODATA` into no-ops instead of raw MIPS `__asm__` directives — the escape hatch the decomp already provides for building off-MIPS |
| `-D'asm(x)='` | neutralises the two `register T v asm("reg")` binding hints (decomp artifacts preserving retail codegen); the separate `__asm__` token is untouched |
| `-I<project root>` | resolves the sources' quoted `#include "assets/NNNNNNNN.inc"` lines |
| `-include pc_forward_decls.h` | two functions are called before their definition with a return type an implicit `int` forward reference cannot match |
| `-DPC_PORT`, `-DPC_FEAT`, `-DVH_REGION_*` | the port gates; defined only here, never by the matching build |

CMake folds `-D'asm(x)='` and the `-include` into one force-included prelude
(`include/cmake_game_prelude.h`): per-source CMake options drop function-like macros and de-duplicate
repeated `-include` flags, so a single `-include` is the only reliable form.
Under CMake, `set_source_files_properties(... COMPILE_OPTIONS ...)` *overwrites* the property, so
the `PORT_WARN` append for backend files must come after every per-file profile is set, or it is
silently dropped.

**Warning policy.** Game sources compile with `GAME_WARN`, which silences their known-benign 1996-C
warning classes (K&R declarations vs prototypes, PSX pointer-width idioms). Port code compiles with
`PORT_WARN` (`-Wall -Wextra`) and must stay warning-clean — a new warning there is signal. Keep the two
sets apart. `PORT_WARN` also carries the region define, so every backend rule sees `VH_REGION_*` and
`pc_platform.h` can never silently default to US values in a JP build.

#### Header dependency tracking

Every compile passes `-MMD -MP`, and the Makefile's final line `-include`s the emitted `.d` files as
extra prerequisites. Without this a rule depends only on its `.c`, so a header edit rebuilds nothing
and the build "succeeds" while still containing the old code. `-MP` adds phony targets for the headers
so a deleted or renamed header does not wedge the build. Compiler *flags* are not tracked by either
build system, which is why the release builds from clean (see [releasing.md](releasing.md)).

A GNU Make trap that bit the debug-flag rules: a **second** target-specific `+=` for the same target
appends to the *global* value and drops the first, so two `$(BUILD_DIR)/src/core/object.o: GAME_C_FLAGS
+= …` lines silently lose one set of defines. Put every define for one object on a single line.

#### Region source selection

`REGION=us` (default) compiles `../../src`; `REGION=jp` compiles into `build-jp/` from the `jp/` tree.
The TUs listed in `platform/pc/shared_tus.txt` are byte-identical between the trees once the PC gate
blocks are stripped, so **both** regions compile them from the US tree and every gated fix lands once;
the JP build takes the remaining TUs from `jp/src`. Of the headers, four genuinely differ:
`audio.h`/`card.h`/`units.h` are staged from `jp/include` as-is (they need no gates), and `field.h`
from `platform/pc/include/region-jp/field.h`, a hand-merged copy that keeps the US gate blocks.
`make check-shared` (`tools/check_shared.py`) proves the shared set is still identical; a file that
diverges must leave `shared_tus.txt` and compile per region. The JP retail binary carries two names
for one address (`DrawSjisText`/`DrawText`, `MsgBox_IsFinished`/`ConsumeMsgBoxPagePause`), satisfied
at link time with `--defsym`.

The language-pack engine is US-only (it is built on the US ASCII text path); the JP core links
`pc_lang_stub.c`, the engine's documented no-pack behaviour, in its place. The JP build also
regenerates every data reconstruction (`pc_battle_data.c`, `pc_kanji_font.c`, …) from `SLPM_860.07`
into its build tree with the region-parametric generators; the `src/` copies are the US originals.
The JP kanji font is the full charset-2+3 ROM set (3489 glyphs) because the JP game draws its whole
text repertoire through `Krom2RawAdd`; the US build embeds only the 209-glyph subset it uses.

#### The unified binary

`make unified` builds both region cores with `UNIFIED=1` into their own build directories (the flags
differ, and stale objects are a real hazard), partial-links each region's entire object set into one
relocatable blob (`ld -r`), renames every **defined** external symbol with a `us_`/`jp_` prefix
(`objcopy --redefine-syms`), and links both blobs under `src/pc_region_main.c`. Weak and common
symbols must be renamed too: a tentative definition left unrenamed would silently merge across the two
blobs. On COFF the `.refptr.*` thunks are filtered from the rename (`tools/prefix_blob.py`). `UNIFIED`
compiles out `pc_bootstrap.c`'s bootstrap constructors; `pc_region_main.c` calls those pieces
explicitly after classifying the disc. Only one core executes per process; the other blob's data-init
constructors run harmlessly on their own renamed globals.

#### The data-segment generator's width

`tools/build_data_segment.py` probes real `sizeof()` values to slice the data segment out of the PSX
ELF, so it must run at the same pointer width as the build: every pointer-containing struct changes
size (`Object` is 96 bytes at `-m32`, 160 at `-m64`), and a mismatch does not fail — it silently emits
wrongly-sized objects. The Makefile passes its own `$(M32)` as `VH_TARGET_MARCH` and `VH_BUILD_DIR`
so 32- and 64-bit trees coexist. Under CMake `VH_TARGET_MARCH` is omitted (the generator uses the
native width of `VH_CC`), and environment variables are assembled into a list so that only non-empty
ones are passed: `cmake -E env NAME=` with an empty value makes CMake treat the next token as the
command, which stops the generator from ever running. Every `-l` on the generator's probe link must
resolve — `ld: cannot find -lXXX` aborts the probe before any "undefined reference" line is emitted,
and the generator then produces an *empty* data segment; hence the per-platform library names
(`-lGL`/`-lopengl32`, `-lopenal`/`-lOpenAL32`, the static-libav `-L` prefix on Windows).

#### Debug build flags

Per-file `PC_DEBUG_*` hooks (`AI_LOG=1`, `SPRITE_LOG=1`, `NO_FADE=1`, …) are listed in
[`platform/pc/OPTIONS.md`](../platform/pc/OPTIONS.md). The `PC_DEBUG_UI_LOG` and `PC_DEBUG_SPRITE_LOG`
probes are compiled in unconditionally and gate themselves at runtime on `VH_*` variables, so there is
no compile-time switch to forget. The matching build never defines any of them.

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
