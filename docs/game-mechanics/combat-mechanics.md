# Combat mechanics

How damage, defence, evasion, magic and status are actually computed — decoded from
`src/battle/math.c` (`CalculateDamage`) and `src/battle/executors.c`. The recurring theme: the values
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
input** (`src/battle/math.c:891-910`). Attacker's `atkVar` and defender's `defVar` roll from the same
distribution, so they **largely cancel**, leaving:

> **damage ≈ `100000 / resist`.**

So physical combat is driven almost entirely by `resist`. The class `Attack`/`Defense` numbers on the
status screen (`gClassBaseAttack`/`gClassBaseDefense`) never enter the formula — they are read only to
draw the status screen (`src/ui/window.c`).

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
(`src/battle/math.c:467`). Player classes occupy advantage indices 28–48; enemies 0–27. It is the
**physical** matchup only (magic uses a fixed base, below), and it is also the AI's target-preference
term (`src/battle/ai.c:510`). A class's `advantage` row/column is the main thing that makes it tanky or fragile
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

Before damage lands, a dodge roll (`src/battle/math.c:556-570`):

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
deal 0 damage and then clears (`src/battle/math.c:572-575`) — a guaranteed negation, unrelated to the
agility stat.

## Magic — a separate chain

Magic does **not** use `gAdvantage`. It starts from a fixed base and applies the target's
`magicSusceptibility` (`src/battle/math.c:750`):

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

`CalculateSpellPowerAndExp` is the counterpart to `CalculateAttackDamage` for this chain — it computes
the outcome once a target is confirmed, branching on the spell's effect type. Before it runs,
`Objf027_TargetingSpell` validates the cast: it paints the cast range and tracks the cursor, checks the
chosen cell against `gSpells[].targeting` (enemy / ally / enemy group / ally group / free cell), and
repeats the check for area spells against the blast footprint. Confirming closes the spell window and
hands off to `OBJF_UNIT_CASTING` at the caster's tile; two of the five targeting types are bare returns
because no retail spell uses them.

## Status ailments

`ailmentSusceptibility` gates whether a status effect (poison/paralyze/…) lands
(`src/battle/math.c:661`): **1 = fully immune** (chance forced to 0), 2 = half chance, 3 = normal. The
Monk/Ninja and Healer lines are the only ailment-immune classes. The roll itself lives in
`TryInflictingAilment`, which has a suspected operator-precedence bug in retail's expression; it is kept
byte-exact rather than corrected (documented at the function).

## Situational modifiers (all fold into `resist`)

- **High ground:** attacking from higher elevation lowers `resist` (more damage) by `elevationDelta·3`
  — or `·1` for an Archer attacker (archers care less about height).
- **Facing:** a **side** attack adds `dirBonus 1`, a **rear** attack `dirBonus 2`; each point lowers
  `resist` by 2 **and** cuts the defender's evasion.
- **Terrain:** `gTerrainBonus[terrain]` is subtracted from damage (defensive terrain reduces hits).
- **Supporter bonus:** adjacent allies of the attacker flank the defender, lowering `resist`.

## Experience & levelling

Level is **derived from `experience`** (a `BigInt`) via `gExperienceLevels`
(`src/battle/math.c:913`) — not stored directly, which is why searching for a `.level` write misses
the scaling logic. EXP is granted by `BigIntAdd(unit.experience, gState.experience)` at two sites, each
already guarded by a hard cap `if (unit.level < 50)` (`src/battle/executors.c:648` combat, `:1429`
spell-cast). Support-spell casts grant `(exp-to-next-level)/3` each (`CalculateSupportSpellExp`), which
is the basis of the infinite-EXP exploit. `experience` is stored as **eight big-endian `u16` limbs**
rather than a native integer type — the range needed to hold every level's cumulative EXP without
overflow — manipulated with `BigIntCompare`/`BigIntAdd`/`BigIntSubtract`/`BigIntDivide`.
`CalculateUnitStats` derives the level from `experience` and then derives stats from the level;
`DetermineMaxMpAndStatVariance` separately rolls each unit's 80–119% per-stat variance and its class MP
pool; `SyncGainedHp` carries a unit's current HP forward across a level-up, so gaining a level in
combat doesn't reset the damage already taken.

