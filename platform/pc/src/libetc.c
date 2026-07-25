/*
 * PC backend for PsyQ/libetc.h's pad/vsync interface, backed by SDL2.
 *
 * VSync() reproduces the original's frame-locked timing (NTSC ~60Hz) via a
 * simple frame limiter, matching the original game's behavior rather than
 * decoupling simulation from render rate -- that's a real future feature,
 * not attempted here. PadRead() currently maps SDL2 keyboard state onto the
 * standard PS1 digital-pad bit layout; a real SDL_GameController mapping is
 * a natural follow-up, not required to prove the interface out.
 */
#include <SDL2/SDL.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "PsyQ/libetc.h"
#include "state.h"
#include "object.h"
#include "graphics.h"
#include "field.h"
#include "pc_platform.h"
#include "pc_overlay.h"

/* PSX NTSC vblank rate. The old `FRAME_MS = 1000/60` truncated to 16 (integer), pacing the game at
 * 62.5Hz => it ran ~4% too fast, the uniform "everything a touch shorter than BizHawk" drift
 * (feedback-26). Use the real NTSC vblank period as a DOUBLE, with a fractional-deadline accumulator
 * in VSync() so integer SDL_Delay still averages to the correct rate (some frames 16ms, some 17). */
#define VBLANK_HZ 59.94
#define FRAME_MS_F (1000.0 / VBLANK_HZ)   /* ~16.683 ms */

static double s_nextVBlankMs = 0.0; /* fractional frame deadline (SDL_GetTicks ms basis) */
static int s_vsyncInitialized = 0;
static int s_vblankCount = 0;

/* Diagnostic FPS logging (2026-07-12, investigating whether VSync(2) during
 * battle actually halves the real display-update rate, or whether real
 * hardware's positive-mode VSync() means something other than "block for
 * mode frame-durations"). Counts real VSync() *calls* (once per UpdateEngine()
 * call, i.e. once per real logic tick / display swap) separately from the
 * inner mode-driven wait loop, and prints calls-per-real-second once a
 * second to stderr, tagged with the mode last seen. */
static Uint32 s_fpsWindowStart = 0;
static int s_fpsWindowCalls = 0;
static int s_fpsLastMode = -2;

/* Camera-target full-watch-log (2026-07-12, exchange/20-camera-viewport-coordinates.md).
 * Mirrors exchange/21-camera-target-full-watch-log.lua's BizHawk capture field-for-field so
 * the two CSVs are diffable at matching game-state checkpoints (bmFuncIdx/bmState/bmState2),
 * not by raw frame number -- the two builds' frame counts never lined up 1:1 (see
 * feedback-02.md's timing table), but the BattleMgr state machine is the same code on both
 * platforms and gives a reliable alignment anchor. Logs every VSync() call (one row per real
 * logic tick), matching the "dense capture, not sampled" methodology now standard for this
 * project (see the phase-c-pc-port skill's "BizHawk as ground truth" section).
 *
 * One deliberate column difference from the Lua script: BizHawk's `targetAddr` is a raw PS1
 * KUSEG address, meaningless as a cross-platform value -- our own process's pointer values are
 * just as meaningless to a human reader. Logging the target's own gObjectArray index instead
 * (targetObjIdx) keeps the column comparable in spirit (identifies WHICH object is the target)
 * without a nonsensical raw-address diff; targetX/Y/Z remain real, directly diffable
 * coordinate values either way. */
static FILE *s_camTraceFile = NULL;

static s32 FindCameraObjIdx(s16 *outFuncIdx) {
    for (s32 i = 0; i < OBJ_DATA_CT; i++) {
        s16 f = gObjectArray[i].functionIndex;
        if (f == OBJF_CAMERA_TBD_017 || f == OBJF_CAMERA_TBD_026 || f == OBJF_CAMERA_TBD_588) {
            *outFuncIdx = f;
            return i;
        }
    }
    *outFuncIdx = -1;
    return -1;
}

static s32 ObjPtrToIdx(struct Object *p) {
    if (p == NULL) return -1;
    ptrdiff_t d = p - gObjectArray;
    if (d < 0 || d >= OBJ_DATA_CT) return -1;
    return (s32)d;
}

/* feedback-14 follow-up: per-sprite pipeline log. Enabled by VH_SPRITE_LOG env var. */
static FILE *s_spriteFateFile = NULL;
void PC_DebugSpriteLog(int tileX, int tileZ, int winX, int winZ, int mapSX, int mapSZ,
                       int culled, int gfxIdx, int sx, int sy, int otz, int otIdx) {
    static int enabled = -1;
    if (enabled < 0) enabled = (getenv("VH_SPRITE_LOG") != NULL) ? 1 : 0;
    if (!enabled) return;
    if (s_spriteFateFile == NULL) {
        s_spriteFateFile = fopen("vh_sprite_fate.csv", "w");
        if (s_spriteFateFile == NULL) { enabled = 0; return; }
        fprintf(s_spriteFateFile,
                "frame,tileX,tileZ,winX,winZ,mapSX,mapSZ,inWindowX,inWindowZ,"
                "culled,gfxIdx,sx,sy,otz,otIdx,ofx,ofy,h,rt00,rt02,rt22,trx,trz,terrainOtz,zsf4\n");
    }
    /* also compute the window-membership per axis to make the cull reason obvious */
    int inX = (tileX >= winX && tileX <= mapSX + winX - 1);
    int inZ = (tileZ >= winZ && tileZ <= mapSZ + winZ - 1);
    /* GTE projection state used for THIS sprite (ofx/ofy are stored <<16; report >>16 too) */
    int ofx = 0, ofy = 0; int h = 0, rt00 = 0, rt02 = 0, rt22 = 0, trx = 0, trz = 0;
    PC_GteDebugState(&ofx, &ofy, &h, &rt00, &rt02, &rt22, &trx, &trz);
    fprintf(s_spriteFateFile, "%ld,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%ld,%ld,%d,%d,%d,%d,%d,%d,%d,%d\n",
            s_vblankCount, tileX, tileZ, winX, winZ, mapSX, mapSZ, inX, inZ,
            culled, gfxIdx, sx, sy, otz, otIdx,
            (int)(ofx >> 16), (int)(ofy >> 16), h, rt00, rt02, rt22, trx, trz,
            PC_GteLastOtz(), PC_GteZsf4());
    fflush(s_spriteFateFile);
}

/* Per event-entity anim-set probe (build SPRITE_LOG=1, enable VH_SPRITE_LOG). Dumps each event
 * entity's baseAnimSet/altAnimSet pointer, the animSet actually used, animIdx, facing, animData,
 * and resulting gfxIdx + tile -- to find a prop entity spawned with the WRONG (character) anim-set.
 * Deduped per object slot: only writes a row when that slot's gfxIdx changes, so volume stays low. */
