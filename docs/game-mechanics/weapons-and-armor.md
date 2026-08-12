# Weapons & armor

Equipment is the single **largest** term in the physical `resist` formula — and simultaneously the one
that **does not differentiate classes**, because every class converges to the same real ceiling. As
with class stats, the number the shop shows you is not the number combat uses.

## How equipment enters combat

Through `resist`, not through any attack/defense stat — and **only for player units** (both terms are
gated on `team == TEAM_PLAYER`, `src/battle_math.c:479-484`):

```c
if (attacker.team == TEAM_PLAYER) resist −= gItemEquipmentPower[attacker.weapon];  // → more damage
if (defender.team == TEAM_PLAYER) resist += gItemEquipmentPower[defender.helmet];
if (defender.team == TEAM_PLAYER) resist += gItemEquipmentPower[defender.armor];
```

**Enemy equipment power never enters the formula.** A top weapon takes a *player* defender's `resist`
from ~20 to ~7 — nearly **3× damage**. In absolute terms it is
the biggest lever in the game. But every class receives essentially the same benefit, so it cancels out
of any *class-vs-class* comparison: equipment is a **power-progression axis, not a class axis.**

## Two power tables that disagree

There are **two** tables, and they differ on **61 of 104 entries**:

- `gItemEquipmentDisplayPower[104]` — what the shop / status screen shows. Inflated and non-linear.
- `gItemEquipmentPower[104]` — what combat actually uses. Compressed.

> **Never balance from the displayed number.** Same "display vs real" split as class attack/defense.

## Weapons — the full real-power progression

Every weapon's **real** power (`gItemEquipmentPower`), by type, in acquisition order — the number
combat actually uses, not the shop display. Used-by is the class line that wields the type.

| type | progression (name · real power) | used by |
|---|---|---|
| SWORD | S.sword 0 · L.sword 1 · I.sword 4 · G.sword 8 · M.sword 12 · **Caliburn 13** · *(V.Heart 14, Ash)* | Knight line |
| BOW | H.bow 0 · L.bow 1 · Iron bow 4 · Stl.bow 8 · Kill bow 9 · Grt.bow 12 · **Rune bow 13** | Archer line |
| STAFF | Staff 0 · L.staff 1 · I.staff 4 · M.staff 8 · S.staff 9 · Arkstaff 12 · **Runewand 13** | Mage / Priest |
| AXE | Iron axe 4 · Battleax 8 · Bloodaxe 10 · Grt.Axe 12 · **Ragnarok 13** | Armored line |
| SPEAR | I.spear 4 · S.lance 8 · Halberd 12 · **D.Spear 13** | Airman line |
| **CLAWS** | Ironclaw 4 · Stl claw 8 · P.claws 10 · **D.claws 12** | **Monk/Ninja line** |

The **display** table (`gItemEquipmentDisplayPower`) is inflated and non-linear — e.g. Ragnarok *shows*
39 against Runewand's 14, yet they are **identical in combat** (both real 13). Balance from the real
column above, never the shown one.

### The claws gap is in the top TWO tiers

Real power tracks the acquisition tier exactly — the "Iron" weapons (real 4) are all bought at the
first shop that sells them, the "Steel/Great" tier (real 8) at the next, and the endgame-buyable tier
at real 12. Comparing each type's weapons **by tier position from the top**:

| tier | SWORD | BOW | STAFF | AXE | SPEAR | **CLAWS** |
|---|--:|--:|--:|--:|--:|--:|
| ultimate | 13 | 13 | 13 | 13 | 13 | **12** ⬇ |
| penultimate | 12 | 12 | 12 | 12 | 12 | **10** ⬇ |

*(ultimate ignores Ash's V.Heart 14.)* The claw line's **top two** weapons — **P.claws 10** and
**D.claws 12** — each land a **full tier below** the norm (12 and 13). The lower two (Ironclaw 4,
Stl claw 8) sit correctly. Aligning the claw powers from the *bottom* obscures this (they match axes
`4/8/10/12` there); the acquisition/tier-position frame is the balance-relevant one.

⇒ **The weapon rebalance is two bytes: `P.claws 10 → 12` and `D.claws 12 → 13`.** That makes the claw
line **`4 / 8 / 12 / 13` — identical to the spear line** (the other 4-weapon melee type). No collateral:
`gItemEquipmentPower` is read **only for player units** (see above), so these bumps affect only the
Monk/Ninja party members who wield claws — never an enemy (enemy Monks *do* get claws via the map
loadout, but their equipment power is never used).

## Defensive gear also converges — heavy plate == cloth robe

| slot | best | display | real | | slot | best | display | real |
|---|---|---:|---:|---|---|---|---:|---:|
| HELM | DrgnHelm | 18 | **5** | | BAND | R.Crown | 13 | **5** |
| ARMOR | Kevlar | **39** | **7** | | ROBE | MojoRobe | 26 | **7** |

Only Ash's Vandal set (V.Heart / V.Helm / V.Armor, real **20** each) breaks the pattern.

⇒ **"Heavy armour access" does not make a class tanky.** A mage in a MojoRobe has *identical* real gear
protection (7) to a fully-plated Armored unit. The Armored line's real durability comes from its
`gAdvantage` resistance, not its armour — see [classes.md](classes.md).

## Takeaways

- Equipment is a **progression** term (huge in absolute damage), not a **class** term (every class
  converges).
- Balance from `gItemEquipmentPower` (real), never `gItemEquipmentDisplayPower` (shown).
- The **claws-at-12** gap is the only real equipment-side class disadvantage in the game.
