# Tactical Mode (1.4)

**Tactical Mode is an optional, opt-in rebalance of Vandal Hearts** — a second way to play for people
who want a tighter, more deliberate tactical experience than the retail game offers. It is **off by
default and fully isolated**: the normal game stays byte-for-byte the original, and Tactical saves live
in their own place, so turning it on never touches a vanilla playthrough.

> **Status: released as an opt-in mode.** Everything here is implemented, playable, and validated across
> a full playthrough. Normal mode is unaffected. See the [roadmap](roadmap.md) and
> [changelog](../CHANGELOG.md).

## Design goals

**Tactical Mode doesn't aim to make the game harder — it aims to make it more varied.** Retail Vandal
Hearts contains a handful of exploits and design choices that push an "optimal" playthrough toward
ignoring several characters and whole classes. The mode targets those specifically:

- **Experience exploits** enabled by the Bishop/Archbishop buffs and MP transfer spells, which snowball
  the party 10–30 levels above the content.
- **Trials of Toroah** whose enemies scale to *Ash's* level, so benching Ash trivializes them — and
  which otherwise exist only to unlock the hidden Vandalier class.
- **The Armored path** (Guardsman/Dragoon) having mobility too poor to justify fielding over its Knight
  counterparts.
- **The Monk/Ninja path**, built around agility — a stat Vandal Hearts never actually implements —
  while competing with the AoE-master Sorcerer/Enchanter and the long-range Bishop/Archbishop healers.
- **The Vandalier**, whose retail "ultimate class" leans on a leftover debug path that grants all spells
  and infinite item use, turning Ash into a one-man army.

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
| Level cap | 10 | 15 | 19 | 24 | 28 | 32 |

The curve climbs a steady **+4 per chapter** after the opening chapter (starters begin at level 5–6, so
chapter 1 is a +5 ramp), and chapter 6's cap of **32** lands the endgame party right on the final
boss's level — validated across a full playthrough.

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

| Spell | area | target | rng | field | power | mp | effect | reqLv |
|---|---|---|--:|--:|--:|--:|---|---|
| **Spread Force** | AOE | enemy-grp | 0 | 3 | **8** *(13)* | **8** *(7)* | damage | 12 |
| Cure Wide | AOE | ally-grp | 0 | **3** *(1)* | 1 | **6** *(4)* | cure | 15 |
| Healing Circle | AOE | ally-grp | 0 | **3** *(1)* | 15 | 6 | heal | 17 |
| Perfect Guard | single | ally | **6** *(4)* | 0 | 1 | **12** *(15)* | AGL+ (negate 1 hit) & **magic resist** | 19 |
| Thunder Flash | AOE | enemy-grp | 0 | **3** *(2)* | 14 | **14** *(12)* | damage | 21 |
| Healing Wave | AOE | ally-grp | 0 | **3** *(2)* | 28 | **12** *(10)* | heal | 23 |
| Mystic Energy | **AOE** (single) | **ally-grp** (ally) | **0** (4) | **3** (0) | 1 | **35** (15) | Defense buff **& magic resist** | 25 |

The Monk/Ninja now learns **Spread Force** — a low-power, no-range group hit that rewards standing in
the formation — in place of retail's redundant Stone Shower (see *Sorcerer & Enchanter* below for where
Stone Shower's slot went). **Thunder Flash** trades a wider field for a higher cost; the two group heals
are separated into distinct roles — **Healing Circle** a cheap, efficient top-up, **Healing Wave** the
heavy heal. **Perfect Guard** is cheaper and now also confers magic resistance (see below).

**Mystic Energy — redesigned.** Retail Mystic Energy was a single-target ATK/DEF buff — underwhelming
for a level-25 spell and overcosted against the Priest line's dedicated ATK and DEF buffs. It's now a
true ultimate: a costly, potent defensive bubble around the Ninja.

- `target`: single → **ally-group**
- `range`: 4 → **0**  ·  `field`: 0 → **3**  ·  `mp`: 15 → **35**
- `effect`: grants **defBoost 3** and **magic resistance 1** to every ally in the field until the start
  of the next player turn.
- Now casts as a true area buff — an icy-blue protective aura on each ally in range, with a "DEF up"
  indicator (the boost is defensive only).

For reference, the normal defense buff (Mystic Shield) grants only defBoost 1, and magic resistance 1
matches the caster line's own resistance. It has strong synergy with the Dragoon — soaking a late-game
magic barrage — but its cost is deliberately steep: at 35 MP, casting it twice in a row costs more than
a full late-game caster pool, so a second consecutive cast needs an MP-refill item. A tactical tool, not
a spammable one.

**The resulting identity:** a hybrid that *wants* to be near the front line, supporting allies and
poking magic-weak enemies.

- All support spells share one shape (`range 0, field 3`); all attack spells share another
  (`range 0, field 2`) — the class rewards standing inside the formation.
- The exception is **Perfect Guard**, whose extended range lets the Ninja hold the front while still
  bubbling a threatened backliner. In Tactical it also grants **magic resistance** for a turn, turning
  it into a single-unit "evasion + anti-magic" shield — a cheap, targeted way to protect a magic-weak
  ally right before an enemy caster's turn (its description was always "Protect Magic"; now it is).
