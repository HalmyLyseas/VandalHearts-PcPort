/* G1 differential rasterizer harness (Phase 0b) — GP0 trace replay.
 *
 * Invokes the REAL production rasterizer by #including libgpu.c directly (so it always tracks the code,
 * incl. the coming DDA rewrite — no copy, no divergence). Replays a decompressed DuckStation .psxgpu GPU
 * trace into it and dumps each rendered frame to PPM. Next step diffs those vs DuckStation's reference.
 *
 * KEY: GPUPort0Data packets are DMA BLOCKS, not one-command-each — a command (esp. an A0 VRAM upload with
 * its pixel data) spans packets. So we concatenate all Port0 words into one GP0 FIFO stream and parse
 * commands by their true word-length (opcode+flags; A0 length from its w*h). VSync packets mark frames.
 *
 * Build (from repo root):
 *   cc -std=gnu99 -O2 -Iplatform/pc/include -Iinclude \
 *      platform/pc/tools/raster_harness/harness.c -lm -o /tmp/raster_harness
 * Run:  zstd -d "….psxgpu.zst" -o /tmp/trace.psxgpu ; /tmp/raster_harness /tmp/trace.psxgpu
 *
 * Harness limitation: our FillTriangle is flat-colour (no Gouraud interpolation) — Gouraud polys replay
 * with v0's colour. VH's casting-ray effect is flat-modulated, so the effect region (G1's target) is faithful.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef uint32_t u32; typedef uint16_t u16; typedef uint8_t u8; typedef int16_t s16;

void PC_GpuPresent(u16 *v, int a, int b, int c, int d, int e, int f) { (void)v;(void)a;(void)b;(void)c;(void)d;(void)e;(void)f; }
void PC_UpdateCamOsd(void) {}

#include "../../src/libgpu.c"   /* real rasterizer: s_vram, FillQuad, SampleTexture, s_drawEnv, ... */

/* ---- GP0 state ------------------------------------------------------------------------------- */
static void set_drawmode(u32 w) { s_drawModeTPage = w & 0x3FFF; s_drawModeAbr = (w >> 5) & 3; }
static void set_texwindow(u32 w) { s_twMaskX = w & 0x1F; s_twMaskY = (w >> 5) & 0x1F;
    s_twOffX = (w >> 10) & 0x1F; s_twOffY = (w >> 15) & 0x1F; }
static void set_area_tl(u32 w) { s_drawEnv.clip.x = w & 0x3FF; s_drawEnv.clip.y = (w >> 10) & 0x1FF; }
static void set_area_br(u32 w) { int x2 = w & 0x3FF, y2 = (w >> 10) & 0x1FF;
    s_drawEnv.clip.w = x2 - s_drawEnv.clip.x + 1; s_drawEnv.clip.h = y2 - s_drawEnv.clip.y + 1; }
static void set_offset(u32 w) { int ox = w & 0x7FF, oy = (w >> 11) & 0x7FF;
    s_drawEnv.ofs[0] = (ox & 0x400) ? ox - 0x800 : ox; s_drawEnv.ofs[1] = (oy & 0x400) ? oy - 0x800 : oy; }

