# 64-bit width bugs — a field catalogue

The port was deliberately **32-bit for most of its life**, then moved to 64-bit as the default (Stage
2.3). The `-m32` → `-m64` transition surfaced a whole family of bugs rooted in one fact: **the
PlayStation is a 32-bit machine, and a lot of byte-exact game code silently assumes 32-bit pointers.**
This page is the retrospective catalogue — every width bug found, how it was found, and how it was
fixed — because the *how it was found* is the most transferable part for anyone doing a similar
console→PC port.

The single most important takeaway:

> **Most width bugs are invisible to both AddressSanitizer and UBSan.** They are truncated copies,
> serialized-struct layout mismatches, and typed union aliasing — not out-of-bounds accesses. They
> were found by **building and running**, by a **visual A/B against the 32-bit build**, or by
> **diffing `sizeof` between the two widths** — never by a sanitizer. Keep the 32-bit build alive as
> the golden oracle (`make link M32=-m32 BUILD_DIR=build32`).

See [memory-safety.md](memory-safety.md) for the broader memory-model story (NULL reads, `.rodata`,
the sanitizer sweeps); this page is the width-specific deep dive.

## Why 64-bit is hazardous here

Four mechanisms account for essentially all of it:

1. **Pointers grow 4 → 8 bytes.** Any struct field *after* a pointer shifts to a new offset. A
   `void *` inside a union member (e.g. `Object_Sprite.animData`) pushes everything past it — and any
   code that assumes the old offset, or a fixed struct size, breaks.
2. **`long` is 64-bit on LP64.** PSX `long` is 32-bit. On Linux (LP64) `long` is 64-bit; on Windows
   (LLP64) it's 32-bit. Struct layouts and out-parameters that depend on `long`'s width diverge. The
   fix mapped PSX `long` → `int` throughout the PC-owned PsyQ layer, which also makes the Windows
   (LLP64) build "just work" once Linux does.
3. **Hard-coded 32-bit sizes.** Word-count loops (`for i<24`) and `sizeof`-literal `memcpy`/`memset`
   were written against the 32-bit `sizeof(Object)` (96 bytes) and truncate the grown struct.
4. **Raw offset / union-index / OOB access.** `d.bytes[4]`, typed aliasing between union members, and
   plain wrong-index reads all land somewhere different when the surrounding layout shifts.

## How each class was (and wasn't) caught

| Detector | Catches | Misses |
|---|---|---|
| Static grep of casts/types | `(int)ptr` casts, some `long` uses | union aliasing, implicit-`int` returns, wrong indices |
| `-Wimplicit-function-declaration` | a pointer-returning function with no declaration (gnu89 → implicit `int` → truncated) | everything else |
| Build + run | stack smashes, SIGSEGVs (loud) | silent wrong-value bugs |
| AddressSanitizer (32-bit) | out-of-bounds where the index is simply wrong | truncated copies, layout drift (in-bounds) |
| UBSan `bounds` (64-bit) | statically-sized-array overruns at the shipped width | anything not a bounds violation |
| `tools/struct_width_diff.sh` | `sizeof` drift in structs that meet a fixed byte size | value bugs that aren't about size |
| **A human visual A/B (32 vs 64)** | **the truncated-copy / aliasing class the tools can't** | — |

## The catalogue

Every fix is gated `PC_PORT` (portability), so the matching build never sees it and stays byte-exact
(`596bb082a2de5f1fe977dd3d7e160b03`). Grouped by how they were found.

### Found only by building and running (invisible to static audit)

**1. `long` is 64-bit on LP64 — GTE out-params stack-smashed.**
`MATRIX` grew 32 → 48 bytes and every `long *` GTE out-parameter (`RotTransPers` ×13, `RotTrans` ×18,
`RotTransPers4` ×10) wrote 8 bytes into a 4-byte local in `src/` → `*** stack smashing detected ***`
before the first FMV. *Fix:* map `long`→`int` / `u_long`→`unsigned int` across all PC-owned PsyQ
headers and backends (~325 replacements). ⚠️ Protect `long long` when doing this — `libgte.c`'s
`(unsigned long long)n*d` is the perspective-collapse overflow fix
([subsystems/gte.md](pc-port/subsystems/gte.md)).

