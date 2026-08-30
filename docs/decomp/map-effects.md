# Per-map scripted scenery (`src/maps/*.c`)

The maps folder is more than terrain: a dozen battle maps carry a scripted set piece —
a lowering drawbridge, a draining floodgate, a collapsing bridge, a boss's terrain
reshaping mid-battle — driven by its own small object family. This page is the
choreography reference for those files, decoded from the map drivers and the evaluator
objects that trigger them (see [structure.md](structure.md) for where `maps/` sits in
the tree, and [objf-handlers.md](objf-handlers.md) for each handler's index and file).

## The hub and shared toolkit

`SetupMapExtras()` (`src/maps/common.c`) runs once per field/battle setup, called from
`src/states/game_setup.c` right after the per-map chest/scenery pass
(`src/maps/setup_objects.c`). It switches on `gState.mapNum` and spawns that map's
scripted-scenery driver plus ambient objects (rain, chimney smoke, one map's seven
static torch flames) and the shared camera-zoom service object. `InEventScene()` picks
the cutscene-vs-battle variant of a driver and gates the battle-only ones.

Every map file draws on a shared toolkit: `AdjustTileElevation`, `AdjustFaceElevation[2]`
and `SetFaceElevation` (plus matrix versions `RotateMapTile`/`RotTransMapTile`) edit tile
geometry in place, and `PositionCamera`/`PanCamera` move the camera for a cutaway.

## The button-press idiom

`DepressButton()` sinks a lever tile instantly, for a reloaded save where the button was
already pressed. `Objf346_ButtonDepress` is the animated version — the cutaway a player
actually sees: rotate and pan the camera onto the tile, sink it over four frames, then
park until the parent driver notices and frees it. Several maps spawn it directly; two
lever-driven maps and one drawbridge map wrap it in their own per-map driver that walks
`mapState` 1 (cutaway) → 2 (act on the result) → 3 (ease the camera back). Every driver
that runs a cutaway saves and restores the main camera around it and eases back in rather
than cutting.

## Sand mounds

A diamond-shaped sand mound can be raised and lowered tile by tile: one handler walks
outward ring by ring spawning one raise/lower-tile child per tile, so the mound rises or
sinks as a wave rather than all at once. An event-script entry point spawns the same
mound directly from a cutscene.

## Drawbridges

One shared drawbridge driver serves three maps. It picks the bridge's position, hinge
axis, texture set and displayed battle number from `gState.mapNum`, then draws the raised
bridge as two extra tile models — the file's own embedded tile-model data, retextured per
map — rotated about the hinge every frame. On trigger it pans the camera, drops the
bridge with a bounce, kicks up dust-cloud puffs along the span, lowers the deck tiles and
marks them walkable. A separate instant path applies the lowered state directly on a
reloaded save. The three maps differ only in tile coordinates, hinge orientation and
textures — one driver, three configurations.

## Bridges that come apart

One map's collapsing bridge edits the tile models directly and throws exploding-tile
debris in a stacked vertical burst; separate scene and battle drivers (chosen by
`InEventScene()`) trigger it. Another map's set piece is different in kind: an
ocean-approach cutscene stages two boarding planks raising into place, hides the map's
own water tiles, and draws a tiled open-ocean plane in their place, with a placeholder
object standing in for the boarding sprites.

## Floodgates and elevators

One map's floodgate slides its gate tiles' faces open on trigger — gated by a
"protect the objective" evaluator — spraying particles and draining the water behind it;
a separate instant path lowers the gate and removes the water on a reloaded save. A
neighbouring map instead has two rising/sinking elevator platforms: examining either side
tells the driver which platform should rise and which should sink, and one handler
(shared by both platform indices) branches on which of the two it is.

## Cell doors

One map's four lever tiles are paired with five cell doors through an explicit
lever-to-door index. On a reload every already-pressed lever's door opens instantly; in
play, each lever the evaluator marks pressed takes the camera, runs the cutaway, pans to
its paired door, and raises the door's two tiles out of the floor and retextures them
walkable — then returns to scanning so several doors can open over the course of one
battle.

## Gates, torches and shrinking doors

One map's gate sinks its three tiles while the camera shakes, smoke vents, and the door
textures fade in. A shared flame handler serves two roles: a static semi-transparent
torch (planted seven times on one map) and, on a different map, a flame that orbits a
stored origin at a shrinking radius while fading out — laid out one flame per fixed angle
step through a full circle while the camera height jitters. A separate cutscene handler
trims a set of door textures' height over 32 frames to track a door lowering, elsewhere in
the map files.

## The collapsing rail bridge

One boss map's set piece tracks which of four rail cars has been released. As each
releases, the driver focuses the camera, plays the matching cue, then — with normal field
rendering suppressed — redraws the doomed stretch of track under a sliding camera offset,
substituting bare-rail models for the emptied section and re-rendering clones of any units
caught on it. It finishes by rewriting those tiles to bare rail and marking the crushed
units and their tile as no longer enterable. A companion cutscene handler flies the camera
in along the rails, orbiting a moving focus point.

## The lava-pit platform

A five-tile plus-shaped platform can be saved and restored (its tile models stashed and
reinstated through a scratch buffer) and lowered instantly by N steps for a reload. In
play, each time the evaluator advances its progress counter, the driver focuses on the
unit standing on the platform and spawns one drop: it rocks the saved tile copies with a
sine wobble while re-rendering a clone of the rider's sprite, then commits one step down.
A shared exploding-tile handler — used by several other maps too — arcs a rotating copy of
a tile model, bounces it, trails an explosion effect, and either settles or, on water,
throws a splash and drifts downstream.

## The collapsing span

One map's bridge run branches on whether any player unit stands on its deck when the
evaluator triggers the collapse: with no one on it, the span simply drops; otherwise the
driver walks the deck tile by tile, prompting a dialogue line for each occupied one first.
Either path throws exploding-tile debris built from the span's own tile model, vents
smoke, marks the doomed units as no longer present, and swaps the deck tiles for water.

## The flooding channel

Once its evaluator advances, one map's floodgate driver runs the cutaway, spawns the
floodwater and the raising-gate object together, sweeps a multi-tile channel prompting a
dialogue line for each occupied non-flying unit, and spawns a wash-away handler per
victim: it hides the real sprite and re-renders an accelerating clone until the clone
passes the far edge of the map, then marks the tile empty. The gate object lifts its three
tiles' faces, carrying the attached water face along until level.

## The submerging/surfacing map

One late-game map authors its submerged sections offset well outside the visible map
bounds; setup shifts and sinks that staging copy into place, and a swap routine exchanges
a rectangle of it back in as each section surfaces, re-deriving terrain from the tile's
texture. As the evaluator advances, the driver pans and raises the next section, dressing
each emerging tile with a splash effect that fires once a tile's highest vertex breaks the
surface; a scripted-cutscene variant does the same for the story sequence.

## The barricade

A final map's barricade waits on the evaluator, then spawns a handler that sinks the
barricade while the camera shakes and smoke vents; a companion function applies the
flattened state instantly on a reload. Part of that handler's later states are unreachable
copy-paste from a different map's gate handler and still address that other map's tiles —
dead code kept byte-exact rather than corrected.

## Ambience with no C spawn site

One event opcode names an object-function index directly rather than through a spawn
call, which is why several ambient handlers — chimney smoke, a forcefield effect, rain,
ripples, a campfire — have no C spawn site at all: every reachable instance of them comes
from an event script, not from map setup code.
