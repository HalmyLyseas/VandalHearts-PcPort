# Classes

What mathematically defines a class in *Vandal Hearts*. After the [display-vs-real](combat-mechanics.md)
findings, most of the status screen is cosmetic; a class's real identity is a small set of values in
`gUnitInfo` plus its `gAdvantage` row.

## Two levels: named class vs archetype

Easy to confuse, and it matters:

- **`unitType`** (`include/units.h`) — the **named class** you see (Swordman, Duelist, Guardsman…),
  22 player-facing entries. Its per-class data lives in **`gUnitInfo[unitId]`** (`UnitInfo`).
- **`class`** (`CLASS_*`) — a coarse **archetype**, only 9 values, which is what the **stat tables are
  indexed by**. A whole promotion line shares one archetype: Soldier/Swordman/Duelist are all
  `CLASS_KNIGHT`; Mage/Sorcerer/Enchanter all `CLASS_MAGE`.

⇒ **Promotions do NOT change base stat growth.** Duelist and Soldier pull identical `gClassBase*`
entries. What a promotion actually changes is the `gUnitInfo` row: **movement (`step`), `attackRange`,
`magicSusceptibility`, `ailmentSusceptibility`, `advantage`, and the spell list.** That is the real
balance surface.

## The real per-class profile (`gUnitInfo`)

`MOVE = gTravelRange[step]`, with `gTravelRange = {1,5,6,7,5,5,6,7,8,4,6,6,7,6}`.

| Class | archetype | step | MOVE | atkRange | magSusc | ailSusc | advantage |
|---|---|---:|---:|---:|---:|---:|---:|
| Hero | KNIGHT | 1 | 5 | 1 | 2 | 2 | 28 |
| Champion | KNIGHT | 2 | 6 | 1 | 2 | 2 | 33 |
| Paragon | KNIGHT | 3 | 7 | 1 | 2 | 2 | 41 |
| Vandalier / Vanguard | VANDALIER | 3 | 7 | 1 | 2 | 2 | 41 |
| Soldier | KNIGHT | 1 | 5 | 1 | 3 | 3 | 29 |
| Swordman | KNIGHT | 2 | 6 | 1 | 3 | 3 | 34 |
| Duelist | KNIGHT | 3 | 7 | 1 | 3 | 3 | 42 |
| **Guardsman** | ARMOR | 1 | **5** | 1 | **4** | 3 | 38 |
| **Dragoon** | ARMOR | 4 | **5** | 1 | **4** | 3 | 46 |
| Archer | ARCHER | 1 | 5 | 4 | 3 | 3 | 30 |
| Bowman | ARCHER | 5 | 5 | 6 | 3 | 3 | 35 |
| Sniper | ARCHER | 6 | 6 | 8 | 3 | 3 | 43 |
| Hawknight | AIRMAN | 7 | 7 | 1 | 3 | 3 | 39 |
| Sky Lord | AIRMAN | 8 | 8 | 1 | 3 | 3 | 47 |
| Mage | MAGE | 9 | 4 | 1 | 1 | 2 | 31 |
| Sorcerer | MAGE | 4 | 5 | 1 | 1 | 2 | 36 |
| Enchanter | MAGE | 10 | 6 | 1 | 1 | 2 | 44 |
| Healer | PRIEST | 9 | 4 | 1 | 1 | **1** | 32 |
| Bishop | PRIEST | 4 | 5 | 1 | 1 | **1** | 37 |
| Archbishop | PRIEST | 10 | 6 | 1 | 1 | **1** | 45 |
| **Monk** | MONK | 11 | 6 | 1 | 3 | **1** | 40 |
| **Ninja** | MONK | 12 | 7 | 1 | 3 | **1** | 48 |

## Archetype stat tables — mostly cosmetic

Indexed by `CLASS_*` (`src/battle_0190dc.c:175-303`). **Attack, Defense, MaxHp and Agility are all
display-only** — none appears in any damage formula (see [combat-mechanics.md](combat-mechanics.md)).
They are shown here for completeness, struck through where cosmetic; **do not balance against them.**

| archetype | ~~Attack~~ | ~~Defense~~ | ~~MaxHp~~ | ~~Agility~~ | **BlockChance (evasion)** |
|---|---:|---:|---:|---:|---:|
| KNIGHT | ~~1150~~ | ~~1000~~ | ~~700~~ | ~~1000~~ | 10.0% |
| ARMOR | ~~1300~~ | ~~1400~~ | ~~800~~ | ~~750~~ | **5.0%** (lowest) |
| MONK | ~~900~~ | ~~850~~ | ~~650~~ | ~~1150~~ | 12.5% |
| ARCHER | ~~1000~~ | ~~800~~ | ~~630~~ | ~~950~~ | 10.0% |
| AIRMAN | ~~1400~~ | ~~1100~~ | ~~750~~ | ~~1050~~ | 10.0% |
| MAGE | ~~650~~ | ~~550~~ | ~~550~~ | ~~1200~~ | 12.5% |
| PRIEST | ~~750~~ | ~~600~~ | ~~500~~ | ~~1225~~ | 12.5% |
| VANDALIER | ~~1800~~ | ~~1750~~ | ~~900~~ | ~~2500~~ | ~100% |

