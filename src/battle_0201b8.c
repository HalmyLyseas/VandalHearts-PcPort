#include "common.h"
#include "object.h"
#include "battle.h"
#include "units.h"
#include "state.h"
#include "field.h"
#include "window.h"
#include "graphics.h"
#include "audio.h"

#ifdef PC_FEAT
/* Language packs (platform/pc/src/pc_lang.c): string literals below wrapped in PC_LANGSTR are
 * offered to the active pack by CONTENT (matched by hash); with no pack, or no entry, the literal
 * itself is returned. The matching build expands the macro to the bare literal. */
extern u8 *PC_LangStr(const char *lit);
#define PC_LANGSTR(s) (PC_LangStr(s))
#else
#define PC_LANGSTR(s) (s)
#endif


typedef struct EnemyEventSpawn {
   u8 z, x, stripIdx, level, expMulti;
} EnemyEventSpawn;

typedef struct OromeLakeEnemySpawn {
   u8 z, x, stripIdx, level;
} OromeLakeEnemySpawn;

extern MATRIX gMatrix_8012c068;

HiddenItem gMapHiddenItems[BATTLE_CT][2] = {
    [0] = {{99, 99, ITEM_NULL}, {99, 99, ITEM_NULL}},
    [1] = {{99, 99, ITEM_NULL}, {99, 99, ITEM_NULL}},
    [2] = {{99, 99, ITEM_NULL}, {99, 99, ITEM_NULL}},
    [3] = {{99, 99, ITEM_NULL}, {99, 99, ITEM_NULL}},
    [4] = {{99, 99, ITEM_NULL}, {99, 99, ITEM_NULL}},
    [5] = {{99, 99, ITEM_NULL}, {99, 99, ITEM_NULL}},
    [6] = {{99, 99, ITEM_NULL}, {99, 99, ITEM_NULL}},
    [7] = {{99, 99, ITEM_NULL}, {99, 99, ITEM_NULL}},
    [8] = {{99, 99, ITEM_NULL}, {99, 99, ITEM_NULL}},
    [9] = {{99, 99, ITEM_NULL}, {99, 99, ITEM_NULL}},
    [10] = {{0, 1, ITEM_MAD_BOOK}, {14, 0, ITEM_L_SWORD}},
    [11] = {{11, 11, ITEM_MUSHROOM}, {8, 19, ITEM_L_BOW}},
    [12] = {{17, 3, ITEM_FIRE_GEM}, {99, 99, ITEM_NULL}},
    [13] = {{99, 99, ITEM_NULL}, {99, 99, ITEM_NULL}},
    [14] = {{99, 99, ITEM_NULL}, {99, 99, ITEM_NULL}},
    [15] = {{99, 99, ITEM_NULL}, {99, 99, ITEM_NULL}},
    [16] = {{0, 12, ITEM_MUSHROOM}, {9, 13, ITEM_MOON_PIE}},
    [17] = {{17, 22, ITEM_MAD_BOOK}, {0, 22, ITEM_MACROMAN}},
    [18] = {{26, 5, ITEM_BANANA}, {99, 99, ITEM_NULL}},
    [19] = {{8, 24, ITEM_IRONBOOT}, {0, 8, ITEM_CHAIN}},
    [20] = {{99, 99, ITEM_NULL}, {99, 99, ITEM_NULL}},
    [21] = {{2, 0, ITEM_GOLDCOIN}, {19, 0, ITEM_MAGE_GEM}},
    [22] = {{99, 99, ITEM_NULL}, {99, 99, ITEM_NULL}},
    [23] = {{99, 99, ITEM_NULL}, {99, 99, ITEM_NULL}},
    [24] = {{1, 10, ITEM_GOLDCOIN}, {10, 23, ITEM_UNICORN}},
    [25] = {{3, 8, ITEM_IRONBOOT}, {17, 1, ITEM_TAROT}},
    [26] = {{6, 1, ITEM_M_STAFF}, {99, 99, ITEM_NULL}},
    [27] = {{99, 99, ITEM_NULL}, {99, 99, ITEM_NULL}},
    [28] = {{20, 7, ITEM_GOLDCOIN}, {6, 16, ITEM_MANA_KEY}},
    [29] = {{1, 6, ITEM_GOLD_AXE}, {99, 99, ITEM_NULL}},
    [30] = {{1, 2, ITEM_KINGFOIL}, {18, 8, ITEM_AURA_GEM}},
    [31] = {{99, 99, ITEM_NULL}, {99, 99, ITEM_NULL}},
    [32] = {{4, 52, ITEM_RAMEN}, {99, 99, ITEM_NULL}},
    [33] = {{1, 13, ITEM_KINGFOIL}, {21, 4, ITEM_MITHRIL}},
    [34] = {{99, 99, ITEM_NULL}, {99, 99, ITEM_NULL}},
    [35] = {{3, 2, ITEM_HELSTONE}, {8, 31, ITEM_MITHRIL}},
    [36] = {{99, 99, ITEM_NULL}, {99, 99, ITEM_NULL}},
    [37] = {{6, 8, ITEM_LOGOS_K_}, {99, 99, ITEM_NULL}},
    [38] = {{10, 0, ITEM_SHIVBOOK}, {14, 21, ITEM_MITHRIL}},
    [39] = {{99, 99, ITEM_NULL}, {99, 99, ITEM_NULL}},
    [40] = {{5, 3, ITEM_NCKLACE}, {99, 99, ITEM_NULL}},
    [41] = {{99, 99, ITEM_NULL}, {99, 99, ITEM_NULL}},
    [42] = {{12, 5, ITEM_GOLD_AXE}, {9, 18, ITEM_MOJOROBE}},
    [43] = {{99, 99, ITEM_NULL}, {99, 99, ITEM_NULL}}};

static EnemyEventSpawn sEnemyEventSpawns[6] = {{2, 19, 17, 29, 20}, {4, 19, 18, 25, 12},
                                               {3, 18, 18, 25, 12}, {3, 20, 18, 25, 12},
                                               {1, 18, 19, 24, 12}, {1, 20, 19, 24, 12}};

static OromeLakeEnemySpawn sOromeLakeEnemySpawns1[7] = {
    {6, 13, 13, 23}, {7, 13, 13, 23}, {3, 20, 12, 24}, {2, 19, 12, 24},
    {2, 21, 12, 24}, {1, 20, 12, 24}, {0, 0, 0, 0}};

static OromeLakeEnemySpawn sOromeLakeEnemySpawns2[3] = {
    {9, 11, 12, 24}, {7, 10, 12, 24}, {0, 0, 0, 0}};

void ShowReceivedItemDialog(u8 item, u8 windowId, u8 param_3) {
   s32 len;

   gClearSavedPadState = 1;
   len = GetItemNameLength(item);

   DrawWindow(windowId, 0, 0, 240, 36, 40, 90, WBS_CROSSED, 0);
   DrawText(80, 11, 25, 2, 0, gItemNames[item]);
   DrawText(80 + len * 8, 11, 25, 2, 0, PC_LANGSTR("!"));
   DrawText(16, 11, 25, 2, 0, PC_LANGSTR("you got "));

   // This references param_3 just enough to get the prologue/epilogue correct without otherwise
   // affecting codegen
   if (param_3) {
      DisplayCustomWindow(windowId, 0, 1, 0, 0, 0);
   } else {
      DisplayCustomWindow(windowId, 0, 1, 0, 0, 0);
   }
}

void ShowDepotReceivedItemDialog(u8 item, u8 windowId, u8 param_3) {
   s32 len;

   gClearSavedPadState = 1;
   len = GetItemNameLength(item);

   DrawWindow(windowId, 0, 0, 240, 72, 40, 90, WBS_CROSSED, 0);
   DrawText(80, 11, 25, 2, 0, gItemNames[item]);
   DrawText(80 + len * 8, 11, 25, 2, 0, PC_LANGSTR("!"));
   DrawText(16, 11, 25, 2, 0, PC_LANGSTR("you got "));
   DrawText(16, 28, 25, 2, 0, PC_LANGSTR("Can't hold anymore."));
   DrawText(16, 45, 25, 2, 0, PC_LANGSTR("Sent to supply wagon."));

   if (param_3) {
      DisplayCustomWindow(windowId, 0, 1, 0, 0, 0);
   } else {
      DisplayCustomWindow(windowId, 0, 1, 0, 0, 0);
   }
}

u8 s_padHoldCounter_801231cc;

void UpdateInput(void) {
   static u32 previousPadState = 0;
   static u32 previousPad2State = 0;

   previousPadState = gPadState;
   gPadState = PadRead(0);

   if (previousPadState == gPadState) {
      s_padHoldCounter_801231cc++;
   } else {
      s_padHoldCounter_801231cc = 0;
   }

   gPadStateNewPresses = (previousPadState ^ gPadState) & gPadState;

   if (gState.vsyncMode == 2) {
      if (s_padHoldCounter_801231cc == 10) {
         s_padHoldCounter_801231cc = 8;
         gPadStateNewPresses = gPadState & PAD_DPAD;
      }
   } else {
      if (s_padHoldCounter_801231cc == 20) {
         s_padHoldCounter_801231cc = 17;
         gPadStateNewPresses = gPadState & PAD_DPAD;
      }
   }

   previousPad2State = gPad2State;

   gPad2State = PadRead(0) >> 0x10;
   gPad2StateNewPresses = (previousPad2State ^ gPad2State) & gPad2State;

   if (gClearSavedPadState == 0) {
      gSavedPadState = gPadState;
      gSavedPadStateNewPresses = gPadStateNewPresses;
      gSavedPad2State = gPad2State;
      gSavedPad2StateNewPresses = gPad2StateNewPresses;
   } else {
      gSavedPadState = 0;
      gSavedPadStateNewPresses = 0;
      gSavedPad2State = 0;
      gSavedPad2StateNewPresses = 0;
   }
}

s32 D_801231D0;
s32 D_801231D4;
s32 D_801231D8; // unused

void CenterCamera(u8 cursor) {
   s32 dstX, dstZ;

   if ((!gPlayerControlSuppressed && !gIsEnemyTurn && !gMapCursorSuppressed) || cursor) {

      if (gMapCursorX > (gMapMaxX - 1) - gMapMarginX) {
         D_801231D0 = (gMapMaxX - gMapCursorX) - gMapMarginX;
      }
      if (gMapCursorX < gMapMinX + gMapMarginX + 1) {
         D_801231D0 = (gMapMinX + gMapMarginX) - gMapCursorX;
      }
      if (gMapCursorZ > (gMapMaxZ - 1) - gMapMarginZ) {
         D_801231D4 = (gMapMaxZ - gMapCursorZ) - gMapMarginZ;
      }
      if (gMapCursorZ < gMapMinZ + gMapMarginZ + 1) {
         D_801231D4 = (gMapMinZ + gMapMarginZ) - gMapCursorZ;
      }

      dstX = (-D_80122E28 - (gMapSizeX / 2) + D_801231D0) * 32;
      dstZ = (-D_80122E2C - (gMapSizeZ / 2) + D_801231D4) * 32;

      gCameraPos.vx += (dstX - gCameraPos.vx) >> 2;
      gCameraPos.vz += (dstZ - gCameraPos.vz) >> 2;
   } else {
      D_80122E28 = ((-gMapSizeX / 2) * 32 - gCameraPos.vx) / 32;
      D_80122E2C = ((-gMapSizeZ / 2) * 32 - gCameraPos.vz) / 32;

      D_80122E28 = CLAMP(D_80122E28, gMapMinX, (gMapMaxX + 1) - gMapSizeX);
      D_80122E2C = CLAMP(D_80122E2C, gMapMinZ, (gMapMaxZ + 1) - gMapSizeZ);
   }
}

void UpdateCamera(void) {
   if (gPlayerControlSuppressed || gIsEnemyTurn || gMapCursorSuppressed) {
      UpdateNonPlayerCamera();
   } else {
      UpdatePlayerCamera();
   }
}

void UpdatePlayerCamera(void) {
   static s16 mapYRotationSpeed = 0;
   static s16 mapXRotationSpeed = 0;
   s32 tmp;

   if (gSavedPadStateNewPresses & PAD_L2) {
      mapXRotationSpeed = -0x20;
   }
   if (gSavedPadStateNewPresses & PAD_R2) {
      mapXRotationSpeed = 0x20;
   }

#ifdef PC_FEAT
   /* Stage-3 (1.4 F6): 5-stop elevation. Rest points vx = 0x80 + k*0xC0 (k=0..4) =
    * 11.25 / 28.125 / 45 / 61.875 / 78.75 deg -- the clamped 11.25..78.75 range split into 4 equal
    * 16.875-deg (0xC0) intervals. Every stop lands exactly on the 1/4096-deg angle grid and it includes
    * a clean 45. (A 0x80 bitmask gives 7 stops -- too many, near-redundant; a 5-stop split needs an
    * explicit interval, since the retail `(vx+0x80)&mask` trick only produces power-of-2 spacing.)
    * Pitch (vx) is NOT quadrant-coupled (only yaw is), and every auto-camera (cutscene/SFX) drives vx by
    * raw values + save/restore, so this stays isolated to player control. Speed 0x20 divides 0xC0, so
    * taps land exactly on the stops. */
   gCameraRotation.vx += mapXRotationSpeed;
   if (((gCameraRotation.vx - 0x80) % 0xC0) == 0) {
      mapXRotationSpeed = 0;
   }
   gCameraRotation.vx = CLAMP(gCameraRotation.vx, 0x80, 0x380);
#else
   gCameraRotation.vx += 0x80;
   gCameraRotation.vx += mapXRotationSpeed;
   if ((gCameraRotation.vx & 0xff) == 0) {
      mapXRotationSpeed = 0;
   }
   gCameraRotation.vx -= 0x80;
   gCameraRotation.vx = CLAMP(gCameraRotation.vx, 0x80, 0x380);
#endif

   gCameraRotation.vy += DEG(45);
   gCameraRotation.vy += mapYRotationSpeed;
   if ((gCameraRotation.vy & 0x3ff) == 0) {
      mapYRotationSpeed = 0;
   }
   gCameraRotation.vy -= DEG(45);

   if (gSavedPadState & PAD_R1) {
      mapYRotationSpeed = 0x80;
   }
   if (gSavedPadState & PAD_L1) {
      mapYRotationSpeed = -0x80;
   }

   if (gMapCursorX <= gMapMarginX + D_80122E28) {
      D_80122E28 = gMapCursorX - gMapMarginX;
   }
   if (gMapCursorZ <= gMapMarginZ + D_80122E2C) {
      D_80122E2C = gMapCursorZ - gMapMarginZ;
   }
   // Odd use of temps here; better match? (tmp = 1 also works)
   if (gMapCursorX >= (gMapSizeX + D_80122E28) - (tmp = gMapMarginX + 1)) {
      D_80122E28 = (gMapCursorX - gMapSizeX) + tmp;
   }
   if (gMapCursorZ >= (gMapSizeZ + D_80122E2C) - (tmp = gMapMarginZ + 1)) {
      D_80122E2C = (gMapCursorZ - gMapSizeZ) + tmp;
   }

   D_80122E28 = CLAMP(D_80122E28, gMapMinX, (gMapMaxX + 1) - gMapSizeX);
   D_80122E2C = CLAMP(D_80122E2C, gMapMinZ, (gMapMaxZ + 1) - gMapSizeZ);

   RotMatrix(&gCameraRotation, &gCameraMatrix);
   TransMatrix(&gCameraMatrix, &gCameraZoom);
   RotMatrix(&gLightRotation, &gMatrix_8012c068);

   if (gState.enableMapScaling) {
      ScaleMatrix(&gCameraMatrix, &gMapScale);
   }

   SetRotMatrix(&gCameraMatrix);
   SetTransMatrix(&gCameraMatrix);
}

void UpdateNonPlayerCamera(void) {
   RotMatrix(&gCameraRotation, &gCameraMatrix);
   TransMatrix(&gCameraMatrix, &gCameraZoom);
   RotMatrix(&gLightRotation, &gMatrix_8012c068);

   if (gState.enableMapScaling) {
      ScaleMatrix(&gCameraMatrix, &gMapScale);
   }

   SetRotMatrix(&gCameraMatrix);
   SetTransMatrix(&gCameraMatrix);
}

s16 s_introYRotSpeed_801231dc;
s16 s_introXRotSpeed_801231e0;
s16 s_introDstCamRotY_801231e4;

