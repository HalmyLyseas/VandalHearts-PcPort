---
name: phase-c-pc-port
description: How the Vandal Hearts native PC port (Phase C) works — the swappable-interface architecture, subsystem status, established recipe, and the local psx-spx hardware reference. Use when continuing platform/pc/ work, designing a new subsystem backend, or needing authoritative PS1 hardware/BIOS behavior instead of guessing.
---

# Phase C — native PC port

> **Canonical reference now lives in committed [`docs/`](../../../docs/)** (as of 2026-07-23): the
> two-layer architecture, building, configuration, per-subsystem deep-dives
> (`docs/pc-port/subsystems/`), memory-safety, and cross-platform. Prefer `docs/` for durable technical
> reference and keep it current alongside code. This skill remains the **agent-workflow companion** —
> the working conventions, gating rules, gotchas, and the local psx-spx reference index that guide
> *how to work* in `platform/pc/`. `exchange/` stays gitignored investigation scratch.
>
> **Status (2026-07-23): Stage 2 essentially complete.** All six subsystem backends done and validated
> (Linux + Windows, full demo playthrough); A/V fidelity signed off vs BizHawk. 2.2 memory-safety ✅,
> 2.3 64-bit ✅ (default `-m64`) + ASAN sweep ✅, 2.4 cross-platform ✅ (Windows via MinGW-w64; macOS
> deprioritized). QoL: disc auto-detect, `vandalhearts.ini`, fatal wrong-disc check.

## The architecture, in one paragraph

`src/*.c` calls PSX APIs via `#include "PsyQ/whatever.h"` (transitively, mostly through
`include/common.h`). The matching-decomp build resolves that to the real Sony header
(`-Iinclude`). The PC port provides a **second implementation of the same header paths** under
`platform/pc/include/PsyQ/*.h`. **This means zero changes to any game source file for either
build target.** Implementations live in `platform/pc/src/*.c`, backed by SDL2 (Pad/VSync,
GPU presentation), a raw disc image (CD), OpenAL (Audio), real local files (Kernel/memory-
card), and a software rasterizer (GPU).

**One deliberate, user-approved exception exists**: `src/text.c`'s `sFontGlyphBitmaps[128][9]`
→ `[129][9]` (see `exchange/12-phase-c-bootstrap.md` Bug 10). Real, already-decompiled game
code deliberately reads one row past this array's bound (`GetGlyphIdxForAsciiChar` maps the
space character to glyph index 128), relying on the *original* linker placing all-zero bytes
immediately after it — a real PS1-era trick, not a decomp error, confirmed byte-for-byte
against `SLUS_004.47`. Since the array is `static` (no external linkage), there's no way to pad
it from `platform/pc/` alone; widening the declaration by one always-zero row was the only
option that doesn't just trade one undefined-behavior-dependent hack for another. Don't treat
this as precedent for other convenience edits — it was a narrow, explicitly-discussed call for
one specific out-of-bounds-read pattern, not a loosening of the zero-changes rule.

**Critical build detail, only discovered once real `src/*.c` compilation was attempted
(Step 10) — read this before assuming `-I` order alone does the swap**: `include/common.h`'s
own `#include "PsyQ/..."` lines resolve relative to `common.h`'s *own directory* (`include/`)
*before* any `-I` flag is ever consulted — standard C preprocessor behavior, not overridable
by flag order. Since the real, proprietary Sony headers are still physically present at
`include/PsyQ/` (local-only, gitignored), this means `-I` order **never actually redirected
real game source** to the clean-room headers — it only worked for the standalone subsystem
test programs (which live in `platform/pc/src/`, outside `include/`, and reach `PsyQ/*.h`
directly rather than through `common.h`). The real fix, now in place: `platform/pc/Makefile`'s
`stage` target builds `platform/pc/build/include_stage/`, a tree of **symlinks** — one per
real `include/*.h` file individually, plus a **directory symlink** named `PsyQ` pointing at
`platform/pc/include/PsyQ/` instead of the real one. GCC treats a symlink's own location as
"current directory" for *its own* further includes, not the resolved target's real path, so
this correctly redirects `common.h`'s (and everything downstream of it) `PsyQ/` references.
Compile real game source with `-Ibuild/include_stage` (not just `-Iinclude`).

Full contract (all confirmed-used PSX API symbols, exact signatures, what each does):
`vh/exchange/02-phase-c-interface-contract.md`. Per-file PSX-subsystem breakdown:
`vh/exchange/01-phase-c-psx-api-inventory.md`.

## Subsystem status — all 6 done + A/V fidelity complete

> **Current status (2026-07-17):** all 6 backends done AND **fidelity-complete** (graphics + sound
> user-signed-off vs BizHawk, 2026-07-16). Items the table once called "deferred" have **all landed**
> — MDEC video, the software SPU, and the SEQ sequencer. The project has **forked** (Codeberg) and is
> in Stage-2 de-consolization: 2.0/2.1/2.2 ✅, **2.3 64-bit in progress**. Authoritative phase
> tracker: `exchange/52-stage2-roadmap.md`; per-milestone detail lives in the `milestone_*` memory
> files. The table below now records the backend *architecture*, not a to-do list.