static FILE *s_evtEntFile = NULL;
void PC_DebugEvtEntityLog(int objIdx, int tileX, int tileZ, const void *base, const void *alt,
                          const void *cur, int animIdx, int facing, const void *animData,
                          int gfxIdx, int usingAlt, int runState, int opcode, int cmdState,
                          int cmdOff, int cmdArg) {
    static int enabled = -1;
    static int lastGfx[1024], lastOp[1024], lastState[1024];
    static unsigned char seen[1024];
    if (enabled < 0) enabled = (getenv("VH_SPRITE_LOG") != NULL) ? 1 : 0;
    if (!enabled) return;
    if (objIdx >= 0 && objIdx < 1024) {
        /* log when the sprite OR the command-processing state changes (so we catch the
         * command each entity is parked on, not just gfx changes) */
        if (seen[objIdx] && lastGfx[objIdx] == gfxIdx && lastOp[objIdx] == opcode && lastState[objIdx] == runState) return;
        seen[objIdx] = 1; lastGfx[objIdx] = gfxIdx; lastOp[objIdx] = opcode; lastState[objIdx] = runState;
    }
    if (s_evtEntFile == NULL) {
        s_evtEntFile = fopen("vh_evt_entity.csv", "w");
        if (s_evtEntFile == NULL) { enabled = 0; return; }
        fprintf(s_evtEntFile,
                "frame,objIdx,tileX,tileZ,baseAnimSet,altAnimSet,animSetUsed,animIdx,facing,animData,gfxIdx,usingAlt,runState,opcode,cmdState,cmdOff,cmdArg\n");
    }
    fprintf(s_evtEntFile, "%ld,%d,%d,%d,%p,%p,%p,%d,%d,%p,%d,%d,%d,%d,%d,%d,%d\n",
            s_vblankCount, objIdx, tileX, tileZ, base, alt, cur, animIdx, facing, animData, gfxIdx, usingAlt,
            runState, opcode, cmdState, cmdOff, cmdArg);
    fflush(s_evtEntFile);
}

/* AI spell-target scoring log (build with AI_LOG=1, enable at runtime with VH_AI_LOG). One line per
 * candidate target a caster's AI scored in func_800569A0; the highest SCORE is the target it picks.
 * Shows the term breakdown so a "wrong" choice is explainable -- especially the type-matchup term
 * ADV = -gAdvantage[caster.advantage][target.advantage] (your mage->armor / ranged->flyer doctrine).
 * A high raw gAdvantage means a poor matchup, so it SUBTRACTS from the score (less-preferred). */
static FILE *s_aiLogFile = NULL;
void PC_DebugAiTargetLog(int casterName, int casterAdv, int casterLvl,
                         int tgtName, int tgtClass, int tgtAdv, int tgtLvl, int tgtHpFrac,
                         int lvlTerm, int hpTerm, int advTerm, int advRaw, int terrTerm, int score) {
    static int enabled = -1;
    if (enabled < 0) enabled = (getenv("VH_AI_LOG") != NULL) ? 1 : 0;
    if (!enabled) return;
    if (s_aiLogFile == NULL) {
        s_aiLogFile = fopen("vh_ai_log.txt", "w");
        if (s_aiLogFile == NULL) { enabled = 0; return; }
    }
    fprintf(s_aiLogFile,
            "f=%ld caster[name=%d adv=%d lvl=%d] -> target[name=%d class=%d adv=%d lvl=%d hpFrac=%d]"
            "  terms: base=+280 lvl=%+d hp=%+d ADV=%+d(gAdv=%d) terr=%+d  = SCORE %d\n",
            s_vblankCount, casterName, casterAdv, casterLvl,
            tgtName, tgtClass, tgtAdv, tgtLvl, tgtHpFrac,
            lvlTerm, hpTerm, advTerm, advRaw, terrTerm, score);
    fflush(s_aiLogFile);
}

/* bugreport-03 (DEATHANGEL caster never casts): dumps the AI's top-level spell decision in
 * Objf570_AI_TBD case 0 -- every input that can force the melee branch (state 1) plus the state it
 * actually chose. `state` 1 = melee/move only (OBJF_AI_TBD_403), 2/4 = cast (OBJF_AI_TBD_402).
 * The two zeroing inputs to watch: spells[]==SPELL_NULL, and mp < gSpells[spell].mpCost (which
 * zeroes spellEffectA/B and drops the AI to state 1 even for a fully-equipped caster). */
static FILE *s_aiDecFile = NULL;
void PC_DebugAiDecisionLog(int name, int cls, int team, int lvl, int mp, int maxMp,
                           int spell0, int spell1, int cost0, int cost1,
                           int effect0, int effect1, int effA, int effB, int state) {
    static int enabled = -1;
    if (enabled < 0) enabled = (getenv("VH_AI_LOG") != NULL) ? 1 : 0;
    if (!enabled) return;
    if (s_aiDecFile == NULL) {
        s_aiDecFile = fopen("vh_ai_decision.csv", "w");
        if (s_aiDecFile == NULL) { enabled = 0; return; }
        fprintf(s_aiDecFile, "name,class,team,level,mp,maxMp,spell0,spell1,mpCost0,mpCost1,"
                             "exEffect0,exEffect1,spellEffectA,spellEffectB,chosenState\n");
    }
    fprintf(s_aiDecFile, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
            name, cls, team, lvl, mp, maxMp, spell0, spell1, cost0, cost1,
            effect0, effect1, effA, effB, state);
    fflush(s_aiDecFile);
}

/* Dumps the game state-machine fields for freeze diagnosis (called from the SIGUSR1 handler in
 * pc_bootstrap.c). A "freeze" where the frame loop keeps ticking = a state stuck waiting on a
 * condition; these fields say which state/scene/map it's stuck in so we know where to look. */
/* bugreport (epilogue ocarina silent): log every PerformAudioCommand so we can see whether the
 * ending's PLAY_XA(102) == 0x366 is ever issued (trigger fired) vs never reaching the audio layer.
 * Decodes the 0x?00 opcode nibble for readability. Build AUDIO_CMD_LOG=1, enable VH_AUDIO_LOG. */
void PC_DebugAudioCmdLog(int cmd, int enabled) {
    static int en = -1;
    static FILE *f = NULL;
    if (en < 0) en = (getenv("VH_AUDIO_LOG") != NULL) ? 1 : 0;
    if (!en) return;
    if (f == NULL) {
        f = fopen("vh_audio_cmd.csv", "w");
        if (f == NULL) { en = 0; return; }
        fprintf(f, "t_ms,cmd_hex,op,id,audioEnabled\n");
    }
    {
        int op = (cmd >> 8) & 0xff, id = cmd & 0xff;
        const char *name = op == 0 ? "PARAMLESS" : op == 2 ? "PLAY_SEQ" : op == 3 ? "PLAY_XA"
                         : op == 5 || op == 6 || op == 7 ? "SFX" : op == 0x13 ? "PREPARE_XA" : "?";
        fprintf(f, "%u,0x%04x,%s,%d,%d\n", s_vblankCount, cmd, name, id, enabled);
        fflush(f);
    }
}

