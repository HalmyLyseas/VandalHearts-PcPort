/* SDL2+OpenGL windowing glue for the GPU backend -- kept separate from
 * libgpu.c (the actual VRAM/rasterizer/OT hardware model) the same way
 * libspu.c is kept separate from libsnd.c: one file understands PSX
 * semantics, the other understands the host windowing API. Deliberately
 * minimal (glDrawPixels, no shaders/VBOs) -- this only needs to prove the
 * SDL2+OpenGL presentation path works, matching the project's Pad/VSync POC
 * precedent of proving the mechanism before building it out further. */
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <stdlib.h>

#include "pc_platform.h"

static SDL_Window *s_window;
static SDL_GLContext s_glCtx;
static int s_winW, s_winH, s_winScale;   /* actual (scaled) window size + the VH_SCALE factor used */
static unsigned char *s_rgbaScratch;
static int s_scratchCap;

/* --- Debug camera OSD (feedback-11 follow-up) ------------------------------
 * A tiny self-contained 5x7 bitmap font (subset: 0-9, '-', ' ', ':', ',', '(',
 * ')', and the letters used in the label) blitted straight into s_rgbaScratch
 * before glDrawPixels, so no GL text stack is needed. Enabled via VH_CAM_OSD. */
char g_camOsdText[96] = "";

/* Each glyph: 7 rows, low 5 bits = columns (bit4 = leftmost). */
static const unsigned char FONT_UNKNOWN[7] = {0x1F, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1F};
static const unsigned char FONT_DIGIT[10][7] = {
    {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E}, {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E},
    {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F}, {0x1F, 0x02, 0x04, 0x02, 0x01, 0x11, 0x0E},
    {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02}, {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E},
    {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E}, {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08},
    {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E}, {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C},
};
static void glyphRows(char c, unsigned char out[7]) {
    static const struct { char c; unsigned char r[7]; } LETTERS[] = {
        {'-', {0, 0, 0, 0x1F, 0, 0, 0}},        {':', {0, 0x04, 0, 0, 0, 0x04, 0}},
        {',', {0, 0, 0, 0, 0, 0x04, 0x08}},     {'(', {0x02, 0x04, 0x08, 0x08, 0x08, 0x04, 0x02}},
        {')', {0x08, 0x04, 0x02, 0x02, 0x02, 0x04, 0x08}},
        {'P', {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10}},
        {'I', {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E}},
        {'T', {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}},
        {'C', {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E}},
        {'H', {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}},
        {'Y', {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04}},
        {'A', {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}},
        {'W', {0x11, 0x11, 0x11, 0x15, 0x15, 0x1B, 0x11}},
        {'X', {0x11, 0x0A, 0x04, 0x04, 0x04, 0x0A, 0x11}},
        {'Z', {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F}},
        {'O', {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}},
        {'S', {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E}},
        {'F', {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10}},
        {'R', {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11}},
        {'D', {0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E}},
    };
    int i;
    if (c >= '0' && c <= '9') {
        for (i = 0; i < 7; i++) out[i] = FONT_DIGIT[c - '0'][i];
        return;
    }
    if (c == ' ') { for (i = 0; i < 7; i++) out[i] = 0; return; }
    for (i = 0; i < (int)(sizeof(LETTERS) / sizeof(LETTERS[0])); i++) {
        if (LETTERS[i].c == c) { int j; for (j = 0; j < 7; j++) out[j] = LETTERS[i].r[j]; return; }
    }
    for (i = 0; i < 7; i++) out[i] = FONT_UNKNOWN[i];
}

/* Draw `text` into the RGB scratch at screen (sx,sy) top-left, `scale` px per
 * font cell. Scratch is stored bottom-up (row 0 = bottom), so we flip Y here.
 * Draws a black cell behind lit pixels for contrast against the scene. */
static void osdDrawText(int w, int h, int sx, int sy, int scale, const char *text) {
    int ci = 0;
    const char *p;
    for (p = text; *p; p++, ci++) {
        unsigned char rows[7];
        int gx = sx + ci * 6 * scale;
        int r, cbit, dy, dx;
        glyphRows(*p, rows);
        for (r = 0; r < 7; r++) {
            for (cbit = 0; cbit < 5; cbit++) {
                int lit = (rows[r] >> (4 - cbit)) & 1;
                for (dy = 0; dy < scale; dy++) {
                    for (dx = 0; dx < scale; dx++) {
                        int X = gx + cbit * scale + dx;
                        int Y = sy + r * scale + dy;
                        int fy;
                        unsigned char *o;
                        if (X < 0 || X >= w || Y < 0 || Y >= h) continue;
                        fy = h - 1 - Y;
                        o = &s_rgbaScratch[(fy * w + X) * 3];
                        if (lit) { o[0] = 0x20; o[1] = 0xFF; o[2] = 0x20; } /* bright green */
                        else { o[0] = o[0] >> 2; o[1] = o[1] >> 2; o[2] = o[2] >> 2; } /* dim bg */
                    }
                }
            }
        }
    }
}

