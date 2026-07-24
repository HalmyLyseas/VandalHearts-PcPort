# GPU (libgpu)

The PlayStation's GPU is a fixed-function 2D rasteriser fed by GP0/GP1 command
packets over DMA, backed by 1&nbsp;MB of 16-bit VRAM. The game never touches it
directly — it builds an **ordering table** of primitives with PsyQ's `libgpu`
and hands it to `DrawOTag`, which DMAs the packets to the GPU. The PC backend
replaces that unit wholesale. `platform/pc/src/libgpu.c` keeps a real
1024&nbsp;×&nbsp;512 BGR555 VRAM buffer, walks a real ordering table, and
**software-rasterises** the four primitive types the game actually uses into
that VRAM. SDL2 + OpenGL are used *only* for the last step: blitting the
finished framebuffer to a resizable window each frame
(`platform/pc/src/pc_gpu_window.c`). This is the "OT → per-frame primitive list
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

The current design is the **token bridge** (Stage 2.3). A slot/tag holds a
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
(vertices 1,2,3 then 2,3,4). The triangle filler is a barycentric scanline
rasteriser with **affine UV interpolation and nearest-neighbour sampling — no
bilinear, no perspective correction** — which matches real PS1 GPU behaviour
rather than cutting a corner. A texel value of `0x0000` is treated as fully
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
patterns in the game (`LoadFullscreenImage` with its own rect, and `dojo.c`
slicing one TIM with pointer arithmetic).

### Presentation

The game renders a native **320×240** frame. `PC_GpuPresent`
(`pc_gpu_window.c`) converts the visible VRAM region (per the current `DISPENV`)
from BGR555 to RGB, then blits it with `glDrawPixels` + `glPixelZoom`. The
window opens at `native × scale`; **`VH_SCALE`** overrides the integer factor
(default 2 → 640×480, clamped 1–8). The viewport is recomputed every frame to a
letterboxed, aspect-preserving rectangle, so live resize / maximise work without
extra plumbing. Scaling is display-resolution only (nearest-neighbour, so pixel
art stays crisp) — the 3D is not re-rendered at higher internal density.

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
  `AddObjPrim_Gui` and friends (`object.c`, the primary way most sprites get
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
