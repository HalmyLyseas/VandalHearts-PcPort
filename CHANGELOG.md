# Changelog

Notable changes to the **Vandal Hearts PC port**. This tracks the port layer (Stage 3 gameplay/QoL and
packaging); the underlying decompilation stays byte-for-byte faithful to the retail game, and the normal
mode is unaffected by any of it. Format loosely follows [Keep a Changelog](https://keepachangelog.com/).

## [1.3.0] — Tactical Mode + fixes

### Fixed (both modes)
- Fixed a crash that could occur when transferring items to or from the convoy/depot.
- Fixed a crash that could occur during certain cutscenes.

### Quality of life (both modes)
- **Return to Title** — a new options-overlay entry to jump straight back to the title screen from
  anywhere, without restarting the app or sitting through the intro. Confirms first (unsaved progress is
  lost).
- **Enemy threat overlay** — now consistent at every camera angle (it no longer leaks onto off-field
  tiles, nor drops from genuinely-threatened tiles, when you rotate), and its pulse is softer and easier
  on the eyes over large danger zones.
- **Save management** — destructive prompts (restore, delete) now show their question in red; the
  inspect action is relabeled **"Inspect file content"** for clarity.
- Fixed the **Fullscreen** option being un-toggleable in the overlay.

### New — Tactical Mode (opt-in, in testing)
An optional, fully-isolated rebalance for a more varied tactical experience — a per-chapter level cap
that ends experience-grinding, Trials of Toroah that scale to your chapter and reward gold + XP, class
reworks (Monk/Ninja, Guardsman/Dragoon mobility), a reined-in Vandalier, and restored cut content (item
descriptions, two cut weapons). **Off by default**; the normal mode stays byte-for-byte the original, and
Tactical saves are kept separate. See the [Tactical Mode guide](docs/tactical-mode.md).

> ⚠️ **In testing** — later chapters are still being validated in playtest; balance numbers may change in
> 1.3.x. Normal mode is unaffected.

## [1.2.0] — Video options + save management
- **Video** — window scale (X1–X8) and fullscreen from the in-game overlay, applied live.
- **Save management** — unlimited whole-card backups in the overlay: back up, restore (with a "back up
  first" safe default), delete, and inspect a backup's three slots. Each backup stays a
  real-hardware-valid card.

## [1.1.0] — Controls
- Bidirectional unit-cycle on the shoulder buttons (freeing the Square button).
- **Enemy threat overlay** — press Square to see every enemy's combined move-and-attack reach at once.
- In-game options overlay (**Select + Start**), with right-stick camera-axis invert.
- Saves and config now live next to the executable, independent of the launch directory.

## [1.0.0] — Faithful port
- Complete beginning-to-end experience on Windows and Linux, from your own disc.
- Skip intro / movies with Start; camera rotation and elevation on the right analog stick.
