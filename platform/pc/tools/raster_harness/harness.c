/* G1 differential rasterizer harness (Phase 0b).
 *
 * Invokes the REAL production rasterizer by #including libgpu.c directly (so it always tracks the code,
 * including the upcoming DDA rewrite -- no copy, no divergence). The ~2 externals that only DrawOTag/present
 * use are stubbed; the harness calls FillQuad/FillTriangle straight, bypassing the OT walk.
 *
 * This first cut proves the unblock: compile+link libgpu.c standalone, invoke the rasterizer, dump output.
 * Next: replay the .psxgpu trace's GP0 stream into it and diff vs DuckStation's reference frame.
 *
 * Build (from repo root):
 *   cc -std=gnu99 -O2 -Iplatform/pc/include -Iinclude \
 *      platform/pc/tools/raster_harness/harness.c -lm -o /tmp/raster_harness
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* --- stubs for the DrawOTag/present-only externals (harness never calls DrawOTag) --------------- */
void PC_GpuPresent(unsigned short *vram, int vramW, int vramH, int dx, int dy, int dw, int dh) {
    (void)vram; (void)vramW; (void)vramH; (void)dx; (void)dy; (void)dw; (void)dh;
}
void PC_UpdateCamOsd(void) {}

/* Pull in the real rasterizer (statics: s_vram, FillQuad, SampleTexture, PutPixel, s_drawEnv, ...). */
#include "../../src/libgpu.c"

/* --- helpers ------------------------------------------------------------------------------------- */
static void dump_ppm(const char *path, int x0, int y0, int w, int h) {
    FILE *f = fopen(path, "wb");
    if (!f) { perror(path); return; }
    fprintf(f, "P6\n%d %d\n255\n", w, h);
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            int r, g, b; UnpackColor(s_vram[y0 + y][x0 + x], &r, &g, &b);
            fputc(r, f); fputc(g, f); fputc(b, f);
        }
    fclose(f);
    fprintf(stderr, "[harness] wrote %s (%dx%d)\n", path, w, h);
}

int main(void) {
    /* draw state: full-VRAM clip, no offset, no texture window, opaque flat */
    memset(s_vram, 0, sizeof(s_vram));
    s_drawEnv.clip.x = 0; s_drawEnv.clip.y = 0; s_drawEnv.clip.w = VRAM_W; s_drawEnv.clip.h = VRAM_H;
    s_drawEnv.ofs[0] = 0; s_drawEnv.ofs[1] = 0;
    s_twMaskX = s_twMaskY = s_twOffX = s_twOffY = 0;
    s_drawModeAbr = 0; s_drawModeTPage = 0;

    /* smoke test 1: a flat opaque quad (Z-order TL,TR,BL,BR), rgb white */
    RVert a = { 10, 10, 0, 0 }, b = { 60, 10, 0, 0 }, c = { 10, 40, 0, 0 }, d = { 60, 40, 0, 0 };
    FillQuad(a, b, c, d, 255, 255, 255, 0, 0, 0, 0, 0);
    dump_ppm("/tmp/harness_flatquad.ppm", 0, 0, 80, 60);

    fprintf(stderr, "[harness] OK -- libgpu.c rasterizer invoked standalone.\n");
    return 0;
}
