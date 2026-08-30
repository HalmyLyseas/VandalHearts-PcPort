/* Regression fixtures for the LoadImage/StoreImage VRAM-bound and ParseTimSection/ReadTIM
 * plausibility-bound fixes in src/libgpu.c (codex 1.1, 1.3). Compiled and run under
 * AddressSanitizer by gpu_bounds.sh; not part of the real game build.
 *
 * This links only src/libgpu.c + this file, exactly like audio_bounds_test.c links only
 * libsnd.c/pc_spu.c/libspu.c. libgpu.c's split-out subsystem seams (pc_gpu_internal.h) --
 * pc_raster.c, pc_gpu_trace.c, pc_hdpack.c, pc_gpu_window.c -- are stubbed below rather
 * than linked in, since none of them are exercised by the LoadImage/StoreImage/ReadTIM path.
 */
#include <stdio.h>
#include <string.h>
#include "PsyQ/libgpu.h"
#include "pc_gpu_internal.h"
#include "pc_lang.h"
#include "pc_platform.h"

static int g_fail = 0;
#define CHECK(cond, msg) do { \
    if (!(cond)) { fprintf(stderr, "  FAIL: %s\n", msg); g_fail = 1; } \
} while (0)

/* ---- stubs for the externs libgpu.c normally reaches across the split TUs ----
 * s_drawFrame is defined in libgpu.c itself (pc_gpu_internal.h just declares it extern). */
volatile sig_atomic_t g_vhHiresDumpReq = 0;

static unsigned short s_vram[PC_GPU_VRAM_H][PC_GPU_VRAM_W];
unsigned short (*PC_GpuVram(void))[PC_GPU_VRAM_W] { return s_vram; }
void TPageOrigin(int tpage, int *x, int *y, int *tp) { (void)tpage; if (x) *x = 0; if (y) *y = 0; if (tp) *tp = 0; }
void UnpackColor(unsigned short c, int *r, int *g, int *b) { (void)c; if (r) *r = 0; if (g) *g = 0; if (b) *b = 0; }
void FillQuad(const RenderCtx *rc, RVert v0, RVert v1, RVert v2, RVert v3, int r, int g, int b,
              int textured, int tpage, int clut, int semiTrans, int abr) {
    (void)rc;(void)v0;(void)v1;(void)v2;(void)v3;(void)r;(void)g;(void)b;
    (void)textured;(void)tpage;(void)clut;(void)semiTrans;(void)abr;
}
void FillRect(const RenderCtx *rc, int x0, int y0, int w, int h, int r, int g, int b) {
    (void)rc;(void)x0;(void)y0;(void)w;(void)h;(void)r;(void)g;(void)b;
}
void FillRectRaw(int x0, int y0, int w, int h, int r, int g, int b) {
    (void)x0;(void)y0;(void)w;(void)h;(void)r;(void)g;(void)b;
}
void HiresEnsure(void) { }
void HiresMirrorRect(int x0, int y0, int w, int h) { (void)x0;(void)y0;(void)w;(void)h; }
void HiresFrameReset(void) { }
void HiresAppendQuad(const RenderCtx *rch, RVert a, RVert b, RVert c, RVert d,
                     int r, int g, int bcol, int textured, int tpage, int clut, int semi, int abr) {
    (void)rch;(void)a;(void)b;(void)c;(void)d;(void)r;(void)g;(void)bcol;
    (void)textured;(void)tpage;(void)clut;(void)semi;(void)abr;
}
void HiresAppendRect(const RenderCtx *rch, int x, int y, int w, int h, int r, int g, int b) {
    (void)rch;(void)x;(void)y;(void)w;(void)h;(void)r;(void)g;(void)b;
}
void HiresRasterizeThreaded(int clipY, int clipH) { (void)clipY;(void)clipH; }
int  HiresActive(void) { return 0; }
void HiresPresent(int dispX, int dispY, int dispW, int dispH) { (void)dispX;(void)dispY;(void)dispW;(void)dispH; }
void HdPack_OnLoad(const RECT *rect, const unsigned short *src) { (void)rect;(void)src; }
void TrcInit(void) { }
void TrcWrite(char op, const void *a, u32 na, const void *b, u32 nb) { (void)op;(void)a;(void)na;(void)b;(void)nb; }
void TrcPrim(int type, const void *prim) { (void)type;(void)prim; }
void TrcFrameEnd(void) { }
int  TrcReplaying(void) { return 0; }
int  PC_GpuGetInternalScale(void) { return 1; }
void PC_LangPatchFwdUpload(int px, int py, int pw, int ph, unsigned short *pix) { (void)px;(void)py;(void)pw;(void)ph;(void)pix; }
void PC_UpdateCamOsd(void) { }
void PC_GpuPresent(unsigned short *vram, int vramW, int vramH, int x, int y, int w, int h) {
    (void)vram;(void)vramW;(void)vramH;(void)x;(void)y;(void)w;(void)h;
}

