# CD-ROM and XA audio (libcd)

The CD backend replaces PsyQ's `libcd` — the interface the game uses to load files from the disc,
stream FMV/XA audio, and (via the deferred `DecDCT*`/`St*` entry points) drive movie playback. On
real hardware these calls talk to the PlayStation's CD-ROM controller, which reads 2352-byte sectors
off the spinning disc, decodes interleaved XA-ADPCM audio in hardware, and hands MDEC bitstreams to
the video decoder. On PC there is no drive: the backend reads a **raw disc image file** (`.bin`)
directly and reproduces the parts of that behaviour the game depends on.

Two source files implement it:

- `platform/pc/src/libcd.c` — the `libcd` entry points: mount, sector addressing, the `CdControl`
  command interface, `CdRead`/`CdReadSync` file loads, the CD-timing simulation, and the movie/XA
  demux pump.
- `platform/pc/src/pc_xa.c` — the CD-XA ADPCM decoder and its OpenAL streaming source.

The clean-room header is `platform/pc/include/PsyQ/libcd.h`.

## The disc image model

The disc image is a **raw 2352-byte-per-sector dump** (`Vandal Hearts (USA).bin`, e.g. produced by
`chdman extractcd` from a CHD). Unlike a 2048-byte "cooked" `.iso`, each raw sector carries the full
CD sync/header/subheader/EDC framing around its user data. The backend reads whole raw sectors and
picks the payload out of each one:

```
#define SECTOR_RAW_SIZE     2352   /* one raw CD sector on disc              */
#define SECTOR_DATA_OFFSET  24     /* user data starts 24 bytes in          */
#define SECTOR_DATA_SIZE    2048   /* Mode-2/Form-1 user data per sector     */
```

Sector addressing was verified against the actual disc (2026-07-10):

- The game's baked-in file positions (`gCdFiles[].startingSector`) are **plain ISO9660 LBAs**.
  Confirmed by cross-checking `gCdFiles[CDF_SIBAI1_1_DAT] == 0x27e8` against the real ISO9660
  directory entry for `SIBAI1_1.DAT` (LBA 10216 == 0x27e8).
- This project's extracted `.bin` has **zero pregap**, so raw sector *N* in the file *is* LBA *N*.
  The byte offset of a sector's data is therefore `N * 2352 + 24`.
