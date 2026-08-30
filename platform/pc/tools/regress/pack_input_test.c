/* Regression fixtures for the pack-input-hygiene fixes: VHCUES cue geometry validation
 * (src/pc_movie_subs.c subsLoad) and the HD image decode budget (src/pc_hdpack.c HdLoadWebp /
 * HdLoadHdi). Compiled and run by pack_input.sh; not part of the real game build. */

/* `#include`s both .c files directly to reach their `static` functions -- they share no
 * symbol/macro names, so one TU is safe. Externs pc_hdpack.c needs from the rest of the port
 * are stubbed below; the fixtures call HdLoadWebp/HdLoadHdi directly, so none of these run. */
#define VH_REGION_US
#include "../../src/pc_movie_subs.c"
#include "../../src/pc_hdpack.c"

#include <stdint.h>
#include <time.h>
#include <unistd.h>

/* ---- stubs for externs pc_hdpack.c references but the fixtures never reach ---- */
int g_vhHdPack = 0;
int PC_GetDeployDir(char *out, size_t outSize) { if (out && outSize) out[0] = '\0'; return 0; }
int PC_GpuGetInternalScale(void) { return 1; }
static unsigned short s_fakeVram[PC_GPU_VRAM_H][PC_GPU_VRAM_W];
unsigned short (*PC_GpuVram(void))[PC_GPU_VRAM_W] { return s_fakeVram; }
const char *PC_LangBgDir(void) { return NULL; }
int PC_Verbose(void) { return 0; }
void TPageOrigin(int tpage, int *x, int *y, int *tp) { (void)tpage; *x = *y = *tp = 0; }
void UnpackColor(unsigned short c, int *r, int *g, int *b) { (void)c; *r = *g = *b = 0; }

static int g_fail = 0;
#define CHECK(cond, msg) do { \
    if (!(cond)) { fprintf(stderr, "  FAIL: %s\n", msg); g_fail = 1; } \
} while (0)

static double NowMs(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1e6;
}

/* Fixture a (1.5): an out-of-range cue rejects the whole file -- the loader stays inert. */
static void FixtureA_BadCueRejected(const char *dir) {
    char path[512];
    FILE *f;
    printf("fixture a: an out-of-bounds VHCUES cue rejects the file\n");
    snprintf(path, sizeof(path), "%s/bad.txt", dir);
    f = fopen(path, "w");
    CHECK(f != NULL, "could not create fixture file");
    if (f) {
        fprintf(f, "VHCUES 1\nlba 21618\ncue 0 10 0 0 1000000 1000000\ntext hi\nend\n");
        fclose(f);
    }
    setenv("VH_MOVIE_SUBS", path, 1);
    PC_MovieSubsOpen(0x21618);
    CHECK(!PC_MovieSubsLoaded(), "loader accepted a cue outside the 320x240 frame");
    PC_MovieSubsClose();
    unsetenv("VH_MOVIE_SUBS");
    printf(g_fail ? "fixture a: FAIL\n" : "fixture a: PASS\n");
}

/* Fixture b (1.5): a valid cue still loads and shows up inside its frame range, unchanged. */
static void FixtureB_GoodCueLoads(const char *dir) {
    const PC_MovieCue *out[4];
    char path[512];
    FILE *f;
    int n, before = g_fail;
    printf("fixture b: a valid VHCUES cue still loads and activates\n");
    snprintf(path, sizeof(path), "%s/good.txt", dir);
    f = fopen(path, "w");
    CHECK(f != NULL, "could not create fixture file");
    if (f) {
        fprintf(f, "VHCUES 1\nlba 21619\ncue 10 50 0 195 320 45\ntext hello world\nend\n");
        fclose(f);
    }
    setenv("VH_MOVIE_SUBS", path, 1);
    PC_MovieSubsOpen(0x21619);
    CHECK(PC_MovieSubsLoaded(), "a valid cue file was rejected");
    PC_MovieSubsFrame(20);
    n = PC_MovieSubsActive(out, 4);
    CHECK(n == 1, "the valid cue was not active inside its own frame range");
    if (n == 1) {
        CHECK(out[0]->x == 0 && out[0]->y == 195 && out[0]->w == 320 && out[0]->h == 45,
              "active cue geometry does not match the file");
        CHECK(strcmp(out[0]->lines[0], "hello world") == 0, "active cue text does not match the file");
    }
    PC_MovieSubsClose();
    unsetenv("VH_MOVIE_SUBS");
    printf((g_fail != before) ? "fixture b: FAIL\n" : "fixture b: PASS\n");
}

/* Fixture c (1.6): a 16384x16384 .hdi header is rejected before any pixel data is touched. */
static void FixtureC_HdiOversizeRejected(const char *dir) {
    char path[512];
    unsigned char hdr[12];
    FILE *f;
    unsigned int *px;
    int w = -1, h = -1, before = g_fail;
    double t0, dt;
    printf("fixture c: an oversize .hdi header is rejected without allocating\n");
    snprintf(path, sizeof(path), "%s/0000000000000000.hdi", dir);
    memcpy(hdr, "HDI1", 4);
    hdr[4] = 0; hdr[5] = 0x40; hdr[6] = 0; hdr[7] = 0;     /* w = 16384 */
    hdr[8] = 0; hdr[9] = 0x40; hdr[10] = 0; hdr[11] = 0;   /* h = 16384 */
    f = fopen(path, "wb");
    CHECK(f != NULL, "could not create fixture file");
    if (f) { fwrite(hdr, 1, sizeof(hdr), f); fclose(f); }
    t0 = NowMs();
    px = HdLoadHdi(dir, 0, &w, &h);
    dt = NowMs() - t0;
    CHECK(px == NULL, "HdLoadHdi accepted a 16384x16384 image");
    CHECK(dt < 500.0, "HdLoadHdi took long enough to suggest a huge allocation was attempted");
    printf((g_fail != before) ? "fixture c: FAIL\n" : "fixture c: PASS\n");
}