- Versus the Knight/Armored frontliners, Monk/Ninja are physically weaker but more magic-resistant,
  immune to ailments, and carry AoE ailment cure.
- Bishop/Archbishop remain the best healers, but Monk/Ninja bring unique support and can still heal
  nearby allies when needed. Sorcerer/Enchanter remain the long-range AoE kings, but Monk/Ninja damage
  is no longer negligible — they can now finish enemies with their own kit, at the cost of being in
  melee range.

**Origin spells are kept.** Both promotions retain their three origin spells — Huxley/Sara their first
three Priest spells, Eleni/Zohar their first three Mage spells. (An earlier revision dropped them at the
Ninja tier for a cleaner list; playtesting showed that was a needless nerf to a path that doesn't need
one, so they stay.) The retained basics keep real niche value late — a cheap top-up heal, a longer-range
single-target cure, a mini defense buff, or a paralyze poke — so the class stays flexible rather than
losing utility on promotion.

### Sorcerer & Enchanter — spell tuning

**Intent — give the mage kit two more reasons to reach.** Two targeted changes complement the Monk/Ninja
rework:

- **Roman Fire power `7 → 9`.** In retail it's strictly overshadowed by Phase Shift (learned two levels
  earlier, wider field); at power 9 it matches Phase Shift's punch, so its longer range and lower cost
  become a real niche against clustered or distant foes.
- **Thunder Ball added.** The Monk/Ninja rework hands the mages' old Stone Shower slot over to a new
  **Thunder Ball** — a long-range, small-area attack spell — filling the gap where Stone Shower used to
  sit and giving the caster a precise ranged option distinct from the big-field Phase Shift / Salamander.

| Spell            | area    | target        |   rng | field |     power |     mp | effect     | reqLv  |
| ---------------- | ------- | ------------- | ----: | ----: | --------: | -----: | ---------- | ------ |
| Dark Star        | single  | enemy         |     4 |     0 |         1 |      2 | DAMAGE     | 1      |
| Spellbind        | single  | enemy         |     5 |     0 |         1 |      2 | PARALYZE   | 8      |
| Piercing Ray     | AOE     | enemy-grp     |     4 |     1 |         2 |      4 | DAMAGE     | 10     |
| Envenom          | single  | enemy         |     5 |     0 |         1 |      3 | POISON     | 12     |
| Phase Shift      | AOE     | enemy-grp     |     0 |     7 |         9 |     12 | DAMAGE     | 14     |
| Roman Fire       | AOE     | enemy-grp     |     5 |     2 | **9** (7) |      6 | DAMAGE     | 16     |
| Poison Cloud     | AOE     | enemy-grp     |     5 |     2 |         1 |      4 | POISON     | 19     |
| **Thunder Ball** | **AOE** | **enemy-grp** | **5** | **1** |    **13** | **10** | **DAMAGE** | **21** |
| Avalanche        | single  | enemy         |     6 |     0 |        18 |      9 | DAMAGE     | 23     |
| Salamander       | AOE     | enemy-grp     |     0 |    10 |        13 |     14 | DAMAGE     | 25     |

### Avalanche — an ice re-skin

**A cosmetic touch.** Avalanche is thematically a wall of snow and ice, yet its retail effect is a
rolling boulder of grey stone. In Tactical the boulder is re-skinned as ice — a translucent, brightly-lit
crystalline mass — to match the spell's name. Normal mode keeps the original stone.

![Avalanche in retail (Normal mode): a grey stone boulder](images/features-1.3.1-Avalanche-Before.png)

![Avalanche in Tactical: the boulder re-skinned as bright ice](images/features-1.3.1-Avalanche-New.png)

Only the main boulder is re-textured; the tumbling debris behind it is drawn from fixed rock sprites, so
it stays rocky — reading as an ice-and-rock slide. A full snow effect would need new artwork.

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

### Clarified item descriptions

The mystery `?????` items found on hidden tiles now describe what they do, and the generic "attack
magic item" blurbs are rewritten to name their actual effect.

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

### Magic-aware enemy casters *(added in 1.4)*

Everything above makes magic resistance *matter* — the `magicSusceptibility` rebalance, and the resistance
that **Perfect Guard** and **Mystic Energy** now grant. But retail's enemy AI is **blind** to it: it scores
targets without any magic-resistance term, so it will happily fire a spell straight into your most
resistant unit, and the buffs you spend a turn setting up do nothing to deter it.

Tactical Mode closes that gap. Enemy spellcasters now **weigh magic resistance when choosing a target** —
biasing toward magic-weak units and away from resistant or buffed ones. So a shielded backliner or a
Mystic Energy'd cluster genuinely reads as a *worse* target to the enemy, and the defensive tools this mode
adds finally pull their weight on defense. It's a bias layered on top of the original targeting math, not a
rewrite, and it's **Tactical-only** — normal mode's AI is untouched. The full targeting model (and why
retail behaves the way it does) is in
[game-mechanics/ai-decision-making.md](game-mechanics/ai-decision-making.md).

## A note on the numbers

Tactical Mode has been validated across a full playthrough, but it stays a living design — a few spell
powers and costs may still be fine-tuned in later point releases. The one guarantee that won't change is
the promise at the top of this page: **the normal mode stays exactly the game as it shipped.**