/* ---- fixture (a): LoadImage/StoreImage tolerate a negative destination rect ---- */
static void Fixture1_NegativeCoordBounds(void) {
    unsigned short src[4] = { 0x1111, 0x2222, 0x3333, 0x4444 };
    unsigned short dst[4];
    RECT r;
    printf("fixture 1: LoadImage/StoreImage clip a negative x/y instead of writing/reading before vram[]\n");
    setRECT(&r, -1, 5, 2, 2);
    LoadImage(&r, (unsigned int *)src);   /* rect->x = -1: would write vram[y][-1] pre-fix */
    setRECT(&r, 5, -1, 2, 2);
    StoreImage(&r, (unsigned int *)dst);  /* rect->y = -1: would read vram[-1][x] pre-fix */
    /* Nothing to CHECK here -- an OOB access aborts under ASan before we get this far;
     * reaching this line at all is the pass condition, exactly like audio_bounds_test.c's
     * Fixture1 (the guard's silence, not a return value, is what's being proven). */
    printf("fixture 1: PASS\n");
}

/* ---- TIM blob helpers ---- */
static void BuildTim(unsigned int *tim, unsigned int mode, unsigned int clutSize,
                      unsigned int clutDest, unsigned int clutWh,
                      unsigned int pixSize, unsigned int pixDest, unsigned int pixWh) {
    tim[0] = 0x00000010;
    tim[1] = mode;
    tim[2] = clutSize; tim[3] = clutDest; tim[4] = clutWh;
    tim[5] = pixSize;  tim[6] = pixDest;  tim[7] = pixWh;
}

/* ---- fixture (b): a malformed CLUT section size rejects the whole TIM ---- */
static void Fixture2_MalformedClutSize(void) {
    unsigned int tim[8 + 4];
    TIM_IMAGE t;
    int before = g_fail;
    printf("fixture 2: a garbage CLUT section size yields prect == NULL/zero-size, no OOB parse\n");
    BuildTim(tim, 8 /* mode: type=0, has-clut */, 0xFFFFFFF0u, (0 << 16) | 1, (1 << 16) | 1,
             (3 + 4) * 4, (0 << 16) | 800, (2 << 16) | 2);
    OpenTIM(tim);
    memset(&t, 0xAA, sizeof(t));
    ReadTIM(&t);
    CHECK(t.prect == NULL || (t.prect->w == 0 && t.prect->h == 0),
          "ReadTIM leaves prect NULL or zero-size on a malformed CLUT section");
    CHECK(t.crect == NULL, "ReadTIM leaves crect NULL on a malformed CLUT section");
    printf(g_fail != before ? "fixture 2: FAIL\n" : "fixture 2: PASS\n");
}

/* ---- fixture (c): a valid minimal TIM (no CLUT) parses to the embedded rect ---- */
static void Fixture3_ValidTim(void) {
    unsigned int tim[2 + 3 + 4];
    unsigned short *pix = (unsigned short *)&tim[5];
    TIM_IMAGE t;
    int before = g_fail;
    printf("fixture 3: a valid TIM parses to its embedded destCoord/whWord rect\n");
    tim[0] = 0x00000010;
    tim[1] = 2; /* mode: type=2 (16bpp), no clut */
    tim[2] = (3 + 4) * 4;
    tim[3] = (0 << 16) | 800;
    tim[4] = (2 << 16) | 2;
    pix[0] = 0x7C00; pix[1] = 0x7C00;
    pix[2] = 0x7C00; pix[3] = 0x7C00;
    OpenTIM(tim);
    memset(&t, 0, sizeof(t));
    ReadTIM(&t);
    CHECK(t.prect != NULL && t.prect->x == 800 && t.prect->y == 0 && t.prect->w == 2 && t.prect->h == 2,
          "ReadTIM.prect reflects the TIM's own embedded destCoord/whWord");
    CHECK(t.paddr == (unsigned int *)pix, "ReadTIM.paddr points at the real pixel data");
    printf(g_fail != before ? "fixture 3: FAIL\n" : "fixture 3: PASS\n");
}

int main(void) {
    Fixture1_NegativeCoordBounds();
    Fixture2_MalformedClutSize();
    Fixture3_ValidTim();
    return g_fail;
}
