# Language packs

An optional feature: the port can display the game's text in another language, loaded from a
**language pack** beside the executable. The **source tree and base build contain no translated
text**, and the game runs in its original English without a pack. A pack is **built from your own
disc** with the tools under [`platform/pc/tools/langpack/`](../platform/pc/tools/langpack/) — text
is the game's copyrighted expression, so this project ships none (see [NOTICE](../NOTICE)).

A pack translates **the game's own content** — story dialogue, menus, battle messages, item and
spell names and descriptions, character and class names, terrain names, the text baked into the
game's code, and Tactical Mode's text. It does **not** translate the port's own SELECT+START options
overlay, which stays English in every language (see *What a pack does not cover*).

## Installing and selecting a pack

Place the pack in a `langpacks/<name>/` folder beside the executable (the same place the game finds
its disc image and `vandalhearts.ini`):

```
langpacks/
  fr-fantrad/
    manifest.json
    strings.bin
```

`langpacks/` is one level deeper than the HD pack's folder because several languages can be
installed at once and chosen from a list. Select one with the **LANGUAGE** row in the SELECT+START
options overlay (System category), or set `VH_LANG=<name>` in `vandalhearts.ini`. `VH_LANGPACK=<dir>`
points straight at a pack folder as a developer override.

**A pack applies when the game starts — changing the language needs a restart.** The in-game LANGUAGE
setting marks a pending change with `*` and applies it on the next launch. This is deliberate: a pack
rewrites structures the game builds once at boot (name tables, repointed string tables, the glyph
sheet in video memory, Tactical Mode's patch set), and text already loaded for the current scene
lives in buffers the game only refills on a scene change. The restart is the one point where
everything is guaranteed coherent.

`manifest.json` is the pack's identity: the runtime checks its `game` id (`vandal-hearts-usa`) and
`format` version before loading anything, and refuses a pack for a different game or a newer format
loudly, continuing in English rather than silently doing nothing. It also carries the pack's display
name, version, author and notes.

## What a pack covers

Every place the game draws text is reachable: story dialogue, all menus and battle messages, item
and spell names and their descriptions, character and class names, terrain names, the strings
hardcoded in the game's code (menus like the party panel, "Battle results", the after-battle
experience message), the Tactical Mode flavour text, and the save/load slot captions. A pack is a
**diff** — an entry it does not translate keeps the original text, so a partial Latin translation is
still a working translation.

The save-slot caption (`Chap. 1 Sct. 1  L5   0:15`) is translated at **display time**: the caption
stored in the save file stays language-neutral English, and its labels are swapped for the pack's
when the load/save menu draws it. So the save file stays portable (it still round-trips to emulators
and console, and reads correctly under any pack or none), and even saves made before a pack was
installed show the translated caption.

## What a pack does not cover

These are deliberate, not omissions:

- **The port's own options overlay (SELECT+START) stays English.** It is drawn by the port with its
  own small Latin-only font; translating it would work for French but never for Russian or Greek, so
  a pack would behave differently depending on the script. Keeping it English holds one rule for
  every language: *packs translate the game, the port's menu stays English* (it is simple English).
- **Six layout labels stay Latin — `HP`, `MP`, `AT`, `DF`, `LEVEL`, and the `L` before a level
  number.** These are not text: the game spells them out as lists of picture numbers baked into the
  window layout, so there is no string for a pack to replace. They are short, universally understood
  abbreviations, already Latin in the Japanese original.

## Languages

- **Latin scripts need no drawing.** English and any Latin-script language render today; accented
  letters are composed automatically from the game's own letterforms (with the usual substitutions —
  `ß`→`ss`, `œ`→`oe`, and Spanish `¡ ¿` unrendered).
- **Non-Latin scripts need drawn glyph art.** A team supplies two sheets of drawn letters (a small
  one for almost everything, a large one for item names); Cyrillic and Greek are proven in game.
  Nordic and Polish letters that cannot be composed use the same route.
- **CJK is not possible** — thousands of characters against ~44 free glyph slots, and the game's 8×9
  pixel letter cells are too small for a legible kanji.

Text is mostly capitals, as the game itself is in English; shop and inventory item names are the
only place that shows mixed case.

## Making a pack

The toolchain and a full authoring guide live in
[`platform/pc/tools/langpack/README.md`](../platform/pc/tools/langpack/README.md): export the game's
text from your disc into a working set, translate it, validate the on-screen budgets, and build a
pack. Working sets and built packs contain disc-derived text and are never committed.
