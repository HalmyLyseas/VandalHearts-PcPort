# Contributing

Thanks for your interest! This project is in its **maintenance phase** — the decompilation is
byte-exact, the native port is complete, and the Stage-3 feature arc (v1.1–v1.6) has shipped. New
contributions are most welcome as **bug fixes, small adjustments, and reported issues**.

Start with [docs/architecture.md](docs/architecture.md) — the two-layer structure below is the one
rule set everything else follows from.

## The one rule that must not break

The repository holds two layers:

1. **The matching decompilation** (`src/`, `include/`) rebuilds the original PS1 executable
   **byte-for-byte**. `make check` (top level) verifies it: MD5 `596bb082a2de5f1fe977dd3d7e160b03`.
2. **The native PC port** (`platform/pc/`) compiles that same source against portable backends.

**Any change under `src/` or `include/` must keep `make check` passing.** The matching build defines
none of the port's gates, so every port-side edit to shared source must sit behind one:

| Gate | Use |
|---|---|
| `#ifdef PC_PORT` | portability / 64-bit correctness |
| `#ifdef PC_FEAT` | PC-only gameplay/QoL additions |
| `#ifdef PC_PORT_LP64` | 64-bit-host-only struct-layout fixes |
| `#ifdef PERMUTER` | PC-build layout/behaviour divergences |
| `#ifdef PC_DEBUG_*` | diagnostics, keyed to build flags |

Ungated edits that shift a struct or array break the match *silently* — gate first, then run
`make check`. Do not "clean up" or modernize the decompiled source; its job is byte-exactness,
not readability (see [docs/architecture.md](docs/architecture.md#how-the-port-avoids-breaking-the-match)).

## Port-side conventions (`platform/pc/`)

- **Warning-clean:** port code compiles with `-Wall -Wextra` and must stay at **zero warnings**
  (the decompiled game source has its own silenced profile — don't move suppressions across the
  boundary).
- **Both build systems:** a new source file must be added to **both** the Makefile
  (`BACKEND_SRCS` **plus its own explicit `$(BUILD_DIR)/<name>.o:` rule — there is no generic
  `%.o` pattern rule, and a missing rule fails the build**) and `CMakeLists.txt`
  (`BACKEND_PLAIN`/`DATA_*`) — `platform/pc/tools/check_build_parity.sh` enforces the lists and
  runs before every release build.
- **Both platforms:** anything touching the build or release should compile for Linux **and**
  Windows (MinGW cross-compile, see [docs/cross-platform.md](docs/cross-platform.md)) — every
  release cycle so far has caught a Windows-only break this way.
- Bounded string functions only (`snprintf`, not `sprintf`/`strcpy`).

## Building and testing

[docs/building.md](docs/building.md) covers both builds. Quick version for the port:

```sh
cd platform/pc
make link OPT=1        # optimized build -> build/vandalhearts_pc (needs SDL2/OpenAL/GL/libwebp/libav)
```

You need your own legally-owned game dump (see [README](README.md#playing-the-game)); nothing
copyrighted is in the repo. Two fast headless checks live in `platform/pc/tools/regress/`
(see its README): `smoke_boot.sh` (~7s, boots the real game to the title) and `raster_check.sh`
(golden-image regression for the rasterizer — record once from your disc, byte-exact verify after).
Run both before a PR; beyond that, changes are validated by building both platforms and playing
the affected area, so say in your PR what you tested.

## Housekeeping

- **Screenshots/binary assets:** strip metadata before committing (PNG text/time chunks leak tool
  names and locale timestamps). Prefer neutral filenames.
- **Commit messages:** explain *why*, reference the subsystem; keep matching-decomp and port
  concerns in separate commits.
- **Legal:** never commit game data, Sony SDK material, or upscaled art — see
  [NOTICE](NOTICE) / [DISCLAIMER](DISCLAIMER). The HD pack ships only as a release asset.

## Reporting bugs

Check [docs/known_issues.md](docs/known_issues.md) first, then open an issue — the template asks
for the few things that make a report actionable (OS, version, mode, HD pack on/off, location,
and the console log if the game printed anything).
