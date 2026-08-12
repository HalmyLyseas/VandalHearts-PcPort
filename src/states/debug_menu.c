#include "common.h"
#include "object.h"
#include "state.h"
#include "units.h"
#include "window.h"

void Objf414_DebugMenu(Object *obj) {
   s32 i;

   switch (obj->state) {
   case 0:
      CloseWindow(0x35);
      gWindowChoiceHeight = 17;
      gWindowChoicesTopMargin = 10;
      gWindowActiveIdx = 0x34;
      DrawWindow(0x34, 0, 0, 200, 225, 100, 4, WBS_CROSSED, 9);
      DrawSjisText(
          12, 11, 20, 2, 0,
          "BATTLE\nFONT OFF\nDEBUG MODE\n\nIVENT MAP\nGAME START\nUNIT SELECT\n\nSELECT B");
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
   }
}