| Subsystem | Status | Step file | Notes |
|---|---|---|---|
| Pad/VSync | ✅ done | `04-phase-c-swappable-interface-poc.md` | SDL2 keyboard + SDL_GameController; frame-locked ~59.94Hz |
| CD | ✅ done | `05-phase-c-cd-backend.md` | Reads the real `.bin`, byte-exact vs a real game file. **MDEC/STR video now fully decodes** (from-scratch decoder, ffmpeg pixel-parity — memory `milestone_mdec_video_working`); movie timing real; CD-XA audio streaming works (`milestone_xa_audio_working`) |
| Audio | ✅ done | `06-phase-c-audio-backend.md` | **Sample-accurate software SPU** (pitch/4-pt Gaussian/ADSR/STUDIO_C reverb) is the default music path, + the **SEQ sequencer** (real instruments/tempo/reverb) + CD-XA + VAG SFX — all user-signed-off. Memory `spu_m1_progress`, `milestone_seq_music_sequencer` |
| Kernel | ✅ done | `07-phase-c-kernel-backend.md` | Memory-card save/load (real host files under `saves/` — `milestone_memcard_and_saves`); tick model for AI throttle |
| GTE | ✅ done | `08-phase-c-gte-backend.md` | Software GTE core; RTPS/RTPT/NCLIP/AVSZ4/OP/NCCS bit-exact to psx-spx. The 2026-07-13/14 setup-register + `UnrDivide` 32-bit-overflow + far-object `otz` bugs are **all fixed** — perspective pixel-accurate vs BizHawk (`milestone_perspective_collapse_fix`). `graphics.c`'s raw macros need `-Iplatform/pc/include` **before** `-Iinclude` (see "Traps") |
| GPU | ✅ done | `09-phase-c-gpu-backend.md` | Real 1MB VRAM (BGR555) + OT walk + software rasterizer (`POLY_F4`/`FT4`/`SPRT`/`TILE`) + TIM parsing + SDL2/OpenGL present. Sprite occlusion fixed (`milestone_sprite_occlusion_fix`). **64-bit caveat:** the OT link stores a host pointer truncated to 32 bits (`-m32`-only) — Stage 2.3 replaces it with the ctr-native **token bridge** (`../ctr-native/platform/native_gpu_links.c`) |

**Phase C's swappable-interface mechanism is proven for all 6 subsystems, all 70 `src/*.c`
files compile (Step 10), the link succeeds (Step 11), AND (2026-07-10, Step 12) the game runs
real gameplay logic** — not just an idle loop. `platform/pc/build/vandalhearts_pc` runs
`main()` through the whole intro sequence (VAB sound loading, the opening movie) into
`STATE_27`/`State_Battle`, the actual battle system, before hitting a well-understood data
gap. See `exchange/10-...`, `exchange/11-...`, and `exchange/12-phase-c-bootstrap.md`.

**IMPORTANT build detail: the current build is 32-bit (`-m32`), not 64-bit, as a deliberate,
temporary debugging baseline** — see `exchange/12-phase-c-bootstrap.md`'s "The 32-bit
decision" section before assuming otherwise or "fixing" this. A real chunk of
already-decompiled game source (66+ sites found by one grep pattern) does raw word-indexed
struct writes (`((u32*)p)[23] = 0`) assuming the original 32-bit-pointer PS1 memory layout;
every pointer field in heavily-used structs (`Object`, `UnitStatus`) doubles in size on a
64-bit host, silently corrupting these accesses. 32-bit eliminates that bug class at the
root, giving a working reference to validate a later 64-bit migration against (64-bit is the
real long-term target — required for macOS, which dropped 32-bit entirely in 2019).
`platform/pc/Makefile`'s `M32 := -m32` variable (and the matching `PKG_CONFIG_LIBDIR`
overrides for 32-bit `sdl2`/`openal` lookups) is the toggle point when that migration happens.

> **UPDATE (2026-07-17): this migration is now Stage 2.3, audited + planned** —
> `exchange/52-stage2-roadmap.md` Phase 2.3. Good news: the "66+ raw word-indexed writes" that drove
> this 32-bit decision turned out to be **74 `((u32*)p)[N]` sites that collapse to just 3 `object.c`
> Object-zeroing routines** (`Obj_GetUnused`/`GetFirstUnused`/`GetLastUnusedSkippingTail`, each a
> `((u32*)p)[2..23]=0` block) → fixable with **1 gated `memset((u32*)p+2,0,sizeof(*p)-8)` each**
> (byte-exact at `-m32`, 64-bit-correct); the other 8 sites are flat non-pointer buffer copies (safe).
> The rest of the 64-bit surface is small too (0 ptr↔int casts, 0 dangerous union aliasing, data-gen
> already `sizeof()`-probe-driven). Real remaining blockers: the **GPU OT token bridge** + the
> **NULL-guard pass** (`PC_PORT`, replacing the x86-32 fault decoder). Don't re-derive this — read the
> roadmap's Phase 2.3 audit table.

**How the data segment problem got solved**: 341 real global-variable symbols are used by
`src/*.c` but never given a C-level defining declaration anywhere (the matching-decomp data
segment relies entirely on `symbol_addrs.txt` + the linker script placing raw extracted bytes
at fixed addresses — a normal host linker has no equivalent). Every one was classified by
whether its type can contain a pointer (unsafe to copy 32-bit MIPS bytes onto a 64-bit host):
283 "safe" ones got real byte-exact extraction straight from the byte-exact
`build/SLUS_004.47.elf`; 58 "flagged" ones (top-level pointers, or structs with a pointer
member — including the core runtime-state structs `State`/`UnitStatus`/`Object`/etc., which
plausibly really are runtime-populated, not ROM constants) got plain zero-initialized
definitions instead. One reproducible tool,
`platform/pc/tools/build_data_segment.py` (`make gen-data` / `make link`), does the whole
classify → probe-real-sizeof → extract-real-bytes → generate pipeline. Full methodology,
including the useful discovery that the byte-exact ELF has no separate `.bss` section at all
(everything is one contiguous blob of real file bytes), is in
`exchange/11-phase-c-data-segment.md`.

