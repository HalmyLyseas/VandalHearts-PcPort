---
name: phase-c-pc-port
description: How the Vandal Hearts native PC port works — the swappable-interface architecture, the header-staging build trick, gating rules, the local psx-spx hardware reference, the BizHawk-capture workflow, and the hard-won traps. Use when working in platform/pc/, designing a subsystem backend, or needing authoritative PS1 hardware behavior instead of guessing.
---

# Native PC port — working guide

> **Durable technical reference lives in committed [`docs/`](../../../docs/)**: the two-layer
> architecture, building, configuration, per-subsystem deep-dives (`docs/pc-port/subsystems/`),
> memory-safety, cross-platform/packaging, and `docs/width-bugs.md`. Prefer `docs/` for reference and
> keep it current alongside code. **This skill is the agent-workflow companion** — how to *work* in
> `platform/pc/`: the build mechanism, gating rules, the hardware-reference and BizHawk workflows, and
> the traps. (`exchange/` is gitignored local scratch and does not exist in a clone — never rely on it.)

**Status: Stage 2 (the PC port) is complete.** All six subsystem backends are done and validated on
Windows + Linux through full playthroughs; A/V fidelity is signed off against real hardware (BizHawk).
The default build is **64-bit** (`make link`); 32-bit is an A/B reference (`make link M32=-m32
BUILD_DIR=build32`). See `CLAUDE.md` for the top-level status.

## The architecture, in one paragraph

`src/*.c` calls PSX APIs via `#include "PsyQ/whatever.h"` (transitively, mostly through
`include/common.h`). The matching-decomp build resolves that to the real Sony header. The PC port
provides a **second implementation of the same header paths** under `platform/pc/include/PsyQ/*.h`, so
**no game source file changes for either build target.** Implementations live in `platform/pc/src/*.c`,
backed by SDL2 (Pad/VSync, GPU presentation), a raw disc image (CD), OpenAL (Audio), real host files
(Kernel/memory-card), and a software rasterizer + software GTE.

## The header swap actually needs symlink staging — `-I` order alone does NOT work

The single most important build detail. `include/common.h`'s own `#include "PsyQ/..."` lines resolve
**relative to `common.h`'s own directory (`include/`) before any `-I` flag is consulted** — standard C
preprocessor behavior, not overridable by flag order. Because the real Sony headers are still
physically at `include/PsyQ/` (local-only, gitignored), plain `-I` order **never redirects real game
source** to the clean-room headers; it only works for standalone test programs that live outside
`include/` and reach `PsyQ/*.h` directly.

