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
#include <string.h>

#include "pc_platform.h"
#include "pc_overlay.h"

static SDL_Window *s_window;
static SDL_GLContext s_glCtx;
static int s_winW, s_winH;               /* actual (scaled) window size */

/* Overlay-facing video settings (Stage-3 1.2a). g_vhScale is the VH_SCALE integer factor (1..8);
 * g_vhFullscreen is 0/1. Written by PC_GpuSetScale/SetFullscreen (from the options overlay) and read
 * for display; initialised from VH_SCALE / VH_FULLSCREEN in PC_GpuInit. */
int g_vhScale = 2;
int g_vhFullscreen = 0;
/* 1.6 HD PACK toggle (VH_HDPACK). 0 until a valid pack is auto-detected; libgpu's HdDetect() then sets it
 * (persisted VH_HDPACK, or auto-ON on first detect). The overlay binds it directly; libgpu reads it. */
int g_vhHdPack = 0;
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
    /* Full uppercase A-Z + digits (above) + the punctuation the debug OSD and the options overlay
     * need. bit4 = leftmost of 5 columns. Shared by osdDrawText (debug) and PC_GpuDrawOverlay. */
    static const struct { char c; unsigned char r[7]; } LETTERS[] = {
        {'-', {0, 0, 0, 0x1F, 0, 0, 0}},        {':', {0, 0x04, 0, 0, 0, 0x04, 0}},
        {',', {0, 0, 0, 0, 0, 0x04, 0x08}},     {'(', {0x02, 0x04, 0x08, 0x08, 0x08, 0x04, 0x02}},
        {')', {0x08, 0x04, 0x02, 0x02, 0x02, 0x04, 0x08}},
        {'.', {0, 0, 0, 0, 0, 0, 0x04}},        {'/', {0x01, 0x02, 0x02, 0x04, 0x08, 0x08, 0x10}},
        {'*', {0, 0x04, 0x15, 0x0E, 0x15, 0x04, 0}},
        {'?', {0x0E, 0x11, 0x01, 0x06, 0x04, 0, 0x04}},
        /* 1.4 F2: PlayStation face-button glyphs (sentinel ASCII -> icon), used by the overlay footers:
         * '$'=Square, '@'=Circle, '^'=Triangle, '~'=Cross. Monochrome outlines (the font is 1-colour). */
        {'$', {0x1F, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1F}},   /* square */
        {'@', {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}},   /* circle */
        {'^', {0x04, 0x04, 0x0A, 0x0A, 0x11, 0x11, 0x1F}},   /* triangle */
        {'~', {0x11, 0x0A, 0x04, 0x04, 0x04, 0x0A, 0x11}},   /* cross */
        {'A', {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}},
        {'B', {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E}},
        {'C', {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E}},
        {'D', {0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E}},
        {'E', {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F}},
        {'F', {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10}},
        {'G', {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0E}},
        {'H', {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}},
        {'I', {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E}},
        {'J', {0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0C}},
        {'K', {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11}},
        {'L', {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F}},
        {'M', {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11}},
        {'N', {0x11, 0x11, 0x19, 0x15, 0x13, 0x11, 0x11}},
        {'O', {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}},
        {'P', {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10}},
        {'Q', {0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D}},
        {'R', {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11}},
        {'S', {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E}},
        {'T', {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}},
        {'U', {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}},
        {'V', {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04}},
        {'W', {0x11, 0x11, 0x11, 0x15, 0x15, 0x1B, 0x11}},
        {'X', {0x11, 0x0A, 0x04, 0x04, 0x04, 0x0A, 0x11}},
        {'Y', {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04}},
        {'Z', {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F}},
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
static void osdDrawText(int w, int h, int sx, int sy, int scale, const char *text,
                        unsigned char fr, unsigned char fg, unsigned char fb, int drawBg) {
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
                        if (lit) { o[0] = fr; o[1] = fg; o[2] = fb; }
                        else if (drawBg) { o[0] = o[0] >> 2; o[1] = o[1] >> 2; o[2] = o[2] >> 2; } /* dim */
                        /* else: leave the scene pixel untouched (transparent background) */
                    }
                }
            }
        }
    }
}

