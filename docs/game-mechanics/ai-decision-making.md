# AI decision-making

How an enemy unit decides what to do on its turn — decoded from `src/ai.c`. The short version: the AI is
a **score-maximiser with two independent stages** (pick a spell, *then* pick a target), and the score it
maximises is **dominated by movement cost**, with the target's tactical value a comparatively small term.
Several behaviours players read as "smart" or "dumb" fall straight out of these formulas.

Code references point at `src/ai.c` unless noted.

## The two-stage caster decision

A unit that knows spells decides in two **separate** passes. Nothing about the target influences the
first pass — the AI commits to a spell *before* it looks at who to hit.

### Stage 1 — pick the spell (target-blind) · `ai.c:92–166`

From the unit's up-to-two spells (`spells[0]`, `spells[1]`), it selects one `gCurrentSpell` by:

1. **Affordability** — a spell whose `mpCost > mp` is dropped.
2. **Effect type** — damage vs support routes to different behaviour (offensive / self-heal / group-support).
3. **A coin flip** — if *both* known spells are damage, the choice is literally `rand() % 2` (`ai.c:144`).

> **Retail quirk:** `spellEffectB` is computed from `spells[0]`'s effect, not `spells[1]`'s (`ai.c:104`,
> flagged `//? Why spells[0]?`) — an original-game bug that makes the two-spell choice cruder than
> intended. We preserve it (byte-exact). Consequence: which spell a two-damage-spell caster uses is
> effectively random per turn, so its *targeting* can look inconsistent run-to-run.

### Stage 2 — score every target · `func_800569A0`, `ai.c:525`

For the chosen spell, every unit on the map gets a score in `D_8012F63C[]`, branching on the spell's
**effect type** (this is **global per effect-type, not per-spell** — all damage spells share one formula):

**Damage spells** (`ai.c:542–549`), scoring an *enemy* of the caster:
```c
score  = (casterLevel − targetLevel) * 10        // level gap
       + 280                                      // flat base
       − targetHpFrac / 125                       // prefer already-hurt targets
       − gAdvantage[caster.adv][target.adv]       // weapon-triangle matchup
       − terrainPreference[target tile] / 100;    // terrain
```

**Support/heal spells** (`ai.c:582–583`), scoring a wounded *ally* (only if `hpFrac < 7000`, i.e. below
~70% health):
```c
score  = 400 − targetHpFrac / 25;                 // the more hurt, the higher
```

Note what is — and isn't — in the damage score:
- **Weapon-triangle (`gAdvantage`) applies to *magic* spells too.** A Dark Mage "prefers" targets by the
  same physical rock-paper-scissors matchup used for melee, which is often the single largest
  differentiator between otherwise-equal targets.
- **Magic resistance is absent.** Retail scoring has no `magicSusceptibility` term, so the AI will happily
  fire a spell at a magic-*resistant* unit over a magic-*weak* one — see *Tactical Mode* below.

## Choosing where to cast, and the movement tax

For an area spell the AI doesn't just target a unit — it picks a **cast position**, summing the per-target
scores over the spell's field at every *reachable* centre into `D_8017DF50[z][x]` (`ai.c:397–416`). The
final "cast from here" value is (`func_80056F94`, `ai.c:662`):
```c
result = D_8017DF50[target]                        // the cluster's summed target value (~150–500)
       + 10000                                     // large flat base
       − (moveDist − 1) / travelRange * 1000       // MOVEMENT COST — ~1000 per move-step
       + terrainPreference / 100
       − pathGrid6 * 3;
if (result < 1) result = 0;                         // no valid target here
```

**The movement term dominates.** It subtracts ~1000 *per step of movement*, while the entire target-value
term spans only a few hundred points. So the AI's overwhelming preference is **"don't move far,"** and the
tactical value of the target cluster is a secondary tiebreaker among similarly-reachable positions. In
practice the AI casts from at/near where it stands, at whatever cluster that reaches — target quality only
decides between clusters of comparable distance.

## The "won't touch a much-stronger unit" behaviour is emergent

There is **no explicit "refuse high-level target" rule** — it falls out of the score. The level term is
`(casterLevel − targetLevel) × 10`, so a target ~**16 levels above** the caster contributes about −160,
which cancels the +280 base against the routine −80 (HP) − ~40 (matchup): the score reaches ~0. Since
`func_80056F94` treats `result < 1` as "no target," a sufficiently higher-level unit becomes **effectively
untargetable**. This is why a heavily over-levelled party can make enemy casters appear to freeze or
target erratically — the intended victims have scored themselves out of range. (It's authentic retail
behaviour, not a port bug.)

## Physical attacks use a different scorer

Weapon (melee/ranged) target selection goes through `func_80056C30` (`ai.c:593`, called `ai.c:1132`) — a
**separate** evaluation using `gAdvantage`, `attackRange`, and the ranged-attack grid. It never touches
the spell scorer `func_800569A0`. So "which enemy do I hit with my sword" and "who do I nuke" are decided
by independent code paths.

## Takeaways

- **Spell first, target second** — the AI can't pick the *best spell for a situation*; it commits to a
  spell (sometimes by coin-flip) and only then finds the best target for it.
- **Movement is king** — cast-position choice is dominated by not moving; target value is a minor factor.
- **The weapon triangle bleeds into magic** — magic targeting is skewed by a physical-matchup term.
- **Level gap is a soft wall** — big level advantages don't just reduce damage, they can make your units
  *invisible* to enemy casters.
- **Player/enemy symmetry** — unlike the damage formula (which gates several terms on `team == PLAYER`,
  see [combat-mechanics.md](combat-mechanics.md)), this scoring runs the same for both sides.

## Tactical Mode

Tactical Mode adds a **magic-susceptibility term** to the damage scorer so enemy casters actually weigh
magic resistance — preferring magic-weak targets and avoiding resistant/buffed ones — closing the gap
noted above (the retail AI is blind to the very thing the mode's magic rebalance makes matter). It is a
strong bias layered on top of the formula above, not a rewrite; the base factors stay live. See
[tactical-mode.md](../tactical-mode.md).
