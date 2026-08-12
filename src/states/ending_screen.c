#include "common.h"
#include "state.h"
#include "object.h"
#include "audio.h"
#include "cd_files.h"
#include "graphics.h"
#include "units.h"

s32 s_delay_80123280;

void State_EndingScreen(void) {
   switch (gState.secondary) {
   case 0:
      Obj_ResetFromIdx10();
      FadeInScreen(2, 6);
      gState.vsyncMode = 0;
      gClearSavedPadState = 1;
      gState.fieldRenderingDisabled = 1;
      if (gPartyMembers[UNIT_ASH].weapon == ITEM_V_HEART_2) {
         LoadFullscreenImage(CDF_US_S_END_TIM);
         gTempObj = Obj_GetUnused();
         gTempObj->functionIndex = OBJF_FULLSCREEN_IMAGE;
         s_delay_80123280 = 120;
         gState.secondary += 2;
      } else {
         LoadFullscreenImage(CDF_US_DIARY_TIM);
         gTempObj = Obj_GetUnused();
         gTempObj->functionIndex = OBJF_FULLSCREEN_IMAGE;
         gState.secondary++;
      }
      break;

   case 1:
      break;

   case 2:
      if (--s_delay_80123280 == 0) {
         gState.secondary++;
      }
      break;

   case 3:
      if (PressedCircleOrX_2()) {
         FadeOutScreen(2, 6);
         s_delay_80123280 = 120;
         gState.secondary++;
      }
      break;

   case 4:
      if (--s_delay_80123280 == 0) {
         Obj_ResetFromIdx10();
         LoadFullscreenImage(CDF_US_S_ED2_TIM);
         gTempObj = Obj_GetUnused();
         gTempObj->functionIndex = OBJF_FULLSCREEN_IMAGE;
         FadeInScreen(2, 4);
         s_delay_80123280 = 690;
         gState.secondary++;
      }
      break;

   case 5:
      if (--s_delay_80123280 == 0) {
         PerformAudioCommand(AUDIO_CMD_PLAY_XA(102));
         gState.secondary++;
      }
      break;

   case 6:
      break;
   }
}
