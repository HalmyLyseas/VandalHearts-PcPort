/* pc_etc_internal.h -- seams between libetc.c (pads + VSync pacing) and the TUs extracted from it
 * (pc_diag.c, pc_battle_speed.c). NOT a public API; everything here used to be file-static in
 * libetc.c before the split. */
#ifndef PC_ETC_INTERNAL_H
#define PC_ETC_INTERNAL_H

/* ---- pc_battle_speed.c (Stage-3 1.4 F1 battle fast-forward) ---- */
int  PC_InActiveBattle(void);           /* core/main.c State_Battle() dispatch set {3,23,27,30,31} */
int  PC_BattleSpeedGet(void);           /* effective speed: 1 outside battle (OSD + pacing divisor) */
int  PC_BattleSpeedRaw(void);           /* the raw speed state, for diagnostics labels */
void PC_BattleSpeedReset(void);         /* per-VSync: back to 1x whenever not in battle */
void PC_BattleSpeedInput(unsigned int pad);   /* pad-2 L2/R2 rising edges step the speed */

/* ---- pc_diag.c (env-gated diagnostics + the VH_SMOKE boot harness) ---- */
void PC_DiagFrameEntry(int mode);       /* VH_FRAME_TIME: close the previous work/idle interval */
void PC_DiagIdleDelay(unsigned ms);     /* the pacing SDL_Delay, timed into the idle bucket */
void PC_DiagVSyncRows(void);            /* all VH_*_LOG per-tick CSV rows */
void PC_DiagFps(int mode);              /* VH_FPS_LOG per-second meter */
unsigned int PC_SmokePadHold(void);     /* VH_SMOKE: START-hold bits through the intro movies */
void PC_SmokeFrame(void);               /* VH_SMOKE: exit(0) at the title / exit(1) on timeout */

#endif /* PC_ETC_INTERNAL_H */
