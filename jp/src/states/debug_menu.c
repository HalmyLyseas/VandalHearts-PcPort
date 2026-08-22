#include "common.h"
#include "object.h"
#include "state.h"
#include "units.h"
#include "window.h"

void Objf414_DebugMenu(Object *obj) {
   s32 i;
   Object *o;

   switch (obj->state) {
   case 0:
      CloseWindow(0x35);
      gWindowChoiceHeight = 17;
      gWindowChoicesTopMargin = 10;
      gWindowActiveIdx = 0x34;
      DrawWindow(0x34, 0, 0, 200, 225, 100, 4, WBS_CROSSED, 9);
      // 戦闘マップ / フォントＯＦＦ / デバッグモード / (空行) / イベントマップ /
      // ゲームスタート / ユニット種類選択 / (空行) / セレクトＢ
      DrawSjisText(12, 11, 20, 2, 0,
                   "\x90\xed\x93\xac\x83\x7d\x83\x62\x83\x76\n\x83\x74\x83\x48\x83\x93\x83\x67\x82"
                   "\x6e\x82\x65\x82\x65\n\x83\x66\x83\x6f\x83\x62\x83\x4f\x83\x82\x81\x5b\x83\x68"
                   "\n\n\x83\x43\x83\x78\x83\x93\x83\x67\x83\x7d\x83\x62\x83\x76\n\x83\x51\x81\x5b"
                   "\x83\x80\x83\x58\x83\x5e\x81\x5b\x83\x67\n\x83\x86\x83\x6a\x83\x62\x83\x67\x8e"
                   "\xed\x97\xde\x91\x49\x91\xf0\n\n\x83\x5a\x83\x8c\x83\x4e\x83\x67\x82\x61");
      DisplayBasicWindow(0x34);
      obj->state++;
      obj->state2 = 0;

   // fallthrough
   case 1:
      if (gWindowChoice.raw == 0x3406) {
         obj->state = 15;
      }
      if (gWindowChoice.raw == 0x3401) {
         obj->state = 9;
         for (i = 1; i < PARTY_CT; i++) {
            gPartyMembers[i].advChosePathB = 0;
            gPartyMembers[i].advLevelFirst = 0;
            gPartyMembers[i].advLevelSecond = 0;
         }
      }
      if (gWindowChoice.raw == 0x3409) {
         obj->state = 16;
         obj->state2 = 0;
      }
      if (gWindowChoice.raw == 0x3402) {
         obj->state = 6;
      }
      if (gWindowChoice.raw == 0x3407) {
         obj->state = 10;
      }
      if (gWindowChoice.raw == 0x3403) {
         gState.gold = 10000;
         obj->state = 7;
      }
      if (gWindowChoice.raw == 0x3405) {
         obj->state = 13;
      }
      break;

   case 8:

      switch (obj->state2) {
      case 0:
         CloseWindow(0x34);
         gWindowChoicesTopMargin = 19;
         DrawWindow(0x34, 0, 0, 96, 90, 110, 116, WBS_DRAGON, 3);
         // スタート / ロード / 設定
         DrawSjisText(24, 20, 10, 2, 0,
                      "\x83\x58\x83\x5e\x81\x5b\x83\x67\n\x83\x8d\x81\x5b\x83\x68\n\x90\xdd\x92"
                      "\xe8");
         DisplayBasicWindow(0x34);
         obj->state2++;
         break;

      case 1:
         if (PressedCircleOrX()) {
            obj->state2++;
         }
         break;

      case 2:
         CloseWindow(0x34);
         gWindowChoicesTopMargin = 19;
         DrawWindow(0x34, 0, 0, 120, 72, 96, 130, WBS_DRAGON, 2);
         // 文字スピード / サウンド
         DrawSjisText(24, 20, 10, 2, 0,
                      "\x95\xb6\x8e\x9a\x83\x58\x83\x73\x81\x5b\x83\x68\n\x83\x54\x83\x45\x83\x93"
                      "\x83\x68");
         DisplayBasicWindow(0x34);
         break;
      }

      break;

   case 6:
      gState.field_0xa7 = 1;
      obj->state = 1;
      break;

   case 7:

      switch (obj->state2) {
      case 0:
         CloseWindow(0x34);
         DrawWindow(0x34, 0, 0, 64, 54, 250, 10, WBS_CROSSED, 2);
         // ＯＮ / ＯＦＦ
         DrawSjisText(12, 11, 10, 2, 0, "\x82\x6e\x82\x6d\n\x82\x6e\x82\x65\x82\x65");
         DisplayBasicWindow(0x34);
         obj->state2++;
         break;

      case 1:
         if (gWindowChoice.raw == 0x34ff) {
            obj->state = 0;
            obj->state2 = 99;
            CloseWindow(0x34);
            return;
         }
         if (gWindowChoice.raw == 0x3401) {
            gState.debug = 1;
            obj->state2 = 99;
         }
         if (gWindowChoice.raw == 0x3402) {
            gState.debug = 0;
            obj->state2 = 99;
         }
         break;

      case 99:
         CloseWindow(0x34);
         obj->state = 0;
         break;
      }

      break;

   case 9:

      switch (obj->state2) {
      case 0:
         gState.chapter = 0;
         gState.section = 0;
         CloseWindow(0x34);
         DrawWindow(0x34, 0, 0, 136, 190, 150, 10, WBS_CROSSED, 10);
         // Test, 1-8
         DrawSjisText(12, 11, 10, 2, 0,
                      "\x83\x65\x83\x58\x83\x67\x0a\x82\x50\x0a\x82\x51\x0a\x82\x52\x0a\x82\x53\x0a"
                      "\x82\x54\x0a\x82\x55\x0a\x82\x56\x0a\x82\x57");
         DisplayBasicWindow(0x34);
         obj->state2++;
         break;

      case 1:
         if (gPadStateNewPresses & PAD_RIGHT) {
            obj->state2++;
            return;
         }
         if (gWindowChoice.raw == 0x34ff) {
            obj->state = 0;
            obj->state2 = 99;
            CloseWindow(0x34);
            return;
         }
         if (gWindowChoice.s.windowId == 0x34 && gWindowChoice.s.choice != 0) {
            gState.useDefaultUnits = 1;
            gState.mapNum = gWindowChoice.s.choice + 8;
            obj->state2 = 99;
         }
         if (gWindowChoice.raw == 0x3403) {
            gState.worldMapState = 11;
         }
         break;

      case 2:
         CloseWindow(0x34);
         DrawWindow(0x34, 0, 0, 136, 190, 150, 10, WBS_CROSSED, 10);
         // 9-18
         DrawSjisText(12, 11, 10, 2, 0,
                      "\x82\x58\x0a\x82\x50\x82\x4f\x0a\x82\x50\x82\x50\x0a\x82\x50\x82\x51\x0a\x82"
                      "\x50\x82\x52\x0a\x82\x50\x82\x53\x0a\x82\x50\x82\x54\x0a\x82\x50\x82\x55\x0a"
                      "\x82\x50\x82\x56\x0a\x82\x50\x82\x57");
         DisplayBasicWindow(0x34);
         obj->state2++;
         break;

      case 3:
         if (gPadStateNewPresses & PAD_LEFT) {
            obj->state2 -= 3;
            return;
         }
         if (gPadStateNewPresses & PAD_RIGHT) {
            obj->state2++;
            return;
         }
         if (gWindowChoice.raw == 0x34ff) {
            obj->state = 0;
            obj->state2 = 99;
            CloseWindow(0x34);
            return;
         }
         if (gWindowChoice.s.windowId == 0x34 && gWindowChoice.s.choice != 0) {
            gState.useDefaultUnits = 1;
            gState.mapNum = gWindowChoice.s.choice + 17;
            obj->state2 = 99;
         }
         break;

      case 4:
         CloseWindow(0x34);
         DrawWindow(0x34, 0, 0, 136, 190, 150, 10, WBS_CROSSED, 10);
         // 19-28
         DrawSjisText(12, 11, 10, 2, 0,
                      "\x82\x50\x82\x58\x0a\x82\x51\x82\x4f\x0a\x82\x51\x82\x50\x0a\x82\x51\x82\x51"
                      "\x0a\x82\x51\x82\x52\x0a\x82\x51\x82\x53\x0a\x82\x51\x82\x54\x0a\x82\x51\x82"
                      "\x55\x0a\x82\x51\x82\x56\x0a\x82\x51\x82\x57");
         DisplayBasicWindow(0x34);
         obj->state2++;
         break;

      case 5:
         if (gPadStateNewPresses & PAD_LEFT) {
            obj->state2 -= 3;
            return;
         }
         if (gPadStateNewPresses & PAD_RIGHT) {
            obj->state2++;
            return;
         }
         if (gWindowChoice.raw == 0x34ff) {
            obj->state = 0;
            obj->state2 = 99;
            CloseWindow(0x34);
            return;
         }
         if (gWindowChoice.s.windowId == 0x34 && gWindowChoice.s.choice != 0) {
            gState.useDefaultUnits = 1;
            gState.mapNum = gWindowChoice.s.choice + 27;
            obj->state2 = 99;
         }
         break;

      case 6:
         CloseWindow(0x34);
         DrawWindow(0x34, 0, 0, 136, 190, 150, 10, WBS_CROSSED, 10);
         // 29-34, 1-4
         DrawSjisText(
             12, 11, 10, 2, 0,
             "\x82\x51\x82\x58\x0a\x82\x52\x82\x4f\x0a\x82\x52\x82\x50\x0a\x82\x52\x82\x51\x0a\x82"
             "\x52\x82\x52\x0a\x82\x52\x82\x53\x0a\x82\x50\x0a\x82\x51\x0a\x82\x52\x0a\x82\x53");
         DisplayBasicWindow(0x34);
         obj->state2++;
         break;

      case 7:
         if (gPadStateNewPresses & PAD_LEFT) {
            obj->state2 -= 3;
            return;
         }
         if (gPadStateNewPresses & PAD_RIGHT) {
            obj->state2++;
            return;
         }
         if (gWindowChoice.raw == 0x34ff) {
            obj->state = 0;
            obj->state2 = 99;
            CloseWindow(0x34);
            return;
         }
         if (gWindowChoice.s.windowId == 0x34 && gWindowChoice.s.choice != 0) {
            gState.useDefaultUnits = 1;
            gState.mapNum = gWindowChoice.s.choice + 37;
            obj->state2 = 99;
            if (gWindowChoice.s.choice >= 7) {
               // Since the choices for 1-4 start at the 7th position (after 29-34)
               gState.mapNum = gWindowChoice.s.choice - 7;
            }
         }
         break;

      case 8:
         CloseWindow(0x34);
         DrawWindow(0x34, 0, 0, 136, 190, 150, 10, WBS_CROSSED, 3);
         // 5, 6, Demo
         DrawSjisText(12, 11, 10, 2, 0, "\x82\x54\x0a\x82\x55\x0a\x83\x66\x83\x82");
         DisplayBasicWindow(0x34);
         obj->state2++;
         break;

      case 9:
         if (gPadStateNewPresses & PAD_LEFT) {
            obj->state2 -= 3;
            return;
         }
         if (gWindowChoice.raw == 0x34ff) {
            obj->state = 0;
            obj->state2 = 99;
            CloseWindow(0x34);
            return;
         }
         if (gWindowChoice.s.windowId == 0x34 && gWindowChoice.s.choice != 0) {
            gState.useDefaultUnits = 1;
            gState.mapNum = gWindowChoice.s.choice + 3;
            obj->state2 = 99;
         }
         if (gWindowChoice.raw == 0x3403) {
            gState.useDefaultUnits = 1;
            gState.mapNum = 8;
            obj->state2 = 99;
         }
         break;

      case 99:
         gState.primary = STATE_27;
         gState.secondary = 0;
         gState.state3 = 0;
         gState.state4 = 0;
         obj->functionIndex = OBJF_NULL;
         break;

      } // switch (obj->state2) (via state:9)

      break;

   case 10:

      switch (obj->state2) {
      case 0:
         CloseWindow(0x34);
         DrawWindow(0x34, 0, 0, 160, 226, 150, 10, WBS_CROSSED, 9);
         // 1-9
         DrawSjisText(12, 11, 10, 2, 0,
                      "\x82\x50\x0a\x82\x51\x0a\x82\x52\x0a\x82\x53\x0a\x82\x54\x0a\x82\x55\x0a\x82"
                      "\x56\x0a\x82\x57\x0a\x82\x58");
         DisplayBasicWindow(0x34);
         obj->state2++;
         break;

      case 1:
         if (gWindowChoice.raw == 0x34ff) {
            obj->state = 0;
            obj->state2 = 99;
            CloseWindow(0x34);
            break;
         }
         if (gWindowChoice.s.choice != 0) {
            CloseWindow(0x34);
            obj->state = 0;
         }
         break;

      case 99:
         CloseWindow(0x34);
         obj->state = 0;
         break;
      }

   // fallthrough
   case 11:
   case 12:
   case 13:

      switch (obj->state2) {
      case 0:
         DrawWindow(0x34, 0, 0, 136, 190, 150, 10, WBS_CROSSED, 10);
         // 1-10
         DrawSjisText(12, 11, 10, 2, 0,
                      "\x82\x50\x0a\x82\x51\x0a\x82\x52\x0a\x82\x53\x0a\x82\x54\x0a\x82\x55\x0a\x82"
                      "\x56\x0a\x82\x57\x0a\x82\x58\x0a\x82\x50\x82\x4f");
         DisplayBasicWindow(0x34);
         obj->state2++;
         break;

      case 1:
         if (gPadStateNewPresses & PAD_RIGHT) {
            obj->state2++;
            return;
         }
         if (gWindowChoice.raw == 0x34ff) {
            obj->state = 0;
            obj->state2 = 99;
            CloseWindow(0x34);
            return;
         }
         if (gWindowChoice.s.windowId == 0x34 && gWindowChoice.s.choice != 0) {
            gState.mapNum = gWindowChoice.s.choice + 43;
            obj->state2 = 99;
         }
         break;

      case 2:
         DrawWindow(0x34, 0, 0, 136, 190, 150, 10, WBS_CROSSED, 10);
         // 11-20
         DrawSjisText(12, 11, 10, 2, 0,
                      "\x82\x50\x82\x50\x0a\x82\x50\x82\x51\x0a\x82\x50\x82\x52\x0a\x82\x50\x82\x53"
                      "\x0a\x82\x50\x82\x54\x0a\x82\x50\x82\x55\x0a\x82\x50\x82\x56\x0a\x82\x50\x82"
                      "\x57\x0a\x82\x50\x82\x58\x0a\x82\x51\x82\x4f");
         DisplayBasicWindow(0x34);
         obj->state2++;
         break;

      case 3:
         if (gPadStateNewPresses & PAD_LEFT) {
            obj->state2 -= 3;
            return;
         }
         if (gPadStateNewPresses & PAD_RIGHT) {
            obj->state2++;
            return;
         }
         if (gWindowChoice.raw == 0x34ff) {
            obj->state = 0;
            obj->state2 = 99;
            CloseWindow(0x34);
            return;
         }
         if (gWindowChoice.s.windowId == 0x34 && gWindowChoice.s.choice != 0) {
            gState.mapNum = gWindowChoice.s.choice + 53;
            obj->state2 = 99;
         }
         break;

      case 4:
         DrawWindow(0x34, 0, 0, 136, 190, 150, 10, WBS_CROSSED, 10);
         // 21
         DrawSjisText(12, 11, 10, 2, 0, "\x82\x51\x82\x50");
         DisplayBasicWindow(0x34);
         obj->state2++;
         break;

      case 5:
         if (gPadStateNewPresses & PAD_LEFT) {
            obj->state2 -= 3;
            return;
         }
         if (gWindowChoice.raw == 0x34ff) {
            obj->state = 0;
            obj->state2 = 99;
            CloseWindow(0x34);
            return;
         }
         if (gWindowChoice.s.windowId == 0x34 && gWindowChoice.s.choice != 0) {
            gState.mapNum = gWindowChoice.s.choice + 63;
            obj->state2 = 99;
         }
         break;

      case 99:
         gState.primary = STATE_3;
         gState.secondary = 0;
         gState.state3 = 0;
         gState.state4 = 0;
         obj->functionIndex = OBJF_NULL;
         break;
      }

      break;

   case 15:
      ResetStateForNewGame();
      gState.primary = STATE_MOVIE;
      gState.movieIdxToPlay = 0;
      gState.secondary = 0;
      gState.state3 = 0;
      gState.state4 = 0;
      obj->functionIndex = OBJF_NULL;
      break;

   case 16:
      obj->functionIndex = OBJF_NULL;
      o = Obj_GetUnused();
      o->functionIndex = 584;
      break;
   }
}