void PC_DumpGameState(int fd) {
    (void)fd;
    fprintf(stderr,
        "gState: primary=%d secondary=%d state3=%d state4=%d state6=%d state7=%d "
        "scene=%d mapNum=%d movieIdx=%d inEvent=%d\n",
        (int)gState.primary, (int)gState.secondary, (int)gState.state3, (int)gState.state4,
        (int)gState.state6, (int)gState.state7, (int)gState.scene, (int)gState.mapNum,
        (int)gState.movieIdxToPlay, (int)gState.inEvent);
    fflush(stderr);
}

/* feedback-11 follow-up: mirror BizHawk's RAM Watch on our own window. Called from libgpu.c
 * right before the present, so the label matches the frame being shown. Rendered by
 * pc_gpu_window.c only when VH_CAM_OSD is set. */
void PC_UpdateCamOsd(void) {
    snprintf(g_camOsdText, sizeof(g_camOsdText), "PIT:%d YAW:%d X:%d Z:%d FRD:%d",
             gCameraRotation.vx, gCameraRotation.vy, gCameraPos.vx, gCameraPos.vz,
             gState.fieldRenderingDisabled);
}

static void LogCameraTraceRow(void) {
    static int enabled = -1;
    if (enabled < 0) enabled = (getenv("VH_CAM_LOG") != NULL) ? 1 : 0;   /* opt-in; off for end users */
    if (!enabled) return;
    if (s_camTraceFile == NULL) {
        s_camTraceFile = fopen("vh_camera_target_full_watch_log_pc.csv", "w");
        if (s_camTraceFile == NULL) return;
        fprintf(s_camTraceFile,
                "frame,primary,bmFuncIdx,bmState,bmState2,"
                "camPosX,camPosY,camPosZ,camRotX,camRotY,camZoomZ,"
                "camObjIdx,camObjFuncIdx,camObjState,targetObjIdx,targetX,targetY,targetZ,"
                /* 30-fresh-look-deterministic-camera-hypothesis.md: winOrigin[X/Z] is the
                 * actual render-window origin (D_80122E28/2C, src/battle_0201b8.c:193), the
                 * derived quantity the terrain-window + sprite-cull both key off; camDirQuad is
                 * the yaw quadrant RenderField selects (src/graphics.c:1178). startCur[X/Z/Y]
                 * is gMapCursorStartingPos[mapNum] as read at RUNTIME on this build -- confirms
                 * the (byte-exact-static) table isn't being misread through a port stride/endian
                 * bug. These are what make the pre-AI frame-0 diff decisive. */
                /* 30-* (a): fieldRenderDisabled (gState) = field draws only when 0 -- the
                 * "first non-loading frame" un-blank moment; fadeLevel = gScreenFade's Objf795
                 * fade level (255=black .. 0=full brightness), the fade-in ramp. Together these
                 * pin the exact frame the scene becomes visible to the concurrent camPos, so we
                 * can state same-run whether our field un-blanks while the camera is still parked
                 * at (-32,-32) or already mid-pan. -1 fadeLevel = no active gScreenFade object. */
                "winOriginX,winOriginZ,camDirQuad,startCurX,startCurZ,startCurY,"
                /* live map cursor = the AI-turn camera-focus target CenterCamera eases toward
                 * (battle_0201b8.c:190); pairs with 43-camera-focus-easein-trace.lua's mapCursorX/Z
                 * so both platforms carry the ease-in target for the iteration-vs-gate diff. */
                "mapCursorX,mapCursorZ,"
                "fieldRenderDisabled,fadeLevel\n");
    }

    s16 bmFuncIdx = -1, bmState = -1, bmState2 = -1;
    for (s32 i = 0; i < OBJ_DATA_CT; i++) {
        if (gObjectArray[i].functionIndex == OBJF_BATTLE_MGR) {
            bmFuncIdx = gObjectArray[i].functionIndex;
            bmState = gObjectArray[i].state;
            bmState2 = gObjectArray[i].state2;
            break;
        }
    }

    s16 camObjFuncIdx = -1;
    s32 camObjIdx = FindCameraObjIdx(&camObjFuncIdx);
    s16 camObjState = -1;
    s32 targetObjIdx = -1;
    s16 targetX = -1, targetY = -1, targetZ = -1;
    if (camObjIdx >= 0) {
        Object *camObj = &gObjectArray[camObjIdx];
        camObjState = camObj->state;
        struct Object *target = (camObjFuncIdx == OBJF_CAMERA_TBD_017)
                                     ? camObj->d.objf017.sprite
                                     : camObj->d.objf026.target;
        targetObjIdx = ObjPtrToIdx(target);
        if (targetObjIdx >= 0) {
            targetX = gObjectArray[targetObjIdx].x1.n;
            targetY = gObjectArray[targetObjIdx].y1.n;
            targetZ = gObjectArray[targetObjIdx].z1.n;
        }
    }

    s16 camDirQuad = (s16)((gCameraRotation.vy & 0xfff) >> 10);
    /* mapNum can be out of the 0..BATTLE_CT-1 range briefly outside a battle; clamp the read. */
    s16 startCurX = -1, startCurZ = -1, startCurY = -1;
    if (gState.mapNum >= 0 && gState.mapNum < BATTLE_CT) {
        startCurX = gMapCursorStartingPos[gState.mapNum].x;
        startCurZ = gMapCursorStartingPos[gState.mapNum].z;
        startCurY = gMapCursorStartingPos[gState.mapNum].y;
    }

    s16 fadeLevel = (gScreenFade != NULL) ? gScreenFade->d.objf795.fade : -1;

    fprintf(s_camTraceFile,
            "%ld,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
            s_vblankCount, gState.primary, bmFuncIdx, bmState, bmState2,
            gCameraPos.vx, gCameraPos.vy, gCameraPos.vz,
            gCameraRotation.vx, gCameraRotation.vy, gCameraZoom.vz,
            camObjIdx, camObjFuncIdx, camObjState, targetObjIdx, targetX, targetY, targetZ,
            D_80122E28, D_80122E2C, camDirQuad, startCurX, startCurZ, startCurY,
            gMapCursorX, gMapCursorZ,
            gState.fieldRenderingDisabled, fadeLevel);
    fflush(s_camTraceFile);
}

/* AI-decision-chain trace (timing topic, Finding #1: the "unit active -> blue movement overlay"
 * early-gate). The overlay (gShowBlueMovementGrid=1, set in Objf013_BattleMgr state 7.2) can't fire
 * until BattleMgr STATE 6 exits, and state 6 blocks on `D_80123468 != 0` (battle_0201b8.c:3202),
 * which is set only when the AI-decision object chain finishes (Objf570_AI_TBD -> Objf40x/Objf589,
 * ai.c:278 case 99, via the D_80123480 hand-off). On hardware state 6 lasts ~108 frames; on our build
 * ~18 -> the AI chain completes ~6x faster and the overlay shows early (mid camera-pan). This logs the
 * chain's live state so we can see EXACTLY which sub-state exits early. Env-gated (VH_AI_LOG); reads
 * only game RAM (no src/ changes, matching build untouched). Pairs with 43-camera-focus-easein. */
