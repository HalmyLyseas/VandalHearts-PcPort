# SPU and sound (libspu / libsnd)

The console's sound hardware is the **SPU** (Sound Processing Unit): 24 ADPCM voices with
per-voice pitch, envelopes and a shared reverb, plus a CD/serial input for streamed audio. The
game never touches the SPU registers directly — it goes through Sony's high-level **`libsnd`**
(sequenced music, VAB instrument banks, SFX voices) and **`libspu`** (SPU-RAM / IRQ management).
The PC port replaces both.

Unlike the GPU or GTE backends, the sound stack is **two layers, and both are reimplemented from
scratch**:

1. A **sample-accurate software SPU** — `platform/pc/src/pc_spu.c`. It does what the real SPU chip
   does: decode SPU-ADPCM, step each voice through its sample at a pitch rate, interpolate, shape
   with an ADSR envelope, sum up to 24 voices, run the reverb network, and hand one stereo 44.1 kHz
   stream to a single OpenAL streaming source. `platform/pc/src/libspu.c` is a thin stub for the
   handful of PsyQ `libspu` entry points the game calls (SPU-RAM allocation, IRQ hooks) — concepts
   with no analogue on an OpenAL backend; the one with real behaviour is
   `SpuSetKey(SPU_OFF, SPU_ALLCH)` → "stop all voices".
2. A reimplementation of PsyQ **`libsnd`** — `platform/pc/src/libsnd.c`. This is the *software* that
   ran on the PS1's R3000, not the SPU chip: it parses **VAB** instrument banks, runs a **SEQ**
   sequencer (PS1 SEQ is a MIDI-like format) to play the game's music, and computes PsyQ's exact
   per-note **volume law** before keying a voice on in the software SPU.

