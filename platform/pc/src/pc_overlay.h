/*
 * Stage-3 (1.1C) in-game options overlay -- model/state only.
 *
 * PC-side, backend-level: zero `src/` changes, works in every context (battle, world map, menus,
 * movies). Opened by the SELECT+START chord. This file owns the menu model (a small data-driven
 * item list) and the current selection; the two hooks that drive it live elsewhere:
 *   - input:  libetc.c's pad filter (PC_OverlayFilterPad) calls the Move/Adjust/Activate/Cancel and
 *             Toggle entry points, and freezes the pad the game sees while the overlay is open.
 *   - render: pc_gpu_window.c's PC_GpuDrawOverlay() reads the Title/Count/Selected/Item accessors
 *             and paints a neutral panel over the presented frame.
 * The game keeps running behind the overlay (no pause) -- it simply receives a zero pad and idles.
 */
#ifndef PLATFORM_PC_OVERLAY_H
#define PLATFORM_PC_OVERLAY_H

int  PC_OverlayIsOpen(void);
void PC_OverlayToggle(void);       /* the SELECT+START chord: open <-> close (sole show/hide) */

/* Input, driven by the pad filter while the overlay is open. */
void PC_OverlayMove(int delta);    /* -1 = up, +1 = down: move the selection (wraps) */
void PC_OverlayAdjust(int delta);  /* -1/+1: set the selected toggle (left=off, right=on) */
void PC_OverlayActivate(void);     /* Circle/confirm: flip the selected toggle, or run an action */
void PC_OverlayCancel(void);       /* close the overlay -- available, but currently unbound (the    */
                                   /* chord is the only close; a face button would leak, see .c)   */

/* Read API, for the renderer. */
const char *PC_OverlayTitle(void);
int  PC_OverlayCount(void);
int  PC_OverlaySelected(void);
/* Fills *label; for a toggle also *valueText (e.g. "NORMAL"/"INVERTED"), else *valueText = NULL.
 * Returns 1 if item `i` is a toggle, 0 if it is an action (or out of range). */
int  PC_OverlayItem(int i, const char **label, const char **valueText);
/* 1 if item i is currently greyed/inactive (drawn dimmed by the renderer), else 0. */
int  PC_OverlayItemDisabled(int i);

#endif
