# Project skills

This folder holds project-specific Claude Code skills: on-demand, deep-dive knowledge for
continuing the Vandal Hearts decomp → native PC port project. `CLAUDE.md` at the repo root
stays a short always-loaded overview; anything long enough to need its own structure (build
internals, later the PC-port architecture) goes here instead, one subfolder per skill.

## Convention

- Each skill is a folder: `.claude/skills/<skill-name>/SKILL.md`, with YAML frontmatter
  (`name`, `description`) followed by the actual content.
- Keep skills scoped to one concern each rather than one growing catch-all file.
- Update the relevant skill file as work on that concern progresses — treat it as living
  documentation, not a one-time snapshot. If a skill's content turns out wrong or stale,
  fix it in place.
- When starting a new phase of work that needs its own body of knowledge (e.g. the stage-2
  PC-port hardware-abstraction layer once that begins), add a new skill folder rather than
  overloading an existing one.

## Current skills

- `decomp-build/` — the matching-decomp build system: toolchain, dependencies, how to verify
  a build, current known blockers.
- `phase-c-pc-port/` — the native PC port: swappable-interface architecture, subsystem
  status/recipe, and the local psx-spx hardware reference (page index + what it's already
  validated).
