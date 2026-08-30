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
# 1 — export (run all five into the SAME <work>)
lang_export.py           <disc> <work>     # executable tables + on-disc dialogue -> <work>/strings/
lang_export_literals.py  <src>  <work>     # text hardcoded in game code          -> strings/literals.json
lang_export_tactical.py  <bal>  <work>     # Tactical Mode text                   -> strings/tactical.json
lang_export_cues.py      <work>            # movie-subtitle templates             -> strings/cues/
lang_group.py            <work>            # entity-grouped translator views      -> <work>/translate/

# 2 — translate: edit <work>/translate/*.json and <work>/strings/dialogue/*.json
#     (every entry carries its English source and its display limit)
#     movie subtitles: edit <work>/strings/cues/*.json in place -- fill "text" next to each
#     "en" reference (empty text = that line keeps its burned-in English)

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
# 1 — export        (same five commands as above)
# 2 — translate      IN CAPITALS (movie subtitles may be written in natural mixed case --
#                    the builder folds them to your sheet's capitals automatically)
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
translation**. (Non-Latin packs are the exception — they must be complete; see the workflow above.
Movie subtitles are per-line diffs even there: an untranslated cue shows its burned-in English,
which stays readable — so subtitle completeness is never enforced.)

## Movie subtitles

The story videos (six chapter intros + the two endings) carry burned-in English narration. A pack
can subtitle them: the renderer paints an opaque black cover over the burned text region and draws
your translation on top, in the **large 16×15 font** at the original narration's own on-screen
size — identically with and without the HD pack. A line too long for the frame wraps to a second
row inside the band.

- `lang_export_cues.py <work>` installs the 8 templates into `<work>/strings/cues/`. Each cue is
  one narration line with its **`en` reference** and validated frame timing; you fill **`text`**
  and touch nothing else.
- **Empty `text` = that line keeps its burned-in English.** This is a per-line diff: translate the
  first video only and the rest stay English; leave the END2 credit slides alone (they are names
  and universally-understood roles — the shipped templates deliberately leave them out).
- Write subtitles in **natural mixed case** in any pack. On a capitals-only script sheet the
  builder folds them to capitals for you (real Unicode rules, so ß→SS and friends are right).
- Because subtitles draw the large font, a non-Latin pack that subtitles videos needs its
  `font16x15` sheet — `gen_packart` and the template tool produce it as a matter of course, and
  the build error names it if it's missing.
- The build gate guarantees coverage: a subtitle letter that appears nowhere else in your
  translation is added to the pack's font by codepoint (costing NO glyph slots), synthesised where
  possible, and a letter that cannot be drawn is a build error naming the file, cue, and
  character. `lang_template.py` counts cue letters too, so the art report is complete up front. It
  mirrors the builder's own folding rule when it counts them: a krom-synthesizable character
  (accented Latin) is counted as itself, and everything else as its Unicode uppercase, because the
  builder folds a glyphless letter before erroring and a script sheet is capitals-only -- so the
  report names exactly the art the build gate will demand.
- Timings and cover rects are maintainer-validated per frame against the retail videos —
  translators normally never edit them.

## Mixed case (`--mixed-case`, Latin only)

By default the game renders almost all text in ALL-CAPS — it folds `a`–`z` onto its uppercase
letterforms (only shop/field item names escape). Pass `--mixed-case` at build time and a Latin pack
renders text in the case you actually wrote (`Café déjà reçu`, not `CAFÉ DÉJÀ REÇU`), reusing the
font's own lowercase glyphs — no new art. It is the translator's decision, baked into the pack;
there is no player-facing toggle. Non-Latin packs set their own case in their drawn sheets and do
not use this flag.

## Item names: 16 characters (format 2)

Shop/field item names are the tightest budget in the game: the retail table stores them as
**8 two-byte characters**, drawn in the large font. A pack can opt into **1-byte names, 16
characters long**, drawn through the small font instead — the difference between `MEGAHERB` and a
real translation.

Opt in from the working set: in `<work>/strings/tables.json`, the `gItemNamesSjis` table's `limit`
block, change `"bytes_per_char": 2` to `1` (the export writes `2`, the retail encoding). From then
on its entries hold up to 16 characters; `lang_validate` and `lang_build` both enforce the new
width. The names render through the small font across the shop, depot, sell, party and battle item
lists (with a right-aligned price and the retail `#` markers kept); the tightest boxes truncate at
11–12 characters — the validator does not flag that, check those screens in game.

What it changes structurally:
- The pack's manifest declares **`"format": 2`**, and the whole re-encoded name table always ships
  with it. An older port build refuses a format-2 pack loudly and keeps English; a current build
  falls back to English names if the table is ever missing or damaged — never garbled text.