int PC_GpuInit(int width, int height, const char *title) {
#if defined(_WIN32)
    /* The Windows build defines SDL_MAIN_HANDLED (the real entry point is the game's own main(),
     * not SDL_main), so SDL's normal startup hook never runs -- tell SDL we've handled main before
     * the first init, or SDL_Init warns and skips some Windows-specific setup. Idempotent/harmless
     * if the window is re-opened. */
    SDL_SetMainReady();
#endif
    if (SDL_WasInit(SDL_INIT_VIDEO) == 0) {
        /* No forced video driver -- SDL auto-picks (Wayland on a Wayland session, X11 on X11,
         * "windows"/"cocoa" elsewhere). An explicit SDL_VIDEODRIVER env var still overrides.
         *
         * HISTORY (removed Stage 2.4, 2026-07-23): early in the project we force-set "x11" because
         * SDL2's native Wayland backend crashed during EGL setup ("Proxy and queue point to
         * different wl_displays") -- but ONLY while pc_bootstrap.c mapped page 0
         * (PSX_NULL_MIRROR_BASE) to absorb transient NULL reads. Stage 2.2/2.3 removed that mapping
         * (NULL reads are handled by per-site PC_PORT guards + the fault handler, no page-0 mapping),
         * so the trigger is gone: verified by running native Wayland with the current build, full
         * speed, no crash. The one case that could still hit it is the legacy VH_NULL_FIXUP=0 path
         * (which re-maps page 0 and needs setcap) -- such a user on Wayland can set
         * SDL_VIDEODRIVER=x11 themselves. See exchange/12-phase-c-bootstrap.md for the old derivation. */
        if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) return 0;
    }
    /* Display-resolution scaling: the game renders a native 320x240 framebuffer;
     * we open the window at native*scale and upscale the blit (nearest-neighbour,
     * so pixel art stays crisp). VH_SCALE overrides the integer factor (default 2
     * = 640x480). The window is resizable and the present path recomputes a
     * letterboxed, aspect-preserved viewport each frame, so live resize / maximise
     * also work. This is DISPLAY resolution only -- true internal-resolution
     * upscaling would mean rendering the 3D at higher density in the software GPU,
     * a much larger change with little benefit for this sprite/UI-heavy game. */
    {
        int scale = 2;
        const char *env = getenv("VH_SCALE");
        if (env) { scale = atoi(env); if (scale < 1) scale = 1; if (scale > 8) scale = 8; }
        width  *= scale;
        height *= scale;
        s_winScale = scale;
    }
    s_window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                 width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!s_window) return 0;
    s_winW = width; s_winH = height;   /* the real (scaled) window size, for PC_GpuGetWindowSize */
    s_glCtx = SDL_GL_CreateContext(s_window);
    if (!s_glCtx) { SDL_DestroyWindow(s_window); s_window = NULL; return 0; }
    /* Explicitly OFF, not on. The game's own VSync() (src/libetc.c) already
     * paces frames in software, matching real hardware's ~60Hz timing --
     * this project's own reference PC port of another PS1 decomp
     * (CTR-native, vandalHearts_decomp/ctr-native/platform/native_platform.c)
     * hit this exact question and documented why they land on interval 0:
     * "some GL drivers charge that wait to the next frame's first clear
     * instead of SDL_GL_SwapWindow" -- a second, driver-side wait stacked
     * on top of the game's own pacing rather than replacing it, landing at
     * an unpredictable point in the frame. Particularly relevant here since
     * this build runs on llvmpipe (software Mesa, confirmed via the
     * "driver (null)" EGL warnings at startup) rather than a real GPU
     * driver, where swap-interval behavior is known to be inconsistent
     * under a compositor. */
    SDL_GL_SetSwapInterval(0);
    glViewport(0, 0, width, height);
    return 1;
}

/* Report the actual window size (native 320x240 * VH_SCALE) and the scale factor used, so callers
 * (e.g. the bootstrap log) don't have to re-derive VH_SCALE or duplicate its clamping. Any out-ptr
 * may be NULL. Valid only after a successful PC_GpuInit; zero-initialised otherwise. */
