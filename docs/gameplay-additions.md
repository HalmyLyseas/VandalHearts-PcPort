# Gameplay additions (PC port)

Stage 3 adds optional quality-of-life features on top of the faithful port. They never change the
underlying game — the vanilla experience is preserved — they just make it nicer to play with a modern
controller. Everything here is guarded by the `PC_FEAT` build gate, so the matching decompilation is
unaffected. See the [roadmap](roadmap.md) for what's planned next, and [controls.md](controls.md) for
the full button layout.

## Shipped (1.1)

### Twin-stick camera + shoulder unit-cycle

The battlefield camera moves to the **right analog stick** (rotate + raise/lower the angle), freeing
the **shoulder buttons** to cycle through your own units — **L1 = previous, R1 = next** — jumping the
cursor straight to each unit instead of scrolling the map. Cycling skips units that have already acted,
and it's bidirectional, so you can step back and forth naturally. (Vanilla only cycled forward, on
Square.) The shoulder *buttons* and triggers still drive the camera as well, so keyboard players and
pads without a right stick keep the original controls.

### Enemy threat overlay

Press **Square** to toggle a **purple overlay** showing the combined danger zone of **every enemy on
the map at once** — every tile any enemy could move to *and* attack from this turn. It's the fastest
way to plan positioning: you see all the threatened squares in one glance, instead of inspecting each
enemy's range one at a time.

- **Layering:** the overlay sits *below* your own previews, so it never hides what you're doing. When
  you select a unit to move, its blue movement range stays visible — and any of its reachable tiles
  that are *also* under threat turn **yellow** (reachable **and** dangerous). When you target a spell
  or attack, that preview shows on top of the threat.
- **Stays current:** the overlay refreshes automatically when the board changes — a unit moves, a
  crate is pushed, an enemy dies — so it always reflects the real danger. (It updates while you're
  planning at the cursor; during an action animation it hides and reappears, correct, when the action
  completes.)
- **Scope:** it only appears on your turn, and clears at end of turn. Maps with special win conditions
  work the same — it simply shows whatever enemies are present.

Movies (intros/cutscenes) can be skipped at any time with **Start**.

## Planned

See the [roadmap](roadmap.md). Next up in 1.1 is an in-game options overlay (START + SELECT) whose
first setting is per-axis invert for the right-stick camera; 1.2 adds save-file management and window
scaling to that overlay.
