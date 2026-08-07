#include "common.h"
#include "object.h"
#include "window.h"
#include "state.h"
#include "card.h"
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


u8 s_menuMem_config_801232ac;

void ShowConfigMenu(Object *obj) {
   switch (obj->state2) {
   case 0:
      DrawWindow(0x34, 256, 0, 104, 74, 100, 80, WBS_CROSSED, 3);
      DrawText(256, 10, 20, 2, 0, PC_LANGSTR("Text Speed\nSound\nCamera\n"));
      DisplayBasicWindowWithSetChoice(0x34, s_menuMem_config_801232ac);
      gWindowActiveIdx = 0x34;
      obj->state2++;
      break;

   case 1:
      if (gWindowChoice.raw == 0x34ff) {
         s_menuMem_config_801232ac = GetWindowChoice(0x34) - 1;
         CloseWindow(0x34);
         obj->state = 1;
         obj->state2 = 0;
      }
      if (gWindowChoice.raw == 0x3401) {
         s_menuMem_config_801232ac = 0;
         CloseWindow(0x34);
         obj->state = 8;
         obj->state2 = 0;
      } else if (gWindowChoice.raw == 0x3402) {
         s_menuMem_config_801232ac = 1;
         CloseWindow(0x34);
         obj->state = 9;
         obj->state2 = 0;
      } else if (gWindowChoice.raw == 0x3403) {
         s_menuMem_config_801232ac = 2;
         CloseWindow(0x34);
         obj->state = 10;
         obj->state2 = 0;
      }
   }
}

void ShowTextSpeedSettings(Object *obj) {
   switch (obj->state2) {
   case 0:
      DrawWindow(0x34, 256, 0, 80, 74, 112, 80, WBS_CROSSED, 3);
      DrawText(260, 11, 10, 2, 0, PC_LANGSTR("Fast\nNormal\nSlow"));
      DisplayBasicWindowWithSetChoice(0x34, 2 - gState.textSpeed);
      gWindowActiveIdx = 0x34;
      obj->state2++;
      break;

   case 1:
      if (gWindowChoice.raw == 0x34ff) {
         CloseWindow(0x34);
         obj->state = 7;
         obj->state2 = 0;
      }
      if (gWindowChoice.raw == 0x3401) {
         CloseWindow(0x34);
         gState.textSpeed = 2;
         obj->state = 7;
         obj->state2 = 0;
      } else if (gWindowChoice.raw == 0x3402) {
         CloseWindow(0x34);
         gState.textSpeed = 1;
         obj->state = 7;
         obj->state2 = 0;
      } else if (gWindowChoice.raw == 0x3403) {
         CloseWindow(0x34);
         gState.textSpeed = 0;
         obj->state = 7;
         obj->state2 = 0;
      }
   }
}

void ShowAudioSettings(Object *obj) {
   switch (obj->state2) {
   case 0:
      DrawWindow(0x34, 256, 0, 80, 54, 115, 80, WBS_CROSSED, 2);
      DrawText(260, 11, 10, 2, 0, PC_LANGSTR("Stereo\nMono"));
      DisplayBasicWindowWithSetChoice(0x34, gState.mono);
      gWindowActiveIdx = 0x34;
      obj->state2++;
      break;

   case 1:
      if (gWindowChoice.raw == 0x34ff) {
         CloseWindow(0x34);
         obj->state = 7;
         obj->state2 = 0;
      }
      if (gWindowChoice.raw == 0x3401) {
         CloseWindow(0x34);
         gState.mono = 0;
         PerformAudioCommand(AUDIO_CMD_STEREO);
         obj->state = 7;
         obj->state2 = 0;
      } else if (gWindowChoice.raw == 0x3402) {
         PerformAudioCommand(AUDIO_CMD_MONO);
         CloseWindow(0x34);
         gState.mono = 1;
         obj->state = 7;
         obj->state2 = 0;
      }
   }
}

