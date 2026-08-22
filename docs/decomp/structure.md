# How `src/` is organized

The decompiled game code is 88 files in nine domain folders, 1184 matched functions, zero
address-derived names. This page explains the layout, the naming conventions, and the one
law that governs everything: **a file is a contiguous address range of the retail binary.**

This page describes the US tree (`src/`, the reference). The Japanese decompilation (`jp/src/`,
since 2.0) mirrors the same tree and conventions for its own binary; most files are byte-shared
with the US tree (verified by the port build), and only genuinely diverged revisions exist twice.

## The tree

```
src/
├── core/     the frame loop and shared machinery: main (the game-state machine), engine,
│             object (the actor system) + obj_function_pointers (its 804-slot dispatch
│             table), graphics, audio, cd, card (memory card), text/glyphs (SJIS), movies
├── states/   menu and setup game states: main_menu, game_setup, scene_state_setup,
│             ending_screen, debug_menu
├── battle/   the tactical layer: field (turn driver + front-end), executors (action
│             objects), math (damage/XP/stats), ai (the five-planner enemy AI),
│             evaluators (per-map win/lose rules), path_grids, spell_targeting,
│             presentation (cameras), results, projectile, item_reveal
├── units/    the unit actor (one object per unit on the map), roster bookkeeping,
│             VRAM sprite loading
├── world/    the overworld layer: world map + its state, towns, tavern, dojo, config
├── events/   the cutscene machinery: scene_loaders, entities (the EVDATA interpreter),
│             fx_scenes (per-scene set pieces), fx_helpers (the shared FX toolbox)
├── spells/   one file per spell family (salamander, avalanche, dark_hurricane, ...) plus
│             casting_main (the gSpellsEx MAIN-slot drivers), hit_effects and shared_fx
│             (the two shared FX banks), support_magic (healing/status/buffs)
├── maps/     per-map scripted scenery (map_NN files), buttons (the shared button-press
│             kit), ambience, unpack (map decompression), objects, setup_objects, common
└── ui/       the window system, the unit status window, the shop/depot screens
```

Companion references: [objf-handlers.md](objf-handlers.md) (every object handler, its
file, and how retail reaches it), [spell-fx-dispatch.md](spell-fx-dispatch.md),
[event-scripts.md](event-scripts.md).

## A file is an address range

The retail binary was linked from Konami's compilation units by a compiler (GCC 2.6.3)
with no `-ffunction-sections`: every original TU is one solid block of addresses. The
decomp's files reproduce those blocks — `SLUS_004.47.yaml` pins each file to its exact
range, and the build proves byte-identity (`make check`) after every change.

Consequences worth knowing before you judge the layout:

- **Functions cannot move between files.** A file can be *split* at a function boundary
  and the pieces renamed, but a function whose address sits inside another family's range
  is stuck there. Files document these as **strays** ("kept by address contiguity") in
  their header comments — e.g. the chest-impact effect living in `spells/dark_fire.c`.
- **Some data is address-locked in odd places.** The most visible case: Salamander's
  initializer templates live at the head of `spells/dark_hurricane.c`'s rodata because
  that is where the retail compiler emitted them — the header comments on both sides
  explain the arrangement.
- **The family view lives in the docs, not the tree.** Where a spell's pieces span files
  (dispatch slots in different ranges), [objf-handlers.md](objf-handlers.md) and
  [spell-fx-dispatch.md](spell-fx-dispatch.md) are the map.

## Naming conventions

- **`ObjfNNN_Name`** — object handlers keep their `gObjFunctionPointers` indices in the
  name; multi-slot handlers list every index (`Objf004_005_408_Window`). The index is the
  bridge to the data tables (spells, event scripts, map tables), which address handlers
  by number.
- **`_FX1` / `_FX2` / `_FX3`** — spell-handler suffixes recording the `gSpellsEx` slot
  (MAIN / per-target / the alternate-outcome slot: slay for damage spells, no-stick for
  ailments, dormant for support). See [spell-fx-dispatch.md](spell-fx-dispatch.md).
- **`_Unused`** — in `gObjFunctionPointers` but reachable from no spell table, no retail
  event script, no data table and no code path: cut content, kept byte-exact.
- **`Objf_Unk_*` / `func_*`** — not referenced by anything at all (not even the dispatch
  table); these keep address-derived names by policy: a name would be pure invention.
- **Mechanism names over guesses** — where a handler's spell/scene identity is not proven
  by data or a witnessed run, the name says what the code does (`ConvergingSparkle`), not
  what it might belong to.

## The path grids (battle/path_grids.c)

The tactical layer's spatial queries run on eight `u8[30][65]` grids (`gPathGrid0..6`,
`gPathGrid10`, with a 0xff border ring). One byte per cell serves four value flavours:
breadcrumb steps (walked back to build a path), ascending accumulated move cost,
descending remaining range (a distance potential field, also used by the AI's proximity
scoring), and flat coverage masks (spell fields, attack ranges). The file header carries
the full inventory of which populator writes which flavour into which grid.

## Rules for touching `src/`

1. **Byte-identity is the contract.** After any change under `src/` or `include/`, run
   `make check` — the build must reproduce the retail binary hash exactly. PC-specific
   code goes behind the established gates (`PC_PORT`, `PC_FEAT`, `PC_DEBUG_*`,
   `PERMUTER`); see [architecture.md](../architecture.md).
2. **Renames are free, layout is not.** Renaming files/symbols and adding comments cannot
   change bytes. Anything that moves code or data — splitting a file, hoisting a local,
   reordering declarations — can, in ways the compiler makes non-obvious:
   - GCC 2.6.3 emits jumptable alignment padding that can vanish when a TU is split
     (fixed with explicit pad segments in the yaml);
   - calls to a formerly-same-TU function compile differently without a prototype
     (narrow argument conversions disappear);
   - identical local initializers share one rodata template across functions, and local
     aggregate initializers cannot be rewritten as expression initializers under this
     compiler.
   Every one of these will show up as a hash mismatch — treat `make check` as the judge
   and diff section sizes in the link map to locate the cause.
3. **Do not "fix" retail behavior in `src/`.** Suspected retail bugs are documented in
   place (see `TryInflictingAilment` in `battle/math.c` for an example); behavioral
   changes belong to the PC port layer.