Keeping these two layers distinct matters, and it is the single most important lesson from the
audio work. Emulators (BizHawk / octoshock) execute Sony's real `libsnd` code out of the byte-exact
`SLUS_004.47`, so they only ever exercise **layer 1** (the chip). Layer 2 — VAB parsing, tone
selection, the volume law, the sequencer — is documented **nowhere** and is covered by no emulator.
Every audio bug that survived the first "sounds right" sign-off lived in layer 2 (see
[Gotchas](#gotchas--notes)); the chip emulation was correct throughout.

## The software SPU

`pc_spu.c` renders audio one voice at a time and sums them, exactly like the hardware mixer.

- **Voice pool.** A 32-slot pool (`SPU_VOICES`); the real SPU has 24 voices and measured peak usage
  is exactly 24, so the extra slots are only headroom. `libsnd.c` partitions them: SEQ music uses
  voices 0–19, SFX use 20–23.
- **SPU-ADPCM (VAG) decode** (`DecodeVag`, `libsnd.c`). Standard PS1 ADPCM: 16-byte blocks, 28
  samples each, a fixed 5-tap filter-pair table (`s_filterPos`/`s_filterNeg`), and per-block flag
  bits for the sustain loop (`0x04` = loop-start, `0x01|0x02` = end-block-that-repeats). Decoding
  happens once, at VAB load; the decoded 16-bit PCM is retained on the `Vab` so the software SPU can
  step through it directly.
- **Pitch / resampling.** Each voice carries a 20.12 fixed-point `phase` and a `step` (the SPU
  `VxPitch`, `0x1000` = unity, clamped to `0x4000`). `step` is computed from the tone's centre note
  and the played note via `PitchRatio()` (equal temperament; the real PsyQ table differs by
  <1 cent). Sustain loops wrap the phase back to `loopS`.
- **4-point Gaussian interpolation** (`SpuVoiceSample`). The PS1's 512-entry Gaussian table
  (byte-identical to psx-spx and to ctr-native) reads the four samples around the current position,
  indexed by bits 4–11 of the phase counter. The history fetch (`SpuGetSample`) is **loop-aware**:
  after a sustain loop wraps, the three "previous" samples must come from the loop *tail*
  (`loopE-1..loopE-3`), not the pre-loop attack data, because hardware carries a running 4-sample
  shift register rather than indexing backwards.
- **ADSR envelope** (`SpuDecodeAdsr` / `SpuAdsrRunStep` / `SpuAdsrAdvance`). `adsr1`/`adsr2` are
  decoded into attack (shift/step/exponential), decay (exponential to the sustain target
  `min(0x7fff,(SL+1)<<11)`), sustain (shift/step/exp/direction) and release fields, and the envelope
  is advanced **once per 44.1 kHz output sample** — the sample-accurate transient shaping that a
  60 Hz per-source gain update (the earlier OpenAL approach) physically could not reproduce. A
  sustain phase in *decrease* mode that reaches 0 frees the voice: most SFX end this way rather
  than by key-off (an SFX log shows ~90 looping key-ons against ~14 key-offs), and a voice left
  active at level 0 would hog the pool.
- **Reverb** (`RevProcess` and helpers). The game selects `SPU_REV_MODE_STUDIO_C`, so only that
  preset is embedded (32 registers + the 39-tap FIR). The comb/all-pass network runs over a
  `0x37F0`-sample work buffer at **22.05 kHz** (every other output sample), with the FIR resampling
  44.1 kHz ↔ 22.05 kHz. Only voices whose tone sets `VagAtr.mode` bit 2 (per-voice reverb-enable,
  `EON`) feed the send — the music bank's tones are all `mode=4` (wet), every SFX bank's are
  `mode=0` (dry), so menu beeps don't get a reverb tail.

### The OpenAL streaming sink and how it's driven

There is **one** OpenAL source for all of SPU audio (mirroring `pc_xa.c`'s pattern), fed from a pool
of 12 buffers of `SPU_BLOCK_FRAMES` (512) stereo samples each — roughly 140 ms of slack against
game-thread hitches. There is **no audio thread**; the queue is topped up once per VSync.

The key design point is that **audio is the master clock**. `PC_SeqTick()` (called per VSync from
`libetc.c`) calls `PC_SpuService()`, which for each free buffer calls
`PC_SeqAdvanceSamples(512)` — advancing the SEQ sequencer by exactly that block's worth of samples —
**before** rendering the block. Note on/off timing is therefore locked to the 44.1 kHz sample clock
rather than the jittery video-frame clock, which removed audible loop-point hiccups. `SpuService`
also reclaims finished buffers *unconditionally* (including after an underrun that leaves the source
`STOPPED`) and restarts the source whenever it isn't playing but the queue is primed, so a stall
self-heals instead of going permanently silent.

## PsyQ libsnd and the SEQ sequencer

Music is a **SEQ**: a PS1-specific, MIDI-like event stream (magic `pQES`), played by interpreting
its events in real time against a **VAB** instrument bank.

**VAB structure** (`SsVabOpenHeadSticky` / `SsVabTransBodyPartly`):

- A 32-byte header (`pBAV` magic, program/tone/VAG counts, `VabHdr.mvol`).
- A **fixed 128-slot program table** (`ProgAtr`, 2048 bytes) — one entry per possible program,
  regardless of how many are used. Each entry carries `nTone`, program master volume (`mvol`), and
  program pan.
- A **tone table**: one **16-slot block of tones** per program *that actually has tones* (see
  [tone-block packing](#vab-tone-block-packing) — this is not a naive `prog*16` index). Each tone
  (32 bytes) names a VAG sample, a centre note, fine tune, volume, pan, key range, the ADSR words,
  and the `mode` byte (reverb-enable).
- A **VAG size table** (`u16` per entry, sizes in **8-byte SPU units**), whose cumulative sum gives
  each VAG's offset into the VB body.
- The **VB body**: the raw SPU-ADPCM sample data, streamed in and decoded per-VAG.

So the hierarchy is **programs → tones → VAG samples**: a SEQ program-change selects a program, the
played note selects a tone within it (by key range), and the tone names the VAG to synthesise.

**The `SsSeqOpen` VAB-id breakthrough.** `SsSeqOpen(seqData, vabId)`'s second argument is the **VAB
the sequence plays through** — the game passes `1` (`SD_SEQ`, the dedicated 16-instrument music
bank), *not* a "mode". Early on this argument was ignored and the sequencer bound to the
last-opened VAB, which during a battle is the map's **SFX** bank (a weird one-key-per-tone tiled
layout). The music therefore played on the wrong instruments and sounded like garbage. Honouring the
argument (`libsnd.c:950`) was the timbre breakthrough: garbage → the correct instruments.

**Event handling** (`SeqProcessEvent`). Standard MIDI-style status bytes with running status and
VLQ delta-times: note-on/off (note-on with velocity 0 = note-off), program change (`0xC0`), and
control changes `0x07`/`0x0B` (channel volume), `0x0A` (pan), `0x7B`/`0x78` (all-notes-off). One
non-obvious fact, verified by hand-decoding real song bytes: **PS1 SEQ meta events carry no
SMF-style length field**. A tempo event is `FF 51 tt tt tt` (three raw bytes), not
`FF 51 03 tt tt tt`, and end-of-track is `FF 2F`. Assuming SMF meta encoding read the first tempo
byte as a length, skipped into a note-on, and desynced the whole stream — the "stuck chord after a
scene transition" bug.

### VAB body transfer and CD completion accounting

`SsVabTransBodyPartly` is called once per CD chunk, not once with the whole body:
`ContinueLoadingVab` (`src/core/cd.c`) streams the `.VB` in fixed 90-sector (184320-byte) chunks
through the same buffer address, mirroring the SPU-DMA partial transfers of real hardware. The
backend stages each chunk and returns `-2` ("need more") until the full body has arrived, then
decodes every VAG in one pass. Reporting completion after the first chunk leaves the caller's
chunk loop with no way to request the next one, and `gCdLoader.state` never advances.

The body size to wait for is `gCdFiles[gVabLoader.bodyCdf].sectorCt * 2048` — `cd.c`'s own
notion of "done" — **not** the sum of the VAG size table. The two legitimately differ (a body can
occupy more sectors than its samples need), and a smaller VAG-sum total makes the transfer report
"done" before the loader has read every sector it expects, stalling it permanently. The `size`
argument is the caller's fixed maximum chunk size, so it is clamped to the bytes still outstanding.
Reaching from the audio backend into `gCdFiles` / `gVabLoader` is a deliberate, narrow layering
exception; `gVabLoader` is an anonymous struct in `cd.c` and is mirrored field-for-field as an
`extern struct {...}` in `libsnd.c`.

### Tone selection

`SeqPickTone` first looks for a key-range match inside the requested program (a bank authored
as real multi-program instruments), then falls back to a global search: an exact key anywhere in
the bank, else the nearest key overall. The fallback serves banks laid out as **one multisample
tiled across programs** — each tone keyed to a single note (`min == max`) and the programs'
key-blocks tiling the keyboard (prog 0: 24–31, prog 1: 32–39, … prog 9: 96–103) — where the note,
not the program change, selects the sample.

### SFX voices and hold loops

SFX tones carry a degenerate, never-fading ADSR and most get no key-off. Their VAGs end in a
one-block (~28-sample) sustain loop that decodes to a flat `+28672` DC — a **parking spot**, not
part of the sound: hardware sits there at full envelope (constant DC, inaudible) until the ADSR
release ramps it away. Looping it in the port keeps the voice active forever and starves the
allocator, so a loop shorter than `SFX_HOLD_LOOP_MAX` (128 samples) is treated as one-shot and
the sample is truncated **at `loopS`**, where the real audio ends cleanly (last samples −17 / 1 / 4).
Truncating anywhere inside the DC block emits a rectangular pulse — the menu "pop". Genuine
sustain loops (spell casting sounds) are 900–8500 samples, so the threshold separates them
cleanly; those ring until the game's `SFX_ROLE_RELEASE` key-off. Music notes are untouched: they
get real note-offs.

### Voice ownership and the stuck-note reaper

`libsnd.c` keeps a 32-slot bookkeeping pool (`s_voices`) beside the software SPU's voices:
`active`, the `(vabId, prog)` pair `SsVoKeyOff` looks up, and for SEQ notes the owner
`(ownSeq, ownCh, ownNote)`; each sequence holds the back-reference `noteVoice[ch][note]`. SEQ
notes use voices 0–19 (idle first, then round-robin steal); `SsUtKeyOnV` SFX use 20–23. When a
note-on steals an in-use SEQ voice, the previous owner's `noteVoice` entry is cleared so its later
note-off cannot release the reassigned voice.

The invariant that closes the whole hung-note class: **a live SEQ voice must belong to a playing
sequence that still back-references it.** Two mechanisms enforce it:

- `SeqReleaseOrphanVoices` (on stop, close and pause) force-releases every voice owned by the
  sequence whether or not `noteVoice` still tracks it — the per-note `SeqNoteOff` loop misses
  voices orphaned by a steal chain, and those sustain forever after a scene transition.
- The reaper in `SwSpuSyncActive` (once per rendered audio block) force-releases a still-*held*
  SEQ voice whose owning sequence no longer references it. It is safe by construction: SFX voices
  (`ownSeq < 0`) are skipped, a correctly held note always satisfies
  `noteVoice[ownCh][ownNote] == voice`, and a normally releasing voice is skipped via
  `PC_SpuVoiceReleasing`.

`SsSeqPlay` with any mode other than `SSPLAY_PLAY` freezes the sequencer and releases held notes
(defensive — the game only passes `SSPLAY_PLAY`; play restarts from the top, so there is no
resume state to keep).

### Runaway-delta watchdog

A SEQ carries no length field, so `Seq.end` is the `SEQ_MAX_BYTES` (0x40000) cap (or `gSeqData`'s
generated size when the song lives there), not the
true end. If the parser runs past real track data it reads a garbage VLQ delta that `tickBudget`
can never reach, and the sequence freezes holding its last chord. `SeqAdvanceByUsec` treats any
inter-event delta above 32 quarter-notes (`ppqn * 32`, far beyond any real gap) as end-of-track.
That path also drops held notes when looping (the chord is stale), whereas a clean `0x2F` loop
keeps a note legitimately sustained across the seam.
**Lifecycle and bounds are disc-derived-data-hostile.** `SsVabClose` hard-stops (not key-offs — a
releasing voice still reads its PCM each render) every software-SPU voice pointing at the bank's
decoded samples before freeing them, so swapping a bank while one of its voices is still
sustaining/releasing can't read freed memory. `SsVabTransBodyPartly`'s decode loop checks each VAG's
`offset + size` against the staged body's real length before calling `DecodeVag`, stopping that
bank's decode (and logging once) rather than reading past a malformed VAB's size table. `SsSeqOpen`
bounds a song against the real size the data-segment generator gave `gSeqData`
(`PC_GenSize_gSeqData`) when the blob lives there, instead of the `SEQ_MAX_BYTES` safety cap alone;
`SeqProcessEvent` additionally checks its bound before every fixed-width read (status byte, 1/2/3
data bytes), so a song missing its `FF 2F` terminator stops cleanly at the buffer edge.

## The PsyQ volume law

Getting a note's loudness right required disassembling PsyQ's real key-on volume path out of the
byte-exact `SLUS_004.47` — it is documented nowhere. The full chain, reproduced in `libsnd.c`
(`SeqNoteOn`) and `pc_spu.c`:

```
lin = 16383 · (V/127) · (M/127) · (P/127) · (T/127) · (C/127)
      V = note velocity × CC7 channel volume
      M = VabHdr.mvol   (header master volume)
      P = ProgAtr.mvol  (per-program master volume)
      T = VagAtr.vol    (per-tone volume)
      C = SsSeqSetVol   (per-sequence L/R volume)
```

then **three successive pan stages** — tone pan, program pan, note/channel pan — each attenuating
one side, using a divide by **63, not 64** (`PanFactors`):

```
pan <  64 :  R = R · pan       / 63,   L unchanged
pan >= 64 :  L = L · (127-pan) / 63,   R unchanged
```

so `pan == 64` takes the second branch and yields 63/63 = unity on both channels — a constant-power
"centre is full on both sides" law, no −3 dB centre dip.

The decisive, long-missed stage is the final **SQUARE LAW** (disassembled at `0x800d6d8c`, which
runs after and overwrites the earlier linear stage at `0x800d74b8`):

```
VolL = L·L / 16383      VolR = R·R / 16383
```

i.e. `out = 16383 · (lin/16383)²`. It is applied **after** panning, so the pan factors are squared
too. Because it is quadratic, **every dB of attenuation is doubled** — the port had been rendering
the mix at *half* the hardware's dynamic range in dB. That single omission explained three
independent symptoms at once: a flatter/denser waveform versus the reference's peaks-and-dips, a
listener's "one instrument sits too loud, lighter mood than the original", and why every
synthesis-chain check passed (synthesis was never wrong — the *level law* was). Two pieces of prior
context: our linear chain was already algebraically identical to PsyQ's `lin`, and `VabHdr.mvol` /
`ProgAtr.mvol` had also been skipped entirely (the latter is what keeps the bass, program 0 at
`mvol 127`, above the rest of the bank at `mvol 85–104`).

### SFX key-on volume

`SsUtKeyOnV` and `SsVoKeyOn` (`PlayOnVoice` in `libsnd.c`) run the **same chain** — PsyQ's
`0x800d6d8c` serves all three key-on entry points — with one difference: hardware skips the
per-channel volume stage for SFX (`0x800d6e7c` tests a flag byte for `0x21`, which `SsUtKeyOnV`
sets). So `lin = (V/127)·(M/127)·(P/127)·(T/127)` with `V` the caller's `voll`/`volr`, fed to the
voice's L and R gains separately rather than averaged to mono, then the square law. With typical
`P, T ≈ 100/127` the two bank factors contribute `(0.787·0.787)² ≈ 0.38` (about 8 dB); without
them full-volume SFX such as menu beeps sit near 0 dBFS and the voice cut-off pops.

### Master trim

`SpuSeqGain()` = **1.012** is the single output trim multiplied into every voice gain (music and
SFX). It is derived from the **rendered output** against the octoshock reference: a per-voice
gain × ADSR model predicts a 2.380× compensation for the square law, but the rendered mix still
lands 4.97 dB low (RMS 1295.5 vs 2296.7, ×1.773) because that model ignores voice summation and the
reverb send; the true factor is 2.380 × 1.773 = 4.22 and the trim 0.24 × 4.22 = 1.012. That puts
the port's peak at −5.6 dBFS against the reference's −5.9 dBFS — the same headroom hardware leaves
for SFX — with RMS within 0.01 dB. Rule: derive a *level* trim from rendered output, never from a
model of the inputs.

## VAB tone-block packing

A VAB's tone table holds **one 16-tone block per program that actually has tones**. Programs with
`nTone == 0` are **skipped and consume no block**, so every program after such a hole shifts down:

```
block(p) = count of programs q < p with nTone[q] > 0
```

Indexing tones by the raw program number (`prog*16`) is therefore wrong for any bank with a hole,
and hands those programs **another instrument's samples**. The music bank `SD_COMP` is exactly this
shape — program 11 has `nTone == 0`, so:

```
prog 12 → block 11,  13 → 12,  14 → 13,  15 → 14,  16 → 15
```

Programs 12–16 all played the wrong samples. The audible one was program 14 (the demo-battle lead):
it played VAG 9 (centre note 67) where hardware plays VAG 19 (centre 79), so its high notes came out
**an octave up** and piercing — the exact measured signature (right notes and gains, +10…+18 dB
above 3.5 kHz). We were rendering the *wrong sample correctly*, which is why every gain / sample /
envelope check passed.

The fix (`SsVabOpenHeadSticky`) builds a `block[]` table across the full 128-slot `ProgAtr`, indexes
tones by `block[p]`, sets `numPrograms` to the highest *usable* program index (a corollary: the
header's `ps` counts tone-bearing programs, not the highest index — `SD_COMP` declares `ps=16` yet
legitimately uses program 16), and points the VAG size table at `toneTable + nBlocks*16*32` (using
`nBlocks`, not `numPrograms`, or every VAG offset shifts).

Why it hid so long: only **1 of the 13 VABs** on the disc has an `nTone == 0` hole (the music bank);
every SFX bank has a single program, where a hole is impossible. Raw-program indexing was correct
for 12 of 13 banks — hence a clean playthrough and perfect SFX. And the mapping leaves **no trace in
the file**: on disc the block-index byte is `0xFF` for every program; PsyQ's `SsVabOpenHead` computes
and caches it at *load time*. The port had been treating the VAB header as static data to parse when
PsyQ actually rewrites it in RAM first.

After this fix: 3–6 kHz band error **+3.99 → −1.01 dB**, mean |error| over 30 Hz–12 kHz
**2.19 → 1.33 dB**, and the user judged the result **indistinguishable from hardware**.

## Music is SEQ, not XA

A recurring early misconception was that the battle/loading music is streamed CD audio (XA). It is
**not**. The score is **SEQ**, synthesised on the SPU from the `SD_SEQ` bank; `PlayBattleBGM`
(`src/battle/evaluators.c`) issues `AUDIO_CMD_PLAY_SEQ`. A BizHawk capture of a full demo→menu run settled
it on real hardware: across the whole run there are **zero looping XA tracks**, and during
loading + battle the XA stream is idle except for brief one-shot spell SFX. The music was silent in
early builds purely because SEQ was unimplemented — no XA fix ever helped.

**XA is for streamed audio only**: FMV/movie soundtracks and a handful of spell **hit** SFX (e.g.
the multi-target Salamander / Plasma-Wave impacts, which are `PLAY_XA` cues, not VAB voices). That
path lives in the CD backend — see [cd-xa.md](cd-xa.md). The rule the hard way: identify *which*
subsystem owns a sound (SEQ vs VAB-SFX vs XA) before debugging it.

## Diagnostics

All of these are environment variables read at startup; none is needed for normal play. Together
they cover A/B toggling, per-instrument isolation, and capture.

| Variable | Effect |
|---|---|
| `VH_SEQ_MUTE=1` | Silence only the SEQ **music** (note-ons with an owning sequence), leaving VAG SFX and XA audible — a capture then has SFX without the music masking them |
| `VH_SPU_SOLOPROG=N` | Play only program `N` (isolate one instrument by ear; spectral attribution alone misidentifies instruments, soloing settles it in one run) |
| `VH_SPU_MUTEPROG=N` | Silence program `N` |
| `VH_SPU_DUMPVAG=1` | Dump every decoded VAG as `vh_vag_<vab>_<n>.wav` (44.1 kHz mono) plus `vh_vag_manifest.csv` (`vab,vag,len,loopS,loopE`; append-only, since several VABs load and each restarts its VAG numbering at 1) |
| `VH_SPU_TRACE=1` | Per-voice CSV trace (`vh_spu_voices_ours.csv` / `vh_spu_globals_ours.csv`), one row per active voice per rendered block, laid out to diff field-for-field against a per-voice hardware capture. `block` is a render-block counter, not a video frame — align to a hardware trace by elapsed time. `sampleHz = 44100 · step / 4096`, the same scale as the hardware pitch register. SFX voices are tagged too, otherwise their zero-initialised ids look like a phantom "VAG 0" instrument |
| `VH_SEQ_LOG` | SEQ diagnostic log (`vh_seq_log.txt`). Set to anything: `[open]` (bound VAB, header hexdump), the first 96 `[note]` events, `[watchdog]`, `[stall?]` (a wait over 8 quarter-notes, rate-limited ~1/s), `[reaper]`, `SsSeqStop`. `=1` also adds a ~6×/s `[voices]` census — active-voice count, note-on/off balance, master volume, reverb, wall-clock — plus a per-voice roster; a voice that stays `HELD` across many dumps is a stuck note (`ownSeq<0` = SFX, `ownSeq>=0` = a stalled sequence), and a jumping timestamp during a silence means the game thread blocked rather than muting |
| `VH_SFX_LOG=1` | SFX key-on/off log (`vh_sfx_log.txt`) |
| `VH_SPU_BASS` / `VH_SPU_TREB` / `VH_SPU_BASSFC` / `VH_SPU_TREBFC` | Parameters of the EQ tilt in `pc_spu.c`, which is compiled but **unreachable** (`s_analogOn` is fixed at 0) — see the gotcha below. Setting them has no audible effect |

The software SPU is the only rendering path; there is no OpenAL per-voice fallback, and no
environment toggle for the volume law, master trim or reverb.

## Gotchas / notes

- **The chip was never the bug.** SPU-ADPCM, the Gaussian table, ADSR, reverb and the pitch table
  all matched psx-spx / octoshock from early on. Every substantive audio bug was in the **`libsnd`
  reimplementation** (layer 2): the wrong VAB binding, the missing `ProgAtr.mvol`, the pan divisor,
  the square law, and tone-block packing. No emulator covers that layer, because emulators run
  Sony's real `libsnd` code. When audio is wrong, suspect layer 2 first, and recover ground truth by
  disassembling the real PsyQ routine from `SLUS_004.47`.
- **The EQ tilt in `pc_spu.c` (`AnalogCh`) is a compensation, not a model, and is hardwired
  off.** octoshock's reference output is raw digital — it sums voices and reverb, applies main
  volume, clamps and scales ×0.75, with no analog or low-pass stage — so there is no analog output
  stage to model. The filter is a spectral tilt fitted to a mix that lacked the square law; with
  the law in place it over-corrects (mean |error| vs the reference over 30 Hz–12 kHz: raw mix
  2.20 dB, raw mix + tilt 4.27 dB). Any residual spectral error belongs in the raw mix itself
  (interpolation aliasing, ADSR attack brightness, reverb balance), not in a post-filter.
- **VAG size table is in 8-byte units**, not bytes. Treating the entries as bytes decodes 1/8 of
  each sample and reads every later VAG from an 8×-too-small offset — right notes, wrong data.
- **SFX have a degenerate, never-fading ADSR** and park on a ~28-sample DC "hold" loop; the port
  truncates them at `loopS`, and truncating anywhere else emits the menu "pop" — see
  [SFX voices and hold loops](#sfx-voices-and-hold-loops).
- **Offline VAB analysis from the ISO is unreliable** — the body offset there is not the one the
  game actually transfers, and it reads all samples as non-looping. When a sample's timbre is in
  question, dump what the game really plays (`VH_SPU_DUMPVAG`) rather than reasoning about the disc
  layout.
- **Final fidelity:** mean |error| vs the octoshock reference ~**1.33 dB** over 30 Hz–12 kHz, with
  stereo image and level matched to within a few tenths of a dB — judged indistinguishable from
  hardware by ear. Residual work (a slightly dark top octave, small crest/PLR differences) is
  banked, not audible in A/B.
