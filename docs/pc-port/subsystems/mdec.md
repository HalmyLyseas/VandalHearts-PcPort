# MDEC / STR video (libpress)

The PS1 plays its full-motion video through the **MDEC** (Motion DECoder), a fixed-function
macroblock decoder that turns a compressed bitstream into 16×16 YUV tiles. Vandal Hearts uses it
for exactly two FMVs at boot: the Konami logo (`STR_US/LOGO_USA.STR`) and the game intro
(`STR_US/TITLE_WS.STR`), both 320×240 `.STR` files. The game drives the MDEC through PsyQ's
`libpress` module (`DecDCTin` / `DecDCTout` / `DecDCTvlc` and friends).

On PC there is no MDEC hardware, so the backend is a **from-scratch software decoder**,
`platform/pc/src/pc_mdec.c`. It was reimplemented from the psx-spx documentation (the "BS
Compression" and "macroblockdecodermdec" chapters), with ffmpeg's `libavcodec/mdec.c` consulted
**only** as an offline cross-reference — nothing was linked or copied. It is validated to
pixel-parity with ffmpeg's `-f psxstr` output: 100% of pixels within 8/255 on both bitstream
versions the FMVs use, the small residual being float-vs-integer IDCT rounding that psx-spx notes
is hardware-unspecified.

The decoded frames are handed to the [GPU backend](gpu.md) for presentation; the FMVs' audio is a
separate, interleaved XA stream handled entirely by the [CD-ROM / XA backend](cd-xa.md) — it is
not part of this decoder.

## The STR/MDEC pipeline

A `.STR` movie is a run of raw 2352-byte CD sectors on the disc, interleaving video and XA-audio
sectors. Playing one on PC runs end-to-end as follows:

1. **Arm the movie.** The game's `Movie_Start` (`src/core/cd.c`) seeks to the movie's base LBA and
   issues `CdRead2(Stream|Speed|RT)`. On PC, `CdRead2` (`platform/pc/src/libcd.c`) treats that RT
   read as the start of a movie: it seeds a demux cursor at the just-seeked LBA, starts the movie's
   XA audio (see [cd-xa.md](cd-xa.md)), and renders frame 1 immediately so the first tick isn't
   black.

2. **Pace the frames.** The game pulls frames via `StGetNext`. The PC `StGetNext` does *not* block
   on sector arrival; it always succeeds immediately and advances a paced frame counter once every
   `CALLS_PER_MOVIE_FRAME` (= 4) calls — ~15 fps at the 60 Hz tick, the standard STR rate. Each time
   the counter ticks over, it calls `MovieRenderFrame(N)`. (Why the pacing is faked rather than
   driven by real decode timing is documented at length in the `StGetNext` comment in `libcd.c` —
   it exists to keep the game's own completion logic, and therefore the shared RNG stream, in step
   with real hardware.)

3. **Demux one frame.** `MovieRenderFrame(N)` reads raw sectors from the disc image (the same
   `FILE *` used for XA streaming), skipping audio sectors and any MDEC video sector whose frame
   number isn't `N`. Each `.STR` video sector carries a 0x20-byte header (magic `0x8001`, frame
   number, sector-in-frame index, and — on the first sector — width/height and sector count)
   followed by a `0x7E0`-byte BS-bitstream payload. The payloads for frame `N` are concatenated
   into one buffer until the frame's last sector is seen.

4. **Decode.** The assembled BS bitstream is passed to `PC_MdecDecodeBS(bs, bsLen, w, h,
   outBGR555)`, which decodes the whole frame into a 320×240 BGR555 buffer.

5. **Present.** The decoded buffer is registered with the GPU backend via
   `PC_GpuSetMovieOverlay(bgr, w, h)`. While a movie is active, `PC_GpuPresent`
   (`pc_gpu_window.c`) shows that buffer **fullscreen** instead of reading VRAM. This overlay path
   deliberately sidesteps the movie's 24bpp VRAM packing — the normal present path reads VRAM as
   16bpp only. When the movie ends (`CdlPause` / `CdlReset`), the overlay is set to `NULL` and
   normal VRAM presentation resumes.

Note that `libpress`'s own entry points (`DecDCTout` etc.) are stubs on PC — `DecDCTout` simply
zeroes its buffer. The real decode is driven out-of-band by the demux + overlay path above, not
through the game's `libpress` calls.

## The MDEC decode

`PC_MdecDecodeBS` reproduces the PS1 macroblock decode. A BS frame begins with an 8-byte header
(the `0x3800` magic ID, a 6-bit `q_scale`, and a version word — the two FMVs use **BS v3 (logo)**
and **BS v2 (title)**). The rest is a bitstream read as little-endian 16-bit halfwords, MSB-first
within each halfword.

The image is decoded macroblock by macroblock in **column-major** order (down each 16-pixel
column, then the next column). Each 16×16 macroblock is six 8×8 blocks in the order
**Cr, Cb, Y1, Y2, Y3, Y4** (4:2:0 chroma). For each block:

1. **DC coefficient.**
   - *v2:* a direct signed 10-bit DC value (`0x1FF` signals end of frame).
   - *v3:* a Huffman-coded DC *offset* relative to the previous block's DC, with separate VLC trees
     for chroma and luma (MPEG-1-style); the running DC is accumulated and wrapped to 10 bits. Ten
     consecutive `1` bits signal end of frame.

2. **AC coefficients.** A shared run/level VLC (`ac_next`) yields `(run, level)` pairs, plus an
   escape code (`000001` + 16 raw bits) and an end-of-block marker. Each pair skips `run` zeros and
   places one coefficient.

3. **Dequantise + inverse zig-zag.** Each AC coefficient's raster position is looked up in the
   `ZIGZAG` table and dequantised as `magnitude * q_scale * QUANT[pos] >> 3` (floor), with the
   escape-coded oddification `(v-1)|1` and re-signing that matches ffmpeg's `mdec.c`. The DC is
   dequantised as `dc * QUANT[0]` with no `q_scale`. `QUANT` is the standard MDEC default
   quantisation table in raster order.

4. **IDCT.** A separable 8×8 inverse DCT (`idct8x8`) using a precomputed float cosine table
   (`idct_init`) converts the dequantised block to spatial samples.

5. **YUV→RGB.** For each output pixel the luma sample and the (half-resolution) Cr/Cb samples are
   combined with the standard coefficients (`R = Y + 1.402·Cr`, etc.), offset by +128, clamped to
   0–255, and packed to BGR555 (`(B>>3)<<10 | (G>>3)<<5 | (R>>3)`) into the output frame at the
   macroblock's pixel origin.

### The inverse-zig-zag table (the crux)

The single most important detail — and the bug that took the longest to find — is the direction of
the zig-zag table. psx-spx's published `zigzag[]` is the **inverse** map (raster position →
scan index); what the decoder needs at step 3 is the **forward** scan (scan index → raster
position), psx-spx's `zagzig`. The two tables agree only at indices 0 and 1.

Using the inverse table directly is a *near-miss*: simple frames — mostly a DC term plus the first
AC coefficient — come out pixel-exact, while detailed frames come out blurred because their
high-frequency AC energy is scattered to the wrong raster positions. The lesson: a clean first
frame proves almost nothing; **always validate a codec against a high-detail frame.** The give-away
here was precisely that early/simple frames matched ffmpeg exactly while complex ones did not. The
`ZIGZAG` constant in `pc_mdec.c` is the forward scan, and its comment records this distinction.

## Frame presentation and timing

Decoded frames reach the screen through the GPU overlay described above — a fullscreen blit of the
BGR555 buffer, bypassing VRAM entirely for the movie's duration. Pacing is decoupled from decode:
`StGetNext` advances the frame counter on a fixed cadence (4 backend ticks per frame ≈ 15 fps) and
`MovieRenderFrame` decodes *that* frame number on demand, so audio/video stay aligned to the game's
own tick-driven completion logic rather than to decode speed. The demux cursor scans forward from
where it left off, rewinding to the movie base only if a smaller frame number is requested (a
loop/restart).

**Movie skip:** the game's movie loop (`src/core/movie_state.c`) already honours a per-movie `skip`
flag for player-1 START on movies flagged skippable. The port adds a PC-only QoL gate
(`#ifdef` PC-build) that lets player-1 START skip **any** movie straight to its transition — useful
for the long story FMVs during debugging. (See the PC-port QoL tracker for the broader set of these
additions.)

## Gotchas / notes

- **Zig-zag direction is the whole ballgame.** See above — use the forward scan
  (`zagzig`), not psx-spx's `zigzag[]`. Test on a detailed frame, never just frame 1.
- **Dump the game's own bytes before assuming a version.** The two FMVs are *different* BS
  versions (logo = v3, title = v2). Assuming a single version half-works and looks exactly like a
  decoder bug.
- **The residual is rounding, not error.** The float IDCT diverges from a fixed-point
  implementation by ≤ ~8/255; psx-spx states the exact fixed-point behaviour is hardware-
  unspecified, so this is expected, not a defect.
- **FMV audio is XA, not part of this decoder.** The interleaved audio in a `.STR` is an XA
  stream decoded and mixed by the CD backend; see [cd-xa.md](cd-xa.md) for how a movie's audio is
  started (`CdRead2` with the RT bit) and why filter state matters.
- **`libpress`'s stubs are intentional.** `DecDCTout` and the other `DecDCT*` entry points are
  no-ops on PC (`DecDCTout` zeroes its buffer to show honest black rather than misleading VRAM
  garbage). Real playback runs through the demux + overlay path, not through these calls.
- **Stage-2 backend only.** `pc_mdec.c` is part of the native port, not the byte-exact matching
  build. See the top-level [architecture](../../architecture.md) for the Layer 1 / Layer 2 split.
- **HD-pack video decode (`pc_hdvideo.c`) tolerates a mid-stream geometry change.** When a decoded
  frame's width, height, or pixel format differs from the cached triple, it rebuilds the `SwsContext`
  and output buffer for the new geometry instead of scaling into one sized for the old.