extern u8 D_80123468; /* BattleMgr state-6 release flag (battle.h) */
extern u8 D_80123480; /* Objf570 sub-object hand-off flag (ai.c) */
extern int g_aiVisitCount[8]; /* cumulative GetRCnt visits per AI range (libkernel.c); for calibration */
static FILE *s_aiChainFile = NULL;

static void LogAiChainRow(void) {
    static int enabled = -1;
    if (enabled < 0) enabled = (getenv("VH_AI_LOG") != NULL) ? 1 : 0;
    if (!enabled) return;
    if (s_aiChainFile == NULL) {
        s_aiChainFile = fopen("vh_ai_chain_pc.csv", "w");
        if (s_aiChainFile == NULL) return;
        fprintf(s_aiChainFile,
                "frame,primary,bmState,bmState2,showBlue,D468,D480,"
                "ai570Idx,ai570State,ai570State2,subFuncIdx,subState,subState2,"
                "camPosX,camPosZ,mapCursorX,mapCursorZ,"
                /* cumulative GetRCnt visits per AI range: diff across an AI phase = visits_per_scan */
                "vIsLag,v570,v400,v401,v402,v403,v404,v589\n");
    }

    s16 bmState = -1, bmState2 = -1;
    s16 ai570Idx = -1, ai570State = -1, ai570State2 = -1;
    s16 subFuncIdx = -1, subState = -1, subState2 = -1;
    for (s32 i = 0; i < OBJ_DATA_CT; i++) {
        s16 fi = gObjectArray[i].functionIndex;
        if (fi == OBJF_BATTLE_MGR) {
            bmState = gObjectArray[i].state;
            bmState2 = gObjectArray[i].state2;
        } else if (fi == OBJF_AI_TBD_570) {
            ai570Idx = (s16)i;
            ai570State = gObjectArray[i].state;
            ai570State2 = gObjectArray[i].state2;
        } else if (fi == OBJF_AI_TBD_400 || fi == OBJF_AI_TBD_401 || fi == OBJF_AI_TBD_402 ||
                   fi == OBJF_AI_TBD_403 || fi == OBJF_AI_TBD_404 || fi == OBJF_AI_TBD_589) {
            /* the currently-active AI sub-object (first found; the chain runs them one at a time) */
            subFuncIdx = fi;
            subState = gObjectArray[i].state;
            subState2 = gObjectArray[i].state2;
        }
    }

    fprintf(s_aiChainFile,
            "%ld,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,"
            "%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld\n",
            s_vblankCount, gState.primary, bmState, bmState2, gShowBlueMovementGrid,
            D_80123468, D_80123480, ai570Idx, ai570State, ai570State2,
            subFuncIdx, subState, subState2,
            gCameraPos.vx, gCameraPos.vz, gMapCursorX, gMapCursorZ,
            g_aiVisitCount[0], g_aiVisitCount[1], g_aiVisitCount[2], g_aiVisitCount[3],
            g_aiVisitCount[4], g_aiVisitCount[5], g_aiVisitCount[6], g_aiVisitCount[7]);
    fflush(s_aiChainFile);
}

/* rand()-seed full-watch-log (2026-07-12, exchange/20-camera-viewport-coordinates.md's "ROOT
 * CAUSE FOUND" section). Mirrors the camera trace's dense, every-tick, state-anchored capture
 * style, but for the shared PS1-BIOS-LCG rand() state (platform/pc/src/libkernel.c's
 * s_randSeed, exposed via GetRandSeedForDebug() to avoid changing that variable's linkage for
 * anything else) -- to directly confirm/deny whether libcd.c's CD-seek-jitter simulation
 * desyncs it from real hardware's stream (found empirically at RAM address 0x80009010 via
 * exchange/24-find-rand-seed-address.lua's LCG-signature scan; mirrored here as
 * exchange/25-rand-seed-full-watch-log.lua). */
extern u32 GetRandSeedForDebug(void);
static FILE *s_randTraceFile = NULL;

static void LogRandSeedRow(void) {
    static int enabled = -1;
    if (enabled < 0) enabled = (getenv("VH_RAND_LOG") != NULL) ? 1 : 0;   /* opt-in; off for end users */
    if (!enabled) return;
    if (s_randTraceFile == NULL) {
        s_randTraceFile = fopen("vh_rand_seed_full_watch_log_pc.csv", "w");
        if (s_randTraceFile == NULL) return;
        fprintf(s_randTraceFile, "frame,primary,seed\n");
    }
    fprintf(s_randTraceFile, "%ld,%d,0x%08x\n", s_vblankCount, gState.primary,
            GetRandSeedForDebug());
    fflush(s_randTraceFile);
}

/* Camera-matrix / view-Z (OTZ) baseline mirror (exchange/31-camera-matrix-otz-baseline.lua).
 * Writes the exact same columns as the BizHawk baseline so the two CSVs diff directly, to
 * localize why our terrain OTZ (= avg view-Z) runs high enough (>=406) that graphics.c's
 * distance-darkening saturates terrain to black. Read at the same per-frame point as the
 * BizHawk end-of-frame read, so gCameraMatrix.t[] is equally "post-render stale" on both sides
 * (the reliable comparands are the inputs gCameraRotation/gCameraZoom/gMapScale/enableMapScaling
 * and the rotation submatrix m[][]; align rows by matched pose, not frame number). Env-gated
 * (VH_MTX_LOG) so normal runs are unaffected. */
static FILE *s_mtxTraceFile = NULL;

static void LogCameraMatrixRow(void) {
    static int enabled = -1;
    if (enabled < 0) enabled = (getenv("VH_MTX_LOG") != NULL) ? 1 : 0;
    if (!enabled) return;

    if (s_mtxTraceFile == NULL) {
        s_mtxTraceFile = fopen("vh_camera_matrix_otz_pc.csv", "w");
        if (s_mtxTraceFile == NULL) return;
        fprintf(s_mtxTraceFile,
                "frame,primary,mapNum,fieldDis,mapScaleEn,"
                "camRotX,camRotY,camRotZ,camPosX,camPosY,camPosZ,"
                "camZoomX,camZoomY,camZoomZ,mapScaleX,mapScaleY,mapScaleZ,"
                "m00,m01,m02,m10,m11,m12,m20,m21,m22,t0,t1,t2\n");
    }

    fprintf(s_mtxTraceFile,
            "%ld,%d,%d,%d,%d,"
            "%d,%d,%d,%d,%d,%d,"
            "%ld,%ld,%ld,%ld,%ld,%ld,"
            "%d,%d,%d,%d,%d,%d,%d,%d,%d,%ld,%ld,%ld\n",
            s_vblankCount, gState.primary, gState.mapNum, gState.fieldRenderingDisabled,
            gState.enableMapScaling,
            gCameraRotation.vx, gCameraRotation.vy, gCameraRotation.vz,
            gCameraPos.vx, gCameraPos.vy, gCameraPos.vz,
            gCameraZoom.vx, gCameraZoom.vy, gCameraZoom.vz,
            gMapScale.vx, gMapScale.vy, gMapScale.vz,
            gCameraMatrix.m[0][0], gCameraMatrix.m[0][1], gCameraMatrix.m[0][2],
            gCameraMatrix.m[1][0], gCameraMatrix.m[1][1], gCameraMatrix.m[1][2],
            gCameraMatrix.m[2][0], gCameraMatrix.m[2][1], gCameraMatrix.m[2][2],
            gCameraMatrix.t[0], gCameraMatrix.t[1], gCameraMatrix.t[2]);
    fflush(s_mtxTraceFile);
}