#undef OBJF
#define OBJF 597
void Objf597_BattleIntro(Object *obj) {
   s16 diff;

   gCameraRotation.vy -= s_introYRotSpeed_801231dc;
   gCameraRotation.vx -= s_introXRotSpeed_801231e0;

   switch (obj->state) {
   case 0:
      gSignal1 = 1;
      s_introDstCamRotY_801231e4 = (gMapCursorStartingPos[gState.mapNum].y * DEG(90)) + DEG(45);
      s_introYRotSpeed_801231dc = 8;
      s_introXRotSpeed_801231e0 = 5;
      gCameraZoom.vz = 800;
      gCameraRotation.vx = DEG(90);
      gPlayerControlSuppressed = 1;
      OBJ.timer = 30;
      obj->state++;
   // fallthrough
   case 1:
      if (--OBJ.timer == 0) {
         // Display battle conditions
         DrawWindow(0x42, 0, 0, 320, 72, 330, 80, WBS_DRAGON, 0);
         DrawText(20, 20, 40, 2, 0, gState.currentTextPointers[0]);
         DisplayBasicWindow(0x42);
         DisplayBasicWindow(0x43);
         SlideWindowTo(0x42, 1, 80);
         SlideWindowTo(0x43, 249, 80);
         OBJ.timer = 80;
         obj->state++;
      }
      break;
   case 2:
      if (--OBJ.timer == 0 || PressedCircleOrX()) {
         SlideWindowTo(0x42, -350, 80);
         SlideWindowTo(0x43, -102, 80);
         OBJ.timer = 10;
         obj->state++;
      }
      break;
   case 3:
      if (--OBJ.timer == 0) {
         CloseWindow(0x42);
         CloseWindow(0x43);
         OBJ.timer = 10;
         obj->state++;
      }
      break;
   case 4:
      if (--OBJ.timer == 0) {
         DrawWindow(0x42, 0, 0, 144, 64, 84, 90, WBS_DRAGON, 0);
         DrawText(20, 24, 20, 0, 0, PC_LANGSTR("Begin Battle"));
         DisplayCustomWindow(0x42, 2, 1, 0, 0, 0);
         OBJ.timer = 30;
         obj->state++;
      }
      break;
   case 5:
      if (--OBJ.timer == 0 || PressedCircleOrX()) {
         CloseWindow(0x42);
         OBJ.timer = 20;
         obj->state++;
      }
      break;
   case 6:
      if (--OBJ.timer == 0) {
         s_introYRotSpeed_801231dc = 0;
         s_introXRotSpeed_801231e0 = 0;
         gPlayerControlSuppressed = 0;

         gCameraRotation.vy &= 0xfff;

         diff = s_introDstCamRotY_801231e4 - gCameraRotation.vy;
         if (diff > 0) {
            if (diff > DEG(180)) {
               s_introDstCamRotY_801231e4 -= DEG(360);
            }
         } else if (diff < DEG(-180)) {
            s_introDstCamRotY_801231e4 += DEG(360);
         }

         OBJ.timer = 20;
         obj->state++;
      }
      break;
   case 7:
      if (--OBJ.timer != 0) {
         gCameraRotation.vy += (s_introDstCamRotY_801231e4 - gCameraRotation.vy) >> 2;
         gCameraRotation.vx += (DEG(33.75) - gCameraRotation.vx) >> 2;
         gCameraZoom.vz += (640 - gCameraZoom.vz) >> 2;
      } else {
         gCameraRotation.vy = s_introDstCamRotY_801231e4;
         gCameraRotation.vx = DEG(33.75);
         gCameraZoom.vz = 640;
         gClearSavedPadState = 0;
         obj->functionIndex = OBJF_NULL;
         gSignal1 = 0;
      }
      break;
   }
}

#undef OBJF
#define OBJF 587
void Objf587_BattleEnemyEvent(Object *obj) {
   s32 i;
   Object *sprite, *fx;
   EnemyEventSpawn *pSpawn;

   if (gState.battleEval != BATTLE_EVAL_NONE) {
      obj->functionIndex = OBJF_NULL;
      return;
   }

   switch (obj->state) {
   case 0:
      gPlayerControlSuppressed = 1;
      if (gState.mapNum == 40) {
         if (gState.mapState.s.field_0x0 == 1) {
            obj->state = 16;
            return;
         }
         if (gState.turn == 1 && FindUnitByNameIdx(UNIT_KANE)) {
            obj->state = 10;
            return;
         }
      }
      if (gState.mapNum == 26) {
         if (gState.mapState.s.field_0x13 == 1 && FindUnitByNameIdx(UNIT_LANDO)) {
            gState.mapState.s.field_0x13 = 2;
            obj->state = 11;
            return;
         }
      }
      if (gState.mapNum == 32) {
         if (gState.turn == 2) {
            obj->state = 12;
            return;
         }
         if (gState.turn == 5) {
            obj->state = 13;
            return;
         }
         if (gState.turn == 8) {
            obj->state = 14;
            return;
         }
         if (gState.turn == 11) {
            obj->state = 15;
            return;
         }
      }
      if (gState.mapNum == 38) {
         if (gState.turn == 3) {
            obj->state = 17;
            return;
         }
      }
      gPlayerControlSuppressed = 0;
      obj->state = 99;
      return;

   case 10:

      switch (obj->state2) {
      case 0:
         SetupBattleMsgBox(UNIT_KANE, PORTRAIT_KANE, 13);
         obj->state2++;
         break;
      case 1:
         if (gState.msgFinished) {
            SetupBattleMsgBox(UNIT_ASH, PORTRAIT_ASH, 14);
            obj->state2++;
         }
         break;
      case 2:
         if (gState.msgFinished) {
            SetupBattleMsgBox(UNIT_KANE, PORTRAIT_KANE_ANGRY, 15);
            obj->state2++;
         }
         break;
      case 3:
         if (gState.msgFinished) {
            obj->state = 99;
         }
         break;
      } // switch (obj->state2) (via state:10)

      return;

   case 11:

      switch (obj->state2) {
      case 0:
         gState.mapState.s.field_0x13 = 2;
         SetupBattleMsgBox(UNIT_LANDO, PORTRAIT_LANDO_514, 19);
         obj->state2++;
         break;
      case 1:
         if (gState.msgFinished) {
            obj->state = 99;
         }
         break;
      } // switch (obj->state2) (via state:11)

      return;

   case 12:

      switch (obj->state2) {
      case 0:
         SetupBattleMsgBoxWithoutTail(UNIT_DALLAS, PORTRAIT_C_KNIGHT, 13);
         obj->state2++;
         break;
      case 1:
         if (gState.msgFinished) {
            SetupBattleMsgBox(UNIT_DALLAS, PORTRAIT_DALLAS_516, 14);
            obj->state2++;
         }
         break;
      case 2:
         if (gState.msgFinished) {
            gState.field_0x96 = 1;
            gState.mapState.s.field_0x0++;
            obj->state2++;
         }
         break;
      case 3:
         if (gState.field_0x96 == 0) {
            obj->state = 99;
         }
         break;
      } // switch (obj->state2) (via state:12)

      return;

   case 13:

      switch (obj->state2) {
      case 0:
         SetupBattleMsgBoxWithoutTail(UNIT_DALLAS, PORTRAIT_C_KNIGHT, 15);
         obj->state2++;
         break;
      case 1:
         if (gState.msgFinished) {
            SetupBattleMsgBox(UNIT_DALLAS, PORTRAIT_DALLAS_516, 16);
            obj->state2++;
         }
         break;
      case 2:
         if (gState.msgFinished) {
            gState.field_0x96 = 1;
            gState.mapState.s.field_0x0++;
            obj->state2++;
         }
         break;
      case 3:
         if (gState.field_0x96 == 0) {
            obj->state = 99;
         }
         break;
      } // switch (obj->state2) (via state:13)

      return;

   case 14:

      switch (obj->state2) {
      case 0:
         SetupBattleMsgBoxWithoutTail(UNIT_DALLAS, PORTRAIT_C_KNIGHT, 17);
         obj->state2++;
         break;
      case 1:
         if (gState.msgFinished) {
            SetupBattleMsgBox(UNIT_DALLAS, PORTRAIT_DALLAS_516, 18);
            obj->state2++;
         }
         break;
      case 2:
         if (gState.msgFinished) {
            gState.field_0x96 = 1;
            gState.mapState.s.field_0x0++;
            obj->state2++;
         }
         break;
      case 3:
         if (gState.field_0x96 == 0) {
            obj->state = 99;
         }
         break;
      } // switch (obj->state2) (via state:14)

      return;

   case 15:

      switch (obj->state2) {
      case 0:
         SetupBattleMsgBox(UNIT_DALLAS, PORTRAIT_DALLAS_516, 19);
         obj->state2++;
         break;
      case 1:
         if (gState.msgFinished) {
            gState.field_0x96 = 1;
            gState.mapState.s.field_0x0++;
            obj->state2++;
         }
         break;
      case 2:
         if (gState.field_0x96 == 0) {
            obj->state = 99;
         }
         break;
      } // switch (obj->state2) (via state:15)

      return;

   case 16:

      switch (obj->state2) {
      case 0:
         PerformAudioCommand(AUDIO_CMD_PREPARE_XA(28));
         gState.mapState.s.field_0x0 = 2;
         SetupBattleMsgBox(UNIT_KANE, PORTRAIT_KANE_HAPPY, 16);

         // Find an open tile for Xeno to spawn onto
         i = 12;
         while (gMapUnitsPtr[i][2].s.unitIdx != 0) {
            i--;
         }

         OBJ.xenoSpawnZ = i;
         SetupBattleUnit(17, OBJ.xenoSpawnZ, 2, 10, TEAM_ENEMY, DIR_NORTH, 1, 20, 1);
         gTileStateGridPtr[OBJ.xenoSpawnZ][2].action = TA_25;
         obj->state2++;
         break;

      case 1:
         if (gState.msgFinished) {
            SetupBattleMsgBox(UNIT_XENO, PORTRAIT_XENO, 17);
            obj->state2++;
         }
         break;

      case 2:
      case 3:
         obj->state2++;
         break;

      case 4:
         PerformAudioCommand(AUDIO_CMD_PLAY_XA(28));
         sprite = GetUnitSpriteAtPosition(OBJ.xenoSpawnZ, 2);
         fx = Obj_GetUnused();
         fx->functionIndex = OBJF_STRETCH_WARP_SPRITE;
         fx->d.objf062.sprite = sprite;
         fx->mem = 1;
         obj->state2++;
         break;

      case 5:
         if (gState.msgFinished) {
            SetupBattleMsgBox(UNIT_KANE, PORTRAIT_KANE, 18);
            obj->state2++;
         }
         break;

      case 6:
         if (gState.msgFinished) {
            SetupBattleMsgBox(UNIT_XENO, PORTRAIT_XENO, 19);
            obj->state2++;
         }
         break;

      case 7:
         if (gState.msgFinished) {
            SetupBattleMsgBox(UNIT_KANE, PORTRAIT_KANE_ANGRY, 20);
            obj->state2++;
         }
         break;

      case 8:
         if (gState.msgFinished) {
            SetupBattleMsgBox(UNIT_XENO, PORTRAIT_XENO, 21);
            obj->state2++;
         }
         break;

      case 9:
         if (gState.msgFinished) {
            SetupBattleMsgBox(UNIT_KANE, PORTRAIT_KANE_HAPPY, 22);
            obj->state2++;
         }
         break;

      case 10:
         if (gState.msgFinished) {
            SetupBattleMsgBox(UNIT_XENO, PORTRAIT_XENO, 23);
            obj->state2++;
         }
         break;

      case 11:
         if (gState.msgFinished) {
            PerformAudioCommand(AUDIO_CMD_FADE_OUT_32_4);
            OBJ.timer = 20;
            obj->state2++;
         }
         break;

      case 12:
         if (--OBJ.timer == 0) {
            PerformAudioCommand(AUDIO_CMD_STOP_ALL);
            LoadSeqSet(0x19);
            FinishLoadingSeq();
            PerformAudioCommand(AUDIO_CMD_PLAY_SFX(230));
            sprite = FindUnitSpriteByNameIdx(UNIT_KANE);
            fx = Obj_GetUnused();
            fx->functionIndex = OBJF_MID_BATTLE_TRANSFORMATION;
            fx->x1.n = sprite->x1.n;
            fx->z1.n = sprite->z1.n;
            gState.D_801405A4 = 0;
            SetupBattleMsgBox(UNIT_KANE, PORTRAIT_KANE_CURSED, 24);
            obj->state2++;
            OBJ.timer = 200;
         }
         break;

      case 13:
         if (--OBJ.timer == 0) {
            gState.msgBoxFinished = 1;
         }
         if (gState.msgFinished) {
            PerformAudioCommand(AUDIO_CMD_PLAY_SFX(231));
            gState.D_801405A4 = 1;
            PerformAudioCommand(AUDIO_CMD_PREPARE_XA(28));
            SetupBattleMsgBox(UNIT_XENO, PORTRAIT_XENO, 25);
            PerformAudioCommand(AUDIO_CMD_PLAY_SEQ(11));
            obj->state2++;
         }
         break;

      case 14:
         if (gState.msgFinished) {
            sprite = FindUnitSpriteByNameIdx(UNIT_KANE);
            OBJ_TILE_STATE(sprite).action = TA_22;
            OBJ.spawnZ = sprite->z1.s.hi;
            OBJ.spawnX = sprite->x1.s.hi;
            PerformAudioCommand(AUDIO_CMD_PLAY_XA(28));
            sprite = GetUnitSpriteAtPosition(OBJ.xenoSpawnZ, 2);
            fx = Obj_GetUnused();
            fx->functionIndex = OBJF_STRETCH_WARP_SPRITE;
            fx->d.objf062.sprite = sprite;
            OBJ.timer = 30;
            obj->state2++;
         }
         break;

      case 15:
         if (--OBJ.timer == 0) {
            SetupBattleUnit(13, OBJ.spawnZ, OBJ.spawnX, 30, TEAM_ENEMY, DIR_WEST, 1, 20, 4);
            gTileStateGridPtr[OBJ.xenoSpawnZ][2].action = TA_22;
            SetupBattleMsgBox(UNIT_ASH, PORTRAIT_ASH, 26);
            obj->state2++;
         }
         break;

      case 16:
         if (gState.msgFinished) {
            SetupBattleMsgBox(UNIT_KANE, PORTRAIT_KANE_BLK_KNIGHT, 29);
            obj->state2++;
         }
         break;

      case 17:
         if (gState.msgFinished) {
            PerformAudioCommand(AUDIO_CMD_FADE_OUT_8_4);
            OBJ.timer = 75;
            obj->state2++;
         }
         break;

      case 18:
         if (--OBJ.timer == 0) {
            PerformAudioCommand(AUDIO_CMD_PLAY_SEQ(1));
            obj->state = 99;
         }
         break;

      } // switch (obj->state2) (via state:16)

      return;

   case 17:

      switch (obj->state2) {
      case 0:
         gState.mapState.s.field_0x13 = 1;
         SetupBattleMsgBox(UNIT_SABINA, PORTRAIT_SABINA, 20);
         obj->state2++;
         break;

      case 1:
      case 2:
      case 3:
         obj->state2++;
         break;

      case 4:
         pSpawn = &sEnemyEventSpawns[0];

         for (i = 0; i < 6; i++) {
            s32 iz, ix;

            iz = pSpawn->z;
            ix = pSpawn->x;

            while (gMapUnitsPtr[iz][ix].s.team != TEAM_NULL ||
                   gTerrainPtr[iz][ix].s.terrain >= TERRAIN_BARREN) {
               iz++;
               if (iz == 16) {
                  iz = 0;
                  ix++;
               }
            }

            SetupBattleUnit(pSpawn->stripIdx, iz, ix, pSpawn->level, TEAM_ENEMY, DIR_SOUTH, 100,
                            pSpawn->expMulti, 16);
            gMapUnitsPtr[iz][ix].s.team = TEAM_ENEMY;
            pSpawn++;
         }

         obj->state2++;
         break;

      case 5:
         if (gState.msgFinished) {
            SetupBattleMsgBox(UNIT_KANE, PORTRAIT_KANE, 13);
            obj->state2++;
         }
         break;

      case 6:
         if (gState.msgFinished) {
            SetupBattleMsgBox(UNIT_SABINA, PORTRAIT_SABINA, 14);
            obj->state2++;
         }
         break;

      case 7:
         if (gState.msgFinished) {
            SetupBattleMsgBox(UNIT_ASH, PORTRAIT_ASH, 15);
            do {
               // do-while fixes regalloc; can't get this function to match without at least one of
               // these; not sure if this implies something should be a macro or if it just happens
               // to nudge the regs into place
               obj->state2++;
            } while (0);
         }
         break;

      case 8:
         if (gState.msgFinished) {
            obj->state = 99;
         }
         break;

      } // switch (obj->state2) (via state:17)

      return;

   case 99:
      gState.field_0x98 = 0;
      gPlayerControlSuppressed = 0;
      obj->functionIndex = OBJF_NULL;
      return;

   } // switch (obj->state)
}

