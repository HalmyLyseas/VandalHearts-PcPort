# Game mechanics — decoded reference

A source-of-truth reference for how *Vandal Hearts* (US, `SLUS_004.47`) actually works under the hood,
decoded from the byte-exact decompilation. It exists because the game is built on a **"display vs
real" split**: many of the numbers the game shows the player are cosmetic, and the values that
actually drive combat are never surfaced. This folder documents the *real* model.

> **Every value here is the original game's own data**, read from the byte-exact build — not
> estimates. Code references (`src/…:line`) point at the decompiled source in this repo. This is
> reverse-engineering / interoperability documentation; it contains mechanical data and our own
> analysis, **not** the game's in-game description text (that stays out of the repo — see
> [`NOTICE`](../../NOTICE)).

## The one thing to internalise: display ≠ real

Three whole systems show the player an inflated or entirely cosmetic number:

| The game shows you… | …but combat actually uses | So the displayed number is |
|---|---|---|
| class **Attack / Defense / Agility** | a per-unit random roll that cancels out | **cosmetic** |
| class **Max HP** (e.g. 800 vs 500) | a normalised `hpFrac` pool identical for every unit | **cosmetic** (display only) |
| **equipment** power (e.g. Axe 39 vs Staff 14) | a separate, compressed real table (both = 13) | **cosmetic** |

⇒ Balancing or judging a class from the status screen is misleading. See each doc for the real levers.

## Contents

- **[combat-mechanics.md](combat-mechanics.md)** — the damage/resist model, evasion, the magic and
  ailment chains, and exactly which inputs matter (and which are fluff). **Read this first.**
- **[classes.md](classes.md)** — what mathematically defines a class: the `gUnitInfo` movement/range/
  susceptibility profile, the archetype tables, and the resistance metric.
- **[weapons-and-armor.md](weapons-and-armor.md)** — the display-vs-real power tables, and why
  equipment is a *progression* axis, not a class one.
- **[spells-and-items.md](spells-and-items.md)** — the full spell + consumable table (range, field,
  power, MP, effect, who learns it) with mechanical effect summaries.