/* Terrain OTZ probe (otz-overflow / black-terrain investigation). RenderMapTile /
 * RenderEdgeMapTile (src/graphics.c) call PC_DebugTerrainTile once per drawn tile with the
 * post-clamp otz and the final vertex-0 colour. We accumulate per-frame aggregates (tile count,
 * otz min/max/mean, and how many tiles ended fully black -- graphics.c's distance-darkening
 * zeroes r0/g0/b0 for otz>=406) and LogTerrainRow() dumps one pose-aligned row per frame. This
 * answers directly whether our terrain otz is really high enough to darken to black at the
 * matched opening pose (camera matrix already confirmed == real hardware). Env-gated
 * (VH_TERRAIN_LOG); the call site itself is a #ifdef PC_DEBUG_TERRAIN_LOG (TERRAIN_LOG=1 build),
 * so the matching build is untouched. */
static FILE *s_terrFile = NULL;
static int s_terrCount = 0, s_terrBlack = 0, s_terrOtzSum = 0;
static int s_terrOtzMin = 0x7fffffff, s_terrOtzMax = 0;
/* GTE state captured on the first tile of the frame (same for every tile that frame):
 * trx/trz = g.tr[0]/g.tr[2] actually loaded at terrain render (the number that bisects
 * "tr too large" vs "vertices too far"); h = projection distance; rt22 = rotation row-2 z. */
static int s_terrTrx = 0, s_terrTrz = 0, s_terrH = 0, s_terrRt22 = 0;

void PC_DebugTerrainTile(int otz, int r0, int g0, int b0) {
    static int enabled = -1;
    if (enabled < 0) enabled = (getenv("VH_TERRAIN_LOG") != NULL) ? 1 : 0;
    if (!enabled) return;
    if (s_terrCount == 0) {
        int ofx, ofy; int h, rt00, rt02, rt22, trx, trz;
        PC_GteDebugState(&ofx, &ofy, &h, &rt00, &rt02, &rt22, &trx, &trz);
        s_terrTrx = trx; s_terrTrz = trz; s_terrH = h; s_terrRt22 = rt22;
    }
    s_terrCount++;
    s_terrOtzSum += otz;
    if (otz < s_terrOtzMin) s_terrOtzMin = otz;
    if (otz > s_terrOtzMax) s_terrOtzMax = otz;
    if (r0 == 0 && g0 == 0 && b0 == 0) s_terrBlack++;
}

static void LogTerrainRow(void) {
    static int enabled = -1;
    if (enabled < 0) enabled = (getenv("VH_TERRAIN_LOG") != NULL) ? 1 : 0;
    if (!enabled) return;

    if (s_terrFile == NULL) {
        s_terrFile = fopen("vh_terrain_otz_pc.csv", "w");
        if (s_terrFile == NULL) return;
        fprintf(s_terrFile, "frame,primary,fieldDis,pitch,yaw,tileCount,"
                            "otzMin,otzMax,otzMean,blackCount,trx,trz,h,rt22\n");
    }
    if (s_terrCount > 0) {
        fprintf(s_terrFile, "%ld,%d,%d,%d,%d,%ld,%d,%d,%ld,%ld,%d,%d,%d,%d\n",
                s_vblankCount, gState.primary, gState.fieldRenderingDisabled,
                gCameraRotation.vx, gCameraRotation.vy,
                s_terrCount, s_terrOtzMin, s_terrOtzMax, s_terrOtzSum / s_terrCount, s_terrBlack,
                s_terrTrx, s_terrTrz, s_terrH, s_terrRt22);
        fflush(s_terrFile);
    }
    s_terrCount = 0; s_terrBlack = 0; s_terrOtzSum = 0;
    s_terrOtzMin = 0x7fffffff; s_terrOtzMax = 0;
}

/* AVSZ sprite-path probe (thread: which routine draws the opening characters?). AddObjPrim4
 * (object.c) uses RotAverage4 => AVSZ otz (= sz3/4-ish), so its sprites interleave with terrain,
 * unlike the raw-sz3 RenderUnitSprite path. If the demo-opening characters are drawn here, their
 * otz/otIdx will land in the terrain band; if AddObjPrim4 isn't called during the opening, our
 * build is missing the intro path entirely. Env-gated (VH_OBJPRIM4_LOG); the call site is
 * PC_DEBUG_SPRITE_LOG (SPRITE_LOG=1 build) so the matching build is untouched. */
static FILE *s_op4File = NULL;

void PC_DebugObjPrim4Log(int gfx, int otz, int otIdx, int sx, int sy, int otOfs) {
    static int enabled = -1;
    if (enabled < 0) enabled = (getenv("VH_OBJPRIM4_LOG") != NULL) ? 1 : 0;
    if (!enabled) return;
    if (s_op4File == NULL) {
        s_op4File = fopen("vh_objprim4.csv", "w");
        if (s_op4File == NULL) return;
        fprintf(s_op4File, "frame,primary,pitch,gfx,otz,otIdx,sx,sy,otOfs\n");
    }
    fprintf(s_op4File, "%ld,%d,%d,%d,%d,%d,%d,%d,%d\n",
            s_vblankCount, gState.primary, gCameraRotation.vx,
            gfx, otz, otIdx, sx, sy, otOfs);
    fflush(s_op4File);
}

/* Terrain projection diagnostic (terrain-collapse / pull-to-center investigation). RenderMapTile
 * calls this with a tile's 4 input model vertices right after the 4 corners are projected; we read
 * back what each corner projected to from the GTE ring (PC_GteProjEntry) plus H/OFX/OFY, so we can
 * compute the exact spread SX-OFX = (n*IR1)>>16 = H*IR1/SZ3 and see whether H, SZ3, or IR is
 * collapsing the spread toward the centre. One tile per frame (first RenderMapTile call), env-gated
 * (VH_TERRAINPROJ_LOG); the call site is #ifdef PC_DEBUG_TERRAIN_LOG so the matching build is untouched. */