#undef OBJF
#define OBJF 585
void Objf585_BattlePlayerEvent(Object *obj) {
   s32 i;

   if (gState.battleEval != BATTLE_EVAL_NONE) {
      obj->functionIndex = OBJF_NULL;
      return;
   }

   switch (obj->state) {
   case 0:
      gPlayerControlSuppressed = 1;
      if (gState.mapNum == 11) {
         if (gState.turn == 2) {
            // Only show dialog if all 3 party members are still alive
            s32 numIncapacitated = 3;
            for (i = 1; i <= 3; i++) {
               if (FindUnitByNameIdx(i)) {
                  numIncapacitated--;
               }
            }
            if (numIncapacitated == 0) {
               obj->state = 10;
               return;
            }
         }
         if (gState.turn == 3 && gState.mapState.s.field_0x0 == 0) {
            // Call attention to drawbridge switch
            obj->state = 11;
            return;
         }
      }
      if (gState.mapNum == 13) {
         if (gState.turn == 2 && FindUnitByNameIdx(UNIT_DIEGO) && FindUnitByNameIdx(UNIT_ELENI)) {
            obj->state = 12;
            return;
         }
         if (gState.turn == 3 && FindUnitByNameIdx(UNIT_ASH)) {
            obj->state = 28;
            return;
         }
      }
      if (gState.mapNum == 14) {
         if (gState.turn == 2) {
            obj->state = 13;
            return;
         }
      }
      if (gState.mapNum == 18) {
         if (gState.turn == 2) {
            obj->state = 14;
            return;
         }
      }
      if (gState.mapNum == 21) {
         if (gState.turn == 2) {
            obj->state = 15;
            return;
         }
         if (gState.turn == 4) {
            obj->state = 16;
            return;
         }
      }
      if (gState.mapNum == 23) {
         if (gState.turn == 3) {
            obj->state = 17;
            return;
         }
      }
      if (gState.mapNum == 26) {
         if (gState.turn == 2 && gState.mapState.s.field_0x13 == 0) {
            obj->state = 18;
            return;
         }
         if (gState.turn == 3 && gState.mapState.s.field_0x13 == 0) {
            obj->state = 19;
            return;
         }
         if (gState.turn == 4 && gState.mapState.s.field_0x13 == 0) {
            obj->state = 20;
            return;
         }
      }
      if (gState.mapNum == 42) {
         if (gState.turn == 2 && FindUnitByNameIdx(UNIT_DOLAN) && FindUnitByNameIdx(UNIT_AMON) &&
             FindUnitByNameIdx(UNIT_SARA)) {
            obj->state = 23;
            return;
         }
         if (gState.turn == 3 && FindUnitByNameIdx(UNIT_CLINT) && FindUnitByNameIdx(UNIT_KIRA)) {
            obj->state = 24;
            return;
         }
      }
      if (gState.mapNum == 41) {
         if (gState.turn == 2) {
            obj->state = 25;
            return;
         }
         if (gState.turn == 3) {
            obj->state = 26;
            return;
         }
         if (gState.turn == 4) {
            obj->state = 27;
            return;
         }
      }
      if (gState.mapNum == 28) {
         if (gState.turn == 2) {
            obj->state = 29;
            return;
         }
      }
      gPlayerControlSuppressed = 0;
      obj->state = 99;
      return;

   case 10:

      switch (obj->state2) {
      case 0:
         SetupBattleMsgBox(UNIT_DIEGO, PORTRAIT_DIEGO, 13);
         obj->state2++;
         break;
      case 1:
         if (gState.msgFinished) {
            SetupBattleMsgBox(UNIT_ASH, PORTRAIT_ASH, 14);
            obj->state2++;
         }
         break;
      case 2:
         if (gState.msgFinished) {
            SetupBattleMsgBox(UNIT_CLINT, PORTRAIT_CLINT, 15);
            obj->state2++;
         }
         break;
      case 3:
         if (gState.msgFinished) {
            SetupBattleMsgBox(UNIT_DIEGO, PORTRAIT_DIEGO, 16);
            obj->state2++;
         }
         break;
      case 4:
         if (gState.msgFinished) {
            obj->state = 99;
         }
         break;
      } // switch (obj->state2) (via state:10)

      return;

   case 11:

      switch (obj->state2) {
      case 0:
         SetupBattleMsgBox(UNIT_ASH, PORTRAIT_ASH, 17);
         obj->state2++;
         break;
      case 1:
         if (gState.msgFinished) {
            obj->state = 99;
         }
         break;
      } // switch (obj->state2) (via state:11)

      return;

   case 12:

      switch (obj->state2) {
      case 0:
         SetupBattleMsgBox(UNIT_DIEGO, PORTRAIT_DIEGO, 13);
         obj->state2++;
         break;
      case 1:
         if (gState.msgFinished) {
            SetupBattleMsgBox(UNIT_ELENI, PORTRAIT_ELENI_25, 14);
            obj->state2++;
         }
         break;
      case 2:
         if (gState.msgFinished) {
            SetupBattleMsgBox(UNIT_DIEGO, PORTRAIT_DIEGO, 15);
            obj->state2++;
         }
         break;
      case 3:
         if (gState.msgFinished) {
            SetupBattleMsgBox(UNIT_ELENI, PORTRAIT_ELENI_25, 16);
            obj->state2++;
         }
         break;
      case 4:
         if (gState.msgFinished) {
            obj->state = 99;
         }
         break;
      } // switch (obj->state2) (via state:12)

      return;

   case 13:

      switch (obj->state2) {
      case 0:
         SetupBattleMsgBox(UNIT_ASH, PORTRAIT_ASH, 13);
         obj->state2++;
         break;
      case 1:
         if (gState.msgFinished) {
            obj->state = 99;
         }
         break;
      }

      return;

   case 14:

      switch (obj->state2) {
      case 0:
         SetupBattleMsgBox(UNIT_ASH, PORTRAIT_ASH, 13);
         obj->state2++;
         break;
      case 1:
         if (gState.msgFinished) {
            obj->state = 99;
         }
         break;
      }

      return;

   case 15:

      switch (obj->state2) {
      case 0:
         SetupBattleMsgBox(UNIT_ASH, PORTRAIT_ASH, 13);
         obj->state2++;
         break;
      case 1:
         if (gState.msgFinished) {
            obj->state = 99;
         }
         break;
      }

      return;

   case 16:

      switch (obj->state2) {
      case 0:
         SetupBattleMsgBox(UNIT_ASH, PORTRAIT_ASH_UPSET, 14);
         obj->state2++;
         break;
      case 1:
         if (gState.msgFinished) {
            obj->state = 99;
         }
         break;
      }

      return;

   case 17:

      switch (obj->state2) {
      case 0:
         SetupBattleMsgBox(UNIT_ASH, PORTRAIT_ASH, 13);
         obj->state2++;
         if (!FindUnitByNameIdx(UNIT_ZOHAR)) {
            obj->state2++;
         }
         break;
      case 1:
         if (gState.msgFinished) {
            SetupBattleMsgBox(UNIT_ZOHAR, PORTRAIT_ZOHAR, 14);
            obj->state2++;
         }
         break;
      case 2:
         if (gState.msgFinished) {
            obj->state = 99;
         }
         break;
      }

      return;

   case 18:

      switch (obj->state2) {
      case 0:
         SetupBattleMsgBox(UNIT_ASH, PORTRAIT_ASH, 14);
         obj->state2++;
         break;
      case 1:
         if (gState.msgFinished) {
            obj->state = 99;
         }
         break;
      }

      return;

   case 19:

      switch (obj->state2) {
      case 0:
         SetupBattleMsgBox(UNIT_ASH, PORTRAIT_ASH, 15);
         obj->state2++;
         break;
      case 1:
         if (gState.msgFinished) {
            obj->state = 99;
         }
         break;
      }

      return;

   case 20:

      switch (obj->state2) {
      case 0:
         SetupBattleMsgBox(UNIT_ASH, PORTRAIT_ASH, 16);
         obj->state2++;
         break;
      case 1:
         if (gState.msgFinished) {
            obj->state = 99;
         }
         break;
      }

      return;

   case 23:

      switch (obj->state2) {
      case 0:
         SetupBattleMsgBox(UNIT_SARA, PORTRAIT_SARA_HAPPY, 13);
         obj->state2++;
         break;
      case 1:
         if (gState.msgFinished) {
            SetupBattleMsgBox(UNIT_AMON, PORTRAIT_AMON_ANGRY, 14);
            obj->state2++;
         }
         break;
      case 2:
         if (gState.msgFinished) {
            SetupBattleMsgBox(UNIT_DOLAN, PORTRAIT_DOLAN, 15);
            obj->state2++;
         }
         break;
      case 3:
         if (gState.msgFinished) {
            SetupBattleMsgBox(UNIT_SARA, PORTRAIT_SARA, 16);
            obj->state2++;
         }
         break;
      case 4:
         if (gState.msgFinished) {
            SetupBattleMsgBox(UNIT_AMON, PORTRAIT_AMON_HAPPY, 17);
            obj->state2++;
         }
         break;
      case 5:
         if (gState.msgFinished) {
            SetupBattleMsgBox(UNIT_SARA, PORTRAIT_SARA_HAPPY, 18);
            obj->state2++;
         }
         break;
      case 6:
         if (gState.msgFinished) {
            obj->state = 99;
         }
         break;
      } // switch (obj->state2) (via state:23)

      return;

   case 24:

      switch (obj->state2) {
      case 0:
         SetupBattleMsgBox(UNIT_CLINT, PORTRAIT_CLINT, 19);
         obj->state2++;
         break;
      case 1:
         if (gState.msgFinished) {
            SetupBattleMsgBox(UNIT_KIRA, PORTRAIT_KIRA, 20);
            obj->state2++;
         }
         break;
      case 2:
         if (gState.msgFinished) {
            SetupBattleMsgBox(UNIT_CLINT, PORTRAIT_CLINT, 21);
            obj->state2++;
         }
         break;
      case 3:
         if (gState.msgFinished) {
            SetupBattleMsgBox(UNIT_KIRA, PORTRAIT_KIRA, 22);
            obj->state2++;
         }
         break;
      case 4:
         if (gState.msgFinished) {
            obj->state = 99;
         }
         break;
      } // switch (obj->state2) (via state:24)

      return;

   case 25:

      switch (obj->state2) {
      case 0:
         SetupBattleMsgBoxWithoutTail(UNIT_ASH, PORTRAIT_APPARITION_675, 13);
         obj->state2++;
         break;
      case 1:
         if (gState.msgFinished) {
            SetupBattleMsgBox(UNIT_ASH, PORTRAIT_ASH, 14);
            obj->state2++;
         }
         break;
      case 2:
         if (gState.msgFinished) {
            obj->state = 99;
         }
         break;
      }

      return;

   case 26:

      switch (obj->state2) {
      case 0:
         SetupBattleMsgBoxWithoutTail(UNIT_ASH, PORTRAIT_KANE_APPARITION, 15);
         obj->state2++;
         break;
      case 1:
         if (gState.msgFinished) {
            SetupBattleMsgBox(UNIT_ASH, PORTRAIT_ASH_UPSET, 16);
            obj->state2++;
         }
         break;
      case 2:
         if (gState.msgFinished) {
            obj->state = 99;
         }
         break;
      }

      return;

   case 27:

      switch (obj->state2) {
      case 0:
         SetupBattleMsgBoxWithoutTail(UNIT_ASH, PORTRAIT_APPARITION_681, 17);
         obj->state2++;
         break;
      case 1:
         if (gState.msgFinished) {
            SetupBattleMsgBox(UNIT_ASH, PORTRAIT_ASH_6, 18);
            obj->state2++;
         }
         break;
      case 2:
         if (gState.msgFinished) {
            obj->state = 99;
         }
         break;
      }

      return;

   case 28:

      switch (obj->state2) {
      case 0:
         SetupPartyBattleUnit(UNIT_KIRA, 9, 8, DIR_NORTH);
      // fallthrough
      case 1:
      case 2:
      case 3:
         obj->state2++;
         break;
      case 4:
         SetupBattleMsgBox(UNIT_KIRA, PORTRAIT_KIRA, 17);
         obj->state2++;
         break;
      case 5:
         if (gState.msgFinished) {
            SetupBattleMsgBox(UNIT_ASH, PORTRAIT_ASH, 18);
            obj->state2++;
         }
         break;
      case 6:
         if (gState.msgFinished) {
            SetupBattleMsgBox(UNIT_KIRA, PORTRAIT_KIRA, 19);
            obj->state2++;
         }
         break;
      case 7:
         if (gState.msgFinished) {
            obj->state = 99;
         }
         break;
      } // switch (obj->state2) (via state:28)

      return;

   case 29:

      switch (obj->state2) {
      case 0:
         PerformAudioCommand(AUDIO_CMD_PREPARE_XA(77));
         SetupBattleMsgBox(UNIT_DUMAS, PORTRAIT_DUMAS, 14);
         obj->state2++;
         break;

      case 1:
         SetupBattleUnit(18, 19, 13, 10, TEAM_ENEMY, DIR_NORTH, 1, 20, 1);
         obj->state2++;
         break;

      case 2:
         if (gState.msgFinished) {
            SetupBattleMsgBox(UNIT_KIRA_2, PORTRAIT_JUGGERNAUT, 15);
            obj->state2++;
         }
         break;

      case 3:
         if (gState.msgFinished) {
            SetupBattleMsgBox(UNIT_DUMAS, PORTRAIT_DUMAS_562, 16);
            obj->state2++;
         }
         break;

      case 4:
         if (gState.msgFinished) {
            SetupBattleMsgBoxWithoutTail(UNIT_DUMAS, PORTRAIT_JUGGERNAUT_INJURED, 22);
            obj->state2++;
         }
         break;

      case 5:
      case 6:
         obj->state2++;
         break;

      case 7:
         gTileStateGridPtr[19][13].action = TA_22;
         obj->state2++;
         break;

      case 8:
      case 9:
      case 10:
         obj->state2++;
         break;

      case 11:
         SetupPartyBattleUnit(UNIT_ASH, 19, 13, DIR_EAST);
         SetupPartyBattleUnit(UNIT_DIEGO, 20, 13, DIR_EAST);
         SetupPartyBattleUnit(UNIT_HUXLEY, 21, 14, DIR_EAST);
         SetupPartyBattleUnit(UNIT_GROG, 20, 14, DIR_EAST);
         SetupPartyBattleUnit(UNIT_DOLAN, 18, 13, DIR_EAST);
         SetupPartyBattleUnit(UNIT_SARA, 18, 14, DIR_EAST);
         SetupPartyBattleUnit(UNIT_ZOHAR, 19, 14, DIR_EAST);
         PerformAudioCommand(AUDIO_CMD_PLAY_XA(77));
         obj->state2++;
         break;

      case 12:
         if (gState.msgFinished) {
            PerformAudioCommand(AUDIO_CMD_FADE_OUT_128_2);
            OBJ.timer = 20;
            obj->state2++;
         }
         break;

      case 13:
         if (--OBJ.timer == 0) {
            PerformAudioCommand(AUDIO_CMD_STOP_ALL);
            LoadSeqSet(23);
            FinishLoadingSeq();
            OBJ.timer = 1;
            obj->state2++;
         }
         break;

      case 14:
         if (--OBJ.timer == 0) {
            OBJ.timer = 10;
            obj->state2++;
         }
         break;

      case 15:
         if (--OBJ.timer == 0) {
            Object *bloodSpurt = Obj_GetUnused();
            bloodSpurt->functionIndex = OBJF_BLOOD_SPURT;
            bloodSpurt->z1.s.hi = 19;
            bloodSpurt->x1.s.hi = 13;
            OBJ.timer = 30;
            obj->state2++;
         }
         break;

      case 16:
         if (--OBJ.timer == 0) {
            PerformAudioCommand(AUDIO_CMD_PLAY_SEQ(2));
            SetupBattleMsgBox(UNIT_ASH, PORTRAIT_ASH, 17);
            obj->state2++;
         }
         break;

      case 17:
         if (gState.msgFinished) {
            SetupBattleMsgBox(UNIT_CLINT, PORTRAIT_CLINT, 18);
            obj->state2++;
         }
         break;

      case 18:
         if (gState.msgFinished) {
            SetupBattleMsgBox(UNIT_ASH, PORTRAIT_ASH, 19);
            obj->state2++;
         }
         break;

      case 19:
         if (gState.msgFinished) {
            SetupBattleMsgBox(UNIT_DUMAS, PORTRAIT_DUMAS_562, 20);
            obj->state2++;
         }
         break;

      case 20:
         if (gState.msgFinished) {
            obj->state = 99;
         }
         break;

      } // switch (obj->state2) (via state:29)

      return;

   case 99:
      gState.field_0x98 = 0;
      gPlayerControlSuppressed = 0;
      obj->functionIndex = OBJF_NULL;
      return;

   } // switch (obj->state)
}

