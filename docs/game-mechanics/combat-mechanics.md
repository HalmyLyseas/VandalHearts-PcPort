# Combat mechanics

How damage, defence, evasion, magic and status are actually computed — decoded from
`src/battle_0190dc.c` (`CalculateDamage`) and `src/battle_013b94.c`. The recurring theme: the values
the player sees are often not the values the game uses.

## Physical damage

Physical combat runs through a single intermediate quantity, **`resist`**, then a subtraction:

```
resist  =  gAdvantage[attacker.advantage][defender.advantage]      // type matchup — the base term
         − gItemEquipmentPower[attacker.weapon]                    // attacker's weapon (player)
         + gItemEquipmentPower[defender.helmet]                    // defender's helm  (player)
         + gItemEquipmentPower[defender.armor]                     // defender's armor (player)
         − CheckForSupporterBonus(...) ×4                          // adjacent allies flanking
         − (highGround elevation delta)                            // ×3, or ×1 for an Archer attacker
         − dirBonus × 2                                            // side/rear attack
         ∓ gLevelDisparityBonus[levelDiff]                        // ~3 per level of gap  ← dominant
resist  =  max(resist, 1)

damage  =  attacker.atkVar10000 + (100000 / resist)               // "power"
         − defender.defVar10000                                    // "defense"
         − gTerrainBonus[defender terrain]
if (damage ≤ 0) damage = 100                                       // 1% chip floor
```

### Why the class attack/defense stats are cosmetic
`atkVar10000` and `defVar10000` look like the unit's attack and defense — but they are **pure per-unit
random rolls**: `(rand() % 40 + 80) × 15 + 8500` ⇒ ~9500–10475, with **no class, level or equipment
input** (`src/battle_0190dc.c:891-910`). Attacker's `atkVar` and defender's `defVar` roll from the same
distribution, so they **largely cancel**, leaving:

> **damage ≈ `100000 / resist`.**

So physical combat is driven almost entirely by `resist`. The class `Attack`/`Defense` numbers on the
status screen (`gClassBaseAttack`/`gClassBaseDefense`) never enter the formula — they are read only to
draw the status screen (`src/window.c`).

### The dominant terms of `resist`
1. **Level gap** (`gLevelDisparityBonus`, ~3 per level). A ~10-level lead pushes `resist` toward its
   floor of 1, where `100000/resist` dwarfs the random spread and everything one-shots. **This is why
   an over-levelled party trivialises the game** — and why the level cap is the keystone of any
   rebalance.
2. **`gAdvantage`** — the type matchup (see below). The single biggest *class-differentiating* term.
3. **Equipment** — a top weapon takes `resist` from ~20 to ~7 (nearly 3× damage), but every class
   converges to the same real ceiling, so it does not differentiate classes. See
   [weapons-and-armor.md](weapons-and-armor.md).
4. **Flanking / facing / high ground / terrain** — situational modifiers.

### `gAdvantage` — the physical type-matchup table
`resist` starts from `gAdvantage[attacker.advantage][defender.advantage]` — a 49×49 table
(`src/battle_0190dc.c:467`). Player classes occupy advantage indices 28–48; enemies 0–27. It is the
**physical** matchup only (magic uses a fixed base, below), and it is also the AI's target-preference
term (`src/ai.c:510`). A class's `advantage` row/column is the main thing that makes it tanky or fragile
against a given opponent.

## Hit points: a normalised pool

The number over a unit's head is not what combat tracks. Combat subtracts damage from **`hpFrac`**, a
**normalised 0–10000 pool that is identical for every unit** (`defender.hpFrac -= damage`). The
displayed HP is only that fraction *rendered* against the class's `MaxHp`:

```
displayed_hp = (MaxHp·2 + MaxHp·(level−2) + MaxHp·hpVar100/100) · hpFrac / 10000 / 100
```

⇒ **`MaxHp` is cosmetic for durability.** A unit with MaxHp 800 and one with MaxHp 500, both at full
`hpFrac`, die to the **same number of proportional hits** — because damage is a flat `hpFrac` amount,
independent of MaxHp. MaxHp only changes the number shown. (This corrects the intuitive reading that the
Armored line is tough "because of its HP bar" — its real durability is its `gAdvantage` resistance.)

## Evasion (called "block")

Before damage lands, a dodge roll (`src/battle_0190dc.c:556-570`):

