# Performance & settings guide

What the display settings cost, what to expect from your hardware, and how to diagnose a
performance problem so a report is actionable.

## What things cost

The port renders in software on the CPU — that is what makes it pixel-accurate. The costs that
matter:

- **`INTERNAL RES` is the big lever.** Cost grows roughly with the square of the factor (×4 ≈ 16×
  the pixels of ×1), but the high-resolution pass is **multithreaded across CPU cores**, so on a
  modern multicore machine every setting holds its frame cap. Measured on an in-battle heavy scene
  (16 threads): ×1 ≈ 1 ms, ×2 ≈ 2 ms, ×4 ≈ 4–5 ms of rasterization per frame.
- **`WINDOW SCALE` / `FULLSCREEN` are nearly free.** They only change how the finished frame is
  shown (~2–3 ms per frame at ×4, independent of content).
- **The HD pack costs almost nothing during play.** Backgrounds decode on a background thread at
  scene load (no frame dip), and HD movies decode one 15 fps video — a light job for any modern
  CPU.
- **Battle fast-forward is included in the budget.** 2× battle speed doubles how often the game
  ticks; the port holds the full 2× even at `INTERNAL RES` ×4.

**Practical guidance:** on a typical modern desktop, use ×4 + HD pack + fullscreen — everything
holds its caps with headroom. On a weak or old machine (or a machine on battery), lower
`INTERNAL RES` first; ×1 makes rendering nearly free. `VH_RASTER_THREADS` in `vandalhearts.ini`
can *cap* the worker count if you want cores left free for something else.

## Diagnosing a problem

Three opt-in meters print to the console (set them in the `[debug]` section of `vandalhearts.ini`
or as environment variables):

| Meter | Prints | Tells you |
|---|---|---|
| `VH_FPS_LOG=1` | frames per second, once per second | whether you are actually below target |
| `VH_FRAME_TIME=1` | `work` vs `idle` per frame + the work-only fps ceiling | whether the machine is out of budget (`work` near the frame time) or something else is wrong (`work` low but fps still short) |
| `VH_PRESENT_TIME=1` | the display path's phases | whether the cost is in showing the frame rather than rendering it |

A performance report with a few `[frame]`/`[FPS]` lines from a slow scene is immediately
actionable — include them, plus your `INTERNAL RES` setting and CPU model.

### Supported settings vs developer diagnostics

The `VH_*` variables fall into two classes, and the boundary is simple: **anything documented in
[`platform/pc/OPTIONS.md`](../platform/pc/OPTIONS.md) / [configuration.md](configuration.md) (or
the three meters above) is a supported setting; everything else is a developer diagnostic** —
one-off investigation loggers (`VH_*_LOG` CSV writers in `platform/pc/src/pc_diag.c`), the
GPU-trace regression harness (`VH_GPU_RECORD`/`VH_GPU_REPLAY`,
[`tools/regress/`](../platform/pc/tools/regress/README.md)), HD-pack authoring probes
(`VH_HD_DUMP`/`VH_HD_TRACE`/`VH_HD_SYNC`), frame-buffer dumps (`VH_VRAM_DUMP`, `VH_HIRES_DUMP`),
and the `VH_SMOKE` boot harness. Diagnostics are documented next to their code, may change or
disappear in any release, and are never needed to play the game.

## Reference numbers

Measured on a 16-thread desktop CPU, in-battle worst case (large spell effect), `INTERNAL RES` ×4,
HD pack on, fullscreen:

- total work per tick ≈ **10–11 ms** (rasterization 4–5 ms, display ~2.6 ms, game logic the rest)
- battle at 2× fast-forward: **60 ticks/s sustained** (16.7 ms budget, ~6 ms headroom)
- menus/world map: 60 fps with >90 % idle

If your numbers are far from these on comparable hardware, that is worth a report.
