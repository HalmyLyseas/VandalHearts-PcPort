# Event scripts (`EVDATA*.DAT`)

Every cutscene in the game is data: one `EVDATA<nn>.DAT` file per event scene, interpreted
at runtime by `Objf409_EventEntity` (`src/events/entities.c`). This page documents the file
format, the interpreter, and the full opcode set — derived from the interpreter source and
a parse of all 93 retail script files.

## File format and loading

An EVDATA file is a leading `s16` entity count followed by the entities' scripts
back-to-back. Each script is a flat run of `s16` **(opcode, argument)** pairs, terminated
by opcode `99`. `LoadEventData` (`src/maps/unpack.c`) walks the terminators once to fill
`gEvtEntityDataPointers` — one pointer per entity.

The scene loader (`src/events/scene_loaders.c`) then binds each entity:
`SetupEventEntity` (`src/units/actor.c`) pairs one script pointer with two sprite-strip
slots and spawns one `Objf409` object per entity. `ReserveSprite` copies individual 48x48
cells into strip slots on demand (opcodes `0x33`–`0x3c`).

## The interpreter

`obj->state3` is the run state: `0` create the entity's sprite, `1` fetch the next
(opcode, argument) pair, `2` execute it. Most opcodes complete immediately and loop back
to fetch; *blocking* opcodes (waits, dialogue pauses, choice menus) stay in state 2 until
their condition clears. After the opcode step, every frame ends by walking the sprite
toward its destination and re-deriving its facing (`StepEntitySpriteTowardDest`,
`UpdateUnitSpriteOrientation`, `src/units/actor.c`).

Entities synchronize through `gState.eventResumeLocation`: a script parks on opcode `1`
(wait-until-location) and another entity raises the location with `0x11`/`0x12` — the
scripts' cross-entity branching mechanism.

## Opcodes

Blocking opcodes are marked ⏸.

| op | action |
|---:|---|
| `1` ⏸ | wait until `gState.eventResumeLocation` reaches arg |
| `2` | play base-set animation arg |
| `3`/`4` | walk: set destination X / Z (turn toward it) |
| `5`/`6` | walk: set destination X / Z, keep current facing |
| `7` | set move speed (X and Z) |
| `8` | set facing direction (arg × 1024) |
| `9` ⏸ | wait until movement finishes |
| `0xa` ⏸ | wait until the animation finishes |
| `0xb` ⏸ | wait arg frames |
| `0xc` | relative branch (arg = pair offset from the current command) |
| `0xd`/`0xe` | camera focus on this entity / clear focus |
| `0xf` ⏸ | re-decode this entity's sprite strip; wait for the decoder |
| `0x10` | if `eventResumeLocation` ≥ arg, skip the next command |
| `0x11`/`0x12` | raise `eventResumeLocation` to arg (resumes entities parked on op `1`) |
| `0x13` | teleport: snap position to the pending destination |
| `0x14` | spawn the event zoom object with zoom arg |
| `0x15`/`0x16` | teleport: set X / Z directly |
| `0x17`/`0x18` | walk relative: destination = current + arg (turn toward it) |
| `0x19`/`0x1a` | walk relative, keep facing |
| `0x1b`/`0x1c` | show / hide the sprite |
| `0x1d` | **spawn an arbitrary object by objf index** (see the census below) |
| `0x1e` | open the message box anchored to this sprite (arg: upper/lower) |
| `0x1f` | close the message box (arg: upper/lower) |
| `0x20`/`0x21` | set message text in box 1 / box 2 |
| `0x22` ⏸ | wait until the message finishes |
| `0x23` ⏸ | wait for the page-pause confirm |
| `0x24` | snap camera rotation to arg × 1024 + 45° |
| `0x25` | ease camera rotation toward arg × 1024 + 45° (shortest way around) |
| `0x26`–`0x28` | pan camera: set X / Y / Z and move now |
| `0x29`–`0x2b` | pan camera: set X / Y / Z (eased by the event camera) |
| `0x2c` | pan target = the focused entity's position |
| `0x2d` | set camera zoom |
| `0x2e` | rebind this entity's sprite strip (arg + 2) |
| `0x2f` | select strip A (0) or strip B (1) |
| `0x30` | play alt-set animation arg |
| `0x31`/`0x32` | set message-box portrait (upper / lower) |
| `0x33`–`0x3c` | `ReserveSprite(arg, strip, N)` — load one cell into sub-slot N = op − 0x33 |
| `0x3d` | perform audio command arg (SFX/XA/SEQ control) |
| `0x3e` | stop the sequencer, load SEQ set arg |
| `0x3f` ⏸ | blood spurt on this sprite, then slay it (35-frame stage) |
| `0x55` | blood spurt only (same handler as `0x3f`, non-blocking) |
| `0x40` ⏸ | end the scene: 50-frame delay, then `STATE_SET_SCENE_STATE` |
| `0x41` | set the event camera height |
| `0x42` | open the message box, tail omitted |
| `0x43`/`0x44` | fade out / in, layer 2, over arg frames |
| `0x45`/`0x46` | fade out / in, layer 1, over arg frames |
| `0x47` | set this entity's face-elevation type |
| `0x48` ⏸ | two-option choice window; stores `gState.eventChoice`, choice 0 also skips the next command |
| `0x49` | one-line prompt window |
| `0x4a` ⏸ | wait for O/X, then close the prompt |
| `0x4b`–`0x4e` | set map window: min X / min Z / size X / size Z |
| `0x4f`/`0x50` | stretch-warp the sprite out / in (teleport visual) |
| `0x51` | close the prompt window |
| `0x52` | spawn a face-elevation edit object, mode arg (`src/maps/unpack.c`) |
| `0x53` | spawn a sliding-face object, mode arg |
| `0x54` | kill every object with functionIndex arg |
| `0x56`/`0x57` | precise (sub-tile) sprite positioning on / off |
| `0x58` | draw the lower dialogue window frame |
| `0x59` | set camera height to this sprite's elevation |
| `0x5a`/`0x5b` | screen-effect object: state 5 (stop) / state 0 (restart) |
| `0x5c` | set screen-effect ordering |
| `0x5d` | set screen-effect semi-trans rate |
| `0x5e`–`0x60` | screen-effect color R / G / B |
| `0x65`–`0x6a` | screen-effect color deltas rd/gd/bd and caps rmax/gmax/bmax |
| `0x6b`/`0x6c` | set / add global light level (R=G=B) |
| `0x6d`–`0x6f` | screen-effect color / deltas / caps, all three channels at once |
| `0x70`/`0x71` | set screen-effect semi-trans mode |
| `0x72` | clear path grid 0 |
| `0x73` | set camera pitch |
| `0x74` | spawn the screen-fade object |
| `0x75`–`0x78` | event-eased fades: out/in layer 1, out/in layer 2 |
| `0x79` | kill the fade object |
| `0x7a` | set message text, variant 2 (box 1) |
| `99` | end of script — the entity parks (no case matches; the sprite keeps rendering) |