## Battle flow: turn structure and the executor objects

Battle presentation runs through `battle/field.c`, the front-end, and `battle/executors.c`, the
action objects it spawns.

`Objf013_BattleMgr` is the turn state machine: state 0 waits on `gIsEnemyTurn`; state 13 raises the
"ENEMY TURN" banner, runs the enemy scripted event and its upkeep; states 2–7 loop picking the next
enemy or AI ally, spawning `OBJF_AI_CHOOSE_ACTION` and waiting on `gAiPlanReady`, then walking the
unit to the planned tile (`gX/gZ_801233d8`); states 8–12 dispatch the planned action by
`gAiActionType` (0 = move/face, 1 = attack, 2 = cast) into `OBJF_UNIT_ATTACKING` /
`OBJF_UNIT_CASTING`; states 99/100 clean up; state 14 raises the "PLAYER TURN" banner and restores
the camera. States 101–105 are the map-40 scripted spawn sequence; map 8 is the attract-mode demo
path.

The idle field cursor is `Objf425_BattleOptions` (unit cycling, the turn counter, zoom/options, end
turn); selecting a unit spawns `Objf003_BattleActions`, which drives the move grid, the path walk,
and the Attack/Magic/Item/Wait menu, in turn spawning `OBJF_TARGETING_ATTACK` or
`OBJF_TARGETING_SPELL`. `Objf030_FieldInfo` is terrain/unit inspection; `Objf585_BattlePlayerEvent` /
`Objf587_BattleEnemyEvent` drive per-map scripted dialogue and reinforcement spawns (win/lose rules
live in `battle/evaluators.c`, below).

**The executor objects and their signal protocol.** `Objf015_TargetingAttack` is the attack-side
cursor/confirm step (the mirror of `Objf027_TargetingSpell`, above) and the treasure-chest prompt; it
spawns `OBJF_UNIT_ATTACKING` / `OBJF_OPENING_CHEST` and reports back through `gSignal2` (1 = cancel,
2 = commit, 99 = executor done). `Objf021_UnitAttacking` runs the attack camera, facing, supporter
markers, `CalculateAttackDamage`, the counterattack, XP and any level-up. `Objf028_UnitCasting`
collects targets into `gTargetCoords`, plays the cast animation, then dispatches visuals data-driven
from `gSpellsEx[gCurrentSpell]`: the `MAIN` slot once, `TARGET`/`DEFEAT` per target (handlers live in
the spell FX units; see [spell-fx-dispatch.md](../decomp/spell-fx-dispatch.md)).
`Objf592_BattleTurnStart` is the per-team start-of-turn upkeep — clearing buffs, healing circles,
paralysis-recovery rolls, poison damage, and per-map respawns.

Three `gSignal` variables carry the handshake between these objects: `gSignal3`/`gSignal4` are "step
finished" replies from the unit sprite's action and any spawned FX objects; `gSignal5` is the camera
handshake between the executor and `Objf017_AttackCamera` / `Objf571_LevelUp` — the camera raises 1
once it is in place, the executor raises 99 to release it, and the camera answers 100.
`Objf017_AttackCamera` swoops onto the actor (zoom 250, pitch 33.75°), holds through the strike, then
restores the saved camera; `Objf026_588_FocusCamera` is the general-purpose focus/follow camera used
by spell FX and events, taking a target sprite, one of four vantage types (the `GetBestViewOfTarget*`
family in `core/graphics.c`) and an optional zoom — table slot 588 is the same handler with a wider
default zoom (350).

## Map setup and win/lose evaluation

