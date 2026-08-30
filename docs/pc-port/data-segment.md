# The data segment & generated data files

The matching build gets its data segment for free: splat extracts the raw `.data`/`.rodata` regions
from the original binary and a linker script places them at fixed addresses. The PC port has no such
linker script and no extracted blob — yet the game code still references hundreds of data symbols
(`gShopInventories`, `gItemCosts`, the sprite-box quad globals, …). Something has to *define* those
symbols with the right size and contents, or the final link fails. That's what the data-segment
generator does, plus a small set of hand-written and regenerated data files.

## The data-segment generator

`platform/pc/tools/build_data_segment.py` runs **mid-build**, after the objects compile but before the
final link, in both build systems. Its job: discover which data symbols are undefined, work out how
big each one is, and emit a `generated_data.c` that defines them. Pointer-free ("safe") symbols get
their real bytes sliced out of the byte-exact executable and copied in by a constructor; anything
pointer-typed or pointer-containing ("flagged") gets zero-initialised storage only, because a pointer
value copied from the binary would be a MIPS virtual address, meaningless in the host process.

The pipeline:

1. **Find undefined symbols.** Link the compiled objects and scrape the linker's
   `undefined reference to '…'` lines (Apple `ld` reports `"_sym", referenced from:` instead; the
   leading underscore is Mach-O decoration). (Every `-l` library must resolve, or the link aborts
   *before* printing undefined refs — a subtle cross-compile trap; see below.)
2. **Find declarations & classify.** For each symbol, locate its `extern` declaration in the
   **staged** headers (`build/include_stage`, not the raw tree — for `REGION=jp` the stage substitutes
   gate-carrying US/merged headers whose array geometries differ from the raw `jp/` copies) plus
   `src/**/*.c` for file-local externs, and classify it as pointer-typed, pointer-containing (a struct
   or union with a pointer member) or plain value. The struct-body regex excludes brace characters so
   an earlier, unrelated typedef in the same header can't be swallowed into the match and have its
   pointer fields misattributed.
3. **Resolve real sizes.** Compile a small probe that `printf`s `sizeof(each symbol)`, **run it**, and
   read the sizes back. Types that only exist as file-local typedefs (`MenuMem1`, `MenuMem2`,
   `EvtEntityProperties`) are re-declared inside the probe (`LOCAL_TYPEDEFS`).
4. **Emit `generated_data.c`.** Map each safe symbol's VRAM address (from `symbol_addrs.txt`, then the
   ELF's `nm` output) to a file offset via the ELF's PROGBITS section headers (or the PS-X EXE header
   when no ELF/`readelf` is available), and emit its bytes plus a constructor `memcpy`. The file is
   only rewritten when its text changes, so a no-op build doesn't recompile the multi-MB unit;
   content equality is a complete staleness check because every input (ELF bytes, target width,
   sanitizer flags, the script itself) changes the emitted text.

### The width and cross-compile traps

Two things about step 3 are easy to get wrong, and both bit this project:

- **Probe at the right width.** Struct sizes containing pointers differ between `-m32` and `-m64`;
  probing at the wrong width silently produces a `generated_data.c` whose objects are the wrong size
  (silent data corruption, not a loud failure). The generator takes the target width from the build
  (`VH_TARGET_MARCH`) rather than guessing.
- **The probe must run on the build host.** Step 3 *compiles and runs* a binary to read `sizeof`. Under
  cross-compilation that binary would be a Windows `.exe` that can't execute on the Linux build host —
  so the probe uses a **host** compiler (`VH_HOST_CC`), which is safe because the probed "safe" symbols
  are pointer-free and therefore the same size on host and target (a worst-case mismatch only
  over-reserves storage, never under). Without this, every size comes back unresolved and the final
  link fails. See [../cross-platform.md](../cross-platform.md).

### Driver hooks and their pitfalls

The generator is build-system-agnostic via environment hooks, so the Makefile and CMake drive the
same script:

| Variable | Meaning |
|---|---|
| `VH_CC` | target compiler for the undefined-symbol link (default `gcc`) |
| `VH_HOST_CC` | compiler for the `sizeof` probe (defaults to `VH_CC`) |
| `VH_OBJ_FILES` | explicit object list; CMake passes `$<TARGET_OBJECTS:…>` because its layout differs from the Makefile's `build/src/*.o` glob |
| `VH_LINK_LIBS` | explicit probe link libraries; default is pkg-config `sdl2`+`openal` plus `-lGL -lm` |
| `VH_TARGET_MARCH` | width flags, parsed as an argument string (`-m32`, `-m64`, `-arch x86_64` …) |
| `VH_EXTRA_CFLAGS` | extra probe-compile flags (Windows `-include`s the MinGW compat shim) |
| `VH_SAN` | the build's sanitizer flags |
| `VH_BUILD_DIR` | output tree (`build`, `build32`, …) |
| `VH_GAME_ROOT`, `VH_PSX_BASENAME`, `VH_PSX_EXE` | region selection (see below) |

Details that matter:

- An **empty `VH_TARGET_MARCH`** means "no width flag" (native width, the CMake 64-bit build). It must
  become an empty argument list, not `['']`: an empty string passed to the compiler is a bogus input
  filename and the probe link fails before any undefined reference is seen. CMake omits the variable
  entirely — an empty `VH_TARGET_MARCH=` through `cmake -E env` makes `env` treat the next token as the
  command.
- **`VH_SAN` must be forwarded.** The undefined-symbol step links the real build's objects; if those
  were compiled with `-fsanitize=…`, the link fails on undefined `__asan_*` symbols unless the same
  flag is passed. The failure isn't obvious — the script just falls back to its slower path.
- **The work directory must exist before the first probe link.** The link writes
  `-o <work>/symprobe`; on a fresh build tree a missing directory yields a "No such file" error that
  carries no `undefined reference` lines, and the generator would emit an *empty* `generated_data.c`.
  A probe failure that yields no names is therefore fatal rather than silently continued.
- **Object paths are `VH_BUILD_DIR`-relative.** With a hardcoded `build/`, a
  `make link M32=-m32 BUILD_DIR=build32` links the 64-bit objects with `-m32`: the link fails on the
  arch mismatch and gcc truncates its `-o` target, deleting the working 64-bit binary.
- The 32-bit path resolves pkg-config libraries from `/usr/lib32/pkgconfig`.

### Size overrides

A few symbols have no authoritative size anywhere: not in `symbol_addrs.txt`, size 0 in the ELF's
symbol table. `SIZE_OVERRIDES` pins them. Gap-to-next-symbol is *not* a usable estimate for the
scratch buffers, because splat emits a `D_XXXXXXXX` label for nearly every individually-referenced
byte inside them, so the "next symbol" is often 1–2 bytes away inside the same conceptual buffer.
Instead each is sized from evidence — the largest `name + 0xNNNN` offset expression used anywhere in
`src/`, plus a safety margin:

| Symbol | Size | Evidence |
|---|---|---|
| `gText` | `0x2ab0` | gap-to-next matches in both region maps |
| `gSeqData` | `0xb800` | no offset usage; gap-to-next matches in both region maps |
| `gScratch1_801317c0` | `0x10000` | usage up to `+0xa000` |
| `gScratch3_80180210` | `0x80000` | usage up to `+0x3a300` (`gUnitDataPtr` et al.); a smaller guess crashes real gameplay code |
| `additional_VRAM` | `0x14700` | see below |
| `gMenuMem_*` | 2 / 12 | file-local typedefs `MenuMem1`/`MenuMem2` |

