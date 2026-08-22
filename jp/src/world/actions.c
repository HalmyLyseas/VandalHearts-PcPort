#include "common.h"
#include "object.h"
#include "window.h"
#include "state.h"
#include "card.h"
#include "audio.h"

u8 s_menuMem_config_801232ac;

void ShowConfigMenu(Object *obj) {
   switch (obj->state2) {
   case 0:
      DrawWindow(0x34, 256, 0, 104, 74, 100, 80, WBS_CROSSED, 3);
      DrawText(260, 11, 10, 2, 0, "\x95\xb6\x8e\x9a\x83\x58\x83\x73\x81\x5b\x83\x68\x0a\x83\x54\x83\x45\x83\x93\x83\x68\x0a\x83\x4a\x83\x81\x83\x89");
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
      DrawWindow(0x34, 256, 0, 48, 74, 120, 80, WBS_CROSSED, 3);
      DrawText(260, 11, 10, 2, 0, "\x91\xac\x82\xa2\x0a\x95\x81\x92\xca\x0a\x92\x78\x82\xa2");
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
      DrawText(260, 11, 10, 2, 0, "\x83\x58\x83\x65\x83\x8c\x83\x49\x0a\x83\x82\x83\x6d\x83\x89\x83\x8b");
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
      DrawText(260, 11, 10, 2, 0, "\x83\x5f\x83\x43\x83\x69\x83\x7e\x83\x62\x83\x4e\x0a\x83\x74\x83\x42\x83\x62\x83\x4e\x83\x58");
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
      DrawText(260, 11, 10, 2, 0, "\x88\xda\x93\xae\x0a\x8e\x9d\x95\xa8\x0a\x83\x58\x83\x65\x81\x5b\x83\x5e\x83\x58\x0a\x90\xdd\x92\xe8\x0a\x83\x5a\x81\x5b\x83\x75\x0a\x83\x8d\x81\x5b\x83\x68");
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