void SetupBattleMsgBox(u8 nameIdx, s16 portrait, s16 textIdx) {
   Object *msgbox;

   gState.msgFinished = 0;

   msgbox = Obj_GetUnused();
   msgbox->functionIndex = OBJF_BATTLE_MSGBOX;
   msgbox->d.objf586.nameIdx = nameIdx;
   msgbox->d.objf586.portrait = portrait;
   msgbox->d.objf586.textIdx = textIdx;
}

void SetupBattleMsgBoxWithoutTail(u8 nameIdx, s16 portrait, s16 textIdx) {
   Object *msgbox;

   gState.msgFinished = 0;

   msgbox = Obj_GetUnused();
   msgbox->functionIndex = OBJF_BATTLE_MSGBOX;
   msgbox->d.objf586.nameIdx = nameIdx;
   msgbox->d.objf586.portrait = portrait;
   msgbox->d.objf586.textIdx = textIdx;
   msgbox->d.objf586.omitTail = 1;
}

#undef OBJF
#define OBJF 586
void Objf586_BattleMsgBox(Object *obj) {
   Object *sprite, *camObj;

   switch (obj->state) {
   case 0:
      sprite = FindUnitSpriteByNameIdx(OBJ.nameIdx);
      camObj = Obj_GetUnused();
      camObj->functionIndex = OBJF_CAMERA_TBD_588;
      camObj->d.objf588.target = sprite;
      OBJ.timer = 20;
      obj->state++;
      break;
   case 1:
      if (--OBJ.timer == 0) {
         sprite = FindUnitSpriteByNameIdx(OBJ.nameIdx);
         MsgBox_ShowForSprite(sprite, 0, OBJ.omitTail);
         MsgBox_SetPortrait(OBJ.portrait, 0);
         MsgBox_SetText(1, OBJ.textIdx, 0x100);
         obj->state++;
      }
      break;
   case 2:
      if (gState.msgBoxFinished) {
         MsgBox_Close(0);
         gState.msgFinished = 1;
         obj->functionIndex = OBJF_NULL;
         return;
      }
      break;
   }
}

void DisplaySpellStatusWindow(UnitStatus *unit, u8 windowId) {
   s32 numSpells, i;

   gWindowChoiceHeight = 18;
   gWindowChoicesTopMargin = 9;

   numSpells = 0;
   while (unit->spells[numSpells] != SPELL_NULL) {
      numSpells++;
      if (numSpells == 10) {
         break;
      }
   }

   DrawWindow(windowId, 0, 50, 144, numSpells * 18 + 18, 70, (10 - numSpells) * 9 + 10, WBS_CROSSED,
              0);

   for (i = 0; i < numSpells; i++) {
      DrawText(12, (i * 8 + i) * 2 + 60, 20, 0, 0, gSpellNames[unit->spells[i]]);
   }

   if (unit->team == TEAM_PLAYER) {
      DisplayBasicWindow(windowId);
   } else {
      DisplayCustomWindow(windowId, 0, 1, 0, 0, 23);
   }
}

#ifdef PC_FEAT
extern int PC_LangItemNames1Byte(void);
/* A format-2 language pack stores item names as 1-byte ASCII; draw them in the small font, hard-
 * truncated to this narrow unit-item panel's cap (12). One shared implementation lives in
 * platform/pc/src/pc_lang_font.c; inline extern per the src/ house style. exchange/91, feedback-36. */
extern void PC_LangDrawItemName1Byte(s32 x, s32 y, s32 color, const u8 *name, s32 cap);
#endif

void DisplayItemsStatusWindow(UnitStatus *unit, u8 windowId) {
   Object *icon;
   s32 y, numItems = 0;

   gWindowChoicesTopMargin = 9;

   if (unit->items[0] != ITEM_NULL) {
      numItems++;
   }
   if (unit->items[1] != ITEM_NULL) {
      numItems++;
   }

   DrawWindow(windowId, 0, 50, 144, numItems * 18 + 18, 70, (10 - numItems) * 9 + 10, WBS_CROSSED,
              0);

   y = 0;
   if (unit->items[0] != ITEM_NULL) {
#ifdef PC_FEAT
      if (PC_LangItemNames1Byte())
         PC_LangDrawItemName1Byte(28, y * 18 + 60, 0, gItemNamesSjis[unit->items[0]], 12);
      else
#endif
      DrawSjisText(28, y * 18 + 60, 20, 0, 0, gItemNamesSjis[unit->items[0]]);
      y++;
   }
   if (unit->items[1] != ITEM_NULL) {
#ifdef PC_FEAT
      if (PC_LangItemNames1Byte())
         PC_LangDrawItemName1Byte(28, y * 18 + 60, 0, gItemNamesSjis[unit->items[1]], 12);
      else
#endif
      DrawSjisText(28, y * 18 + 60, 20, 0, 0, gItemNamesSjis[unit->items[1]]);
      y++;
   }

   if (unit->team == TEAM_PLAYER) {
      DisplayBasicWindow(windowId);
   } else {
      DisplayCustomWindow(windowId, 0, 1, 0, 0, 23);
   }

   icon = Obj_GetUnused();
   icon->functionIndex = OBJF_DISPLAY_ICON;
   icon->d.sprite.gfxIdx = GFX_ITEM_ICONS_OFS + unit->items[0];
   icon->x1.n = 79;
   icon->y1.n = (10 - numItems) * 9 + 20;
   y = (unit->items[0] != ITEM_NULL) ? 18 : 0;

   icon = Obj_GetUnused();
   icon->functionIndex = OBJF_DISPLAY_ICON;
   icon->d.sprite.gfxIdx = GFX_ITEM_ICONS_OFS + unit->items[1];
   icon->x1.n = 79;
   icon->y1.n = (10 - numItems) * 9 + 20 + y;
}

u8 s_unitX_801231e8;
u8 s_unitZ_801231ec;
u8 s_hiddenItem_801231f0;
u8 s_hiddenItemSentToDepot_801231f4;

