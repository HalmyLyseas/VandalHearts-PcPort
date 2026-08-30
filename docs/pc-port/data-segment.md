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
big each one is, and emit a `generated_data.c` that defines them as zero-initialised storage of the
correct size (the game fills them at runtime; only the *size* has to be right for the link and the
memory layout to be correct).

The pipeline:

1. **Find undefined symbols.** Link the compiled objects and scrape the linker's
   `undefined reference to '…'` lines. (Every `-l` library must resolve, or the link aborts *before*
   printing undefined refs — a subtle cross-compile trap; see below.)
2. **Find declarations & classify.** For each symbol, locate its declaration in the headers and
   classify it as a pointer type or a plain value type (pointer-containing types are handled
   separately, since their size is width-sensitive).
3. **Resolve real sizes.** Compile a small probe that `printf`s `sizeof(each symbol)`, **run it**, and
   read the sizes back.
4. **Emit `generated_data.c`.** Define each symbol as storage of its measured size.

### The width and cross-compile traps

Two things about step 3 are easy to get wrong, and both bit this project:

- **Probe at the right width.** Struct sizes containing pointers differ between `-m32` and `-m64`;
  probing at the wrong width silently produces a `generated_data.c` whose objects are the wrong size
  (silent data corruption, not a loud failure). The generator takes the target width from the build
  rather than guessing.
- **The probe must run on the build host.** Step 3 *compiles and runs* a binary to read `sizeof`. Under
  cross-compilation that binary would be a Windows `.exe` that can't execute on the Linux build host —
  so the probe uses a **host** compiler (`VH_HOST_CC`), which is safe because the probed "safe" symbols
  are pointer-free and therefore the same size on host and target. Without this, every size comes back
  unresolved and the final link fails. See [../cross-platform.md](../cross-platform.md).

The generator is build-system-agnostic via environment hooks (`VH_CC`, `VH_HOST_CC`, `VH_OBJ_FILES`,
`VH_LINK_LIBS`, `VH_TARGET_MARCH`, `VH_EXTRA_CFLAGS`), so the Makefile and CMake drive the same script.

Before any of that, the generator validates the input executable itself: it requires the file size to
cover the header's own `load_size` and compares the whole-file md5 against the known retail hash for
the selected region, aborting with a one-line message on a wrong-region or truncated exe.
`VH_ALLOW_UNVERIFIED_EXE=1` skips only the hash check (the size check always applies), for a
deliberately modified executable.

## Hand-written data files (committed)

A handful of game-data tables can't be produced by the normal link and are reconstructed by hand from
the byte-exact binary (values read directly from `SLUS_004.47` at addresses given by
`build/SLUS_004.47.map`). These are committed:

- `pc_battle_data.c` — unit/battle stat tables
- `pc_unit_anim_data.c` — unit animation tables
- `pc_sprite_box_quads.c` — sprite-box quad globals
- `pc_event_anim_data.c` — event animation tables

### The frozen-live-global trap

`pc_sprite_box_quads.c` illustrates a recurring bug class in these files. A pointer table must point at
the *live* global the game writes each frame (`gQuad_800fe53c` &c.), **not** at a static copy of its
initial bytes. An early version pointed at static blobs, so arrows/effects/shadows rendered frozen.
When reconstructing a pointer table, confirm each entry references the live named global, not a
snapshot. (See `pc_data_gen_frozen_live_global.md` in the memory notes.)

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
