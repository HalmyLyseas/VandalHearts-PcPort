# Tactical Mode (1.3)

**Tactical Mode is an optional, opt-in rebalance of Vandal Hearts** — a second way to play, aimed at
people who want a tighter, more deliberate tactical experience than the retail game offers. It is
**off by default and fully isolated**: the normal game is byte-for-byte the original, and Tactical
saves live in their own place, so turning it on never touches a vanilla playthrough.

> **Status: in testing.** Everything described here is implemented and playable, but the mode has not
> yet shipped in a numbered release and the exact numbers may still change during playtesting. See the
> [roadmap](roadmap.md).

## Why it exists

The retail game is generous in a way that quietly flattens its own tactics. A single support spell,
cast over and over, can pump one unit's level far past the content; once a unit out-levels the map,
the level gap alone decides most fights and positioning stops mattering. Several classes never get a
fair look because a couple of "obviously better" options snowball, and one late promotion turns the
main character into an all-spells, infinite-item god.

Tactical Mode curbs the snowball and gives the sidelined classes a real reason to exist, so that
**party composition and positioning matter for the whole game** — without altering a single thing in
the faithful mode.

## Turning it on

Open the in-game options overlay (**Select + Start**) and flip **Tactical Mode** to **ON**.

![The options overlay with the new Tactical Mode toggle and Return to Title entry](images/features-1.3-OverlayNewEntries.png)

- The toggle can only be changed **at the main title screen** — everywhere else it is shown but
  greyed, so a run can never switch modes underneath itself.
- Start a **New Game** with it on to begin a Tactical run. The choice is remembered in
  `vandalhearts.ini`, so it stays set between sessions.
- **Saves are separate.** Tactical saves go to their own folder (`saves_tactical/`) and the game's own
  Load/Save screens only ever show the current mode's card — a vanilla save and a Tactical save can't
  be loaded into the wrong mode. Each save also carries a small internal marker so that even a
  hand-moved file lands back in the right mode.

## What changes

Every change below applies **only in Tactical Mode**. In the normal mode, all of these keep their
exact retail values.

### A level ceiling that kills the grind

Experience is capped per chapter, so a unit can catch up to the party but never run away from the
content:

| Chapter | 1 | 2 | 3 | 4 | 5 | 6 |
|---|--:|--:|--:|--:|--:|--:|
| Level cap | 10 | 15 | 20 | 25 | 27 | 30 |

The instant a unit reaches its chapter's cap it stops gaining experience, so the repeat-a-support-spell
exploit simply stops paying out. Because the cap rises each chapter, a well-played party stays right at
the edge of the curve — challenged, never trivial. (The cap also doubles as natural spell pacing: the
higher-tier spells come online around the chapter where the cap reaches their level.)

### The Trials of Toroah scale to you

The optional Trials used to copy **Ash's** level onto their enemies — so benching Ash made the Trials
trivial. In Tactical Mode the Trial enemies spawn at the **current chapter's cap** instead. They are
always tuned to a well-played party's ceiling, no matter who you field, so the Trials stay meaningful
and Ash is no longer something to leave at home to cheese them.

### Classes worth fielding

The goal is that each frontline class has a distinct reason to be picked, rather than one option
dominating:

| Class | Change | Why |
|---|---|---|
| **Monk / Ninja** | Bigger MP pool, better magic resistance, a reworked spell kit, slightly stronger claws | Reborn as the **anti-magic skirmisher** — a self-centred support/burst caster who wants to be *in* the formation, complementing the magic-weak Dragoon standing next to it |
| **Guardsman / Dragoon** | More movement range | Their weakness was mobility; a couple of extra tiles lets the physical tanks actually reach the fight |
| **Duelist** | Unchanged | Already the solid all-rounder; with the grind capped and elite weapons scarce, it justifies itself |

The Monk/Ninja spell list is reshaped around a clear identity — small melee-range area attacks and
self-centred group healing/support — so the class rewards good positioning instead of sitting in the
back. (One signature spell, **Mystic Energy**, is reworked below.)

### Ash is a real choice again

The Vandalier — Ash's ultimate promotion — no longer opens a menu of **every spell in the game** plus
free, infinite-use item spells. Instead it casts its **actual** spell kit, so promoting Ash is a
genuine tactical decision rather than an "I win" button. **Plasma Wave is kept** on the Vandalier, so
it still feels like a capstone — just an MP-limited one, now that the free-MP loop is gone. The special
Vandalier ending is unaffected.

### Mystic Energy, reworked

Formerly a modest attack-and-defense buff, **Mystic Energy** becomes a heavy **defensive** ultimate:
it grants a large defense boost **and** near-immunity to magic for a turn. Cast by a Ninja onto a
magic-weak Dragoon, it's the party's answer to a late-game magic barrage. It costs much more to reflect
that power, so it's a decisive play, not a spammable one.

### Restored and clarified content

- **Item descriptions.** The mystery `?????` items found on hidden tiles now say what they do (Mad Book
  casts Spellbind, Mushroom casts Poison Cloud, and so on), and the generic "attack magic item" blurbs
  are rewritten to describe their actual effect.
- **Two cut weapons return.** The **Bloodaxe** and **Killer bow** are complete, finished items — art,
  stats and all — that the retail game left unobtainable, filling real gaps in the axe and bow
  progressions. Tactical Mode sells them in the late-game shops (from Chapter V onward), slotting them
  back in as the rightful second-best of their line.

## A note on the numbers

Tactical Mode is a design in playtest, not a finished spec. Some values (a handful of spell powers and
costs in particular) may be tuned as the mode is played through end to end. The one thing that will not
change is the guarantee at the top of this page: **the normal mode stays exactly the game as it
shipped.**
