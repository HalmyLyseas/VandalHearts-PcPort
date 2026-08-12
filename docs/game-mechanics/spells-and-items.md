# Spells & consumables

The full spell/consumable table, decoded from `gSpells` / `gSpellsEx` / `gSpellLevelRequirement` /
`gSpellLists`. Columns are the game's own mechanical data — this doc carries no in-game description
text (see [README](README.md)); the *Notes* are our own mechanical summaries.

## Reading the table

- **rng** — cast range from the caster. **`0` = self-centred** (the blast is centred on the caster, so
  they must physically stand where they want it).
- **field** — blast radius. **`255` = whole map.**
- **pow** — feeds the damage/heal formula.
- **mp** — cost. **`0` = item-triggered** (cast as a spell but consumes no MP — see the item band).
- **reqLv** — learn level, from `gSpellLevelRequirement[36]` (player spells only).
- **learned by** — from `gSpellLists`. `A` = specialist path, `B` = Monk/Ninja path. Only the five
  casters have lists: Ash, Eleni, Huxley, Sara, Zohar.

**ID bands:** **1–32** player spells · **33–48** enemy-caster spells · **49–59** item effects (mp 0) ·
**60–71** monster spells · **72–79** unnamed/unused.

> ⚠️ `gSpells` has 80 slots but `gSpellsEx` only 73 — effect data for id ≥ 73 is a latent out-of-bounds
> read. Harmless today (nothing is assigned an id that high), but real if one ever were. It is also why
> giving a player unit a spell id ≥ 36 needs a guard against the `gSpellLevelRequirement[36]` bound.

## Player spells (1–32)

| id | name | area | target | rng | field | pow | mp | effect | reqLv | learned by |
|---:|---|---|---|---:|---:|---:|---:|---|---:|---|
| 1 | FAERIE_LIGHT | single | ally | 0 | 0 | 4 | 2 | HEAL | 1 | Ash A/B |
| 2 | ICE_STORM | single | enemy | 1 | 0 | 2 | 4 | DAMAGE | 8 | Ash A/B |
| 3 | ROLLING_FIRE | AOE | enemy-grp | 2 | 1 | 8 | 8 | DAMAGE | 14 | Ash A/B |
| 4 | FAERIE_STAR | single | ally | 2 | 0 | 28 | 5 | HEAL | 18 | Ash A/B |
| 5 | DELTA_MIRAGE | single | enemy | 1 | 0 | 18 | 10 | DAMAGE | 23 | Ash A/B |
| 6 | DARK_STAR | single | enemy | 4 | 0 | 1 | 2 | DAMAGE | 1 | Eleni/Zohar A/B |
| 7 | SPELLBIND | single | enemy | 5 | 0 | 1 | 2 | PARALYZE | 8 | Eleni/Zohar A/B |
| 8 | PIERCING_RAY | AOE | enemy-grp | 4 | 1 | 2 | 4 | DAMAGE | 10 | Eleni/Zohar A/B |
| 9 | ENVENOM | single | enemy | 5 | 0 | 1 | 3 | POISON | 12 | Eleni/Zohar A |
| 10 | PHASE_SHIFT | AOE | enemy-grp | 0 | **7** | 9 | 12 | DAMAGE | 14 | Eleni/Zohar A |
| 11 | ROMAN_FIRE | AOE | enemy-grp | 5 | 2 | 7 | 6 | DAMAGE | 16 | Eleni/Zohar A |
| 12 | POISON_CLOUD | AOE | enemy-grp | 5 | 2 | 1 | 4 | POISON | 19 | Eleni/Zohar A |
| 13 | SPREAD_FORCE | AOE | enemy-grp | 0 | 3 | 13 | 7 | DAMAGE | 21 | Eleni/Zohar A |
| 14 | AVALANCHE | single | enemy | 6 | 0 | 18 | 9 | DAMAGE | 23 | Eleni/Zohar A |
| 15 | SALAMANDER | AOE | enemy-grp | 0 | **10** | 13 | 14 | DAMAGE | 25 | Eleni/Zohar A |
| 16 | HEALING | single | ally | 4 | 0 | 2 | 3 | HEAL | 1 | Huxley/Sara A/B |
| 17 | MYSTIC_SHIELD | single | ally | 4 | 0 | 1 | 3 | DEF+ | 8 | Huxley/Sara A/B |
| 18 | CURE | single | ally | 4 | 0 | 1 | 2 | CURE | 10 | Huxley/Sara A/B |
| 19 | HEALING_PLUS | AOE | ally-grp | 4 | 1 | 9 | 4 | HEAL | 12 | Huxley/Sara A |
| 20 | BLESS_WEAPON | single | ally | 4 | 0 | 1 | 3 | ATK+ | 13 | Huxley/Sara A |
| 21 | HOLY_LIGHTNING | single | enemy | 4 | 0 | 7 | 7 | DAMAGE | 16 | Huxley/Sara A |
| 22 | ULTRA_HEALING | AOE | ally-grp | 5 | 2 | 28 | 5 | HEAL | 18 | Huxley/Sara A |
| 23 | MAGIC_CHARGE | single | ally | 4 | 0 | 1 | 10 | MP transfer | 20 | Huxley/Sara A |
| 24 | HOLY_PRESSURE | AOE | enemy-grp | 5 | 1 | 13 | 7 | DAMAGE | 22 | Huxley/Sara A |
| 25 | SUPREME_HEALING | AOE | ally-grp | **255** | **255** | 28 | 30 | HEAL | 25 | Huxley/Sara A |
| 26 | STONE_SHOWER | AOE | enemy-grp | 0 | 1 | 4 | 10 | DAMAGE | 12 | Eleni/Huxley/Sara/Zohar **B** |
| 27 | CURE_WIDE | AOE | ally-grp | 0 | 1 | 1 | 4 | CURE | 15 | …B |
| 28 | HEALING_CIRCLE | AOE | ally-grp | 0 | 1 | 15 | 6 | HEAL | 17 | …B |
| 29 | PERFECT_GUARD | single | ally | 4 | 0 | 1 | 15 | AGL+ (1-shot negate) | 19 | …B |
| 30 | THUNDER_FLASH | AOE | enemy-grp | 0 | 2 | 14 | 12 | DAMAGE | 21 | …B |
| 31 | HEALING_WAVE | AOE | ally-grp | 0 | 2 | 28 | 10 | HEAL | 23 | …B |
| 32 | MYSTIC_ENERGY | single | ally | 4 | 0 | 1 | 15 | ATK/DEF+ | 25 | …B |