/* --- Stage-3 (1.1C) in-game options overlay renderer ---------------------------
 * Paints the pc_overlay.c menu over the presented frame, into the same bottom-up RGB scratch as the
 * debug OSD. Deliberately plain/neutral (user direction): a semi-transparent dark-grey panel, a
 * light-grey/light-blue clean bitmap font (the shared 5x7 glyphs), a light-blue selection -- nothing
 * that competes with the game's own art. Sizes itself from the longest line so it always fits. */

/* Blend (r,g,b) at alpha a/255 into the scratch pixel at screen (X,Y) (top-left origin; the scratch
 * is bottom-up, so flip Y here as osdDrawText does). Out-of-bounds is a no-op. */
static void ovlBlend(int w, int h, int X, int Y, int r, int g, int b, int a) {
    unsigned char *o;
    int fy;
    if (X < 0 || X >= w || Y < 0 || Y >= h) return;
    fy = h - 1 - Y;
    o = &s_rgbaScratch[(fy * w + X) * 3];
    o[0] = (unsigned char)((o[0] * (255 - a) + r * a) / 255);
    o[1] = (unsigned char)((o[1] * (255 - a) + g * a) / 255);
    o[2] = (unsigned char)((o[2] * (255 - a) + b * a) / 255);
}

static void ovlFillRect(int w, int h, int x, int y, int rw, int rh, int r, int g, int b, int a) {
    int dx, dy;
    for (dy = 0; dy < rh; dy++)
        for (dx = 0; dx < rw; dx++)
            ovlBlend(w, h, x + dx, y + dy, r, g, b, a);
}

/* Opaque colored text (no per-cell background -- the panel supplies contrast). 6*scale px per cell. */
static void ovlText(int w, int h, int sx, int sy, int scale, const char *text, int r, int g, int b) {
    int ci = 0;
    const char *p;
    for (p = text; *p; p++, ci++) {
        unsigned char rows[7];
        int gx = sx + ci * 6 * scale, rr, cbit, dy, dx;
        glyphRows(*p, rows);
        for (rr = 0; rr < 7; rr++)
            for (cbit = 0; cbit < 5; cbit++) {
                if (!((rows[rr] >> (4 - cbit)) & 1)) continue;
                for (dy = 0; dy < scale; dy++)
                    for (dx = 0; dx < scale; dx++)
                        ovlBlend(w, h, gx + cbit * scale + dx, sy + rr * scale + dy, r, g, b, 255);
            }
    }
}

static int ovlTextPx(const char *s, int scale) {   /* rendered width; last cell has no trailing gap */
    int n = (int)strlen(s);
    return n > 0 ? (n * 6 - 1) * scale : 0;
}

/* Shared overlay layout constants (scale-1 base units) + panel box. */
#define OVL_PADX1 8
#define OVL_PADY1 7
#define OVL_LINE1 10
#define OVL_TGAP1 6
#define OVL_GLY1  7

static void ovlPanelBox(int w, int h, int px, int py, int panelW, int panelH) {
    ovlFillRect(w, h, px, py, panelW, panelH, 26, 29, 36, 216);           /* translucent dark grey */
    ovlFillRect(w, h, px, py, panelW, 1, 92, 108, 134, 235);             /* cool-grey border */
    ovlFillRect(w, h, px, py + panelH - 1, panelW, 1, 92, 108, 134, 235);
    ovlFillRect(w, h, px, py, 1, panelH, 92, 108, 134, 235);
    ovlFillRect(w, h, px + panelW - 1, py, 1, panelH, 92, 108, 134, 235);
}

/* Largest integer scale at which a `base`-px (scale-1) panel still fits width `w` with margins. */
static int ovlFitScale(int w, int base) {
    int s = 1;
    while ((s + 1) * (base + 2 * OVL_PADX1) <= w - 8) s++;
    return s;
}

/* 1.4 F2: overlay button-label style (1 == XBOX; see libetc.c PC_ButtonLabelStyle). Only the port
 * overlay's own footers swap PlayStation symbols for Xbox letters; the game's prompts are untouched. */
extern int PC_ButtonLabelStyle(void);
extern const char *PC_OverlayItemWidestValue(int i);
#define OVL_LABELS_XBOX (PC_ButtonLabelStyle() == 1)