**What's next (updated 2026-07-13)** — **RESOLVED: the "different terrain at battle entry" symptom
was a rendering bug, not RNG/timing.** The long investigation in
`exchange/20-camera-viewport-coordinates.md` + `feedback-01..17.md` chased several framings that
were each explored and then **REFUTED by matched-pose measurement** — residual RNG-stream desync,
reveal/loading timing, camera-pan phase. The camera pan, camPos, render window, map data and
game-logic tick alignment were all confirmed correct/bit-identical to real hardware. The actual
root cause was **two GTE-backend bugs in `platform/pc/src/libgte.c`, both a "setup value left
unshifted/unset" that stayed invisible until unit sprites actually rendered:**
1. `SetGeomOffset(ofx,ofy)` stored the screen-pixel offsets **raw instead of `<<16`** (GTE OFX/OFY
   are 1.15.16 fixed-point; `TransformOne` does `sx=(n*ir1+ofx)>>16`). Raw 160/120 got swallowed →
   projection centre collapsed to (0,0) → **every 3D-projected element translated up-left by
   (160,120)**, pushing unit sprites off-screen (terrain, being larger, stayed partly visible +
   shifted — the user's long-correct "vector translation" read). Fixed: `g.ofx = ofx << 16`.
   (feedback-15.)
2. **`InitGeom`'s GTE control-register defaults were GUESSED and wrong.** `zsf3/zsf4` (AVSZ
   Z-scale) were first left 0 (terrain OTZ=0, flat), then estimated as `0x555/0x400` from the
   psx-spx "normally 1/3, 1/4" note (feedback-16). That estimate was **4x too large**, so terrain
   OTZ (`= zsf4·ΣSZ/4096`) came out 4x inflated → every tile past graphics.c's `otz>=406`
   distance-darkening threshold → **black terrain**. **Fixed 2026-07-14 by disassembling the REAL
   PsyQ `InitGeom` from the byte-exact `SLUS_004.47` (`0x800d04a8`): the actual values are
   `zsf3=0x155, zsf4=0x100` (also `H=1000, DQA=-4194, DQB=0x1400000`, all previously 0).** Terrain
   validated bright (feedback-18). **REUSABLE METHOD → see the trap below: never guess a PsyQ-lib
   GTE/GPU constant; disassemble the real routine from the binary** (`mipsel-linux-gnu-objdump -D
   -b binary -m mips:3000 -EL --adjust-vma=0x8000f800 --start-address=<sym> ...`, then read the
   `li rt,imm; ctc2 rt,$reg` pairs — `$24/25`=OFX/OFY, `$26`=H, `$27/28`=DQA/DQB, `$29/30`=ZSF3/ZSF4).