Stat growth is linear in level: `stat = (base + (base/3)·(level−2) + (base/3)·var100/100) / 100`
(`var100 = 80..119`, rolled per unit) — but since the stats it grows are cosmetic, only the
`gClassBlockChance` and MP columns carry into play.

`gClassMpMultiplier`: MP = `mult × level` (capped 99). **MAGE = 2, PRIEST = 2, MONK = 1** — the
Monk/Ninja hybrid caster runs at *half* a caster's MP rate.

## What actually matters (the short list)

**Primary differentiators**
1. **Physical resistance** — the class's `gAdvantage` row vs enemy attackers *(dominant term)*
2. **Magic vulnerability** — `magicSusceptibility` *(independent axis; NOT from `gAdvantage`)*
3. **Movement** — `step` (bundles MOVE + climb + terrain cost; see below)

**Secondary**
4. `ailmentSusceptibility` (1 = immune) · 5. `gClassBlockChance` (evasion) · 6. `attackRange`
   (also gates counter-immunity — a defender only counters if the attacker is within the *defender's*
   range) · 7. spell list · 8. `gClassMpMultiplier`

**Ignore** — `gClassBaseAttack/Defense/MaxHp/Agility` (display-only) and equipment *access* (every
class converges to the same real ceiling — see [weapons-and-armor.md](weapons-and-armor.md)).

## Resistance metric — the frontliner comparison

Physical resistance measured against **enemy** attacker rows (player-vs-player pairs are all neutral).
Higher = tankier. Magic uses the separate `magSusc` chain. Assumes level parity, frontal attack.

| class | phys resist (mean vs enemies) | magSusc | ailSusc | evasion | MOVE |
|---|---:|---:|---:|---:|---:|
| Duelist | 43.2 | 3 | 3 | 10% | 7 |
| **Dragoon** | **54.7** (most protected in game) | **4** (vulnerable) | 3 | 5% | 5 |
| Sky Lord | 45.0 | 3 | 3 | 10% | 8 |
| Ninja | 39.9 | 3 | **1 (immune)** | 12.5% | 7 |
| Archbishop | 36.6 | 1 | 1 | 12.5% | 6 |

Two structural facts: **each promotion tier adds ~+10 physical resistance** (promotion is a defensive
upgrade as much as offensive); and the **weakness structure runs through two systems** —
flyers↔ranged live in `gAdvantage`, but Armored↔magic is *not* in `gAdvantage` at all (Dragoon is the
most physically protected class in the game) — it is purely `magSusc 4`.

## Movement is a full profile, not just a range

`step` indexes **four** tables together (`include/field.h`), so it sets horizontal range *and* vertical
mobility *and* terrain cost at once:

| table | meaning |
|---|---|
| `gTravelRange[14]` | horizontal movement points |
| `gTravelAscentCost[14][20]` | cost to climb `diff` elevation in one step |
| `gTravelDescentCost[14][20]` | cost to drop `diff` elevation |
| `gTravelTerrainCost[14][11]` | cost per terrain type |

Used in `PopulateMovementGrid` (`src/path_grids.c`): `0` = free (entered without spending movement),
`255` = impassable (no unit has >8 points, so a 255 cost can never be paid). Climb ceiling by step:

| step | MOVE | max climb | classes |
|---:|---:|---:|---|
| 1 | 5 | **+1** | Archer, **Guardsman**, Hero, Soldier |
| 2 | 6 | +2 | Champion, Swordman |
| 3 | 7 | +2 (cheap) | Duelist, Paragon, Vandalier |
| 4 | 5 | +2 (costly) | Bishop, **Dragoon**, Sorcerer |
| 5 | 5 | +2 | Bowman |
| 6 | 6 | +2 | Sniper |
| 7 | 7 | +2 | Hawknight |
| 8 | 8 | +2 | Sky Lord |
| 9 | 4 | +1 | Healer, Mage |
| 10 | 6 | +2 | Archbishop, Enchanter |
| 11 | 6 | +2 | Monk |
| 12 | 7 | +2 | Ninja |

Note this makes the Armored line's mobility problem worse than the MOVE column alone suggests:
**Guardsman is on the base-tier `step 1`** (climb only +1, worse than Swordman's +2), and **Dragoon
gains no MOVE on promotion** (5→5) while paying more to climb than its Duelist sibling.

## Historical note: agility is a cut mechanic

An agility-driven combat system was designed and then abandoned. `agiVar10000` is computed exactly like
`atkVar10000`/`defVar10000` but is **read nowhere** (`src/battle_0190dc.c`); `unit.agility` is read once,
only to draw the status screen; and `gClassBlockChance` looks like the hand-authored static stand-in for
the evasion an agility system would have produced (its per-class values track `gClassBaseAgility`
closely). The Monk/Ninja line is statted as *the agile class* (highest `BaseAgility`) in a game where
agility does nothing — which is a large part of why that line feels incoherent.