/* ---- VRAM transfers -------------------------------------------------------------------------- */
static void cpu_to_vram(const u32 *w) {
    int dx = w[1] & 0x3FF, dy = (w[1] >> 16) & 0x1FF;
    int ww = ((int)((w[2] & 0xFFFF) - 1) & 0x3FF) + 1, hh = ((int)(((w[2] >> 16) & 0xFFFF) - 1) & 0x1FF) + 1;
    const u16 *px = (const u16 *)&w[3];
    for (int y = 0; y < hh; y++) for (int x = 0; x < ww; x++)
        s_vram[(dy + y) & 511][(dx + x) & 1023] = px[y * ww + x];
}
static void vram_to_vram(const u32 *w) {
    int sx = w[1] & 0x3FF, sy = (w[1] >> 16) & 0x1FF, dx = w[2] & 0x3FF, dy = (w[2] >> 16) & 0x1FF;
    int ww = ((int)((w[3] & 0xFFFF) - 1) & 0x3FF) + 1, hh = ((int)(((w[3] >> 16) & 0xFFFF) - 1) & 0x1FF) + 1;
    for (int y = 0; y < hh; y++) for (int x = 0; x < ww; x++)
        s_vram[(dy + y) & 511][(dx + x) & 1023] = s_vram[(sy + y) & 511][(sx + x) & 1023];
}
static void fill_rect(const u32 *w) {
    int rgb = w[0] & 0xFFFFFF; u16 c = PackColor(rgb & 0xFF, (rgb >> 8) & 0xFF, (rgb >> 16) & 0xFF);
    int dx = w[1] & 0x3F0, dy = (w[1] >> 16) & 0x1FF, ww = ((w[2] & 0x3FF) + 0xF) & ~0xF, hh = (w[2] >> 16) & 0x1FF;
    for (int y = 0; y < hh; y++) for (int x = 0; x < ww; x++) s_vram[(dy + y) & 511][(dx + x) & 1023] = c;
}
static void draw_polygon(const u32 *w) {
    u8 op = w[0] >> 24; int gouraud = (op >> 4) & 1, quad = (op >> 3) & 1, textured = (op >> 2) & 1;
    int semi = (op >> 1) & 1, raw = op & 1, nv = quad ? 4 : 3, frgb = w[0] & 0xFFFFFF;
    RVert v[4]; int idx = 1, tpage = 0, clut = 0;
    for (int i = 0; i < nv; i++) {
        if (i > 0 && gouraud) idx++;
        u32 vw = w[idx++]; int vx = vw & 0x7FF, vy = (vw >> 16) & 0x7FF;
        v[i].x = (vx & 0x400) ? vx - 0x800 : vx; v[i].y = (vy & 0x400) ? vy - 0x800 : vy;
        if (textured) { u32 uvw = w[idx++]; v[i].u = uvw & 0xFF; v[i].v = (uvw >> 8) & 0xFF;
            if (i == 0) clut = (uvw >> 16) & 0xFFFF; if (i == 1) tpage = (uvw >> 16) & 0x3FFF;
        } else { v[i].u = v[i].v = 0; }
    }
    int mr = raw ? 128 : (frgb & 0xFF), mg = raw ? 128 : ((frgb >> 8) & 0xFF), mb = raw ? 128 : ((frgb >> 16) & 0xFF);
    int abr = textured ? ((tpage >> 5) & 3) : s_drawModeAbr;
    if (quad) FillQuad(v[0], v[1], v[2], v[3], mr, mg, mb, textured, tpage, clut, semi, abr);
    else      FillTriangle(v[0], v[1], v[2], mr, mg, mb, textured, tpage, clut, semi, abr);
}

/* Word length of the GP0 command starting at w[]. -1 = unhandled -> stop. */
static int gp0_cmd_len(const u32 *w) {
    u8 op = w[0] >> 24;
    if (op == 0x00 || op == 0x01 || (op >= 0xE1 && op <= 0xE6)) return 1;   /* nop/clearcache/state */
    if (op == 0x02) return 3;                                               /* fill */
    if (op >= 0x20 && op <= 0x3F) {                                         /* polygon */
        int g = (op >> 4) & 1, q = (op >> 3) & 1, t = (op >> 2) & 1, nv = q ? 4 : 3;
        return 1 + nv + (g ? nv - 1 : 0) + (t ? nv : 0);
    }
    if (op >= 0x60 && op <= 0x7F) {                                         /* rect */
        int t = (op >> 2) & 1, sz = (op >> 3) & 3;
        return 1 + 1 + (t ? 1 : 0) + (sz == 0 ? 1 : 0);
    }
    if (op >= 0x80 && op <= 0x9F) return 4;                                 /* VRAM->VRAM */
    if (op >= 0xC0 && op <= 0xDF) return 3;                                 /* VRAM->CPU (data via readback pkts) */
    if (op >= 0xA0 && op <= 0xBF) {                                         /* CPU->VRAM: 3 + ceil(w*h/2) */
        int ww = ((int)((w[2] & 0xFFFF) - 1) & 0x3FF) + 1, hh = ((int)(((w[2] >> 16) & 0xFFFF) - 1) & 0x1FF) + 1;
        return 3 + (ww * hh + 1) / 2;
    }
    return -1;
}
static void exec_gp0(const u32 *w) {
    u8 op = w[0] >> 24;
    if (op == 0x02) fill_rect(w);
    else if (op >= 0x20 && op <= 0x3F) draw_polygon(w);
    else if (op >= 0x80 && op <= 0x9F) vram_to_vram(w);
    else if (op >= 0xA0 && op <= 0xBF) cpu_to_vram(w);
    else if (op == 0xE1) set_drawmode(w[0]); else if (op == 0xE2) set_texwindow(w[0]);
    else if (op == 0xE3) set_area_tl(w[0]); else if (op == 0xE4) set_area_br(w[0]);
    else if (op == 0xE5) set_offset(w[0]);
}