`additional_VRAM` is the instructive one. The linker symbol `additional_VRAM_END` gives it
`0x12970` bytes, but `SwapOutCodeToVram` (`src/units/load.c`) ends with a `StoreImage` into
`&additional_VRAM[0x10380]` with `w=60, h=144` — a 17280-byte write ending at `0x14700`, 7568 bytes
past the linker size. On real hardware that write spills into adjacent, harmless memory and is
genuine working game behaviour; in the PC build it lands on whatever global the host compiler places
next (`gClutIds`, zeroing every unit sprite's palette to black). `additional_VRAM_END` marks a
*segment* boundary in the original address space, not the last byte the code touches, so the buffer
is sized to the observed write instead. The whole override table is identical for both regions
(JP's `SwapOutCodeToVram` uses the same ceiling); re-verify it if either tree's usage changes.

### Manually defined symbols

`MANUALLY_DEFINED` lists flagged symbols that have a real definition elsewhere in `platform/pc/`; the
generator must skip even the zero-initialised tentative definition for them, or the two definitions
collide at link time. Each entry's pointer targets are resolved against the byte-exact binary and
rebuilt as a local blob plus per-element pointer fixups (the reconstructions below). The set is
deliberately not a general mechanism for the remaining ~56 zeroed flagged pointers — only the ones
that are read at runtime and were root-caused. Two entries are special: `gTacticalMode` is a PC-only
flag defined in `pc_balance.c` (not a ROM symbol), and `gTextPointers` (0x8012be9c) is **not** listed
on purpose — it is a runtime-filled `.bss` table, all-NULL in ROM, that the game populates at load.

## Region parameterisation

Every reconstruction is region-parametric. The generators take `VH_REGION=us|jp`,
`VH_GAME_ROOT=<tree>` (the game tree providing `symbol_addrs.txt` and the byte-exact executable; the
US repo root by default, `jp/` for `REGION=jp`), `VH_PSX_EXE=<exe>` and `VH_GENERATED_OUT=<path>`.
Symbol **names** are shared between the trees (the JP decomp keeps US naming at JP addresses), so one
script serves both — only addresses and window bounds are region-keyed. The US files are committed
under `platform/pc/src/`; the JP build regenerates every reconstruction into `$(BUILD_DIR)/gen/`
from its own executable and never writes into `src/`. The JP map carries no `// size:` annotations,
so where a generator needs a table size it falls back to the US map's annotation for the same name,
clamped by the JP map's own gap-to-next-symbol (a hard upper bound — the plain gap overshoots, e.g.
`gAnimSet_800f2db4` is `0xd0` bytes but its gap is `0x24c0`).

## Hand-written data files (committed)

A handful of pointer-typed game-data tables are reconstructed from the byte-exact binary (values read
directly from `SLUS_004.47` at addresses given by `build/SLUS_004.47.map`, ROM offset via the
containing subsegment's rom/vram base). Each follows the same relocation rule: the source span is
embedded verbatim as a byte blob and every pointer becomes `blob + (target_vram - blob_base)`, so
in-data terminator conventions land at the same relative positions the game expects.

### `pc_battle_data.c` — battle unit initial states

`gBattleEnemyUnitInitialStates[74]` and `gBattlePartyUnitInitialStates[50]` (`include/battle.h`).
`SetupBattleUnits` (`src/states/game_setup.c`) dereferences
`gBattleEnemyUnitInitialStates[gState.mapNum]` unconditionally, so a NULL entry is a guaranteed crash
on entering any battle. Every one of the 74+50 pointers lands inside one contiguous span,
0x800f9f68–0x800fbbb8 (splat subsegment `[0x0e9b58, data]`), extracted as two blobs. Records
terminate on `stripIdx == 0` (enemy) / `partyIdx == UNIT_INVALID (0xff)` (party). Several map indices
share one all-zero sentinel pointer whose single terminator byte aliases the start of map 0's own
data; `SetupBattleUnits` reads only `stripIdx` before stopping, so the overlap is safe by
construction. Both blobs are read-only in the game code and are `const`.

### `pc_unit_anim_data.c` — `gUnitAnimSets`

The three-level per-unit animation table: set pointer → per-`AnimIdx` anim-pointer array →
self-terminating byte strips parsed by `UpdateUnitSpriteAnimation` (`src/core/object.c`). Every level
is extracted and relocated to local symbols; sub-arrays shared by several units and genuinely-NULL
slots are preserved. The array is sized `[489]`, not the hardware `[144]`: `SetupSprites` indexes it
with ids up to 488 from the shipped scene table, and entries 144..488 are NULL for safety (the
hardware bytes there are adjacent non-pointer data). Full rationale at the `PERMUTER`-gated extern in
`include/graphics.h`. Generated by `tools/gen_unit_anim_data.py`; the US output is committed.

The `[489]` size comes from an exhaustive scan of the shipped `gSceneSpriteStripUnitIds` table
(byte-identical in both regions), which carries ids up to 488. Two earlier widenings undersized it
the same way `gTravelAscentCost` was undersized (see [width-bugs.md](../width-bugs.md)): first `[192]`
off the first observed index, then `[301]` off a reasoned ceiling that the actual scene table
contradicted. Sizing from an exhaustive data scan, rather than a single observed or reasoned value, is
the fix in both cases.

The widening is a safety measure, not a faithfulness one: unlike the generator-extracted battle
tables, `gUnitAnimSets` is a *reconstructed* pointer table, so the hardware bytes past index 144
(adjacent `gUnitClutIds`, read as pointers) cannot be faithfully reproduced — entries 144..488 are
NULL so the read stays in-bounds and yields NULL instead of dereferencing a wild pointer (the pointer
this table backs is dereferenced in `src/units/actor.c`). Cutscene units take their animation set from
the event-entity tables instead (below), which is why the NULL padding hasn't been observed to break
a cutscene sprite; if one ever does look wrong, the fix is to reconstruct that event-sprite animation
set, not to grow this array further.

### `pc_event_anim_data.c` — cutscene `gAnimSet_*` tables

The cutscene/event unit animation-set pointer tables. With these NULL, event units resolve
`animData = NULL → gfxIdx = 0` → a blank texture → invisible cutscene units (the battle path is
unaffected because it goes through `gUnitAnimSets`). Every `gAnimSet_*` table from the symbol map is
re-based onto one verbatim blob spanning from the lowest strip target to the end of the last table;
the window interleaves strip bytes with the tables themselves, and the embedded raw table bytes are
never indexed, which keeps offsets trivial. `gAnimSet_80101fc0` is excluded because
`src/maps/unpack.c` defines it. Generated by `tools/gen_event_anim_data.py`; the US output is
committed.

### `pc_sprite_box_quads.c` — `gSpriteBoxQuads`

`gSpriteBoxQuads[19]` (`include/graphics.h`): each entry points directly at a `Quad`
(`typedef SVECTOR Quad[4]`, 32 bytes) consumed by `RenderUnitSprite` via `RotTransPers4`. All 19
targets fall in one 512-byte span of 16 Quads ending exactly where the array itself begins; several
entries share a Quad (8, 9 and 11 point at the same one), and that sharing is preserved.

**The blob must be writable RAM, not `rodata`.** Unlike the battle and unit-animation blobs, the
Quad data is written at runtime: `AddObjPrim8`/`RenderUnitSprite` (`src/core/object.c`) negate a
box's Y coordinates in place and restore them a few lines later, and `src/units/actor.c`,
`src/spells/shared_fx.c`, `src/spells/dark_hurricane.c` and `src/maps/map_28_31.c` swap
`gSpriteBoxQuads[N]` between Quad sources. Declaring the blob `const` puts it in read-only memory and
the first in-place rotate is a write-protection SIGSEGV at a perfectly valid address.

### The frozen-live-global trap

`pc_sprite_box_quads.c` also illustrates a recurring bug class in these files. On hardware
`gSpriteBoxQuads[0/7/8/9/11]` and the named globals `gQuad_800fe53c`/`gQuad_800fe63c`/
`gQuad_800fe65c` are the *same object* (same address). Splitting each into a frozen blob copy plus a
separate writable global means the per-frame by-name writes (`RotateProjectile` in
`src/battle/projectile.c` for the arrow, a dozen `fx_*` effects, the airman shadow in
`src/units/actor.c`) never reach the renderer, which reads the frozen copy — a flat, un-rotated arrow
and stuck effects. Those entries therefore alias the live named globals. When reconstructing any
pointer table, confirm each entry references the live named global where one exists, not a snapshot.

## Regenerated game/BIOS-derived data files (gitignored)

Five data files are **not committed** — they're regenerated at build time by `tools/gen_*.py` from your
own copy of the game, because they reconstruct copyrighted content:

| File | Content | Source |
|---|---|---|
| `pc_kanji_font.c` | PS1 BIOS kanji glyphs | PsyQ `KROMDAT.BIN` |
| `pc_spell_descriptions.c` | in-game spell text | `SLUS_004.47` |
| `pc_item_descriptions.c` | in-game item text | `SLUS_004.47` |
| `pc_evt_entities.c` | event-entity pointer table | `SLUS_004.47` |
| `pc_string_table.c` | string pointer table | `SLUS_004.47` |

Both inputs (`build/SLUS_004.47` and the PsyQ `KROMDAT.BIN`) are already required by the build, so
regeneration adds no new dependency — but it keeps the source tree free of Sony BIOS data and Konami
text (the same model as `generated_data.c`). They're listed in `.gitignore`; both build systems
regenerate them automatically. See [NOTICE](../../NOTICE) for the licensing rationale and
[building.md](../building.md).

### `gen_evt_entities.py` — `gEvtEntities`

`gEvtEntities[95]` (US 0x8010308c) holds per-event pointers to `EvtEntityProperties` arrays
(`altAnims`, `baseAnims`, two strip indices). `LoadEvent` gives every cutscene entity its anim set
from here, so a NULL table means invisible cutscene units. Each `altAnims`/`baseAnims` pointer is
relocated to the PC symbol it targets: a named `gAnimSet_*` table (or an element inside one), a
`gUnitAnimSets` slot, or one of the *unnamed* anim-set arrays that live in a fixed window just below
`gStringTable` and just below `gAnimSet_80104034` (US `0x80101000–0x80104000`, JP
`0x80103504–0x80105fa4`; the two windows span the same content). Unnamed arrays are read until the
first entry that is neither NULL nor a pointer into that window — that is the real end of each array.
The read cap is 256 entries, not 32: `sArr_80102538` is indexed at 32 by a Grog-house prop
(`animIdx = 32`), so a 32-entry cap truncates a real array and the lookup reads the wrong sprite.
Over-reading into a contiguous following array is harmless (extra valid pointers nothing indexes).
The anim bytes those arrays point at are embedded as one blob (`sEvtAltBlob`).

### `gen_string_table.py` — `gStringTable`

`gStringTable[100]` (US 0x8010102c, JP 0x80103530) is defined in `src/core/text.c` via
`#include "assets/8010102c.inc"`, whose entries are absolute PSX addresses into the byte-exact
placeholder blobs of `text.c` (`D_800151C8[888]`, the `D_80122FBx[8]` arrays). On hardware the data
sits at exactly those addresses; on PC it is relocated, so every entry dangles and everything drawn
from the table (world-map destination names via `gStringTable[33]/[34]`, every `#N` escape in
`DrawText`/`DrawSjisText`) renders blank. Rather than gate the byte-exact definition, a constructor
rewrites each entry to an embedded copy of the real string at startup. Static-initialised data is
ready before any constructor runs, and the game's own runtime self-copies
(`gStringTable[0]/[32] = gStringTable[x]`) copy already-fixed pointers. NULL entries (88..99) and the
PC-added sentinel entry 100 (used transiently by `src/world/dojo.c`) are normalised to one stable empty
string, so every in-range lookup is safe on every target without relying on fault fixups.

### `gen_item_descriptions.py` — item description tables

`gItemDescriptions[101]` (US 0x800ef3d8) and `gItemDescriptions2[101]` (US 0x800ef56c) are `s8*`
tables of item/equipment info strings: the single-line form drawn in the party/overworld item window
(`src/ui/window.c`) and the shop form with an embedded `\n` (`src/ui/supplies.c`). Each pointer is
dereferenced in the binary and emitted as a C string. In JP one table at 0x800f16cc serves both
names, so `gItemDescriptions2` is emitted as an `alias` of `gItemDescriptions` rather than duplicated.
`gen_spell_descriptions.py` follows the same pattern for `gSpellDescriptions`.
