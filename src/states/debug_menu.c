#include "common.h"
#include "object.h"
#include "state.h"
#include "units.h"
#include "window.h"

void Objf414_DebugMenu(Object *obj) {
   s32 i;
#ifdef PC_FEAT
   Object *o;   /* exchange/107: SELECT B spawns the scene selector (case 16 below) */
   extern void ResetStateForNewGame(void);   /* no header carries it; JP calls it implicitly */
#endif

   switch (obj->state) {
   case 0:
      CloseWindow(0x35);
      gWindowChoiceHeight = 17;
      gWindowChoicesTopMargin = 10;
      gWindowActiveIdx = 0x34;
      DrawWindow(0x34, 0, 0, 200, 225, 100, 4, WBS_CROSSED, 9);
#ifdef PC_FEAT
      /* Retail feeds an ASCII literal to the SJIS renderer, which consumes two bytes per glyph and
       * decodes invalid SJIS, so nothing draws (blank on real hardware too). Full-width SJIS
       * renders natively. See docs/gameplay-additions.md, "The debug menu". */
      DrawSjisText(12, 11, 20, 2, 0,
               "\x82\x61\x82\x60\x82\x73\x82\x73\x82\x6b\x82\x64\n\x82\x65\x82\x6e\x82\x6d\x82\x73\x81\x40\x82\x6e\x82\x65\x82\x65\n\x82\x63\x82\x64\x82\x61\x82\x74\x82\x66\x81\x40\x82\x6c\x82\x6e\x82\x63\x82\x64\n\n\x82\x68\x82\x75\x82\x64\x82\x6d\x82\x73\x81\x40\x82\x6c\x82\x60\x82\x6f\n\x82\x66\x82\x60\x82\x6c\x82\x64\x81\x40\x82\x72\x82\x73\x82\x60\x82\x71\x82\x73\n\x82\x74\x82\x6d\x82\x68\x82\x73\x81\x40\x82\x72\x82\x64\x82\x6b\x82\x64\x82\x62\x82\x73\n\n\x82\x72\x82\x64\x82\x6b\x82\x64\x82\x62\x82\x73\x81\x40\x82\x61");
#else
      DrawSjisText(
          12, 11, 20, 2, 0,
          "BATTLE\nFONT OFF\nDEBUG MODE\n\nIVENT MAP\nGAME START\nUNIT SELECT\n\nSELECT B");
#endif
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
#ifdef PC_FEAT
         /* The retail katakana header sits outside the US font's glyph set, so it rendered blank
          * on US hardware too. Full-width TEST instead. */
         DrawSjisText(12, 11, 10, 2, 0,
                      "\x82\x73\x82\x64\x82\x72\x82\x73\n\x82\x50\n\x82\x51\n\x82\x52\n\x82\x53\n\x82\x54\n\x82\x55\n\x82\x56\n\x82\x57");
#else
         DrawSjisText(12, 11, 10, 2, 0,
                      "\x83\x65\x83\x58\x83\x67\x0a\x82\x50\x0a\x82\x51\x0a\x82\x52\x0a\x82\x53\x0a"
                      "\x82\x54\x0a\x82\x55\x0a\x82\x56\x0a\x82\x57");
#endif
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
#ifdef PC_FEAT
         /* Same katakana-outside-font blank as the TEST header: デモ -> full-width DEMO. */
         DrawSjisText(12, 11, 10, 2, 0, "\x82\x54\n\x82\x55\n\x82\x63\x82\x64\x82\x6c\x82\x6e");
#else
         DrawSjisText(12, 11, 10, 2, 0, "\x82\x54\x0a\x82\x55\x0a\x83\x66\x83\x82");
#endif
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

#ifdef PC_FEAT
   /* The states below exist in the JP build's hub but were stripped from US retail, though the
    * hub's own routing still points at them. Ported from jp/src/states/debug_menu.c with the SJIS
    * lists as ASCII; logic unchanged. See docs/gameplay-additions.md, "The debug menu". */
   case 6:   /* FONT OFF: toggle the map-font flag and return to the hub */
      gState.field_0xa7 = 1;
      obj->state = 1;
      break;

   case 7:   /* DEBUG MODE: ON/OFF picker -> gState.debug */
      switch (obj->state2) {
      case 0:
         CloseWindow(0x34);
         DrawWindow(0x34, 0, 0, 64, 54, 250, 10, WBS_CROSSED, 2);
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

   case 10:  /* UNIT SELECT: pick 1-9 (returns to the hub, JP behaviour) */
      switch (obj->state2) {
      case 0:
         CloseWindow(0x34);
         DrawWindow(0x34, 0, 0, 160, 226, 150, 10, WBS_CROSSED, 9);
         DrawSjisText(12, 11, 10, 2, 0, "\x82\x50\n\x82\x51\n\x82\x52\n\x82\x53\n\x82\x54\n\x82\x55\n\x82\x56\n\x82\x57\n\x82\x58");
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
   case 13:  /* IVENT MAP: warp to the event 3D maps (mapNum 44+), pages 1-10/11-20/21 */
      switch (obj->state2) {
      case 0:
         DrawWindow(0x34, 0, 0, 136, 190, 150, 10, WBS_CROSSED, 10);
         DrawSjisText(12, 11, 10, 2, 0, "\x82\x50\n\x82\x51\n\x82\x52\n\x82\x53\n\x82\x54\n\x82\x55\n\x82\x56\n\x82\x57\n\x82\x58\n\x82\x50\x82\x4f");
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
         DrawSjisText(12, 11, 10, 2, 0, "\x82\x50\x82\x50\n\x82\x50\x82\x51\n\x82\x50\x82\x52\n\x82\x50\x82\x53\n\x82\x50\x82\x54\n\x82\x50\x82\x55\n\x82\x50\x82\x56\n\x82\x50\x82\x57\n\x82\x50\x82\x58\n\x82\x51\x82\x4f");
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

   case 15:  /* GAME START: fresh new-game boot through the intro movie */
      ResetStateForNewGame();
      gState.primary = STATE_MOVIE;
      gState.movieIdxToPlay = 0;
      gState.secondary = 0;
      gState.state3 = 0;
      gState.state4 = 0;
      obj->functionIndex = OBJF_NULL;
      break;

   case 16:  /* SELECT B: spawn the scene selector (core/main.c Objf584) */
      obj->functionIndex = OBJF_NULL;
      o = Obj_GetUnused();
      o->functionIndex = 584;
      break;
#endif /* PC_FEAT */
   }
}
