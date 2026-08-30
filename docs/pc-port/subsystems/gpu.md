# GPU (libgpu)

The PlayStation's GPU is a fixed-function 2D rasteriser fed by GP0/GP1 command
packets over DMA, backed by 1&nbsp;MB of 16-bit VRAM. The game never touches it
directly — it builds an **ordering table** of primitives with PsyQ's `libgpu`
and hands it to `DrawOTag`, which DMAs the packets to the GPU. The PC backend
replaces that unit wholesale, split across five files in `platform/pc/src/`:
`libgpu.c` (the PsyQ API surface, VRAM transfers, the ordering-table token
bridge and the `DrawOTag` walker), `pc_raster.c` (the framebuffers — a real
1024&nbsp;×&nbsp;512 BGR555 VRAM plus the hi-res buffer — and the
**software rasteriser** for the four primitive types the game actually uses),
`pc_hdpack.c` (the optional HD background replacement), `pc_gpu_trace.c` (the
record/replay regression harness), and `pc_gpu_window.c` (SDL2 + Metal on macOS or OpenGL elsewhere, used
*only* for the last step: blitting the finished framebuffer to a resizable
window each frame). This is the "OT → per-frame primitive list
→ rasterise" translation the interface contract calls for, not a 1:1 re-submission
of GPU command words.

See [../../architecture.md](../../architecture.md) for how the port sits on top
of the byte-exact decomp without disturbing it.

## What the game asks for

The clean-room header `platform/pc/include/PsyQ/libgpu.h` reimplements only the
`libgpu` surface the game's source actually references (30 files, 41 symbols).
The functions the backend provides:

| Group | Symbols |
|---|---|
| Environment / init | `ResetGraph`, `SetGraphDebug`, `SetDispMask`, `DrawSync` |
| Draw/display env | `SetDefDrawEnv`, `SetDefDispEnv`, `PutDrawEnv`, `PutDispEnv`, `GetDispEnv` |
| Ordering table | `ClearOTag`, `AddPrim`, `DrawOTag` |
| VRAM transfers | `LoadImage`, `StoreImage`, `MoveImage`, `ClearImage` |
| Draw mode / attrs | `SetDrawMode`, `GetTPage`, `GetClut` |
| Primitive setup | `SetPolyF4`, `SetPolyFT4`, `SetSprt`, `SetTile`, `SetSemiTrans` |
| TIM textures | `OpenTIM`, `ReadTIM` |
| Debug font (stubs) | `FntPrint`, `FntFlush` |

Only **four primitive types** are ever built by the game and therefore
rasterised:

| Primitive | Meaning | Header struct |
|---|---|---|
| `POLY_F4` | flat-shaded quad | `POLY_F4` |
| `POLY_FT4` | flat-shaded textured quad | `POLY_FT4` |
| `SPRT` | free-size sprite (textured rect) | `SPRT` |
| `TILE` | free-size solid-colour rect | `TILE` |

`DR_MODE` is also walked from the OT (it carries the draw mode / texture
window) but is a state change, not a drawn shape.

The ordering table lives inside the game's own `Graphics` struct
(`include/graphics.h`), which is byte-exact decompiled data the port **cannot
resize**:

```c
typedef struct Graphics {
    DRAWENV  drawEnv;
    DISPENV  dispEnv;
    POLY_FT4 quads[1300];
    u32      ot[OT_SIZE];   /* OT_SIZE == 1024 */
} Graphics;
```

`ot` is `u32[1024]` — **4 bytes per slot**, matching the original 32-bit-pointer
PS1 memory model. The game double-buffers two of these
(`gGraphicBuffers[2]`, `gGraphicsPtr`). Each frame it calls
`ClearOTag(gGraphicsPtr->ot, OT_SIZE)`, inserts primitives at depth-sorted
buckets with `AddPrim`, then `DrawOTag(gGraphicsPtr->ot)`.

## The ordering table and the link-token bridge