/* ALL overlay screens share ONE glyph scale so a sub-window (return-to-title confirm, save
 * management, detail) never renders at a different size than the main settings list it opened from.
 * The scale TRACKS the internal resolution: the presented buffer is 320 * VH_INTERNAL_SCALE wide, so
 * scaling the overlay by that same factor keeps it the SAME relative size at every internal res
 * (otherwise it stayed at scale 2 and shrank at x3/x4). The reference is the MAIN list (the widest
 * panel, and computable on every sub-screen); we clamp DOWN only in the rare case it wouldn't fit. */
static int ovlSharedScale(int w) {
    int i, count = PC_OverlayCount(), base = 0, want, fit;
    for (i = 0; i < count; i++) {
        const char *label, *wv; int lw;
        PC_OverlayItem(i, &label, NULL);
        wv = PC_OverlayItemWidestValue(i);
        lw = ovlTextPx(label, 1) + (wv ? 12 + ovlTextPx(wv, 1) : 0);
        if (lw > base) base = lw;
    }
    want = w / 320;                          /* 320 = native fb width; buffer is 320*scale wide => scale */
    if (want < 1) want = 1;
    fit = ovlFitScale(w, base);              /* cap so the main list still fits (defensive) */
    return want < fit ? want : fit;
}

/* MAIN: the settings list. */
static void drawMain(int w, int h) {
    const char *title = PC_OverlayTitle();
    int count = PC_OverlayCount(), sel = PC_OverlaySelected();
    int scale, i, base = ovlTextPx(title, 1);
    int padX, padY, lineH, titleGap, panelW, panelH, px, py, ty;
    for (i = 0; i < count; i++) {
        const char *label, *wv;
        int lw;
        PC_OverlayItem(i, &label, NULL);
        wv = PC_OverlayItemWidestValue(i);   /* size to the widest value, not the current one (1.4 F2 #4) */
        lw = ovlTextPx(label, 1) + (wv ? 12 + ovlTextPx(wv, 1) : 0);
        if (lw > base) base = lw;
    }
    scale = ovlSharedScale(w);   /* all screens use the main list's scale (consistent) */
    padX = OVL_PADX1 * scale; padY = OVL_PADY1 * scale; lineH = OVL_LINE1 * scale; titleGap = OVL_TGAP1 * scale;
    panelW = base * scale + 2 * padX;
    panelH = padY + OVL_GLY1 * scale + titleGap + count * lineH + padY;
    px = (w - panelW) / 2; py = (h - panelH) / 2;
    ovlPanelBox(w, h, px, py, panelW, panelH);
    ty = py + padY;
    ovlText(w, h, px + (panelW - ovlTextPx(title, scale)) / 2, ty, scale, title, 168, 198, 236);
    ty += OVL_GLY1 * scale + titleGap;
    for (i = 0; i < count; i++) {
        const char *label, *val;
        int selected = (i == sel), disabled = PC_OverlayItemDisabled(i);
        int lr = selected ? 122 : 200, lg = selected ? 182 : 206, lb = selected ? 240 : 214;
        if (disabled) { lr = 96; lg = 100; lb = 110; }
        PC_OverlayItem(i, &label, &val);
        if (selected) ovlFillRect(w, h, px + 3, ty - 2, panelW - 6, OVL_GLY1 * scale + 3, 48, 56, 72, 235);
        ovlText(w, h, px + padX, ty, scale, label, lr, lg, lb);
        if (val) ovlText(w, h, px + panelW - padX - ovlTextPx(val, scale), ty, scale, val, lr, lg, lb);
        ty += lineH;
    }
}