```c
rnd         = rand() & 0x7fff                              // 0..32767
blockChance = (defLvl > atkLvl) ? (defLvl − atkLvl)·32 : 0 // level lead adds evasion
blockChance += gClassBlockChance[defender.class]           // per-archetype base
blockChance −= (blockChance / 2) · dirBonus                // side/rear attack halves/quarters it
if (blockChance > rnd) damage = 0                          // full miss
```

This is **evasion (a dodge), not shielding** — a full negation, not a reduction. There is **no separate
accuracy stat**; "chance to hit" is simply `1 − blockChance`. Per-archetype values track what an
agility→evasion system *would* have produced (mages/priests/monks highest at 12.5%, Armored lowest at
5%) — see the "cut agility system" note in [classes.md](classes.md).

`PERFECT_GUARD` / `IRONBOOT` set `aglBoosted`, a one-shot flag that makes the **next** incoming attack
deal 0 damage and then clears (`src/battle_0190dc.c:572-575`) — a guaranteed negation, unrelated to the
agility stat.

## Magic — a separate chain

Magic does **not** use `gAdvantage`. It starts from a fixed base and applies the target's
`magicSusceptibility` (`src/battle_0190dc.c:750`):

```
resist   = 10 − (spell.power − target.level)·2
resist ×= magicSusceptibility factor
```

| `magSusc` | factor on resist | meaning | who |
|---:|---|---|---|
| 1 | ×1.5 (most resistant) | eats magic best | Mage/Sorcerer/Enchanter, Healer/Bishop/Archbishop |
| 2 | ×1.25 | resistant | Ash's line |
| 3 | ×1.0 | neutral | most classes |
| 4 | ×0.75 (**vulnerable**) | takes ~33% more | **Guardsman/Dragoon only** |
| 5 | ×0.5 (most vulnerable) | — | no player class |

Because physical resistance (`gAdvantage`) and magic vulnerability (`magSusc`) live in **different
tables**, they are **independent axes** — which is how the Dragoon can be the most physically protected
class in the game *and* the most magic-vulnerable at the same time.

## Status ailments

`ailmentSusceptibility` gates whether a status effect (poison/paralyze/…) lands
(`src/battle_0190dc.c:661`): **1 = fully immune** (chance forced to 0), 2 = half chance, 3 = normal. The
Monk/Ninja and Healer lines are the only ailment-immune classes.

## Situational modifiers (all fold into `resist`)

- **High ground:** attacking from higher elevation lowers `resist` (more damage) by `elevationDelta·3`
  — or `·1` for an Archer attacker (archers care less about height).
- **Facing:** a **side** attack adds `dirBonus 1`, a **rear** attack `dirBonus 2`; each point lowers
  `resist` by 2 **and** cuts the defender's evasion.
- **Terrain:** `gTerrainBonus[terrain]` is subtracted from damage (defensive terrain reduces hits).
- **Supporter bonus:** adjacent allies of the attacker flank the defender, lowering `resist`.

## Experience & levelling

Level is **derived from `experience`** (a `BigInt`) via `gExperienceLevels`
(`src/battle_0190dc.c:913`) — not stored directly, which is why searching for a `.level` write misses
the scaling logic. EXP is granted by `BigIntAdd(unit.experience, gState.experience)` at two sites, each
already guarded by a hard cap `if (unit.level < 50)` (`src/battle_013b94.c:648` combat, `:1429`
spell-cast). Support-spell casts grant `(exp-to-next-level)/3` each (`CalculateSupportSpellExp`), which
is the basis of the infinite-EXP exploit.

## Summary — what is real vs cosmetic

| Real (drives combat) | Cosmetic (display only / cancels) |
|---|---|
| `gAdvantage` (physical type matchup) | `gClassBaseAttack`, `gClassBaseDefense`, `gClassBaseAgility` |
| `gItemEquipmentPower` (the *real* table) | `gClassBaseMaxHp` (scales displayed HP only) |
| level **gap** (`gLevelDisparityBonus`) | `gItemEquipmentDisplayPower` (the shown table) |
| `magicSusceptibility`, `ailmentSusceptibility` | `unit.agility` (never read in combat) |
| `gClassBlockChance` (evasion) | `atkVar10000` / `defVar10000` largely cancel |
| `step` (movement profile), `attackRange` | — |
| facing, high ground, terrain, flanking | — |
