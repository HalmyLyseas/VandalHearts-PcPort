/* pc_battle_speed.c -- battle-only fast-forward: the speed state, the battle-state gate, and the
 * pad-2 L2/R2 input edge. VSync() calls in for the pacing divisor and the leave-battle reset
 * (seams: pc_etc_internal.h). See docs/pc-port/subsystems/kernel.md, "Battle fast-forward". */
#include "PsyQ/libetc.h"
#include "state.h"
#include "pc_platform.h"
#include "pc_overlay.h"
#include "pc_etc_internal.h"

/* ---- Battle-only fast-forward ---------------------------------------------------------------
 * A whole-tick speed multiplier, effective only in an active battle. See
 * docs/pc-port/subsystems/kernel.md, "Battle fast-forward". */
#define VH_BATTLE_SPEED_MAX 2
static int s_battleSpeed = 1;   /* 1..MAX; reset to 1x on leaving battle (see VSync) */

int PC_InActiveBattle(void) {
    /* Matches core/main.c's State_Battle() dispatch set exactly. See
     * docs/pc-port/subsystems/kernel.md, "Battle fast-forward". */
    switch (gState.primary) {
    case STATE_3:
    case STATE_LOAD_IN_BATTLE_SAVE:
    case STATE_27:
    case STATE_30:
    case STATE_31:
        return 1;
    default:
        return 0;
    }
}

/* Effective speed for the OSD indicator: 1 when not in a battle so the readout hides. */
int PC_BattleSpeedGet(void) {
    return PC_InActiveBattle() ? s_battleSpeed : 1;
}

/* Edge-detect the pad-2 (high-word) L2/R2 bits and step the speed. Fed the full 32-bit pad word from
 * PadRead(). Only adjustable in an active battle with the options overlay closed. */
void PC_BattleSpeedInput(unsigned int pad) {
    static unsigned int prev = 0;
    unsigned int hi = pad >> 16;                     /* pad 2 lives in the high word */
    unsigned int newpress = hi & ~prev;              /* rising edges -> one step per tap */
    prev = hi;
    if (!PC_InActiveBattle() || PC_OverlayIsOpen()) return;
    if ((newpress & PADR2) && s_battleSpeed < VH_BATTLE_SPEED_MAX) s_battleSpeed++;
    if ((newpress & PADL2) && s_battleSpeed > 1)                   s_battleSpeed--;
}


int PC_BattleSpeedRaw(void) { return s_battleSpeed; }   /* diagnostics label (VH_FRAME_TIME) */

/* Fast-forward resets to 1x on leaving battle, so it never carries into the next battle or an
 * overworld save -- each battle starts at normal speed (called once per VSync). */
void PC_BattleSpeedReset(void) { if (!PC_InActiveBattle()) s_battleSpeed = 1; }