/* SAVES: the archive browser -- flat list (scrolls), a position line if long, and a 2-line legend. */
static void drawSaves(int w, int h) {
    int xbox = OVL_LABELS_XBOX;   /* PS: $=Square @=Circle ^=Triangle ~=Cross (glyph font entries) */
    const char *L1 = xbox ? "X: BACK UP   B: RESTORE"  : "$: BACK UP   @: RESTORE";
    const char *L2 = xbox ? "Y: DELETE   A: BACK"      : "^: DELETE   ~: BACK";
    const char *L3 = "START: INSPECT FILE CONTENT";  /* START is the same label on both */
    static const char *EMPTY = "(NO BACKUPS YET)";
    const int MAXVIS = 6;
    const char *title = PC_OverlayTitle();
    int count = PC_OverlaySaveCount(), sel = PC_OverlaySaveSelected();
    int scale, i, base, visN, scrollOff = 0, showPos, bodyLines;
    int padX, padY, lineH, titleGap, panelW, panelH, px, py, ty;
    char posbuf[32];

    base = ovlTextPx(title, 1);
    if (ovlTextPx(L1, 1) > base) base = ovlTextPx(L1, 1);
    if (ovlTextPx(L2, 1) > base) base = ovlTextPx(L2, 1);
    if (ovlTextPx(L3, 1) > base) base = ovlTextPx(L3, 1);
    if (count == 0 && ovlTextPx(EMPTY, 1) > base) base = ovlTextPx(EMPTY, 1);
    for (i = 0; i < count; i++) { int lw = ovlTextPx(PC_OverlaySaveLabel(i), 1); if (lw > base) base = lw; }

    visN = (count < MAXVIS) ? count : MAXVIS;
    if (count == 0) visN = 1;                            /* the EMPTY line occupies a row */
    if (count > MAXVIS) {                                /* scroll window follows the cursor */
        scrollOff = sel - MAXVIS / 2;
        if (scrollOff < 0) scrollOff = 0;
        if (scrollOff > count - MAXVIS) scrollOff = count - MAXVIS;
    }
    showPos = (count > MAXVIS) ? 1 : 0;
    bodyLines = visN + showPos + 1 /*separator*/ + 3 /*legend*/;

    scale = ovlSharedScale(w);   /* all screens use the main list's scale (consistent) */
    padX = OVL_PADX1 * scale; padY = OVL_PADY1 * scale; lineH = OVL_LINE1 * scale; titleGap = OVL_TGAP1 * scale;
    panelW = base * scale + 2 * padX;
    panelH = padY + OVL_GLY1 * scale + titleGap + bodyLines * lineH + padY;
    px = (w - panelW) / 2; py = (h - panelH) / 2;
    ovlPanelBox(w, h, px, py, panelW, panelH);
    ty = py + padY;
    ovlText(w, h, px + (panelW - ovlTextPx(title, scale)) / 2, ty, scale, title, 168, 198, 236);
    ty += OVL_GLY1 * scale + titleGap;

    if (count == 0) {
        ovlText(w, h, px + (panelW - ovlTextPx(EMPTY, scale)) / 2, ty, scale, EMPTY, 130, 134, 142);
        ty += lineH;
    } else {
        for (i = 0; i < visN; i++) {
            int idx = scrollOff + i, selected = (idx == sel);
            const char *label = PC_OverlaySaveLabel(idx);
            int lr = selected ? 122 : 200, lg = selected ? 182 : 206, lb = selected ? 240 : 214;
            if (selected) ovlFillRect(w, h, px + 3, ty - 2, panelW - 6, OVL_GLY1 * scale + 3, 48, 56, 72, 235);
            ovlText(w, h, px + padX, ty, scale, label ? label : "", lr, lg, lb);
            if (PC_OverlaySaveActive(idx))               /* mark the archive matching the current card */
                ovlText(w, h, px + panelW - padX - ovlTextPx("(*)", scale), ty, scale, "(*)", 150, 210, 160);
            ty += lineH;
        }
    }
    if (showPos) {
        snprintf(posbuf, sizeof(posbuf), "( %d / %d )", sel + 1, count);
        ovlText(w, h, px + (panelW - ovlTextPx(posbuf, scale)) / 2, ty, scale, posbuf, 130, 134, 142);
        ty += lineH;
    }
    ovlFillRect(w, h, px + padX, ty + OVL_GLY1 * scale / 2, panelW - 2 * padX, 1, 92, 108, 134, 200);
    ty += lineH;
    ovlText(w, h, px + (panelW - ovlTextPx(L1, scale)) / 2, ty, scale, L1, 150, 170, 190); ty += lineH;
    ovlText(w, h, px + (panelW - ovlTextPx(L2, scale)) / 2, ty, scale, L2, 150, 170, 190); ty += lineH;
    ovlText(w, h, px + (panelW - ovlTextPx(L3, scale)) / 2, ty, scale, L3, 150, 170, 190);
}