void PC_GpuGetWindowSize(int *w, int *h, int *scale) {
    if (w)     *w = s_winW;
    if (h)     *h = s_winH;
    if (scale) *scale = s_winScale;
}

/* Fullscreen movie (MDEC/FMV) overlay. When a .STR movie is playing, libcd decodes each frame
 * to a 320x240 BGR555 buffer and registers it here; PC_GpuPresent then shows that instead of the
 * VRAM region, sidestepping the movie's 24bpp VRAM packing (the present path reads 16bpp only). */
static const unsigned short *s_movieOverlay = NULL;
static int s_movieOvW = 0, s_movieOvH = 0;
void PC_GpuSetMovieOverlay(const unsigned short *bgr555, int w, int h) {
    s_movieOverlay = bgr555; s_movieOvW = w; s_movieOvH = h;
}

void PC_GpuPresent(unsigned short *vram, int vramW, int vramH,
                    int x, int y, int w, int h) {
    int px, py;
    (void)vramH;

    if (!s_window || !s_glCtx) return; /* headless: no-op */

    /* A movie is playing -> present its decoded frame as a fullscreen overlay. */
    if (s_movieOverlay && s_movieOvW > 0 && s_movieOvH > 0) {
        vram = (unsigned short *)s_movieOverlay;
        vramW = s_movieOvW; x = 0; y = 0; w = s_movieOvW; h = s_movieOvH;
    }

    /* Drain the SDL event queue so the process is actually closeable. Initializing
     * SDL_INIT_VIDEO installs SDL's own SIGINT/SIGTERM handlers, which translate
     * Ctrl+C into an SDL_QUIT event instead of terminating the process; the window
     * manager's close button posts the same SDL_QUIT. Because nothing had ever
     * polled the queue, both were swallowed and the process could only be killed
     * externally. Handle SDL_QUIT and Escape by exiting cleanly. (SDL_GetKeyboardState
     * in PadRead still works -- polling here also refreshes that internal key state.) */
    {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT ||
                (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_ESCAPE)) {
                exit(0);
            }
        }
    }

    if (w <= 0 || h <= 0) return;

    if (s_scratchCap < w * h * 3) {
        free(s_rgbaScratch);
        s_rgbaScratch = malloc((size_t)w * h * 3);
        s_scratchCap = w * h * 3;
    }
    if (!s_rgbaScratch) return;

    for (py = 0; py < h; py++) {
        for (px = 0; px < w; px++) {
            unsigned short c = vram[(y + py) * vramW + (x + px)];
            unsigned char *out = &s_rgbaScratch[((h - 1 - py) * w + px) * 3];
            out[0] = (unsigned char)((c & 0x1F) << 3);
            out[1] = (unsigned char)(((c >> 5) & 0x1F) << 3);
            out[2] = (unsigned char)(((c >> 10) & 0x1F) << 3);
        }
    }

    {
        static int s_osdEnabled = -1;
        if (s_osdEnabled < 0) s_osdEnabled = (getenv("VH_CAM_OSD") != NULL) ? 1 : 0;
        if (s_osdEnabled && g_camOsdText[0]) osdDrawText(w, h, 2, 2, 1, g_camOsdText);
    }

    /* Scale the native w*h framebuffer up to fill the current window, preserving
     * the source aspect ratio (letterboxed with black bars). Recomputed every
     * frame so window resize/maximise is handled without extra event plumbing. */
    {
        int winW = w, winH = h, vpW, vpH, vpX, vpY;
        float srcAspect = (float)w / (float)h;
        SDL_GL_GetDrawableSize(s_window, &winW, &winH);
        if (winW < 1) winW = 1;
        if (winH < 1) winH = 1;
        vpW = winW;
        vpH = (int)(winW / srcAspect + 0.5f);
        if (vpH > winH) { vpH = winH; vpW = (int)(winH * srcAspect + 0.5f); }
        vpX = (winW - vpW) / 2;
        vpY = (winH - vpH) / 2;

        glViewport(0, 0, winW, winH);
        glClear(GL_COLOR_BUFFER_BIT);          /* black letterbox bars */
        glViewport(vpX, vpY, vpW, vpH);
        glPixelZoom((float)vpW / (float)w, (float)vpH / (float)h);
        glRasterPos2f(-1.0f, -1.0f);           /* bottom-left of the viewport */
        glDrawPixels(w, h, GL_RGB, GL_UNSIGNED_BYTE, s_rgbaScratch);
    }
    SDL_GL_SwapWindow(s_window);
}
