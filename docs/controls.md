# Controls

The PC port maps a standard gamepad (via SDL2's game-controller layer) and the keyboard onto the
PlayStation controller the game expects. Most functions are the original game's; the PC-port additions
are called out in [Gameplay additions](gameplay-additions.md) and marked **(PC)** below.

Face buttons are positional — the physical **south / east / west / north** buttons map to PlayStation
**Cross / Circle / Square / Triangle**, so the layout is correct on both Xbox-style (A/B/X/Y) and
Nintendo-style pads.

## Gamepad — in battle

| Control | PlayStation button | Function |
|---|---|---|
| D-pad / **left stick** | D-pad | Move the cursor |
| South (A) | Cross | Cancel / back |
| East (B) | Circle | Confirm — select a unit, choose an action, confirm a menu item |
| North (Y) | Triangle | Overhead map view |
| West (X) | Square | **(PC)** Toggle the enemy threat overlay |
| **L1 / R1** (shoulders) | — | **(PC)** Cycle through your units — **L1** previous, **R1** next |
| **Right stick** | — | **(PC)** Camera — horizontal = rotate, vertical = raise/lower the angle |
| L2 / R2 (triggers) | L2 / R2 | Camera elevation (raise / lower the view angle) |
| Start | Start | Battle menu / options; skips an intro movie |

Pressing Circle on an empty tile also opens the battle menu (Battle Condition, Turn Over, Zoom, Status,
Options, Save, Load).

## Keyboard — in battle

| Key | PlayStation button | Function |
|---|---|---|
| Arrow keys | D-pad | Move the cursor |
| **S** | Cross | Cancel / back |
| **D** | Circle | Confirm |
| **W** | Triangle | Overhead map view |
| **A** | Square | **(PC)** Toggle the enemy threat overlay |
| **`[` / `]`** | — | **(PC)** Cycle through your units — `[` previous, `]` next |
| **Q / E** | L1 / R1 | Camera rotate (left / right) |
| Enter | Start | Battle menu / options; skips an intro movie |
| Space | Select | (Select; unused in normal play) |

## Camera (PC addition)

Vanilla rotates the battlefield camera with the shoulder buttons. The PC port moves camera control to
the **right analog stick** — push left/right to rotate, up/down to raise or lower the viewing angle —
which frees the physical shoulders for the unit-cycle above. The shoulder *buttons* and triggers still
drive the camera too (rotate / elevation), so keyboard and no-right-stick pads keep the original feel.

> Independent invert for the two right-stick axes is planned, via the in-game options overlay — see
> [gameplay-additions.md](gameplay-additions.md) and the [roadmap](roadmap.md).

Movie playback: press **Start** to skip any intro/cutscene movie.
