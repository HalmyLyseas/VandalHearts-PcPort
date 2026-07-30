# Roadmap

Vandal Hearts — PC Port is delivered in stages. The two foundational stages are complete; **Stage 3**
is an optional layer of gameplay and quality-of-life enhancements built on top of the faithful port.

> These are **plans and intentions, not commitments or dates.** This is a non-commercial hobby
> preservation project — priorities may shift, and items may change or be dropped. The faithful
> beginning-to-end experience is always preserved; Stage 3 additions are optional and, where they
> change gameplay, opt-in.

## Done

- **Stage 1 — Matching decompilation.** The C source rebuilds the original executable byte-for-byte.
- **Stage 2 — Native PC port.** The full game runs end-to-end on Windows and Linux from your own disc,
  with faithful audio and video, packaged as one-file downloads.

### 1.0 — Faithful port + first additions ✅
- Complete beginning-to-end experience on Windows and Linux
- Skip intro / movies with START
- Camera rotation and elevation mapped to the right analog stick

### 1.1 — Controls ✅ (released 2026-07-25)
- Cycle through your units with the shoulder buttons (both directions), freeing the Square button
- **Enemy threat overlay** — press Square to see the combined movement-and-attack reach of *all*
  enemy units at once, so you can plan positioning without inspecting them one at a time
- An in-game options overlay (**Select + Start**), with per-axis right-stick invert as its first
  settings (the vertical axis ships inverted by default — the modern twin-stick convention)
- Saves and config now live next to the executable / AppImage, independent of where you launch from
- A documented control scheme and a dedicated page for the PC-port additions

### 1.2 — User experience ✅
- **Save management** in the overlay: unlimited whole-card backups (working around the three-slot
  limit), restore any backup over the current card (with a "back up first" safe default), delete, and
  a `(*)` marker for the backup matching your current card — each backup stays a real-hardware-valid
  save. Press **Start** on a backup to inspect its three slots' chapter / level / playtime.
- **Window scale** (X1–X8) and **fullscreen** from the overlay, applied live and saved to the config.

### 1.3 — Tactical Mode + fixes ✅ (released)
For **all players**: fixes two crashes latent since 1.2 (item transfer, certain cutscenes) and polishes
the options overlay and the enemy-threat overlay. It also adds a **Return to Title** overlay
convenience. And it introduces **Tactical Mode** — a large, **opt-in** rebalance for a more varied
tactical experience (per-chapter level cap, Trials that scale to your chapter and reward gold + XP,
class reworks, a reined-in Vandalier, and clarified item descriptions). Normal mode stays
byte-for-byte the original, and Tactical saves are kept separate. Full player guide:
**[tactical-mode.md](tactical-mode.md)**; per-release detail in the [changelog](../CHANGELOG.md).

> **Tactical Mode is opt-in and validated** across a full playthrough. It stays a living design, so a few
> balance numbers may still be fine-tuned in later point releases. This only affects the opt-in mode —
> the normal experience is unaffected. The **1.3.1** point release folded in the full-playthrough balance
> tuning and endgame fixes.

## Next — 1.4 (planned)

The 1.4 line continues the "things only a source-level port can do" theme — changes to game logic and the
port layer, with no new artwork required:

- **Faster battles** — a battle-only fast-forward (2× / 3×, on the shoulder triggers) for quicker play,
  with outcomes **identical** to normal speed.
- **Controller-aware prompts** — the port's own overlay button hints adapt to your controller
  (PlayStation or Xbox labels), auto-detected with a manual override.
- **Smarter Tactical AI** — enemy spellcasters weigh magic resistance when choosing targets, so
  positioning and defensive buffs (like Mystic Energy / Perfect Guard) matter on defense too. **Tactical
  Mode only.**
- **Unused-content investigation** — a few maps are referenced in the game data but never reachable in
  normal play; we'll look into what they are and restore any that turn out to be complete, coherent
  battles.

## Beyond — 1.5 and later

A dedicated **higher-quality graphics** track, kept separate because it's asset-heavy and best done in
stages: upscaled full-motion videos, then higher internal rendering resolution, and eventually
higher-detail textures and sprites — likely offered as an optional "Improved" graphics mode. (Bundled
"collector" extras such as artwork or manuals are unlikely — that material is copyrighted.)