**2. Raw union index views past a pointer.** `src/battle_013b94.c` did `obj->d.bytes[4] = 1` to set
the low byte of `Object_017.camSavedX`; `Object_017` begins with `struct Object *sprite`, so at 64-bit
byte 4 lands *inside* the pointer and corrupts it → SIGSEGV in `Objf017_Camera` at the first attack.
7 sites. *Fix (battle_013b94.c:278):* `LO(obj->d.objf017.camSavedX)` — the exact line the decompiler
had already left commented above it.

**3. Union coords aliasing → white blobs.** An effect (`Objf314_InwardRay`) writes its quad through
its own struct into `OBJ.coords`, and the shared renderer reads it back as `obj->d.sprite.coords` — a
deliberate union alias that holds only while every aliasing struct puts `coords` at the same offset.
`Object_Sprite.animData` (a `void *`) sits just before `coords`, so on LP64 `coords` moved from +24 to
+32 while the 9 plain-`s16` effect structs stayed at +24 → the renderer read 8 bytes past the written
vertices, drawing white blobs instead of thin casting rays. *Fix (include/object.h:412):* gated
`PC_PORT_COORDS_ALIAS_PAD8` / `PAD4` padding so all 14 aliasing members land at +32 on 64-bit.

**3b. The *same* aliasing, one field group the #3 fix missed — a LEADING pointer.** The endgame
"Dimensional Rift" wormhole (`Objf719_DimensionalRift`) rendered as two flat **white planes** instead
of a textured pentacle — the quads were the right shape but had no texture. Same union-alias mechanism
as #3, but this time in `gfxIdx`/`clut`, not `coords`. `Object_719` has its own pointer
(`entitySpriteParam`) as the **leading field at 0x24** — reusing `Object_Sprite`'s four hidden/facing
bytes, which works at 32-bit (a 4-byte pointer) but not at LP64 (8 bytes): the pointer shoves
`gfxIdx`/`clut` +4 off `Object_Sprite`'s alias, so `AddObjPrim4` reads `gfxIdx` from the *high bytes of
the pointer* → garbage texture index → white quads. The #3 pad only realigned `coords`, so the shape
was right but the texture blank — a textbook "the fix was incomplete" case. **Two structs have this
shape (`Object_675`, `Object_719`); `Object_396` is the correct model (4-byte gap at 0x24, its pointer
at the 0x38 `animData` slot).** *Fix (`PC_PORT_LP64`-gated):* rewrite `_675`/`_719` like `_396` — a
4-byte gap at 0x24 and the pointer relocated to 0x38 — so `gfxIdx`/`clut`/`boxIdx`/`coords` all realign
(verified with `gdb` offsetof). **Lesson: when a fix realigns *one* aliased field group, check that
*all* aliased fields of the affected structs realign — a leading pointer shifts the fields *before*
the padded one too, and those stay broken until you look.** Found by a user's visual A/B, not a
sanitizer (the wrong texture is a valid in-bounds read of a garbage index).