**DEDICATED OPEN THREAD — "which routine draws the opening-sequence characters, and does it use
AVSZ otz?"** The zsf4 fix un-masked that demo-opening units render *behind* terrain. Findings so far
(see `exchange/feedback-19.md` + `32-real-hw-ot-population.lua`): our **terrain otz is correct**
(confirmed twice — visual + a real-HW OT-slot read showing hardware puts terrain+units in ONE otIdx
cluster ~704–959); our **units sit ~4× too deep** (otIdx ≤508, below the terrain band). The battle
`RenderUnitSprite` path takes otz from raw `sz3` (`RotTransPers` → SZ3, verified vs real-lib disasm)
while terrain takes `sz3/4` (AVSZ·zsf4) — both byte-exact and under the *same* camera matrix (equal
`trx/trz/h/rt` in the logs), so the math *forces* units 4× behind terrain on hardware too, which
**contradicts the OT read**. **VALIDATED (feedback-20):** an env-gated debug hack dividing the sprite otz by 4
(`VH_SPRITE_OTZ_DIV=4`, in `libgte.c`'s `RotTransPers` — screen X/Y untouched, only the OT index)
made the opening units reappear correctly interleaved, matching the expected characters. So OT depth
is the sole visual cause and the thread is the right + sufficient target. Real fix still open, and static analysis is EXHAUSTED (documented so nobody re-treads it):
- The opening characters go through `RenderUnitSprite` → raw `sz3` otz (disassembly of the real
  `RenderUnitSprite` @ 0x8003a0e8 confirms it calls `RotTransPers`/SZ3, not AVSZ).
- The AVSZ sprite path `AddObjPrim4`/`RotAverage4` is **NOT called during the opening** (probe:
  `VH_OBJPRIM4_LOG`, gate in object.c under `PC_DEBUG_SPRITE_LOG`; CSV never created) — AVSZ-path
  hypothesis RULED OUT.
- Yet the real-HW OT read has nothing at otIdx <704 (where raw-`sz3` units would land); units sit in
  the terrain cluster. So hardware places these units at an AVSZ-like otz that **neither** of our two
  sprite paths produces, under a camera + terrain that both match hardware. Genuine contradiction.
Given this is stage 2 (PC port — runtime behavior need not match the PSX binary), the likely reality
is a **backend (libgte/libgpu) OT-placement difference for the sprite path** that `/4` masks. Next
options (pick, don't re-derive): (a) a real-HW OT read that IDENTIFIES unit prims specifically (match
POLY_FT4 screen x0/y0 to known unit positions) to read hardware's actual unit otz directly; (b)
instrument the OTHER sprite draws (`AddObjPrim2/3`, object.c 508/574/643) in case a third path draws
them; (c) pragmatic port deviation: give the sprite path an AVSZ/`sz3`-scaled otz in the backend and
revisit root-cause later. `/4` (`VH_SPRITE_OTZ_DIV`) is the working visual validation meanwhile.
Note: the earlier Grog/Hassan **horizontal-flip is NOT an independent bug** — `/4` (OT-index only)
fixed it too, so it was a secondary occlusion artifact. Separate open item: a mid-image "wrapping"
distortion (its own later followup). **Methodology lesson**:
several GTE setup values (OFX/OFY shift, ZSF/H/DQA/DQB) were wrong/unset in the backend and silent
until the exact consuming render ran — when a GTE-backed render looks translated/mis-depthed/mis-lit
but the camera numbers match, suspect a wrong GTE *setup* register, and get its ground truth by
disassembling the real PsyQ routine rather than trusting psx-spx "normally…" defaults. **Reusable
technique from this arc: read the real-HW Ordering Table from RAM** (`gGraphicsPtr->ot`, slot =
`OT_SIZE-otz`; populated iff the 24-bit link points into the `quads[]` region below the `ot[]`
offset) to see where hardware actually places prims — decisive when on-paper otz reasoning stalls.

Separately, still genuinely deferred: real controller/player-2 input, MDEC video *decode* (timing
is real, decode is not), music/sequencer playback, and the 64-bit migration (see the 32-bit
callout above).

**Recipe for a new subsystem** (established across all 4 done so far): pull the symbol list +
signatures from step file 02 → grep actual call sites too, not just header declarations (see
"Traps" below) → write a clean-room header (`platform/pc/include/PsyQ/<name>.h`) → write the
real implementation (`platform/pc/src/<name>.c`) → verify against **real extracted game data**
where possible, not synthetic test input → write a standalone test program → update the
relevant `exchange/0N-phase-c-*.md` step file and the checkpoint doc.

## The local psx-spx hardware reference (added 2026-07-10)

Cloned at `vandalHearts_decomp/psx-spx.github.io/docs/*.md` (sibling to `vh/`, same tier as
`maspsx`/`old-gcc`/`psy-q`/`PsyCross`) — this is the community continuation of Martin Korth's
"Nocash PSX Specifications," the most detailed and trusted PS1 hardware/BIOS reference that
exists. ~49,000 lines across 34 pages. Independent community documentation of public hardware
facts — same "functional facts, not copyrightable expression" category as the struct layouts
already reproduced in `platform/pc/include/PsyQ/*.h`, freely referenceable, no clean-room
concern.

**Use this BEFORE guessing at exact hardware behavior** — it's what separates "verified
against real data" (CD/Audio/Kernel's confidence level) from "reasoned approximation, flagged
as such" (the RCNT1 tick rate, PS1's exact pitch table, ADSR shape). It's most valuable for
GTE and GPU specifically, since those can't be verified by extracting game data the way CD/
Audio were — their correctness depends on exact hardware rounding/saturation/primitive
semantics that only a spec (or a real console) can confirm.

**Page index, most relevant first for remaining/refinement work:**

| Page | Covers | Relevant to |
|---|---|---|
| `geometrytransformationenginegte.md` (664 lines) | GTE registers, opcodes, saturation, exact fixed-point behavior | GTE (✅ done — see below) |
| `gtepipelinetimings.md` | GTE instruction timing | Irrelevant to a software reimplementation (no pipeline to stall) |
| `graphicsprocessingunitgpu.md` (1516 lines) | GPU primitives, VRAM, texture pages, timing | GPU (✅ done — see below) |
| `soundprocessingunitspu.md` (1143 lines) | SPU registers, ADSR, reverb, ADPCM | Audio refinement (ADSR, exact pitch table) |
| `kernelbios.md` (3325 lines) | All BIOS calls incl. **Event Functions** (§ ~1276, validated our `OpenEvent`/`TestEvent` implementation exactly) and **Memory Card Functions** (§ ~939) | Kernel refinement |
| `timers.md` | Root Counter registers (raw hardware level) | Kernel's `GetRCnt`/`ResetRCnt` — note: even this source flags RCNT1's typical use as "horizontal retrace**?**" with its own uncertainty marker; the exact tick rate genuinely isn't pinned down without knowing the game's own `SetRCnt` configuration (not found in this project's confirmed symbol usage) |
| `cdromdrive.md`, `cdromformat.md`, `cdrominternalinfoonpsxcdromcontroller.md` | CD-ROM controller commands/status | CD refinement, `CdRead2`/streaming if ever tackled |
| `macroblockdecodermdec.md` (425 lines) | MDEC video decompression | Would be the reference if FMV/`DecDCT*` is ever implemented |
| `controllersandmemorycards.md` (2735 lines) | Pad protocol, memory card format | Pad refinement, real card format if ever needed beyond this project's own simplified local-file mapping |

**Already validated against it (2026-07-10):** the Kernel backend's event system
(`OpenEvent`/`EnableEvent`/`TestEvent`) behavior — auto-clear on successful test, "initially
disabled," class+spec matching — matches `kernelbios.md`'s documented BIOS semantics exactly.
One cosmetic difference noted, not fixed (no functional impact): real event handles are
`F1000000h`-and-up addresses; this backend uses small integer indices instead. No code
inspects the handle's bit pattern, so this doesn't matter, but worth knowing if a future
subsystem ever does care about the real format.

**Also validated (2026-07-10):** the GTE backend's `RTPS`/`RTPT`/`NCLIP`/`AVSZ4`/`OP`/`NCCS`
implementations were built directly from `geometrytransformationenginegte.md`'s formulas,
including the exact Unsigned Newton-Raphson hardware division algorithm and its 257-entry
lookup table (generated programmatically from the documented formula, not hand-transcribed,
to avoid transcription error). Full writeup: `exchange/08-phase-c-gte-backend.md`.

**Also validated (2026-07-10):** the GPU backend's `GetTPage`/`GetClut` bit-packing and the
4/8/15-bit CLUT-indexed texture addressing came directly from `graphicsprocessingunitgpu.md`'s
"Texpage Attribute"/"Clut Attribute"/"VRAM Overview" sections; the TIM texture-file format
came from `cdromfileformats.md`'s "TIM Format" section. Full writeup:
`exchange/09-phase-c-gpu-backend.md`.

## BizHawk as ground truth (added 2026-07-11) — request a capture whenever there's real doubt

Once real gameplay logic is reachable (not just rendering plumbing), psx-spx alone stops being
enough — it documents the hardware, not what a specific game's data/logic actually produces at
runtime. For that, the user has a working BizHawk (TAS-purpose emulator) setup running the real
retail game, and Lua scripting access to it. **User's own standing invitation: ask for a
BizHawk capture whenever there's doubt about expected reference behavior for a known
address/symbol — don't just guess or rely on trace-only inference from our own build.** This
has repeatedly resolved real ambiguity faster than static reasoning alone (see
`exchange/13-bizhawk-ram-watch.md`/`13-battlemgr-trace.lua` for the `Objf013_BattleMgr.unitSprite`
NULL-pointer puzzle, and `exchange/14-sprite-render-trace.lua` for Bug 16's sprite-rendering
investigation).

**How the technique works**: any global/struct-field address needed is computed from
`build/SLUS_004.47.map` (VRAM address) plus the decompiled struct's byte offsets (either from
`include/*.h`'s own explicit `/* 0xNN */` comments where present, or via `gdb`'s
`sizeof`/`offsetof`-style queries against our own build — safe to do since our build compiles
against the real Sony PsyQ SDK headers, so struct layout matches real hardware exactly, not
just our own reimplementation). BizHawk's Lua `mainmemory.read_*` functions take a RAM-domain-
relative offset, i.e. `vram_address - 0x80000000` — subtract before use. Write a `.lua` script
under `exchange/NN-topic.lua` (gitignored along with the rest of `exchange/`) with clear
usage-instructions comments (the user runs it themselves, in a Windows VM since BizHawk's Linux
build doesn't run properly here), have them run it and hand back the resulting CSV/log, then
analyze. Two proven capture shapes so far: (1) per-frame state-transition logging (find an
object by scanning `gObjectArray` for a known `functionIndex`, log its fields + relevant
globals every frame or on change), and (2) periodic bulk dumps (e.g. every 15 frames, dump an
entire array like the GPU quad buffer) when the thing being compared is high-volume and
sampling density matters more than every-frame precision.

**A caveat already learned**: don't assume two values are directly comparable just because they
look similar (e.g. GPU `tpage`/`clut` IDs are runtime-assigned VRAM locations — real hardware
and our build can legitimately load textures in a different order and get different numbers for
the *same* texture, so matching quads between captures by `tpage` doesn't work; look for a
different correlation key, or compare aggregate statistics — e.g. "does this value ever exceed
±1000 anywhere in the whole capture" — instead of trying to match individual records 1:1).

**Default methodology as of 2026-07-12 — prefer a dense, full-run Lua capture over ad hoc/
sampled RAM Watch checks.** Earlier investigations that spot-checked values at a handful of
frames or relied on visual eyeballing of screenshots repeatedly produced wrong conclusions that
only got corrected once a full-run capture was done (see `followup_ai_battlemgr_timing` and
`investigation_sprite_rendering_paused` memory entries — sampling looked convincing both times
and was wrong both times). The now-standard shape, exemplified by
`exchange/19-battlemgr-full-watch-log.lua` and `exchange/21-camera-target-full-watch-log.lua`:
one `.lua` script, run from power-on through the entire capture window, logging **every frame**
(not on-change, not periodic) to a CSV — cheap enough in practice (~19000 frames × a dozen-ish
int columns is a small file) that there's rarely a real reason to sample instead. Reserve
periodic/on-change logging for genuinely high-volume per-frame data (e.g. dumping an entire GPU
quad buffer every frame would be excessive; every-15-frames is fine there) — default to dense
for scalar/struct-field state.

Once a BizHawk baseline CSV exists, **mirror the same capture on our own build** so the two are
diffable numerically instead of visually: add equivalent instrumentation in `platform/pc/src/*.c`
(strongly prefer this over `src/*.c`) that reads the same globals/struct fields via `extern` and
logs the same CSV shape, gated behind a debug build flag or just left in like `libetc.c`'s
permanent FPS-counter diagnostic. This turns "does this look right" into "does this row match,"
and was the direct ask that produced this section (see
`exchange/20-camera-viewport-coordinates.md`).

**When a hook genuinely has to live in decompiled `src/*.c`** (the fade/loading/sprite-log hooks
below sit in `src/screen_effects.c`, `src/cd.c`, `src/object.c` because that's where the code
path is), wrap it in `#ifdef PC_DEBUG_*` with the original line preserved in the `#else`/unguarded
branch. The matching build never defines those macros, so the block compiles out and byte-exact
matching is preserved — but the rule is now "gated, not absent" rather than "never touch `src/`."
Layout/behaviour changes (not just debug hooks) use `#ifdef PERMUTER` instead. See the "Gated
instrumentation / stage-2-safe edits in decompiled source" note below before assuming any `#ifdef`
in `src/` is a bug — and re-run `make check` after any `src/*.c` edit.

## Render-debugging tooling + matched-pose methodology (added 2026-07-13)

Built while hunting the GTE-backend rendering bugs above; reuse these instead of re-deriving.
All debug additions are **gated** (compile flag or env var) so the matching build and normal PC
build are unaffected; the actual *fixes* live in `platform/pc/src/libgte.c`.

- **The winning method: compare our render to BizHawk at a MATCHED CAMERA POSE, not a matched
  frame.** The demo camera never holds still and the two builds have timing skew, so frame-N-vs-
  frame-N comparison always caught different poses and produced wrong conclusions for many rounds.
  The fix: put the live camera pose on-screen in *both* builds and pick frames where the pose
  tuple `(pitch, yaw, camPosX, camPosZ)` is identical (the deterministic intro pan passes through
  the same poses on both). BizHawk side: RAM Watch on `gCameraPos`/`gCameraRotation`/etc. Our
  side: `VH_CAM_OSD=1` renders `PIT/YAW/X/Z/FRD` top-left of the window (`libetc.c`
  `PC_UpdateCamOsd` → `pc_gpu_window.c` blits a tiny 5x7 font into the present buffer). Only once
  a truly matched pose was compared did the real (rendering, not timing) bug become undeniable.
- **`platform/pc` debug flags** (Makefile, gated so the matching build never sees them):
  `NO_FADE=1` forces the `Objf369` screen fade transparent (see the scene during the normally-
  fade-hidden battle-entry reveal); `NO_LOADING=1` forces the "Now Loading" screen off;
  `SPRITE_LOG=1` instruments `RenderUnitSprite` (object.c). Toggle by rebuilding (`make link
  NO_FADE=1 ...`); a `FORCE` rule rebuilds the one affected object.
- **`VH_SPRITE_LOG=1`** (env var, with a `SPRITE_LOG=1` build) writes `vh_sprite_fate.csv`: for
  every unit sprite each frame — tile pos, render window, cull result, projected screen coords,
  `otz`/`otIdx`, and the **live GTE state** (`ofx/ofy/h`, `rt` matrix signature, `tr`, last
  terrain `otz`, `zsf4`). This is what pinpointed `ofx=0` and `zsf4=0`. GTE state is read via
  `PC_GteDebugState`/`PC_GteLastOtz`/`PC_GteZsf4` accessors added to `libgte.c` (the `g` struct is
  static, so an accessor is the only way to read it from `libetc.c`/`object.c`).
- **`exchange/30-disable-fade-bizhawk.lua`** does the BizHawk equivalent of `NO_FADE`+`NO_LOADING`
  (forces `Objf369` overlay colour 0 and `suppressLoadingScreen=1`) and logs
  `fieldRenderingDisabled` + pitch, so real hardware can be compared at the same debug conditions.

### Gated instrumentation / stage-2-safe edits in decompiled source — don't panic if you see `#ifdef` in `src/`

Some decompiled files carry `#ifdef` blocks. **This is allowed, but only when gated**, because the
matching build must stay byte-exact. Three macro families are in use (the matching build defines
**none** of them):

- **`PC_DEBUG_*`** — debug-only hooks, keyed to Makefile flags. The matching build never defines
  them, so they compile out entirely:

  | File | Macro | Flag | What it does |
  | --- | --- | --- | --- |
  | `src/screen_effects.c` (~L154, L187) | `PC_DEBUG_DISABLE_FADE` | `NO_FADE=1` | forces `Objf369` fade transparent |
  | `src/cd.c` (~L832) | `PC_DEBUG_NO_LOADING` | `NO_LOADING=1` | skips the "Now Loading" screen |
  | `src/object.c` (~L725, L837) | `PC_DEBUG_SPRITE_LOG` | `SPRITE_LOG=1` | logs `RenderUnitSprite` cull/projection/GTE state |
  | `src/graphics.c` (RenderMapTile/RenderEdgeMapTile) | `PC_DEBUG_TERRAIN_LOG` | `TERRAIN_LOG=1` | per-frame terrain otz stats + black-tile counts → `vh_terrain_otz_pc.csv` (env `VH_TERRAIN_LOG`) |

- **`PERMUTER`** — behavioural/layout PC-port fixes that must NOT reach the matching build.
  `platform/pc/Makefile` defines `-DPERMUTER` for all game source; the matching Makefile never
  does. Current use: `src/text.c`'s `sFontGlyphBitmaps[129][9]` (PC) vs `[128][9]` (matching) —
  see the trap below.

- **`PC_PORT`** — **(NEW, Stage 2.3)** portability / 64-bit-correctness guards. Also defined for all
  game source by `platform/pc/Makefile` (`GAME_C_FLAGS`), never by the matching build. First use:
  **per-site NULL-deref guards** replacing the x86-32-only fault-handler decoder (the portable path
  for the 2.3/2.4 goal). Pattern mirrors the 2.2 handler's policy exactly so runtime is unchanged:
  reads → `ptr ? ptr->f : 0` (read-0, NOT a bare skip); stores → `if (ptr)` (discard). Keep the
  original code verbatim in the non-`PC_PORT` path so byte-exact holds by construction. Sites so far:
  `src/battle_0201b8.c:3768-3770` (camera-follow), `src/fx_0506c0.c:1890,1892` (unit-blocking). Find
  remaining sites via the fault handler's `vh_null_reads.log` + `make crash-trace`. See
  `exchange/52-stage2-roadmap.md` Phase 2.3 Step A.

`grep -rnE "PC_DEBUG|PERMUTER|PC_PORT" src/` finds them all. **All are included in the byte-exact
re-verification** (MD5 `596bb082a2de5f1fe977dd3d7e160b03`; the matching build compiles none of them).

> **TRAP — a stage-2 fix to decompiled `src/*.c` that changes code size or data layout breaks
> stage-1 silently.** This actually happened: `sFontGlyphBitmaps` was widened `[128]`→`[129]`
> (a legit PC start-menu-artifact fix — index 128 is read one past the original array) *without a
> gate*, adding 12 bytes to `.data`, shifting every later symbol, and breaking the match — unnoticed
> for ~2 days because `make check` wasn't re-run. Lesson: **(1)** gate every layout/behaviour change
> to decompiled source behind `#ifdef PERMUTER`; **(2)** re-run `make check` after ANY `src/*.c`
> edit, even one that "obviously can't matter"; **(3)** when the match breaks by a few bytes with a
> uniform tail shift, bisect via the first differing byte → the instruction whose relocated
> immediate changed → the moved symbol → the section that grew (here: offset 31021 → `sw` imm
> `0x2e50`→`0x2e5c` → `.sdata` +12 → text.c `.data` +12). The GTE *fixes* live in
> `platform/pc/src/libgte.c` and touch no decompiled source.
- **The always-on camera/rand CSV mirrors** (`libetc.c` `LogCameraTraceRow`/`LogRandSeedRow` →
  `vh_camera_target_full_watch_log_pc.csv` etc.) mirror the BizHawk Lua captures field-for-field.

## Memory-safety tooling (Stage 2.3 — added 2026-07-23)

Three complementary checks, each catching a class the others miss. Full write-up + findings ledger
in `exchange/58-asan-sweep.md`; setup detail in the `asan_sweep_setup` memory.

- **`make asan32` + `./run_asan.sh`** (in `platform/pc/`) — AddressSanitizer playthrough. **Must be
  32-bit**: 64-bit ASAN's shadow starts at `0x7fff8000` and collides with the `0x80000000` PSX RAM
  arena `pc_bootstrap.c` reserves (the hardcoded PSX scratch literals need it) → `mmap` fails → dies
  in the first `CdRead`. No GCC option relocates the shadow; the 32-bit shadow is at `0x20000000`, so
  the arena is free. No coverage lost — OOB bugs are width-independent. `run_asan.sh` sets the
  required options (`handle_segv=0` because `pc_bootstrap`'s SIGSEGV handler is load-bearing;
  `halt_on_error=0`+`-fsanitize-recover` so one run surfaces every site; `VH_RCNT1_NORMALIZE=1` so the
  sprite decoder's `GetRCnt(RCntCNT1)<=470` budget doesn't starve at ~12 FPS and freeze the enemy
  turn). Findings land in `asan.log.<pid>`, archived to `asan_runs/`.
- **`make ubsan`** — `-fsanitize=bounds`, and this one **works at 64-bit** (no shadow region). Covers
  the "is it also OOB at the real width?" gap `asan32` can't speak to. Limit: only where the array
  bound is statically visible in the TU (pointer-based accesses unchecked).
- **`tools/struct_width_diff.sh`** — `gdb` sizeof-diff of `build/` vs `build32/`. The width-bug class
  **neither sanitizer can see**: a struct that changes size at LP64 and overflows a *fixed* byte
  buffer elsewhere (e.g. `UnitStatus` 120→136 broke in-battle saves, `bugreport-07`). Needs no run.

**The sweep (2026-07-23) found 7 real OOB bugs** static audit could not — a wrong *index*, where
only the consequence changes with pointer width. All fixed `PERMUTER`-gated + byte-exact:
`gClutIds[124]→128`, `gWindowDisplayX/Y[16]→70` (was overwriting live XA audio state),
`gUnitAnimSets[144]→301`, `gStringTable[100]→101`, travel tables `[14][20]→[20][20]`,
`D_8017DF50[27]→[29][64]`. Two non-bugs left documented (`ReserveSprite` stack OOB = authentic UB;
`gTexwSpriteSetFrames[-12]` negative-`gfxIdx` = one-frame cosmetic, never reproduced).

**Method lesson (bit me 3× in one sweep): a mis-sized-array *report* is NOT automatically a
too-small *declaration*.** `gClutIds`/`gWindowDisplayX` genuinely were undersized; `gTravelAscentCost`
was NOT (correct size, the *index* — a boundary-tile elevation `diff` — was the OOB). Prove which
index is out of range against the byte-exact binary and the code before widening, and widen the
*outer* dimension only (never change a stride). When the offending index is data-driven, add a
self-gating `PC_DEBUG_*` probe (see `PC_DEBUG_PATH_STEP` in `src/path_grids.c`) to capture the real
value instead of guessing from one observed sample.

## Traps already hit (avoid repeating)

- **Latent 32-bit arithmetic overflow in the backend (the -m32 build).** `libgte.c`'s `UnrDivide`
  (GTE perspective divide) did `n = ((n*d)+0x8000)>>16` with `n,d` as 32-bit `unsigned long`; `n*d`
  overflows 32-bit whenever the division result exceeds 0x10000 (i.e. SZ3 < H=512), so **all near
  geometry collapsed onto the projection centre OFX/OFY** (terrain + sprites, never UI — UI skips
  the divide). Symptom looked spatial *and* temporal (different vertices cross the SZ3<512 threshold
  as the camera moves). Fixed with an explicit 64-bit product `(unsigned long long)n*d`. **Lesson:
  the byte-exact PSX build can't catch these (it runs on real 32-bit hardware/emulation where the
  GTE divides in hw); the -m32 port is exactly where they surface. Audit backend fixed-point
  multiplies/shifts for products that can exceed 32 bits and widen the intermediate — don't "fix" by
  moving to a 64-bit build.** Found with a per-vertex projection log (`VH_TERRAINPROJ_LOG` +
  `PC_GteProjEntry` ring) diffing `UnrDivide` output against the closed form — when a projected
  render looks warped but the camera/inputs check out, log the actual per-vertex `sx,sy,IR,SZ3,n`.
- **A GTE *setup* register left unset or unshifted in the backend is silent until the exact
  render path that consumes it runs.** Three did this at once (`OFX`/`OFY` stored raw not `<<16`;
  `ZSF3`/`ZSF4` never assigned) and produced no error, no crash, and correct-looking terrain —
  only *unit sprites* exposed them (translated off-screen, then flat-depth occluded). When a
  GTE-backed render looks translated or mis-occluded but the camera numbers (camPos/rotation)
  match real hardware bit-for-bit, **audit the GTE control registers** (`OFX,OFY,H,ZSF3,ZSF4,
  DQA,DQB`, the RT/TR/light matrices) against `geometrytransformationenginegte.md` and confirm
  each is both *set* and in the right fixed-point scale, before suspecting the transform math.
  Found 2026-07-13 (feedback-15/16); both fixes are one line each in `libgte.c`.
- **Don't over-unify remaining symptoms into one root cause.** After the `ofx` fix, the leftover
  symptoms (missing foreground units, wrong occlusion, sprite flip, black patches) looked like
  they might share a cause; they didn't — occlusion was `zsf4`, the black patches are an `otz`-
  overflow Z-scale issue, the flip is a separate UV/facing bug. Each was worth isolating at a
  matched pose separately (this same investigation had already been burned repeatedly by
  premature unification — see the RNG/timing/reveal framings that were all refuted).

- **Some real API surface is never declared in any header, anywhere in the original tree** —
  found for Kernel's `GetRCnt`/`ResetRCnt`/`OpenEvent`/`EnableEvent` and the whole memory-card
  file-I/O API. Old GCC 2.x allowed calling undeclared functions (implicit `int`, legal 1990s
  C); modern C doesn't. **Grep actual call sites in `src/*.c`, not just header declarations**,
  before concluding a subsystem's contract is complete.
- **The real PsyQ headers are DOS-era CRLF text.** Plain `grep` without `-a` can silently
  report zero matches on a symbol that's genuinely there (`file` shows "CRLF line
  terminators" on these). Always `grep -a` against `include/PsyQ/`. Bit this twice already
  (`CdlModeStream` during Phase B, `struct DIRENTRY` during the Kernel backend).
- **`s32`/`u32` (project's own types.h) ≠ `long`/`unsigned long` (PsyQ's sys/types.h)** on
  this 64-bit target — `s32` is `int` (32-bit), `long` is 64-bit. When a game source file
  locally forward-declares a function (matching the "undeclared in any header" trap above),
  match its exact declared types, not a plausible-looking `long`.
- **Don't shadow a *real system* header name just because PsyQ happens to have one with the
  same name.** `platform/pc/include/PsyQ/sys/types.h` (created for CD/Audio) shadowed the
  actual `<sys/types.h>` whenever `-Iinclude/PsyQ` was on the include path, since quoted
  `#include "sys/types.h"` searches `-I` dirs before system dirs — this silently broke
  `stdlib.h`'s own transitive `int32_t` dependency for any file also pulling in SDL2/OpenAL
  headers (`libetc.c`, `libsnd.c` failed to compile; found during GTE regression-testing,
  2026-07-10). Fixed by deleting it — the real system header already provides
  `u_char`/`u_short`/`u_int`/`u_long` (glibc, `__USE_MISC`); PsyQ only shipped its own because
  the original PSX toolchain's libc didn't have one. **Before adding a clean-room header
  under a generic/system-sounding name, check whether the real system already provides it.**
- **A game source file can `#include` a *bare, non-`PsyQ/`-prefixed* project header full of
  raw MIPS asm**, sitting outside the `PsyQ/*.h` swap mechanism entirely — found for
  `graphics.c`'s `#include "inline_gte.h"` (the project's own header, not Sony's). Fixed by
  extending the same swap trick to that one bare name too
  (`platform/pc/include/inline_gte.h`), which requires the PC build to list
  `-Iplatform/pc/include` **before** `-Iinclude` for the override to win. GPU turned out not
  to have this wrinkle — `libgpu.h` is only ever pulled in via `graphics.h` (a normal
  `PsyQ/`-prefixed include), so the standard swap mechanism covered it cleanly. Still worth
  checking for any *new* subsystem before assuming the standard mechanism is enough.
- **Quoted `#include "..."` resolves relative to the including file's own directory BEFORE
  any `-I` flag, always, no exceptions.** This is the single biggest lesson from Step 10 (real
  compilation) — see the architecture section above. Any future header-swap trick needs to
  account for this: if the file doing the `#include` lives inside a directory that already
  contains a same-named real target at that relative path, `-I` order is irrelevant, the
  real file always wins. Verify a swap actually works by tracing with `gcc -H`, not by
  assuming `-I` order is enough — the standalone subsystem tests all "passed" while this bug
  was completely invisible, because none of them exercised the path where it mattered.
- **Never pass a `PsyQ/` subdirectory as its own `-I` root — only its parent.** Bit twice now:
  once for quoted `sys/types.h` shadowing the real system header (GTE work), once for
  **angle-bracket** `<stdio.h>` self-matching our own `PsyQ/stdio.h` shim (Step 10) — exposing
  `platform/pc/include/PsyQ/` directly makes its contents reachable via `<...>` search too,
  not just the `"PsyQ/..."` quoted form it's meant for. The second occurrence broke
  *previously-working* standalone subsystem builds the moment libc-forwarding shims
  (`stdio.h`/`memory.h`/`strings.h`) were added under `PsyQ/` — always re-verify the existing
  subsystem tests after adding a new header there.
- **When a clean-room struct can't preserve real hardware's exact bit-packing** (a 64-bit host
  pointer can't fit in a 24-bit bitfield, the way GPU's `P_TAG.addr` does on real hardware),
  and the swap is safe (nothing inspects the raw bits, only ever touched through named
  macros/functions) — **every struct that gets reinterpret-cast to a shared "type-tag" struct
  (like `P_TAG`) must keep the exact same header field layout**, or a generic accessor
  (`setcode`/`getcode`) will silently corrupt an unrelated field. Found for GPU's `DR_MODE`:
  its real layout (`tag`+`code[2]`) didn't share the `r0/g0/b0/code` byte positions every
  other primitive struct had, so tagging it via the shared `setcode()` helper corrupted its
  own payload at byte offset 11. A test that depended on the corrupted state (`SPRT` sampling
  the texture page `DR_MODE` was supposed to configure) caught it — **if a subsystem has
  several structs sharing a common "header" via pointer-cast tricks, verify their layouts
  actually agree byte-for-byte, don't assume it from the field list alone.**
- **A platform-side simulation must never draw from the same `rand()` stream real game logic
  depends on, even for something that feels purely cosmetic.** `libcd.c`'s CD-seek-jitter
  simulation called the shared, byte-exact-PS1-BIOS `rand()` (`libkernel.c`) for its own jitter
  term — real hardware's physical seek jitter never touches the game's software RNG at all, so
  this silently injected extra draws into a stream AI decision logic (`src/ai.c` spell
  selection, `src/battle_eval.c` tie-breaking) depends on being byte-exact, well before any
  gameplay logic runs. **Any platform-only randomness (jitter, timing noise, anything with no
  real-hardware behavioral counterpart to match) needs its own private PRNG, full stop** — never
  reach for the shared one just because it's already there. Found via
  `exchange/20-camera-viewport-coordinates.md`'s RNG investigation.
- **A multiplicative LCG (this game's real `rand()` algorithm) is maximally sensitive to ANY
  nonzero tick-count offset — "close" alignment does not mean "close enough."** Even after
  fixing two real RNG bugs and a movie-timing bug, bringing game-logic-level tick alignment to
  within 1.9% of real hardware, the game's actual AI/target-selection decisions still diverged
  from real hardware at the very next checkpoint that consulted `rand()`. Don't assume a
  small measured timing gap implies a proportionally small behavioral gap once RNG is involved
  — it doesn't; the LCG fully scrambles on any offset, however small. `exchange/
  20-camera-viewport-coordinates.md`'s 2026-07-13 entries.
- **When implementing a "timing-accurate skip" for a real-hardware sequence a stub has been
  short-circuiting, check how many times the caller retries before assuming a fail/retry loop
  will pace correctly.** Three implementation attempts building this project's MDEC/movie-timing
  fix (`platform/pc/src/libcd.c`'s `StGetNext`) each failed for a different, instructive reason
  (a call-counter counter exhausted within a single tick's inner busy-loop; a raw
  `SDL_GetTicks()` check per call turning into 1M+ syscalls/tick and a real, user-visible UI
  stall; a real-tick counter that fixed the stall but still failed, because the *outer* caller
  only retries ONCE per tick, not the million-try budget that belongs to the *inner* loop one
  level down) before landing on the correct model: make the call always succeed immediately
  (matching what the real hardware path actually does — data arrives far faster than the
  polling rate), and pace a *separate, secondary* value instead of the call's own success/
  failure. If a stub's "always fail" behavior was previously safe only because the whole
  sequence finished near-instantly, don't assume any given pacing approach is safe once real
  duration is restored — verify by watching for OS-level unresponsiveness, not just internal
  tick counters (both of which can look fine independently while the other is actually broken).
- **When visually comparing our build's rendered frames against BizHawk/real-hardware
  reference frames, compare actual scene content (objects, structures, sprites present) — never
  color palette or general composition.** Both platforms render the same tileset, so any two
  battle-map screenshots will share color tones regardless of whether they show the same part of
  the map. Misjudged a "match" this way twice in one session (`exchange/
  20-camera-viewport-coordinates.md`) — one instance got written into a persistent memory file
  as "confirmed fixed" before being caught and retracted. Enumerate what's actually present in
  each frame and confirm the lists agree; prefer numeric/positional confirmation (RAM-watched
  coordinates, object indices) over visual judgment when available.
