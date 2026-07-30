# Changelog

Notable changes to the **Vandal Hearts PC port**. This tracks the port layer (Stage 3 gameplay/QoL and
packaging); the underlying decompilation stays byte-for-byte faithful to the retail game, and the normal
mode is unaffected by any of it. Format loosely follows [Keep a Changelog](https://keepachangelog.com/).

## [1.3.1] — Tactical Mode tuning + fixes

The full campaign has now been playtested end to end; this release folds in the resulting balance tuning
and fixes, plus two rendering/stability fixes that apply to both modes. Normal mode remains byte-for-byte
the original.

### Fixed (both modes)
- **Spell & cutscene effects** — certain textured effects (energy rays, dissolve/fade planes, warp
  surfaces) were rendering as flat semi-transparent shapes with their texture dropped; they now show
  their proper textured appearance.
- Fixed a rare crash when the on-screen attack marker (miss / support / poison) appeared over certain
  map tiles — most likely on the long, narrow Trials corridor.

### Quality of life (both modes)
- **Enemy threat overlay** — reachable-and-threatened tiles now show in a distinct **orange** (they
  previously shared yellow with spell/attack targeting), so a spell's AoE preview stays readable even
  inside a danger zone. Overlay tints were also softened for easier reading over any terrain.

### Tactical Mode (opt-in)

**Progression**
- **Level-cap curve retuned** to **10 / 15 / 19 / 24 / 28 / 32** — a smoother, steady climb that keeps
  the party a step under the cap heading into each chapter's Trials and lands the endgame party right on
  the final boss's level. (Validated across a full playthrough.)
- **Trials of Toroah** — fixed the final Trial not awarding attack experience.

**Casters & spells**
- **Monk / Ninja rework** — the path now learns a genuine **Spread Force** and a retuned **Thunder
  Flash**, and *keeps* its inherited base spells through both promotions (they're no longer shed at
  Ninja), so the class stays flexible rather than losing utility.
- **Mystic Energy** reworked into an **area-of-effect party buff** — a defensive group screen that
  raises defense and grants **magic resistance** to every ally in the field, with a new icy-blue aura and
  a per-ally cast effect. Cost tuned so a second back-to-back cast needs an MP refill.
- **Thunder Ball** added to the mage kit — a long-range, small-area attack spell — and **Roman Fire**
  buffed (power 7 → 9) so it's a real pick alongside Phase Shift rather than strictly outclassed.
- **Perfect Guard** — cheaper (**12 MP**) and now also grants **magic resistance**, making it a
  single-unit "evasion + anti-magic" shield.
- **Healing Circle / Healing Wave** retuned into distinct roles — Circle is a cheap, efficient group
  top-up; Wave is the heavy group heal.
- Fixed **Monk / Ninja maximum MP** overshooting its intended value.

**Visuals**
- **Avalanche** re-skinned as an *ice* avalanche in Tactical (an icy boulder), matching the spell's
  intended theme.

**Changed since 1.3.0**
- The two "restored cut weapons" from 1.3.0 have been **removed** — they turned out to be unique enemy
  boss weapons, so they stay boss-exclusive. (Restored item description text is unchanged.)

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
