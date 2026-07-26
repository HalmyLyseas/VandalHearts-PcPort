# Tactical Mode (1.3)

**Tactical Mode is an optional, opt-in rebalance of Vandal Hearts** — a second way to play for people
who want a tighter, more deliberate tactical experience than the retail game offers. It is **off by
default and fully isolated**: the normal game stays byte-for-byte the original, and Tactical saves live
in their own place, so turning it on never touches a vanilla playthrough.

> **Status: released as opt-in beta (in v1.3.0), still in testing.** Everything here is implemented and
> playable. Later chapters are still being validated in playtest, so the exact balance numbers may change
> in 1.3.x point releases. Normal mode is unaffected. See the [roadmap](roadmap.md) and
> [changelog](../CHANGELOG.md).

## Design goals

**Tactical Mode doesn't aim to make the game harder — it aims to make it more varied.** Retail Vandal
Hearts contains a handful of exploits and design choices that push an "optimal" playthrough toward
ignoring several characters and whole classes. The mode targets those specifically:

- **Experience exploits** enabled by the Bishop/Archbishop long-range heals, which snowball the party
  10–30 levels above the content.
- **Trials of Toroah** whose enemies scale to *Ash's* level, so benching Ash trivializes them — and
  which otherwise exist only to unlock the hidden Vandalier class.
- **The Armored path** (Guardsman/Dragoon) having mobility too poor to justify fielding over its Knight
  counterparts.
- **The Monk/Ninja path**, built around agility — a stat Vandal Hearts never actually implements —
  while competing with the AoE-master Sorcerer/Enchanter and the long-range Bishop/Archbishop healers.
- **The Vandalier**, whose retail "ultimate class" leans on a leftover debug path that grants all spells
  and infinite item use, turning Ash into a one-man army.

On top of the rebalance, some **cut content** discovered in the code is restored.

Every change below applies **only in Tactical Mode**; in the normal mode they all keep their exact
retail values.

## Turning it on

Open the in-game options overlay (**Select + Start**) and set **Tactical Mode** to **ON**.

![The options overlay with the new Tactical Mode toggle and Return to Title entry](images/features-1.3-OverlayNewEntries.png)

- The toggle is editable **only at the main title screen** — everywhere else it is shown but greyed, so
  a run can never switch modes underneath itself.
- Start a **New Game** with it on to begin a Tactical run. The choice is saved to `vandalhearts.ini`, so
  it persists between sessions.
- **Saves are separate.** Tactical saves go to their own folder (`saves_tactical/`), and the game's own
  Load/Save screens only ever show the current mode's card, so a vanilla save and a Tactical save can't
  be loaded into the wrong mode. Each save also carries a small internal marker, so even a hand-moved
  file lands back in the right mode.

## Changes

### Experience & the level cap

**Intent — stop the snowball, and bring the combat math back to life.** The support-cast experience
exploit lets a party run 10–30 levels ahead of the enemies it faces. Beyond the raw stats and spells
that come with levels, Vandal Hearts' combat model makes the *level gap itself* a dominant damage
factor — and a large gap can even break enemy AI targeting, producing incoherent behavior.

Experience is now **capped per chapter**, so a unit can catch up to the party but never run away from
the content:

| Chapter | 1 | 2 | 3 | 4 | 5 | 6 |
|---|--:|--:|--:|--:|--:|--:|
| Level cap | 10 | 15 | 20 | 25 | 27 | 30 |

- The instant a unit reaches its chapter's cap it stops gaining experience — so the
  repeat-a-support-spell exploit simply stops paying out.
- Because the cap rises each chapter, a well-played party sits right at the edge of the curve —
  challenged, never trivial.
- Keeping the party near enemy level restores meaning to mechanics the old level gap papered over:
  **damage mitigation**, and the **inherent class differences** in tanking physical vs magical damage.
- The cap doubles as natural **spell pacing**: higher-tier spells come online around the chapter whose
  cap reaches their level.

### Trials of Toroah

**Intent — make Ash matter, and make the Trials worth clearing on their own.** Retail Trials copy
**Ash's** level onto their enemies, so leaving Ash at home makes them trivial; and outside of unlocking
the Vandalier in chapter 6, they offer little reward for the consumables and time they cost.

