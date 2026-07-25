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

### The claws gap is ceiling-only, not per-tier

A natural assumption is that claws are underpowered at *every* tier. The data says otherwise: **the
claw line is the axe line minus its top weapon.** Claws `4 / 8 / 10 / 12` are **identical to axes
`4 / 8 / 10 / 12`** at every shared tier — axes simply add a **5th** weapon (Ragnarok 13) that claws
lack. (Versus spears' coarser `4 / 8 / 12 / 13` the claws even have a smoother low-end.)

So the *only* real asymmetry is the **ceiling**: claws top out at **12**, one below every other type's
**13** (Ash's 14 aside). It is the single weapon line that never reaches parity.

⇒ **The complete weapon rebalance is one byte: `D.claws` real power `12 → 13`.** The lower claw tiers
already match axes and need no change; there is no per-tier deficit to chase.

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
