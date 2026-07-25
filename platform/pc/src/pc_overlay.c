/*
 * Stage-3 (1.1C) in-game options overlay -- menu model + state. See pc_overlay.h for the design.
 *
 * The item list is data-driven so 1.2 (save mgmt, window scale/fullscreen) and 1.3 (mode select)
 * slot in by adding rows -- no changes to the input or render hooks. 1.1C ships the two right-stick
 * axis-invert toggles. Toggle changes apply immediately (the settings are the live globals the pad
 * mapping reads) and persist right away to vandalhearts.ini via PC_SaveIniConfig.
 *
 * There is deliberately NO "Close" item: the SELECT+START chord is the sole show/hide trigger. A
 * Close item would have to answer to a face button (Cross/Circle), and that button -- still held on
 * the closing frame -- leaks through to the game behind (e.g. Circle would pop the battle menu). The
 * chord self-masks (SELECT gates START), and libetc.c's filter additionally swallows any button held
 * at close until it is released, so no close path leaks. Nothing is lost: changes are instant + saved.
 */
#include "pc_overlay.h"
#include "pc_platform.h"   /* PC_SaveIniConfig, PC_GpuSetScale/SetFullscreen, g_vhScale/g_vhFullscreen */
#include <stddef.h>
#include <stdio.h>         /* snprintf (CHOICE value formatting) */

/* Live settings the overlay reads/writes. Camera invert (libetc.c) is read every frame by the pad
 * mapping. Video (pc_gpu_window.c) is applied via the apply() callback below. */
extern int g_camInvertX;
extern int g_camInvertY;

enum { OVL_TOGGLE, OVL_ACTION, OVL_CHOICE };

typedef struct {
    const char *label;
    int         kind;
    int        *value;                      /* TOGGLE/CHOICE: the live setting            */
    const char *iniSection, *iniKey;        /* TOGGLE/CHOICE: where to persist it         */
    const char *offText, *onText;           /* TOGGLE: value labels (0 / 1)               */
    int         minv, maxv, step;           /* CHOICE: inclusive range + increment        */
    const char *prefix;                     /* CHOICE: display prefix, e.g. "X" -> "X3"    */
    void      (*apply)(int);                /* TOGGLE/CHOICE: apply the new value (side effect); may be NULL */
    void      (*action)(void);              /* OVL_ACTION: run on activate                */
    int       (*disabled)(void);            /* optional: 1 => greyed (inactive right now); may be NULL */
} Item;

/* Window scale and fullscreen are two mutually-exclusive display modes; the INACTIVE one is greyed.
 * WINDOW SCALE dims while fullscreen is on (desktop res wins); FULLSCREEN dims while windowed. Both
 * stay selectable -- touching the greyed one switches to that mode (scale change also drops
 * fullscreen, see the cross-item rule in setValue). */
static int dis_whenFullscreen(void) { return g_vhFullscreen; }
static int dis_whenWindowed(void)   { return !g_vhFullscreen; }

/* A global's address is a compile-time constant, so this const table with &g_* is valid. */
static const Item s_items[] = {
    { "WINDOW SCALE",  OVL_CHOICE, &g_vhScale,      "video",  "VH_SCALE",
      NULL, NULL,               1, 8, 1, "X",  PC_GpuSetScale,      NULL, dis_whenFullscreen },
    { "FULLSCREEN",    OVL_TOGGLE, &g_vhFullscreen, "video",  "VH_FULLSCREEN",
      "OFF", "ON",              0, 0, 0, NULL, PC_GpuSetFullscreen, NULL, dis_whenWindowed },
    { "CAMERA X-AXIS", OVL_TOGGLE, &g_camInvertX,   "camera", "VH_CAM_INVERT_X",
      "NORMAL", "INVERTED",     0, 0, 0, NULL, NULL,                NULL, NULL },
    { "CAMERA Y-AXIS", OVL_TOGGLE, &g_camInvertY,   "camera", "VH_CAM_INVERT_Y",
      "NORMAL", "INVERTED",     0, 0, 0, NULL, NULL,                NULL, NULL },
};
#define N_ITEMS ((int)(sizeof(s_items) / sizeof(s_items[0])))

static int s_open = 0;
static int s_sel  = 0;