static void dump_ppm(const char *path, int x0, int y0, int w, int h) {
    FILE *f = fopen(path, "wb"); if (!f) { perror(path); return; }
    fprintf(f, "P6\n%d %d\n255\n", w, h);
    for (int y = 0; y < h; y++) for (int x = 0; x < w; x++) {
        int r, g, b; UnpackColor(s_vram[(y0 + y) & 511][(x0 + x) & 1023], &r, &g, &b);
        fputc(r, f); fputc(g, f); fputc(b, f);
    }
    fclose(f);
}

int main(int argc, char **argv) {
    if (argc < 2) { fprintf(stderr, "usage: %s <decompressed.psxgpu>\n", argv[0]); return 2; }
    FILE *f = fopen(argv[1], "rb"); if (!f) { perror(argv[1]); return 1; }
    fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET);
    u8 *data = malloc(sz); if (fread(data, 1, sz, f) != (size_t)sz) return 1; fclose(f);
    if (memcmp(data, "PSXGPUDUMPv1\0", 14) != 0) { fprintf(stderr, "bad magic\n"); return 1; }

    /* pass 1: concatenate all GPUPort0Data words into one GP0 stream; record VSync word-positions */
    u32 *gp0 = malloc(sz); size_t gp0n = 0; long *vsync = malloc(sz); int nvsync = 0;
    for (long pos = 14; pos + 4 <= sz; ) {
        u32 hdr; memcpy(&hdr, data + pos, 4); pos += 4;
        int len = hdr & 0xFFFFFF, type = (hdr >> 24) & 0xFF;
        if (type == 0x00) { memcpy(gp0 + gp0n, data + pos, (size_t)len * 4); gp0n += len; }
        else if (type == 0x02) vsync[nvsync++] = (long)gp0n;
        pos += (long)len * 4;
    }
    fprintf(stderr, "[harness] GP0 stream = %zu words, %d frames\n", gp0n, nvsync);

    /* pass 2: linear-parse the GP0 stream; dump a frame each time we cross a VSync position */
    memset(s_vram, 0, sizeof(s_vram));
    s_drawEnv.clip.x = 0; s_drawEnv.clip.y = 0; s_drawEnv.clip.w = VRAM_W; s_drawEnv.clip.h = VRAM_H;
    s_drawEnv.ofs[0] = s_drawEnv.ofs[1] = 0;
    s_twMaskX = s_twMaskY = s_twOffX = s_twOffY = 0; s_drawModeAbr = 0; s_drawModeTPage = 0;
    size_t i = 0; int frame = 0, npoly = 0;
    while (i < gp0n) {
        while (frame < nvsync && (long)i >= vsync[frame]) {
            char p[64]; snprintf(p, sizeof(p), "/tmp/harness_frame%d.ppm", frame);
            int y0 = s_drawEnv.clip.y > 16 ? s_drawEnv.clip.y - 16 : 0;
            dump_ppm(p, 0, y0, 384, 288);
            fprintf(stderr, "[harness] frame %d @word %zu: %d polys, drawarea=(%d,%d %dx%d)\n",
                    frame, i, npoly, s_drawEnv.clip.x, s_drawEnv.clip.y, s_drawEnv.clip.w, s_drawEnv.clip.h);
            frame++;
        }
        int clen = gp0_cmd_len(gp0 + i);
        if (clen <= 0) { fprintf(stderr, "[harness] unhandled op 0x%02x @word %zu — stop\n", gp0[i] >> 24, i); break; }
        u8 op = gp0[i] >> 24; if (op >= 0x20 && op <= 0x3F) npoly++;
        exec_gp0(gp0 + i); i += clen;
    }
    for (; frame < nvsync; frame++) {                 /* drain frames whose VSync is at/after stream end */
        char p[64]; snprintf(p, sizeof(p), "/tmp/harness_frame%d.ppm", frame);
        int y0 = s_drawEnv.clip.y > 16 ? s_drawEnv.clip.y - 16 : 0;
        dump_ppm(p, 0, y0, 384, 288);
        fprintf(stderr, "[harness] frame %d (final): %d polys, drawarea=(%d,%d %dx%d)\n",
                frame, npoly, s_drawEnv.clip.x, s_drawEnv.clip.y, s_drawEnv.clip.w, s_drawEnv.clip.h);
    }
    dump_ppm("/tmp/harness_vram.ppm", 0, 0, 1024, 512);
    fprintf(stderr, "[harness] done: %d polygons replayed.\n", npoly);
    return 0;
}