/* CONFIRM: a small centered prompt (message + target + options). */
static void drawConfirm(int w, int h) {
    const char *title = PC_OverlayTitle(), *msg = PC_OverlayConfirmMsg(), *tgt = PC_OverlayConfirmTarget();
    int n = PC_OverlayConfirmCount(), sel = PC_OverlayConfirmSelected();
    int scale, i, base, bodyLines, hasTgt = (tgt && tgt[0]) ? 1 : 0;
    int padX, padY, lineH, titleGap, panelW, panelH, px, py, ty;

    base = ovlTextPx(title, 1);
    if (ovlTextPx(msg, 1) > base) base = ovlTextPx(msg, 1);
    if (hasTgt && ovlTextPx(tgt, 1) > base) base = ovlTextPx(tgt, 1);
    for (i = 0; i < n; i++) { int lw = ovlTextPx(PC_OverlayConfirmOption(i), 1); if (lw > base) base = lw; }

    bodyLines = 1 /*msg*/ + hasTgt + 1 /*separator*/ + n;
    scale = ovlSharedScale(w);   /* all screens use the main list's scale (consistent) */
    padX = OVL_PADX1 * scale; padY = OVL_PADY1 * scale; lineH = OVL_LINE1 * scale; titleGap = OVL_TGAP1 * scale;
    panelW = base * scale + 2 * padX;
    panelH = padY + OVL_GLY1 * scale + titleGap + bodyLines * lineH + padY;
    px = (w - panelW) / 2; py = (h - panelH) / 2;
    ovlPanelBox(w, h, px, py, panelW, panelH);
    ty = py + padY;
    ovlText(w, h, px + (panelW - ovlTextPx(title, scale)) / 2, ty, scale, title, 168, 198, 236);
    ty += OVL_GLY1 * scale + titleGap;
    {
        int mr = 210, mg = 214, mb = 220;                       /* neutral question */
        if (PC_OverlayConfirmMsgWarn()) { mr = 236; mg = 96; mb = 96; }   /* destructive -> red */
        ovlText(w, h, px + (panelW - ovlTextPx(msg, scale)) / 2, ty, scale, msg, mr, mg, mb); ty += lineH;
    }
    if (hasTgt) {
        int tr = 150, tg = 170, tb = 190;                       /* neutral label */
        if (PC_OverlayConfirmTargetWarn()) { tr = 236; tg = 96; tb = 96; }   /* warning -> red */
        ovlText(w, h, px + (panelW - ovlTextPx(tgt, scale)) / 2, ty, scale, tgt, tr, tg, tb); ty += lineH;
    }
    ovlFillRect(w, h, px + padX, ty + OVL_GLY1 * scale / 2, panelW - 2 * padX, 1, 92, 108, 134, 200); ty += lineH;
    for (i = 0; i < n; i++) {
        int selected = (i == sel);
        const char *opt = PC_OverlayConfirmOption(i);
        int lr = selected ? 122 : 200, lg = selected ? 182 : 206, lb = selected ? 240 : 214;
        if (selected) ovlFillRect(w, h, px + 3, ty - 2, panelW - 6, OVL_GLY1 * scale + 3, 48, 56, 72, 235);
        ovlText(w, h, px + (panelW - ovlTextPx(opt, scale)) / 2, ty, scale, opt, lr, lg, lb);
        ty += lineH;
    }
}