- This was pinned down by locating `SLUS_004.47`'s **`"PS-X EXE"` header at exactly LBA 23**,
  matching that executable's own ISO9660 directory entry. That same fact doubles as a cheap
  wrong-disc signature check (see [validation](#disc-mounting-auto-detect-and-validation)).

The game addresses sectors in BCD **MSF** (minute/second/frame) form via `CdlLOC`. `CdIntToPos` and
`CdControl(CdlSetloc)` apply the standard Red Book 150-sector (`00:02:00`) pregap offset
symmetrically (`#define MSF_PREGAP_SECTORS 150`), so an LBA converted to MSF and back round-trips to
the original LBA. Internally the backend works in LBAs and only converts at the `CdlLOC` boundary.

## The CdControl command interface

Real `libcd` is asynchronous: `CdControl` issues a drive command and returns immediately (`1` =
accepted, `0` = not accepted / retry), and the caller polls `CdSync`/`CdReadSync` for completion.
The PC backend keeps that contract — several game state machines depend on the exact
accepted/rejected semantics — but performs the underlying data transfer synchronously (a local file
has no I/O latency to hide) and only *paces when completion is reported* (see
[CD-timing](#cd-timing-simulation)).

`CdControl` (`libcd.c`) services exactly the commands the game issues. Their opcodes come from
`libcd.h`:

| Command | Opcode | What the backend does |
|---|---|---|
| `CdlNop` | `0x01` | Accepted, no-op. |
| `CdlSetloc` | `0x02` | Records the target LBA (`CdPosToLBA(param)`). No seek happens here — the seek time is folded into the next `CdRead`, matching octoshock's `ReadBase()`. |
| `CdlSetmode` | `0x0e` | Stores the mode byte (`CdlModeSpeed` 1x/2x, `CdlModeRT` real-time XA). |
| `CdlReadN` | `0x06` | In RT mode, begins/continues XA-ADPCM streaming from the seeked LBA. |
| `CdlSeekL` | `0x15` | In RT mode, repositions the XA cursor but stays **silent** (audio only flows at the following `CdlReadN` — see [gotchas](#gotchas--notes)). |
| `CdlSetfilter` | `0x0d` | Selects the XA `{file, channel}` to demux (`CdlFILTER`). |
| `CdlPause` | `0x09` | Soft-pause: stop feeding new sectors but keep the OpenAL source, its queued audio, and the ADPCM history, so a same-track resume is seamless. A movie pause additionally flushes. |
| `CdlReset` | `0x0a` | Hard stop: tear down the XA stream and any movie overlay. |

Regular file loads (`cd.c`'s `LoadCdFile`/`ContinueLoadingCdFile`) go through `CdRead(sectors, buf,
mode)` → `CdReadSync`. `CdRead` reads the requested run of sectors straight out of the `.bin` at the
`CdlSetloc` target (copying only the 2048 data bytes of each raw sector into `buf`), then schedules a
completion time. `CdReadSync` returns a positive "sectors remaining" estimate while that time is in
the future and the final result once it elapses — mirroring real hardware's positive-while-busy
return that callers test with `> 0`.

`CdControlB` is a synchronous alias for `CdControl`. `CdMix` (CD→SPU volume) is accepted as a no-op;
the XA path controls its own OpenAL gain instead.

## CD-timing simulation

`CdRead` could complete instantly — the data is already in a local file — but that turned out to be
wrong in a load-bearing way. With instant loads the battle-entry loading screen ran ~7.6x faster than
real hardware, and several battle-start camera/pan effects that interpolate *during* the load ended
up far short of where the real hardware's equivalent moment leaves them once the scene becomes
visible. Loading pacing is also tied to RNG determinism elsewhere in the port.

So `CdRead`/`CdReadSync` schedule completion against `SDL_GetTicks()` wall-clock time using a model
**ported from BizHawk's octoshock PS1 core** (`psx/octoshock/psx/cdc.cpp`, `PS_CDC::CalcSeekTime()`)
— the only one of BizHawk's two PS1 cores that models drive timing at all. `CalcSeekTimeMs`
reproduces octoshock's formula in milliseconds (their 33.8688 MHz cycle unit divided out, since this
backend paces against the same wall clock `VSync` uses, not a cycle-exact clock):

- a piecewise-linear seek time by LBA distance, with bucketed bonuses for big (`>= 2250` sectors) and
  small (`[3, 12)` sectors) jumps;
- a flat 75-or-150 sectors/sec transfer rate (1x/2x, keyed on `CdlModeSpeed`);
- a small jitter term drawn from a **private** PRNG (a fixed-seed LCG), deliberately *not* the game's
  shared `rand()` — sharing that stream injected an extra draw on every `CdRead` and desynced the LCG
  from real hardware.

The simulated laser-head position (`s_headLBA`) is tracked separately from the `CdlSetloc` target
(`s_targetLBA`), so sequential file reads correctly incur no seek penalty. Simplifications versus
octoshock: no motor-off/re-spin-up penalty beyond a one-time cost on this process's first-ever
`CdRead`, and no pause/resume bonus (this game's access pattern is plain sequential file loads).

Result: the battle-entry loading screen now takes **~5.36s**, matching real hardware's ~5.08s (it was
~0.67s before). This does **not** change any post-loading behaviour — nothing advances the per-frame
object loop during a loading screen, so a realistically slower load lets nothing "catch up".

## XA audio streaming

CD-XA carries **interleaved ADPCM audio** in the sector stream. The game (`src/audio.c`) requests it
via `PerformAudioCommand(PLAY_XA/PREPARE_XA)`, which drives a `CdlSetmode(RT)` →
`CdlSetfilter(file,chan)` → `CdlSeekL` → `CdlReadN` sequence. On hardware the CD controller decodes
the matching sectors and mixes them into the SPU's CD input; the PC backend reproduces that with a
software decoder feeding OpenAL:

- `libcd.c`'s **sector pump** `PC_CdXaUpdate()` runs once per `VSync`. While streaming, it reads raw
  sectors from the `.bin` at the stream cursor and hands each to `PC_XaSubmitSector()`, flow-
  controlled by the OpenAL queue depth so it never over-reads (drops) or under-feeds (underruns).
- `pc_xa.c` decodes each sector that matches the active `{file, channel, audio}` filter. It follows
  the standard CD-XA sound-group layout (18 groups × 128 bytes of user data; 4-bit or 8-bit, mono or
  stereo; 37800 or 18900 Hz from the coding-info byte) and the public PS1 ADPCM coefficient pairs
  (the same table `libsnd.c` uses). ADPCM history is continuous across sectors per channel.
- Decoded PCM is queued on a dedicated OpenAL streaming source (24-buffer pool). OpenAL owns the
  realtime mixing thread, so the feed is a cheap per-`VSync` top-up (`PC_XaService`) with no locking.
  Serial volume (`SsSetSerialVol`) maps to the source's `AL_GAIN`.

**Scope** (validated against hardware with a BizHawk track map): XA carries the
**intro/FMV audio** and the **streamed spell SFX** — notably the big elemental spells
(Hurricane/Salamander/Avalanche/Plasma-Wave, XA files 17/18/19/22/23/33). There are **zero looping XA
tracks** in the game, so this layer only ever streams finite one-shots.

> **The game's background music is SEQ, not XA.** The loading/battle BGM is sequenced music
> (`battle_eval.c` `PlayBattleBGM` → `AUDIO_CMD_PLAY_SEQ` → SPU voices), an entirely separate
> subsystem. If music is silent, it is never an XA problem. See [spu.md](spu.md).

Movie (STR) playback shares this machinery: `CdRead2(Stream|RT)` (issued by `Movie_Start`) starts a
movie's interleaved XA audio and seeds the video demux, the `St*` ring API and `StGetNext` pace the
frame counter, and `pc_mdec.c` decodes the video sectors — see [mdec.md](mdec.md).

## Disc mounting, auto-detect and validation

Mounting happens at startup in `pc_bootstrap.c`'s constructor. The disc path is resolved in
precedence order:

1. **`VH_DISC_IMAGE`** environment variable (or `vandalhearts.ini`) — an explicit override.
2. **`DefaultDiscPath()`** auto-detect, anchored to the executable's own directory (via
   `/proc/self/exe` / `GetModuleFileNameA` / `_NSGetExecutablePath`, so it is cwd-independent):
   1. a **`game/` folder next to the executable** containing a `*.bin` — the recommended
      portable-binary layout;
   2. a **`*.bin` sitting directly beside the executable**;
   3. the **dev repo layout** (`game/` four levels up from `platform/pc/build`).

`PC_CdMount` opens the file; whether it opened is separate from whether its *content* is valid.
Validation is `PC_CdDiscSignatureOk()` in `libcd.c`: it reads LBA 23 and checks for the `"PS-X EXE"`
magic at `SECTOR_DATA_OFFSET` (24). Because the ISO layout fixes `SLUS_004.47` at LBA 23, this passes
for any genuine Vandal Hearts (USA) `.bin` and cheaply rejects the common mistakes — a different
game, the wrong region, a `.cue`/`.iso`/`.mp3` picked by accident, or a truncated image — for the
price of one sector read. (It is a signature check, not an exact-region MD5, so valid alternate dumps
are not rejected.)

Either failure — file won't open, or signature check fails — is **fatal**. `PC_FatalDiscError` prints
the tried path plus a hint to stderr, shows a native **message box on Windows** (`MessageBoxA`), and
exits, rather than booting into garbage with no disc data (every `CdRead` failing, nothing rendering).

## Gotchas / notes

- **`.bin`, not `.cue`/`.iso`.** The backend needs a raw 2352-byte/sector image. A cooked 2048-byte
  `.iso` has no sector framing (the `SECTOR_DATA_OFFSET` math and the XA subheaders both break), and a
  `.cue` is only a text sheet. Both are rejected by the signature check.
- **XA audio must start at `CdlReadN`, not `CdlSeekL`.** The game times spell animations to the
  `CdlReadN` (which fires ~2.7s after the `CdlSeekL`, after the drive's physical seek would complete),
  capturing `gXaStartTime` there. Feeding audio at `CdlSeekL` made every spell SFX start ~2.7s early
  and finish before impact ("Avalanche ends at the apex"). The fix: `CdlSeekL` only repositions the
  cursor and stays silent; `CdlReadN` begins the stream.
- **Read *through* non-matching sectors after a seek.** Some tracks' `{file,chan}` data begins
  hundreds of sectors past their seek LBA, inside a shared multi-song interleave (e.g. the epilogue
  ocarina). The end-of-track miss-limit is only applied *after* the first matching sector is seen;
  before that, a bounded pre-match scan reads through, mirroring hardware.
- **`CdControl` must accept the XA commands.** Early on, `CdControl` fell through to `return 0` for
  `CdlSetfilter`/`CdlSeekL`/`CdlReadN`. The game reads `CdControl()==0` as "not accepted, retry", so
  its XA state machine looped forever, jamming the audio job queue and hanging `WaitForAudio()`
  deterministically when the demo tried to return to the menu. Accepting these commands (`return 1`)
  fixed the hang; `CdSync` was also corrected to actually write its status-result buffer instead of
  leaving callers reading stale memory.
- **MDEC video** decode itself lives in `pc_mdec.c`; the `DecDCT*` entry points here are thin
  stubs/no-ops (`DecDCTout` zeroes its buffer so an undecoded region shows clean black rather than VRAM
  garbage). See [mdec.md](mdec.md).
- **Debug logging** is env-gated and off by default: `VH_XA_LOG=1` logs CD commands to
  `vh_xa_log.txt`; `VH_XA_CSV=1` writes a per-`VSync` `vh_xa_ours.csv` for diffing against the
  BizHawk track-map capture. For isolating XA from the rest of the mix, `VH_SPU_GAIN=0` mutes SPU
  (SEQ + VAG) leaving only XA, and `VH_SEQ_MUTE=1` mutes only SEQ music.

## See also

- [spu.md](spu.md) — SEQ music, VAG SFX, the software SPU (where the game's *music* lives).
- [mdec.md](mdec.md) — MDEC/STR video decode (shares this file's demux and XA-audio pump).
- [../../configuration.md](../../configuration.md) — `VH_DISC_IMAGE` and other runtime options.
- [../../architecture.md](../../architecture.md) — the swappable-interface layer these backends fit into.