- Trial enemies now spawn at the **current chapter's cap**, tuned to a well-played party's ceiling no
  matter who you field. Benching Ash no longer keeps them easy.
- Clearing a Trial now grants **gold and experience**, calibrated to that chapter's final-battle values
  (regular enemies, not the boss), compensating the consumables and time an early clear demands.
- Losing a unit in a Trial now carries a **gold penalty**, also scaled to the chapter's final battle —
  the Trials are a resource source, but not a free one.

### Guardsman & Dragoon — mobility

**Intent — make the Armored path an active front-line choice.** Guardsman/Dragoon are the party's
physical tanks and top physical damage dealers, but their mobility made them impractical to field over
the Swordsman/Duelist line. Reading the code showed the real culprit wasn't horizontal range but
**vertical traversal** — their poor ability to climb elevation. Movement in Vandal Hearts is a single
`step` **profile** that bundles horizontal range, climb ceiling, and terrain cost together, so each
change swaps the whole profile:

- **Guardsman → Bowman profile (`step 1 → 5`).** Movement range stays **5**; climb ceiling improves
  (**+1 → +2**). Verticality only — no extra horizontal reach.
- **Dragoon → Sniper profile (`step 4 → 6`).** Gains **1 movement** (**5 → 6**) and the improved climb.

The Swordsman/Duelist line keeps its edge in two areas — one more horizontal tile than the Armored path,
and neutral (non-vulnerable) magic defense — so the choice stays meaningful.

### Monk & Ninja — full rework

**Intent — give the path a reason to exist.** More than any other, the Monk/Ninja needed a ground-up
pass: it's a hybrid (inherently harder to balance), and almost everything in retail worked against it.

- Its kit is built around **agility**, which the game never implements as a mechanic.
- It's outclassed at both ends: Bishop/Archbishop are long-range specialist healers (their Supreme
  Healing is a whole-map heal — literally `range 255 / field 255`) *and* the engine of the experience
  exploit this mode kills; Sorcerer/Enchanter own AoE with Phase Shift and Salamander.
- It leans on its spell list yet has **half the MP pool** of the other casters.

**Foundation changes:**

- **MP pool** brought to exact caster parity (`2 × level`) — `gClassMpMultiplier` **1 → 2**, and the
  retail Monk-only `+advLevelFirst` promotion bonus (which existed to soften the half rate) is dropped
  in Tactical so the two don't stack and overshoot.
- **Magic resistance** improved to sit between the caster and frontline tiers — `magicSusceptibility`
  **3 → 2** (weaker than Priest/Mage, stronger than Swordsman/Duelist).
- **Claws** raised to match their tier's other weapons — Panzer Claw real power **10 → 12**, Dragon Claw
  **12 → 13**.

**Reworked spell list** (bold = changed from retail):

| Spell | area | target | rng | field | power | mp | effect |
|---|---|---|--:|--:|--:|--:|---|
| Stone Shower | AOE | enemy-grp | 0 | 1 | **8** *(4)* | **7** *(10)* | damage |
| Cure Wide | AOE | ally-grp | 0 | **3** *(1)* | 1 | **6** *(4)* | cure |
| Healing Circle | AOE | ally-grp | 0 | **3** *(1)* | **20** *(15)* | **7** *(6)* | heal |
| Perfect Guard | single | ally | **7** *(4)* | 0 | 1 | 15 | AGL+ (1-shot negate) |
| Thunder Flash | AOE | enemy-grp | 0 | **1** *(2)* | **17** *(14)* | **9** *(12)* | damage |
| Healing Wave | AOE | ally-grp | 0 | **3** *(2)* | 28 | 10 | heal |

**Mystic Energy — redesigned.** Retail Mystic Energy was a single-target ATK/DEF buff — underwhelming
for a level-25 spell and overcosted against the Priest line's dedicated ATK and DEF buffs. It's now a
true ultimate: a costly, potent defensive bubble around the Ninja.

- `target`: single → **ally-group**
- `range`: 4 → **0**  ·  `field`: 0 → **3**  ·  `mp`: 15 → **30**
- `effect`: grants **defBoost 3** and **magic resistance 1** to allies in range until the start of the
  next player turn.

