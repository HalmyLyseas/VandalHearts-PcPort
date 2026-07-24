# The PC port: internals overview

This section documents *how the native PC port works* — the machinery under `platform/pc/`. Read
[../architecture.md](../architecture.md) first for the two-layer picture and the gating conventions;
this page picks up from there.

## The swappable-interface layer

The port's whole design rests on one idea: **the game keeps calling the PsyQ SDK by name, and on PC
those names resolve to portable C instead of Sony's asm.** The byte-exact game code in `src/` is not
modified to call anything new — `DrawOTag`, `RotTransPers`, `CdRead`, `SsSeqPlay`, `VSync` are the same
symbols they always were. What changes is what those symbols *link to*.

Two pieces make that swap happen:

1. **Clean-room PsyQ headers** (`platform/pc/include/PsyQ/`). These declare the same function
   signatures and struct layouts the game expects, reconstructed as functional facts (see
   `exchange/02-phase-c-interface-contract.md`) — not copied from Sony's SDK. They are the *contract*:
   as long as the port honours these signatures, `src/` compiles against them unchanged.
2. **The `include_stage` symlink tree** (built by both build systems). This is a subtle but essential
   trick. GCC resolves a quoted `#include "PsyQ/x.h"` relative to the *including file's own directory*
   before it consults any `-I` flag. Since `include/common.h` (etc.) themselves `#include
   "PsyQ/…"`, a plain `-I` couldn't stop them from finding the real Sony headers still present locally
   at `include/PsyQ/`. `include_stage/` symlinks every real project header in individually, plus a
   directory symlink for `PsyQ/` pointing at *our* clean-room headers — same effect as replacing
   `include/PsyQ/`, without touching the real tree.

The backends themselves (`platform/pc/src/lib*.c`) implement those signatures over SDL2 (window,
input, GL context), OpenGL (the framebuffer blit), and OpenAL (audio output).

## Behavioural reimplementation, not emulation

These backends reproduce *documented PsyQ behaviour*, they don't emulate PS1 hardware cycle-by-cycle.
Where the original relied on a known SDK behaviour, the PC side reproduces the observable effect. The
important discipline: **where a constant or algorithm was uncertain, it was recovered by disassembling
the real PsyQ routine out of the byte-exact `SLUS_004.47`, not guessed.** PsyQ is statically linked
into the retail executable, so its exact code is right there to `objdump`. This has paid off
repeatedly — the GTE projection constants, the `RotTransPers` OTZ shift, the reverb depth law — each
was wrong when guessed and correct once read from the binary. When a backend's behaviour is in doubt,
disassemble the real routine rather than theorise.

## The six subsystems

| Subsystem | Replaces | Backend files | Deep-dive |
|---|---|---|---|
| **GPU** | `libgpu` (rasteriser, OT, primitives) | `libgpu.c`, `pc_gpu_window.c` | [subsystems/gpu.md](subsystems/gpu.md) |
| **GTE** | `libgte` (fixed-point 3D math) | `libgte.c` | [subsystems/gte.md](subsystems/gte.md) |
| **SPU / sound** | `libspu` + `libsnd` (voices, SEQ music) | `libspu.c`, `libsnd.c`, `pc_spu.c` | [subsystems/spu.md](subsystems/spu.md) |
| **CD / XA** | `libcd` (disc reads, XA audio) | `libcd.c`, `pc_xa.c` | [subsystems/cd-xa.md](subsystems/cd-xa.md) |
| **MDEC / video** | `libpress` (STR FMV decode) | `pc_mdec.c` | [subsystems/mdec.md](subsystems/mdec.md) |
| **Kernel / timing** | `libapi` + `libetc` (events, VSync, saves) | `libkernel.c`, `libetc.c` | [subsystems/kernel.md](subsystems/kernel.md) |

All six are complete and validated on Linux and Windows via a full demo playthrough; A/V fidelity was
checked against real hardware (BizHawk) — GTE/perspective, terrain and sprite rendering, the software
SPU + SEQ music, CD-XA streamed audio, MDEC video, and Shift-JIS/kanji text.

## The glue

Beyond the six subsystems, `platform/pc/src/pc_*.c` provides the startup and support glue:

- **`pc_bootstrap.c`** — runs before `main()`: reserves the PSX RAM ranges, makes read-only data
  writable, installs the fault-handler net, loads `vandalhearts.ini`, mounts and validates the disc,
  and opens the window. See [bootstrap.md](bootstrap.md).
- **The data-segment generator** (`tools/build_data_segment.py`) — mid-build, reconstructs the raw
  data segment the linker script expects. See [data-segment.md](data-segment.md).
- **`pc_gpu_window.c`** — the SDL2 window, GL context, and the 320×240 → scaled present.
- **Hand-written and generated data files** (`pc_*_data.c`, and the gitignored generated ones) — game
  data tables the port needs that aren't produced by the normal link. See
  [data-segment.md](data-segment.md).

## Where to go next

- Startup sequence, disc handling, crash diagnostics: [bootstrap.md](bootstrap.md)
- The data-segment generator and generated data files: [data-segment.md](data-segment.md)
- Any subsystem: [subsystems/](subsystems/)
- The 64-bit / memory-safety story: [../memory-safety.md](../memory-safety.md)