**The `B` (Monk/Ninja) list, 26–32,** is nearly all `rng 0` (self-centred) with small `field` (1–2) — a
melee-delivery kit, and largely dominated by the `A`-path alternatives (e.g. HEALING_WAVE pow 28/field
2/**rng 0**/mp 10 vs ULTRA_HEALING pow 28/field 2/**rng 5**/mp 5): same power and field, but no range at
double the cost. This is the mechanical basis for the Monk/Ninja being the weakest caster line.

## Enemy-caster spells (33–48)

| id | name | area | target | rng | field | pow | mp | effect |
|---:|---|---|---|---:|---:|---:|---:|---|
| 33 | SELF_HEALING | single | ally | 0 | 0 | 13 | 2 | HEAL |
| 34 | HEALING_2 | single | ally | 4 | 0 | 17 | 3 | HEAL |
| 35 | EXTRA_HEALING | AOE | ally-grp | 5 | 1 | 23 | 5 | HEAL |
| 36 | HYPER_HEALING | AOE | ally-grp | 5 | 2 | 28 | 8 | HEAL |
| 37 | HEALING_CIRCLE_2 | AOE | ally-grp | 0 | 1 | 28 | 5 | HEAL |
| 38 | PIERCING_LIGHT | single | enemy | 4 | 0 | 3 | 3 | DAMAGE |
| 39 | RAINBOW_STROKE | single | enemy | 6 | 0 | 9 | 5 | DAMAGE |
| 40 | MAGIC_ARROW | AOE | enemy-grp | 5 | 1 | 17 | 5 | DAMAGE |
| 41 | THUNDER_BALL | AOE | enemy-grp | 5 | 2 | 12 | 5 | DAMAGE |
| 42 | DARK_HURRICANE | AOE | enemy-grp | 6 | 1 | 20 | 8 | DAMAGE |
| 43 | RAINBOW_STORM | AOE | enemy-grp | 4 | 2 | 5 | 7 | DAMAGE |
| 44 | DAGGER_STORM | AOE | enemy-grp | 0 | 1 | 16 | 5 | DAMAGE |
| 45 | **PLASMA_WAVE** | AOE | enemy-grp | 0 | **255** | 22 | 15 | DAMAGE |
| 46 | DARK_FIRE | AOE | enemy-grp | 5 | 2 | 20 | 15 | DAMAGE |
| 47 | EXPLOSION | single | enemy | 6 | 0 | 22 | 15 | DAMAGE |
| 48 | DYNAMO_HUM | AOE | enemy-grp | 0 | 2 | 23 | 20 | DAMAGE |