## Map data files (`M_*.PRS`)

Map geometry ships compressed as `M_*.PRS` files, decoded by `UnpackMapFileData`
(`src/maps/unpack.c`). One control byte supplies eight mode bits, each selecting either a
literal byte or one of three back-reference forms: a 10-bit absolute index into the
dictionary, a 4-bit relative distance, or a run of up to 70 literals. The dictionary is a
1 KB ring (primed with a write cursor starting at 990) that lives in the PS1's fast RAM at
`0x1f800000` — see [memory-safety.md](../memory-safety.md) for how the port's US and
Japanese trees handle that hardware address differently. `ProcessMapFileData` reads the
4-byte length header and unpacks into `gMapDataPtr`; `LoadMap`
(`src/states/game_setup.c`) is the only caller.

`src/maps/unpack.c` also owns two small map-scenery handlers reached through event opcodes
`0x52`/`0x53`: `Objf683_AdjustFaceElevation` and `Objf684_SlidingFace` both index a
76-entry sliding-face table by `obj->state2` and move one tile-model face's elevation —
the first in a single step, the second a few units per frame.

From roughly its midpoint, the rest of the file is data: dozens of byte arrays in the
unit-sprite animation format — `s8` (frameIdx, delay) pairs closed by a zero pair, plus
5-byte records (opcodes `0x3b`/`0x3c`) carrying a per-frame position offset — gathered into
the game's animation-set table. Entries come in front/back pairs because animation is
indexed `animSet[animIdx + facingFront]`; `src/events/scene_loaders.c` passes these as
event entities' base/alt animation sets.

## The `0x1d` spawn census

Opcode `0x1d` is the escape hatch: it spawns **any** object by raw `gObjFunctionPointers`
index, and it is the mechanism behind almost every per-scene effect (the set pieces in
`src/events/fx_scenes.c`, the ambience in `src/maps/ambience.c`, the per-map cinematics).
A parse of all 93 retail EVDATA files yields the complete set of event-spawned indices —
56 of them:

```
 87  89 253 265 266 271 303 305 319 323 328 340 393
530 535 536 540 541 542 543 544 545
673 676 679 680 687 689 691 692 694 697 699
713 720 724 725 726 729 730 731
741 742 743 744 746 750 752 753 754 755 756 758
794 797 798
```

Each index's handler and file are in [objf-handlers.md](objf-handlers.md). This census is
one of the reachability sources behind that table's "reached via" column — an index that
appears in no spell row, no event script, no data table and no code path is cut content.