int  PC_OverlayIsOpen(void)   { return s_open; }
int  PC_OverlayCount(void)    { return N_ITEMS; }
int  PC_OverlaySelected(void) { return s_sel; }
const char *PC_OverlayTitle(void) { return "OPTIONS"; }

void PC_OverlayToggle(void) {
    s_open = !s_open;
    if (s_open) s_sel = 0;                   /* always reopen at the top */
}

static void persist(const Item *it) {
    char buf[16];
    if (!it->iniKey) return;
    if (it->kind == OVL_CHOICE) {
        snprintf(buf, sizeof(buf), "%d", *it->value);
        PC_SaveIniConfig(it->iniSection, it->iniKey, buf);
    } else if (it->kind == OVL_TOGGLE) {
        PC_SaveIniConfig(it->iniSection, it->iniKey, *it->value ? "1" : "0");
    }
}

/* Set the item's value to nv: the apply() callback (if any) owns writing *value + its side effect;
 * otherwise write *value directly. Then persist, and apply any cross-item rule. */
static void setValue(const Item *it, int nv) {
    if (it->apply) it->apply(nv);
    else           *it->value = nv;
    persist(it);
    /* Cross-item rule: changing the window scale drops fullscreen (so the new scale is actually
     * visible), and persists that too. Pointer-guarded to the scale item; recurses only one level
     * (the fullscreen item carries no such rule). PC_GpuSetScale already stored the new windowed
     * size, so exiting fullscreen restores the window at the chosen scale. */
    if (it->value == &g_vhScale && g_vhFullscreen) {
        int i;
        for (i = 0; i < N_ITEMS; i++)
            if (s_items[i].value == &g_vhFullscreen) { setValue(&s_items[i], 0); break; }
    }
}

void PC_OverlayMove(int delta) {
    if (!s_open) return;
    s_sel += delta;
    if (s_sel < 0)         s_sel = N_ITEMS - 1;   /* wrap */
    if (s_sel >= N_ITEMS)  s_sel = 0;
}

void PC_OverlayAdjust(int delta) {
    const Item *it;
    if (!s_open) return;
    it = &s_items[s_sel];
    if (it->kind == OVL_TOGGLE) {
        int nv = (delta < 0) ? 0 : 1;             /* left = off, right = on */
        if (*it->value != nv) setValue(it, nv);
    } else if (it->kind == OVL_CHOICE) {
        int nv = *it->value + delta * it->step;   /* left/right steps within [minv, maxv] */
        if (nv < it->minv) nv = it->minv;
        if (nv > it->maxv) nv = it->maxv;
        if (nv != *it->value) setValue(it, nv);
    }
}

void PC_OverlayActivate(void) {
    const Item *it;
    if (!s_open) return;
    it = &s_items[s_sel];
    if (it->kind == OVL_TOGGLE) {
        setValue(it, !*it->value);
    } else if (it->kind == OVL_CHOICE) {
        int nv = *it->value + it->step;           /* cycle forward, wrap to min */
        if (nv > it->maxv) nv = it->minv;
        setValue(it, nv);
    } else if (it->action) {
        it->action();
    }
}

void PC_OverlayCancel(void) {
    if (s_open) s_open = 0;
}

int PC_OverlayItem(int i, const char **label, const char **valueText) {
    static char vbuf[24];       /* CHOICE formatting; consumed by the caller before the next call */
    const Item *it;
    if (i < 0 || i >= N_ITEMS) {
        if (label) *label = "";
        if (valueText) *valueText = NULL;
        return 0;
    }
    it = &s_items[i];
    if (label) *label = it->label;
    if (it->kind == OVL_TOGGLE) {
        if (valueText) *valueText = *it->value ? it->onText : it->offText;
        return 1;
    }
    if (it->kind == OVL_CHOICE) {
        if (valueText) {
            snprintf(vbuf, sizeof(vbuf), "%s%d", it->prefix ? it->prefix : "", *it->value);
            *valueText = vbuf;
        }
        return 1;
    }
    if (valueText) *valueText = NULL;
    return 0;
}

/* 1 if item i is currently greyed/inactive (e.g. WINDOW SCALE while fullscreen). Renderer dims it. */
int PC_OverlayItemDisabled(int i) {
    if (i < 0 || i >= N_ITEMS) return 0;
    return s_items[i].disabled ? s_items[i].disabled() : 0;
}