static FILE *s_tprojFile = NULL;

void PC_DebugTerrainProjLog(int v0x, int v0y, int v0z, int v1x, int v1y, int v1z,
                            int v2x, int v2y, int v2z, int v3x, int v3y, int v3z) {
    static int enabled = -1;
    static int lastFrame = -1;
    int c, vin[4][3];
    int ofx, ofy; int h, rt00, rt02, rt22, trx, trz;

    if (enabled < 0) enabled = (getenv("VH_TERRAINPROJ_LOG") != NULL) ? 1 : 0;
    if (!enabled) return;
    if (s_vblankCount == lastFrame) return;   /* first drawn tile of the frame only */
    lastFrame = s_vblankCount;

    if (s_tprojFile == NULL) {
        s_tprojFile = fopen("vh_terrain_proj.csv", "w");
        if (s_tprojFile == NULL) return;
        fprintf(s_tprojFile, "frame,pitch,yaw,h,ofx,ofy");
        for (c = 0; c < 4; c++)
            fprintf(s_tprojFile, ",v%dx,v%dy,v%dz,sx%d,sy%d,ir1_%d,ir2_%d,ir3_%d,sz3_%d,n%d",
                    c, c, c, c, c, c, c, c, c, c);
        fprintf(s_tprojFile, "\n");
    }

    PC_GteDebugState(&ofx, &ofy, &h, &rt00, &rt02, &rt22, &trx, &trz);
    vin[0][0]=v0x; vin[0][1]=v0y; vin[0][2]=v0z;
    vin[1][0]=v1x; vin[1][1]=v1y; vin[1][2]=v1z;
    vin[2][0]=v2x; vin[2][1]=v2y; vin[2][2]=v2z;
    vin[3][0]=v3x; vin[3][1]=v3y; vin[3][2]=v3z;

    fprintf(s_tprojFile, "%ld,%d,%d,%d,%ld,%ld", s_vblankCount,
            gCameraRotation.vx, gCameraRotation.vy, h, ofx, ofy);
    for (c = 0; c < 4; c++) {
        int sx, sy, ir1, ir2, ir3, sz3, n;
        /* corners were pushed v0,v1,v2 (RTPT) then v3 (RTPS) => back = 3,2,1,0 */
        PC_GteProjEntry(3 - c, &sx, &sy, &ir1, &ir2, &ir3, &sz3, &n);
        fprintf(s_tprojFile, ",%d,%d,%d,%ld,%ld,%ld,%ld,%ld,%ld,%ld",
                vin[c][0], vin[c][1], vin[c][2], sx, sy, ir1, ir2, ir3, sz3, n);
    }
    fprintf(s_tprojFile, "\n");
    fflush(s_tprojFile);
}

/* Full projected sprite-quad log (flip-vs-occlusion investigation). RenderUnitSprite calls this
 * right after RotTransPers4 sets the 4 quad corners, for every DRAWN unit sprite (past the otIdx
 * gate). With all 4 corners we can read the actual quad geometry: whether the "top-half crop" is a
 * well-formed quad occluded by terrain vs a malformed/degenerate quad; whether the "flip" is a
 * corner-X-order reversal; and whether both correlate with the same otz(=SZ3) threshold. Env-gated
 * (VH_SPRITE_QUAD_LOG); call site is #ifdef PC_DEBUG_SPRITE_LOG (SPRITE_LOG=1 build). */
static FILE *s_squadFile = NULL;

void PC_DebugSpriteQuadLog(int gfx, int otz, int otIdx, int facingLeft,
                           int direction, int coords0z, int facingFront,
                           int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3,
                           int tileX, int tileZ) {
    static int enabled = -1;
    if (enabled < 0) enabled = (getenv("VH_SPRITE_QUAD_LOG") != NULL) ? 1 : 0;
    if (!enabled) return;
    if (s_squadFile == NULL) {
        s_squadFile = fopen("vh_sprite_quad.csv", "w");
        if (s_squadFile == NULL) return;
        /* pitch/yaw = gCameraRotation.vx/vy; facingLeft is driven ENTIRELY by
         * dir = ((yaw + direction + coords0z) & 0xfff) >> 10; facingLeft = !(dir==0||dir==1).
         * Logging the three raw inputs lets us reconstruct facingLeft and diff it against
         * BizHawk at a matched pose (see 41-sprite-orientation-baseline.lua). */
        fprintf(s_squadFile, "frame,pitch,yaw,gfx,otz,otIdx,facingLeft,direction,coords0z,facingFront,"
                            "tileX,tileZ,x0,y0,x1,y1,x2,y2,x3,y3");
        { int c; for (c = 0; c < 4; c++)
              fprintf(s_squadFile, ",ir2_%d,sz3_%d,n%d", c, c, c); }
        fprintf(s_squadFile, "\n");
    }
    fprintf(s_squadFile, "%ld,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
            s_vblankCount, gCameraRotation.vx, gCameraRotation.vy, gfx, otz, otIdx, facingLeft,
            direction, coords0z, facingFront,
            tileX, tileZ, x0, y0, x1, y1, x2, y2, x3, y3);
    /* box-quad corners went through TransformOne (RotTransPers4: v0,v1,v2 via RTPT then v3) so the
     * projection ring holds them back=3,2,1,0; dump each corner's IR2/SZ3/n to pin the ~4x Y-squish
     * (height = H*IR2/SZ3): is SZ3 4x too big, IR2 (box Y-half-extent) 4x too small, or n saturated? */
    { int c; int sx, sy, ir1, ir2, ir3, sz3, n;
      for (c = 0; c < 4; c++) {
          PC_GteProjEntry(3 - c, &sx, &sy, &ir1, &ir2, &ir3, &sz3, &n);
          fprintf(s_squadFile, ",%ld,%ld,%ld", ir2, sz3, n);
      } }
    fprintf(s_squadFile, "\n");
    fflush(s_squadFile);
}

void PadInit(int mode) {
    (void)mode;
    /* SDL2 is initialized by the platform's own startup code; nothing to
     * do here beyond letting the caller know pad reads are ready. */
}

/* Optional SDL_GameController support, alongside the keyboard map. Any plugged-in
 * controller (Xbox/PlayStation/etc., via SDL's built-in mapping DB) is read each
 * frame and OR'd into the same PSX pad word. Lazily initialized so a headless run
 * or a session with no pad costs nothing. */
static SDL_GameController *s_pad = NULL;
static int s_gcSubsysReady = -1; /* -1 untried, 0 failed, 1 ready */

static void pc_pad_ensure_open(void) {
    int i, n;
    if (s_gcSubsysReady == -1) {
        s_gcSubsysReady = (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) == 0) ? 1 : 0;
    }
    if (s_gcSubsysReady != 1) return;
    /* Drop a controller that was unplugged. */
    if (s_pad && !SDL_GameControllerGetAttached(s_pad)) {
        SDL_GameControllerClose(s_pad);
        s_pad = NULL;
    }
    if (s_pad) return;
    /* Open the first available controller (covers plug-in after launch). */
    n = SDL_NumJoysticks();
    for (i = 0; i < n; i++) {
        if (SDL_IsGameController(i)) {
            s_pad = SDL_GameControllerOpen(i);
            if (s_pad) break;
        }
    }
}

