# Troubleshooting

The most common problems, their causes, and their fixes. If yours isn't here, check
[known_issues.md](known_issues.md), then open an issue — the report template asks for the few
details that make a problem findable.

## The game won't start

**"Disc image not found."** The port needs your own *Vandal Hearts (USA)* disc dump as a raw
`.bin`. Put it in a `game/` folder next to the executable (or directly beside it), or set the path
explicitly with `VH_DISC_IMAGE` in `vandalhearts.ini`. The error message lists the exact path it
tried.

**"Wrong disc" error.** The port verifies the disc's boot signature before starting. If it refuses
your image, the image is not the USA release (`SLUS-00447`), or the dump is not a raw 2352-byte
`.bin`. Re-dump the disc as `.bin`/`.cue` and use the `.bin`.

**"Disc image is incomplete / corrupted" error — or, on v1.6.1 and older, the game starts but
hangs (window "not responding") right after the `[HD] pack detected` console line.** This is the
signature of a **damaged disc image**: usually an interrupted copy/download (truncated file) or a
bad rip (garbage sectors). Since **v1.6.2** the game detects this at startup or on first touching
the damaged data, and the error names the exact broken sector; older versions hung silently in the
disc reads — the HD banner was just the last thing printed before them, which is why toggling the
HD pack changed nothing. Two quick self-checks on your `.bin`:

- its **size must be an exact multiple of 2352 bytes** (a partial copy almost never is);
- one known-good dump of *Vandal Hearts (USA)* is **664,849,248 bytes** with SHA-256
  `4A8F984975775588B9ACE6B117895E4091A0F645B9678BD95FDA0EFFF3EBA561`. A different hash does **not**
  by itself mean your dump is bad — valid dumps vary by ripping tool — but combined with this
  symptom it is strong confirmation.

The only fix is a fresh copy: re-copy the file, or re-dump your disc.

**Windows: an error box appears before anything else, then nothing.** A runtime DLL is missing —
this failure happens before the game can log anything, which is how you recognize it. Re-extract
the **entire** release zip into one folder; the `.exe` needs all 8 DLLs next to it.

**Linux: the AppImage does nothing when launched.** The AppImage runtime needs FUSE2
(`fuse2`/`libfuse2` in most distros). Also make the file executable first:
`chmod +x VandalHearts-*.AppImage`.

## Display

**The picture looks soft or too small.** Set `WINDOW SCALE` (windowed) or `FULLSCREEN` in the
options overlay (**SELECT + START**). For sharper 3D, raise `INTERNAL RES` — see
[performance.md](performance.md) for what it costs.

**The window opened on the wrong monitor / wrong size.** Set `VH_SCALE` and `VH_FULLSCREEN` in
`vandalhearts.ini`; the window is also freely resizable and remembers your overlay choices.

## HD pack

**The HD PACK row is greyed out.** The row's value says why:

| Value | Meaning | Fix |
|---|---|---|
| `NO PACK` | no `hdpacks/` folder was found beside the executable | install the pack from the release page |
| `OUTDATED PACK` | the pack's manifest is an old version | download the current pack, or regenerate the manifest (`tools/hdpack/vh_hdpack_manifest.py`) |
| `WRONG GAME` | the pack was built for a different game version | use the pack for `SLUS-00447` (USA) |

**HD is ON but the current screen still looks native.** Enabling HD takes effect from the next
screen or background load; the screen you are on updates when it next reloads. Turning HD off is
immediate. This is by design — see [hd-pack.md](hd-pack.md).

**Backgrounds are HD but movies aren't (or the reverse).** The pack's startup line in the console
reports what it found, e.g. `[HD] pack detected + valid: ... (75 backgrounds, 16 videos)`. A
missing-FMV warning there means the pack's `videos/` folder is incomplete.

## Saves

**Where are my saves?** Ordinary files in `saves/` next to the executable (Tactical Mode uses its
own `saves_tactical/`). Copy the folder to move or back up progress — saves are
architecture-agnostic and work across machines and builds.

**I restored the wrong backup.** The restore flow's default is "back up then restore", so the card
you replaced was itself backed up first — restore that one.

## Reporting a bug

Run the game from a terminal (on Windows: from a console window) and reproduce the problem — the
console output usually names the failure. Attach those lines to your report, plus your
`vandalhearts.ini` if you changed it, and a save from `saves/` if the bug needs your progress.
Performance problems have their own diagnostics — see [performance.md](performance.md).