**3c. The same leading-pointer shift, but a *cross-type reinterpret* — the struct itself was fine.**
The Chapter-2 Dolf casting cutscene drew its blue **lightning** as a solid garbage **blob** — ~17k
quads sampling VRAM page 0 (the framebuffer). Same leading-pointer mechanism as #3b, but with a twist
that makes it a *different* bug to fix. `func_800ABFB8` (the lightning builder, `src/split_09a268.c`)
reads `obj->d.sprite.gfxIdx` (offset 0x28) from callers that pass a **non-sprite** object —
`Objf319_Map67_Scn34_TBD` passes *its own* object, whose struct has a leading pointer at 0x24. It has an
existing `if (gfxIdx == GFX_NULL) gfxIdx = GFX_LIGHTNING_5` fallback, and at 32-bit offset 0x28 reads a
clean **0** (`GFX_NULL`), so the fallback selects the lightning texture. At LP64 the 0x24 pointer grows
to 8 bytes (0x24–0x2B) and the read lands *inside* it → a **constant** garbage index (`21977` — the
port's fixed `0x80000000` RAM arena makes the pointer's low half stable) → slips past the `== GFX_NULL`
test → OOB `gGfxTPageIds[21977]` → `tpage = 0` → the quads sample VRAM page 0 = the framebuffer =
garbage blob. **Crucially, unlike #3b, `Objf319`'s struct is *correct for its own use*** (`OBJ.entitySprite`
reads fine) — so the #3b-style "realign the struct" fix does *not* apply. The defect is the **cross-type
reinterpret** in the shared helper, which relies on a union-aliased field reading 0 on PSX. *Fix
(`PC_PORT`-gated, `split_09a268.c`):* extend the helper's own `GFX_NULL` fallback to treat any
**out-of-range** index as `GFX_NULL`, restoring the intended `GFX_LIGHTNING_5`. Found by an every-frame
VRAM dump → auto-locate the peak-blob frame → a frame-correlated per-quad log of `gfx` + resolved
`tpage` + owning `functionIndex` (`VH_OPAQUE_GFX_LOG`, all 9 `AddObjPrim*` opaque branches) → filter
`tpage=0x0000` → the wild `gfx=21977` → source trace. No sanitizer sees it (a valid in-bounds sample of
a garbage-indexed texture page).

**4. A function returning a pointer through `s32`.** `Krom2RawAdd` (BIOS kanji-glyph lookup,
`platform/pc/src/libkernel.c:612`) was declared `s32` returning `(s32)(intptr_t)&glyph` — its own
comment said *"as s32, matching the -m32 pointer width."* Callers dereference it → SIGSEGV in
`DrawSjisGlyph` on the first battle menu. *Fix:* return `void *` (+ a `(void *)-1` sentinel), no `src/`
change. **It also needed a forward declaration:** `src/text.c` / `src/window.c` never `#include` the
header, so under `-std=gnu89` the function was implicitly `int` and the return was truncated *regardless
of the header* — fixed via the force-included `pc_forward_decls.h`. (Audit corollary: of ~170
undeclared-but-called functions, exactly 3 return a pointer; all now declared.)

**5. An out-of-bounds read whose overrun lands differently at 64-bit.** `sText_FileLoadCaptions`
(`src/main_menu.c:147`) is a 3-entry array read with `i < numChoices` where `numChoices == 4` for the
file-load menu. At 32-bit the 4th read hit harmless adjacent static data; at 64-bit it reads 8 bytes
past onto something else → title-screen "Load" SIGSEGV. *Fix:* a `PC_PORT`-gated 4th `""` entry (the
real array is 4 wide — adjacent `slotOccupied[4]` confirms — so empty draws nothing, matching what
32-bit did by accident). ⚠️ **This class is not width-dependent *code*** — the index is simply wrong;
only the *consequence* changes with pointer size. This is the class the ASan sweep exists for.

**6. A truncated struct copy.** `CopyObject` (`src/graphics.c:610`) copied a hard-coded **24 u32 words
= 96 bytes = the 32-bit `sizeof(Object)`**. On 64-bit the union's pointers grow, so fields past them —
notably `Object_Sprite.animYOfs` (0x5A → 0x62) — fall *outside* the copy. Symptom: the **level-up hop**
(rendered from `CopyObject`'d copies of the unit sprite) played its animation frames (`gfxIdx` at 0x28,
*before* `animData` → copied) but never lifted (`animYOfs`, *after* `animData` → truncated → 0).
*Fix:* copy `sizeof(Object) / sizeof(u32)` words. Found purely by a user's 32-vs-64 visual A/B — no
sanitizer sees a copy that stays in-bounds of its destination.

**7. Serialized-struct layout drift (save files).** The in-battle save serializes `UnitStatus`, whose
size is pointer-width-sensitive (120 bytes at ILP32, 136 at LP64 — it embeds two live pointers). Left
naive, a 64-bit build would write a differently-sized blob and the checksum length would overrun.
*Fix (`src/card.c:112`):* `Pc_PackInBattleSave`/`Unpack` serialize to a fixed 120-byte PSX on-disk
layout, so saves are architecture-agnostic and cross-loadable between the 32- and 64-bit builds. Found
by `tools/struct_width_diff.sh` (ASan can't see it — it's layout, not an OOB). See
[subsystems/kernel.md](pc-port/subsystems/kernel.md).