/* DETAIL: the three regular slots inside the inspected archive. */
static void drawDetail(int w, int h) {
    const char *LEG = OVL_LABELS_XBOX ? "A: BACK" : "~: BACK";   /* ~ = PS Cross glyph */
    const char *title = PC_OverlayDetailTitle();
    int scale, i, base, bodyLines;
    int padX, padY, lineH, titleGap, panelW, panelH, px, py, ty;
    char rows[3][48];
    for (i = 0; i < 3; i++) {
        const char *cap = PC_OverlayDetailSlot(i);
        snprintf(rows[i], sizeof(rows[i]), "SLOT %d   %s", i + 1, cap ? cap : "EMPTY");
    }
    base = ovlTextPx(title, 1);
    if (ovlTextPx(LEG, 1) > base) base = ovlTextPx(LEG, 1);
    for (i = 0; i < 3; i++) { int lw = ovlTextPx(rows[i], 1); if (lw > base) base = lw; }
    bodyLines = 3 /*slots*/ + 1 /*separator*/ + 1 /*legend*/;
    scale = ovlSharedScale(w);   /* all screens use the main list's scale (consistent) */
    padX = OVL_PADX1 * scale; padY = OVL_PADY1 * scale; lineH = OVL_LINE1 * scale; titleGap = OVL_TGAP1 * scale;
    panelW = base * scale + 2 * padX;
    panelH = padY + OVL_GLY1 * scale + titleGap + bodyLines * lineH + padY;
    px = (w - panelW) / 2; py = (h - panelH) / 2;
    ovlPanelBox(w, h, px, py, panelW, panelH);
    ty = py + padY;
    ovlText(w, h, px + (panelW - ovlTextPx(title, scale)) / 2, ty, scale, title, 168, 198, 236);
    ty += OVL_GLY1 * scale + titleGap;
    for (i = 0; i < 3; i++) {
        int empty = (PC_OverlayDetailSlot(i) == NULL);
        int lr = empty ? 120 : 200, lg = empty ? 124 : 206, lb = empty ? 132 : 214;
        ovlText(w, h, px + padX, ty, scale, rows[i], lr, lg, lb);
        ty += lineH;
    }
    ovlFillRect(w, h, px + padX, ty + OVL_GLY1 * scale / 2, panelW - 2 * padX, 1, 92, 108, 134, 200); ty += lineH;
    ovlText(w, h, px + (panelW - ovlTextPx(LEG, scale)) / 2, ty, scale, LEG, 150, 170, 190);
}

void PC_GpuDrawOverlay(int w, int h) {
    switch (PC_OverlayScreen()) {
    case OVL_SCREEN_SAVES:   drawSaves(w, h);   break;
    case OVL_SCREEN_CONFIRM: drawConfirm(w, h); break;
    case OVL_SCREEN_DETAIL:  drawDetail(w, h);  break;
    default:                 drawMain(w, h);    break;
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
         * speed, no crash. */
        if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) return 0;
    }
    /* Display-resolution scaling: the game renders a native 320x240 framebuffer;
     * we open the window at native*scale and upscale the blit (nearest-neighbour,
     * so pixel art stays crisp). VH_SCALE overrides the integer factor (default 2
     * = 640x480). The window is resizable and the present path recomputes a
     * letterboxed, aspect-preserved viewport each frame, so live resize / maximise
     * also work. This is DISPLAY resolution; true INTERNAL-resolution supersampling
     * (rendering the software GPU at a denser sample rate) is the separate
     * VH_INTERNAL_SCALE / "INTERNAL RES" option (1.5/G2), resolved here so the
     * overlay setting is valid before the first frame. */
    {
        int scale = 2;
        const char *env = getenv("VH_SCALE");
        const char *fs, *isc;
        if (env) { scale = atoi(env); if (scale < 1) scale = 1; if (scale > 8) scale = 8; }
        width  *= scale;
        height *= scale;
        g_vhScale = scale;
        fs = getenv("VH_FULLSCREEN");
        g_vhFullscreen = (fs && fs[0] == '1') ? 1 : 0;
        isc = getenv("VH_INTERNAL_SCALE");
        PC_GpuSetInternalScale(isc ? atoi(isc) : 1);   /* resolve g_vhInternalScale (1 = off) */
    }
    s_window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                 width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!s_window) return 0;
    s_winW = width; s_winH = height;   /* the real (scaled) window size, for PC_GpuGetWindowSize */
    if (g_vhFullscreen)                /* apply the persisted fullscreen preference at startup */
        SDL_SetWindowFullscreen(s_window, SDL_WINDOW_FULLSCREEN_DESKTOP);
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
    if (scale) *scale = g_vhScale;
}