#undef OBJF
#define OBJF 003
void Objf003_BattleActions(Object *obj) {
   // TODO: Eliminate gotos?
   // obj->state3: cursorState
   // obj->x3: unitX
   // obj->z3: unitZ
   UnitStatus *unit, *cursorUnit;
   Object *obj1, *obj2;
   s32 i; // iz, mapNum
   s32 ix;

   if (gState.battleEval != BATTLE_EVAL_NONE) {
      obj->functionIndex = OBJF_NULL;
      return;
   }

   unit = OBJ.unit;

   switch (obj->state) {
   case 0:
      if (obj->state2 == 0) {
         gState.droppedItem = ITEM_NULL;
         s_unitX_801231e8 = obj->x3.s.hi = obj->x1.s.hi;
         s_unitZ_801231ec = obj->z3.s.hi = obj->z1.s.hi;
         OBJ.unit = unit = &gUnits[OBJ_MAP_UNIT(obj).s.unitIdx];
         gState.lastSelectedUnit = unit->name + 1;
         unit->hideMarker = 1;
         OBJ.range = unit->travelRange;
         OBJ.remainingRange = OBJ.range;
         UpdateUnitInfoWindow(unit);
         DisplayBasicWindow(0x1f);
         PopulateBlueMovementGrid(obj->z1.s.hi, obj->x1.s.hi, unit->travelRange);
         gShowBlueMovementGrid = 0;
         obj2 = unit->sprite;
         StartUnitSpritesDecoder(obj2->d.sprite.stripIdx);
         obj->state = 16;
         obj->state2 = 0;
         goto BattleActions_State16;
      }

      break; // -> switch(cursorState)

   case 1:
      gState.mapCursorOutOfRange = 0;
      gWindowChoiceHeight = 17;
      gWindowChoicesTopMargin = 10;
      DrawWindow(0x35, 316, 0, 80, 90, 115, 80, WBS_CROSSED, 4);
      DrawText(316, 11, 20, 2, 0, PC_LANGSTR("Action\nDone\nExamine\nPush"));
      DisplayBasicWindow(0x35);
      gWindowActiveIdx = 0x35;
      gClearSavedPadState = 1;
      obj->state++;
      break;

   case 2:
      if (gWindowChoice.raw == 0x3501) {
         // Action
         CloseWindow(0x35);
         if (unit->spells[0] != SPELL_NULL) {
            obj->state = 10;
         } else {
            obj->state = 11;
         }
         obj->state2 = 0;
      } else if (gWindowChoice.raw == 0x3502) {
         // Done
         CloseWindow(0x35);
         obj->state = 8;
         obj->state2 = 0;
      } else if (gWindowChoice.raw == 0x3504) {
         // Push
         CloseWindow(0x35);
         obj->state = 15;
         obj->state2 = 0;
      } else if (gWindowChoice.raw == 0x3503) {
         // Examine
         CloseWindow(0x35);
         obj->state = 9;
         obj->state2 = 0;
      } else if (gWindowChoice.raw == 0x35ff) {
         // Cancel
         if (OBJ.range != 0) {
            CloseWindow(0x35);
            if (obj->x3.s.hi != obj->x1.s.hi || obj->z3.s.hi != obj->z1.s.hi) {
               OBJ_TARGET_TILE_STATE(obj).action = TA_9;
               obj->x3.s.hi = obj->x1.s.hi;
               obj->z3.s.hi = obj->z1.s.hi;
            }
            obj->state = 7;
            obj->state2 = 0;
         }
      }
      break;

   case 7:
      if (gBlueMovementGridPtr[gMapCursorZ][gMapCursorX] != PATH_STEP_UNSET) {
         gState.mapCursorOutOfRange = 0;
      } else {
         gState.mapCursorOutOfRange = 1;
      }

      switch (obj->state2) {
      case 0:
         DrawWindow(0x34, 316, 0, 56, 36, 10, 152, WBS_CROSSED, 0);
         DrawText(316, 11, 25, 0, 0, PC_LANGSTR("Move"));
         DisplayCustomWindow(0x34, 2, 1, 0, 0, 0);
         obj->state2++;

      // fallthrough
      case 1:
         gShowBlueMovementGrid = 1;
         gClearSavedPadState = 0;

         if (!gDecodingSprites && (gSavedPadStateNewPresses & PAD_CIRCLE) &&
             gBlueMovementGridPtr[gMapCursorZ][gMapCursorX] != PATH_STEP_INVALID &&
             gBlueMovementGridPtr[gMapCursorZ][gMapCursorX] != PATH_STEP_UNSET &&
             gMapUnitsPtr[gMapCursorZ][gMapCursorX].raw == 0) {
            func_8002B3A8(obj->z1.s.hi, obj->x1.s.hi, OBJ.range, 2);
            OBJ.remainingRange = OBJ.range - gPathGrid2_Ptr[gMapCursorZ][gMapCursorX];
            obj->x3.s.hi = gMapCursorX;
            obj->z3.s.hi = gMapCursorZ;
            PlotPathBackToUnit(gMapCursorZ, gMapCursorX);
            OBJ_TILE_STATE(obj).action = TA_6;
            gSignal3 = 0;
            gClearSavedPadState = 1;
            obj->state2++;
         } else if ((gSavedPadStateNewPresses & PAD_CIRCLE) &&
                    gBlueMovementGridPtr[gMapCursorZ][gMapCursorX] == PATH_STEP_INVALID) {
            OBJ.remainingRange = OBJ.range;
            CloseWindow(0x34);
            gShowBlueMovementGrid = 0;
            gState.mapCursorOutOfRange = 0;
            obj->state = 1;
         } else if (gSavedPadStateNewPresses & PAD_X) {
            OBJ.remainingRange = OBJ.range;
            CloseWindow(0x34);
            gShowBlueMovementGrid = 0;
            gState.mapCursorOutOfRange = 0;
            obj->state = 16;
            obj->state2 = 0;
         }
         break;

      case 2:
         if (gSignal3 != 0) {
            CloseWindow(0x34);
            gShowBlueMovementGrid = 0;
            gState.mapCursorOutOfRange = 0;
            obj->state = 1;
         }
         break;
      } // switch (obj->state2) (via state:7)

      break;

   case 8:

      switch (obj->state2) {
      case 0:
         OBJ_TARGET_TILE_STATE(obj).action = TA_CHOOSING_DIRECTION;
         obj2 = Obj_GetUnused();
         obj2->functionIndex = OBJF_CHOOSE_DONE_DIRECTION;
         obj2->d.objf016.unit = OBJ.unit;
         gSignal3 = 0;
         gSignal2 = 0;
         obj->state2++;
      // fallthrough
      case 1:
         if (gSignal2 == 1) {
            obj->state = 99;
            obj->state2 = 0;
         }
         if (gSignal2 == 2) {
            if (OBJ.range != 0) {
               obj->state = 16;
               obj->state2 = 0;
            } else {
               obj->state = 1;
            }
            if (obj->x3.s.hi != obj->x1.s.hi || obj->z3.s.hi != obj->z1.s.hi) {
               obj->state = 1;
            }
         }
         break;
      } // switch (obj->state2) (via state:8)

      break;

   case 9:
      // Examine

      switch (obj->state2) {
      case 0:
         CloseWindow(0x1e);
         OBJ.performedSubaction = 1;
         gState.searchX = obj->x3.s.hi;
         gState.searchZ = obj->z3.s.hi;
         s_hiddenItem_801231f0 = ITEM_NULL;
         i = gState.mapNum;
         if (!gState.mapState.s.foundHiddenItem1 && gState.searchZ == gMapHiddenItems[i][0].z &&
             gState.searchX == gMapHiddenItems[i][0].x) {
            s_hiddenItem_801231f0 = gMapHiddenItems[i][0].item;
            if (unit->items[0] == ITEM_NULL) {
               unit->items[0] = s_hiddenItem_801231f0;
               s_hiddenItemSentToDepot_801231f4 = 0;
            } else if (unit->items[1] == ITEM_NULL) {
               unit->items[1] = s_hiddenItem_801231f0;
               s_hiddenItemSentToDepot_801231f4 = 0;
            } else {
               gState.depot[s_hiddenItem_801231f0]++;
               s_hiddenItemSentToDepot_801231f4 = 1;
            }
            gState.mapState.s.foundHiddenItem1 = 1;
            obj->state2++;
         } else if (!gState.mapState.s.foundHiddenItem2 &&
                    gState.searchZ == gMapHiddenItems[i][1].z &&
                    gState.searchX == gMapHiddenItems[i][1].x) {
            s_hiddenItem_801231f0 = gMapHiddenItems[i][1].item;
            if (unit->items[0] == ITEM_NULL) {
               unit->items[0] = s_hiddenItem_801231f0;
               s_hiddenItemSentToDepot_801231f4 = 0;
            } else if (unit->items[1] == ITEM_NULL) {
               unit->items[1] = s_hiddenItem_801231f0;
               s_hiddenItemSentToDepot_801231f4 = 0;
            } else {
               gState.depot[s_hiddenItem_801231f0]++;
               s_hiddenItemSentToDepot_801231f4 = 1;
            }
            gState.mapState.s.foundHiddenItem2 = 1;
            obj->state2++;
         } else {
            obj->state2 += 4;
         }
         break;

      case 1:
         PerformAudioCommand(AUDIO_CMD_PLAY_XA(116));
         gSignal3 = 0;
         obj2 = unit->sprite;
         obj1 = Obj_GetUnused();
         obj1->x1.n = obj2->x1.n;
         obj1->z1.n = obj2->z1.n;
         obj1->y1.n = obj2->y1.n;
         obj1->functionIndex = OBJF_REVEAL_HIDDEN_ITEM;
         obj1->d.objf294.gfxIdx = GFX_ITEM_ICONS_OFS + s_hiddenItem_801231f0;
         OBJ.timer = 80;
         obj->state2++;
         break;

      case 2:
         if (--OBJ.timer == 0) {
            if (!s_hiddenItemSentToDepot_801231f4) {
               ShowReceivedItemDialog(s_hiddenItem_801231f0, 0x3c, 1);
            } else {
               ShowDepotReceivedItemDialog(s_hiddenItem_801231f0, 0x3c, 1);
            }
            OBJ.timer = 80;
            gSignal3 = 1;
            obj->state2++;
         }
         break;

      case 3:
         if (PressedCircleOrX() || --OBJ.timer == 0) {
            CloseWindow(0x3c);
            obj->state2++;
         }
         break;

      case 4:
         gState.needEval = 1;
         gState.signal = 1;
         OBJ.timer = 0;
         obj->state2++;
         break;

      case 5:
         if (++OBJ.timer > 100) {
            OBJ.timer = 100;
         }
         if (gState.signal == 0) {
            if (OBJ.timer < 20 && s_hiddenItem_801231f0 == ITEM_NULL) {
               gClearSavedPadState = 1;
               DrawWindow(0x3c, 0, 0, 232, 36, 44, 90, WBS_CROSSED, 0);
               DrawText(12, 11, 25, 2, 0, PC_LANGSTR("you didn't find anything!"));
               DisplayCustomWindow(0x3c, 0, 1, 0, 0, 0);
               OBJ.timer = 30;
               obj->state2++;
            } else {
               obj->state2 += 2;
            }
         }
         break;

      case 6:
         if (PressedCircleOrX() || --OBJ.timer == 0) {
            CloseWindow(0x3c);
            obj->state2++;
         }
         break;

      case 7:
         OBJ.range = OBJ.remainingRange;
         obj->x1.s.hi = obj->x3.s.hi;
         obj->z1.s.hi = obj->z3.s.hi;
         PopulateBlueMovementGrid(obj->z1.s.hi, obj->x1.s.hi, OBJ.range);
         obj->state2++;
         break;

      case 8:
         if (OBJ.range != 0) {
            obj->state = 16;
            obj->state2 = 0;
         } else {
            obj->state = 1;
         }
         break;
      } // switch (obj->state2) (via state:9)

      break;

   case 10:

      switch (obj->state2) {
      case 0:
         gWindowChoiceHeight = 17;
         gWindowChoicesTopMargin = 10;
         DrawWindow(0x36, 316, 0, 80, 72, 115, 80, WBS_CROSSED, 3);
         DrawText(316, 11, 20, 2, 0, PC_LANGSTR("Attack\nMagic\nItem"));
         DisplayBasicWindow(0x36);
         gWindowActiveIdx = 0x36;
         gClearSavedPadState = 1;
         obj->state2++;
         break;

      case 1:
         if (gWindowChoice.raw == 0x3601) {
            // Attack
            CloseWindow(0x36);
            gClearSavedPadState = 0;
            obj->state = 12;
            obj->state2 = 0;
         } else if (gWindowChoice.raw == 0x3602) {
            // Magic
            CloseWindow(0x1e);
            CloseWindow(0x36);
            obj->state = 13;
            obj->state2 = 0;
         } else if (gWindowChoice.raw == 0x3603) {
            // Item
            CloseWindow(0x1e);
            CloseWindow(0x36);
            obj->state = 17;
            obj->state2 = 0;
         } else if (gWindowChoice.raw == 0x36ff) {
            // Cancel
            //@3e44
            CloseWindow(0x36);
         LAB_80033808:
            if (obj->x3.s.hi == obj->x1.s.hi && obj->z3.s.hi == obj->z1.s.hi) {
               obj->state = 16;
               obj->state2 = 0;
               if (OBJ.range == 0) {
                  obj->state = 1;
               }
            } else {
               obj->state = 1;
            }
         }
         break;
      } // switch (obj->state2) (via state:10)

      break;

   case 11:

      switch (obj->state2) {
      case 0:
         gWindowChoiceHeight = 17;
         gWindowChoicesTopMargin = 10;
         DrawWindow(0x37, 316, 0, 80, 54, 150, 100, WBS_CROSSED, 2);
         DrawText(316, 11, 20, 2, 0, PC_LANGSTR("Attack\nItem"));
         DisplayBasicWindow(0x37);
         gWindowActiveIdx = 0x37;
         gClearSavedPadState = 1;
         obj->state2++;
         break;

      case 1:
         if (gWindowChoice.raw == 0x3701) {
            // Attack
            CloseWindow(0x37);
            gClearSavedPadState = 0;
            obj->state = 12;
            obj->state2 = 0;
         } else if (gWindowChoice.raw == 0x3702) {
            // Item
            CloseWindow(0x37);
            obj->state = 17;
            obj->state2 = 0;
         } else if (gWindowChoice.raw == 0x37ff) {
            // Cancel
            //@3fb0
            CloseWindow(0x37);
            goto LAB_80033808; //?: could match via tail merge?
         }
         break;
      } // switch (obj->state2) (via state:11)

      break;

   case 12:

      switch (obj->state2) {
      case 0:
         obj2 = Obj_GetUnused();
         obj2->functionIndex = OBJF_TARGETING_ATTACK;
         gSignal2 = 0;
         obj2->x1.n = obj->x3.n;
         obj2->z1.n = obj->z3.n;
         obj->state2++;

      // fallthrough
      case 1:
         if (gSignal2 == 1) {
            gState.mapCursorOutOfRange = 0;
            if (obj->x3.s.hi == obj->x1.s.hi && obj->z3.s.hi == obj->z1.s.hi) {
               obj->state = 16;
               obj->state2 = 0;
               if (OBJ.range == 0) {
                  obj->state = 1;
               }
            } else {
               obj->state = 1;
            }
         }
         if (gSignal2 == 2) {
            for (i = gMapMinZ; i <= gMapMaxZ; i++) {
               for (ix = gMapMinX; ix <= gMapMaxX; ix++) {
                  gBlueMovementGridPtr[i][ix] = PATH_STEP_UNSET;
               }
            }
            obj->state2++;
         }
         break;

      case 2:
         if (gSignal2 == 99) {
            obj->state = 99;
            obj->state2 = 0;
         }
         break;
      } // switch (obj->state2) (via state:12)

      break;

   case 13:
   case 17:

      switch (obj->state2) {
      case 0:
         SlideWindowTo(0x39, 8, 268);
         obj2 = Obj_GetUnused();
         if (obj->state == 13) {
            obj2->functionIndex = OBJF_BATTLE_SPELLS_LIST;
         } else {
            obj2->functionIndex = OBJF_BATTLE_ITEMS_LIST;
         }
         obj2->d.objf031.unit = OBJ.unit;
         gSignal2 = 0;
         obj->state2++;

      // fallthrough
      case 1:
         if (gSignal2 == 1) {
            gState.mapCursorOutOfRange = 0;
            SlideWindowTo(0x39, 8, 188);
            if (obj->x3.s.hi == obj->x1.s.hi && obj->z3.s.hi == obj->z1.s.hi) {
               obj->state = 16;
               obj->state2 = 0;
               if (OBJ.range == 0) {
                  obj->state = 1;
               }
            } else {
               obj->state = 1;
            }
         }
         if (gSignal2 == 2) {
            gState.mapCursorOutOfRange = 0;
            obj->state2++;
         }
         break;

      case 2:
         obj2 = Obj_GetLastUnused();
         obj2->functionIndex = OBJF_TARGETING_SPELL;
         gSignal2 = 0;
         obj2->x1.n = obj->x3.n;
         obj2->z1.n = obj->z3.n;
         obj->state2++;

      // fallthrough
      case 3:
         if (gSignal2 == 1) {
            obj->state2 = 1;
            if (obj->state == 17) {
               // Restore item on cancel
               unit->items[gState.activeItemSlot] = gState.activeItem;
            }
         }
         if (gSignal2 == 2) {
            gClearSavedPadState = 1;
            obj2 = unit->sprite;
            unit->direction = obj2->d.sprite.direction;
            for (i = gMapMinZ; i <= gMapMaxZ; i++) {
               for (ix = gMapMinX; ix <= gMapMaxX; ix++) {
                  gBlueMovementGridPtr[i][ix] = PATH_STEP_UNSET;
               }
            }
            obj->state2++;
         }
         break;

      case 4:
         if (gSignal2 == 99) {
            obj->state = 99;
            obj->state2 = 0;
         }
         break;
      } // switch (obj->state2) (via state:13/17)

      break;

   case 15:
      // Push

      switch (obj->state2) {
      case 0:
         OBJ_TARGET_TILE_STATE(obj).action = TA_CHOOSING_DIRECTION;
         obj->state2++;
         break;

      case 1:
         obj2 = Obj_GetUnused();
         obj2->functionIndex = OBJF_PUSH;
         obj2->d.objf048.unit = OBJ.unit;
         gSignal3 = 0;
         gSignal2 = 0;
         obj->state2++;

      // fallthrough
      case 2:
         if (gSignal2 == 99) {
            OBJ.performedSubaction = 1;
            OBJ.range = OBJ.remainingRange;
            PopulateBlueMovementGrid(obj->z3.s.hi, obj->x3.s.hi, OBJ.range);
            obj->x1.s.hi = obj->x3.s.hi;
            obj->z1.s.hi = obj->z3.s.hi;
            if (OBJ.range != 0) {
               obj->state = 16;
               obj->state2 = 0;
            } else {
               obj->state = 1;
            }
         } else if (gSignal2 == 2) {
            if (obj->x3.s.hi == obj->x1.s.hi && obj->z3.s.hi == obj->z1.s.hi) {
               if (OBJ.range != 0) {
                  obj->state = 16;
                  obj->state2 = 0;
               } else {
                  obj->state = 1;
               }
            } else {
               obj->state = 1;
            }
         }
         break;
      } // switch (obj->state2) (via state:15)

      break;

   case 16:
   BattleActions_State16:
      gState.mapCursorOutOfRange = 0;

      switch (obj->state2) {
      case 0:
         gWindowChoiceHeight = 0x11;
         gWindowChoicesTopMargin = 10;
         DrawWindow(0x35, 316, 0, 80, 108, 115, 60, WBS_CROSSED, 5);
         DrawText(316, 11, 20, 2, 0, PC_LANGSTR("Move\nAction\nDone\nExamine\nPush"));
         DisplayBasicWindow(0x35);
         gWindowActiveIdx = 0x35;
         gClearSavedPadState = 1;
         obj->state2++;
         break;

      case 1:
         if (gWindowChoice.raw == 0x3501) {
            // Move
            CloseWindow(0x1f);
            CloseWindow(0x35);
            obj->state = 7;
            obj->state2 = 0;
         }
         if (gWindowChoice.raw == 0x3502) {
            // Action
            CloseWindow(0x1f);
            CloseWindow(0x35);
            if (unit->spells[0] != SPELL_NULL) {
               obj->state = 10;
            } else {
               obj->state = 11;
            }
            obj->state2 = 0;
         } else if (gWindowChoice.raw == 0x3503) {
            // Done
            CloseWindow(0x1f);
            CloseWindow(0x35);
            obj->state = 8;
            obj->state2 = 0;
         } else if (gWindowChoice.raw == 0x3505) {
            // Push
            CloseWindow(0x1f);
            CloseWindow(0x35);
            obj->state = 15;
            obj->state2 = 0;
         } else if (gWindowChoice.raw == 0x3504) {
            // Examine
            CloseWindow(0x1f);
            CloseWindow(0x35);
            obj->state = 9;
            obj->state2 = 0;
         } else if (gWindowChoice.raw == 0x35ff) {
            // Cancel
            if (!OBJ.performedSubaction) {
               CloseWindow(0x1f);
               CloseWindow(0x35);
               obj->state = 100;
               obj->state2 = 0;
            }
         }
         break;
      } // switch (obj->state2) (via state:16)

      break;

   case 99:

      switch (obj->state2) {
      case 0:
         gClearSavedPadState = 1;
         gState.mapCursorOutOfRange = 0;
         obj2 = unit->sprite;
         if (OBJ_TERRAIN(obj2).s.terrain == TERRAIN_VILE_BOG) {
            unit->poisoned = 1;
         }
         if (gState.droppedItem != ITEM_NULL) {
            ShowReceivedItemDialog(gState.droppedItem, 0x3c, 0);
            gState.depot[gState.droppedItem]++;
            OBJ.timer = 50;
            obj->state2++;
         } else {
            obj->state2 += 2;
         }
         break;

      case 1:
         if (PressedCircleOrX() || --OBJ.timer == 0) {
            CloseWindow(0x3c);
            obj->state2++;
         }
         break;

      case 2:
         unit->done++;
         if (gState.mapNum == 26 && gState.mapState.s.field_0x13 == 0) {
            // Moved during ambush (Reed Highway)
            gState.mapState.s.field_0x13 = 1;
         }
         obj->state++;
         break;
      } // switch (obj->state2) (via state:99)

      break;

   case 100:
      gState.needEval = 1;
      obj->state++;
      break;

   case 101:
      if (gState.battleEval != BATTLE_EVAL_NONE) {
         return;
      }
      unit->hideMarker = 0;
      if (OBJ.performedSubaction) {
         unit->done++;
      }
      gClearSavedPadState = 0;
      gSignal1 = 0;
      ClearBlueMovementGrid();
      obj->functionIndex = OBJF_NULL;
      CloseWindow(0x1e);
      break;
   } // switch (obj->state)

   switch (obj->state3) {
   case 0:
      if (gMapUnitsPtr[gMapCursorZ][gMapCursorX].s.unitIdx == unit->idx) {
         break;
      }
      obj->state3++;

   // fallthrough
   case 1:
      if (gMapUnitsPtr[gMapCursorZ][gMapCursorX].s.unitIdx == 0) {
         break;
      }
      obj->state3++;

   // fallthrough
   case 2:
   BattleActions_CursorState2:
      OBJ.cursorUnitIdx = gMapUnitsPtr[gMapCursorZ][gMapCursorX].s.unitIdx;
      cursorUnit = &gUnits[gMapUnitsPtr[gMapCursorZ][gMapCursorX].s.unitIdx];
      UpdateCompactUnitInfoWindow(cursorUnit, cursorUnit, 1);
      CloseWindow(0x1e);
      if (cursorUnit->team == TEAM_PLAYER) {
         DisplayBasicWindow(0x1e);
      } else {
         DisplayCustomWindow(0x1e, 0, 1, 0, 0, 23);
      }
      obj->state3++;
      break;

   case 3:
      if (gMapUnitsPtr[gMapCursorZ][gMapCursorX].s.unitIdx == 0) {
         CloseWindow(0x1e);
         OBJ.cursorUnitIdx = 0;
         obj->state3 = 1;
         break;
      }
      if (gMapUnitsPtr[gMapCursorZ][gMapCursorX].s.unitIdx == OBJ.cursorUnitIdx) {
         break;
      }
      obj->state3--;
      goto BattleActions_CursorState2;
   } // switch (cursorState)
}

s16 s_newZoom_801231f8;
u8 s_menuMem_battleOptions_801231fc;