### Found and fixed proactively during the 2.3 build (the mechanical class)

**8. Hard-coded 32-bit `sizeof(Object)` — the zeroing loops.** Three `object.c` routines
(`Obj_GetUnused` and friends) zeroed the Object with `((u32*)p)[2..23] = 0` ("clear the Object but keep
the 0x00–0x07 position vector"). *Fix:* a gated `memset(&p->functionIndex, 0, sizeof(*p) - 2*sizeof(u32))`
each — derived from the struct, so it clears the *grown* struct at 64-bit (`src/object.c:221/267/316`).
`CopyObject` (#6) is the same class but lives in `graphics.c` and does a *copy*, so the object.c-only
audit missed it — it took a gameplay regression to surface.

**9. The GPU ordering-table link.** `platform/pc/src/libgpu.c` stored a host pointer truncated to 32
bits in each primitive's tag word ("`-m32` only"). *Fix:* the **token bridge** — the tag holds a 24-bit
token that a per-frame registry maps to the real host pointer. Width-independent, and it restores the
faithful 24-bit `addr` / 8-bit `len` tag layout. Full detail in
[subsystems/gpu.md](pc-port/subsystems/gpu.md).

**10. The data-segment generator's `sizeof` probe.** It probed struct sizes at a hard-coded `-m32`;
pointer-containing structs are the wrong size at 64-bit. *Fix:* take the width from the build. See
[pc-port/data-segment.md](pc-port/data-segment.md).

## The recurring lessons

Distilled from the whole 2.3 effort — these are the transferable ones:

- **A static audit "surface is small" conclusion is optimistic.** The first pass grepped only
  `*(T*)((char*)p+N)` and reported "0 raw-offset accesses" — it missed the **word-indexed** form
  `((u32*)p)[N]` (dozens of sites) and typed **union-index views** (`d.bytes[]`/`d.shorts[]`/…). Grep
  **all three**, not one.
- **Absence of a declaration is itself a 32-bit assumption.** Under `-std=gnu89` an undeclared
  function is implicitly `int` — a pointer return is truncated no matter what a header says.
  `-Wimplicit-function-declaration`, filtered to pointer-returning definitions, is the only reliable
  detector.
- **Comments that assert a 32-bit assumption are a high-yield grep.** Two of these bugs announced
  themselves in a comment ("matching the -m32 pointer width"). Grep the source for that phrasing.
- **A mis-sized-array *report* is not automatically a too-small *declaration*.** Prove which index is
  actually out of range against the byte-exact binary before widening — sometimes the index is simply
  wrong (a real state bug), not the array too small.
- **Typed aliasing between union members is invisible to grep.** The marker is "member A writes field
  X, shared code reads it via member B." Others may still lurk; watch for wrong-looking rendered
  geometry.
- **A cross-type reinterpret can be broken even when the struct is correct.** #3c is the subtle case:
  the struct read *as itself* is fine, but a shared helper reads *another* union member off it and
  relies on an aliased field being 0 on PSX. A leading pointer's LP64 growth makes that field garbage.
  The fix belongs in the **helper** (validate the value), not the struct (realigning it would break its
  own correct use). When a "0 means default" fallback exists, harden it to "0 *or out of range* means
  default" — the port often manifests "unset" as a stable-arena pointer's low half, not a clean 0.
- **When a representation changes, the function is not enough** — every macro / inline / alternate
  spelling of that operation must move with it (the `AddPrim` function was converted to the token
  bridge but the lowercase `addPrim` macro was not, and silently dropped all UI for a while).
- **Keep the 32-bit build as the oracle.** The fastest localiser for the whole silent-value class is a
  side-by-side 32-vs-64 comparison — a visual A/B for rendering, a `sizeof`/offset diff for layout.