/* Stage 3 (1.1C): right-stick camera axis invert. Runtime-mutable (the in-game options overlay
 * toggles these and persists them to vandalhearts.ini's [camera] section); initialised once from
 * VH_CAM_INVERT_X/Y (which the ini loader sets). Not static -- the overlay reads/writes them. */
int g_camInvertX = 0;
int g_camInvertY = 1;   /* default INVERTED (modern twin-stick); PC_LoadCamInvert honours env/ini */

static void PC_LoadCamInvert(void) {
    static int loaded = 0;
    const char *e;
    if (loaded) return;
    loaded = 1;
    /* X defaults to normal; Y defaults to INVERTED (the modern twin-stick convention -- push up =
     * tilt the view down). An explicit VH_CAM_INVERT_Y=0 (env or the shipped ini) restores normal. */
    e = getenv("VH_CAM_INVERT_X"); g_camInvertX = (e && e[0] == '1') ? 1 : 0;
    e = getenv("VH_CAM_INVERT_Y"); g_camInvertY = e ? (e[0] == '1' ? 1 : 0) : 1;
}

static unsigned int pc_pad_read(void) {
    unsigned int p = 0;
    SDL_GameController *c;
    const int AXIS_DZ = 16000;

    PC_LoadCamInvert();          /* load invert defaults once, before the no-controller early-out, so
                                    keyboard-only play + the overlay still see the right values */
    pc_pad_ensure_open();
    c = s_pad;
    if (!c) return 0;

#define GC_BTN(b) SDL_GameControllerGetButton(c, (b))
    if (GC_BTN(SDL_CONTROLLER_BUTTON_DPAD_UP))       p |= PADLup;
    if (GC_BTN(SDL_CONTROLLER_BUTTON_DPAD_DOWN))     p |= PADLdown;
    if (GC_BTN(SDL_CONTROLLER_BUTTON_DPAD_LEFT))     p |= PADLleft;
    if (GC_BTN(SDL_CONTROLLER_BUTTON_DPAD_RIGHT))    p |= PADLright;
    /* SDL face buttons are positional (A=south, B=east, X=west, Y=north),
     * matching PSX Cross/Circle/Square/Triangle by position. */
    if (GC_BTN(SDL_CONTROLLER_BUTTON_A))             p |= PADRdown;   /* Cross    */
    if (GC_BTN(SDL_CONTROLLER_BUTTON_B))             p |= PADRright;  /* Circle   */
    if (GC_BTN(SDL_CONTROLLER_BUTTON_X))             p |= PADRleft;   /* Square   */
    if (GC_BTN(SDL_CONTROLLER_BUTTON_Y))             p |= PADRup;     /* Triangle */
    /* Stage 3 (1.1): the physical shoulders drive the ALLY-CYCLE, not the camera.
     * They go into the HIGH pad word ("pad 2", PADL1/PADR1 << 16), which the game reads
     * separately (gPad2State) from the camera rotation. The camera stays on the right
     * stick, which feeds the LOW-word L1/R1 below (gPadState). This is the decouple that
     * lets L1/R1 mean "cycle" while the stick still rotates the view. See exchange/62. */
    if (GC_BTN(SDL_CONTROLLER_BUTTON_LEFTSHOULDER))  p |= (unsigned)PADL1 << 16;
    if (GC_BTN(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER)) p |= (unsigned)PADR1 << 16;
    if (GC_BTN(SDL_CONTROLLER_BUTTON_START))         p |= PADstart;
    if (GC_BTN(SDL_CONTROLLER_BUTTON_BACK))          p |= PADselect;
    /* (L3/R3 are analog-pad only; VH reads a digital pad, so they're unmapped.) */
#undef GC_BTN
    /* Analog triggers -> L2/R2. */
    if (SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_TRIGGERLEFT)  > AXIS_DZ) p |= PADL2;
    if (SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) > AXIS_DZ) p |= PADR2;
    /* Left analog stick also drives the D-pad, so movement works either way. */
    {
        int lx = SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_LEFTX);
        int ly = SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_LEFTY);
        if (lx < -AXIS_DZ) p |= PADLleft;  else if (lx > AXIS_DZ) p |= PADLright;
        if (ly < -AXIS_DZ) p |= PADLup;    else if (ly > AXIS_DZ) p |= PADLdown;
    }
    /* QoL: right analog stick -> camera shoulder buttons, twin-stick feel.
     * Horizontal = L1/R1 (camera rotate), vertical = L2/R2 (camera elevation).
     * ORs with the physical shoulders/triggers above (both inputs work).
     * SDL Y axis is +down, matching the left-stick convention. */
    {
        int rx = SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_RIGHTX);
        int ry = SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_RIGHTY);
        /* Stage 3 (1.1C): per-axis right-stick invert (loaded once at the top of pc_pad_read). */
        if (g_camInvertX) rx = -rx;
        if (g_camInvertY) ry = -ry;
        if (rx < -AXIS_DZ) p |= PADL1;  else if (rx > AXIS_DZ) p |= PADR1;  /* left->L1, right->R1 */
        if (ry >  AXIS_DZ) p |= PADL2;  else if (ry < -AXIS_DZ) p |= PADR2; /* down->L2, up->R2   */
    }
    return p;
}

/* Stage-3 (1.1C) options-overlay pad filter. Sits between the raw pad word and what the game reads,
 * so it works in every context (battle, world map, movies) with zero src/ changes.
 *
 * Contract -- MUST be idempotent across repeated same-frame calls: the game reads PadRead(0) twice
 * per battle frame (gPadState battle_0201b8:125, gPad2State :149) and busy-waits on it in engine.c.
 * So no stateful multi-frame buffering of edges; only a chord *latch* (fires once per press, stable
 * when re-read), stateless SELECT masking, and the release-tracked swallow mask below.
 *
 * Chord = SELECT+START is the SOLE show/hide trigger. While SELECT is held we strip START+SELECT
 * (low word) from what the game sees, so the movie/battle START-skip (gPadStateNewPresses &
 * PAD_START) never mistakes the chord for a skip; START alone is untouched, so normal skipping still
 * works. (SELECT on the low word has no non-debug game use.) While open, the overlay owns all input
 * and the game receives a zero pad -- it keeps rendering but idles (no pause).
 *
 * Leak fix (user 2026-07-25): because the game sees a zero pad while the overlay is open, any button
 * still held at the instant it closes would read to the game as a fresh 0->held press (e.g. Circle
 * would pop the battle menu). So on close we latch every currently-held button into `swallow` and
 * keep masking each from the game until it is physically released. Covers all buttons/close paths. */