#undef OBJF
#define OBJF 425
void Objf425_BattleOptions(Object *obj) {
   // TODO: eliminate goto; earlier attempt to re-write in a more natural way threw off a bunch of
   // stuff; need to try other variations
#ifndef PC_FEAT
   static u8 turnBuffer[13] = "\x82\x73\x82\x74\x82\x71\x82\x6d\x81\x40\x81\x40";
#endif
   s32 i;
   UnitStatus *unit;
   s16 dx, dz;
   s16 unitX, unitZ;
   Object *obj1;
   s16 turn;

   if (gState.battleEval != BATTLE_EVAL_NONE) {
      obj->functionIndex = OBJF_NULL;
      return;
   }

   switch (obj->state) {
   case 0:
      s_menuMem_battleOptions_801231fc = 0;
      if (gSignal1 == 0) {
#ifdef PC_FEAT
         /* Stage 3 (1.1B): Square toggles the enemy threat overlay -- the union of every living
          * enemy's move+attack reach, drawn purple. Square was freed by the 1.1A control change
          * (ally-cycle moved to the shoulders). Rebuilt once on enable (positions are static during
          * the player turn). See exchange/62. */
         if ((gSavedPadStateNewPresses & PAD_SQUARE) && !gPlayerControlSuppressed &&
             !gClearSavedPadState) {
            gShowThreatGrid = !gShowThreatGrid;
         }
         /* Refresh the overlay when the board changed (unit/crate moved, a death). This is done HERE,
          * in the idle field-cursor state, NOT per-frame in UpdateInput: ComputeThreatGrid clobbers
          * the shared pathfinding scratch (grids 3/4 + gImpededSteps), so running it while the game
          * is mid-action corrupts that logic -- it broke crate pushes (bugreport-03). The idle phase
          * has no pathfinding in flight, so it's safe. Cost: only recomputes on an actual change. */
         if (gShowThreatGrid && !gPlayerControlSuppressed) {
            static u32 s_threatSig = 0;
            u32 sig = 0;
            s32 k, gz, gx;
            UnitStatus *u;
            for (k = 1; k < UNIT_CT; k++) {
               u = &gUnits[k];
               sig = sig * 131u + (u8)u->idx;
               if (u->idx != 0 && u->sprite != 0) {
                  sig = sig * 131u + (u8)u->team;
                  sig = sig * 131u + (u16)u->sprite->z1.s.hi;
                  sig = sig * 131u + (u16)u->sprite->x1.s.hi;
               }
            }
            for (gz = 0; gz < 30; gz++)
               for (gx = 0; gx < 65; gx++)
                  sig = sig * 131u + gCrateGrid[gz][gx];
            if (sig != s_threatSig) {
               ComputeThreatGrid();
               s_threatSig = sig;
            }
         }
         /* Stage 3 (1.1A): bidirectional ally-cycle on the shoulder buttons. The PC backend
          * routes the physical L1/R1 to pad 2 (gPad2State), leaving the camera (right stick
          * -> pad 1) untouched. R1 = next unit (the original Square behaviour), L1 = previous. */
         {
            s32 cycleDir = (gSavedPad2StateNewPresses & PAD_R1) ?  1 :
                           (gSavedPad2StateNewPresses & PAD_L1) ? -1 : 0;
            if (cycleDir != 0 && !gPlayerControlSuppressed && !gClearSavedPadState) {
               /* Step ONE unit in the press direction, THEN find the next selectable unit,
                * and store the SELECTED index (not one past it). The original forward-only
                * Square cycle advanced lastSelectedUnit one past the selection; that +1/-1
                * offset was invisible in a single direction but mis-stepped for ~2 presses
                * when REVERSING (bugreport-01). This do-while keeps it symmetric. */
               i = 0;
               do {
                  gState.lastSelectedUnit += cycleDir;
                  if (gState.lastSelectedUnit > 12) gState.lastSelectedUnit = 1;
                  if (gState.lastSelectedUnit < 1)  gState.lastSelectedUnit = 12;
                  unit = FindUnitByNameIdx(gState.lastSelectedUnit);
                  if (i++ > 12) goto LAB_800347e4;   /* no selectable unit */
               } while (unit == 0 || unit->done);
               dx = 0;
               dz = 0;
               unitX = unit->sprite->x1.s.hi;
               unitZ = unit->sprite->z1.s.hi;
               if (gMapCursorX < unitX) dx = 1;
               if (gMapCursorX > unitX) dx = -1;
               if (gMapCursorZ < unitZ) dz = 1;
               if (gMapCursorZ > unitZ) dz = -1;
               for (; gMapCursorX != unitX; gMapCursorX += dx) CenterCamera(1);
               for (; gMapCursorZ != unitZ; gMapCursorZ += dz) CenterCamera(1);
               obj1 = Obj_FindByFunction(OBJF_BATTLE_MAP_CURSOR_CONTROL);
               obj1->x1.s.hi = gMapCursorX;
               obj1->z1.s.hi = gMapCursorZ;
            }
         }
#else
         if ((gSavedPadStateNewPresses & PAD_SQUARE) && !gPlayerControlSuppressed &&
             !gClearSavedPadState) {
            // Pressing Square selects next available unit
            i = 0;
            if (gState.lastSelectedUnit > 12) {
               gState.lastSelectedUnit = 1;
            }

            while ((unit = FindUnitByNameIdx(gState.lastSelectedUnit)) == 0 || unit->done) {
               gState.lastSelectedUnit++;
               if (gState.lastSelectedUnit > 12) {
                  gState.lastSelectedUnit = 1;
               }
               if (i++ > 12) {
                  // FIXME
                  goto LAB_800347e4; // -> to next if
               }
            }

            dx = 0;
            dz = 0;
            gState.lastSelectedUnit++;
            unitX = unit->sprite->x1.s.hi;
            unitZ = unit->sprite->z1.s.hi;

            if (gMapCursorX < unitX) {
               dx = 1;
            }
            if (gMapCursorX > unitX) {
               dx = -1;
            }
            if (gMapCursorZ < unitZ) {
               dz = 1;
            }
            if (gMapCursorZ > unitZ) {
               dz = -1;
            }

            for (; gMapCursorX != unitX; gMapCursorX += dx) {
               CenterCamera(1);
            }
            for (; gMapCursorZ != unitZ; gMapCursorZ += dz) {
               CenterCamera(1);
            }

            obj1 = Obj_FindByFunction(OBJF_BATTLE_MAP_CURSOR_CONTROL);
            obj1->x1.s.hi = gMapCursorX;
            obj1->z1.s.hi = gMapCursorZ;
            // break; // -> to next if
         } // Square
#endif

      LAB_800347e4:
         if (gSignal1 == 0 && (gSavedPadStateNewPresses & PAD_CIRCLE) &&
             gMapUnitsPtr[gMapCursorZ][gMapCursorX].raw == 0 && !gPlayerControlSuppressed) {
            // Pressing Circle on an unoccupied tile opens battle options
            gClearSavedPadState = 1;
            gSignal1 = 1;
            obj->state++;
            gWindowChoiceHeight = 17;
            gWindowChoicesTopMargin = 10;
            CloseWindow(0x35);
            DrawWindow(0x35, 256, 120, 96, 36, 16, 16, WBS_CROSSED, 0);

            turn = gState.turn;
            if (turn > 99) {
               turn %= 100;
            }
#ifdef PC_FEAT
            {
               /* Language packs: retail embeds the number into a static "TURN  " buffer at a fixed
                * offset. Compose it in a local buffer so the number (full-width digits, script-
                * neutral) follows the translated prefix -- the same shape as ShowExpDialog. The
                * "TURN" prefix is a PC_LANGSTR literal (exported via the wrapped-literal scan, since
                * it is composed rather than passed to a draw call). With no pack the layout is
                * byte-for-byte the retail one. */
               u8 tb[32];
               const u8 *pfx = (const u8 *)PC_LANGSTR("\x82\x73\x82\x74\x82\x71\x82\x6d");
               s32 pl = 0;
               while (pfx[pl] != '\0' && pl < 24) {
                  tb[pl] = pfx[pl];
                  pl++;
               }
               // SJIS 8140: Space, 824F: Zero
               tb[pl] = 0x81;
               tb[pl + 1] = 0x40;
               tb[pl + 2] = 0x82;
               tb[pl + 3] = 0x4f + (turn % 10);
               if (turn > 9) {
                  i = (turn / 10);
                  tb[pl] = 0x82;
                  tb[pl + 1] = 0x4f + i;
               }
               tb[pl + 4] = 0;
               DrawSjisText(260, 131, 10, 2, 0, tb);
            }
#else
            // SJIS 8140: Space, 824F: Zero
            turnBuffer[8] = 0x81;
            turnBuffer[9] = 0x40;
            turnBuffer[10] = 0x82;
            turnBuffer[11] = 0x4f + (turn % 10);

            if (turn > 9) {
               i = (turn / 10);
               turnBuffer[8] = 0x82;
               turnBuffer[9] = 0x4f + i;
            }

            DrawSjisText(260, 131, 10, 2, 0, turnBuffer);
#endif
            DisplayBasicWindow(0x35);
         } // Circle
      }
      break;

   case 1:
      gWindowChoiceHeight = 17;
      gWindowChoicesTopMargin = 10;
      DrawWindow(0x34, 0, 0, 176, 144, 62, 50, WBS_CROSSED, 7);
      DrawText(12, 11, 17, 2, 0,
               PC_LANGSTR("BATTLE CONDITION\nTURN OVER\nCHANGE ZOOM\nSTATUS\nOPTIONS\nSAVE\nLOAD"));
      DisplayBasicWindowWithSetChoice(0x34, s_menuMem_battleOptions_801231fc);
      gWindowActiveIdx = 0x34;
      obj->state++;
      break;

   case 2:
      if (gWindowChoice.raw == 0x3402) {
         // Turn over
         s_menuMem_battleOptions_801231fc = 1;
         CloseWindow(0x34);
         CloseWindow(0x35);
         obj->state = 3;
         obj->state2 = 0;
      }
      if (gWindowChoice.raw == 0x3403) {
         // Change zoom
         s_menuMem_battleOptions_801231fc = 2;
         CloseWindow(0x34);
         obj->state = 5;
         obj->state2 = 0;
      }
      if (gWindowChoice.raw == 0x3404) {
         // Status
         s_menuMem_battleOptions_801231fc = 3;
         CloseWindow(0x34);
         obj->state = 0xb;
         obj->state2 = 0;
      }
      if (gWindowChoice.raw == 0x3401) {
         // Battle condition
         s_menuMem_battleOptions_801231fc = 0;
         CloseWindow(0x34);
         obj->state = 6;
         obj->state2 = 0;
      }
      if (gWindowChoice.raw == 0x3405) {
         // Options
         s_menuMem_battleOptions_801231fc = 4;
         CloseWindow(0x34);
         obj->state = 7;
         obj->state2 = 0;
      }
      if (gWindowChoice.raw == 0x34ff) {
         // Cancel
         CloseWindow(0x34);
         obj->state = 99;
         obj->state2 = 0;
      }
      if (gWindowChoice.raw == 0x3406) {
         // Save
         CloseWindow(0x34);
         CloseWindow(0x35);
         obj->state = 12;
         obj->state2 = 0;
      }
      if (gWindowChoice.raw == 0x3407) {
         // Load
         CloseWindow(0x34);
         CloseWindow(0x35);
         obj->state = 13;
         obj->state2 = 0;
      }
      break;

   case 3:

      switch (obj->state2) {
      case 0:
         gIsEnemyTurn = 1;
#ifdef PC_FEAT
         gShowThreatGrid = 0;   /* Stage 3 (1.1B): drop the threat overlay at turn end so it can't
                                 * linger with stale positions into/after the enemy turn. */
#endif
         obj->state2++;
         break;
      case 1:
         if (!gIsEnemyTurn) {
            gSignal1 = 0;
            obj->state = 99;
            obj->state2 = 0;
         }
         break;
      }

      break;

   case 5:

      switch (obj->state2) {
      case 0:
         DrawWindow(0x34, 256, 0, 88, 74, 105, 80, WBS_CROSSED, 3);
         DrawText(260, 11, 10, 2, 0, PC_LANGSTR("Close\nMedium\nDistant"));
         DisplayBasicWindowWithSetChoice(0x34, gState.zoom);
         gWindowActiveIdx = 0x34;
         obj->state2++;
         break;

      case 1:
         OBJ.timer = 15;
         if (gWindowChoice.raw == 0x3401) {
            CloseWindow(0x34);
            gState.zoom = 0;
            s_newZoom_801231f8 = 450;
            obj->state2++;
         } else if (gWindowChoice.raw == 0x3402) {
            CloseWindow(0x34);
            gState.zoom = 1;
            s_newZoom_801231f8 = 640;
            obj->state2++;
         } else if (gWindowChoice.raw == 0x3403) {
            CloseWindow(0x34);
            gState.zoom = 2;
            s_newZoom_801231f8 = 896;
            obj->state2++;
         } else if (gWindowChoice.raw == 0x34ff || gWindowChoice.raw == 0x3404) {
            CloseWindow(0x34);
            obj->state = 1;
            obj->state2 = 0;
         }
         break;

      case 2:
         if (--OBJ.timer != 0) {
            gCameraZoom.vz += (s_newZoom_801231f8 - gCameraZoom.vz) >> 2;
         } else {
            gCameraZoom.vz = s_newZoom_801231f8;
            obj->state = 99;
            obj->state2 = 0;
         }
         break;
      }

      break;

   case 6:
      // Battle conditions
      switch (obj->state2) {
      case 0:
         DrawWindow(0x34, 0, 0, 320, 72, 1, 80, WBS_DRAGON, 0);
         DrawText(20, 20, 34, 2, 0, gState.currentTextPointers[0]);
         DisplayBasicWindow(0x34);
         DisplayBasicWindow(0x35);
         gWindowActiveIdx = 0x34;
         obj->state2++;
         break;
      case 1:
         if ((PadRead(1) >> 0x10) & PAD_SELECT) {
            // 2P Select: Enable fast-motion cheat
            gState.vsyncNoWait = 1;
         }
         if (gPadStateNewPresses & PAD_X) {
            obj->state = 99;
            obj->state2 = 0;
         }
         break;
      }

      break;

   case 7:
      ShowConfigMenu(obj);
      break;

   case 8:
      ShowTextSpeedSettings(obj);
      break;

   case 9:
      ShowAudioSettings(obj);
      break;

   case 10:
      ShowCameraSettings(obj);
      break;

   case 11:

      switch (obj->state2) {
      case 0:
         CloseWindow(0x34);
         CloseWindow(0x35);
         gSignal2 = 0;
         obj1 = Obj_GetUnused();
         obj1->functionIndex = OBJF_STATUS_WINDOW_MGR;
         obj->state2++;
      // fallthrough
      case 1:
         if (gSignal2 == 100) {
            gClearSavedPadState = 0;
            obj->state = 99;
            obj->state2 = 0;
         }
         break;
      }

      break;

   case 12:

      switch (obj->state2) {
      case 0:
         obj1 = Obj_GetUnused();
         obj1->functionIndex = OBJF_FILE_SAVE_MENU_IBS;
         gState.D_8014053E = 0;
         obj->state2++;
         break;
      case 1:
         if (gState.D_8014053E != 0) {
            obj->state = 99;
            obj->state2 = 0;
         }
         break;
      }

      break;

   case 13:

      switch (obj->state2) {
      case 0:
         obj1 = Obj_GetUnused();
         obj1->functionIndex = OBJF_FILE_LOAD_MENU_IBS;
         gState.D_8014053E = 0;
         obj->state2++;
         break;
      case 1:
         if (gState.D_8014053E != 0) {
            obj->state = 99;
            obj->state2 = 0;
         }
         break;
      }

      break;

   case 99:
      CloseWindow(0x34);
      CloseWindow(0x35);
      gSignal1 = 0;
      gClearSavedPadState = 0;
      obj->state = 0;
      obj->state2 = 0;
      break;
   } // switch (obj->state)
}