void ShowCameraSettings(Object *obj) {
   switch (obj->state2) {
   case 0:
      DrawWindow(0x34, 256, 0, 104, 54, 100, 80, WBS_CROSSED, 2);
      DrawText(260, 11, 10, 2, 0, PC_LANGSTR("Dynamic\nFixed"));
      DisplayBasicWindowWithSetChoice(0x34, gState.cameraMode);
      gWindowActiveIdx = 0x34;
      obj->state2++;
      break;

   case 1:
      if (gWindowChoice.raw == 0x34ff) {
         CloseWindow(0x34);
         obj->state = 7;
         obj->state2 = 0;
      }
      if (gWindowChoice.raw == 0x3401) {
         CloseWindow(0x34);
         gState.cameraMode = CAMERA_MODE_DYNAMIC;
         obj->state = 7;
         obj->state2 = 0;
      } else if (gWindowChoice.raw == 0x3402) {
         CloseWindow(0x34);
         gState.cameraMode = CAMERA_MODE_FIXED;
         obj->state = 7;
         obj->state2 = 0;
      }
   }
}

u8 s_newState_801232a0;
u8 s_delay_801232a4;
u8 s_menuMem_worldActions_801232a8;

s32 Objf598_WorldActions(Object *obj) {
   Object *statusWindowMgr;

   switch (obj->state) {
   case 0:
      s_menuMem_worldActions_801232a8 = 0;
      obj->state++;

   // fallthrough
   case 1:
      gWindowChoiceHeight = 17;
      gWindowChoicesTopMargin = 10;
      DrawWindow(0x34, 256, 0, 104, 126, 10, 16, WBS_CROSSED, 6);
      DrawText(260, 11, 10, 2, 0, PC_LANGSTR("Move\nItems\nStatus\nOptions\nSave\nLoad"));
      DisplayBasicWindowWithSetChoice(0x34, s_menuMem_worldActions_801232a8);
      gWindowActiveIdx = 0x34;
      obj->state++;
      break;

   case 2:
      if (gWindowChoice.raw == 0x3401) {
         s_menuMem_worldActions_801232a8 = 0;
         CloseWindow(0x34);
         obj->functionIndex = OBJF_NULL;
         gSignal1 = 0;
         break;
      }
      if (gWindowChoice.raw == 0x3404) {
         s_menuMem_worldActions_801232a8 = 3;
         obj->state = 7;
         obj->state2 = 0;
      }
      if (gWindowChoice.raw == 0x3402) {
         s_menuMem_worldActions_801232a8 = 1;
         gState.prevState = gState.primary;
         s_newState_801232a0 = STATE_DEPOT;
         obj->state = 100;
         obj->state2 = 0;
         break;
      }
      if (gWindowChoice.raw == 0x3403) {
         s_menuMem_worldActions_801232a8 = 3;
         obj->state = 6;
         obj->state2 = 0;
      }
      if (gWindowChoice.raw == 0x3405) {
         s_menuMem_worldActions_801232a8 = 4;
         if (gState.primary == STATE_6) {
            gState.saveLocation = SAVE_LOC_WORLD_MAP;
            gState.state6 = STATE_28;
         } else {
            gState.saveLocation = SAVE_LOC_TOWN;
            gState.state6 = STATE_20;
         }
         s_newState_801232a0 = STATE_FILE_SAVE_SCREEN;
         obj->state = 100;
         obj->state2 = 0;
      }
      if (gWindowChoice.raw == 0x3406) {
         s_menuMem_worldActions_801232a8 = 5;
         if (gState.primary == STATE_6) {
            gState.state6 = STATE_28;
         } else {
            gState.state6 = STATE_20;
         }
         s_newState_801232a0 = STATE_FILE_LOAD_SCREEN;
         obj->state = 100;
         obj->state2 = 0;
      }
      break;

   case 6:

      switch (obj->state2) {
      case 0:
         s_menuMem_worldActions_801232a8 = 2;
         CloseWindow(0x34);
         gSignal2 = 0;
         statusWindowMgr = Obj_GetUnused();
         statusWindowMgr->functionIndex = OBJF_STATUS_WINDOW_MGR;
         obj->state2++;
      // fallthrough
      case 1:
         if (gSignal2 == 100) {
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

   case 99:
      obj->state = 1;
      obj->state2 = 0;
      break;

   case 100:

      switch (obj->state2) {
      case 0:
         FadeOutScreen(2, 6);
         s_delay_801232a4 = 50;
         obj->state2++;
      // fallthrough
      case 1:
         if (--s_delay_801232a4 == 0) {
            gState.primary = s_newState_801232a0;
            gState.secondary = 0;
            gState.state3 = 0;
            gState.state4 = 0;
         }
         break;
      }

      break;
   }
}