/* Modal error dialog for fatal, user-actionable failures (PC_FatalDiscError). SDL documents this as
 * callable at ANY time, even before SDL_Init -- which matters, because disc validation fails before
 * the window exists. On Linux this is what a double-click AppImage launch shows instead of dying
 * silently with the message only on a terminal nobody opened; headless runs (SDL_VIDEODRIVER=dummy,
 * regress harness) just get SDL's error return, which we ignore -- stderr already has the text. */
void PC_ShowErrorBox(const char *title, const char *body) {
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title, body, NULL);
}

/* OS-agnostic online CPU count for the threaded rasterizer -- see pc_platform.h. SDL abstracts the
 * per-platform query (sysconf on POSIX, GetSystemInfo on Windows), so libgpu.c stays SDL-free and
 * needs no #ifdef. Never returns < 1. */
int PC_CpuCount(void) {
    int n = SDL_GetCPUCount();
    return (n > 0) ? n : 1;
}

/* Stage-3 (1.2a) options-overlay setters. The present path already re-letterboxes to any window size
 * each frame, so these just resize / toggle the window -- no render changes. */
void PC_GpuSetScale(int scale) {
    if (scale < 1) { scale = 1; } if (scale > 8) { scale = 8; }
    g_vhScale = scale;
    s_winW = 320 * scale;   /* native 320x240 framebuffer * scale (see PC_GpuInit) */
    s_winH = 240 * scale;
    /* Resize the windowed size. While fullscreen this has no visible effect; it takes hold when
     * fullscreen is turned back off (SDL restores this size). */
    if (s_window && !g_vhFullscreen) SDL_SetWindowSize(s_window, s_winW, s_winH);
}