void FieldInfo_HandleCursorUnit(Object *obj) {
   UnitStatus *cursorUnit;

   switch (obj->mem) {
   case 0:
      // Bug: cursorUnit is uninitialized here; presumably because this function's code was lifted
      // from Objf003_BattleActions
#ifdef PC_PORT
      /* PC_PORT (Stage 2.3): cursorUnit is uninitialised (the dev-acknowledged bug above), so
       * cursorUnit->idx is a read through a garbage low-address pointer. The 2.2 fault handler
       * zeroed it; mirror that deterministically (read-0), reducing the compare to "is the cursor
       * tile empty?" -- bit-identical to the validated build. Portable replacement for the x86-32
       * fault decoder. NULL site: FieldInfo_HandleCursorUnit+0x7d. See exchange/56. */
      if (gMapUnitsPtr[gMapCursorZ][gMapCursorX].s.unitIdx == 0) {
         break;
      }
#else
      if (gMapUnitsPtr[gMapCursorZ][gMapCursorX].s.unitIdx == cursorUnit->idx) {
         break;
      }
#endif
      obj->mem++;

   // fallthrough
   case 1:
      if (gMapUnitsPtr[gMapCursorZ][gMapCursorX].s.unitIdx == 0) {
         break;
      }
      obj->mem++;

   // fallthrough
   case 2:
   FieldInfo_CursorState2:
      obj->d.objf030.unitIdx = gMapUnitsPtr[gMapCursorZ][gMapCursorX].s.unitIdx;
      cursorUnit = &gUnits[gMapUnitsPtr[gMapCursorZ][gMapCursorX].s.unitIdx];
      UpdateCompactUnitInfoWindow(cursorUnit, cursorUnit, 1);
      CloseWindow(0x1e);

      if (cursorUnit->team == TEAM_PLAYER) {
         DisplayBasicWindow(0x1e);
      } else {
         DisplayCustomWindow(0x1e, 0, 1, 0, 0, 23);
      }
      obj->mem++;
      break;

   case 3:
      if (gMapUnitsPtr[gMapCursorZ][gMapCursorX].s.unitIdx == 0) {
         CloseWindow(0x1e);
         obj->d.objf030.unitIdx = 0;
         obj->mem = 1;
         break;
      }
      if (gMapUnitsPtr[gMapCursorZ][gMapCursorX].s.unitIdx == obj->d.objf030.unitIdx) {
         break;
      }
      obj->mem--;
      goto FieldInfo_CursorState2;
   }
}

u8 s_unitX_80123200;
u8 s_unitZ_80123204;

#undef OBJF
#define OBJF 030
#ifdef PC_FEAT
/* Language packs (platform/pc/src/pc_lang.c) need to reach terrainText below, but it is a
 * function-static: no external linkage, so it cannot be replaced by symbol like every other text
 * table. Hand it to the language layer once, the first time this object runs. */
extern void PC_LangApplyTerrainText(void *table, int bytes);
#endif
void Objf030_FieldInfo(Object *obj) {
   static s8 terrainText[10][12] = {"Plains   0%", "Prairie  5%", "Thicket 15%", "Barren   0%",
                                    "Water   20%", "Vile bog 0%", "Lava     0%", "Boundary30%",
                                    "Obstacle   ", "No entry   "};
   // obj->state3: terrainInfoState
   Object *obj1;
   UnitStatus *unit;

#ifdef PC_FEAT
   {
      static s32 langApplied = 0;
      if (!langApplied) {
         langApplied = 1;
         PC_LangApplyTerrainText(terrainText, sizeof(terrainText));
      }
   }
#endif

   if (gState.battleEval != BATTLE_EVAL_NONE) {
      obj->functionIndex = OBJF_NULL;
      return;
   }

   if (!gIsEnemyTurn) {
      switch (obj->state) {
      case 0:
         FieldInfo_HandleCursorUnit(obj);

         if ((gSavedPadStateNewPresses & PAD_CIRCLE) && gSignal1 == 0 &&
             gMapUnitsPtr[gMapCursorZ][gMapCursorX].s.unitIdx != 0) {
            CloseWindow(0x1e);

            // Debug mode allows controlling units regardless of team, done state, etc
            if (gState.debug == 0) {
               unit = &gUnits[gMapUnitsPtr[gMapCursorZ][gMapCursorX].s.unitIdx];
               if (gMapUnitsPtr[gMapCursorZ][gMapCursorX].s.team != TEAM_PLAYER ||
                   unit->name == 30 || unit->name == 31 || unit->unitType == UNIT_TYPE_MAGE_TOWER) {
                  s_unitX_80123200 = gMapCursorX;
                  s_unitZ_80123204 = gMapCursorZ;
                  gSignal1 = 1;
                  obj->state = 4;
                  obj->state2 = 0;
                  break;
               } else if (unit->done || unit->paralyzed) {
                  s_unitX_80123200 = gMapCursorX;
                  s_unitZ_80123204 = gMapCursorZ;
                  gSignal1 = 1;
                  obj->state = 4;
                  obj->state2 = 0;
                  break;
               }
            }

            gSignal1 = 1;
            obj1 = Obj_GetUnused();
            obj1->functionIndex = OBJF_BATTLE_ACTIONS;
            obj1->x1.s.hi = gMapCursorX;
            obj1->z1.s.hi = gMapCursorZ;
            gClearSavedPadState = 1;

         } else if ((gSavedPadStateNewPresses & PAD_X) && gSignal1 == 0 &&
                    gMapUnitsPtr[gMapCursorZ][gMapCursorX].s.unitIdx != 0) {
            CloseWindow(0x1e);
            obj->x1.s.hi = gMapCursorX;
            obj->z1.s.hi = gMapCursorZ;
            gClearSavedPadState = 1;
            gSignal1 = 1;
            obj->state += 2;
            obj->state2 = 0;
         } else if ((gSavedPadStateNewPresses & PAD_TRIANGLE) && gSignal1 == 0 &&
                    !gPlayerControlSuppressed) {
            obj1 = Obj_GetUnused();
            obj1->functionIndex = OBJF_OVERHEAD_MAP_VIEW;
            gClearSavedPadState = 1;
            gSignal1 = 2;
            obj->state += 3;
         }
         break;

      case 1:
         break;

      case 2:

         switch (obj->state2) {
         case 0:
            gSignal2 = 0;
            obj1 = Obj_GetUnused();
            obj1->functionIndex = OBJF_STATUS_WINDOW;
            obj1->d.objf595.unit = unit = &gUnits[OBJ_MAP_UNIT(obj).s.unitIdx];
            obj->state2++;

         // fallthrough
         case 1:
            if (gSignal2 == 99) {
               gClearSavedPadState = 0;
               gSignal1 = 0;
               obj->state = 0;
            }
            break;
         }

         break;

      case 3:
         if (gSignal1 == 99) {
            gClearSavedPadState = 0;
            gPlayerControlSuppressed = 0;
            gSignal1 = 0;
            obj->state = 0;
         }
         break;

      case 4:
         // Preview range for selected enemy or inactive unit
         switch (obj->state2) {
         case 0:
            gState.previewingRange = 1;
            unit = &gUnits[gMapUnitsPtr[s_unitZ_80123204][s_unitX_80123200].s.unitIdx];
            UpdateUnitInfoWindow(unit);
            if (unit->team != TEAM_ENEMY) {
               DisplayBasicWindow(0x1f);
            } else {
               DisplayCustomWindow(0x1f, 0, 1, 0, 0, 23);
            }
            PopulateBlueMovementGrid(s_unitZ_80123204, s_unitX_80123200, unit->travelRange);
            gShowBlueMovementGrid = 1;
            obj->state2++;
            break;

         case 1:
            if (gPadStateNewPresses & PAD_X) {
               gState.previewingRange = 0;
               ClearBlueMovementGrid();
               CloseWindow(0x1f);
               gSignal1 = 0;
               obj->state = 0;
               obj->state2 = 0;
            }
         }

         break;
      } // switch (obj->state)
   }

   switch (obj->state3) {
   case 0:
      DrawWindow(0x39, 396, 0, 112, 36, 8, 188, WBS_CROSSED, 0);
      DisplayCustomWindow(0x39, 2, 1, 0, 0, 0);
      OBJ.previousTerrain = -1;
      obj->state3++;
      break;

   case 1:
      if (OBJ.previousTerrain != gTerrainPtr[gMapCursorZ][gMapCursorX].s.terrain) {
         OBJ.previousTerrain = gTerrainPtr[gMapCursorZ][gMapCursorX].s.terrain;
         DrawText(396, 11, 25, 0, 0, terrainText[OBJ.previousTerrain]);
      }
      if (gPlayerControlSuppressed || gIsEnemyTurn || gState.inEvent || gClearSavedPadState) {
         CloseWindow(0x39);
         obj->state3++;
      }
      break;

   case 2:
      if (!gPlayerControlSuppressed && !gIsEnemyTurn && !gState.inEvent && !gClearSavedPadState) {
         gWindowChoicesCount = 0;
         DisplayCustomWindow(0x39, 2, 1, 0, 0, 0);
         OBJ.previousTerrain = -1;
         obj->state3--;
      }
      break;
   }
}

u16 s_camRotXPhase_80123208;
u16 s_camZoomPhase_8012320c;
s16 s_dstCamRotX_80123210;
s16 s_dstCamRotY_80123214;
s16 s_dstZoom_80123218;
s16 s_dstCamX_8012321c;
s16 s_dstCamY_80123220;
s16 s_dstCamZ_80123224;
s32 s_origMapSizeX_80123228;
s32 s_origMapSizeZ_8012322c;

