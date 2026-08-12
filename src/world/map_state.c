#include "common.h"
#include "state.h"
#include "cd_files.h"
#include "audio.h"
#include "object.h"

s32 State_WorldMap(void) {
   s32 i;

   switch (gState.secondary) {
   case 0:
      if (gState.primary == STATE_6) {
         PerformAudioCommand(AUDIO_CMD_STOP_SEQ);
         LoadSeqSet(0);
         FinishLoadingSeq();
         PerformAudioCommand(AUDIO_CMD_PLAY_SEQ(22));
      }
      gState.primary = STATE_6;
      Obj_ResetFromIdx10();

      for (i = 1; i < PARTY_CT; i++) {
         SyncPartyUnit(i);
      }

      gState.vsyncMode = 0;
      gClearSavedPadState = 1;
      gState.fieldRenderingDisabled = 1;

      switch (gState.worldMapState) {
      case 11:
         LoadFullscreenImage(CDF_MAP_1_1_TIM);
         break;
      case 12:
         LoadFullscreenImage(CDF_MAP_1_2_TIM);
         break;
      case 13:
         LoadFullscreenImage(CDF_MAP_1_3_TIM);
         break;
      case 14:
      case 17:
         LoadFullscreenImage(CDF_MAP_1_4_TIM);
         break;
      case 15:
         LoadFullscreenImage(CDF_MAP_1_5_TIM);
         break;
      case 16:
         LoadFullscreenImage(CDF_MAP_1_6_TIM);
         break;
      case 21:
         LoadFullscreenImage(CDF_MAP_2_1_TIM);
         break;
      case 22:
         LoadFullscreenImage(CDF_MAP_2_2_TIM);
         break;
      case 23:
      case 24:
      case 25:
         LoadFullscreenImage(CDF_MAP_2_3_TIM);
         break;
      case 31:
      case 32:
         LoadFullscreenImage(CDF_MAP_3_1_TIM);
         break;
      case 41:
         LoadFullscreenImage(CDF_MAP_4_1_TIM);
         break;
      case 42:
         LoadFullscreenImage(CDF_MAP_4_2_TIM);
         break;
      case 43:
         LoadFullscreenImage(CDF_MAP_4_3_TIM);
         break;
      case 51:
         LoadFullscreenImage(CDF_MAP_5_1_TIM);
         break;
      case 52:
         LoadFullscreenImage(CDF_MAP_5_2_TIM);
         break;
      case 53:
         LoadFullscreenImage(CDF_MAP_5_3_TIM);
         break;
      case 61:
         LoadFullscreenImage(CDF_MAP_6_1_TIM);
         break;
      case 62:
         LoadFullscreenImage(CDF_MAP_6_2_TIM);
         break;
      case 63:
         LoadFullscreenImage(CDF_MAP_6_3_TIM);
         break;
      case 64:
         LoadFullscreenImage(CDF_MAP_6_4_TIM);
         break;
      case 65:
      case 66:
         LoadFullscreenImage(CDF_MAP_6_5_TIM);
         break;
      case 67:
         LoadFullscreenImage(CDF_MAP_6_7_TIM);
         break;
      case 71:
         LoadFullscreenImage(CDF_MAP_7_1_TIM);
         break;
      case 72:
         LoadFullscreenImage(CDF_MAP_7_2_TIM);
         break;
      case 73:
      case 74:
         LoadFullscreenImage(CDF_MAP_7_3_TIM);
         break;
      }

      gTempObj = Obj_GetUnused();
      gTempObj->functionIndex = OBJF_FULLSCREEN_IMAGE;

      gTempObj = Obj_GetUnused();
      gTempObj->functionIndex = OBJF_WORLD_MAP;

      gState.secondary++;
      break;

   case 1:
      break;
   }
}
