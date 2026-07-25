# Weapons & armor

Equipment is the single **largest** term in the physical `resist` formula — and simultaneously the one
that **does not differentiate classes**, because every class converges to the same real ceiling. As
with class stats, the number the shop shows you is not the number combat uses.

## How equipment enters combat

Through `resist`, not through any attack/defense stat (`src/battle_0190dc.c:480-484`):

```c
resist −= gItemEquipmentPower[attacker.weapon];   // better weapon → lower resist → more damage
resist += gItemEquipmentPower[defender.helmet];
resist += gItemEquipmentPower[defender.armor];
```

A top weapon takes a defender's `resist` from ~20 to ~7 — nearly **3× damage**. In absolute terms it is
the biggest lever in the game. But every class receives essentially the same benefit, so it cancels out
of any *class-vs-class* comparison: equipment is a **power-progression axis, not a class axis.**

## Two power tables that disagree

There are **two** tables, and they differ on **61 of 104 entries**:

- `gItemEquipmentDisplayPower[104]` — what the shop / status screen shows. Inflated and non-linear.
- `gItemEquipmentPower[104]` — what combat actually uses. Compressed.

> **Never balance from the displayed number.** Same "display vs real" split as class attack/defense.

## Weapons — every type converges at the top

| type | best weapon | display | **real** | used by |
|---|---|---:|---:|---|
| SWORD | Caliburn *(Ash's V.Heart 40→14 is special)* | 28 | **13** | Knight line |
| AXE | Ragnarok | **39** | **13** | Armored line |
| BOW | Rune bow | 25 | **13** | Archer line |
| STAFF | Runewand | **14** | **13** | Mage / Priest lines |
| SPEAR | D.Spear | 14 | **13** | Airman line |
| **CLAWS** | **D.claws** | 24 | **12** | **Monk/Ninja line** |

Ragnarok *displays* 39 against Runewand's 14 — yet they are **identical in combat** (both real 13).
The one asymmetry: **claws top out at real 12, one below every other type's 13** — the Monk/Ninja
weapon line is the only one that never reaches parity.

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
