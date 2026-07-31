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

### 1.4 — Quality of life ✅ (released 2026-07-31)

1.4 is a quality-of-life pass. Most of it — the fast-forward, the controller-matched prompts, the camera
tweak — is convenience an emulator or launcher could offer too; the one change that genuinely needs the
game's source is the **smarter Tactical AI**, which reaches into the enemy's own targeting logic. No new
artwork required:

- **Faster battles** — a battle-only **2×** fast-forward on the triggers for quicker play, with outcomes
  **identical** to normal speed.
- **Controller-aware prompts** — the port's own overlay button hints show **Xbox** letters or
  **PlayStation** symbols, switchable in the overlay (defaults to Xbox).
- **Smarter Tactical AI** — enemy spellcasters weigh magic resistance when choosing targets, so
  positioning and defensive buffs (like Mystic Energy / Perfect Guard) matter on defense too. **Tactical
  Mode only.**
- **Finer camera elevation** — an extra, evenly-spaced up/down camera angle (including a level 45°) for
  reading maps with stepped terrain.

## Next — 1.5 and later

A **graphics fidelity** track, kept deliberately conservative — the goal is to *sharpen without
reinterpreting* the game. Two pieces:

- **A more hardware-accurate software renderer** — matching the PS1 GPU's exact texture sampling and
  coverage so translucent spell/casting effects render like the original (the remaining casting-ray
  difference in [known issues](known_issues.md) is the motivating case).
- **Higher internal rendering resolution** — drawing the 3D at a denser sample rate for crisper terrain
  and edges, most likely as an optional "Enhanced" mode alongside the pixel-accurate one. **No assets are
  re-authored:** the game's own sprites, textures, videos and UI are untouched.

Deliberately **out of scope:** upscaled/re-encoded videos, redrawn or AI-upscaled sprites, and camera
changes like finer rotation — they'd re-author the art or alter the game's feel, which this project avoids.
(Bundled "collector" extras such as artwork or manuals are also out — that material is copyrighted.)