On real hardware each OT slot / primitive tag is a 32-bit word packing a 24-bit
`addr` (the "next" pointer — real PS1 addresses fit in 21 bits) and an 8-bit
`len`. `DrawOTag` follows `addr` from slot to slot, DMAing each linked
primitive, until a null terminator. The port must reproduce that linked walk,
but **a slot is only 4 bytes** and a modern host pointer is 8. Two earlier
representations were tried and rejected:

1. **Store a host pointer truncated to 32 bits** — correct only under `-m32`;
   this was the single thing pinning the whole build to a 32-bit target.
2. **A global handle↔pointer registry reset each `ClearOTag`** — fit the 4-byte
   slot but broke PS1-style double buffering: the game builds the *next* frame's
   OT while the *previous* one is still being drawn, and a single shared counter
   let clearing the next OT invalidate handles already registered in the
   previous one. Result: every OT walked empty, a permanent black screen.

The current design is the **token bridge**. A slot/tag holds a
**token**, not an address: a 1-based index into a per-frame registry that owns
the real host-width pointer. Tokens are 32 bits by construction, so the same
code is correct at `-m32` and `-m64`.

```
token 0      = end of chain  (the hardware NULL/terminator)
token 1..N   = registry index; the entry carries the pointer AND an
               "is this a bare OT bucket vs a drawable primitive?" flag
```

- **`ClearOTag`** threads the whole array into one chain
  (`ot[0] → ot[1] → … → ot[n-1] → 0`) rather than zeroing slots independently.
  The game adds primitives at buckets other than index 0 but always starts the
  walk at `ot[0]`, so every bucket must sit on the path from `ot[0]`. This is
  also where the token registry resets — `ClearOTag` is always the head of the
  per-frame `ClearOTag → AddPrim* → DrawOTag` cycle, so tokens live exactly one
  frame.
- **`AddPrim`** is an ordinary head-insert into a bucket's linked list, minting a
  token for the primitive.
- **`DrawOTag`** resolves each token, and for a drawable primitive dispatches on
  its type byte to the rasteriser. The registry's explicit `isBucket` flag
  replaces an old hack that stole a pointer alignment bit to mark buckets.

Because `src/` only ever links the OT through `AddPrim`/`addPrim` (44 call
sites, zero uses of `setaddr`/`getaddr`/`termPrim`/`nextPrim`), the token bridge
needed **no decompiled-source edits**. A registry overflow (`PC_OT_MAX_TOKENS`,
65536) drops the tail of a bucket and warns loudly rather than corrupting
memory.

## Primitives and rasterisation

VRAM is a `unsigned short s_vram[512][1024]` array of BGR555 halfwords — the
real PS1 pixel format (bit 15 = mask / semi-transparency source). All primitive
work reads and writes this array; nothing is uploaded to the GPU as geometry.

`DrawOTag` maps each primitive to a software fill:

| Primitive | How it rasterises |
|---|---|
| `POLY_F4` | quad → two triangles, flat colour |
| `POLY_FT4` | quad → two triangles, textured (its own `tpage`/`clut`) |
| `SPRT` | expanded to a textured quad; uses the `SetDrawMode` tpage (sprites have no tpage field of their own) and its own `clut` |
| `TILE` | offset/clip-respecting solid rect fill |

Quads are split into two triangles exactly as the hardware documents
(vertices 1,2,3 then 2,3,4). The default triangle filler is the fixed-point
DDA described under [PSX-accurate DDA](#psx-accurate-dda-vh_accurate); the
legacy filler (`VH_ACCURATE=0`) is a barycentric scanline rasteriser. Both use
**affine UV interpolation and nearest-neighbour sampling — no bilinear, no
perspective correction** — which matches real PS1 GPU behaviour rather than
cutting a corner. Texture modulation is `(texel * colour) / 128`, so a
colour of 128 is neutral. A texel value of `0x0000` is treated as fully
transparent (skipped), the documented hardware quirk. Texture sampling honours
4bpp / 8bpp CLUT-indexed and 15bpp direct texture pages, with `GetTPage`/`GetClut`
bit-packing taken from the psx-spx texpage/CLUT tables.

Fills respect the current `DRAWENV`: the drawing offset (`ofs`) shifts every
primitive, and the clip rect bounds it. `TILE` and the poly fills go through
this offset/clip path; `ClearImage` maps to the raw "quick rectangle fill" GPU
command and, like on hardware, ignores the offset and clip (absolute VRAM rect).
`LoadImage`/`StoreImage`/`MoveImage` are straight VRAM blits. `ReadTIM` parses a
TIM header and hands back pointers *into the file buffer* (it does not upload —
the caller decides where to `LoadImage`), matching the real API's two usage
patterns in the game (`LoadFullscreenImage` with its own rect, and `world/dojo.c`
slicing one TIM with pointer arithmetic).

### Primitive packet layout in the clean-room header

`platform/pc/include/PsyQ/libgpu.h` keeps Sony's struct shapes (so the
byte-exact decomp's `Graphics` struct and every `POLY_FT4`/`SPRT`/`TILE` field
access compile unchanged) with these deliberate departures:

- **`P_TAG.tag` is a 32-bit token**, replacing the hardware word that packs a
  24-bit `addr` with an 8-bit `len`. Every primitive struct starts with the same
  `tag / r0 / g0 / b0 / code` header so a `(P_TAG *)p` cast dispatches uniformly.
  `setlen`/`getlen` are no-ops kept for source compatibility; `setaddr`/`getaddr`
  exist for raw tag inspection only (see Gotchas).
- **`DR_MODE`** is `{tag; code[2]}` on hardware (the raw GP0 `E1h`/`E2h` words).
  Here it carries the common header plus a `tpage` word instead, so `DrawOTag`'s
  generic `getcode()` dispatch works. Only `SetDrawMode` builds it and only
  `DrawOTag` reads it. Its otherwise-unused `r0/g0/b0` bytes carry a packed word:
  bit 22 = dither-enable (`dtd`), bit 23 = "texture window present", and five-bit
  `MaskX | MaskY<<5 | OffX<<10 | OffY<<15` fields (see the texture-window
  section below).
- **The `code` byte** holds a backend discriminator in its low nibble
  (`PC_GPU_PRIM_POLY_F4..DR_MODE` = 1–5) with bit `0x80` as the semi-transparency
  flag set by `SetSemiTrans`. Primitives built by `AddObjPrim_Gui` carry the real
  GP0 byte `0x2c` (`| 0x02` for semi-transparency) instead; `PC_GPU_PRIM_TYPE` /
  `PC_GPU_IS_SEMI` decode both schemes.
- **`TIM_IMAGE`** holds real pointers (`paddr`/`caddr`) into the caller's file
  buffer, which is why it is wider than the PS1 struct on a 64-bit host.

`LoadImage`/`StoreImage` clip both VRAM edges (not just the upper bound) so a
garbage destination rect can never index before `vram[]`, and `ParseTimSection`
rejects an implausible section size or rect (leaving `ReadTIM`'s `prect`
NULL/zero-size) instead of parsing off into memory outside the TIM buffer.

### Presentation