/* Writes a raw P6 PPM of w x h pixels, every pixel (r,g,b). Row-at-a-time so a large solid
 * fixture (9000x9000) does not need a full-image buffer. */
static int WritePpmSolid(const char *path, int w, int h, unsigned char r, unsigned char g, unsigned char b) {
    FILE *f = fopen(path, "wb");
    unsigned char *row;
    int y;
    if (!f) return 0;
    fprintf(f, "P6\n%d %d\n255\n", w, h);
    row = (unsigned char *)malloc((size_t)w * 3);
    if (!row) { fclose(f); return 0; }
    for (y = 0; y < w; y++) { row[y*3] = r; row[y*3+1] = g; row[y*3+2] = b; }
    for (y = 0; y < h; y++) fwrite(row, 1, (size_t)w * 3, f);
    free(row);
    fclose(f);
    return 1;
}

/* Fixture d (1.6): a 9000x9000 WebP (over HD_MAX_SIDE) is rejected by the header budget check
 * before WebPDecodeRGBAInto ever runs -- proven the same way as fixture c, by elapsed time. */
static void FixtureD_WebpOversizeRejected(const char *dir) {
    char ppm[512], webp[512], cmd[1200];
    unsigned int *px;
    int w = -1, h = -1, before = g_fail, rc;
    double t0, dt;
    printf("fixture d: a 9000x9000 solid-colour WebP is rejected by the budget before decode\n");
    snprintf(ppm, sizeof(ppm), "%s/big.ppm", dir);
    snprintf(webp, sizeof(webp), "%s/0000000000000001.webp", dir);
    CHECK(WritePpmSolid(ppm, 9000, 9000, 200, 40, 40), "could not write the big fixture PPM");
    snprintf(cmd, sizeof(cmd), "cwebp -quiet -q 10 -m 0 '%s' -o '%s'", ppm, webp);
    rc = system(cmd);
    CHECK(rc == 0, "cwebp failed to encode the big fixture");
    t0 = NowMs();
    px = HdLoadWebp(dir, 1, &w, &h);
    dt = NowMs() - t0;
    CHECK(px == NULL, "HdLoadWebp accepted a 9000x9000 image");
    CHECK(dt < 500.0, "HdLoadWebp took long enough to suggest a huge allocation/decode was attempted");
    printf((g_fail != before) ? "fixture d: FAIL\n" : "fixture d: PASS\n");
}

/* Fixture e (1.6): a small, in-budget WebP still decodes correctly through the new
 * WebPDecodeRGBAInto path, with the right dims and exact pixel values (lossless round-trip). */
static void FixtureE_WebpRoundTrips(const char *dir) {
    char ppm[512], webp[512], cmd[1200];
    unsigned int *px;
    int w = -1, h = -1, before = g_fail, rc, x, y, mismatch = 0;
    const int W = 64, H = 64;
    FILE *f;
    printf("fixture e: a 64x64 WebP decodes to the right dims and pixels\n");
    snprintf(ppm, sizeof(ppm), "%s/small.ppm", dir);
    snprintf(webp, sizeof(webp), "%s/0000000000000002.webp", dir);
    f = fopen(ppm, "wb");
    CHECK(f != NULL, "could not create the small fixture PPM");
    if (f) {
        fprintf(f, "P6\n%d %d\n255\n", W, H);
        for (y = 0; y < H; y++)
            for (x = 0; x < W; x++) {
                unsigned char px3[3] = { (unsigned char)(x * 4), (unsigned char)(y * 4),
                                          (unsigned char)((x + y) * 2) };
                fwrite(px3, 1, 3, f);
            }
        fclose(f);
    }
    snprintf(cmd, sizeof(cmd), "cwebp -quiet -lossless '%s' -o '%s'", ppm, webp);
    rc = system(cmd);
    CHECK(rc == 0, "cwebp failed to encode the small fixture");
    px = HdLoadWebp(dir, 2, &w, &h);
    CHECK(px != NULL, "HdLoadWebp rejected an in-budget 64x64 image");
    CHECK(w == W && h == H, "decoded dims do not match the source");
    if (px && w == W && h == H) {
        const unsigned char *b = (const unsigned char *)px;
        for (y = 0; y < H && !mismatch; y++)
            for (x = 0; x < W; x++) {
                unsigned char er = (unsigned char)(x * 4), eg = (unsigned char)(y * 4),
                              eb = (unsigned char)((x + y) * 2);
                size_t i = ((size_t)y * W + x) * 4;
                if (b[i] != er || b[i+1] != eg || b[i+2] != eb || b[i+3] != 255) { mismatch = 1; break; }
            }
        CHECK(!mismatch, "decoded pixel values do not match the encoded source (lossless)");
    }
    free(px);
    printf((g_fail != before) ? "fixture e: FAIL\n" : "fixture e: PASS\n");
}

int main(void) {
    char dir[] = "/tmp/pack_input_test.XXXXXX";
    if (!mkdtemp(dir)) { perror("mkdtemp"); return 2; }

    FixtureA_BadCueRejected(dir);
    FixtureB_GoodCueLoads(dir);
    FixtureC_HdiOversizeRejected(dir);
    FixtureD_WebpOversizeRejected(dir);
    FixtureE_WebpRoundTrips(dir);

    return g_fail ? 1 : 0;
}