#undef OBJF
#define OBJF 013
void Objf013_BattleMgr(Object *obj) {
   static s32 iUnit = 0;
   Object *unitSprite, *obj1;
   UnitStatus *unit;
   s32 i;
   s32 diff;
   OromeLakeEnemySpawn *pSpawn;

   if (gState.battleEval != BATTLE_EVAL_NONE) {
      obj->functionIndex = OBJF_NULL;
      return;
   }

   unitSprite = OBJ.unitSprite;
   unit = OBJ.unit;

   switch (obj->state) {
   case 0:
      if (gIsEnemyTurn) {
         s_origMapSizeX_80123228 = gMapSizeX;
         s_origMapSizeZ_8012322c = gMapSizeZ;
         if (gState.mapNum == 29) {
            if (gMapSizeX >= 16) {
               gMapSizeX = 16;
            }
            if (gMapSizeZ >= 16) {
               gMapSizeZ = 16;
            }
         } else if (gState.mapNum == 8) {
            gMapSizeX = 17;
            gMapSizeZ = 17;
         } else {
            if (gMapSizeX >= 18) {
               gMapSizeX = 18;
            }
            if (gMapSizeZ >= 18) {
               gMapSizeZ = 18;
            }
         }

         s_dstCamRotX_80123210 = gCameraRotation.vx;
         s_dstCamRotY_80123214 = gCameraRotation.vy & 0xfff;
         s_dstZoom_80123218 = gCameraZoom.vz;
         s_dstCamX_8012321c = gCameraPos.vx;
         s_dstCamY_80123220 = gCameraPos.vy;
         s_dstCamZ_80123224 = gCameraPos.vz;

         obj->state = 13;
         obj->state2 = 0;
         iUnit = 0;

         if (gState.mapNum == 8) {
            gPlayerControlSuppressed = 0;
            OBJ.timer = 1;
            OBJ.todo_x2d = 1;
            obj->state = 1;
            obj->state2 = 0;
         }
      }
      break;

   case 1:

      switch (obj->state2) {
      case 0:
         gShowBlueMovementGrid = 0;
         if (--OBJ.timer == 0) {
            obj->state2++;
         }
         break;
      case 1:
         gState.needEval = 1;
         obj->state2++;
         break;
      case 2:
         if (gState.battleEval == 0) {
            obj->state++;
         } else {
            obj->state2++;
         }
         break;
      }

      break;

   case 2:
      gState.droppedItem = ITEM_NULL;

   BattleMgr_TryNextUnit:
      do {
         iUnit++;
         if (iUnit == 40) {
            if (gState.mapNum != 8) {
               OBJ.todo_x2d = 0;
               gMapSizeX = s_origMapSizeX_80123228;
               gMapSizeZ = s_origMapSizeZ_8012322c;
               iUnit = 1;
               obj->state = 14;
               obj->state2 = 0;
               for (i = 1; i < 40; i++) {
                  unit = &gUnits[i];
                  unit->done = 0;
               }
               return;
            } else {
               iUnit = 0;
               gState.D_80140859 = 1;
            }
         }
         if (iUnit == 39) {
            gState.turn++;
         }

         unit = &gUnits[iUnit];

         // FIXME: More sensible way to match these jumps? Outer loop?
         if (gState.mapNum != 8) {
            if (unit->idx == 0) {
               goto BattleMgr_TryNextUnit;
            }
            if (unit->name == UNIT_LEENA && !unit->paralyzed) {
               break;
            }
            if (unit->idx == 0) {
               goto BattleMgr_TryNextUnit;
            }
            if (unit->name == UNIT_CLIVE && !unit->paralyzed) {
               break;
            }
            if (unit->idx != 0 && unit->team == TEAM_ENEMY && !unit->paralyzed) {
               if (unit->unitType == UNIT_TYPE_DEATH_DEV_) {
                  continue;
               } else {
                  break;
               }
            }
         } else {
            if (unit->idx != 0 && !unit->paralyzed) {
               break;
            }
         }
      } while (1);
      //} while (unit->idx == 0 || unit->paralyzed);

      obj1 = unit->sprite;
      OBJ.unit = unit;
      OBJ.unitSprite = obj1;
      unitSprite = obj1;
      OBJ.todo_x2d = 1;
      obj->state++;
      break;

   case 3:
      PopulateBlueMovementGrid(unitSprite->z1.s.hi, unitSprite->x1.s.hi, unit->travelRange);
      StartUnitSpritesDecoder(unitSprite->d.sprite.stripIdx);
      obj->state++;
      break;

   case 4:
      if (!gDecodingSprites) {
         UpdateCompactUnitInfoWindow(unit, unit, 0);
         if (unit->name == 30 || unit->name == 31) {
            DisplayCustomWindow(0x1d, 0, 1, 0, 0, 0);
         } else {
            DisplayCustomWindow(0x1d, 0, 1, 0, 0, 23);
         }
         OBJ.timer = 15;
         obj->state++;
      }
      break;

   case 5:
      gAiPlanReady = 0;
      obj1 = Obj_GetLastUnused();
      obj1->functionIndex = OBJF_AI_CHOOSE_ACTION;
      obj1->x1.s.hi = unitSprite->x1.s.hi;
      obj1->z1.s.hi = unitSprite->z1.s.hi;
      obj->state++;
      break;

   case 6:
      if (gAiPlanReady != 0) {
         if (unitSprite->x1.s.hi == gX_801233d8 && unitSprite->z1.s.hi == gZ_801233dc) {
            obj->state += 2;
            obj->state2 = 0;
         } else {
            obj->state++;
            obj->state2 = 0;
         }
      }
      break;

   case 7:

      switch (obj->state2) {
      case 0:
         PerformAudioCommand(AUDIO_CMD_PREPARE_XA(136));
         if (unit->unitType == UNIT_TYPE_DEATH_ANT) {
            OBJ_TILE_STATE(unitSprite).action = TA_29;
            OBJ_TILE_STATE(unitSprite).cachedByte = 16;
            gSignal3 = 0;
            obj->state2++;
         } else {
            obj->state2 += 2;
         }
         break;

      case 1:
         obj1 = Obj_GetUnused();
         obj1->functionIndex = OBJF_CLOUD;
         obj1->x1.n = unitSprite->x1.n;
         obj1->z1.n = unitSprite->z1.n;
         obj1->y1.n = unitSprite->y1.n;

         if (gSignal3 != 0) {
            PerformAudioCommand(AUDIO_CMD_PLAY_XA(136));
            unitSprite->d.sprite.hidden = 1;
            OBJ_TILE_STATE(unitSprite).cachedByte = 0;
            gState.mapState.s.field_0x0 = 2;
            gState.field_0x96 = 1;
            obj->state2++;
         }
         break;

      case 2:
         if (gState.field_0x96 == 0) {
            PerformAudioCommand(AUDIO_CMD_PREPARE_XA(137));
            gShowBlueMovementGrid = 1;
            PlotPathBackToUnit(gZ_801233dc, gX_801233d8);
            OBJ_TILE_STATE(unitSprite).action = TA_6;
            gSignal3 = 0;
            obj->state2++;
         }
         break;

      case 3:
         if (unit->unitType == UNIT_TYPE_ANT_ARM) {
            obj1 = Obj_GetUnused();
            obj1->functionIndex = OBJF_CLOUD;
            obj1->x1.n = unitSprite->x1.n;
            obj1->z1.n = unitSprite->z1.n;
            obj1->y1.n = unitSprite->y1.n;
         }
         if (gSignal3 == 1) {
            gShowBlueMovementGrid = 0;
            if (unit->unitType == UNIT_TYPE_DEATH_ANT) {
               OBJ_TILE_STATE(unitSprite).action = TA_29;
               OBJ_TILE_STATE(unitSprite).cachedByte = 22;
               OBJ.timer = 2;
               gSignal3 = 0;
               obj->state2++;
            } else {
               obj->state2 += 4;
            }
         }
         break;

      case 4:
         if (--OBJ.timer == 0) {
            obj->state2++;
            unitSprite->d.sprite.hidden = 0;
         }
         break;

      case 5:
         obj1 = Obj_GetUnused();
         obj1->functionIndex = OBJF_CLOUD;
         obj1->x1.n = unitSprite->x1.n;
         obj1->z1.n = unitSprite->z1.n;
         obj1->y1.n = unitSprite->y1.n;

         if (gSignal3 != 0) {
            OBJ_TILE_STATE(unitSprite).cachedByte = 0;
            PerformAudioCommand(AUDIO_CMD_PLAY_XA(137));
            gState.field_0x96 = 1;
            gState.mapState.s.field_0x0 = 1;
            gState.mapState.s.field_0x1 = gZ_801233dc;
            gState.mapState.s.field_0x2 = gX_801233d8;
            obj->state2++;
         }
         break;

      case 6:
         if (gState.field_0x96 == 0) {
            UpdateElevation();
            obj->state2++;
         }
         break;

      case 7:
         CloseWindow(0x1d);
         unit->direction = unitSprite->d.sprite.direction;
         obj->state++;
         obj->state2 = 0;
         break;

      } // switch (obj->state2) (via state:7)

      break;

   case 8:
      OBJ.todo_x2d = 0;
      ClearBlueMovementGrid();
      if (gAiActionType == 0) {
         if (gDir_80123470 == 0xffff) {
            obj->state += 1;
            obj->state2 = 0;
         } else {
            obj->state += 2;
            obj->state2 = 0;
         }
      } else {
         CloseWindow(0x1d);
         if (gAiActionType == 1) {
            obj->state += 3;
            obj->state2 = 0;
         } else {
            obj->state += 4;
            obj->state2 = 0;
         }
      }
      break;

   case 9:

      switch (obj->state2) {
      case 0:
         OBJ.timer = 5;
         obj->state2++;
      // fallthrough
      case 1:
         if (--OBJ.timer == 0) {
            CloseWindow(0x1d);
            obj->state = 99;
            obj->state2 = 0;
         }
         break;
      }

      break;

   case 10:

      switch (obj->state2) {
      case 0:
         unit->direction = gDir_80123470;
         OBJ_TILE_STATE(unitSprite).action = TA_11;
         OBJ.timer = 5;
         obj->state2++;
      // fallthrough
      case 1:
         if (--OBJ.timer == 0) {
            CloseWindow(0x1d);
            obj->state = 99;
            obj->state2 = 0;
         }
         break;
      }

      break;

   case 11:

      switch (obj->state2) {
      case 0:
         ClearGrid(1);
         if (unit->class == CLASS_ARCHER) {
            PopulateRangedAttackGrid(unitSprite->z1.s.hi, unitSprite->x1.s.hi, unit->attackRange,
                                     0);
         } else {
            PopulateMeleeAttackGrid(unitSprite->z1.s.hi, unitSprite->x1.s.hi, 0, unit->attackRange);
         }
         gYellowTargetGridPtr[gTargetZ_80123418][gTargetX_80123414] += 1;
         OBJ.timer = 30;
         obj->state2++;
         break;

      case 1:
         if (--OBJ.timer == 0) {
            ClearGrid(0);
            ClearGrid(1);
            obj1 = Obj_GetLastUnused();
            obj1->functionIndex = OBJF_UNIT_ATTACKING;
            obj1->x1.s.hi = gX_801233d8;
            obj1->z1.s.hi = gZ_801233dc;
            gTargetX = gTargetX_80123414;
            gTargetZ = gTargetZ_80123418;
            gSignal2 = 0;
            obj->state2++;
         }
         break;

      case 2:
         if (gSignal2 == 99) {
            OBJ.timer = 5;
            obj->state2++;
         }
         break;

      case 3:
         if (--OBJ.timer == 0) {
            obj->state = 99;
            obj->state2 = 0;
         }
         break;
      } // switch (obj->state2) (via state:11)

      break;

   case 12:

      switch (obj->state2) {
      case 0:
         ClearGrid(0);
         ClearGrid(1);
         if (gSpells[gCurrentSpell].area == SPELL_AREA_AOE) {
            PopulateCastingGrid(gZ_801233dc, gX_801233d8, gSpells[gCurrentSpell].range, 0);
            PopulateCastingGrid(gTargetZ_80123418, gTargetX_80123414,
                                gSpells[gCurrentSpell].fieldSize, 1);
         } else if (gSpells[gCurrentSpell].targeting >= SPELL_TARGET_ENEMY_GROUP) {
            PopulateCastingGrid(gZ_801233dc, gX_801233d8, gSpells[gCurrentSpell].range, 1);
         } else {
            PopulateCastingGrid(gZ_801233dc, gX_801233d8, gSpells[gCurrentSpell].range, 0);
            gPathGrid1_Ptr[gTargetZ_80123418][gTargetX_80123414] += 1;
         }

         OBJ.timer = 50;
         obj->state2++;
         break;

      case 1:
         if (--OBJ.timer == 0) {
            ClearGrid(0);
            ClearGrid(1);
            obj1 = Obj_GetLastUnused();
            obj1->functionIndex = OBJF_UNIT_CASTING;
            obj1->x1.s.hi = gX_801233d8;
            obj1->z1.s.hi = gZ_801233dc;
            gTargetX = gTargetX_80123414;
            gTargetZ = gTargetZ_80123418;
            gSignal2 = 0;
            obj->state2++;
         }
         break;

      case 2:
         if (gSignal2 == 99) {
            OBJ.timer = 5;
            obj->state2++;
         }
         break;

      case 3:
         if (--OBJ.timer == 0) {
            obj->state = 99;
            obj->state2 = 0;
         }
         break;
      } // switch (obj->state2) (via state:12)

      break;

   case 13:

      switch (obj->state2) {
      case 0:
         DrawWindow(0x38, 0, 0, 160, 64, 75, 90, WBS_DRAGON, 0);
         // ENEMY TURN
         DrawSjisText(
             20, 24, 20, 0, 0,
             PC_LANGSTR("\x82\x64\x82\x6d\x82\x64\x82\x6c\x82\x78\x81\x40\x82\x73\x82\x74\x82\x71\x82\x6d"));
         DisplayCustomWindow(0x38, 2, 1, 0, 0, 0);
         OBJ.timer = 45;
         obj->state2++;

      // fallthrough
      case 1:
         if (OBJ.timer == 25) {
            PerformAudioCommand(AUDIO_CMD_FADE_OUT_32_4);
         }
         if (--OBJ.timer == 0) {
            PerformAudioCommand(AUDIO_CMD_STOP_ALL);
            CloseWindow(0x38);
            OBJ.timer = 10;
            obj->state2++;
         }
         break;

      case 2:
         if (--OBJ.timer == 0) {
            PerformAudioCommand(AUDIO_CMD_PLAY_SEQ(1));
            OBJ.timer = 16;
            obj->state2++;
         }
         break;

      case 3:
         if (--OBJ.timer == 0) {
            gState.field_0x98 = 1;
            obj1 = Obj_GetUnused();
            obj1->functionIndex = OBJF_BATTLE_ENEMY_EVENT;
            obj->state2++;
         }
         break;

      case 4:
         if (gState.field_0x98 == 0) {
            obj1 = Obj_GetUnused();
            obj1->functionIndex = OBJF_BATTLE_TURN_START;
            obj1->d.objf592.team = TEAM_ENEMY;
            gSignal2 = 0;
            obj->state2++;
         }
         break;

      case 5:
         if (gSignal2 == 99) {
            gState.needEval = 1;
            obj->state2++;
         }
         break;

      case 6:
         OBJ.timer = 5;
         obj->state = 1;
         obj->state2 = 0;
         break;
      } // switch (obj->state2) (via state:13)

      break;

   case 14:

      switch (obj->state2) {
      case 0:
         OBJ.timer = 1;
         obj->state2++;

      // fallthrough
      case 1:
         if (--OBJ.timer == 0) {
            DrawWindow(0x38, 0, 0, 176, 64, 70, 90, WBS_DRAGON, 0);
            // PLAYER TURN
            DrawSjisText(
                20, 24, 20, 0, 0,
                PC_LANGSTR("\x82\x6f\x82\x6b\x82\x60\x82\x78\x82\x64\x82\x71\x81\x40\x82\x73\x82\x74\x82"
                "\x71\x82\x6d"));
            DisplayCustomWindow(0x38, 2, 1, 0, 0, 0);
            OBJ.timer = 45;
            obj->state2++;
         }
         break;

      case 2:
         if (OBJ.timer == 25) {
            PerformAudioCommand(AUDIO_CMD_FADE_OUT_32_4);
         }
         if (OBJ.timer == 10) {
            PerformAudioCommand(AUDIO_CMD_STOP_ALL);
         }
         if (--OBJ.timer == 0) {
            PlayBattleBGM(gState.mapNum);
            CloseWindow(0x38);
            OBJ.timer = 20;
            obj->state2++;
         }
         break;

      case 3:
         if (--OBJ.timer == 0) {
            gState.field_0x98 = 1;
            obj1 = Obj_GetUnused();
            obj1->functionIndex = OBJF_BATTLE_TURN_TICKER;
            obj->state2++;
         }
         break;

      case 4:
         if (gState.field_0x98 == 0) {
            gState.field_0x98 = 1;
            obj1 = Obj_GetUnused();
            obj1->functionIndex = OBJF_BATTLE_PLAYER_EVENT;
            obj->state2++;
         }
         break;

      case 5:
         if (gState.field_0x98 == 0) {
            obj1 = Obj_GetUnused();
            obj1->functionIndex = OBJF_BATTLE_TURN_START;
            obj1->d.objf592.team = TEAM_PLAYER;
            gSignal2 = 0;
            obj->state2++;
         }
         break;

      case 6:
         if (gSignal2 == 99) {
            OBJ.timer = 20;
            gCameraRotation.vy &= 0xfff;
            diff = s_dstCamRotY_80123214 - gCameraRotation.vy;
            if (diff > 0) {
               if (diff > DEG(180)) {
                  s_dstCamRotY_80123214 -= DEG(360);
               }
            } else if (diff < DEG(-180)) {
               s_dstCamRotY_80123214 += DEG(360);
            }
            obj->state2++;
         }
         break;

      case 7:
         if (--OBJ.timer != 0) {
            gCameraRotation.vx += (s_dstCamRotX_80123210 - gCameraRotation.vx) >> 2;
            gCameraRotation.vy += (s_dstCamRotY_80123214 - gCameraRotation.vy) >> 2;
            gCameraZoom.vz += (s_dstZoom_80123218 - gCameraZoom.vz) >> 2;
            gCameraPos.vx += (s_dstCamX_8012321c - gCameraPos.vx) >> 2;
            gCameraPos.vy += (s_dstCamY_80123220 - gCameraPos.vy) >> 2;
            gCameraPos.vz += (s_dstCamZ_80123224 - gCameraPos.vz) >> 2;
         } else {
            gCameraRotation.vx = s_dstCamRotX_80123210;
            gCameraRotation.vy = s_dstCamRotY_80123214;
            gCameraZoom.vz = s_dstZoom_80123218;
            gCameraPos.vx = s_dstCamX_8012321c;
            gCameraPos.vy = s_dstCamY_80123220;
            gCameraPos.vz = s_dstCamZ_80123224;
            obj->state2++;
         }
         return;

      case 8:
         gState.needEval = 1;
         obj->state2++;
         return;

      case 9:
         gIsEnemyTurn = 0;
         obj->state = 0;
         return;
      } // switch (obj->state2) (via state:14)

      break;

   case 99:

      switch (obj->state2) {
      case 0:
         obj1 = unit->sprite;
         if (OBJ_TERRAIN(obj1).s.terrain == TERRAIN_VILE_BOG) {
            unit->poisoned = 1;
         }
         if (gState.droppedItem != ITEM_NULL) {
            ShowReceivedItemDialog(gState.droppedItem, 0x3c, 0);
            gState.depot[gState.droppedItem]++;
            OBJ.timer = 50;
            obj->state2++;
         } else {
            obj->state2 += 2;
         }
         break;

      case 1:
         if (PressedCircleOrX() || --OBJ.timer == 0) {
            CloseWindow(0x3c);
            obj->state2++;
         }
         break;

      case 2:
         if (unit->name != UNIT_LEENA) {
            obj->state += 1;
         } else {
            obj->state += 2;
         }
         break;
      } // switch (obj->state2) (via state:99)

      break;

   case 100:
      ClearGrid(0);
      ClearGrid(1);
      OBJ.timer = 5;
      obj->state = 1;
      obj->state2 = 0;
      break;

   case 101:
      if (gState.mapState.s.field_0x0 != 2 &&
          (unitSprite->z1.s.hi == 11 && unitSprite->x1.s.hi == 16)) {
         SetupBattleMsgBox(UNIT_LEENA, PORTRAIT_LEENA, 17);
         obj->state++;
      } else if (gState.mapState.s.field_0x0 != 3 &&
                 (unitSprite->z1.s.hi == 2 && unitSprite->x1.s.hi == 10)) {
         SetupBattleMsgBox(UNIT_LEENA, PORTRAIT_LEENA, 18);
         obj->state++;
      } else {
         obj->state--;
      }
      break;

   case 102:
      gPlayerControlSuppressed = 1;
      obj->state++;
      break;

   case 103:
      if (gState.msgFinished) {
         if (unitSprite->z1.s.hi == 11) {
            gState.mapState.s.field_0x0 = 2;
         } else {
            gState.mapState.s.field_0x0 = 3;
         }
         gState.field_0x96 = 1;
         OBJ.timer = 60;
         obj->state++;
      }
      break;

   case 104:
      if (--OBJ.timer == 0) {
         if (unitSprite->z1.s.hi == 11) {
            pSpawn = &sOromeLakeEnemySpawns1[0];
         } else {
            pSpawn = &sOromeLakeEnemySpawns2[0];
         }
         while (pSpawn->z != 0) {
            SetupBattleUnit(pSpawn->stripIdx, pSpawn->z, pSpawn->x, pSpawn->level, TEAM_ENEMY,
                            DIR_WEST, 100, 12, 99);
            pSpawn++;
         }
         obj->state++;
      }
      break;

   case 105:
      if (gState.field_0x96 == 0) {
         obj->state -= 5;
      }
      break;

   } // switch (obj->state)

   if (!gPlayerControlSuppressed) {
      s16 a, b;
#ifdef PC_PORT
      /* PC_PORT (Stage 2.3): unitSprite can be NULL here when no unit occupies the
       * followed tile. On PSX / under the 2.2 fault handler these three reads yield 0;
       * mirror that exactly (read-0, NOT skip -- the camera still eases toward origin)
       * so the math stays bit-identical to the validated build. Portable replacement
       * for the x86-32 fault decoder. NULL sites #1-3; see exchange/52 Phase 2.3 Step A. */
      if (OBJ.todo_x2d) {
         s32 sx = unitSprite ? unitSprite->x1.n : 0;
         s32 sz = unitSprite ? unitSprite->z1.n : 0;
         s32 sy = unitSprite ? unitSprite->y1.n : 0;
         gCameraPos.vx += (-(sx >> 3) - gCameraPos.vx) >> 4;
         gCameraPos.vz += (-(sz >> 3) - gCameraPos.vz) >> 4;
         gCameraPos.vy += (((sy + CV(1.0)) >> 3) - gCameraPos.vy) >> 4;
      }
#else
      if (OBJ.todo_x2d) {
         gCameraPos.vx += (-(unitSprite->x1.n >> 3) - gCameraPos.vx) >> 4;
         gCameraPos.vz += (-(unitSprite->z1.n >> 3) - gCameraPos.vz) >> 4;
         gCameraPos.vy += (((unitSprite->y1.n + CV(1.0)) >> 3) - gCameraPos.vy) >> 4;
      }
#endif
      if (!gPlayerControlSuppressed && gIsEnemyTurn && gState.cameraMode == CAMERA_MODE_DYNAMIC) {
         gCameraRotation.vy -= 8;
         s_camRotXPhase_80123208 += 16;
         s_camZoomPhase_8012320c += 24;
         a = ((rcos(s_camRotXPhase_80123208 & 0xfff) * 256) >> 12) + 384;
         b = ((rcos(s_camZoomPhase_8012320c & 0xfff) * 150) >> 12) + 650;
         gCameraRotation.vx += (a - gCameraRotation.vx) >> 3;
         gCameraZoom.vz += (b - gCameraZoom.vz) >> 3;
      }
   }
}
