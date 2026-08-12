#include "common.h"
#include "object.h"
#include "window.h"
#include "state.h"
#include "audio.h"
#include "graphics.h"

void WorldMap_HandleCancel(Object *worldMap) {
   if (gWindowChoice.raw == 0x34ff) {
      gState.choices[0] = GetWindowChoice(0x34) - 1;
      CloseWindow(0x3c);
      worldMap->state = 1;
      worldMap->state2 = 0;
   }
}

#undef OBJF
#define OBJF 579
s32 Objf579_WorldMap(Object *obj) {
   extern u8 s_worldMapDstState;

   s16 markerX, markerY;
   Object *obj_a1;

   markerX = 0;
   markerY = 0;
   gWindowChoiceHeight = 18;
   gWindowChoicesTopMargin = 10;

   switch (obj->state) {
   case 0:
      gState.choices[0] = 0;
      FadeInScreen(2, 10);
      gWindowActiveIdx = 0x34;
      obj->state++;

   // fallthrough
   case 1:

      switch (obj->state2) {
      case 0:
         gSignal1 = 1;
         obj_a1 = Obj_GetUnused();
         obj_a1->functionIndex = OBJF_WORLD_ACTIONS;
         obj->state2++;

      // fallthrough
      case 1:
         if (gSignal1 == 0) {
            gWindowActiveIdx = 0x34;
            obj->state = 2;
            obj->state2 = 0;
         }
         break;
      }

      break;

   case 2:

      switch (gState.worldMapState) {
      case 11:

         switch (obj->state2) {
         case 0:
            DrawWindow(0x34, 0, 0, 176, 54, 124, 16, WBS_CROSSED, 2);
            DrawText(12, 11, 20, 3, 1, gStringTable[33]);
            DrawText(12, 29, 20, 3, 0, gStringTable[34]);
            gState.choices[0] = gState.worldMapDestination;
            DisplayCustomWindowWithSetChoice(0x34, 0, 1, 0, 1, 0, gState.choices[0]);
            obj->state2++;
            break;

         case 1:

            switch (GetWindowChoice(0x34)) {
            case 1:
               gState.worldMapDestination = 0;
               break;
            case 2:
               gState.worldMapDestination = 1;
               break;
            }

            WorldMap_HandleCancel(obj);
            if (gWindowChoice.raw == 0x3402) {
               gState.townState = 0;
               s_worldMapDstState = STATE_7;
               obj->state = 99;
               obj->state2 = 0;
            }
            break;
         }

         break;

      case 12:

         switch (obj->state2) {
         case 0:
            DrawWindow(0x34, 0, 0, 176, 72, 124, 16, WBS_CROSSED, 3);
            DrawText(12, 11, 20, 3, 1, gStringTable[33]);
            DrawText(12, 29, 20, 3, 0, "#34\n#35");
            gState.choices[0] = gState.worldMapDestination;
            DisplayCustomWindowWithSetChoice(0x34, 0, 1, 0, 1, 0, gState.choices[0]);
            obj->state2++;
            break;

         case 1:

            switch (GetWindowChoice(0x34)) {
            case 1:
               gState.worldMapDestination = 0;
               break;
            case 2:
               gState.worldMapDestination = 1;
               break;
            case 3:
               gState.worldMapDestination = 2;
               break;
            }

            WorldMap_HandleCancel(obj);
            if (gWindowChoice.raw == 0x3402) {
               gState.townState = 5;
               s_worldMapDstState = STATE_7;
               obj->state = 99;
               obj->state2 = 0;
            }
            if (gWindowChoice.raw == 0x3403) {
               s_worldMapDstState = STATE_26;
               gState.scene = 8;
               obj->state = 99;
               obj->state2 = 0;
            }
            break;
         }

         break;

      case 13:

         switch (obj->state2) {
         case 0:
            DrawWindow(0x34, 0, 0, 176, 90, 124, 16, WBS_CROSSED, 4);
            DrawText(12, 11, 20, 3, 1, gStringTable[33]);
            DrawText(12, 29, 20, 3, 0, gStringTable[34]);
            DrawText(12, 47, 20, 3, 1, gStringTable[35]);
            DrawText(12, 65, 20, 3, 0, gStringTable[36]);
            gState.choices[0] = gState.worldMapDestination;
            DisplayCustomWindowWithSetChoice(0x34, 0, 1, 0, 1, 0, gState.choices[0]);
            obj->state2++;
            break;

         case 1:

            switch (GetWindowChoice(0x34)) {
            case 1:
               gState.worldMapDestination = 0;
               break;
            case 2:
               gState.worldMapDestination = 1;
               break;
            case 3:
               gState.worldMapDestination = 2;
               break;
            case 4:
               gState.worldMapDestination = 3;
               break;
            }

            WorldMap_HandleCancel(obj);
            if (gWindowChoice.raw == 0x3402) {
               gState.townState = 5;
               s_worldMapDstState = STATE_7;
               obj->state = 99;
               obj->state2 = 0;
            }
            if (gWindowChoice.raw == 0x3404) {
               s_worldMapDstState = STATE_26;
               gState.scene = 11;
               obj->state = 99;
               obj->state2 = 0;
            }
            break;
         }

         break;

      case 14:

         switch (obj->state2) {
         case 0:
            DrawWindow(0x34, 0, 0, 176, 108, 124, 16, WBS_CROSSED, 5);
            DrawText(12, 11, 20, 3, 1, gStringTable[33]);
            DrawText(12, 29, 20, 3, 0, gStringTable[34]);
            DrawText(12, 47, 20, 3, 1, "#35\n#36");
            DrawText(12, 83, 20, 3, 0, gStringTable[37]);
            gState.choices[0] = gState.worldMapDestination;
            DisplayCustomWindowWithSetChoice(0x34, 0, 1, 0, 1, 0, gState.choices[0]);
            obj->state2++;
            break;

         case 1:
            SlideWindowTo(0x34, 124, 16);
            if (GetWindowChoice(0x34) > 3) {
               SlideWindowTo(0x34, 10, 16);
            }

            switch (GetWindowChoice(0x34)) {
            case 1:
               gState.worldMapDestination = 0;
               break;
            case 2:
               gState.worldMapDestination = 1;
               break;
            case 3:
               gState.worldMapDestination = 2;
               break;
            case 4:
               gState.worldMapDestination = 3;
               break;
            case 5:
               gState.worldMapDestination = 4;
               break;
            }

            WorldMap_HandleCancel(obj);
            if (gWindowChoice.raw == 0x3402) {
               gState.townState = 5;
               s_worldMapDstState = STATE_7;
               obj->state = 99;
               obj->state2 = 0;
            }
            if (gWindowChoice.raw == 0x3405) {
               gState.townState = 6;
               s_worldMapDstState = STATE_7;
               obj->state = 99;
               obj->state2 = 0;
            }
            break;
         }

         break;

      case 15:

         switch (obj->state2) {
         case 0:
            DrawWindow(0x34, 0, 0, 176, 126, 124, 16, WBS_CROSSED, 6);
            DrawText(12, 11, 20, 3, 1, gStringTable[33]);
            DrawText(12, 29, 20, 3, 0, gStringTable[34]);
            DrawText(12, 47, 20, 3, 1, "#35\n#36");
            DrawText(12, 83, 20, 3, 0, "#37\n#68");
            gState.choices[0] = gState.worldMapDestination;
            DisplayCustomWindowWithSetChoice(0x34, 0, 1, 0, 1, 0, gState.choices[0]);
            obj->state2++;
            break;

         case 1:
            SlideWindowTo(0x34, 124, 16);
            if (GetWindowChoice(0x34) > 3) {
               SlideWindowTo(0x34, 10, 16);
            }

            switch (GetWindowChoice(0x34)) {
            case 1:
               gState.worldMapDestination = 0;
               break;
            case 2:
               gState.worldMapDestination = 1;
               break;
            case 3:
               gState.worldMapDestination = 2;
               break;
            case 4:
               gState.worldMapDestination = 3;
               break;
            case 5:
               gState.worldMapDestination = 4;
               break;
            case 6:
               gState.worldMapDestination = 5;
               break;
            }

            WorldMap_HandleCancel(obj);
            if (gWindowChoice.raw == 0x3402) {
               gState.townState = 5;
               s_worldMapDstState = STATE_7;
               obj->state = 99;
               obj->state2 = 0;
            }
            if (gWindowChoice.raw == 0x3405) {
               gState.townState = 9;
               s_worldMapDstState = STATE_7;
               obj->state = 99;
               obj->state2 = 0;
            }
            if (gWindowChoice.raw == 0x3406) {
               s_worldMapDstState = STATE_26;
               gState.scene = 15;
               obj->state = 99;
               obj->state2 = 0;
            }
            break;
         }

         break;

      case 16:

         switch (obj->state2) {
         case 0:
            DrawWindow(0x34, 0, 0, 176, 144, 124, 16, WBS_CROSSED, 7);
            DrawText(12, 11, 20, 3, 1, gStringTable[33]);
            DrawText(12, 29, 20, 3, 0, gStringTable[34]);
            DrawText(12, 47, 20, 3, 1, "#35\n#36");
            DrawText(12, 83, 20, 3, 0, gStringTable[37]);
            DrawText(12, 101, 20, 3, 1, "#68");
            DrawText(12, 119, 20, 3, 0, "#69");
            gState.choices[0] = gState.worldMapDestination;
            DisplayCustomWindowWithSetChoice(0x34, 0, 1, 0, 1, 0, gState.choices[0]);
            obj->state2++;
            break;

         case 1:
            SlideWindowTo(0x34, 124, 16);
            if (GetWindowChoice(0x34) > 3) {
               SlideWindowTo(0x34, 10, 16);
            }

            switch (GetWindowChoice(0x34)) {
            case 1:
               gState.worldMapDestination = 0;
               break;
            case 2:
               gState.worldMapDestination = 1;
               break;
            case 3:
               gState.worldMapDestination = 2;
               break;
            case 4:
               gState.worldMapDestination = 3;
               break;
            case 5:
               gState.worldMapDestination = 4;
               break;
            case 6:
               gState.worldMapDestination = 5;
               break;
            case 7:
               gState.worldMapDestination = 6;
               break;
            }

            WorldMap_HandleCancel(obj);
            if (gWindowChoice.raw == 0x3402) {
               gState.townState = 5;
               s_worldMapDstState = STATE_7;
               obj->state = 99;
               obj->state2 = 0;
            }
            if (gWindowChoice.raw == 0x3405) {
               gState.townState = 10;
               s_worldMapDstState = STATE_7;
               obj->state = 99;
               obj->state2 = 0;
            }
            if (gWindowChoice.raw == 0x3407) {
               s_worldMapDstState = STATE_26;
               gState.scene = 17;
               obj->state = 99;
               obj->state2 = 0;
            }
            break;
         }

         break;

      case 17:

         switch (obj->state2) {
         case 0:
            DrawWindow(0x34, 0, 0, 176, 108, 124, 16, WBS_CROSSED, 5);
            DrawText(12, 11, 20, 3, 1, gStringTable[33]);
            DrawText(12, 29, 20, 3, 0, gStringTable[34]);
            DrawText(12, 47, 20, 3, 1, "#35\n#36");
            DrawText(12, 83, 20, 3, 0, gStringTable[37]);
            gState.choices[0] = gState.worldMapDestination;
            DisplayCustomWindowWithSetChoice(0x34, 0, 1, 0, 1, 0, gState.choices[0]);
            obj->state2++;
            break;

         case 1:
            SlideWindowTo(0x34, 124, 16);
            if (GetWindowChoice(0x34) > 3) {
               SlideWindowTo(0x34, 10, 16);
            }

            switch (GetWindowChoice(0x34)) {
            case 1:
               gState.worldMapDestination = 0;
               break;
            case 2:
               gState.worldMapDestination = 1;
               break;
            case 3:
               gState.worldMapDestination = 2;
               break;
            case 4:
               gState.worldMapDestination = 3;
               break;
            case 5:
               gState.worldMapDestination = 4;
               break;
            }

            WorldMap_HandleCancel(obj);
            if (gWindowChoice.raw == 0x3402) {
               gState.townState = 5;
               s_worldMapDstState = STATE_7;
               obj->state = 99;
               obj->state2 = 0;
            }
            if (gWindowChoice.raw == 0x3405) {
               gState.townState = 7;
               s_worldMapDstState = STATE_7;
               obj->state = 99;
               obj->state2 = 0;
            }
            break;
         }

         break;

      case 21:

         switch (obj->state2) {
         case 0:
            DrawWindow(0x34, 0, 0, 176, 54, 10, 166, WBS_CROSSED, 2);
            DrawText(12, 11, 20, 3, 0, gStringTable[38]);
            DrawText(12, 29, 20, 3, 0, gStringTable[39]);
            gState.choices[0] = gState.worldMapDestination - 7;
            DisplayCustomWindowWithSetChoice(0x34, 0, 1, 0, 1, 0, gState.choices[0]);
            obj->state2++;
            break;

         case 1:

            switch (GetWindowChoice(0x34)) {
            case 1:
               gState.worldMapDestination = 7;
               break;
            case 2:
               gState.worldMapDestination = 8;
               break;
            }

            WorldMap_HandleCancel(obj);
            if (gWindowChoice.raw == 0x3401) {
               gState.townState = 11;
               s_worldMapDstState = STATE_7;
               obj->state = 99;
               obj->state2 = 0;
            }
            if (gWindowChoice.raw == 0x3402) {
               s_worldMapDstState = STATE_26;
               gState.scene = 21;
               obj->state = 99;
               obj->state2 = 0;
            }
            break;
         }

         break;

      case 22:

         switch (obj->state2) {
         case 0:
            DrawWindow(0x34, 0, 0, 176, 72, 10, 148, WBS_CROSSED, 3);
            DrawText(12, 11, 20, 3, 0, gStringTable[38]);
            DrawText(12, 29, 20, 3, 1, gStringTable[39]);
            DrawText(12, 47, 20, 3, 0, gStringTable[40]);
            gState.choices[0] = gState.worldMapDestination - 7;
            DisplayCustomWindowWithSetChoice(0x34, 0, 1, 0, 1, 0, gState.choices[0]);
            obj->state2++;
            break;

         case 1:

            switch (GetWindowChoice(0x34)) {
            case 1:
               gState.worldMapDestination = 7;
               break;
            case 2:
               gState.worldMapDestination = 8;
               break;
            case 3:
               gState.worldMapDestination = 9;
               break;
            }

            WorldMap_HandleCancel(obj);
            if (gWindowChoice.raw == 0x3401) {
               gState.townState = 11;
               s_worldMapDstState = STATE_7;
               obj->state = 99;
               obj->state2 = 0;
            }
            if (gWindowChoice.raw == 0x3403) {
               s_worldMapDstState = STATE_26;
               gState.scene = 24;
               obj->state = 99;
               obj->state2 = 0;
            }
            break;
         }

         break;

      case 23:

         switch (obj->state2) {
         case 0:
            DrawWindow(0x34, 0, 0, 176, 90, 10, 130, WBS_CROSSED, 4);
            DrawText(12, 11, 20, 3, 0, gStringTable[38]);
            DrawText(12, 29, 20, 3, 1, "#39\n#40");
            DrawText(12, 65, 20, 3, 0, gStringTable[41]);
            gState.choices[0] = gState.worldMapDestination - 7;
            DisplayCustomWindowWithSetChoice(0x34, 0, 1, 0, 1, 0, gState.choices[0]);
            obj->state2++;
            break;

         case 1:

            switch (GetWindowChoice(0x34)) {
            case 1:
               gState.worldMapDestination = 7;
               break;
            case 2:
               gState.worldMapDestination = 8;
               break;
            case 3:
               gState.worldMapDestination = 9;
               break;
            case 4:
               gState.worldMapDestination = 10;
               break;
            }

            WorldMap_HandleCancel(obj);
            if (gWindowChoice.raw == 0x3401) {
               gState.townState = 11;
               s_worldMapDstState = STATE_7;
               obj->state = 99;
               obj->state2 = 0;
            }
            if (gWindowChoice.raw == 0x3404) {
               s_worldMapDstState = STATE_26;
               gState.scene = 27;
               obj->state = 99;
               obj->state2 = 0;
            }
            break;
         }

         break;

      case 24:

         switch (obj->state2) {
         case 0:
            DrawWindow(0x34, 0, 0, 176, 90, 10, 130, WBS_CROSSED, 4);
            DrawText(12, 11, 20, 3, 0, gStringTable[38]);
            DrawText(12, 29, 20, 3, 1, "#39\n#40");
            DrawText(12, 65, 20, 2, 0, gStringTable[41]);
            gState.choices[0] = gState.worldMapDestination - 7;
            DisplayCustomWindowWithSetChoice(0x34, 0, 1, 0, 1, 0, gState.choices[0]);
            obj->state2++;
            break;

         case 1:

            switch (GetWindowChoice(0x34)) {
            case 1:
               gState.worldMapDestination = 7;
               break;
            case 2:
               gState.worldMapDestination = 8;
               break;
            case 3:
               gState.worldMapDestination = 9;
               break;
            case 4:
               gState.worldMapDestination = 10;
               break;
            }

            WorldMap_HandleCancel(obj);
            if (gWindowChoice.raw == 0x3401) {
               gState.townState = 11;
               s_worldMapDstState = STATE_7;
               obj->state = 99;
               obj->state2 = 0;
            }
            if (gWindowChoice.raw == 0x3404) {
               s_worldMapDstState = STATE_26;
               gState.scene = 29;
               obj->state = 99;
               obj->state2 = 0;
            }
            break;
         }

         break;

      case 25:

         switch (obj->state2) {
         case 0:
            DrawWindow(0x34, 0, 0, 176, 90, 10, 130, WBS_CROSSED, 4);
            DrawText(12, 11, 20, 3, 0, gStringTable[38]);
            DrawText(12, 29, 20, 3, 1, "#39\n#40");
            DrawText(12, 65, 20, 2, 0, gStringTable[41]);
            gState.choices[0] = gState.worldMapDestination - 7;
            DisplayCustomWindowWithSetChoice(0x34, 0, 1, 0, 1, 0, gState.choices[0]);
            obj->state2++;
            break;

         case 1:

            switch (GetWindowChoice(0x34)) {
            case 1:
               gState.worldMapDestination = 7;
               break;
            case 2:
               gState.worldMapDestination = 8;
               break;
            case 3:
               gState.worldMapDestination = 9;
               break;
            case 4:
               gState.worldMapDestination = 10;
               break;
            }

            WorldMap_HandleCancel(obj);
            if (gWindowChoice.raw == 0x3401) {
               gState.townState = 11;
               s_worldMapDstState = STATE_7;
               obj->state = 99;
               obj->state2 = 0;
            }
            if (gWindowChoice.raw == 0x3404) {
               s_worldMapDstState = STATE_26;
               gState.scene = 32;
               obj->state = 99;
               obj->state2 = 0;
            }
            break;
         }

         break;

      case 31:

         switch (obj->state2) {
         case 0:
            DrawWindow(0x34, 0, 0, 176, 54, 10, 16, WBS_CROSSED, 2);
            DrawText(12, 11, 20, 3, 1, gStringTable[42]);
            DrawText(12, 29, 20, 3, 0, gStringTable[43]);
            gState.choices[0] = gState.worldMapDestination - 11;
            DisplayCustomWindowWithSetChoice(0x34, 0, 1, 0, 1, 0, gState.choices[0]);
            obj->state2++;
            break;

         case 1:

            switch (GetWindowChoice(0x34)) {
            case 1:
               gState.worldMapDestination = 11;
               break;
            case 2:
               gState.worldMapDestination = 12;
               break;
            }

            WorldMap_HandleCancel(obj);
            if (gWindowChoice.raw == 0x3402) {
               gState.townState = 13;
               s_worldMapDstState = STATE_7;
               obj->state = 99;
               obj->state2 = 0;
            }
            break;
         }

         break;

      case 32:

         switch (obj->state2) {
         case 0:
            DrawWindow(0x34, 0, 0, 176, 54, 10, 16, WBS_CROSSED, 2);
            DrawText(12, 11, 20, 3, 0, "#42\n#43");
            gState.choices[0] = gState.worldMapDestination - 11;
            DisplayCustomWindowWithSetChoice(0x34, 0, 1, 0, 1, 0, gState.choices[0]);
            obj->state2++;
            break;

         case 1:

            switch (GetWindowChoice(0x34)) {
            case 1:
               gState.worldMapDestination = 11;
               break;
            case 2:
               gState.worldMapDestination = 12;
               break;
            }

            WorldMap_HandleCancel(obj);
            if (gWindowChoice.raw == 0x3401) {
               s_worldMapDstState = STATE_26;
               gState.scene = 38;
               obj->state = 99;
               obj->state2 = 0;
            }
            if (gWindowChoice.raw == 0x3402) {
               gState.townState = 16;
               s_worldMapDstState = STATE_7;
               obj->state = 99;
               obj->state2 = 0;
            }
            break;
         }

         break;

      case 41:

         switch (obj->state2) {
         case 0:
            DrawWindow(0x34, 0, 0, 176, 54, 10, 16, WBS_CROSSED, 2);
            DrawText(12, 11, 20, 3, 0, "#44\n#45");
            gState.choices[0] = gState.worldMapDestination - 13;
            DisplayCustomWindowWithSetChoice(0x34, 0, 1, 0, 1, 0, gState.choices[0]);
            obj->state2++;
            break;

         case 1:

            switch (GetWindowChoice(0x34)) {
            case 1:
               gState.worldMapDestination = 13;
               break;
            case 2:
               gState.worldMapDestination = 14;
               break;
            }

            WorldMap_HandleCancel(obj);
            if (gWindowChoice.raw == 0x3401) {
               gState.townState = 17;
               s_worldMapDstState = STATE_7;
               obj->state = 99;
               obj->state2 = 0;
            }
            if (gWindowChoice.raw == 0x3402) {
               s_worldMapDstState = STATE_26;
               gState.scene = 43;
               obj->state = 99;
               obj->state2 = 0;
            }
            break;
         }

         break;

      case 42:

         switch (obj->state2) {
         case 0:
            DrawWindow(0x34, 0, 0, 176, 72, 10, 16, WBS_CROSSED, 3);
            DrawText(12, 11, 20, 3, 0, gStringTable[44]);
            DrawText(12, 29, 20, 3, 1, gStringTable[45]);
            DrawText(12, 47, 20, 3, 0, gStringTable[46]);
            gState.choices[0] = gState.worldMapDestination - 13;
            DisplayCustomWindowWithSetChoice(0x34, 0, 1, 0, 1, 0, gState.choices[0]);
            obj->state2++;
            break;

         case 1:

            switch (GetWindowChoice(0x34)) {
            case 1:
               gState.worldMapDestination = 13;
               break;
            case 2:
               gState.worldMapDestination = 14;
               break;
            case 3:
               gState.worldMapDestination = 15;
               break;
            }

            WorldMap_HandleCancel(obj);
            if (gWindowChoice.raw == 0x3401) {
               gState.townState = 17;
               s_worldMapDstState = STATE_7;
               obj->state = 99;
               obj->state2 = 0;
            }
            if (gWindowChoice.raw == 0x3403) {
               s_worldMapDstState = STATE_26;
               gState.scene = 46;
               obj->state = 99;
               obj->state2 = 0;
            }
            break;
         }

         break;

      case 43:

         switch (obj->state2) {
         case 0:
            DrawWindow(0x34, 0, 0, 176, 90, 10, 16, WBS_CROSSED, 4);
            DrawText(12, 11, 20, 3, 0, gStringTable[44]);
            DrawText(12, 29, 20, 3, 1, "#45\n#46");
            DrawText(12, 65, 20, 3, 0, gStringTable[47]);
            gState.choices[0] = gState.worldMapDestination - 13;
            DisplayCustomWindowWithSetChoice(0x34, 0, 1, 0, 1, 0, gState.choices[0]);
            obj->state2++;
            break;

         case 1:

            switch (GetWindowChoice(0x34)) {
            case 1:
               gState.worldMapDestination = 13;
               break;
            case 2:
               gState.worldMapDestination = 14;
               break;
            case 3:
               gState.worldMapDestination = 15;
               break;
            case 4:
               gState.worldMapDestination = 16;
               break;
            }

            WorldMap_HandleCancel(obj);
            if (gWindowChoice.raw == 0x3401) {
               gState.townState = 17;
               s_worldMapDstState = STATE_7;
               obj->state = 99;
               obj->state2 = 0;
            }
            if (gWindowChoice.raw == 0x3404) {
               s_worldMapDstState = STATE_26;
               gState.scene = 47;
               obj->state = 99;
               obj->state2 = 0;
            }
            break;
         }

         break;

      case 51:

         switch (obj->state2) {
         case 0:
            DrawWindow(0x34, 0, 0, 176, 54, 10, 16, WBS_CROSSED, 2);
            DrawText(12, 11, 20, 3, 0, "#48\n#49");
            gState.choices[0] = gState.worldMapDestination - 17;
            DisplayCustomWindowWithSetChoice(0x34, 0, 1, 0, 1, 0, gState.choices[0]);
            obj->state2++;
            break;

         case 1:

            switch (GetWindowChoice(0x34)) {
            case 1:
               gState.worldMapDestination = 17;
               break;
            case 2:
               gState.worldMapDestination = 18;
               break;
            }

            WorldMap_HandleCancel(obj);
            if (gWindowChoice.raw == 0x3401) {
               gState.townState = 19;
               s_worldMapDstState = STATE_7;
               obj->state = 99;
               obj->state2 = 0;
            }
            if (gWindowChoice.raw == 0x3402) {
               s_worldMapDstState = STATE_26;
               gState.scene = 55;
               obj->state = 99;
               obj->state2 = 0;
            }
            break;
         }

         break;

      case 52:

         switch (obj->state2) {
         case 0:
            DrawWindow(0x34, 0, 0, 176, 72, 10, 16, WBS_CROSSED, 3);
            DrawText(12, 11, 20, 3, 0, gStringTable[48]);
            DrawText(12, 29, 20, 3, 1, gStringTable[49]);
            DrawText(12, 47, 20, 3, 0, gStringTable[50]);
            gState.choices[0] = gState.worldMapDestination - 17;
            DisplayCustomWindowWithSetChoice(0x34, 0, 1, 0, 1, 0, gState.choices[0]);
            obj->state2++;
            break;

         case 1:

            switch (GetWindowChoice(0x34)) {
            case 1:
               gState.worldMapDestination = 17;
               break;
            case 2:
               gState.worldMapDestination = 18;
               break;
            case 3:
               gState.worldMapDestination = 19;
               break;
            }

            WorldMap_HandleCancel(obj);
            if (gWindowChoice.raw == 0x3401) {
               gState.townState = 19;
               s_worldMapDstState = STATE_7;
               obj->state = 99;
               obj->state2 = 0;
            }
            if (gWindowChoice.raw == 0x3403) {
               gState.townState = 20;
               s_worldMapDstState = STATE_7;
               obj->state = 99;
               obj->state2 = 0;
            }
            break;
         }

         break;

      case 53:

         switch (obj->state2) {
         case 0:
            DrawWindow(0x34, 0, 0, 176, 108, 10, 16, WBS_CROSSED, 5);
            DrawText(12, 11, 20, 3, 0, gStringTable[48]);
            DrawText(12, 29, 20, 3, 1, gStringTable[49]);
            DrawText(12, 47, 20, 3, 0, gStringTable[50]);
            DrawText(12, 65, 20, 3, 1, gStringTable[51]);
            DrawText(12, 83, 20, 3, 0, gStringTable[52]);
            gState.choices[0] = gState.worldMapDestination - 17;
            DisplayCustomWindowWithSetChoice(0x34, 0, 1, 0, 1, 0, gState.choices[0]);
            obj->state2++;
            break;

         case 1:

            switch (GetWindowChoice(0x34)) {
            case 1:
               gState.worldMapDestination = 17;
               break;
            case 2:
               gState.worldMapDestination = 18;
               break;
            case 3:
               gState.worldMapDestination = 19;
               break;
            case 4:
               gState.worldMapDestination = 20;
               break;
            case 5:
               gState.worldMapDestination = 21;
               break;
            }

            WorldMap_HandleCancel(obj);
            if (gWindowChoice.raw == 0x3401) {
               gState.townState = 19;
               s_worldMapDstState = STATE_7;
               obj->state = 99;
               obj->state2 = 0;
            }
            if (gWindowChoice.raw == 0x3403) {
               gState.townState = 24;
               s_worldMapDstState = STATE_7;
               obj->state = 99;
               obj->state2 = 0;
            }
            if (gWindowChoice.raw == 0x3405) {
               s_worldMapDstState = STATE_26;
               gState.scene = 65;
               obj->state = 99;
               obj->state2 = 0;
            }
            break;
         }

         break;

      case 61:

         switch (obj->state2) {
         case 0:
            DrawWindow(0x34, 0, 0, 176, 54, 124, 16, WBS_CROSSED, 2);
            DrawText(12, 11, 20, 3, 1, gStringTable[53]);
            DrawText(12, 29, 20, 3, 0, gStringTable[54]);
            gState.choices[0] = gState.worldMapDestination - 22;
            DisplayCustomWindowWithSetChoice(0x34, 0, 1, 0, 1, 0, gState.choices[0]);
            obj->state2++;
            break;

         case 1:

            switch (GetWindowChoice(0x34)) {
            case 1:
               gState.worldMapDestination = 22;
               break;
            case 2:
               gState.worldMapDestination = 23;
               break;
            }

            WorldMap_HandleCancel(obj);
            if (gWindowChoice.raw == 0x3402) {
               gState.townState = 25;
               s_worldMapDstState = STATE_7;
               obj->state = 99;
               obj->state2 = 0;
            }
            break;
         }

         break;

      case 62:

         switch (obj->state2) {
         case 0:
            DrawWindow(0x34, 0, 0, 176, 72, 124, 16, WBS_CROSSED, 3);
            DrawText(12, 11, 20, 3, 1, gStringTable[53]);
            DrawText(12, 29, 20, 3, 0, "#54\n#55");
            gState.choices[0] = gState.worldMapDestination - 22;
            DisplayCustomWindowWithSetChoice(0x34, 0, 1, 0, 1, 0, gState.choices[0]);
            obj->state2++;
            break;

         case 1:

            switch (GetWindowChoice(0x34)) {
            case 1:
               gState.worldMapDestination = 22;
               break;
            case 2:
               gState.worldMapDestination = 23;
               break;
            case 3:
               gState.worldMapDestination = 24;
               break;
            }

            WorldMap_HandleCancel(obj);
            if (gWindowChoice.raw == 0x3402) {
               gState.townState = 26;
               s_worldMapDstState = STATE_7;
               obj->state = 99;
               obj->state2 = 0;
            }
            if (gWindowChoice.raw == 0x3403) {
               s_worldMapDstState = STATE_26;
               obj->state = 99;
               obj->state2 = 0;
               gState.scene = 71;
            }
            break;
         }

         break;

      case 63:

         switch (obj->state2) {
         case 0:
            DrawWindow(0x34, 0, 0, 176, 90, 10, 16, WBS_CROSSED, 4);
            DrawText(12, 11, 20, 3, 1, gStringTable[53]);
            DrawText(12, 29, 20, 3, 0, gStringTable[54]);
            DrawText(12, 47, 20, 3, 1, gStringTable[55]);
            DrawText(12, 65, 20, 2, 0, gStringTable[56]);
            gState.choices[0] = gState.worldMapDestination - 22;
            DisplayCustomWindowWithSetChoice(0x34, 0, 1, 0, 1, 0, gState.choices[0]);
            obj->state2++;
            break;

         case 1:

            switch (GetWindowChoice(0x34)) {
            case 1:
               gState.worldMapDestination = 22;
               break;
            case 2:
               gState.worldMapDestination = 23;
               break;
            case 3:
               gState.worldMapDestination = 24;
               break;
            case 4:
               gState.worldMapDestination = 25;
               break;
            }

            WorldMap_HandleCancel(obj);
            if (gWindowChoice.raw == 0x3402) {
               gState.townState = 26;
               s_worldMapDstState = STATE_7;
               obj->state = 99;
               obj->state2 = 0;
            }
            if (gWindowChoice.raw == 0x3404) {
               gState.townState = 27;
               s_worldMapDstState = STATE_7;
               obj->state = 99;
               obj->state2 = 0;
            }
            break;
         }

         break;

      case 64:

         switch (obj->state2) {
         case 0:
            DrawWindow(0x34, 0, 0, 176, 108, 124, 120, WBS_CROSSED, 5);
            DrawText(12, 11, 20, 3, 1, gStringTable[53]);
            DrawText(12, 29, 20, 3, 0, gStringTable[54]);
            DrawText(12, 47, 20, 3, 1, gStringTable[55]);
            DrawText(12, 65, 20, 3, 0, "#56\n#57");
            gState.choices[0] = gState.worldMapDestination - 22;
            DisplayCustomWindowWithSetChoice(0x34, 0, 1, 0, 1, 0, gState.choices[0]);
            obj->state2++;
            break;

         case 1:

            switch (GetWindowChoice(0x34)) {
            case 1:
               gState.worldMapDestination = 22;
               break;
            case 2:
               gState.worldMapDestination = 23;
               break;
            case 3:
               gState.worldMapDestination = 24;
               break;
            case 4:
               gState.worldMapDestination = 25;
               break;
            case 5:
               gState.worldMapDestination = 26;
               break;
            }

            WorldMap_HandleCancel(obj);
            if (gWindowChoice.raw == 0x3402) {
               gState.townState = 26;
               s_worldMapDstState = STATE_7;
               obj->state = 99;
               obj->state2 = 0;
            }
            if (gWindowChoice.raw == 0x3404) {
               gState.townState = 30;
               s_worldMapDstState = STATE_7;
               obj->state = 99;
               obj->state2 = 0;
            }
            if (gWindowChoice.raw == 0x3405) {
               s_worldMapDstState = STATE_26;
               gState.scene = 74;
               obj->state = 99;
               obj->state2 = 0;
            }
            break;
         }

         break;

      case 65:

         switch (obj->state2) {
         case 0:
            DrawWindow(0x34, 0, 0, 176, 126, 124, 102, WBS_CROSSED, 6);
            DrawText(12, 11, 20, 3, 1, gStringTable[53]);
            DrawText(12, 29, 20, 3, 0, gStringTable[54]);
            DrawText(12, 47, 20, 3, 1, gStringTable[55]);
            DrawText(12, 65, 20, 3, 0, gStringTable[56]);
            DrawText(12, 83, 20, 3, 1, gStringTable[57]);
            DrawText(12, 101, 20, 3, 0, gStringTable[58]);
            gState.choices[0] = gState.worldMapDestination - 22;
            DisplayCustomWindowWithSetChoice(0x34, 0, 1, 0, 1, 0, gState.choices[0]);
            obj->state2++;
            break;

         case 1:

            switch (GetWindowChoice(0x34)) {
            case 1:
               gState.worldMapDestination = 22;
               break;
            case 2:
               gState.worldMapDestination = 23;
               break;
            case 3:
               gState.worldMapDestination = 24;
               break;
            case 4:
               gState.worldMapDestination = 25;
               break;
            case 5:
               gState.worldMapDestination = 26;
               break;
            case 6:
               gState.worldMapDestination = 27;
               break;
            }

            WorldMap_HandleCancel(obj);
            if (gWindowChoice.raw == 0x3402) {
               gState.townState = 26;
               s_worldMapDstState = STATE_7;
               obj->state = 99;
               obj->state2 = 0;
            }
            if (gWindowChoice.raw == 0x3404) {
               gState.townState = 31;
               s_worldMapDstState = STATE_7;
               obj->state = 99;
               obj->state2 = 0;
            }
            if (gWindowChoice.raw == 0x3406) {
               s_worldMapDstState = STATE_26;
               gState.scene = 77;
               obj->state = 99;
               obj->state2 = 0;
            }
            break;
         }

         break;

      case 66:

         switch (obj->state2) {
         case 0:
            DrawWindow(0x34, 0, 0, 176, 126, 124, 102, WBS_CROSSED, 6);
            DrawText(12, 11, 20, 3, 1, gStringTable[53]);
            DrawText(12, 29, 20, 3, 0, gStringTable[54]);
            DrawText(12, 47, 20, 3, 1, gStringTable[55]);
            DrawText(12, 65, 20, 3, 0, gStringTable[56]);
            DrawText(12, 83, 20, 3, 1, "#57\n#58");
            gState.choices[0] = gState.worldMapDestination - 22;
            DisplayCustomWindowWithSetChoice(0x34, 0, 1, 0, 1, 0, gState.choices[0]);
            obj->state2++;
            break;

         case 1:

            switch (GetWindowChoice(0x34)) {
            case 1:
               gState.worldMapDestination = 22;
               break;
            case 2:
               gState.worldMapDestination = 23;
               break;
            case 3:
               gState.worldMapDestination = 24;
               break;
            case 4:
               gState.worldMapDestination = 25;
               break;
            case 5:
               gState.worldMapDestination = 26;
               break;
            case 6:
               gState.worldMapDestination = 27;
               break;
            }

            WorldMap_HandleCancel(obj);
            if (gWindowChoice.raw == 0x3402) {
               gState.townState = 26;
               s_worldMapDstState = STATE_7;
               obj->state = 99;
               obj->state2 = 0;
            }
            if (gWindowChoice.raw == 0x3404) {
               s_worldMapDstState = STATE_26;
               gState.scene = 78;
               obj->state = 99;
               obj->state2 = 0;
            }
            break;
         }

         break;

      case 67:

         switch (obj->state2) {
         case 0:
            DrawWindow(0x34, 0, 0, 176, 144, 124, 84, WBS_CROSSED, 7);
            DrawText(12, 11, 20, 3, 1, gStringTable[53]);
            DrawText(12, 29, 20, 3, 0, gStringTable[54]);
            DrawText(12, 47, 20, 3, 1, "#55\n#56\n#57\n#58");
            DrawText(12, 119, 20, 3, 0, gStringTable[59]);
            gState.choices[0] = gState.worldMapDestination - 22;
            DisplayCustomWindowWithSetChoice(0x34, 0, 1, 0, 1, 0, gState.choices[0]);
            obj->state2++;
            break;

         case 1:

            switch (GetWindowChoice(0x34)) {
            case 1:
               gState.worldMapDestination = 22;
               break;
            case 2:
               gState.worldMapDestination = 23;
               break;
            case 3:
               gState.worldMapDestination = 24;
               break;
            case 4:
               gState.worldMapDestination = 25;
               break;
            case 5:
               gState.worldMapDestination = 26;
               break;
            case 6:
               gState.worldMapDestination = 27;
               break;
            case 7:
               gState.worldMapDestination = 28;
               break;
            }

            WorldMap_HandleCancel(obj);
            if (gWindowChoice.raw == 0x3402) {
               gState.townState = 26;
               s_worldMapDstState = STATE_7;
               obj->state = 99;
               obj->state2 = 0;
            }
            if (gWindowChoice.raw == 0x3407) {
               s_worldMapDstState = STATE_26;
               gState.scene = 82;
               obj->state = 99;
               obj->state2 = 0;
            }
            break;
         }

         break;

      case 71:

         switch (obj->state2) {
         case 0:
            DrawWindow(0x34, 0, 0, 176, 54, 10, 174, WBS_CROSSED, 2);
            DrawText(12, 11, 20, 2, 0, "#60\n#61");
            gState.choices[0] = gState.worldMapDestination - 29;
            DisplayCustomWindowWithSetChoice(0x34, 0, 1, 0, 1, 0, gState.choices[0]);
            obj->state2++;
            break;

         case 1:

            switch (GetWindowChoice(0x34)) {
            case 1:
               gState.worldMapDestination = 29;
               break;
            case 2:
               gState.worldMapDestination = 30;
               break;
            }

            WorldMap_HandleCancel(obj);
            if (gWindowChoice.raw == 0x3401) {
               gState.townState = 32;
               s_worldMapDstState = STATE_7;
               obj->state = 99;
               obj->state2 = 0;
            }
            if (gWindowChoice.raw == 0x3402) {
               s_worldMapDstState = STATE_26;
               gState.scene = 84;
               obj->state = 99;
               obj->state2 = 0;
            }
            break;
         }

         break;

      case 72:

         switch (obj->state2) {
         case 0:
            DrawWindow(0x34, 0, 0, 176, 72, 10, 156, WBS_CROSSED, 3);
            DrawText(12, 11, 20, 3, 0, gStringTable[60]);
            DrawText(12, 29, 20, 3, 1, gStringTable[61]);
            DrawText(12, 47, 20, 3, 0, gStringTable[62]);
            gState.choices[0] = gState.worldMapDestination - 29;
            DisplayCustomWindowWithSetChoice(0x34, 0, 1, 0, 1, 0, gState.choices[0]);
            obj->state2++;
            break;

         case 1:

            switch (GetWindowChoice(0x34)) {
            case 1:
               gState.worldMapDestination = 29;
               break;
            case 2:
               gState.worldMapDestination = 30;
               break;
            case 3:
               gState.worldMapDestination = 31;
               break;
            }

            WorldMap_HandleCancel(obj);
            if (gWindowChoice.raw == 0x3401) {
               gState.townState = 32;
               s_worldMapDstState = STATE_7;
               obj->state = 99;
               obj->state2 = 0;
            }
            if (gWindowChoice.raw == 0x3403) {
               s_worldMapDstState = STATE_26;
               gState.scene = 85;
               obj->state = 99;
               obj->state2 = 0;
            }
            break;
         }

         break;

      case 73:

         switch (obj->state2) {
         case 0:
            DrawWindow(0x34, 0, 0, 176, 90, 124, 138, WBS_CROSSED, 4);
            DrawText(12, 11, 20, 3, 0, gStringTable[60]);
            DrawText(12, 29, 20, 3, 1, "#61\n#62");
            DrawText(12, 65, 20, 3, 0, gStringTable[34]);
            gState.choices[0] = gState.worldMapDestination - 29;
            DisplayCustomWindowWithSetChoice(0x34, 0, 1, 0, 1, 0, gState.choices[0]);
            obj->state2++;
            break;

         case 1:

            switch (GetWindowChoice(0x34)) {
            case 1:
               gState.worldMapDestination = 29;
               break;
            case 2:
               gState.worldMapDestination = 30;
               break;
            case 3:
               gState.worldMapDestination = 31;
               break;
            case 4:
               gState.worldMapDestination = 32;
               break;
            }

            WorldMap_HandleCancel(obj);
            if (gWindowChoice.raw == 0x3401) {
               gState.townState = 32;
               s_worldMapDstState = STATE_7;
               obj->state = 99;
               obj->state2 = 0;
            }
            if (gWindowChoice.raw == 0x3404) {
               s_worldMapDstState = STATE_26;
               gState.scene = 90;
               obj->state = 99;
               obj->state2 = 0;
            }
            break;
         }

         break;

      case 74:

         switch (obj->state2) {
         case 0:
            DrawWindow(0x34, 0, 0, 176, 90, 124, 138, WBS_CROSSED, 4);
            DrawText(12, 11, 20, 3, 0, gStringTable[60]);
            DrawText(12, 29, 20, 3, 1, "#61\n#62");
            DrawText(12, 65, 20, 3, 0, gStringTable[34]);
            gState.choices[0] = gState.worldMapDestination - 29;
            DisplayCustomWindowWithSetChoice(0x34, 0, 1, 0, 1, 0, gState.choices[0]);
            obj->state2++;
            break;

         case 1:

            switch (GetWindowChoice(0x34)) {
            case 1:
               gState.worldMapDestination = 29;
               break;
            case 2:
               gState.worldMapDestination = 30;
               break;
            case 3:
               gState.worldMapDestination = 31;
               break;
            case 4:
               gState.worldMapDestination = 32;
               break;
            }

            WorldMap_HandleCancel(obj);
            if (gWindowChoice.raw == 0x3401) {
               gState.townState = 32;
               s_worldMapDstState = STATE_7;
               obj->state = 99;
               obj->state2 = 0;
            }
            if (gWindowChoice.raw == 0x3404) {
               s_worldMapDstState = STATE_26;
               gState.scene = 92;
               obj->state = 99;
               obj->state2 = 0;
            }
            break;
         }

         break;
      }

      if (obj->state2 != 0) {
         switch (gState.worldMapDestination) {
         case 0:
            markerX = 37;
            markerY = 147;
            break;
         case 1:
            markerX = 39;
            markerY = 102;
            break;
         case 2:
            markerX = 115;
            markerY = 108;
            break;
         case 3:
            markerX = 201;
            markerY = 118;
            break;
         case 4:
            markerX = 236;
            markerY = 157;
            break;
         case 5:
            markerX = 290;
            markerY = 173;
            break;
         case 6:
            markerX = 223;
            markerY = 206;
            break;
         case 7:
            markerX = 160;
            markerY = 49;
            break;
         case 8:
            markerX = 148;
            markerY = 84;
            break;
         case 9:
            markerX = 181;
            markerY = 99;
            break;
         case 10:
            markerX = 176;
            markerY = 123;
            break;
         case 11:
            markerX = 208;
            markerY = 102;
            break;
         case 12:
            markerX = 120;
            markerY = 156;
            break;
         case 13:
            markerX = 241;
            markerY = 146;
            break;
         case 14:
            markerX = 196;
            markerY = 121;
            break;
         case 15:
            markerX = 153;
            markerY = 110;
            break;
         case 16:
            markerX = 90;
            markerY = 175;
            break;
         case 17:
            markerX = 176;
            markerY = 193;
            break;
         case 18:
            markerX = 146;
            markerY = 155;
            break;
         case 19:
            markerX = 180;
            markerY = 126;
            break;
         case 20:
            markerX = 274;
            markerY = 111;
            break;
         case 21:
            markerX = 246;
            markerY = 72;
            break;
         case 22:
            markerX = 119;
            markerY = 196;
            break;
         case 23:
            markerX = 85;
            markerY = 168;
            break;
         case 24:
            markerX = 158;
            markerY = 126;
            break;
         case 25:
            markerX = 166;
            markerY = 70;
            break;
         case 26:
            markerX = 207;
            markerY = 43;
            break;
         case 27:
            markerX = 89;
            markerY = 79;
            break;
         case 28:
            markerX = 278;
            markerY = 56;
            break;
         case 29:
            markerX = 177;
            markerY = 53;
            break;
         case 30:
            markerX = 131;
            markerY = 101;
            break;
         case 31:
            markerX = 86;
            markerY = 143;
            break;
         case 32:
            markerX = 76;
            markerY = 194;
            break;
         }

         obj_a1 = Obj_GetUnused();
         obj_a1->d.sprite.gfxIdx = GFX_MAP_MARKER;
         obj_a1->d.sprite.otOfs = 50;
         obj_a1->x1.n = markerX - 15;
         obj_a1->x3.n = markerX + 15;
         obj_a1->y1.n = markerY - 15;
         obj_a1->y3.n = markerY + 15;
         AddObjPrim_Gui(gGraphicsPtr->ot, obj_a1);
      }
      break;

   case 99:

      switch (obj->state2) {
      case 0:
         gWindowActiveIdx = 0;
         PerformAudioCommand(AUDIO_CMD_FADE_OUT_32_4);
         FadeOutScreen(2, 6);
         OBJ.timer = 50;
         obj->state2++;

      // fallthrough
      case 1:
         if (--OBJ.timer == 0) {
            gState.primary = s_worldMapDstState;
            gState.secondary = 0;
            gState.state3 = 0;
            gState.state4 = 0;
         }
         break;
      }

      break;
   }
}