For reference, the normal defense buff (Mystic Shield) grants only defBoost 1, and magic resistance 1
matches the caster line's own resistance. It has strong synergy with the Dragoon — soaking a late-game
magic barrage — but its cost keeps it a deliberate tactical tool, not a spammable one.

**The resulting identity:** a hybrid that *wants* to be near the front line, supporting allies and
poking magic-weak enemies.

- All support spells share one shape (`range 0, field 3`); all attack spells share another
  (`range 0, field 1`) — the class rewards standing inside the formation.
- The exception is **Perfect Guard**, whose extended range lets the Ninja hold the front while still
  bubbling a threatened backliner.
- Versus the Knight/Armored frontliners, Monk/Ninja are physically weaker but more magic-resistant,
  immune to ailments, and carry AoE ailment cure.
- Bishop/Archbishop remain the best healers, but Monk/Ninja bring unique support and can still heal
  nearby allies when needed. Sorcerer/Enchanter remain the long-range AoE kings, but Monk/Ninja damage
  is no longer negligible — they can now finish enemies with their own kit, at the cost of being in
  melee range.

**Ninja spell-list identity (lore + cleanup).** A **Monk** (first promotion) keeps its three origin
spells — Huxley/Sara their first three Priest spells, Eleni/Zohar their first three Mage spells. On
promoting to **Ninja**, those are dropped: the unit fully commits to its new path. By that level the
origin spells are obsolete anyway, so this is pure flavor plus a tidier spell list.

### Vandalier — reined in

**Intent — let it be the ultimate class without being a solo win button.** The Vandalier, unlocked
through the Trials, reaches its retail power via a leftover **debug** path that grants *all* spells and
infinite item use. In practice Ash can solo chapter 6 — it's even the *fastest* way to finish it: fewer
units to move, effectively infinite HP/MP through items, and Plasma Wave as a whole-map nuke. That
overshadows what actually makes the Vandalier special:

- Unique weapons and armor well beyond what any other unit can equip.
- The best class in the game statistically by a wide margin (`gAdvantage` table).

Those alone are more than enough to feel ultimate — and reining in the rest brings the *other* party
members back into the final chapter.

- **Removed** the `gState.debug` hook that gave the Vandalier all spells and all items.
- **Kept Plasma Wave** on its spell list — the signature power-fantasy cast survives, but it's now
  MP-limited like any spell, with the infinite-MP item loop gone.

### Restored & clarified content

**Item descriptions.** The mystery `?????` items found on hidden tiles now describe what they do, and
the generic "attack magic item" blurbs are rewritten to name their actual effect.

| Item | Retail | Tactical |
|---|---|---|
| Mad Book | `??????????` | Casts Spellbind |
| Mushroom | `??????????` | Casts Poison Cloud |
| Fire Gem | Attack magic item | Burn enemies in field |
| Moon Pie | `??????????` | Casts Self Healing |
| Mood Ring | Attack magic item | Crushes foes in rings |
| Iron Boot | `??????????` | Casts Perfect Guard |
| Aura Gem | Attack magic item | Mega light attack |
| Unicorn | `??????????` | Casts Rainbow Storm |
| Kingfoil | `??????????` | Casts Healing Circle |
| Helstone | `??????????` | Casts Thunder Ball |
| Wyrmfang | Attack magic item | Huge rings of fire |
| Shiv Book | `??????????` | Casts Dagger Storm |
| Necklace | `??????????` | Casts Dark Hurricane |

**Two cut weapons restored.** The **Bloodaxe** and **Killer bow** are complete, finished items — art,
stats, and all — that the retail game left unobtainable, filling real gaps in the axe and bow
progressions. Tactical Mode sells them in the late-game shops (from Chapter V onward), slotting them
back in as the rightful second-best of their line.

## A note on the numbers

Tactical Mode is a design in playtest, not a finished spec. Some values — a few spell powers and costs
in particular — may still be tuned as the mode is played end to end. The one guarantee that won't
change is the promise at the top of this page: **the normal mode stays exactly the game as it shipped.**
