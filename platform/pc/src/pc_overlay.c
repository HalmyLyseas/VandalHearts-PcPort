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
#include "pc_platform.h"   /* PC_SaveIniConfig */
#include <stddef.h>

/* The live settings, defined in libetc.c and read every frame by the right-stick camera mapping.
 * The overlay writes them directly, so a change takes effect on the very next pad read. */
extern int g_camInvertX;
extern int g_camInvertY;

enum { OVL_TOGGLE, OVL_ACTION };

typedef struct {
    const char *label;
    int         kind;
    int        *value;                      /* OVL_TOGGLE: the live setting              */
    const char *iniSection, *iniKey;        /* OVL_TOGGLE: where to persist it           */
    const char *offText, *onText;           /* OVL_TOGGLE: value labels (0 / 1)          */
    void      (*action)(void);              /* OVL_ACTION: run on activate               */
} Item;

/* A global's address is a compile-time constant, so this const table with &g_camInvert* is valid. */
static const Item s_items[] = {
    { "CAMERA X-AXIS", OVL_TOGGLE, &g_camInvertX, "camera", "VH_CAM_INVERT_X",
      "NORMAL", "INVERTED", NULL },
    { "CAMERA Y-AXIS", OVL_TOGGLE, &g_camInvertY, "camera", "VH_CAM_INVERT_Y",
      "NORMAL", "INVERTED", NULL },
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
    if (it->kind == OVL_TOGGLE && it->iniKey)
        PC_SaveIniConfig(it->iniSection, it->iniKey, *it->value ? "1" : "0");
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
        if (*it->value != nv) { *it->value = nv; persist(it); }
    }
}

void PC_OverlayActivate(void) {
    const Item *it;
    if (!s_open) return;
    it = &s_items[s_sel];
    if (it->kind == OVL_TOGGLE) {
        *it->value = !*it->value;
        persist(it);
    } else if (it->action) {
        it->action();
    }
}

void PC_OverlayCancel(void) {
    if (s_open) s_open = 0;
}

int PC_OverlayItem(int i, const char **label, const char **valueText) {
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
    if (valueText) *valueText = NULL;
    return 0;
}
