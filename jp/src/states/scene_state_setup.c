#include "common.h"
#include "state.h"
#include "graphics.h"
#include "units.h"
#include "cd_files.h"

extern s16 gSceneLoaders[96];
extern s8 gSceneMapNum[96];

void State_SetupScene(void) {
   s32 i;

   FetchOverlayCodeFromVram();
   Obj_ResetFromIdx10();

   if (gState.scene == 1) {
      SetupParty(0);
   }
   if (gState.scene == 10) {
      SetupParty(1);
   }
   if (gState.scene == 11) {
      SetupParty(2);
   }
   if (gState.scene == 16) {
      SetupParty(3);
   }
   if (gState.scene == 21) {
      SetupParty(4);
   }
   if (gState.scene == 37) {
      SetupParty(5);
   }
   if (gState.scene == 48) {
      SetupParty(6);
   }
   if (gState.scene == 31) {
      SetupParty(7);
   }

   gState.vsyncMode = 0;
   gClearSavedPadState = 1;
   gState.fieldRenderingDisabled = 1;
   gState.state7 = gSceneLoaders[gState.scene];
   gState.mapNum = gSceneMapNum[gState.scene];

   if (gState.primary == STATE_12) {
      gState.primary = STATE_4;
      for (i = 0; i < ARRAY_COUNT(gSpriteStripUnitIds); i++) {
         gSpriteStripUnitIds[i] = 0;
      }
      for (i = 0; i < ARRAY_COUNT(gCurrentUnitSet); i++) {
         gSpriteStripUnitIds[i] = gSceneSpriteStripUnitIds[gState.scene][i];
         gCurrentUnitSet[i] = gSceneUnitSets[gState.scene][i];
      }
   }

   if (gState.primary == STATE_26) {
      gState.primary = STATE_25;
      for (i = 0; i < ARRAY_COUNT(gSpriteStripUnitIds); i++) {
         gSpriteStripUnitIds[i] = 0;
      }
      for (i = 0; i < ARRAY_COUNT(gCurrentUnitSet); i++) {
         gSpriteStripUnitIds[i] = gSceneSpriteStripUnitIds[gState.scene][i];
         gCurrentUnitSet[i] = gSceneUnitSets[gState.scene][i];
      }
   }

   for (i = 0; i < ARRAY_COUNT(gCurrentUnitSet); i++) {
      if (gCurrentUnitSet[i] == 0) {
         gCurrentUnitSet[i] = gSpriteStripUnitIds[i];
      }
   }

   if (gState.primary == STATE_18) {
      gState.primary = STATE_16;
   }

   if (gState.primary == STATE_19) {
      gState.primary = STATE_17;
   }

   gState.secondary = 0;
   gState.state3 = 0;
   gState.state4 = 0;
}