- In a **non-Latin** pack (`--packart`), 1-byte item names render from the **small** sheet like all
  other text — the large sheet then only matters for the few remaining large-font screens (the TURN
  counter, YES/NO prompts). The staging tip below still applies either way.
- Untranslated entries keep their original English name, re-encoded — a partial table is fine.

It is the translator's decision, baked into the pack, exactly like `--mixed-case`.

## What a pack does not cover

The port's own in-game options overlay (SELECT+START) stays English in every language. It is drawn
by the port with its own small font; keeping it out means every pack behaves the same regardless of
script. The END2 credit roll also stays English by design (see [Movie subtitles](#movie-subtitles)).

## The two fonts

The game uses **two fonts** and picks one per screen. A script it has never seen therefore needs its
alphabet drawn **twice**, once at each size. The sheets are independent files — you can ship one
before the other.

| | **small sheet — 8×9 px** | **large sheet — 16×15 px** |
|---|---|---|
| **where your letters show up** | story dialogue, every menu, item/spell descriptions, spell names, character names, class names, terrain names, battle messages | item names in shop/field/inventory; the TURN counter; YES/NO prompts; **movie subtitles** |
| **how much of the game** | almost all of it | a handful of screens |
| **letters you can add** | **44** | no practical limit |
| **if you don't supply it** | nothing readable renders — pack unusable | those screens stay English; the rest works |
| **drawing it** | very tight — plan on capitals only | roomier, easier per letter |

**You don't draw digits, punctuation or spaces** — those exist at both sizes and keep working.

(A **format-2** pack moves shop/field item names into the *small* font — see
[Item names: 16 characters](#item-names-16-characters-format-2). The table above describes the
retail, format-1 layout.)

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

**Two things to know up front:**
- **You — the pack author — need the HD pack.** It is where the hashes and reference images come
  from — download it, and its `backgrounds/<hash>.webp` filenames *are* the hashes, matched to the
  visuals you can look at. **Players need nothing extra**: the finished background ships inside your
  pack and shows even with the HD pack absent or toggled off.
- **They render at every internal scale.** At scale ≥ 2 the full 1280×960 art shows in the hi-res
  pass; at 1× the engine keeps a native-size shadow pass alive just for the pack, and the art is
  downscaled to native resolution — translated text stays readable, but keep it large enough that a
  4→1 downscale doesn't eat thin strokes. (HD-pack backgrounds without a langpack still need ≥ 2×.)

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

The dry-run build behind layer 1 needs `--packart` for a non-Latin working set — without it the
builder rejects every non-Latin string as a bad Latin one — and runs with `allow_incomplete` so an
unfinished script pack surfaces its other errors (glyph capacity, collisions, budgets) instead of
hard-failing on incompleteness, which is reported separately. Its internal pack name must itself
pass the naming check ("Pack naming"), or the dry run bails before a single hard rule runs.

## Other tools

- `lang_probe.py` — builds a labelled test pack (one marker per text source) for engine verification.
- `en_audit.py` — scans the exported English for defects provable from the game's own data.

---

# 3. Pipeline internals

Design facts a maintainer of the tools needs; translators can stop reading here. The runtime side
(the `strings.bin` format, glyph slots, hostile-input rules) is in
[`docs/language-packs.md`](../../../../docs/language-packs.md), "Developer reference".

## Scope: the US retail disc is the universe

Anything the US build never reaches does not exist for translation purposes — no PAL/JP
consideration, no "might be used". Six on-disc files still carry Japanese (`SIBAI5`, `SIBAI7`,
`SIBAIA`, `SIBAIE`, `SIBAIF`, `EVDEMO7`); they are exactly the six absent from `gEvtTextFiles[95]`
(the event → text-file map), and their only references in `src/` are the `gCdFiles[]` disc-table
entries in `core/cd.c`, which record every file's LBA — no `LoadText` call names them. Unreachable
on both counts, so the exporter drops them. The same rule governs code literals: a Japanese literal
is either **proven** dead (listed in `lang_export_literals.py`'s `DEAD` with its proof) or an
unresolved question for a maintainer that stops the export — never something handed to a translator
or dropped silently.

Retail text is never spelled out in the tools. Exceptions keyed to a specific line (dead literals,
dialogue lines proven not to render through the message box) are keyed by the FNV-1a hash of the
line's bytes, with a proof that says *where* the line is, not what it says.

## Text sources the draw-call sweep cannot see

`lang_export_literals.py` sweeps `src/` for string literals passed straight to a draw call. Three
kinds of text are reachable by a pack but invisible to that sweep, so they are curated by hand and
guarded by a self-check that fails the export if one disappears or grows:

- **Composed prefixes** — a literal built into a buffer before drawing (`"You got "` + a number,
  `"TURN"` + a number), wrapped in `PC_LANGSTR` at the composition site (`WRAPPED_LITERAL_FILES`).
- **Arrays of strings** — the title screen, memory-card prompts, the party list. The code that
  consumes the array is wrapped, and `PC_LangStr` hashes whatever it is handed at run time, so one
  wrap covers every entry (`ARRAY_SOURCES`).
- **A string held in a char array** reached only through a pointer (`sEmptyFileCaption`), which
  neither the literal sweep nor the pointer-array scan matches (`CHAR_ARRAY_LITERALS`).

Three name tables — `gUnitTypeNames`, `gItemNames`, `gClassAdvancementNames` — draw through a third
mechanism, `StringToGlyphs` into a sprite glyph strip, neither `DrawText` nor `DrawSjisText`.
`gItemNames` is a second, longer item-name table (139 entries against `gItemNamesSjis`'s 101): the
equip/status panel uses it, the shop and field use the Shift-JIS one.

## On-screen budgets

Budgets come from the real call sites in `src/`, and the disc is the oracle: a rule that fails
retail's own shipped text is the rule being wrong.

- **Pointer tables** — the column budget is the 3rd argument of `DrawText` at each call site.
  `DrawText_Internal` **wraps** at the budget (column resets, row++) and does not clip, so exceeding
  it costs extra rows and risks overflowing the window vertically, not losing characters.
  `gStringTable` 20 columns at all its call sites; `gSpellDescriptions` and `gItemDescriptions`
  35 (a 288×36 bar); `gItemDescriptions2` 29 (the safe minimum of 29–35 across its screens).
- **Dialogue files** — stored bitwise-inverted with CRLF lines; `LoadText` walks them as up to 100
  entries, a blank line toggling entry start/end and `END` terminating. `SHOP_T` draws through
  `DrawText` in `ui/supplies.c` at 30 columns and wraps; every other file goes through the message
  box, which **hard-clips** at 26 columns (the tail is silently lost). Entry 1 of every battle file is
  the victory/defeat condition panel, drawn by `battle/field.c` with `DrawText` at 40 and 34 columns
  (wrapping) — it is budgeted at the tighter of the two, not the message box's 26.
- **`gText`** — `LoadText` unpacks a whole file into one shared 10928-byte buffer (`symbol_addrs.txt`:
  `gText` size `0x2ab0`), a second budget independent of the on-disc size. The builder measures it by
  simulating `LoadText` on the patched bytes, so it charges the encoding a script pack really writes
  (1-byte codes, not UTF-8 lengths).
- **Fixed tables** — the record width, with the padding read off the disc rather than assumed:
  character and spell names are plain ASCII padded with NUL; item names are full-width Shift-JIS
  padded with `0x8140`. Each table's slot 0 is an all-filler "empty" record, which is why unedited
  records are kept verbatim rather than re-synthesised. `terrainText`'s column alignment is literal
  spaces inside the string (`"Plains   0%"` vs `"Thicket 15%"`) — never trim or normalise it.

## How glyph synthesis works

Accented Latin is composed from the disc's own letterforms: the US font holds lowercase `a`–`z` at
glyph indices 13–38 and uppercase at 68–93, and a mark is a few pixels OR'd into rows the base
letter leaves blank. In the 8×9 font marks occupy rows 0–1, which lowercase leaves empty;
**uppercase ink starts at row 1**, so uppercase accents cannot be synthesised at 8×9 and are
reported as "needs pack art" rather than merged into the letter. Cedilla uses row 8, blank in every
US letterform. An above-mark on `i`/`j` **replaces** the dot (`î` is dotless-i + circumflex): rows
0–3 are cleared and the mark seats one row lower so it does not float over the empty dot row.

The 16×15 krom font (item names, the TURN banner, the dojo YES/NO) has room to shift: an uppercase
letterform moves down one row to free the top for the mark, so that path supports true uppercase
accents. Base letterforms come from the BIOS charset parsed out of `pc_kanji_font.c`.

## Fixed-width tables and the charmap

Fixed records keep byte = char = column, so a non-ASCII character there gets a free 1-byte **pack
code** assigned by the builder; the retail code→glyph map is rewritten so that code names a free
glyph slot, and the synthesised bitmap is written into that slot — all through the `K_CHARMAP`
section and `core/text.c`'s hand-off hook. The tables that carry pack codes are `gSpellNames`,
`gClassAdvancementNames`, `terrainText`, `gCharacterNames`, `gUnitTypeNames` and `gItemNames`; the
same records feed both the bitmap store and the glyph-sheet cells the strip path blits from. Pointer
strings and dialogue carry real UTF-8 instead: the engine draws any codepoint the pack ships a glyph
for, with no index space and no character-count ceiling. The slot pool, the code pool and script
mode are described in the runtime reference.

`lang_validate` mirrors the builder's rules rather than importing its results: the format-2 record
width (16 one-byte chars when `bytes_per_char` is 1), the cue-letter case folding, and the `#N`
cycle check all exist so that validate and build can never disagree.
