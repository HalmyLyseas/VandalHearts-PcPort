# CD-ROM and XA audio (libcd)

The CD backend replaces PsyQ's `libcd` — the interface the game uses to load files from the disc,
stream FMV/XA audio, and (via the `DecDCT*`/`St*` entry points) drive movie playback. On real
hardware these calls talk to the PlayStation's CD-ROM controller, which reads 2352-byte sectors
off the spinning disc, decodes interleaved XA-ADPCM audio in hardware, and hands MDEC bitstreams to
the video decoder. On PC there is no drive: the backend reads a **raw disc image file** (`.bin`)
directly and reproduces the parts of that behaviour the game depends on.

Two source files implement it:

- `platform/pc/src/libcd.c` — the `libcd` entry points: mount, sector addressing, the `CdControl`
  command interface, `CdRead`/`CdReadSync` file loads, the CD-timing simulation, and the movie/XA
  demux pump.
- `platform/pc/src/pc_xa.c` — the CD-XA ADPCM decoder and its OpenAL streaming source.

The clean-room headers are `platform/pc/include/PsyQ/libcd.h` and `libpress.h` (the latter forwards
the shared `DecDCT*` declarations to `libcd.h` and adds only the extra `libpress` entry points).

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

Sector addressing, verified against the actual disc:

- The game's baked-in file positions (`gCdFiles[].startingSector`) are **plain ISO9660 LBAs**.
  Confirmed by cross-checking `gCdFiles[CDF_SIBAI1_1_DAT] == 0x27e8` against the real ISO9660
  directory entry for `SIBAI1_1.DAT` (LBA 10216 == 0x27e8). `platform/pc/src/test_libcd.c` is a
  standalone check that reads that file through the game's own `CdlSetloc`+`CdRead` flow and
  compares the bytes against an independently-extracted copy.
- The extracted `.bin` has **zero pregap**, so raw sector *N* in the file *is* LBA *N*. The byte
  offset of a sector's data is therefore `N * 2352 + 24`.