`State_Battle` loads the map's text, units, portraits, textures and BGM, restores a deferred
in-battle save if one exists, then spawns the battle-ender object (`Objf424_BattleEnder`) and the
map's evaluator before handing off to the intro (`Objf597_BattleIntro`). `gBattleEvaluator[mapNum]`
maps each battle to one `Objf4xx_EvaluateMapNN_<objective>` object, polled through `gState.needEval`
(re-evaluate the board) and `gState.signal` (a search/switch/event trigger raised by the field); `NN`
is the map number, and the displayed battle number is `mapNum − 9`. Plain maps use
`Objf434_EvaluateStandardBattle` (all enemies dead = victory, Ash lost = defeat); named maps add
arrival/escape zones, boss or unit-type kill counts, protect clauses, and some drive
`gState.mapState` for scripted set pieces (see [map-effects.md](../decomp/map-effects.md)). The
verdict lands in `gState.battleEval`; `Objf420_BattleVictory` / `Objf423_BattleDefeat` (one handler,
two slots) play the win/lose letter sprites, then hand off to the results screen or raise `gSignal2`
for the defeat path.

## Battle results screen

`TallySlainUnit` records a kill: party members set a flag in `gPartyMemberSlain`, everyone else
bumps the matching `gSlainUnits` count; both arrays are zeroed per battle and mirrored into the
in-battle save. `CommitPartyStatus` flushes every live unit back into `gPartyMembers` before the
results run. `Objf594_BattleResults` (spawned once the victory banner finishes) draws the results
windows, then walks the kill list and the lost-party list, spawning one `Objf593_BattleResultsUnit`
child every 10 frames — each renders one unit's strip sprite, with a red-X marker under any entry
that is a penalty (a lost party member, or a negative reward value) — alongside a gold counter that
re-renders the running total each time a child posts its reward. Slots wrap at 8 per row, restarting
the grid past 32.

## Unit sprites: loading, state machine, and portraits

`Objf014_BattleUnit` is the on-map unit sprite: `obj->state` selects the action, `obj->state2` the
step — 0 spawn, 1 idle + player-controlled movement, 2 move, 5/9/10 struck and blocking, 6 spell
casting, 8 melee and 12 ranged attack, 15 high step, 16 level up, 7/13/17/18/19 the death variants
(calling `TallySlainUnit`, with a blood or rock-spurt effect), 20 defeat speech.

`LoadUnits` walks the per-unit `UNIT_xx.DAT` files and, for every id present in the current battle's
unit set, uploads that unit's sprite strip to VRAM and copies the packed sheet for the sprite decoder
to unpack. Equipment icons are stored in the gaps between sprite strips, so the load brackets itself
with a save/restore of that region — and the VRAM stride constant that save/restore uses is itself
read *through* the defeat-speech table (a retail data overlap reproduced literally; editing that
table changes the stride). Around the same load, a parking mechanism stashes the game's
`.additional` code overlay (which backs supplies, dojo, world-map and town screens) into four spare
VRAM rects at boot and fetches it back before every state that runs that code. `CreateUnit` has two
entry points: `CreateUnitInNextSlot` takes the first free unit slot, `CreateUnitInLastSlot` forces
the top slot (used to guarantee Leena's slot during her setup). The portrait objects —
`Objf008_BattlePortrait`, `Objf413_MsgBoxPortrait` (with separate mouth/eye speaking/blinking
sub-machines), `Objf447_UnitPortrait`, `Objf575_StatusPortrait` — share one idiom: search the
pending-portraits list for the wanted id (falling back to slot 0), take the palette from the portrait
CLUT-id table, and aim the loaded portrait cell at the right screen rect. `MsgBox_ShowForSprite` /
`MsgBox_SetPortrait` / `MsgBox_Close` drive the upper and lower dialogue windows, anchored to
whichever sprite is speaking. Defeat speech is table-driven and not universal: it is disabled on the
attract-mode demo map and limited to party members on the Trial maps.

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
