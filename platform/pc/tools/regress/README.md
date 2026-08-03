# Regression harness

Two fast, headless checks for the maintenance phase. Both need only a built exe and your own disc
(same as any run); neither needs a display or audio device. Run them after any change that could
touch boot, the data segment, or rendering.

```sh
tools/regress/smoke_boot.sh     # ~7s : boots the real game to the title screen (exit 0 = whole
                                #       boot chain OK: data-segment constructors, disc mount, MDEC
                                #       movie, SPU/XA init, font, rasterizer). Movies auto-skipped.

tools/regress/raster_check.sh   # ~20s first run (records a boot GPU trace + stores its VRAM
                                #       signature), <1s after: replays the trace through the
                                #       production rasterizer and compares the signature
                                #       byte-for-byte. Any rasterization change FAILS it.
```

`raster_check.sh` details: the trace (`build/regress/boot.vht`, gitignored -- it contains
game-derived texture data) captures every VRAM upload and every primitive drawn during boot +
title; replay feeds them through the real `DrawOTag`/`LoadImage` pipeline with no game logic, so
the output is deterministic. After an *intended* rasterizer change, re-baseline with
`rm build/regress/boot.vht*` and re-run. `VH_GPU_REPLAY_VERBOSE=1` prints per-frame hashes to
localize a failure. Validated: a 1-pixel draw-offset perturbation is caught; identical builds
reproduce identical signatures.

Env knobs (also in ../../OPTIONS.md): `VH_SMOKE=1` (exit at title), `VH_SMOKE_LINGER=N` (extra
frames after title, for recording), `VH_GPU_RECORD=<file>` + `VH_GPU_RECORD_FRAMES=N`,
`VH_GPU_REPLAY=<file>`.
