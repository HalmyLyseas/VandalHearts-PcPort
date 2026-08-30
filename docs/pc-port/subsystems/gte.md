# GTE (libgte)

The GTE (Geometry Transformation Engine) is the PS1's fixed-point 3D coprocessor: it does the
rotate/translate/perspective-project math that turns model-space vertices into screen coordinates,
plus the ordering-table Z (OTZ) values the GPU uses to depth-sort polygons. The game reaches it
through Sony's `libgte` — a mix of raw COP2 opcode macros (`gte_rtps`, `gte_avsz4`, …) in the one
file that needs coprocessor-level fidelity, and higher-level SDK wrappers (`RotTransPers`,
`RotMatrix`, `rcos`, …) everywhere else. The PC backend
([`platform/pc/src/libgte.c`](../../../platform/pc/src/libgte.c)) is a single software model of that
coprocessor, shared by both entry paths, so the raw macros and the SDK wrappers can never disagree
on GTE state. Behaviour comes from the Nocash PSX Specifications (psx-spx) hardware reference for the
opcodes and from **disassembling the real PsyQ routines out of the byte-exact `SLUS_004.47`** for the
things psx-spx doesn't document (SDK-level constants and return conventions).

## What the game asks for

Two headers define the interface, both clean-room reimplementations with the exact struct layouts
and signatures the byte-exact decompile already recovered — no text from Sony's originals:

- [`platform/pc/include/PsyQ/libgte.h`](../../../platform/pc/include/PsyQ/libgte.h) — the ~25
  high-level SDK functions used by 37 of the 38 GTE-touching files (they only need to be
  geometrically correct):
  - Setup: `InitGeom`, `SetGeomOffset`, `SetGeomScreen`.
  - Matrix registers: `SetRotMatrix`, `SetTransMatrix`, `SetLightMatrix`, `SetColorMatrix`,
    `SetBackColor`; the software stack `PushMatrix` / `PopMatrix`.
  - Matrix builders: `RotMatrix`, `TransMatrix`, `ScaleMatrix`.
  - Transforms: `RotTrans` (rotate+translate only), `RotTransPers` (single vertex, projected),
    `RotTransPers4` / `RotAverage4` (a quad's four corners plus an averaged OTZ).
  - Scalar math: `VectorNormalS`, `SquareRoot0/12`, `csqrt`, `rcos`, `rsin`, `ratan2`.
- [`platform/pc/include/inline_gte.h`](../../../platform/pc/include/inline_gte.h) — the raw COP2
  macro set, used only by `src/core/graphics.c` (the one file needing genuine coprocessor fidelity). It
  shadows the project's own MIPS-asm `include/inline_gte.h`: the PC build lists
  `-Iplatform/pc/include` before `-Iinclude`, so `core/graphics.c`'s `#include "inline_gte.h"` resolves
  here instead, with **zero changes** to `core/graphics.c` or the real header. Each macro is a thin call
  into a `PC_GTE_*` function: `gte_ldv0/ldv3/ldrgb/ldopv1/ldopv2` (load), `gte_rtps/rtpt` (project),
  `gte_nclip` (back-face cross product), `gte_avsz4` (OTZ from the SZ FIFO), `gte_op0` (outer
  product), `gte_nccs` (normal-colour-col lighting), and the `gte_st*` stores.

`MATRIX` is `short m[3][3]` + `int t[3]`; `ONE` is `4096` (the Q12 fixed-point scale). Every integer
in the header is deliberately `int`/`unsigned int`, never `long`: PSX `long` is 32-bit, but on an
LP64 host `long` is 64-bit, which would silently grow `MATRIX` and make `int *` out-parameters write
8 bytes into the 4-byte locals `src/` passes — a stack smash. Do not reintroduce `long` here.

## Fixed-point matrix math

The backend keeps one global instance of the GTE register file (`g` in `libgte.c`): the control
matrices (`rt`, `light`, `colorMat`), translation/background/far-colour vectors, the projection
constants, and the data-register FIFOs (`sz0..sz3`, `sxy0..sxy2`, IR registers, MAC accumulators,
the RGB FIFO, and `flag`).

All the real work funnels through `TransformOne(vx, vy, vz)`, which reproduces the RTPS formula in
portable C at the hardware's Q12 scale:

1. `MAC1..3 = (TR << 12 + RT · v) >> 12` — rotate by `rt`, translate by `tr`, keep the result in the
   accumulators; `IR1..3` are the saturated 16-bit copies.
2. Push the SZ FIFO: `SZ3 = saturate_u16(MAC3)` (the post-transform Z).
3. `n = UnrDivide(H, SZ3)` — the perspective divide `H / SZ3` using the documented Unsigned
   Newton–Raphson hardware algorithm (a 257-entry reciprocal table generated at startup from the
   psx-spx formula, `CountLeadingZeroes16`-based normalisation, two refinement steps).
4. Screen X/Y: `SX = (n · IR1 + OFX) >> 16`, `SY = (n · IR2 + OFY) >> 16`, each **saturated to
   `-0x400..+0x3FF`** as the real GTE does (FLAG bits 14/13), then pushed into the SXY FIFO.
5. Depth cue: `IR0 = saturate(( n · DQA + DQB ) >> 12)`.

`RTPS`/`RTPT` run `TransformOne` for one / three vertices; `RotTransPers` loads a vertex, runs it,
and packs `SXY`; `RotTransPers4` runs the quad then `AVSZ4`. `AVSZ4` computes the ordering-table Z
as `OTZ = saturate_u16( ZSF4 · (SZ0+SZ1+SZ2+SZ3) >> 12 )`. Signedness matters in step 4: `IR1`/`IR2`
are legitimately negative for anything left of / above the projection centre, so the multiply is a
**signed** widen — an earlier `(unsigned short)` cast turned `-166` into `65370` and threw those
coordinates tens of thousands of pixels off-screen.

Matrix builders (`RotMatrix`, `ScaleMatrix`) and the scalar transcendentals (`rcos`, `rsin`,
`ratan2`, the square roots) are SDK-library routines with no coprocessor formula — but they are
**checked against the real PsyQ routines disassembled from `SLUS_004.47`**, and the results are
reassuring rather than a source of risk (see [SDK routines, verified](#sdk-routines-verified-against-the-binary)).
In short: our `rsin`/`rcos` are **bit-identical** to PsyQ's, `RotMatrix`'s axis order **matches**
(its integer arithmetic does not — see [RotMatrix: float vs PsyQ integer path](#rotmatrix-float-vs-psyq-integer-path)),
and `ratan2`/the square roots are *more* accurate than PsyQ's table approximations. None of them
needs "hardening"; the C here is standard math where PsyQ used a table, and the two agree (or ours
is the finer of the two).

## Recovering the real constants (don't guess PsyQ)

`InitGeom` seeds the GTE control registers. psx-spx documents the *opcodes* but not what Sony's
`InitGeom` writes, and the values are not something to eyeball. The whole set was recovered by
**disassembling the real PsyQ `InitGeom` at `0x800d04a8`** (a run of `li t0,imm; ctc2 t0,$reg`)
straight out of the byte-exact `SLUS_004.47`. The values now in `libgte.c` are those bytes:

| Register | Symbol | Value | Role |
|---|---|---|---|
| `$29` | `ZSF3` | `0x0155` | Z-average scale (triangle OTZ) |
| `$30` | `ZSF4` | `0x0100` | Z-average scale (quad OTZ): `OTZ = ZSF4 · ΣSZ >> 12` |
| `$26` | `H` | `1000` | projection plane distance (overridden to `0x200`=512 by `SetGeomScreen` before rendering) |
| `$27` | `DQA` | `-4194` | depth-cue slope → `IR0` |
| `$28` | `DQB` | `0x1400000` | depth-cue offset → `IR0` |

Why this is a load-bearing lesson: the earlier values were **guessed** from the psx-spx "normally
1/3, 1/4" note as `ZSF3=0x555 / ZSF4=0x400` — exactly **4× too large**. With `ZSF4` 4× inflated,
terrain OTZ (via `gte_avsz4`) came out ~4× deep — the nearest tile projected to OTZ ~472 instead of
~118 — pushing essentially all terrain past `core/graphics.c`'s distance-darkening black threshold
(`otz >= 406`). That was the notorious "black terrain" symptom. `0x100` is the disassembled
ground-truth value, and terrain renders correctly with it. The rule the project follows: when a
PsyQ constant or behaviour is unknown, disassemble it from our own binary rather than approximate —
the same technique fixed the sprite-occlusion return value below (see
[`overview.md`](../overview.md) and the disassembly recipe in the project's technique notes).

## Two fixed-point bugs worth knowing

Both are backend-only (`libgte.c` is not part of the matching build) and both were reproduced
bit-for-bit before being fixed — this is the kind of arithmetic the byte-exact stage-1 build
*cannot* catch, because the real GTE divides in hardware with no C-width limit.

**1. Perspective collapse — a 32-bit multiply overflow in `UnrDivide`.** Everything (terrain and
sprites, never the UI) warped radially onto the projection centre `(OFX, OFY)`. Root cause: the
final step of the UNR divide, `n = ((n · d) + 0x8000) >> 16`, was computed with `n` and `d` as
32-bit. `n · d` overflows 32 bits whenever the quotient exceeds `0x10000` — i.e. whenever
`SZ3 < H` (H=512), meaning any geometry *nearer* than the projection plane. The wrap produced a tiny
garbage `n`, so those vertices projected to `SX ≈ OFX`, `SY ≈ OFY` and collapsed to screen centre,
while farther vertices projected fine — a defect that looked both spatial (fixed epicentre) and
temporal (vertices cross the threshold as the camera pans). It was isolated with a per-vertex
projection ring (`PC_GteProjEntry`, still in the file): `UnrDivide(512, 495)` returned `2251`
instead of `67787`, and an arbitrary-precision Python replica gave `67787` — pinning it to C width,
not the algorithm. **Fix:** do the product in 64-bit so it is width-independent —
`n = (((unsigned long long)n * d) + 0x8000) >> 16`. The lesson generalises: fix these by widening
the specific intermediate, *not* by switching to a 64-bit build.

**2. Sprite occlusion — `RotTransPers` must return `SZ3 >> 2`, not raw `SZ3`.** Unit sprites drawn
via `RotTransPers` sat ~4× too deep in the ordering table and got painted over by terrain (the
"cropped / top-half" symptom). The real PsyQ `RotTransPers`, disassembled at `0x800d0178`, is
`mfc2 v0,$19` (GTE data reg 19 = `SZ3`); `jr ra`; `sra v0,v0,0x2` in the delay slot — it returns
**`SZ3 >> 2`**. That puts the returned OTZ on the *same scale* as the terrain's AVSZ path
(`ZSF4 · ΣSZ >> 12` = `SZ3/4` with `ZSF4 = 0x100`), so sprites interleave with terrain instead of
sinking behind it. **Fix:** `return g.sz3 >> 2;`. Callers use the return value only as an OT depth
index (the packed screen `*sxy` is stored separately and is unaffected), so the `÷4` is uniformly
correct. The sibling routines match: real `RotTransPers4` (`0x800d0428`) and `RotAverage4`
(`0x800cf620`) both hand back the AVSZ4 OTZ, which already carries the `÷4`.

## Gotchas / notes

- **`SetGeomOffset` shifts its arguments `<< 16`.** The GTE `OFX`/`OFY` registers are 1.15.16
  fixed-point and `TransformOne` adds them at that scale before the `>> 16`. Storing the pixel
  arguments raw collapsed the projection centre to `(0,0)` and translated all projected content
  up-left, pushing sprites off-screen while terrain stayed partly visible.
- **`gte_stotz` stores a full 32-bit word.** The real store (`SWC2`) always writes 32 bits, and OTZ
  is a saturated 16-bit value zero-extended into its register, so hardware always clears the upper
  16 bits of the destination. Writing only 16 bits left the caller's full-width `int otz` with stack
  garbage in the top half — normally harmless, but it once made `otz` large enough to push `AddPrim`
  out of bounds and `SIGSEGV`. `PC_GTE_StoreOTZ` writes the full word.
- **`PushMatrix`/`PopMatrix` are a 16-deep software stack.** The GTE has no native matrix stack;
  these are SDK-level. A single saved slot (the first implementation) was silently clobbered by
  nested pushes — a per-frame camera push with per-object pushes inside it — corrupting sprite
  coordinates over successive frames. 16 is a generous scene-graph bound, not a hardware number.
- **Two entry paths, one core.** The `gte_*` macros and the SDK wrappers both mutate the same `g`
  state; keep any new opcode consistent across both so `core/graphics.c` and the other 37 files stay in
  sync.
- **`gte_ldopv1` is modelled functionally.** Hardware `ctc2`s the three words into control registers
  0/2/4 (`RT11`/`RT13`/`RT22`), using the RT diagonal as the `OP` vector (psx-spx). The backend keeps
  them in a separate `opvD[3]` scratch that only `OP0` reads — nothing touches RT between `gte_ldopv1`
  and `gte_op0`, and the scratch can never collide with the `PushMatrix`/`PopMatrix` stack.

### Diagnostic hooks

All read-only, consumed by `platform/pc/src/pc_diag.c` (declared in `pc_platform.h`):

- `PC_GteProjEntry(back, …)` — a ring of the last 8 `TransformOne` results (`SX`, `SY`, `IR1..3`,
  `SZ3`, `n`), `back=0` most recent. A quad's four corners are `back` 3,2,1,0 (v0..v2 from `RTPT`,
  then v3 from `RTPS`); the spread `SX − OFX = H · IR1 / SZ3` can then be broken down term by term.
- `PC_GteDebugState` — `OFX`/`OFY`/`H`, three `RT` samples and `TRX`/`TRZ` as used by the last
  transform, so the sprite log can tell a wrong geometry offset from a stale facing matrix
  (`rt` = scaled identity instead of the rotated camera matrix).
- `PC_GteLastOtz` / `PC_GteZsf4` — the last `AVSZ4` result (terrain OTZ) and the `ZSF4` scale.
- `VH_GTE_LOG=1` — `RotAverage4` prints, per `AddObjPrim4` quad, the input `vy` of the bottom (v0)
  and top (v2) vertices against the projected screen Y; a tall wall whose `dSy` stays ~0 while `dSx`
  is large means the height is being collapsed. Screen coordinates are `PackSXY` (x = low 16 bits,
  y = high 16 bits, both signed shorts).

## SDK routines, verified against the binary

`RotMatrix` and the scalar transcendentals have no COP2 formula to disassemble the way `InitGeom`'s
constants did — they are SDK library code. To retire the earlier "reasoned choice, not ground truth"
caveat, all of them are disassembled from `SLUS_004.47` and compared to the C here. The outcome: the
C is faithful, and where it deviates it is *deliberately more accurate*.

| Routine | PsyQ implementation | This backend | Result |
|---|---|---|---|
| `rsin` / `rcos` | quarter-wave sine table at `0x801206C0` (4096 = 360°), quadrant-folded | `sin`/`cos` × `ONE`, rounded | **Bit-identical** — zero difference across all 4096 angles. |
| `RotMatrix` | pure-integer compose over a packed sin/cos table; the `rz`-independent third column (`sy`, `-sx·cy`, `cx·cy`) is written first | `double` compose, same third column | **Axis order matches**, all nine terms and signs. Values differ by up to 15/4096 because PsyQ's table and re-quantisation are coarser — see [RotMatrix: float vs PsyQ integer path](#rotmatrix-float-vs-psyq-integer-path). |
| `SquareRoot0` / `SquareRoot12` / `csqrt` | GTE leading-zero normalise (`mtc2 $30`/`mfc2 $31`) + a **coarse table** (`~0x8011C3D0`) + shift — an approximation | true `(int)sqrt` | Differ, but **ours is the more accurate**. |
| `ratan2` | integer `div` ratio into an **arctangent table** (`~0x8011BB0C`) | libm `atan2`, scaled to 4096 = 360° | Differ in the low bits; ours is the more accurate. |

So none of these is a latent bug or an unverified guess. `rsin`/`rcos` and `RotMatrix`'s order are
confirmed faithful; `sqrt`/`ratan2` are intentional accuracy refinements of PsyQ's table shortcuts.
Extracting PsyQ's `sqrt`/`ratan2` tables to bit-match the console is possible with the same technique,
but it would trade accuracy for exact-hardware reproduction — only worth it if strict PSX determinism
becomes a goal, and there is no symptom motivating it today.

## RotMatrix: float vs PsyQ integer path

The PsyQ `RotMatrix` at `0x800d0aa8` is pure integer arithmetic over a **packed sin/cos table at
`0x8011C6C0`** (4096 entries, 16 KB; entry *i* = `(cos_i << 16) | sin_i`, Q12), composed with
`multu` + `sra ,0xc`. `libgte.c` carries a transcription of it (`RotMatrixExact`) alongside the
default `double` path. Three details of the integer routine are easy to get wrong:

- `sra` is a floor shift and the negation happens *before* it (`negu` then `sra`), so the negated
  terms are `floor(-x/4096)`, not `-floor(x/4096)`.
- The triple products are re-quantised at each stage: `P = (-sy·cz) >> 12` and `Q = (-sy·sz) >> 12`
  are taken to Q12 first, then multiplied again. A full-width product shifted once does not match.
- `m[0][2]` is stored raw and unshifted — it is `sy` straight out of the table.

Measured over the angles the game uses, the `double` path differs from the integer one in ~83% of
matrix entries, by up to 15/4096 (~0.37%). The gap is dominated by the table's own deviation from
ideal sin/cos (~6.8 units), not by rounding mode — `lrint` and `floor` change essentially nothing.
The order — the only real correctness risk — is identical.

Three runtime switches, all measurement aids and all off by default, select between the paths. Each
takes the path of the table file, which is **game data**: extract it from your own copy with
`platform/pc/tools/gen_gte_table.py`; it is read at runtime, never compiled in or committed.

| Variable | Effect |
|---|---|
| `VH_GTE_EXACT=<table>` | `RotMatrix` uses the integer transcription. |
| `VH_GTE_EXACT_CHECK=<table>` | Stay on the float path, compute the exact matrix alongside, and report the worst disagreement on stderr — checks the transcription on real angles without perturbing the run. |
| `VH_GTE_AB=<table>` + `VH_GTE_AB_USE=float\|exact` | **Equal-cost A/B**: both matrices are computed on every call in *both* runs; only which one is kept differs. |

The equal-cost form exists because `RotMatrix` runs ~92× per frame: simply switching
implementations changes frame time enough to shift scene-load completion by a frame, which
desynchronises `rand()` and makes the two runs play different battles. Equalising the cost removes
that confound, so any surviving divergence is attributable to the matrix values themselves. Only
`m[][]` is copied from the exact result — neither path writes the translation `t[]`.
