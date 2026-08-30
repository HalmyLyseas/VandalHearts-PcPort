# Documentation

Developer and user documentation for the *Vandal Hearts* matching decompilation → native PC port.
For the project summary and legal information, see the top-level [README](../README.md).

## Start here

**[architecture.md](architecture.md)** — the two-layer design (byte-exact decompilation + native PC
port), how the port avoids breaking the match, and the repository layout. Read this first.

**[roadmap.md](roadmap.md)** — what's shipped and what's planned (Stage 3 gameplay/QoL). Plans, not
commitments.

**[../CHANGELOG.md](../CHANGELOG.md)** — per-release notes (what changed in each version).

## Building & running

- **[building.md](building.md)** — building the matching decompilation (`make check`) and the native PC
  port (Makefile / CMake), plus the Windows cross-compile and sanitizer builds.
- **[releasing.md](releasing.md)** — the release checklist: correctness gates (smoke + raster
  regression, `make check`), both-platform stage builds, hygiene, publish.
- **[configuration.md](configuration.md)** — supplying the disc image, the `vandalhearts.ini` config
  file, runtime options, and diagnostics. (Full option reference:
  [`platform/pc/OPTIONS.md`](../platform/pc/OPTIONS.md).)
- **[controls.md](controls.md)** — the full gamepad + keyboard control scheme in battle.
- **[troubleshooting.md](troubleshooting.md)** — common problems and their fixes: startup, display,
  HD pack, saves, and how to report a bug usefully.
- **[performance.md](performance.md)** — what the display settings cost, reference numbers, and the
  built-in diagnostics for performance reports.
- **[manual/](manual/)** — source of the Player Manual PDF attached to each release (setup,
  controls, features, troubleshooting in one printable document).
- **[hd-pack.md](hd-pack.md)** — the optional 1.6 HD background pack: installing one, the HD PACK option,
  and building a pack with the tools in `platform/pc/tools/hdpack/`.
- **[language-packs.md](language-packs.md)** — the optional language packs: installing and selecting one,
  what a pack covers, and building one with the tools in `platform/pc/tools/langpack/`.
- **[gameplay-additions.md](gameplay-additions.md)** — the optional Stage-3 QoL features (twin-stick
  camera, unit-cycle, enemy threat overlay, save management) and what they do.
- **[tactical-mode.md](tactical-mode.md)** — the opt-in 1.3 gameplay rebalance (level cap, class
  rebalancing, restored content) and how it stays isolated from the faithful mode.

## Game mechanics (decoded reference)

**[game-mechanics/](game-mechanics/README.md)** — how the game actually works under the hood, decoded
from the byte-exact source. The recurring theme is a **display-vs-real split**: much of what the status
screen shows is cosmetic, while the values that drive combat are never surfaced.

- [combat-mechanics.md](game-mechanics/combat-mechanics.md) — the damage/resist model, evasion, magic
  & ailment chains, and what's real vs fluff
- [classes.md](game-mechanics/classes.md) — the real per-class levers (`gUnitInfo`, resistance metric,
  movement profile)
- [weapons-and-armor.md](game-mechanics/weapons-and-armor.md) — the display-vs-real power tables
- [spells-and-items.md](game-mechanics/spells-and-items.md) — the full spell + consumable table

## The decompiled code (contributor reference)

**[decomp/](decomp/structure.md)** — the map of `src/` for anyone reading or extending the
decompilation:

- [structure.md](decomp/structure.md) — the nine-folder tree, the file-=-address-range law,
  naming conventions, and the byte-exactness rules for touching `src/`
- [objf-handlers.md](decomp/objf-handlers.md) — every object handler in the 804-slot dispatch
  table: name, file, and how retail reaches it (generated from the source + retail data)
- [spell-fx-dispatch.md](decomp/spell-fx-dispatch.md) — the `gSpellsEx` FX model and the full
  71-spell dispatch table, validated live in-game
- [event-scripts.md](decomp/event-scripts.md) — the EVDATA cutscene bytecode: format, the
  interpreter, all opcodes, the census of event-spawned objects, and the `M_*.PRS` map-file format
- [map-effects.md](decomp/map-effects.md) — the per-map scripted scenery in `src/maps/*.c`:
  drawbridges, floodgates, collapsing bridges and the other map-specific set pieces

## The PC port internals

- **[pc-port/overview.md](pc-port/overview.md)** — the swappable-interface design and how the six
  subsystem backends fit together.
- **[pc-port/bootstrap.md](pc-port/bootstrap.md)** — startup: RAM reservation, config, disc mount &
  validation, crash diagnostics.
- **[pc-port/data-segment.md](pc-port/data-segment.md)** — the mid-build data-segment generator and the
  hand-written / regenerated data files.

### Subsystem deep-dives

Each PSX hardware unit and its portable replacement:

| Doc | Subsystem |
|---|---|
| [pc-port/subsystems/gpu.md](pc-port/subsystems/gpu.md) | GPU — rasteriser, ordering table, primitives (SDL2 + Metal/OpenGL presentation) |
| [pc-port/subsystems/gte.md](pc-port/subsystems/gte.md) | GTE — fixed-point 3D geometry math |
| [pc-port/subsystems/spu.md](pc-port/subsystems/spu.md) | SPU / sound — software SPU + the SEQ music sequencer |
| [pc-port/subsystems/cd-xa.md](pc-port/subsystems/cd-xa.md) | CD-ROM & XA audio — disc image model, timing, streaming |
| [pc-port/subsystems/mdec.md](pc-port/subsystems/mdec.md) | MDEC — STR FMV video decode |
| [pc-port/subsystems/kernel.md](pc-port/subsystems/kernel.md) | Kernel — events, VSync timing, pad input, saves |

## Cross-cutting topics

- **[memory-safety.md](memory-safety.md)** — surviving the PS1's protection-free memory model, the
  64-bit port (LP64/LLP64), the width bugs, and the sanitizer sweeps.
- **[width-bugs.md](width-bugs.md)** — the full field catalogue of 64-bit width bugs: every one found,
  how it was caught (and why sanitizers missed most), and the transferable audit lessons.
- **[cross-platform.md](cross-platform.md)** — the Windows (MinGW-w64) build & runtime, and the macOS
  status.

## A note on `exchange/`

The gitignored `exchange/` folder is the project's *investigation history* — BizHawk trace scripts,
derivations, and dead-ends accumulated while building the port. It is working scratch, not committed
documentation; the durable findings from it are distilled into the pages above. If you're chasing *how*
a particular behaviour was pinned down (e.g. a GTE constant or the SPU volume law), that's where the
raw trail lives.