The fix, in `platform/pc/Makefile`'s `stage` target: build `platform/pc/build/include_stage/`, a tree
of **symlinks** — one per real `include/*.h` file, plus a **directory symlink** named `PsyQ` pointing
at `platform/pc/include/PsyQ/` instead of the real one. GCC treats a symlink's own location (not its
resolved target) as the "current directory" for that file's further includes, so this correctly
redirects `common.h`'s (and everything downstream) `PsyQ/` references. **Compile real game source with
`-Ibuild/include_stage`** (not just `-Iinclude`). A bare, non-`PsyQ/`-prefixed header
(`core/graphics.c`'s `#include "inline_gte.h"`) needs the same treatment and requires
`-Iplatform/pc/include` *before* `-Iinclude`.

## One deliberate matching-build exception

`src/core/text.c`'s `sFontGlyphBitmaps[128][9]` → `[129][9]`. Already-decompiled game code reads one row
past the array (`GetGlyphIdxForAsciiChar` maps space to glyph index 128), relying on the original
linker placing all-zero bytes right after it — a real PS1-era trick, confirmed byte-for-byte. The array
is `static`, so it can't be padded from `platform/pc/` alone; widening by one always-zero row was the
only clean option. It is `#ifdef PERMUTER`-gated. **Not a precedent** for other convenience edits.

## Subsystem backends (all done)

| Subsystem | Backed by | Notes |
|---|---|---|
| Pad/VSync | SDL2 | keyboard + `SDL_GameController`; frame-locked ~59.94 Hz |
| CD | raw `.bin` | byte-exact vs real game reads; MDEC/STR video decodes (ffmpeg pixel-parity); CD-XA streamed audio |
| Audio | OpenAL | sample-accurate software SPU (pitch/4-pt Gaussian/ADSR/reverb) + SEQ sequencer + CD-XA + VAG SFX |
| Kernel | host files | memory-card save/load under `saves/`; tick model for AI throttle |
| GTE | software | RTPS/RTPT/NCLIP/AVSZ4/OP/NCCS bit-exact to psx-spx; perspective pixel-accurate vs BizHawk |
| GPU | software raster + SDL2/OpenGL | 1 MB VRAM (BGR555) + OT walk + `POLY_F4`/`FT4`/`SPRT`/`TILE` + TIM parsing |

Per-subsystem implementation detail is in `docs/pc-port/subsystems/`. The data segment (globals used by
`src/*.c` but never given a C-level defining declaration) is solved by
`platform/pc/tools/build_data_segment.py`: it classifies each symbol by whether its type can hold a
pointer, extracts real bytes for pointer-free "safe" ones straight from `build/SLUS_004.47.elf`, and
zero-initializes the "flagged" ones (top-level pointers / structs with a pointer member). See
`docs/pc-port/data-segment.md`.

**Recipe for a new backend** (should one ever be needed): pull the symbol list + exact signatures from
the header, **grep actual call sites in `src/*.c` too** (some real API is never declared in any header —
old GCC allowed implicit-`int` calls), write the clean-room header
(`platform/pc/include/PsyQ/<name>.h`) + implementation (`platform/pc/src/<name>.c`), verify against
**real extracted game data** rather than synthetic input, and document it in `docs/pc-port/subsystems/`.

## Gating rules — the matching build must stay byte-exact

Re-run `make check` after **any** `src/`/`include/` edit (target MD5
`596bb082a2de5f1fe977dd3d7e160b03`). The matching build defines none of the gates below, so every
port-side edit to shared source sits behind one:

- **`PERMUTER`** — behavioural/layout PC changes (e.g. `sFontGlyphBitmaps[129]` above, the ASan
  array-widenings). Defined for all game source by `platform/pc/Makefile`, never by the matching build.
- **`PC_PORT`** — portability / 64-bit correctness guards (per-site NULL-deref guards replacing the
  x86-32 fault decoder: reads → `ptr ? ptr->f : 0`, stores → `if (ptr)`; keep the original verbatim in
  the `#else`).
- **`PC_PORT_LP64`** — 64-bit-host-only struct-layout fixes (e.g. `Object_719`/`_675` in
  `include/object.h`, where a leading pointer's 4→8-byte growth shifts aliased fields).
- **`PC_FEAT`** — (Stage 3) PC-only gameplay/QoL additions (the bidirectional ally-cycle in
  `battle/field.c`, the enemy threat overlay, …). Defined for all game source by the PC build only;
  distinct from `PC_PORT` so gameplay changes grep separately. Keep the original verbatim in the `#else`.
- **`PC_DEBUG_*`** — per-file debug/instrumentation hooks, keyed to Makefile flags.

`grep -rnE "PERMUTER|PC_PORT|PC_FEAT|PC_DEBUG" src/ include/` finds them all.

> **TRAP that actually broke the match for ~2 days:** `sFontGlyphBitmaps` was widened `[128]`→`[129]`
> *without a gate*, adding 12 bytes to `.data`, shifting every later symbol — unnoticed because
> `make check` wasn't re-run. **(1)** gate every layout/behaviour change; **(2)** re-run `make check`
> after ANY `src/` edit, even one that "obviously can't matter"; **(3)** when the match breaks by a few
> bytes with a uniform tail shift, bisect from the first differing byte → the instruction whose
> relocated immediate changed → the moved symbol → the section that grew.

## The local psx-spx hardware reference — use it before guessing

Clone the community "Nocash PSX Specifications" (psx-spx.github.io `docs/*.md`, ~49k lines across 34
pages) alongside the repo — independent community documentation of public hardware facts, freely
referenceable. **Consult it before guessing at exact hardware behavior.** It is most valuable for GTE
and GPU, whose correctness depends on exact rounding/saturation/primitive semantics that can't be
inferred by extracting game data (the way CD/Audio/Kernel could be). Most-relevant pages:
`geometrytransformationenginegte.md`, `graphicsprocessingunitgpu.md`, `soundprocessingunitspu.md`,
`kernelbios.md`, `macroblockdecodermdec.md`, `cdromdrive.md`, `controllersandmemorycards.md`.

> **But psx-spx documents the hardware, not a specific SDK routine's constants.** Never guess a
> PsyQ-library GTE/GPU constant from a psx-spx "normally 1/3, 1/4…" note — that estimate was 4× off for
> `zsf3/zsf4` and blacked out all terrain. **Disassemble the real routine from the byte-exact binary:**
> `mipsel-linux-gnu-objdump -D -b binary -m mips:3000 -EL --adjust-vma=0x8000f800 --start-address=<sym>
> …`, then read the `li rt,imm; ctc2 rt,$reg` pairs (`$24/25`=OFX/OFY, `$26`=H, `$27/28`=DQA/DQB,
> `$29/30`=ZSF3/ZSF4). This is how `InitGeom`'s real constants (`zsf3=0x155, zsf4=0x100, H=1000,
> DQA=-4194`) were recovered.

## BizHawk as ground truth — request a capture whenever there's real doubt

Once real gameplay logic is reachable, psx-spx alone stops being enough — it documents the hardware,
not what a specific game's data/logic produces at runtime. The user runs BizHawk (TAS emulator) on the
real retail game with Lua scripting. **Standing invitation: ask for a capture whenever there's doubt
about reference behavior for a known address/symbol — don't guess.** This has repeatedly resolved
ambiguity faster than static reasoning.

- **Compute any needed address** from `build/SLUS_004.47.map` (VRAM address) + the struct's byte
  offsets (from `include/*.h`'s `/* 0xNN */` comments, or `gdb` sizeof/offsetof against our own build —
  valid because our build uses the real Sony SDK headers, so layout matches hardware). BizHawk's Lua
  `mainmemory.read_*` takes a RAM-relative offset: `vram_address - 0x80000000`.
- **Prefer a dense, full-run capture** (every frame → CSV) over ad-hoc/sampled RAM-Watch checks;
  spot-checks and screenshot-eyeballing have produced wrong conclusions that only a full-run capture
  corrected. Reserve on-change/periodic logging for genuinely high-volume per-frame data (e.g. dumping
  the whole GPU quad buffer).
- **Then mirror the same capture on our build** — add equivalent instrumentation in `platform/pc/src/*.c`
  (strongly prefer this over `src/*.c`) reading the same globals/fields via `extern`, emitting the same
  CSV shape. This turns "does this look right" into "does this row match."
- **Compare at a matched *pose*, not a matched *frame*.** The demo camera never holds still and the two
  builds have timing skew, so frame-N-vs-frame-N always catches different poses. Put the live pose tuple
  `(pitch, yaw, camPosX, camPosZ)` on-screen in both (ours: `VH_CAM_OSD=1`) and compare frames where it
  matches. Likewise compare **scene content** (objects/sprites present), never colour palette — both
  render the same tileset, so any two frames share tones regardless of whether they show the same place.

## Getting a port save into an emulator (DuckStation) — the save→memory-card converter

Some reference checks need a *specific save state*, not just the demo intro (e.g. casting a late-game
spell to diff its rendering). The port stores each save as a raw PS1 save-block file (`saves/` or
`saves_tactical/BASLUS-00447VH`, ~14 KB, starts with `SC`). Wrap it in a valid 128 KB PS1 memory-card
image and load it in an emulator.

- **Use DuckStation, not BizHawk, for this.** BizHawk repeatedly refused to mount the converted card;
  DuckStation loads it fine and is easier to drive for save-state-specific captures.
- **DuckStation wants the `.mcd` extension** (its native raw card format), **not** `.mcr`. Same 128 KB raw
  bytes either way — only the extension differs. Name the output `*.mcd`.

Card layout (verified byte-for-byte against a real card): block 0 = directory — frame 0 `MC` header;
frame 1 the `BASLUS-00447VH` entry (state `0x51`, filesize 16384 = **2 blocks**, next-link idx 1); frame 2
state `0x53` last-link; frames 3-15 free `0xA0`; **frames 16-35 all-`0xFF`** (broken-sector list = none);
36-63 zero. Data blocks 1-2 at `0x2000` = the port save, zero-padded to 16384. Frame checksum = XOR of
bytes 0-126, stored at byte 127. Self-contained generator (no template needed):

```python
import struct
def build_card(save_path, out_path):          # out_path MUST end .mcd for DuckStation
    save = open(save_path, "rb").read()
    assert save[:2] == b"SC" and len(save) <= 0x4000     # PS1 save block, <= 2 blocks
    card = bytearray(131072)
    def put(n, data):
        f = bytearray(128); f[:len(data)] = data
        ck = 0
        for b in f[:127]: ck ^= b
        f[127] = ck; card[n*128:(n+1)*128] = f
    put(0, b"MC")
    e = bytearray(0x0A); e[0] = 0x51; e[4:8] = struct.pack("<I", 0x4000); e[8:10] = struct.pack("<H", 1)
    put(1, bytes(e) + b"BASLUS-00447VH")
    put(2, bytes([0x53,0,0,0]) + struct.pack("<I",0) + struct.pack("<H",0xFFFF))
    for n in range(3,16):  put(n, bytes([0xA0,0,0,0]) + struct.pack("<I",0) + struct.pack("<H",0xFFFF))
    for n in range(16,36): card[n*128:(n+1)*128] = b"\xff" * 128    # broken-sector list = none
    card[0x2000:0x2000+len(save)] = save                            # data blocks 1-2
    open(out_path, "wb").write(card)
```

All three save slots survive (the whole save file is copied in). In DuckStation: Settings → Memory Cards
→ point a slot at the `.mcd`, boot the retail disc, Load Game.

## Memory-safety tooling

Three complementary checks, each catching a class the others miss (detail in `docs/memory-safety.md`
and `docs/width-bugs.md`):

- **`make asan32` + `./run_asan.sh`** — AddressSanitizer playthrough. **Must be 32-bit:** 64-bit ASan's
  shadow at `0x7fff8000` collides with the `0x80000000` PSX RAM arena → `mmap` fails. No coverage lost
  (OOB is width-independent). The runner sets the needed options (`handle_segv=0` because
  `pc_bootstrap`'s SIGSEGV handler is load-bearing; `halt_on_error=0` so one run surfaces every site).
- **`make ubsan`** — `-fsanitize=bounds`, works at 64-bit; covers "is it also OOB at the real width?"
  Limited to statically-visible array bounds (pointer-based accesses unchecked).
- **`tools/struct_width_diff.sh`** — `gdb` sizeof-diff of `build/` vs `build32/`, for the width-bug
  class **neither sanitizer sees**: a struct that changes size at LP64 and overflows a *fixed* byte
  buffer elsewhere (e.g. `UnitStatus` 120→136 broke in-battle saves).

> **Method lesson (bit us 3× in one sweep): a mis-sized-array *report* is NOT automatically a too-small
> *declaration*.** Some arrays genuinely were undersized; `gTravelAscentCost` was the *right* size and
> the *index* (a boundary-tile elevation `diff`) was OOB. Prove which index is out of range against the
> byte-exact binary before widening, and widen the *outer* dimension only — never change a stride. When
> the offending index is data-driven, add a self-gating `PC_DEBUG_*` probe to capture the real value
> rather than guessing from one sample.

## Traps already hit (avoid repeating)

- **Quoted `#include "..."` resolves relative to the including file's own directory BEFORE any `-I`
  flag, always.** The biggest lesson from real `src/*.c` compilation (see the header-staging section).
  Verify a header swap with `gcc -H`, not by assuming `-I` order — standalone tests "passed" while this
  was invisible because none exercised the path where it mattered.
- **Never pass a `PsyQ/` subdirectory as its own `-I` root — only its parent.** Bit twice: quoted
  `sys/types.h` shadowing the real system header, and angle-bracket `<stdio.h>` self-matching a
  `PsyQ/stdio.h` shim. And **don't shadow a real system header name** with a clean-room one — a
  `PsyQ/sys/types.h` silently broke `stdlib.h`'s transitive `int32_t`; the real system already provides
  `u_char`/`u_short`/… (glibc `__USE_MISC`). Check before adding a system-sounding clean-room header.
- **The real PsyQ headers are DOS-era CRLF text — always `grep -a`.** Plain `grep` silently reports
  zero matches on a symbol that's genuinely there. Cost real time twice (`CdlModeStream`, `struct
  DIRENTRY`).
- **`s32`/`u32` (project types.h) ≠ `long`/`unsigned long` (PsyQ sys/types.h) at 64-bit** — `s32` is
  `int` (32-bit), `long` is 64-bit. When a game file forward-declares an otherwise-undeclared function,
  match its exact declared types, not a plausible `long` (which corrupts GTE out-params at LP64).
- **Latent 32-bit arithmetic overflow in backend fixed-point.** `libgte.c`'s `UnrDivide` did
  `((n*d)+0x8000)>>16` with 32-bit operands; `n*d` overflows when the result exceeds 0x10000 (SZ3 < H),
  collapsing all near geometry onto the projection centre. Fixed with a 64-bit product. The byte-exact
  PSX build can't catch these (real hardware divides in the GTE); the `-m32` port is where they surface.
  Audit backend multiplies/shifts for products exceeding 32 bits and **widen the intermediate — don't
  "fix" by switching to a 64-bit build.**
- **A GTE *setup* register left unset/unshifted is silent until the exact render path consuming it
  runs.** `OFX`/`OFY` stored raw not `<<16`, and `ZSF3`/`ZSF4` never assigned, produced no error and
  correct-looking terrain — only *unit sprites* exposed them. When a GTE render looks translated or
  mis-occluded but camPos/rotation match hardware bit-for-bit, audit the GTE control registers
  (`OFX,OFY,H,ZSF3,ZSF4,DQA,DQB`, RT/TR/light matrices) for both *set* and *right fixed-point scale*
  before suspecting the transform math. And **don't over-unify leftover symptoms into one root cause** —
  after the `ofx` fix, occlusion / black patches / sprite flip each had a *separate* cause.
- **A platform-only simulation must never draw from the shared byte-exact `rand()` stream**, even for
  something cosmetic. `libcd.c`'s seek-jitter sim called the shared PS1-BIOS `rand()`, injecting extra
  draws into the stream AI/battle logic depends on — real seek jitter never touches the game's RNG. Give
  any platform-only randomness its own private PRNG. (And a multiplicative LCG is maximally sensitive to
  *any* nonzero tick offset — "close" timing alignment does not imply "close" behavior once RNG is
  consulted.)
- **When reinterpret-casting structs to a shared type-tag struct (like `P_TAG`), every such struct must
  keep the exact same header field layout**, or a generic accessor (`setcode`/`getcode`) silently
  corrupts an unrelated field. Found for GPU's `DR_MODE` (`tag`+`code[2]`, not the usual `r0/g0/b0/code`
  bytes) — tagging it via the shared helper corrupted its payload. Verify layouts agree byte-for-byte,
  don't assume from the field list.
- **Some real API surface is never declared in any header** (Kernel's `GetRCnt`/`OpenEvent`, the
  memory-card file I/O). Old GCC allowed implicit-`int` calls; modern C doesn't. Grep actual call sites,
  not just headers, before concluding a subsystem's contract is complete.
