/*
 * Stage-3 in-game options overlay -- model/state. PC-side, backend-level (zero src/ changes), opened
 * by the SELECT+START chord, works everywhere, no pause. See pc_overlay.c for the design.
 *
 * Screens: MAIN (settings list) -> SAVES (save-management browser) -> CONFIRM (Yes/No / 3-way).
 * The pad filter (libetc.c) forwards one button edge at a time via PC_OverlayInput(); the renderer
 * (pc_gpu_window.c) reads the accessors below and paints the current screen.
 */
#ifndef PLATFORM_PC_OVERLAY_H
#define PLATFORM_PC_OVERLAY_H

/* Buttons the pad filter forwards (positional face buttons + Start). */
enum {
    OVL_BTN_UP, OVL_BTN_DOWN, OVL_BTN_LEFT, OVL_BTN_RIGHT,
    OVL_BTN_CIRCLE, OVL_BTN_CROSS, OVL_BTN_SQUARE, OVL_BTN_TRIANGLE, OVL_BTN_START
};
/* Current screen. */
enum { OVL_SCREEN_MAIN, OVL_SCREEN_SAVES, OVL_SCREEN_CONFIRM, OVL_SCREEN_DETAIL };

int  PC_OverlayIsOpen(void);
void PC_OverlayToggle(void);        /* chord: open <-> close (always reopens on the MAIN screen) */
void PC_OverlayInput(int button);   /* one OVL_BTN_* edge from the pad filter */

int  PC_OverlayScreen(void);        /* OVL_SCREEN_* */
const char *PC_OverlayTitle(void);  /* the current screen's title */

/* MAIN screen -- the settings list. */
int  PC_OverlayCount(void);
int  PC_OverlaySelected(void);
/* Fills *label; for toggle/choice also *valueText. Returns 1 if the item shows a value, else 0. */
int  PC_OverlayItem(int i, const char **label, const char **valueText);
int  PC_OverlayItemDisabled(int i); /* 1 => greyed/inactive */

/* SAVES screen -- the archive browser. */
int  PC_OverlaySaveCount(void);
int  PC_OverlaySaveSelected(void);
const char *PC_OverlaySaveLabel(int i);   /* row i's display label ("2026-07-25 15:30"), or NULL */
int  PC_OverlaySaveActive(int i);         /* 1 if row i is byte-identical to the current card */
int  PC_OverlaySaveHasActive(void);       /* 1 if there is an active card to back up */

/* CONFIRM screen -- a small prompt. */
const char *PC_OverlayConfirmMsg(void);    /* the question line */
int  PC_OverlayConfirmMsgWarn(void);       /* 1 => render the question line as a red warning */
const char *PC_OverlayConfirmTarget(void); /* the archive label being acted on */
int  PC_OverlayConfirmTargetWarn(void);    /* 1 => render the target line as a red warning */
int  PC_OverlayConfirmCount(void);         /* number of options */
const char *PC_OverlayConfirmOption(int i);
int  PC_OverlayConfirmSelected(void);

/* DETAIL screen -- the 3 slots inside the inspected archive. */
const char *PC_OverlayDetailTitle(void);   /* the archive's label (date) */
const char *PC_OverlayDetailSlot(int i);   /* slot i's caption (0..2), or NULL if empty */

#endif
