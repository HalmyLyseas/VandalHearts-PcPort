# Architecture

This project has two distinct layers, kept deliberately separate:

1. A **matching decompilation** of the US PlayStation 1 release of *Vandal Hearts* (`SLUS_004.47`)
   — C source that recompiles to a byte-for-byte identical copy of the original executable.
2. A **native PC port** built on top of that source — every PlayStation hardware dependency replaced
   with a portable equivalent (SDL2 + Metal/OpenGL + OpenAL), so the game runs on a modern desktop from its
   own disc data.

Understanding why these are separate, and how the port avoids disturbing the match, is the key to
working in this codebase.

## Layer 1 — the matching decompilation

The decompilation is the foundation. Its single job is fidelity: the C in `src/` must compile (with a
period-correct GCC 2.x toolchain, via [splat](https://github.com/ethteck/splat) and
[maspsx](https://github.com/mkst/maspsx)) back into the exact bytes of the retail `SLUS_004.47`.
`make check` proves this by rebuilding the executable and comparing its MD5 to the original
(`596bb082a2de5f1fe977dd3d7e160b03`).

- **1184 functions across 70 `src/*.c` files** are decompiled and matching. PsyQ SDK library
  functions (Sony's proprietary runtime) are intentionally left as raw asm — they aren't the target
  of the decomp, and on PC they're replaced wholesale (see Layer 2).
- Because the goal is byte-identity, `src/` is written to match the *original compiler's output*, not
  to be idiomatic or portable. Struct layouts, field order, and even some otherwise-dead data regions
  are fixed by what the original binary contains. **Do not "clean up" `src/` for readability or
  portability** — that is Layer 2's concern, and doing it in `src/` breaks the match.

See [building.md](building.md) for how the matching build is produced and verified.

## Layer 2 — the native PC port (de-consolization)

The port lives under `platform/pc/` and turns the console-bound matching build into a program that
runs natively. The strategy is a **swappable hardware-interface layer**: the game calls the same PsyQ
SDK entry points it always did (`DrawOTag`, `RotTransPers`, `CdRead`, `SsSeqPlay`, `VSync`, …), but on
PC those names resolve to portable C reimplementations instead of Sony's asm.

```
   src/*.c  (byte-exact game logic — UNCHANGED)
      │  calls PsyQ SDK entry points by name
      ▼
   platform/pc/include/PsyQ/*.h   (clean-room headers: the same signatures)
      │
      ▼
   platform/pc/src/lib*.c         (portable reimplementations)
      │
      ▼
   SDL2 (window/input) · Metal on macOS / OpenGL elsewhere (blit) · OpenAL (audio output)
```

The six subsystem backends each replace one PSX hardware unit:

| PSX unit | PsyQ module | PC backend | Deep-dive |
|---|---|---|---|
| GPU (rasteriser) | `libgpu` | `libgpu.c` + `pc_raster.c` + `pc_hdpack.c` + `pc_gpu_trace.c` + `pc_gpu_window.c` | [subsystems/gpu.md](pc-port/subsystems/gpu.md) |
| GTE (geometry coprocessor) | `libgte` | `libgte.c` | [subsystems/gte.md](pc-port/subsystems/gte.md) |
| SPU (sound) | `libspu` / `libsnd` | `libspu.c` / `libsnd.c` + `pc_spu.c` | [subsystems/spu.md](pc-port/subsystems/spu.md) |
| CD-ROM / XA audio | `libcd` | `libcd.c` + `pc_xa.c` | [subsystems/cd-xa.md](pc-port/subsystems/cd-xa.md) |
| MDEC (video decode) | `libpress` | `pc_mdec.c` | [subsystems/mdec.md](pc-port/subsystems/mdec.md) |
| Kernel / events / timing | `libapi` / `libetc` | `libkernel.c` / `libetc.c` (+ `pc_diag.c`, `pc_battle_speed.c`) | [subsystems/kernel.md](pc-port/subsystems/kernel.md) |

The backends are *behavioural* reimplementations, not hardware emulators: where the original relied on
a documented PsyQ behaviour, the PC side reproduces that behaviour — and where a constant or algorithm
was uncertain, it was recovered by disassembling the real PsyQ routine out of the byte-exact
`SLUS_004.47` rather than guessed. See [pc-port/overview.md](pc-port/overview.md).

## How the port avoids breaking the match

The port must add PC-specific code to `src/` in a few places (NULL guards, portability shims, debug
hooks) without changing the bytes the matching build produces. This is done with three preprocessor
gates. **The matching build defines none of them**, so every gated edit compiles out of Layer 1:

| Gate | Purpose | Defined by |
|---|---|---|
| `PERMUTER` | PC-build behavioural / layout changes (e.g. a widened array that would shift struct offsets) | the PC build, globally |
| `PC_PORT` | Portability / 64-bit correctness (e.g. per-site NULL-deref guards) | the PC build, globally |
| `PC_DEBUG_*` | Optional per-file instrumentation / probes | specific build flags only |

`grep -rnE "PERMUTER|PC_PORT|PC_DEBUG" src/` finds every one. **After any `src/` edit, re-run
`make check`** — an ungated change silently breaks byte-identity. History bears this out: an
unconditional widening of one `src/text.c` array once broke the match for two days before it was
caught. See [memory-safety.md](memory-safety.md) for why several of these gates exist (the 64-bit port
surfaced bugs that were invisible to static review).

## Repository layout

The **repository root is Layer 1's home**, deliberately kept in the standard splat-decomp shape
(build + config + symbol map at top level, `asm/`/`assets/`/`build/` generated beside them) — the
layout every splat-based decompilation uses, and the one the upstream decomp used. One root file
crosses the layers: `symbol_addrs.txt` is the address ground-truth **both** sides consume — splat
extraction on the decomp side, and the port's data-segment generator
(`platform/pc/tools/build_data_segment.py`) on every port build.

```
src/                 matching-decomp C source (Layer 1) — byte-exact, do not de-consolize here
include/             matching-decomp project headers
SLUS_004.47.yaml     splat configuration (segment/section layout, symbols)
symbol_addrs.txt     authoritative address → symbol map (shared: decomp extraction + port data-gen)
undefined_additional.txt   extra linker-symbol input for the matching link
Makefile             matching-decomp build orchestration (make extract / check)

platform/pc/         the native PC port (Layer 2)
  src/lib*.c         the six subsystem backends
  src/pc_*.c         PC glue: bootstrap, windowing, data files, video/audio sinks
  include/PsyQ/      clean-room PsyQ headers (the swappable interface)
  include/           PC-side headers (pc_platform.h, types, compat shims)
  tools/             data-generation tooling (see pc-port/data-segment.md)
  Makefile           PC-port build (Linux/native)
  CMakeLists.txt     PC-port build (Linux + Windows cross-compile)
  cmake/             toolchain files (MinGW-w64)
  OPTIONS.md         runtime option reference (also see configuration.md)

docs/                this documentation
```

Two paths that are **local-build-only and gitignored** (never committed — see
[building.md](building.md) and [NOTICE](../NOTICE)): `include/PsyQ/` populated with the real Sony SDK
headers for the *matching* build, and a handful of game/BIOS-derived data files regenerated at build
time from your own copy of the game.

## Where to go next

- Build it: [building.md](building.md)
- Run and configure it: [configuration.md](configuration.md)
- Understand the port internals: [pc-port/overview.md](pc-port/overview.md)
- Per-subsystem detail: [pc-port/subsystems/](pc-port/subsystems/)
- The 64-bit / memory-safety work: [memory-safety.md](memory-safety.md)
- Windows / macOS specifics: [cross-platform.md](cross-platform.md)