PLASMA_WAVE (45) is the whole-map (`field 255`) hit already validated in the port — the signature
Vandalier spell.

## Item effects (49–59) — cast as spells, **0 MP, never consumed**

| id | name | area | target | rng | field | pow | effect |
|---:|---|---|---|---:|---:|---:|---|
| 49 | HERB | single | ally | 1 | 0 | 5 | HEAL |
| 50 | MEGAHERB | single | ally | 1 | 0 | 15 | HEAL |
| 51 | ELIXIR | single | ally | 1 | 0 | 1 | CURE |
| 52 | MAGE_OIL | single | ally | 1 | 0 | 1 | MP+ |
| 53 | MAGE_GEM | single | ally | 1 | 0 | 1 | MP+ |
| 54 | LIFE_ORB | single | ally | 1 | 0 | **99** | HEAL |
| 55 | HOLY_H2O | AOE | ally-grp | **255** | **255** | **99** | HEAL |
| 56 | FIRE_GEM | AOE | enemy-grp | 3 | 1 | 1 | DAMAGE |
| 57 | MOODRING | AOE | enemy-grp | 0 | 2 | 1 | DAMAGE |
| 58 | AURA_GEM | AOE | enemy-grp | 5 | 3 | 1 | DAMAGE |
| 59 | WYRMFANG | AOE | enemy-grp | 0 | 8 | 19 | DAMAGE |

These `mp 0` item-effect entries are why **ELIXIR** makes the Monk/Ninja ailment immunity worth little
(anyone can carry one), and — cast through the Vandalier debug menu — are the source of the "infinite
items / free HP & MP" behaviour (LIFE_ORB pow 99, HOLY_H2O whole-map heal).

## Monster spells (60–71)

| id | name | area | target | rng | field | pow | effect |
|---:|---|---|---|---:|---:|---:|---|
| 60 | ROLLING_THUNDER | single | enemy | 8 | 0 | 15 | DAMAGE |
| 61 | HARMFUL_WAVE | single | enemy | 8 | 0 | 16 | DAMAGE |
| 62 | EVIL_STREAM | single | enemy | 9 | 0 | 20 | DAMAGE |
| 63 | MAD_BOOK | single | enemy | 5 | 0 | 1 | PARALYZE |
| 64 | MUSHROOM | AOE | enemy-grp | 5 | 2 | 1 | POISON |
| 65 | MOON_PIE | single | ally | 0 | 0 | 5 | HEAL |
| 66 | IRONBOOT | single | ally | 4 | 0 | 1 | AGL+ |
| 67 | UNICORN | AOE | enemy-grp | 4 | 2 | 5 | DAMAGE |
| 68 | KINGFOIL | AOE | ally-grp | 0 | 1 | 28 | HEAL |
| 69 | HELSTONE | AOE | enemy-grp | 5 | 2 | 14 | DAMAGE |
| 70 | SHIVBOOK | AOE | enemy-grp | 0 | 1 | 16 | DAMAGE |
| 71 | NCKLACE | AOE | enemy-grp | 6 | 1 | 20 | DAMAGE |

Enemy spells reach **further** than player ones: EVIL_STREAM (62) `rng 9`, ROLLING_THUNDER/HARMFUL_WAVE
`rng 8`, vs the player maximum of `rng 6` (AVALANCHE).

## Ranges of note

- **Whole-map (`rng 255 / field 255`):** SUPREME_HEALING (25), HOLY_H2O (55). PLASMA_WAVE (45) is
  `field 255` from a `rng 0` caster.
- **Self-centred (`rng 0`)** is not unique to Monk/Ninja — PHASE_SHIFT, SPREAD_FORCE and SALAMANDER are
  too. The difference is **field size**: 7 / 3 / 10 for the Sorcerer versus 1–2 for Monk/Ninja. Standing
  in the enemy formation is only worth it if the blast is big.

## How a unit gets its spells

`unit.spells[i] = gSpellLists[partyIdx][path][i]`, then gated by level:
`gSpellLevelRequirement[spellId]` (`PopulateUnitSpellList`, `src/game_setup.c:243-266`). Lists are
**per party member × promotion path**, not per class — `path A` = specialist (Sorcerer/Archbishop),
`path B` = Monk/Ninja. A promotion into the B path replaces the origin's endgame spells rather than
adding to them (the B list from L12 on is identical for all four casters).
