# Release checklist

The pre-flight for publishing a release. The build/publish mechanics live in
[cross-platform.md](cross-platform.md) (`platform/pc/packaging/make-release.sh`); this page is the
**order of operations and the checks** — most exist because skipping them has bitten a past release.
Releases are built locally, never CI (the data-segment generator needs the byte-exact game files).

## 1. Correctness gates (before any packaging)

- [ ] **`make check` is byte-exact** (top level) if anything under `src/` or `include/` changed —
      target MD5 `596bb082a2de5f1fe977dd3d7e160b03`. Non-negotiable.
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
- [ ] New screenshots: **strip PNG metadata** before committing (text/time chunks leak tool names
      and locale timestamps), reference them from the docs, and keep raw-URL links in release
      notes pinned to a pushed commit.

## 3. Stage-build BOTH platforms first — always

```sh
platform/pc/packaging/make-release.sh vX.Y.Z --no-publish [--hdpack=<assembled hdpacks dir>]
```

- The script builds **from clean** (it wipes `build_win`/`build_deb` first — incremental objects
  compiled against a different library era have shipped a crash before), front-loads the
  **build-parity check** (Makefile ↔ CMake source lists), and builds
  Windows (MinGW cross, **static libav** — cached at `platform/pc/ffmpeg-mingw-static/`, rebuilt
  by the script if missing) and the Linux AppImage (the `vh-deb12` container; the script verifies
  its HD dev packages).
- **Every graphics-era release caught a Windows-only break at this step.** Do not publish a build
  that only ran on Linux.
- [ ] Windows zip runs in a VM (8 DLLs expected; the exe must import **no** ffmpeg DLLs).
- [ ] AppImage runs (self-contained; FUSE2 note in [cross-platform.md](cross-platform.md)).
- [ ] With `--hdpack`: the zip holds `hdpacks/{backgrounds,videos}/` + manifest, and the pack art
      is metadata-stripped (PII scan below covers it).

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