void PC_GpuSetFullscreen(int on) {
    g_vhFullscreen = on ? 1 : 0;
    if (!s_window) return;
    SDL_SetWindowFullscreen(s_window, g_vhFullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
    if (!g_vhFullscreen) SDL_SetWindowSize(s_window, s_winW, s_winH);   /* restore the windowed scale */
}

/* Fullscreen movie (MDEC/FMV) overlay. When a .STR movie is playing, libcd decodes each frame
 * to a 320x240 BGR555 buffer and registers it here; PC_GpuPresent then shows that instead of the
 * VRAM region, sidestepping the movie's 24bpp VRAM packing (the present path reads 16bpp only). */
static const unsigned short *s_movieOverlay = NULL;
static const unsigned char  *s_movieOverlayRGB = NULL;   /* 1.6 HD video: direct RGB24 (w*h*3, top-down) */
static int s_movieOvW = 0, s_movieOvH = 0;
void PC_GpuSetMovieOverlay(const unsigned short *bgr555, int w, int h) {
    s_movieOverlay = bgr555; s_movieOverlayRGB = NULL; s_movieOvW = w; s_movieOvH = h;
}
/* 1.6 HD FMV: present a decoded HD frame directly at 24-bit (no 15-bit banding). Mutually exclusive with
 * the native BGR555 overlay; the frame is scaled to the window aspect-preserved like the native one. */
void PC_GpuSetMovieOverlayRGB(const unsigned char *rgb, int w, int h) {
    s_movieOverlayRGB = rgb; s_movieOverlay = NULL; s_movieOvW = w; s_movieOvH = h;
}

/* VH_PRESENT_TIME=1: phase timing for the present path (convert / OSD / GL submit+swap), mean over
 * 120-frame windows -- the raster has VH_RASTER_TIME, this is its display-side counterpart. */
static double presentNowMs(void) {
    return (double)SDL_GetPerformanceCounter() * 1000.0 / (double)SDL_GetPerformanceFrequency();
}

void PC_GpuPresent(unsigned short *vram, int vramW, int vramH,
                    int x, int y, int w, int h) {
    int px, py;
    static int s_ptTime = -1; static double s_ptConv, s_ptOsd, s_ptGl; static unsigned s_ptN;
    double pt0 = 0, pt1 = 0, pt2 = 0, pt3 = 0;
    (void)vramH;

    if (!s_window || !s_glCtx) return; /* headless: no-op */
    if (s_ptTime < 0) s_ptTime = getenv("VH_PRESENT_TIME") ? 1 : 0;
    if (s_ptTime) pt0 = presentNowMs();

    /* A movie is playing -> present its decoded frame as a fullscreen overlay (native BGR555, or HD RGB24). */
    if ((s_movieOverlay || s_movieOverlayRGB) && s_movieOvW > 0 && s_movieOvH > 0) {
        vram = (unsigned short *)s_movieOverlay;   /* NULL on the RGB path (used only by the BGR555 convert) */
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

    if (s_movieOverlayRGB) {
        /* HD FMV: already RGB24 -- just flip rows to match the BGR555 path's vertical flip (glRasterPos
         * draws bottom-up). One memcpy per row. */
        for (py = 0; py < h; py++)
            memcpy(&s_rgbaScratch[(h - 1 - py) * w * 3], &s_movieOverlayRGB[py * w * 3], (size_t)w * 3);
    } else
    for (py = 0; py < h; py++) {
        for (px = 0; px < w; px++) {
            unsigned short c = vram[(y + py) * vramW + (x + px)];
            unsigned char *out = &s_rgbaScratch[((h - 1 - py) * w + px) * 3];
            /* Display expansion 5-bit -> 8-bit by BIT-REPLICATION ((v<<3)|(v>>2)), matching real
             * hardware / DuckStation's output: a full 5-bit channel (31) maps to 255, not 248, so
             * whites reach full brightness. Purely a screen-output convention -- the internal render
             * (s_vram, UnpackColor, blends) stays native 5-bit and is unaffected. */
            int r5 = c & 0x1F, g5 = (c >> 5) & 0x1F, b5 = (c >> 10) & 0x1F;
            out[0] = (unsigned char)((r5 << 3) | (r5 >> 2));
            out[1] = (unsigned char)((g5 << 3) | (g5 >> 2));
            out[2] = (unsigned char)((b5 << 3) | (b5 >> 2));
        }
    }

    if (s_ptTime) pt1 = presentNowMs();

    {
        static int s_osdEnabled = -1;
        if (s_osdEnabled < 0) s_osdEnabled = (getenv("VH_CAM_OSD") != NULL) ? 1 : 0;
        if (s_osdEnabled && g_camOsdText[0]) osdDrawText(w, h, 2, 2, 1, g_camOsdText, 0x20, 0xFF, 0x20, 1);
    }

    /* Stage-3 (1.4 F1): battle fast-forward readout. PC_BattleSpeedGet() returns 1 outside a battle, so
     * the "BATTLE SPEED X2" tag only shows while fast-forward is active. Uppercase (the OSD font has no
     * lowercase glyphs). Top-right, scale 2. */
    {
        extern int PC_BattleSpeedGet(void);
        int spd = PC_BattleSpeedGet();
        if (spd > 1) {
            static const char lbl[] = "BATTLE SPEED X";
            char buf[sizeof(lbl) + 1];
            int i = 0;
            for (; lbl[i]; i++) buf[i] = lbl[i];
            buf[i++] = (char)('0' + spd);
            buf[i] = '\0';
            osdDrawText(w, h, w - i * 6 * 2 - 4, 4, 2, buf, 0xFF, 0xFF, 0xFF, 0); /* white, transparent bg */
        }
    }

    if (PC_OverlayIsOpen()) PC_GpuDrawOverlay(w, h);   /* Stage-3 (1.1C) options overlay, on top */

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
    if (s_ptTime) pt2 = presentNowMs();
    SDL_GL_SwapWindow(s_window);
    if (s_ptTime) {
        pt3 = presentNowMs();
        s_ptConv += pt1 - pt0; s_ptOsd += pt2 - pt1; s_ptGl += pt3 - pt2;
        if (++s_ptN >= 120) {
            fprintf(stderr, "[present] convert=%.2f osd+ovl=%.2f gl+swap=%.2f total=%.2f ms/frame (%u frames)\n",
                    s_ptConv / s_ptN, s_ptOsd / s_ptN, s_ptGl / s_ptN,
                    (s_ptConv + s_ptOsd + s_ptGl) / s_ptN, s_ptN);
            s_ptConv = s_ptOsd = s_ptGl = 0; s_ptN = 0;
        }
    }
}