The game renders a native **320×240** frame. `PC_GpuPresent`
(`pc_gpu_window.c`) converts the visible VRAM region (per the current `DISPENV`)
from BGR555 to RGB, then uploads it to the host presentation backend: a streaming SDL texture on
the explicit Metal renderer on macOS, or `glDrawPixels` + `glPixelZoom` on Linux/Windows. The
window opens at `native × scale`; **`VH_SCALE`** overrides the integer factor
(default 2 → 640×480, clamped 1–8). The viewport is recomputed every frame to a
letterboxed, aspect-preserving rectangle, so live resize / maximise work without
extra plumbing. Scaling is display-resolution only (nearest-neighbour, so pixel
art stays crisp) — the 3D is not re-rendered at higher internal density unless
[internal-resolution supersampling](#internal-resolution-supersampling-vh_internal_scale)
is on, in which case the hi-res buffer is what gets presented.

Presentation is deferred to the **end of `DrawOTag`**, not to `PutDispEnv`.
Because this backend rasterises synchronously, presenting at `PutDispEnv` (as the
real hardware timing allows) would show the double-buffer slot's *previous*
contents — one full swap cycle stale every frame. Every real call site pairs
`PutDispEnv` immediately with `DrawOTag`, so presenting at the end of the walk is
correct here.

## Texture window (GP0 E2h)

The GPU's texture window (GP0 `E2h`) makes texel coordinates wrap within a
sub-region of a 256×256 texture page:

```
Texcoord = (Texcoord AND NOT(Mask*8)) OR ((Offset AND Mask)*8)
```

It is persistent GPU state — it stays armed across primitives and frames until
re-set, exactly like real hardware.

**Why it mattered** (`milestone_texture_window_fix`, bugreport-02): the sailing
battle intro (Map 15, `Objf299_Map15_Ocean`) tiles a single 32×32 animated water
tile across the whole sea using ~80 chunks whose UVs span 0..255 and rely on a
**32×32 texture window** to wrap them onto that one tile. The window is armed
through `SetDrawMode(..., &rect)` with `rect.w = rect.h = 32` (a following
`w = 0` `SetDrawMode` re-arms the full page as a reset). The backend originally
discarded `SetDrawMode`'s `tw` rect, so the chunks sampled the mostly-empty page
— giant purple rectangles and noise bands on the ocean. The misleading early
lead ("the game never calls `SetTexWindow`") was wrong: the window arrives via
`SetDrawMode`'s rect argument, never a standalone `SetTexWindow`.

**How it's implemented** (backend-only, no `src/` change):

- `SetDrawMode` computes `Mask = (256 − size) >> 3` and `Offset = pos >> 3` from
  the rect and packs them (plus a bit-23 "window present" flag) into the
  `DR_MODE` primitive's otherwise-unused `r0`/`g0`/`b0` bytes. A `NULL` rect
  means "leave the window unchanged", matching PsyQ.
- `DrawOTag`'s `DR_MODE` case updates the persistent window state **in OT order**
  (real hardware threads GP0 `E2h` in the packet stream), so the LIFO-per-bucket
  ordering applies the 32-window to the chunks and then resets, just like the
  original.
- `SampleTexture` applies `u = (u & ~(Mask*8)) | ((Offset & Mask)*8)` per texel;
  `Mask == 0` is a no-op (full page). The fix is general — any map effect using a
  texture window benefits.

## Rendering layers: accurate rasterizer, supersampling, HD sampling

Everything above describes the base pipeline; three layers sit on top of it — the fills and
supersampling in `pc_raster.c`, the HD sampling in `pc_hdpack.c`:

- **PS1-accurate rasterization (`VH_ACCURATE`, the default).** The fills are a fixed-point integer
  DDA that evaluates pixel coverage *and* texture UVs at the exact positions the PS1 GPU does, with
  ordered dithering (gated on the GPU's dither-enable state, GP0 E1h.9), the 5-bit blend, and the
  hardware's fill conventions — validated ~99.8–99.99 % pixel-exact against a reference-emulator
  VRAM capture. The softer legacy fills remain behind `VH_ACCURATE=0`. Rules below.
- **Internal-resolution supersampling (`VH_INTERNAL_SCALE` 1–4×, "INTERNAL RES").** Each primitive
  is rasterized twice: natively into `s_vram` (authoritative — uploads, `StoreImage`, and all
  read-back see only this), and, when the scale is >1, into a separate `s_hires` buffer at scaled
  geometry via a per-frame deferred display list. The hi-res pass is fanned out across worker
  threads (`VH_RASTER_THREADS`, disjoint scanline bands, bit-identical output) and is what gets
  presented. *Crust-free* sampling biases hi-res texel sampling onto tile interiors on perspective
  quads (2D UI is auto-detected and stays pixel-aligned), which is what removes the tile-seam grid.
- **HD background sampling (`HD PACK`).** `LoadImage` content-hashes each upload; a matching
  pack image is decoded on a background thread and, once published, the hi-res pass samples it at
  sub-texel precision instead of the native texels (8bpp draws only — battle 4bpp sprites sharing
  the same VRAM are never replaced). See [hd-pack.md](../../hd-pack.md).

### PSX-accurate DDA (`VH_ACCURATE`)

`FillTriangleDDA` in `pc_raster.c` is a port of DuckStation's Mednafen-derived software
rasterizer (`DrawTriangle` / `DrawTrianglePart` / `DrawSpan` / `UVStepper`); only the per-pixel
shading (texture sample, modulate, dither, blend) is the port's own. The gate is a runtime check of
`VH_ACCURATE` (ini or env), not an `#ifdef`, so one binary can A/B both paths.

- **Coverage.** Vertices are sorted top/mid/bottom while tracking which one is the top-left
  (`tl`). Edges step in 32.32 fixed point: `dda_makefp(x)` seeds `x << 32` plus a bias of
  `(1<<32) - (1<<11)`, and `dda_makestep(dx, dy)` biases the division toward the edge's direction
  by `±(dy-1)`. A span fills `[x_start, x_bound)` — left-inclusive, right-exclusive — and rows are
  clipped to the inclusive drawing area. Coverage and UVs are both evaluated at the pixel's
  **integer** position, so there is no centre-versus-corner mismatch between them (that mismatch
  is what produces tile-edge seams and the fuzzy "extrapolated UV" texels of a centre-sampled
  filler). A zero-height or zero-area triangle draws nothing.
- **Texture UVs.** 12 integer + 12 + 12 fraction bits (`DDA_ASHIFT`, `DDA_APOST`): the UV origin
  is seeded at the top-left vertex plus half a texel, then stepped back to screen (0,0), so any
  pixel's UV is `origin + x·dUdx + y·dUdy`. The per-axis steps come from the triangle determinant,
  `ATTRIB_STEP(A,B) = ((det(A,B) * 4096) / det) << 12`. Sampled U/V are masked to 8 bits, which is
  how a UV past 256 wraps on a 256×256 page. The texture window (below) applies after the mask.
- **Ordered dithering.** The GPU dithers the *front* colour in the 24→15-bit truncation, before
  any semi-transparency blend, on modulated/untextured polygons only. The 4×4 signed matrix
  (`DITHER4`, same offsets as DuckStation's `DITHER_MATRIX`) is indexed `[y & 3][x & 3]`; the
  hardware LUT `clamp((v + off) >> 3, 0, 31)` is reproduced by clamp-then-shift. It is applied only
  when the dither-enable bit (GP0 `E1h`.9) is set, which the port tracks from `DRAWENV.dtd`,
  `SetDrawMode`'s `dtd` and `DR_MODE` in OT order. The game runs the battle field with `dtd = 0`
  (`states/game_setup.c`), so terrain and UI must not be dithered; only `dtd = 1` scenes (some
  effects) are. The blend result itself is written un-dithered.
- **5-bit semi-transparency blend.** `BlendG1` is the hardware's parallel bit-math on the packed
  BGR555 halfwords (per DuckStation's `ShadePixel`): the four `ABR` modes are `0.5·B + 0.5·F`,
  `B + F`, `B − F` and `B + 0.25·F`, each a per-channel 5-bit add/subtract with saturation. The
  front pixel arrives with bit 15 set; for untextured polygons bit 15 is cleared afterwards. A
  textured semi-transparent polygon only blends where the texel's own STP bit (15) is set —
  a CLUT whose entries all lack bit 15 draws opaque.
- **Legacy path (`VH_ACCURATE=0`).** Barycentric coverage sampled at the pixel centre, floor UV,
  8-bit per-channel blend, no dither.

### Internal-resolution supersampling (`VH_INTERNAL_SCALE`)

- **Geometry × S, UVs native.** When rasterizing into the hi-res target the vertices, draw offset
  and clip are multiplied by S while UVs stay native; the DDA's own UV-step math then advances the
  native texture at 1/S per hi-res pixel — one texture sample per hi-res pixel, coverage and texture
  both at S× density. The native pass (`target == 0`) always uses S = 1.
- **Buffer lifecycle.** `s_hires` is allocated once at the maximum scale (4×) so the options
  overlay can change the scale live without reallocating; each frame uses the current scale's
  stride. A live scale change clears the buffer, or the old stride briefly shows as garbage. Bulk
  VRAM writes that bypass the rasterizer (`LoadImage`, `MoveImage`, `ClearImage`) are mirrored in
  as S×S nearest blocks (`HiresMirrorRect`).
- **Sample re-centring.** The DDA's `+0.5`-texel seed is right for the native grid, but at S× with
  a ~1:1 texture it lands the sub-samples exactly on texel boundaries, where fixed-point floor
  drops or duplicates whole columns (text strokes vanish). The hi-res pass therefore re-centres on
  the hi-res pixel: `+0.5 · (dU/dx + dU/dy)` per axis. For minified textures (terrain) this equals
  the native seed; for ~1:1 it becomes `+0.5/S` texel, giving each source texel S evenly spaced
  samples.
- **U/V == 256 clamp.** A full 256-wide/tall texture's exclusive right/bottom edge sits at
  U/V = 256, which the 32-bit UV fixed point cannot hold (`256 << 24` overflows to 0); the finer
  hi-res sampling would tip the last pixel to texel 0 instead of texel 255 — a dark seam on
  full-page background sprites. The hi-res pass clamps that exact value to 255. Only single-page
  sprites (`u0 + w == 256`) have a vertex precisely at 256; tiled sprites (U past 256) wrap via the
  natural overflow and never do. The native pass never reaches 256.
- **Edge clamp at present.** No primitive's scaled span quite reaches the last presented hi-res
  column/row, so it would retain stale previous-frame content (a flickering 1 px strip).
  `HiresPresent` replicates the neighbouring column/row into it — the same native edge pixel.
- **Scale-1 shadow pass.** The hi-res pass also runs at S = 1 when a language pack ships localized
  backgrounds, because those are sampled only in this pass and a translated title card must not
  depend on a graphics setting. At S = 1 every hi-res-only tweak (re-centring, crust bias, the 256
  clamp) is skipped, so the pass is pixel-identical to native outside a replaced region. HD packs
  deliberately do not arm this: their backgrounds stay a ≥ 2× feature.

### Crust-free tile sampling

Terrain tiles carry a dark border "crust" in their texture cells. On the native grid the hardware
sampling positions never land on it, but the denser hi-res grid does, producing a faint dark grid
along terrain/lava/water seams. The fix uses the re-centring above: for perspective primitives the
half-texel is **not** subtracted, which shifts the hi-res sample +0.5 texel onto the interior texel
— reproducing what the native grid samples, with no softening and the hardware dither kept.
Axis-aligned 2D UI/text keeps true centre sampling, because the same bias there shifts glyph and
border columns by half a texel and doubles or drops columns ("vertical lines" in windows).

The discriminator is `flat2d`: the quad projects to an axis-aligned screen rectangle, i.e. its four
rounded vertices span exactly two distinct X and two distinct Y values (a perspective tile is a
parallelogram with ≥ 3 distinct in one axis). It is decided once per **quad** in `FillQuad` so both
triangles agree — a per-triangle decision splits a quad's halves and leaves a diagonal seam.

### Banded worker pool

During the OT walk the native pass draws inline while each hi-res primitive is **appended** to a
per-frame display list with a self-contained `RenderCtx` snapshot (clip, offset, dither, texture
window, target/scale). After the single-threaded walk the list is rasterized in OT order by N
threads, each owning a scanline band of the drawing area: bands write disjoint hi-res rows and the
list is read-only, so there is no locking. The pool is persistent — threads are created once and
released each frame by a generation counter (no per-frame create/join); the main thread takes the
last band. `VH_RASTER_THREADS` overrides the count (default: online CPUs, capped at 32). A frame is
single-threaded when the list is tiny — except when an HD replacement is loaded, since one
fullscreen HD background is a single primitive with millions of expensive samples.

### Offline raster harness

`platform/pc/tools/raster_harness/harness.c` is a differential test against a real emulator: it
compiles the production GPU source in directly, replays a decompressed DuckStation `.psxgpu` GP0
trace into it, and dumps each frame to PPM for diffing against the emulator's own VRAM capture.
`GPUPort0Data` packets in that format are DMA *blocks*, not one command each — an `A0h` VRAM upload
spans packets — so the harness concatenates all Port-0 words into one GP0 stream and parses commands
by their true word length; VSync packets delimit frames. The port's fills are flat-colour, so Gouraud
polygons replay with vertex 0's colour. PPM output expands 5→8 bits by replication
(`(v << 3) | (v >> 2)`), matching both the present path and the emulator dump so the diff measures
rasterization error only. Build from the repo root with
`cc -std=gnu99 -O2 -Iplatform/pc/include -Iinclude platform/pc/tools/raster_harness/harness.c -lm -lpthread`
and run it on a `zstd -d`-decompressed trace.

**Regression harness:** `VH_GPU_RECORD` / `VH_GPU_REPLAY` serialize and deterministically replay
everything this file consumes (VRAM ops + walker-dispatched primitives) — see
[`platform/pc/tools/regress/`](../../../platform/pc/tools/regress/README.md). Run `raster_check.sh`
after any change here.

## Gotchas / notes

- **Never write a raw pointer into an OT tag.** Only `AddPrim`/`ClearOTag`/
  `DrawOTag` may mint or resolve tokens. The `setaddr`/`getaddr` macros still
  exist for raw tag inspection, but storing an address through them produces a
  value `DrawOTag` resolves to `NULL`, which **terminates the whole walk and
  silently drops every primitive after it**. This is exactly what happened when
  the token bridge first landed with `addPrim` still doing a raw `setaddr` — the
  3D looked perfect (earlier buckets, walked first) while the compass, logo and
  every textbox vanished. `DrawOTag` now warns once when it hits an unresolvable
  link.

- **Two primitive-tag conventions coexist.** `SetPolyF4`/`SetPolyFT4`/… write our
  own discriminators (`PC_GPU_PRIM_*`, 1–5) into the `code` byte. But
  `AddObjPrim_Gui` and friends (`core/object.c`, the primary way most sprites get
  drawn, used across 10 files) instead write the **real hardware GP0 command
  byte** directly — `poly->code = GPU_CODE_POLY_FT4` (`0x2c`, optionally
  `| GPU_CODE_SEMI_TRANS` = `0x02`). `PC_GPU_PRIM_TYPE`/`PC_GPU_IS_SEMI` recognise
  both schemes; forgetting the `0x2c` path made a splash-screen sprite silently
  fail to draw.

- **The frozen-live-global data-gen bug class** (`pc_data_gen_frozen_live_global`).
  A global that is *both* aliased by a pointer table *and* written by name each
  frame can be split by the data-segment generator into a frozen static copy plus
  a live global, disconnecting per-frame writes from the renderer. This bit the
  GPU path directly: `pc_sprite_box_quads.c` pointed `gSpriteBoxQuads[0/7/8/9/11]`
  at frozen blob copies instead of the live `gQuad_800fe53c/63c/65c` globals, so
  rotated/animated attack effects (the archer arrow, many `fx_*` effects, flyer
  shadows) rendered frozen while the by-name writes landed on the live object the
  renderer never read. Fixed by pointing the table at the live globals. The
  full audit was done and came back clean (those three quads were the only
  coincidences) — **do not re-audit**, but keep the class in mind for any new
  generated pointer table.

- **Backend fixed-point can overflow at `-m32`.** The related terrain/sprite
  "perspective collapse" bug (`milestone_perspective_collapse_fix`) was a 32-bit
  multiply overflow in `libgte.c`'s perspective divide, not a rasteriser bug — but
  it is the same hazard class. When fixing, widen the specific intermediate to
  64-bit so it is width-independent; do not "fix" it by switching build width.
  See [gte.md](gte.md).

- **`texel 0x0000` is transparent** independent of the semi-transparency flag, and
  semi-transparency itself uses the four standard PSX blend modes (`ABR` 0–3)
  driven by the active `SetDrawMode` / `POLY_FT4.tpage`. Both are deliberate
  hardware behaviour, not shortcuts.
