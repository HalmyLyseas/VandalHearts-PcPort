# Release checklist

The pre-flight for publishing a release. The build/publish mechanics live in
[cross-platform.md](cross-platform.md) (`platform/pc/packaging/make-release.sh`); this page is the
**order of operations and the checks** — most exist because skipping them has bitten a past release.
Releases are built locally, never CI (the data-segment generator needs the byte-exact game files).

## 1. Correctness gates (before any packaging)

- [ ] **`make check` is byte-exact** if anything under `src/`, `include/` or `jp/` changed — US
      (top level) MD5 `596bb082a2de5f1fe977dd3d7e160b03` **and** JP (`cd jp && make check`) MD5
      `53849277b08184863bd45f10925995a6`, plus `make check-shared` (platform/pc) proving the
      shared game files are still identical between the trees. Non-negotiable.
- [ ] **Comment hygiene:** `make check-comments` (platform/pc, ~1s) — no comment run over 3 lines
      and no project-log vocabulary (dates, `exchange/`, stage/version markers) in any tracked file.
- [ ] **Boot smoke:** `platform/pc/tools/regress/smoke_boot.sh` (~7s) — the whole boot chain
      (data-segment constructors, disc mount, MDEC, SPU/XA, font, rasterizer).
- [ ] **Raster golden-image:** `platform/pc/tools/regress/raster_check.sh` (<1s) — byte-exact VRAM
      signature vs your recorded baseline. If it fails on an *intended* rasterizer change,
      re-baseline (`rm platform/pc/build/regress/boot.vht*`, re-run) and say so in the notes.
- [ ] **Warning-clean port build**, both systems: `make link OPT=1` and the CMake build produce
      **zero** `platform/pc` warnings (`-Wall -Wextra` is the enforced policy).
- [ ] **Struct-width A/B** (`platform/pc/tools/struct_width_diff.sh`, build/ vs build32/) if any
      struct layout or the `Object` union was touched — the width-bug class is invisible to
      sanitizers ([width-bugs.md](width-bugs.md)).
- [ ] In-game validation of whatever actually changed, on at least one platform.

## 2. Version bookkeeping

- [ ] **`CHANGELOG.md`** gains the `[x.y.z]` section — it *is* the release notes
      (make-release.sh extracts it into the GitHub release body).
- [ ] [roadmap.md](roadmap.md), [known_issues.md](known_issues.md), the READMEs and `CLAUDE.md`
      say what is now true (shipped vs planned; new options documented in
      [`platform/pc/OPTIONS.md`](../platform/pc/OPTIONS.md) / [configuration.md](configuration.md)).
      Mark known-issues entries this release fixes as RESOLVED, and bump the version example in
      `.github/ISSUE_TEMPLATE/bug_report.yml` — both went stale at 1.6.1 and were caught by users.
- [ ] New screenshots: **strip PNG metadata** before committing (text/time chunks leak tool names
      and locale timestamps), reference them from the docs, and keep raw-URL links in release
      notes pinned to a pushed commit.

## 3. Stage-build BOTH platforms first — always

```sh
platform/pc/packaging/make-release.sh vX.Y.Z --no-publish [--hdpack=<assembled hdpacks dir>]
```

- The tag must match `vX.Y.Z` or `vX.Y.Z-suffix` (e.g. `v1.6.2`, `v2.0.0-rc1`) — the script
  rejects anything else before touching the filesystem, since the tag becomes the staging path
  it wipes with `rm -rf`.
- The script builds **from clean** (it wipes `build_win`/`build_deb` first — incremental objects
  compiled against a different library era have shipped a crash before), front-loads the
  **build-parity check** (Makefile ↔ CMake source lists) and **`make check-shared`** (the
  shared-region identity gate — a diverged shared TU fails the release here, not silently), and
  builds Windows (MinGW cross, **static libav** — cached at `platform/pc/ffmpeg-mingw-static/`,
  rebuilt by the script if missing) and the Linux AppImage (the `vh-deb12` container; the script
  verifies its HD dev packages).
- **Why clean builds are a hard rule:** neither build system tracks compiler-flag or include-path
  changes, so an incremental build can link objects compiled against one library era with archives
  from another (a `pc_hdvideo.o` compiled against the container's shared libav headers linked into
  the static-libav binary crashes in `avcodec_parameters_to_context` from mismatched struct offsets).
- **The parity check** (`tools/check_build_parity.sh`) parses the actual source *lists*
  (`BACKEND_SRCS` + `$(BUILD_DIR)/<stem>.o` usages in the Makefile; `BACKEND_PLAIN`/`DATA_PERM`/
  `DATA_PLAIN` + the `foreach` stems in CMake), not whole-file mentions — a comment naming a file is
  not a compile. It fails closed if either parse yields implausibly few entries, so a list-format
  change cannot silently disable it. `src/test_*.c` harnesses are exempt.
- **The script refuses to run while user data sits in `build-uni/`** (disc `.bin`s, `saves/`,
  `hdpacks/`, `langpacks/`): the AppImage stage runs `rm -rf build-uni*`, which would delete a test
  deployment parked there. Keep deployments in `platform/pc/deploy/` (gitignored) instead.
- The shipped Windows exe is **stripped** (debug info has no runtime use there — no backtrace
  machinery — and embeds local build paths); the script then greps the exe for `/home/` and the login
  name and refuses to package one that still carries them. The unstripped exe stays in
  `build_win_uni/` for debugging.
- **Every graphics-era release caught a Windows-only break at this step.** Do not publish a build
  that only ran on Linux.
- [ ] Windows zip runs in a VM (8 DLLs expected; the exe must import **no** ffmpeg DLLs).
- [ ] AppImage runs (self-contained; FUSE2 note in [cross-platform.md](cross-platform.md)).
- [ ] Both packaged binaries are the **unified** build (all supported discs; the script's
      Windows path runs `build-unified-win.sh`, the Linux path `make unified` from clean) — boot
      each against a US **and** a JP disc.
- [ ] With `--hdpack=<hdpacks root>`: the script packages one zip per `<game-id>/manifest.json`
      subfolder it finds (`VandalHearts-<tag>-hdpack-<GAME-ID>.zip`, holding `hdpacks/<game-id>/…`)
      — the release notes' Downloads table lists exactly the zips this run produced, never a fixed
      guess. The pre-2.0 flat layout (a root `manifest.json`, no per-game subfolder) still packages
      as a single `…-hdpack.zip` with a deprecation warning. A symlink inside the pack pointing
      outside it is refused rather than dereferenced into the asset. The pack art is
      metadata-stripped (PII scan below covers it). Re-upload the packs on every release (users
      download from the latest release page, not old ones).

## 4. Publication hygiene (public repo)

- [ ] **PII scan** over tracked content *and* the release artifacts (names, logins, hostnames,
      `/home/` paths); commits authored by the noreply identity.
- [ ] No references to `exchange/` or other local-only paths in anything user-facing.
- [ ] Legal files still true: `NOTICE` / `DISCLAIMER` cover everything the release distributes
      (binaries embed game-derived data; the optional HD pack is disclosed derivative art).

## 5. Publish

```sh
platform/pc/packaging/make-release.sh vX.Y.Z [--hdpack=<dir>]   # tags HEAD, uploads, notes from CHANGELOG
```

- [ ] Tag == pushed `master` HEAD; assets all present with `SHA256SUMS.txt`; release-notes images
      render (raw URLs resolve).
- [ ] Post-publish: click through the release page once as a stranger would — download table,
      known-issues link, screenshots.