void State_SetSceneState(void) {
   gState.secondary = 0;
   gState.state3 = 0;
   gState.state4 = 0;

   switch (gState.scene) {
   case 0:
      gState.primary = STATE_MOVIE;
      gState.movieIdxToPlay = MOV_EPI1_WS_STR;
      break;

   case 1:
      gState.primary = STATE_30;
      gState.mapNum = 10;
      gState.gold = 0;
      gState.chapter = 1;
      gState.section = 1;
      break;

   case 2:
      gState.worldMapDestination = 0;
      gState.primary = STATE_6;
      gState.worldMapState = 11;
      break;

   case 3:
      gState.primary = STATE_7;
      gState.townState = 2;
      break;

   case 4:
      gState.primary = STATE_30;
      gState.mapNum = 11;
      gState.chapter = 1;
      gState.section = 2;
      break;

   case 5:
      gState.primary = STATE_12;
      gState.scene = 6;
      break;

   case 6:
      gState.primary = STATE_7;
      gState.townState = 5;
      break;

   case 7:
      gState.primary = STATE_12;
      gState.scene = 20;
      break;

   case 8:
      gState.primary = STATE_30;
      gState.mapNum = 12;
      gState.chapter = 1;
      gState.section = 3;
      break;

   case 9:
      gState.primary = STATE_19;
      gState.scene = 10;
      break;

   case 10:
      gState.worldMapDestination = 2;
      gState.primary = STATE_6;
      gState.worldMapState = 13;
      break;

   case 11:
      gState.primary = STATE_30;
      gState.mapNum = 13;
      gState.chapter = 1;
      gState.section = 4;
      break;

   case 12:
      gState.primary = STATE_19;
      gState.scene = 13;
      break;

   case 13:
      gState.worldMapDestination = 3;
      gState.primary = STATE_6;
      gState.worldMapState = 14;
      break;

   case 14:
      gState.primary = STATE_20;
      gState.townState = 9;
      break;

   case 15:
      gState.primary = STATE_30;
      gState.mapNum = 14;
      gState.chapter = 1;
      gState.section = 5;
      break;

   case 16:
      gState.primary = STATE_20;
      gState.townState = 10;
      break;

   case 17:
      gState.primary = STATE_30;
      gState.mapNum = 15;
      gState.chapter = 1;
      gState.section = 6;
      break;

   case 18:
      gState.primary = STATE_CHAPTER_COMPLETE;
      gState.chapterOutro = 0;
      gState.chapter = 2;
      gState.section = 1;
      break;

   case 19:
      gState.primary = STATE_30;
      gState.mapNum = 16;
      gState.chapter = 2;
      gState.section = 1;
      break;

   case 20:
      gState.primary = STATE_7;
      gState.townState = 11;
      break;

   case 21:
      gState.primary = STATE_30;
      gState.mapNum = 17;
      gState.chapter = 2;
      gState.section = 2;
      break;

   case 22:
      gState.primary = STATE_19;
      gState.scene = 23;
      break;

   case 23:
      gState.primary = STATE_7;
      gState.townState = 12;
      break;

   case 24:
      gState.primary = STATE_30;
      gState.mapNum = 18;
      gState.chapter = 2;
      gState.section = 3;
      break;

   case 25:
      gState.primary = STATE_18;
      gState.scene = 26;
      break;

   case 26:
      gState.worldMapDestination = 9;
      gState.primary = STATE_6;
      gState.worldMapState = 23;
      break;

   case 27:
      gState.primary = STATE_30;
      gState.mapNum = 19;
      gState.chapter = 2;
      gState.section = 4;
      break;

   case 28:
      if (gState.eventChoice == 0) {
         gState.worldMapDestination = 9;
         gState.primary = STATE_6;
         gState.worldMapState = 24;
      } else {
         gState.primary = STATE_12;
         gState.scene = 29;
      }
      break;

   case 29:
      gState.primary = STATE_30;
      gState.mapNum = 20;
      gState.chapter = 2;
      gState.section = 5;
      break;

   case 30:
      gState.primary = STATE_12;
      gState.scene = 31;
      break;

   case 31:
      gState.primary = STATE_6;
      gState.worldMapState = 25;
      gState.worldMapDestination = 10;
      break;

   case 32:
      gState.primary = STATE_30;
      gState.mapNum = 21;
      gState.chapter = 2;
      gState.section = 6;
      break;

   case 33:
      gState.primary = STATE_12;
      gState.scene = 34;
      break;

   case 34:
      gState.primary = STATE_CHAPTER_COMPLETE;
      gState.chapterOutro = 1;
      gState.chapter = 3;
      gState.section = 1;
      break;

   case 35:
      gState.primary = STATE_30;
      gState.mapNum = 22;
      gState.chapter = 3;
      gState.section = 1;
      break;

   case 36:
      gState.worldMapDestination = 11;
      gState.primary = STATE_6;
      gState.worldMapState = 31;
      break;

   case 37:
      gState.primary = STATE_7;
      gState.townState = 16;
      break;

   case 38:
      gState.primary = STATE_30;
      gState.mapNum = 23;
      gState.chapter = 3;
      gState.section = 2;
      break;

   case 39:
      gState.primary = STATE_12;
      gState.scene = 40;
      break;

   case 40:
      gState.primary = STATE_30;
      gState.mapNum = 24;
      gState.chapter = 3;
      gState.section = 3;
      break;

   case 41:
      gState.primary = STATE_19;
      gState.scene = 42;
      break;

   case 42:
      gState.primary = STATE_7;
      gState.townState = 17;
      break;

   case 43:
      gState.primary = STATE_30;
      gState.mapNum = 25;
      gState.chapter = 3;
      gState.section = 4;
      break;

   case 44:
      gState.worldMapDestination = 14;
      gState.primary = STATE_6;
      gState.worldMapState = 42;
      break;

   case 46:
      gState.primary = STATE_30;
      gState.mapNum = 26;
      gState.chapter = 3;
      gState.section = 5;
      break;

   case 47:
      gState.primary = STATE_12;
      gState.scene = 48;
      break;

   case 48:
      gState.primary = STATE_30;
      gState.mapNum = 27;
      gState.chapter = 3;
      gState.section = 6;
      break;

   case 49:
      gState.primary = STATE_30;
      gState.mapNum = 28;
      gState.chapter = 3;
      gState.section = 7;
      break;

   case 50:
      gState.primary = STATE_CHAPTER_COMPLETE;
      gState.chapterOutro = 2;
      gState.chapter = 4;
      gState.section = 1;
      break;

   case 51:
      gState.primary = STATE_19;
      gState.scene = 52;
      break;

   case 52:
      gState.primary = STATE_7;
      gState.townState = 18;
      break;

   case 53:
      gState.primary = STATE_30;
      gState.mapNum = 29;
      gState.chapter = 4;
      gState.section = 2;
      break;

   case 54:
      gState.worldMapDestination = 17;
      gState.primary = STATE_6;
      gState.worldMapState = 51;
      break;

   case 55:
      gState.primary = STATE_30;
      gState.mapNum = 30;
      gState.chapter = 4;
      gState.section = 3;
      break;

   case 56:
      gState.worldMapDestination = 18;
      gState.primary = STATE_6;
      gState.worldMapState = 52;
      break;

   case 57:
      gState.primary = STATE_7;
      gState.townState = 22;
      break;

   case 60:
      gState.primary = STATE_30;
      gState.mapNum = 31;
      gState.chapter = 4;
      gState.section = 4;
      break;

   case 61:
      gState.primary = STATE_19;
      gState.scene = 62;
      break;

   case 62:
      gState.primary = STATE_7;
      gState.townState = 23;
      break;

   case 63:
      gState.primary = STATE_30;
      gState.mapNum = 32;
      gState.chapter = 4;
      gState.section = 5;
      break;

   case 64:
      gState.worldMapDestination = 20;
      gState.primary = STATE_6;
      gState.worldMapState = 53;
      break;

   case 65:
      gState.primary = STATE_30;
      gState.mapNum = 33;
      gState.chapter = 4;
      gState.section = 6;
      break;

   case 66:
      gState.primary = STATE_19;
      gState.scene = 67;
      BigIntAdd(gPartyMembers[UNIT_KIRA].experience, gExperienceLevels[20]);
      break;

   case 67:
      gState.primary = STATE_CHAPTER_COMPLETE;
      gState.chapterOutro = 3;
      gState.chapter = 5;
      gState.section = 1;
      break;

   case 68:
      gState.primary = STATE_12;
      gState.scene = 69;
      break;

   case 69:
      gState.primary = STATE_30;
      gState.mapNum = 34;
      gState.chapter = 5;
      gState.section = 1;
      break;

   case 70:
      gState.primary = STATE_7;
      gState.townState = 26;
      break;

   case 71:
      gState.primary = STATE_3;
      gState.mapNum = 35;
      gState.chapter = 5;
      gState.section = 2;
      break;

   case 72:
      gState.worldMapDestination = 24;
      gState.primary = STATE_6;
      gState.worldMapState = 63;
      break;

   case 73:
      gState.primary = STATE_7;
      gState.townState = 30;
      break;

   case 74:
      gState.primary = STATE_30;
      gState.mapNum = 36;
      gState.chapter = 5;
      gState.section = 3;
      break;

   case 75:
      gState.primary = STATE_19;
      gState.scene = 76;
      break;

   case 76:
      gState.primary = STATE_7;
      gState.townState = 31;
      break;

   case 77:
      gState.primary = STATE_30;
      gState.mapNum = 37;
      gState.chapter = 5;
      gState.section = 4;
      break;

   case 78:
      gState.primary = STATE_3;
      gState.mapNum = 38;
      gState.chapter = 5;
      gState.section = 5;
      break;

   case 79:
      gState.primary = STATE_12;
      gState.scene = 80;
      break;

   case 80:
      gState.primary = STATE_18;
      gState.scene = 81;
      break;

   case 81:
      gState.worldMapDestination = 25;
      gState.primary = STATE_6;
      gState.worldMapState = 67;
      break;

   case 82:
      gState.primary = STATE_30;
      gState.mapNum = 39;
      gState.chapter = 5;
      gState.section = 6;
      break;

   case 83:
      gState.primary = STATE_CHAPTER_COMPLETE;
      gState.chapterOutro = 4;
      gState.chapter = 6;
      gState.section = 1;
      gState.depot[gPartyMembers[UNIT_ASH].weapon] += 1;
      gPartyMembers[UNIT_ASH].weapon = ITEM_V_HEART;
      break;

   case 84:
      gState.primary = STATE_30;
      gState.mapNum = 40;
      gState.chapter = 6;
      gState.section = 2;
      break;

   case 85:
      gState.primary = STATE_30;
      gState.mapNum = 41;
      gState.chapter = 6;
      gState.section = 3;
      break;

   case 86:
      gState.primary = STATE_12;
      gState.scene = 87;
      break;

   case 87:
      gState.primary = STATE_18;
      gState.scene = 88;
      break;

   case 88:
      gState.primary = STATE_12;
      gState.scene = 89;
      break;

   case 89:
      gState.worldMapDestination = 29;
      gState.primary = STATE_6;
      gState.worldMapState = 73;
      break;

   case 90:
      gState.primary = STATE_30;
      gState.mapNum = 42;
      gState.chapter = 6;
      gState.section = 4;
      break;

   case 91:
      if (gState.eventChoice == 0) {
         gState.worldMapDestination = 32;
         gState.primary = STATE_6;
         gState.worldMapState = 74;
      } else {
         gState.primary = STATE_12;
         gState.scene = 92;
      }
      break;

   case 92:
      gState.primary = STATE_30;
      gState.mapNum = 43;
      gState.chapter = 6;
      gState.section = 5;
      break;

   case 93:
      gState.primary = STATE_MOVIE;
      gState.movieIdxToPlay = MOV_END1_WS_STR;
      break;

   case 94:
      gState.primary = STATE_ENDING_SCREEN;
      break;
   }
}
