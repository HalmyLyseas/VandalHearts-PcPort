# Language-pack tooling

The pipeline that builds optional **language packs** for the PC port. A pack lives in
`langpacks/<name>/` beside the executable and is selected with `VH_LANG=<name>` in
`vandalhearts.ini` — no pack, or no selection, means the untouched original text. The base build
ships no language content.

Like everything else in this project, the pipeline works from **your own disc image**: game text is
never committed to this repository, and neither are built packs or working sets — the tools
regenerate everything from the disc.

`<disc>` is your own disc image, `<src>` is the repository's `src/` directory, `<bal>` is
`platform/pc/src/pc_balance.c`, `<work>` is your working folder and `<out>` is the build output
folder.

---

# 1. Workflows

Pick the one that matches your language. **Latin** covers ASCII plus accented Latin the game can
compose on its own (see [Supported characters](#supported-characters)). **Non-Latin** is any script
the game has never drawn — Cyrillic, Greek, Nordic, Polish — which must ship its own glyph art.

## Latin pack

```
# 1 — export (run all four into the SAME <work>)
lang_export.py           <disc> <work>     # executable tables + on-disc dialogue -> <work>/strings/
lang_export_literals.py  <src>  <work>     # text hardcoded in game code          -> strings/literals.json
lang_export_tactical.py  <bal>  <work>     # Tactical Mode text                   -> strings/tactical.json
lang_group.py            <work>            # entity-grouped translator views      -> <work>/translate/

# 2 — translate: edit <work>/translate/*.json and <work>/strings/dialogue/*.json
#     (every entry carries its English source and its display limit)

# 3 — merge your translate/ edits back into strings/
lang_merge.py            <work>            # --revert-cleared: a field you emptied reverts to English

# 4 — validate (hard rules + on-screen fit lint)
lang_validate.py         <disc> <work>

# 5 — build
lang_build.py            <disc> <work> <out> --lang <name>
                         # optional: --name --author --version --notes  --mixed-case
```

Result: `<out>/langpacks/<name>/{manifest.json, strings.bin}`.

## Non-Latin pack (Cyrillic, Greek, …)

Same as above, **plus a glyph-art step between merge and validate**, and `--packart` on both
validate and build. Two things are different in kind:

- **The small font is capitals-only** — write your translation in CAPITALS. (The game already
  prints most text in capitals; the small sheet holds ~44 glyphs, which fits one alphabet's
  capitals but not capitals *and* lowercase. See [The two fonts](#the-two-fonts).)
- **A script pack must be complete** — in script mode any untranslated string would render as
  nonsense, so the builder refuses a partial pack (`--allow-incomplete` exists for mid-work testing
  only).

```
# 1 — export        (same four commands as above)
# 2 — translate      IN CAPITALS
# 3 — merge
lang_merge.py            <work>

# 4 — draw the script's glyph sheets FROM the finished (merged) translation
lang_template.py         <work> --out <artdir>   # derive the exact letters + rasterise a sheet
                         # or draw a whole alphabet directly: gen_packart.py <artdir> --script ru

# 5 — validate WITH the sheets
lang_validate.py         <disc> <work> --packart <artdir>

# 6 — build WITH the sheets
lang_build.py            <disc> <work> <out> --lang <name> --packart <artdir>
```

Step 4 must come **after** merge: `lang_template` reads the merged `strings/`, so drawing before
merge would miss every letter used only in your `translate/` menu and item edits.

---

# 2. Topics

Short reference sections — read the ones relevant to what you're doing.

## Working folders

`<work>` and `<out>` hold text extracted from your disc, so they must **never be committed**. Use
`platform/pc/tools/langpack/work/` and `.../out/` (both git-ignored), or any location outside the
repository. The `.gitignore` here also ignores any `strings/`, `translate/` or `langpacks/` folder
created under this directory — but a path outside the repo is the surest choice.

## Save your JSON as UTF-8

The whole chain is UTF-8, and the tools read and write it as UTF-8 on every platform — **not** the
system default, which matters on Windows (a legacy code page). Any editor's plain "UTF-8" save is
correct. A file saved in another encoding stops the tools with a message naming the file and the
offending byte, rather than silently corrupting the text.

## Verify the export before translating

An untouched working set should validate clean and build to an *empty* pack:

```
lang_validate.py <disc> <work>                   ->  0 error(s), 0 warning(s)
lang_build.py    <disc> <work> <out> --lang en-test ->  (0 sections)
```

Zero sections is the point, not a failure: a pack only carries what you changed, so "nothing
translated yet" must produce nothing. If either step reports otherwise, the export is incomplete —
fix that before translating.

## Pack naming

`<languageTag>-<freeDescription>` — an ISO 639-1 code (optionally with a region subtag), then
lowercase `a-z 0-9 . _ -` only: `en-fix`, `fr-fantrad`, `pt-br-fantrad`. Enforced by `lang_build.py`;
the constraint keeps names URL-safe, shell-safe, and immune to case-sensitivity differences between
platforms.

`manifest.json` is the pack's real identity — the runtime checks its `game` id and `format` version
before loading anything, and displays its `name`/`version`. Everything else in a pack folder (a
README, credits, notes in any language) belongs to the pack's authors; the loader reads only
`manifest.json` and `strings.bin`.

## When a pack applies — restart required

**A pack applies at game start; changing the selection requires a restart.** The in-game LANGUAGE
setting marks a pending change with `*` and it takes effect on the next launch. This is deliberate:
a pack rewrites structures the game builds once at boot (name tables, repointed string tables, the
glyph sheet in video memory, Tactical Mode's patch set), and text already loaded for the current
scene lives in buffers the game only refills on a scene change. A live switch would leave the game
half in each language. For authors the iteration loop is: edit → `lang_build.py` → restart. There is
no in-game reload.

## A pack is a diff

Untranslated entries show the original text, so a **partial Latin translation is a working
translation**. (Non-Latin packs are the exception — they must be complete; see the workflow above.)

## Mixed case (`--mixed-case`, Latin only)

By default the game renders almost all text in ALL-CAPS — it folds `a`–`z` onto its uppercase
letterforms (only shop/field item names escape). Pass `--mixed-case` at build time and a Latin pack
renders text in the case you actually wrote (`Café déjà reçu`, not `CAFÉ DÉJÀ REÇU`), reusing the
font's own lowercase glyphs — no new art. It is the translator's decision, baked into the pack;
there is no player-facing toggle. Non-Latin packs set their own case in their drawn sheets and do
not use this flag.

## What a pack does not cover

The port's own in-game options overlay (SELECT+START) stays English in every language. It is drawn
by the port with its own small font; keeping it out means every pack behaves the same regardless of
script.

## The two fonts

The game uses **two fonts** and picks one per screen. A script it has never seen therefore needs its
alphabet drawn **twice**, once at each size. The sheets are independent files — you can ship one
before the other.

| | **small sheet — 8×9 px** | **large sheet — 16×15 px** |
|---|---|---|
| **where your letters show up** | story dialogue, every menu, item/spell descriptions, spell names, character names, class names, terrain names, battle messages | item names in shop/field/inventory; the TURN counter; YES/NO prompts |
| **how much of the game** | almost all of it | a handful of screens |
| **letters you can add** | **44** | no practical limit |
| **if you don't supply it** | nothing readable renders — pack unusable | those screens stay English; the rest works |
| **drawing it** | very tight — plan on capitals only | roomier, easier per letter |

**You don't draw digits, punctuation or spaces** — those exist at both sizes and keep working.

**44 is the real ceiling, and it only applies to the small sheet.** It fits a full Russian alphabet
(33) or Greek (~24) in capitals. It does *not* fit capitals *and* lowercase of a non-Latin script
(~66) — this is a hard limit of how the game stores its font. Since the game already prints most
text in capitals, capitals-only is not the regression it sounds like.

**Staging tip:** translate everything *except* item names first and ship with only the small sheet.
Untranslated item names keep their original text and render normally, so you get a playable,
almost-complete pack while the large sheet is still being drawn.

## Drawing a non-Latin script

`--packart <dir>` supplies the sheets and switches the whole pack to **script mode** (1-byte codes
throughout — UTF-8 cannot carry these letters through the game's dialogue path).

**1. Which letters does my translation need?** `lang_template.py` reads the merged `strings/` and
lists every letter the game cannot draw on its own — exactly what a `--packart` sheet must supply:

```
lang_template.py <work>              # report: the letters, split by sheet, + the command to run
lang_template.py <work> --out <dir>  # …and rasterise a starting sheet for exactly those letters
```

It skips ASCII and the accented Latin the builder synthesises for free, so what remains is precisely
what needs drawing. If nothing does, it says so — the translation builds as a Latin pack.

**2. Draw them from a bitmap font.** `gen_packart.py` rasterises GNU Unifont (bundled as
`unifont-subset.bdf`) straight into both sheets — call it directly for a whole alphabet, or let
`lang_template --out` call it for exactly your translation's letters:

```
gen_packart.py <dir> --script ru          # a preset alphabet in CAPITALS (ru = Russian, el = Greek)
gen_packart.py <dir> --script el
gen_packart.py <dir> --cps U+0410-U+042F  # or explicit codepoints
```

It writes `font8x9.*` + `font16x15.*` ready for `--packart <dir>`, plus `proof_8x9.png` /
`proof_16x15.png` — each cell magnified with its codepoint, so you can confirm every letter came out
right. The **16×15 sheet is production quality**; the **8×9 sheet is legible** and a good base — a
team may hand-tweak a few of the densest letters, but nobody starts from a blank grid. KROMDAT / the
PlayStation BIOS is not involved. (Unifont is OFL/GPL; see `NOTICE-unifont.txt`.)

### Punctuation the base game can't draw

The game draws the common marks (`? ! , . - : ' " + / < = %`) but has **no glyph for these ASCII
characters**: `& ( ) * ; > @ [ \ ] ^ _ \` { | } ~` — notably `;`, which Greek uses as its question
mark. If your translation
uses one of these, treat it exactly like a letter: **draw it**. `lang_template` reports it alongside
the alphabet (`U+003B ; SEMICOLON`), `gen_packart` rasterises it from Unifont, and the builder
installs it at its own byte so it renders as itself. `gen_packart --script el` includes `;`
automatically. Without a supplied glyph, using one of these characters is a build error (it would
otherwise be silently reassigned to an alphabet letter).

`<dir>` holds up to two sheets, each a **PNG image + a `.txt` manifest**:

| files | cell size | covers | required? |
|---|---|---|---|
| `font8x9.png` + `font8x9.txt` | 8×9 px | small font — almost everything | **yes** |
| `font16x15.png` + `font16x15.txt` | 16×15 px | large font — item names, TURN, YES/NO | optional |

- **PNG** — a 1-bit image of the letters packed into a grid, filled **left to right, then top to
  bottom** (reading order). A cell is exactly the cell size; image width fixes cells-per-row
  (`width ÷ cell width`). **A black pixel is ink**; white is background. An all-white cell means
  "not supplied" and is skipped.
- **`.txt`** — one `U+XXXX` per line, in the **same reading order** as the cells. Lines starting
  with `#` are comments; blank lines are ignored.

Draw **only the letters your alphabet adds** (33 Russian, ~24 Greek). Rendering the sheet back out
(the proof PNG) is the best check: every letter should land in the cell its codepoint predicts.

## Supported characters

**Encoding and repertoire are two different things.** Pack files are UTF-8 — that is the encoding,
not a capability claim. What can *render* is defined by glyph supply, from two sources:

- **the synthesiser (no art needed):** ASCII, plus any Latin letter that decomposes into a base
  letter (`a`–`z`) and one of six marks — composed automatically from the game's own letterforms.
- **pack-supplied glyph art (`--packart`):** everything else — Nordic (`å ø æ ð þ`), Polish
  (`ł ż ą ę`), and every non-Latin script (Cyrillic and Greek are drawn and proven in game).

A codepoint with neither a synthesised nor a supplied glyph is **refused at build time** — never
silently dropped. Generated by `lang_build.py --repertoire` (this table cannot drift from what the
builder enforces):

```
Marks: grave, acute, circumflex, tilde, diaeresis, cedilla

Lowercase -- renders EVERYWHERE (67):
  à á â ã ä ç è é ê ë ì í î ï ñ ò ó ô õ ö ù ú û ü ý ÿ ć ĉ ĝ ģ ĥ ĩ ĵ ķ ĺ ļ ń ņ ŕ ŗ
  ś ŝ ş ţ ũ ŵ ŷ ź ǵ ǹ ȩ ḑ ḧ ḩ ḱ ḿ ṕ ṽ ẁ ẃ ẅ ẍ ẑ ẗ ẽ ỳ ỹ

Uppercase -- item-name path only (66):
  À Á Â Ã Ä Ç È É Ê Ë Ì Í Î Ï Ñ Ò Ó Ô Õ Ö Ù Ú Û Ü Ý Ć Ĉ Ĝ Ģ Ĥ Ĩ Ĵ Ķ Ĺ Ļ Ń Ņ Ŕ Ŗ
  Ś Ŝ Ş Ţ Ũ Ŵ Ŷ Ÿ Ź Ǵ Ǹ Ȩ Ḑ Ḧ Ḩ Ḱ Ḿ Ṕ Ṽ Ẁ Ẃ Ẅ Ẍ Ẑ Ẽ Ỳ Ỹ

Not synthesised from the US font -- supply drawn glyph art with --packart:
  å ø æ ß ð þ œ ¡ ¿ · -- and every non-Latin script (Cyrillic, Greek: proven in game)
```

Uppercase accents outside item names: use the accepted convention of unaccented capitals (`Etat`,
`A bientot`).

## Language coverage, honestly

| status | languages |
|---|---|
| **full, no art** | Italian, Portuguese |
| **with standard substitutions, no art** | French (`œ`→`oe`), German (`ß`→`ss`), Spanish (`¡ ¿` not yet renderable), Catalan (`l·l` middle dot not yet renderable) |
| **needs pack art (`--packart`)** | Swedish / Danish / Norwegian / Icelandic (`å ø æ ð þ`), Polish (`ł ż ą ę`); every non-Latin script — **Cyrillic and Greek work today** once you draw their sheets, proven in game |
| **not possible** | CJK — thousands of characters against ~44 glyph slots, and 8×9 px is too small for a legible kanji |

**One script per pack.** Non-Latin packs reassign the letter byte-slots to a single drawn alphabet,
so a Latin word left in a Cyrillic pack would render as Cyrillic — the builder refuses it. You cannot
mix, say, Cyrillic and Greek in one pack; pick one script.

## Localized backgrounds

Some backgrounds have **text baked into the image** (title cards, signposts, map labels). A pack can
replace one with a translated version, using the **same content-hash swap as the HD pack** — the file
is a 1280×960 WebP named `<hash>.webp`, exactly the HD-pack convention. Priority is
**langpack > HD pack > retail**, so a translated background overrides the HD (untranslated) one.

**Two requirements to know up front:**
- **You need the HD pack.** It is where the hashes and reference images come from — download it, and
  its `backgrounds/<hash>.webp` filenames *are* the hashes, matched to the visuals you can look at.
- **They render only at internal scale ≥ 2** (the hi-res pass), like all background replacement. At
  scale 1× the player sees the original background. Translated backgrounds reach the hi-res crowd; a
  player who wants them keeps supersampling on.

**Workflow:**

```
# 1 — browse the HD pack's backgrounds/<hash>.webp; find the one whose baked-in text you want to
#     translate, and note its <hash> (the filename).
# 2 — redraw that image at 1280x960 with the text translated (your own upscaler, or the workflow in
#     platform/pc/tools/hdpack/vh_bg_restore.py), and save it as:
#         <work>/backgrounds/<hash>.webp
# 3 — validate (cross-checks the hash against the HD pack you point at):
lang_validate.py <disc> <work> --hdpack <hdpack-dir>
# 4 — build: the backgrounds are copied into the pack and listed in its manifest.
lang_build.py <disc> <work> <out> --lang <name>
```

The pack gains a `backgrounds/` folder beside `strings.bin`; the runtime resolves it before the HD
pack. `lang_validate --hdpack` checks each file is a valid WebP, exactly **1280×960**, with a 16-hex
hash filename that is a real background in that HD pack (a typo'd hash is otherwise a silent no-op).

## How validation works

Two layers:

- **`lang_build`** enforces the hard rules — record widths, encoding/byte safety, glyph capacity —
  and refuses to build a broken pack. `lang_validate` runs the same rules as a dry-run so you catch
  them before building.
- **`lang_validate`** additionally lints **what fits on screen**, counting rendered glyphs — control
  codes are free, `#N` insertions are measured at their referenced string's width. Over-budget lines
  are warnings by default, errors with `--strict`.

## Other tools

- `lang_probe.py` — builds a labelled test pack (one marker per text source) for engine verification.
- `en_audit.py` — scans the exported English for defects provable from the game's own data.