static unsigned int PC_OverlayFilterPad(unsigned int raw) {
    static unsigned int prev = 0;
    static int chordLatch = 0;
    static unsigned int swallow = 0;             /* buttons held at close, masked until released */
    int wasOpen = PC_OverlayIsOpen();
    unsigned int lo = raw & 0xFFFFu;
    int selHeld   = (lo & PADselect) != 0;
    int startHeld = (lo & PADstart)  != 0;
    unsigned int newpress = raw & ~prev;         /* rising edges vs the previous distinct read */
    unsigned int out = raw;

    if (selHeld && startHeld) {                   /* chord: toggle once per press (latched) */
        if (!chordLatch) { PC_OverlayToggle(); chordLatch = 1; }
    } else {
        chordLatch = 0;
    }
    if (selHeld) out &= ~(unsigned)(PADstart | PADselect);   /* SELECT gates START (movie-skip safe) */

    if (PC_OverlayIsOpen()) {
        /* Forward one edge at a time; the overlay routes each per its current screen. Cross is
         * Back/Cancel inside the overlay (the SELECT+START chord remains the only full close). */
        if (newpress & PADLup)    PC_OverlayInput(OVL_BTN_UP);
        if (newpress & PADLdown)  PC_OverlayInput(OVL_BTN_DOWN);
        if (newpress & PADLleft)  PC_OverlayInput(OVL_BTN_LEFT);
        if (newpress & PADLright) PC_OverlayInput(OVL_BTN_RIGHT);
        if (newpress & PADRright) PC_OverlayInput(OVL_BTN_CIRCLE);   /* Circle   */
        if (newpress & PADRdown)  PC_OverlayInput(OVL_BTN_CROSS);    /* Cross    */
        if (newpress & PADRleft)  PC_OverlayInput(OVL_BTN_SQUARE);   /* Square   */
        if (newpress & PADRup)    PC_OverlayInput(OVL_BTN_TRIANGLE); /* Triangle */
        swallow = raw;                                       /* keep the swallow set primed with all held */
        prev = raw;
        return 0;                                            /* freeze the game's pad while open */
    }
    if (wasOpen) swallow |= raw;                  /* closed THIS frame (e.g. via chord): swallow held */
    swallow &= raw;                               /* drop buttons the user has since released */
    out &= ~swallow;                              /* hide the still-held leftovers from the game */
    prev = raw;
    return out;
}

unsigned int PadRead(int id) {
    (void)id;

    SDL_PumpEvents();
    const Uint8 *keys = SDL_GetKeyboardState(NULL);

    unsigned int p1 = 0;
    if (keys[SDL_SCANCODE_UP])     p1 |= PADLup;
    if (keys[SDL_SCANCODE_DOWN])   p1 |= PADLdown;
    if (keys[SDL_SCANCODE_LEFT])   p1 |= PADLleft;
    if (keys[SDL_SCANCODE_RIGHT])  p1 |= PADLright;
    if (keys[SDL_SCANCODE_W])      p1 |= PADRup;       /* Triangle */
    if (keys[SDL_SCANCODE_S])      p1 |= PADRdown;     /* X */
    if (keys[SDL_SCANCODE_A])      p1 |= PADRleft;     /* Square */
    if (keys[SDL_SCANCODE_D])      p1 |= PADRright;    /* Circle */
    if (keys[SDL_SCANCODE_Q])      p1 |= PADL1;                 /* camera rotate (low word) */
    if (keys[SDL_SCANCODE_E])      p1 |= PADR1;
    /* Stage 3 (1.1): keyboard ally-cycle on [ / ] -> high word (pad 2), mirroring the
     * gamepad shoulders. Keeps keyboard functional after Square is freed for the overlay.
     * Full keyboard layout is documented/refined in docs/controls.md. */
    if (keys[SDL_SCANCODE_LEFTBRACKET])  p1 |= (unsigned)PADL1 << 16;   /* cycle previous */
    if (keys[SDL_SCANCODE_RIGHTBRACKET]) p1 |= (unsigned)PADR1 << 16;   /* cycle next */
    if (keys[SDL_SCANCODE_RETURN]) p1 |= PADstart;
    if (keys[SDL_SCANCODE_SPACE])  p1 |= PADselect;

    /* OR in any connected gamepad. */
    p1 |= pc_pad_read();

    /* PadRead(0) packs both controller ports into one 32-bit value (port 0
     * in the low 16 bits, port 1 in the high 16); the original code reads
     * player 2 via `PadRead(0/1) >> 0x10`. No second controller mapped
     * yet, so the high half stays zero. */
    return PC_OverlayFilterPad(p1);   /* Stage-3 (1.1C): chord/overlay input filter */
}

int VSync(int mode) {
    if (!s_vsyncInitialized) {
        s_nextVBlankMs = (double)SDL_GetTicks();
        s_vsyncInitialized = 1;
    }

    if (mode < 0) {
        /* Query mode: report the running vblank count without waiting. */
        return (int)s_vblankCount;
    }

    int waits = (mode == 0) ? 1 : mode;
    for (int i = 0; i < waits; i++) {
        Uint32 now = SDL_GetTicks();
        s_nextVBlankMs += FRAME_MS_F;         /* fractional deadline: accumulates the 0.68ms/frame */
        if ((double)now < s_nextVBlankMs) {
            SDL_Delay((Uint32)(s_nextVBlankMs - now));
        } else {
            /* fell behind (slow frame / stall) -- resync rather than racing to catch up, so a hitch
             * doesn't make the game briefly run fast to "make up" lost time. */
            s_nextVBlankMs = (double)now;
        }
        s_vblankCount++;
    }

    LogCameraTraceRow();
    LogRandSeedRow();
    LogAiChainRow();
    LogCameraMatrixRow();
    LogTerrainRow();

    { extern void PC_CdXaUpdate(void); PC_CdXaUpdate(); } /* XA music streaming pump */
    { extern void PC_SeqTick(void); PC_SeqTick(); }       /* SEQ (sequenced music) sequencer */

    s_fpsLastMode = mode;
    s_fpsWindowCalls++;
    if (s_fpsWindowStart == 0) {
        s_fpsWindowStart = SDL_GetTicks();
    } else {
        Uint32 windowElapsed = SDL_GetTicks() - s_fpsWindowStart;
        if (windowElapsed >= 1000) {
            double fps = s_fpsWindowCalls * 1000.0 / windowElapsed;
            fprintf(stderr, "[FPS] VSync() calls/sec=%.2f (mode=%d) window_ms=%u\n",
                    fps, s_fpsLastMode, windowElapsed);
            s_fpsWindowStart = SDL_GetTicks();
            s_fpsWindowCalls = 0;
        }
    }

    return (int)s_vblankCount;
}

int ResetCallback(void) {
    /* No VSync/interrupt callbacks are registered by anything in this
     * interface yet -- nothing to reset. */
    return 0;
}