- This is pinned down by `SLUS_004.47`'s **`"PS-X EXE"` header sitting at exactly LBA 23**,
  matching that executable's own ISO9660 directory entry. That same fact doubles as a cheap
  wrong-disc signature check (see [validation](#disc-mounting-auto-detect-and-validation)).

The game addresses sectors in BCD **MSF** (minute/second/frame) form via `CdlLOC`. `CdIntToPos` and
`CdControl(CdlSetloc)` apply the standard Red Book 150-sector (`00:02:00`) pregap offset
symmetrically (`#define MSF_PREGAP_SECTORS 150`), so an LBA converted to MSF and back round-trips to
the original LBA. Internally the backend works in LBAs and only converts at the `CdlLOC` boundary.

### Region boot signatures and disc classification

Each region's boot executable is pinned at a fixed LBA by its ISO layout, and the `"PS-X EXE"` magic
is at data offset 0 of that sector:

| Region | Boot exe | Boot LBA | Image size |
|---|---|---|---|
| USA / Asia (SLUS-00447 / SCPS-45183, one shared master) | `SLUS_004.47` | 23 | — |
| Japan | `SLPM_860.07` | 15200 | 283,860 sectors |

`PC_CdDiscSignatureOk()` probes the built region's boot LBA (`VH_REGION_BOOT_LBA`,
`pc_platform.h`). `PC_CdDiscRelease()` goes further and **names** the disc so a wrong-region image
gets a specific diagnostic: it reads the memory-card save-file id embedded in each boot exe, which
is the canonical release tag —

| Id | Release | VRAM address in the exe |
|---|---|---|
| `BASLUS-00447VH` | USA | `0x800f5551` (in `SLUS_004.47`) |
| `BISCPS-45183VH` | Asia | `0x800f5551` (same master, only the id differs) |
| `BISLPM-86007VH` | Japan | `0x800f76a9` (in `SLPM_860.07`) |

The exe loads at VRAM `0x80010000` behind a `0x800`-byte header and is stored in contiguous
2048-byte ISO sectors, so each id sits at a fixed raw-image offset (`boot LBA * 2352 + 24 +
(vram - 0x80010000 + 0x800)`, split across the sector boundary arithmetic; neither id straddles a
sector). Both regions' layouts are probed regardless of which region was built.

### Sector-integrity guards

A damaged image (a zeroed span, a shifted/spliced copy, a truncated file) can pass the one-sector
boot signature and then hang the game silently: garbage loads as garbage and the loader state
machines spin forever, and reads past a truncated image's end fail in a retry loop the game never
leaves. The backend therefore checks every sector it reads:

- every raw sector starts with the 12-byte **sync pattern** `00 FF×10 00`;
- bytes 12–14 carry the sector's **own BCD MSF address**, which must equal the LBA sought
  (`lba + 150`) — this catches shifted or spliced images.

A mismatch, or EOF on a game-file (`CdRead`) or movie-video read, is **fatal** via
`CdFatalCorrupt()` → `PC_FatalDiscError`, naming the sector and telling the user to re-dump (an
image must be an exact multiple of 2352 bytes; `PC_CdImageBytes()` is the bootstrap's truncation
gate). The XA pump treats EOF as a graceful end-of-track — it legitimately scans toward track ends
— except while a movie's audio stream has not yet matched a single sector: a movie's XA is its
frame clock, and a stream lying entirely past the image's end would make `Movie_SyncFrame` wait
forever, so that case is fatal too.

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
| `CdlPause` | `0x09` | Soft-pause (see [pause semantics](#pause-semantics)). |
| `CdlReset` | `0x0a` | Hard stop: tear down the XA stream and any movie overlay. |

`CdlSeekL`/`CdlReadN` honour a `CdlLOC` passed directly as `param`: `core/audio.c`'s XA path calls
`CdControl(CdlSeekL, &gXaCdlLOC)` with **no preceding `CdlSetloc`**, so ignoring the parameter would
seek to a stale target. A `NULL` param means "use the last `CdlSetloc`" (the file-load path).

Regular file loads (`core/cd.c`'s `LoadCdFile`/`ContinueLoadingCdFile`) go through `CdRead(sectors, buf,
mode)` → `CdReadSync`. `CdRead` reads the requested run of sectors straight out of the `.bin` at the
`CdlSetloc` target (copying only the 2048 data bytes of each raw sector into `buf`), then schedules a
completion time. `CdReadSync` returns a positive "sectors remaining" estimate while that time is in
the future and the final result once it elapses — mirroring real hardware's positive-while-busy
return that callers test with `> 0`.

`CdSync` writes `result[0] = 0` when a result buffer is given: callers read status bits out of it
(`AudioJob_PlayXa` checks `CdlStatSeek`), and since the backend has no background seek state,
"idle, no flags" is the correct value rather than whatever the buffer held.

`CdControlB` is a synchronous alias for `CdControl`. `CdMix` (CD→SPU volume) is accepted as a no-op;
the XA path controls its own OpenAL gain instead. `CdReadyCallback` returns 0 without delivering
callbacks — `CdRead` has already completed by the time it returns.

### The language-pack hook

`CdRead` calls `PC_LangPatchRead(lba, sectors, buf)` (`pc_lang.c`) after the copy. A translated
on-disc text file is substituted there, keyed by the read's LBA: `core/cd.c` reads each text file
whole in one `CdRead` from `gCdFiles[cdf].startingSector`, so the substitution is indistinguishable
from the disc having held those bytes and the game parses them with its own unmodified `LoadText`.
Without a pack it is a no-op.

## CD-timing simulation

`CdRead` could complete instantly — the data is already in a local file — but instant loads are
wrong in a load-bearing way. With them the battle-entry loading screen runs ~7.6× faster than real
hardware, and several battle-start camera/pan effects that interpolate *during* the load end up far
short of where the real hardware's equivalent moment leaves them once the scene becomes visible.
Loading pacing is also tied to RNG determinism elsewhere in the port.

So `CdRead`/`CdReadSync` schedule completion against `SDL_GetTicks()` wall-clock time. The model is
composed of three parts: the octoshock seek/transfer model, a per-read start overhead, and (at boot
only) a scale factor.

### The seek/transfer model

Ported from BizHawk's octoshock PS1 core (`psx/octoshock/psx/cdc.cpp`, `PS_CDC::CalcSeekTime()`) —
the only one of BizHawk's two PS1 cores that models drive timing at all (mednadisc's CD layer is
plain file I/O). Octoshock's model is itself a hand-tuned empirical approximation of real hardware,
not a physical seek-curve derivation. `CalcSeekTimeMs` reproduces it in milliseconds (their
33,868,800 Hz cycle unit divided out, since this backend paces against the same wall clock `VSync`
uses, not a cycle-exact clock):

| Term | Value | Origin |
|---|---|---|
| Base seek | `abs(from - to) * 1000 / (72 * 60 * 75)` ms | octoshock's coarse "72×" seek-speed assumption |
| Seek floor | 20,000 cycles ≈ 0.59 ms | octoshock |
| Big jump | `>= 2250` sectors of travel: **+300 ms** | octoshock |
| Small jump | `[3, 12)` sectors of travel: **+4 sector-times** | octoshock |
| Jitter | `0 .. 25,000` cycles ≈ up to 0.74 ms | octoshock's `PSX_GetRandU32(0, 25000)` |
| Transfer | 75 or 150 sectors/s (1×/2×, keyed on `CdlModeSpeed`) | Red Book / CD-ROM 2× |
| Motor start-up | **+1000 ms**, once, on the process's first `CdRead` | disc spin-up |

The jitter term draws from a **private** fixed-seed LCG (`SeekJitterRand`), deliberately *not* the
game's shared `rand()`: seek jitter is mechanical variance that never touches the game's software
RNG, and sharing that stream adds an extra draw on every `CdRead` — many per boot/menu/loading
sequence — fully desyncing the LCG from real hardware's sequence before any battle logic runs.

The simulated laser-head position (`s_headLBA`) is tracked separately from the `CdlSetloc` target
(`s_targetLBA`), so sequential file reads correctly incur no seek penalty. Simplifications versus
octoshock: no motor-off/re-spin-up penalty beyond the one-time start-up cost, and no pause/resume
bonus (this game's access pattern is plain sequential file loads).

### Per-read start overhead

The octoshock model charges seek + transfer only. The real drive also pays a pause→ReadN **start
cost on every read** (command processing plus laser re-sync after the stream stops). Whether that
cost is visible depends on the loader:

- the **US loader** runs a `Setmode` + `CdSync` + 3-frame dance between files
  (`src/core/cd.c:958-992`), ~84–139 ms of game-side idle per file, which overlaps the drive's start
  work — so with no start charge the US port already lands within −5% of hardware;
- the **JP loader** issues `Setloc` → `CdRead` back-to-back (`jp/src/core/cd.c`; ~3–31 ms idle),
  serialising the cost on hardware — with no start charge the JP battle load runs ~1 s (−18%) short
  of its BizHawk baseline.

The model: the drive needs `READ_START_MS` (**145 ms**) after the previous read's completion before
the next can stream, and game-side idle counts toward it (that is the overlap). `CdRead` charges
`READ_START_MS − idle` when idle is shorter. The constant is fitted against both regions' BizHawk
baselines from natural-run logs: the JP 8-read battle-load window goes from 4.24 s to **5.33 s vs
BizHawk's 5.33 s** (the window's first read is charged too), and the same region-blind charge lands
the US load at its baseline as well.

### The mode argument

Real PsyQ `CdRead` (disassembled from `SLUS_004.47`: wrapper `0x800d25a8`, read kick `0x800d3da0`)
compares its `mode` argument's low byte against the drive-mode shadow (`0x80120f64`, updated by every
`CdlSetmode`) and issues `CdlSetmode(arg)` on mismatch before `CdlReadN`. The JP game depends on
this: it passes `CdlModeSpeed` per read with **no** separate `Setmode` step (`jp/src/core/cd.c:948`),
so ignoring the argument would run every JP load at 1× where hardware runs 2×.

**Deliberate deviation for `mode == 0`:** the US game does `CdlSetmode(0x80)` then `CdRead(..., 0)`
(`src/core/cd.c:966/992`). Applying a zero mode literally would drop the US drive to 1× mid-load,
which contradicts measurement — retail US hardware loads the demo battle in **~5.25 s** (BizHawk
video capture) versus ~4.98 s for the port with the seek/transfer model alone, i.e. a 2×-class
transfer either way. So a zero mode keeps the current mode; the behaviour is faithful in outcome.

### Boot-grace fast loads

A fresh process boot spends 5–8 s in the timing model loading the boot files (`LoadSoundSet` etc.)
behind a black window — authentic mechanics, but on a console those seconds hide behind the BIOS
boot animation. From process start until the **first movie stream** begins (the intro logo; a
one-way latch cleared where `CdRead2` sets `s_movieActive = 1`), the composed delay is scaled down
**16×** and the start-overhead charge is skipped. Delays stay non-zero, so completion still lands on
a later `VSync` (the async-ordering contract).

This scope is provably safe: boot loads run no engine frames (`rand()` never ticks, so the RNG
stream is untouched) and no XA/FMV stream can be active before the first movie by construction.
Every load from the logo onward — title, New Game, area transitions, the demo battle — pays the
hardware-exact model. A START mash cannot outrun the latch (the stream starts in movie-state case
10; the skip input is read in case 11). Caveat: a `VH_DEBUG_MENU=1` boot never plays a movie, so
that dev path keeps fast loads for the whole session. `VH_FAST_BOOT=0` (ini/env) restores full
hardware timing from the first read, for A/B validation.

### CD load accounting

`VH_CD_LOG=1` writes `vh_cd_log.txt`: one line per `CdRead` with sector count, speed, seek / start /
transfer milliseconds and running totals, stamped with wall-clock ms since startup so a
battle-load window can be isolated from boot/movie reads and compared against a hardware baseline.
It is read-only instrumentation and separates "fewer sectors" from "mis-charged time" when a load
differs from hardware.

## XA audio streaming

CD-XA carries **interleaved ADPCM audio** in the sector stream. The game (`src/core/audio.c`) requests it
via `PerformAudioCommand(PLAY_XA/PREPARE_XA)`, which drives a `CdlSetmode(RT)` →
`CdlSetfilter(file,chan)` → `CdlSeekL` → `CdlReadN` sequence. On hardware the CD controller decodes
the matching sectors and mixes them into the SPU's CD input; the PC backend reproduces that with a
software decoder feeding OpenAL:

- `libcd.c`'s **sector pump** `PC_CdXaUpdate()` runs once per `VSync` (from `libetc.c`). While
  streaming, it reads raw sectors from the `.bin` at the stream cursor and hands each to
  `PC_XaSubmitSector()`, flow-controlled by the OpenAL queue depth so it never over-reads (drops) or
  under-feeds (underruns).
- `pc_xa.c` decodes each sector that matches the active `{file, channel, audio}` filter (layout
  below) using the public PS1 ADPCM coefficient pairs (the same table `libsnd.c` uses). ADPCM
  history is continuous across sectors per channel.
- Decoded PCM is queued on a dedicated OpenAL streaming source (24-buffer pool, ~1.3 s at stereo).
  OpenAL owns the realtime mixing thread, so the feed is a cheap per-`VSync` top-up
  (`PC_XaService`) with no locking. Serial volume (`SsSetSerialVol`) maps to the source's `AL_GAIN`
  (average of the two 0..127 sides → 0..1; 0.8 until the game sets it).

### CD-XA sector layout

From psx-spx `cdromformat.md` and the standard sound-group format:

- **Subheader** at byte 16: `file`, `channel`, `submode` (bit 2 = `0x04` audio), `coding-info`
  (bits 0–1 mono/stereo, 1 = stereo; bit 2 rate, 0 = 37800 Hz, 1 = 18900 Hz; bits 4–5 bits/sample,
  0 = 4-bit, 1 = 8-bit).
- **User data** at byte 24: 2304 bytes = 18 sound groups × 128 bytes. Each group is 16 parameter
  bytes + 112 data bytes (28 data-words × 4 bytes).
  - 4-bit: 8 blocks per group; block *b*'s parameter byte is `group[4 | (b & 3) | ((b & 4) << 1)]`
    (the redundant-copy layout, verified against octoshock); sample *i* is the low/high nibble of
    `byte[16 + i*4 + (b >> 1)]`.
  - 8-bit: 4 blocks per group; sample *i* is `byte[16 + i*4 + b]`.
- **Per block**: `shift = param & 0xF`, `filter = param >> 4`;
  `s = (raw >> shift) + ((prev1*f0 + prev2*f1 + 32) >> 6)`, clamped to 16 bits. A shift above 12
  behaves as 9 (hardware); a filter index above 4 is clamped.
- Stereo interleaves channels across blocks (`ch = b & 1`); the worst-case sector (mono 4-bit)
  yields 18 × 8 × 28 = 4032 frames.

### Flow control and end-of-track

- **Queue targets.** During a movie the pump keeps `XA_TARGET_BUFFERS` = 12 sectors (~0.6 s of
  stereo) queued, robust against underrun. Outside a movie XA carries only the short battle
  hit-sound clips, and the game mutes the shared source (`SsSetSerialVol`) on its own frame
  schedule while OpenAL output lags by the buffer depth — a deep read-ahead makes that mute land on
  audio not yet played, cutting clip tails and swallowing hits. So clips use
  `XA_TARGET_BUFFERS_CLIP` = 4, keeping the buffered audio close to the playhead. A per-pump guard
  (`XA_MAX_SECTORS_PER_PUMP` = 400) bounds a starved scan.
- **End of track.** `XA_END_MISS_LIMIT` = 150 consecutive non-matching sectors ends a track — well
  above the ~8-sector gap of a 1/8 interleave, so a live track is never cut. The limit applies only
  **after the first matching sector** (`s_xaMatchedYet`); before that a bounded pre-match scan
  (`XA_PREMATCH_SCAN_LIMIT` = 4500) reads through, mirroring hardware, because some tracks' data
  begins deep inside a shared multi-song interleave (the epilogue ocarina, XA 187: its `(file 9,
  chan 5)` sectors start ~400 sectors past its seek LBA). Hardware reads through indefinitely; the
  cap keeps a bad filter/LBA from scanning the whole disc while still ending within a dozen frames.
- **Ended vs. underrun.** When a track ends the pump sets `s_xaBaseLBA = -1` (so the game's next
  loop/new-track seek restarts cleanly) and calls `PC_XaEndStream()`. That tells `pc_xa.c` the
  stream has *ended* rather than momentarily underrun: `PC_XaService` stops re-issuing
  `alSourcePlay` on the drained source, which would otherwise replay the last stale processed
  buffer over and over (a train of ~6 short "afterhit" blips after each spell-hit clip). Buffered
  audio still plays out once; the buffers are reclaimed at the next `PC_XaReset`.
- **Buffer recycling only while playing.** `XaRecycle` reclaims processed buffers only when the
  source is `AL_PLAYING`. On a stopped/initial source `AL_BUFFERS_PROCESSED` is unreliable — it
  reports freshly-queued buffers as processed too — so reclaiming there would drain the queue
  being built up and the source could never (re)start. Deferring is safe: those buffers stay
  queued and are reclaimed on the next playing pass. Intentional teardown (`PC_XaReset`) does its
  own explicit unqueue after `alSourceStop`.

### Pause semantics

`CdlPause` is a **soft pause**. The game's XA play/loop path (`QueuePlayXa`) pauses and replays the
*same* track several times a second as it polls; tearing the OpenAL stream down on each pause
produces a reset storm → constant underrun → silence. So a pause only stops feeding new sectors and
keeps the source, its queued audio, the base LBA and the ADPCM history, and a same-track resume
(`CdlSeekL`/`CdlReadN` with an unchanged base) continues seamlessly. A genuinely long pause simply
drains the queue and goes quiet.

A pause **while a movie is active** (`Movie_Finish` at a one-shot end, or a player START-skip) is
different: a skipped movie is cut mid-stream with ~1 s of decoded audio still queued, and a soft
pause would only mute it via serial volume 0 — it becomes audible again when a later scene restores
the volume ("last note stuck in the buffer"). So a movie pause flushes the audio queue
(`PC_XaReset`) and closes the HD decoder, but **leaves the last decoded frame on the overlay** so a
wait-for-button movie end shows the final image rather than black; `ClearScreen` (a `PERMUTER` hook
in `src/core/movie_state.c`) drops the overlay when the next scene is drawn. Subtitle cues are also
left open until then, so the held frame keeps its cover.

**Scope** (validated against hardware with a BizHawk track map): XA carries the
**intro/FMV audio** and the **streamed spell SFX** — notably the big elemental spells
(Hurricane/Salamander/Avalanche/Plasma-Wave, XA files 17/18/19/22/23/33). There are **zero looping XA
tracks** in the game, so this layer only ever streams finite one-shots.

> **The game's background music is SEQ, not XA.** The loading/battle BGM is sequenced music
> (`battle/evaluators.c` `PlayBattleBGM` → `AUDIO_CMD_PLAY_SEQ` → SPU voices), an entirely separate
> subsystem. If music is silent, it is never an XA problem. See [spu.md](spu.md).

## Movie playback hooks

Movie (STR) playback shares this machinery; the video decode itself lives in `pc_mdec.c` (see
[mdec.md](mdec.md)). The pieces that live in `libcd.c`:

- **`CdRead2(Stream|RT)` starts the movie.** Only `Movie_Start` (`src/core/cd.c`) issues it, and on
  hardware that RT read is what starts the movie's interleaved XA audio. `Movie_Start` seeks (state
  2) *before* the `CdRead2(0x1c0)` that sets the RT bit (state 6), so the stream must begin here
  from the seeked LBA; a backend that only started XA from `CdlSeekL` seeing RT already set leaves
  the first movie (the boot logo) silent while every later movie inherits the stale RT mode.
  `CdRead2` also seeds the video demux cursor at the movie's base LBA and renders frame 1
  immediately so the first tick isn't black. The cursor rewinds only when the track base changes,
  so an already-running stream is undisturbed.
- **Filter reset.** A movie's sectors carry only its own audio stream, so `CdRead2` sets the filter
  to match-any (`s_xaFile = s_xaChan = -1`). A stale `CdlSetfilter` left by gameplay spell XA would
  otherwise filter a story movie's (different) file/channel out, giving silent video.
- **HD video swap.** If the HD pack provides `hdpacks/videos/<baseLBA hex>.mp4` for the movie
  (`MovieHdTryOpen`, keyed by the base LBA and opened once per movie — the stream-start block
  re-runs several times), `MovieRenderFrame` presents the decoded HD frame (`pc_hdvideo.c`; game
  frame 1 → mp4 frame 0) via `PC_GpuSetMovieOverlayRGB` and skips MDEC. The STR is still read for
  its XA audio and frame timing, so sync is untouched. If HD decode ends or fails mid-movie, the
  remainder falls back to native MDEC.
- **Subtitles.** `PC_MovieSubsOpen(baseLBA)` / `PC_MovieSubsFrame(n)` (`pc_movie_subs.c`) drive
  language-pack subtitle cues for the movie, keyed the same way.

### Movie frame pacing

`StGetNext` does not decode anything; it feeds the game's *own* completion logic a paced sequence
of frame headers so that the same decompiled code (`Movie_GetNextFrame`, `src/core/cd.c`: `if
(sMovieSectorHeader->frameCount >= s_totalFrames_80123268) s_movieFinished_8012326c = 1;`) signals
"finished" at the right tick. Getting the tick count right matters beyond looks: real hardware
spends **2556 ticks** in `STATE_MOVIE` for the intro pair (BizHawk capture, frames 1771–4326), and
`rand()` is called once per tick throughout (`UpdateEngine`, `src/core/engine.c`), so a movie that
finishes early desyncs the shared RNG stream before any gameplay logic runs.

The model follows hardware: `StGetNext` **succeeds on every call**, and only the reported
`frameCount` advances — once every `CALLS_PER_MOVIE_FRAME` = 4 successful calls (15 fps at the
60 Hz tick, psx-spx's cited standard STR rate). On hardware sectors arrive at the CD's 75–150/s,
far faster than the ~60 calls/s the game makes, so frame rate comes from multiple sector arrivals
accumulating per frame, not from `StGetNext` blocking (psx-spx `cdromfileformats.md`, "STR Frame
Rate"). The rate reproduces the measured span: the intro chains two movies via
`src/core/movie_state.c`'s case-100 re-entry (logo `frameCt = 0x8f` = 143, title `frameCt = 0x1db`
= 475; 618 frames), and `(2556 − ~20 per-movie start-up ticks) / 618 ≈ 4.1` ticks/frame ≈ 14.6 fps.

Pacing by making `StGetNext` *fail* on most calls is not viable through this call path, for two
structural reasons in the game code:

- `Movie_GetNextFrame` retries `StGetNext` in a tight, non-yielding loop of up to `0x100000`
  iterations, so anything gated on a call counter is exhausted within one tick, and anything that
  reads the wall clock inside `StGetNext` turns into a million syscalls per tick (a visible
  multi-second stall).
- `Movie_DecodeNextFrame` calls `Movie_GetNextFrame` only **once**; on failure it returns −1,
  `Movie_PlayNextFrame` then returns 1 unconditionally, and `State_Movie` case 11 treats any
  non-zero result as "finished" — there is no "no frame yet, try later" outcome.

Because case 11 calls `Movie_PlayNextFrame` once per tick and it always succeeds on the first inner
try, a plain per-call counter in `StGetNext` *is* a real-tick counter. `StSetStream` resets the
counter for each new movie (chained movies would otherwise inherit the previous count), and the
returned `addr[0]`/`addr[1]` must equal the header's `dummy1`/`dummy2` (both 0) or `src/core/cd.c`'s
sanity check silently discards the frame without advancing.

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
Validation is `PC_CdDiscSignatureOk()` in `libcd.c`: it reads the region's boot LBA (23 for
US/Asia) and checks for the `"PS-X EXE"` magic at `SECTOR_DATA_OFFSET` (24). Because the ISO layout
fixes the boot exe at that LBA, this passes for any genuine dump of that region and cheaply rejects
the common mistakes — a different game, a `.cue`/`.iso`/`.mp3` picked by accident, or a truncated
image — for the price of one sector read. (It is a signature check, not an exact MD5, so valid
alternate dumps are not rejected.) A valid disc of the *other* region is diagnosed by name via
`PC_CdDiscRelease()` (see [region boot signatures](#region-boot-signatures-and-disc-classification)).

Either failure — file won't open, or signature check fails — is **fatal**. `PC_FatalDiscError` prints
the tried path plus a hint to stderr, shows a native **message box on Windows** (`MessageBoxA`), and
exits, rather than booting into garbage with no disc data (every `CdRead` failing, nothing rendering).

## Gotchas / notes

- **`.bin`, not `.cue`/`.iso`.** The backend needs a raw 2352-byte/sector image. A cooked 2048-byte
  `.iso` has no sector framing (the `SECTOR_DATA_OFFSET` math and the XA subheaders both break), and a
  `.cue` is only a text sheet. Both are rejected by the signature check.
- **XA audio must start at `CdlReadN`, not `CdlSeekL`.** The game times spell animations to the
  `CdlReadN` (which fires ~2.7 s after the `CdlSeekL`, after the drive's physical seek would
  complete), capturing `gXaStartTime` there (`AudioJob_PlayXa`: SeekL in state 5 → `CdSync` wait →
  ReadN in state 7). Feeding audio at `CdlSeekL` makes every spell SFX start ~2.7 s early and finish
  before impact ("Avalanche ends at the apex"). So `CdlSeekL` only repositions the cursor and stays
  silent, and `CdlReadN` begins the stream. Rewinding the cursor at `CdlSeekL` is also what lets a
  loop restart replay from the base, and `PREPARE_XA` (a seek with no ReadN) correctly stays silent
  until its matching PLAY. Movies stream via `CdRead2` and are unaffected.
- **Read *through* non-matching sectors after a seek.** See [end of track](#flow-control-and-end-of-track):
  the miss limit applies only after the first matching sector.
- **`CdControl` must accept the XA commands.** The game reads `CdControl() == 0` as "not accepted,
  retry", so a backend that returns 0 for `CdlSetfilter`/`CdlSeekL`/`CdlReadN` makes the XA state
  machine loop forever, jamming the audio job queue and hanging `WaitForAudio()` deterministically
  when the demo returns to the menu. All of them return 1.
- **MDEC video** decode itself lives in `pc_mdec.c`; the `DecDCT*` entry points here are thin
  stubs/no-ops (`DecDCTout` zeroes its buffer so an undecoded region shows clean black rather than
  stale VRAM garbage, which is easy to mistake for a texture/CLUT bug). See [mdec.md](mdec.md).
- **Debug logging** is env-gated and off by default: `VH_XA_LOG=1` logs XA seek/read commands to
  `vh_xa_log.txt`; `VH_XA_CSV=1` writes a per-`VSync` `vh_xa_ours.csv` (frame, wall time,
  `gXaCurrentID`/`gXaDuration`/`gXaStartTime`/volume, streaming flag, queued buffers, source state,
  filter, cursor LBA) for diffing against a BizHawk track-map capture — the tell for a clip cut
  short is `gXaCurrentID` still set with its duration unexpired while `srcState`/`queued` are 0;
  `VH_CD_LOG=1` writes the CD load accounting above. For isolating XA from the music, `VH_SEQ_MUTE=1`
  mutes SEQ playback.

## See also

- [spu.md](spu.md) — SEQ music, VAG SFX, the software SPU (where the game's *music* lives).
- [mdec.md](mdec.md) — MDEC/STR video decode (shares this file's demux and XA-audio pump).
- [../../configuration.md](../../configuration.md) — `VH_DISC_IMAGE`, `VH_FAST_BOOT` and other
  runtime options.
- [../../architecture.md](../../architecture.md) — the swappable-interface layer these backends fit into.
