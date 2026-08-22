#include "common.h"
#include "state.h"
#include "object.h"
#include "units.h"
#include "window.h"
#include "graphics.h"
#include "glyphs.h"
#include "field.h"
#include "audio.h"
#include "battle.h"

#ifdef PC_FEAT
/* Stage-3 1.3 (GAP 3): in Tactical, the promoted Vandal Heart no longer opens the all-spells/all-items
 * "god" spell menu -- the 5 guards below neutralise the `weapon == V_HEART_2` term so Ash falls to his
 * real gSpellLists kit. Normal mode keeps the retail god-path. pc_balance.c owns gTacticalMode. */
extern int gTacticalMode;
#endif


s32 WindowIsOffScreen(Object *);
void DrawSmallEquipmentWindow(u8);
void DrawWindow(s16, s16, s16, s16, s16, s16, s16, u8, u8);
s32 FUN_8001d384(u8 *, u8 *);
s32 FUN_8001d3fc(u8 *, u8 *);
void UpdateSkillStatusWindow(UnitStatus *);
void Objf574_DisplayIcon(Object *);
void ClearIcons(void);
void UpdateCompactUnitInfoWindow(UnitStatus *, UnitStatus *, u8);
void UpdateUnitInfoWindow(UnitStatus *);
void Objf002_MenuChoice(Object *);
void DisplayBasicWindow(s32);
void DisplayBasicWindowWithSetChoice(s32, s32);
void DisplayCustomWindow(s32, u8, u8, u8, u8, u8);
void DisplayCustomWindowWithSetChoice(s32, u8, u8, u8, u8, u8, u8);
void CloseWindow(s32);
s32 GetWindowChoice(s32);
s32 GetWindowChoice2(s32);
void SlideWindowTo(s32, s16, s16);
void Objf004_005_408_Window(Object *);
void DrawGlyphStrip(u8 *, s16, s16, u8);
void ClearGlyphStripBottom(u8 *, s16, s16);
void DrawGlyphStripGroup(u8 *, s16);
u16 DrawEmbossedSjisGlyph(u16, s32, s32, s32, s32);
void Objf422_LowerMsgBoxTail(Object *);
void Objf421_UpperMsgBoxTail(Object *);
void Objf573_BattleItemsList(Object *);
void Objf031_BattleSpellsList(Object *);

s32 WindowIsOffScreen(Object *obj) {
   if (obj->x1.n < 0)
      return 1;
   if (obj->x1.n > SCREEN_WIDTH)
      return 1;
   if (obj->y1.n < 0)
      return 1;
   if (obj->y1.n > SCREEN_HEIGHT)
      return 1;
   return 0;
}

void DrawSmallEquipmentWindow(u8 partyIdx) {
   s32 i;
   if ((partyIdx != 0) && (partyIdx < PARTY_CT)) {
      for (i = 2; i < 12; i++) {
         gGlyphStrip_02[i] = GLYPH_BG;
         gGlyphStrip_03[i] = GLYPH_BG;
         gGlyphStrip_04[i] = GLYPH_BG;
         gGlyphStrip_05[i] = GLYPH_BG;
         gGlyphStrip_06[i] = GLYPH_BG;
         gGlyphStrip_07[i] = GLYPH_BG;
         gGlyphStrip_08[i] = GLYPH_BG;
         gGlyphStrip_09[i] = GLYPH_BG;
         gGlyphStrip_0A[i] = GLYPH_BG;
         gGlyphStrip_0B[i] = GLYPH_BG;
      }
      gGlyphStrip_03[2] = GLYPH_WEAPON;
      gGlyphStrip_03[3] = GLYPH_CHAR_COLON;
      gGlyphStrip_05[2] = GLYPH_HELMET;
      gGlyphStrip_05[3] = GLYPH_CHAR_COLON;
      gGlyphStrip_07[2] = GLYPH_ARMOR;
      gGlyphStrip_07[3] = GLYPH_CHAR_COLON;
      // Each kana item name spans two strip rows: upper half on the even strip,
      // lower half on the odd strip (12px SJIS glyphs across 2 rows).
      FUN_8001d3fc(&gGlyphStrip_02[4], gItemNames[gPartyMembers[partyIdx].weapon]);
      FUN_8001d384(&gGlyphStrip_03[4], gItemNames[gPartyMembers[partyIdx].weapon]);
      FUN_8001d3fc(&gGlyphStrip_04[4], gItemNames[gPartyMembers[partyIdx].helmet]);
      FUN_8001d384(&gGlyphStrip_05[4], gItemNames[gPartyMembers[partyIdx].helmet]);
      FUN_8001d3fc(&gGlyphStrip_06[4], gItemNames[gPartyMembers[partyIdx].armor]);
      FUN_8001d384(&gGlyphStrip_07[4], gItemNames[gPartyMembers[partyIdx].armor]);
      FUN_8001d3fc(&gGlyphStrip_08[4], gItemNames[gPartyMembers[partyIdx].items[0]]);
      FUN_8001d384(&gGlyphStrip_09[4], gItemNames[gPartyMembers[partyIdx].items[0]]);
      FUN_8001d3fc(&gGlyphStrip_0A[4], gItemNames[gPartyMembers[partyIdx].items[1]]);
      FUN_8001d384(&gGlyphStrip_0B[4], gItemNames[gPartyMembers[partyIdx].items[1]]);
      DrawGlyphStripGroup(gGlyphStripGroups[1], GFX_WINDOW_TBD_658);
   }
}

void DrawWindow(s16 windowId, s16 x, s16 y, s16 width, s16 height, s16 dispX, s16 dispY,
                u8 borderStyle, u8 choices) {
   s32 usingMultipleTPages;
   s32 type; // TBD
   s16 yPadding;
   s32 maxColumnsPerTPage;
   s32 i;

   usingMultipleTPages = 0;
   gWindowChoicesCount = choices;
   type = 0;

   if (x >= 1024) {
      x -= 1024;
      if (x >= 256) {
         x -= 256;
         type = 3;
      } else {
         type = 4;
      }
   }
   if (x >= 256) {
      x -= 256;
      type = 1;
   }

   width = width >> 3;
   yPadding = height % 9;
   height = height / 9;

   if (borderStyle == WBS_DRAGON) {
      maxColumnsPerTPage = 32;
      if (width >= 32) {
         usingMultipleTPages++;
      }
   } else {
      maxColumnsPerTPage = 31;
      if (width >= 31) {
         usingMultipleTPages++;
      }
   }

   if (!usingMultipleTPages) {
      if (borderStyle != WBS_DRAGON) {
         if (borderStyle == WBS_CROSSED) {
            // WBS_CROSSED:

            gGlyphStrip_C8[1] = GLYPH_CROSSED_BORDER_TOP_LEFT;
            for (i = 2; i < width; i++) {
               gGlyphStrip_C8[i] = GLYPH_CROSSED_BORDER_TOP;
            }
            gGlyphStrip_C8[i] = GLYPH_CROSSED_BORDER_TOP_RIGHT;
            gGlyphStrip_C8[i + 1] = GLYPH_TERMINATOR;

            gGlyphStrip_C9[1] = GLYPH_CROSSED_BORDER_LEFT;
            for (i = 2; i < width; i++) {
               gGlyphStrip_C9[i] = GLYPH_BG;
            }
            gGlyphStrip_C9[i] = GLYPH_CROSSED_BORDER_RIGHT;
            gGlyphStrip_C9[i + 1] = GLYPH_TERMINATOR;

            gGlyphStrip_CA[1] = GLYPH_CROSSED_BORDER_BOTTOM_LEFT;
            for (i = 2; i < width; i++) {
               gGlyphStrip_CA[i] = GLYPH_CROSSED_BORDER_BOTTOM;
            }
            gGlyphStrip_CA[i] = GLYPH_CROSSED_BORDER_BOTTOM_RIGHT;
            gGlyphStrip_CA[i + 1] = GLYPH_TERMINATOR;
         } else {
            // WBS_ROUNDED:

            gGlyphStrip_C8[1] = GLYPH_ROUNDED_BORDER_TOP_LEFT;
            for (i = 2; i < width; i++) {
               gGlyphStrip_C8[i] = GLYPH_ROUNDED_BORDER_TOP;
            }
            gGlyphStrip_C8[i] = GLYPH_ROUNDED_BORDER_TOP_RIGHT;
            gGlyphStrip_C8[i + 1] = GLYPH_TERMINATOR;

            gGlyphStrip_C9[1] = GLYPH_ROUNDED_BORDER_LEFT;
            for (i = 2; i < width; i++) {
               gGlyphStrip_C9[i] = GLYPH_BG;
            }
            gGlyphStrip_C9[i] = GLYPH_ROUNDED_BORDER_RIGHT;
            gGlyphStrip_C9[i + 1] = GLYPH_TERMINATOR;

            gGlyphStrip_CA[1] = GLYPH_ROUNDED_BORDER_BOTTOM_LEFT;
            for (i = 2; i < width; i++) {
               gGlyphStrip_CA[i] = GLYPH_ROUNDED_BORDER_BOTTOM;
            }
            gGlyphStrip_CA[i] = GLYPH_ROUNDED_BORDER_BOTTOM_RIGHT;
            gGlyphStrip_CA[i + 1] = GLYPH_TERMINATOR;
         }

         gGlyphStripGroup_800f0bd4[0] = 0;
         gGlyphStripGroup_800f0bd4[1] = 0xc8;
         for (i = 2; i < height; i++) {
            gGlyphStripGroup_800f0bd4[i] = 0xc9;
         }
         gGlyphStripGroup_800f0bd4[i++] = 0xca;
         gGlyphStripGroup_800f0bd4[i] = 0;
      } else {
         // WBS_DRAGON:

         gGlyphStrip_C8[1] = GLYPH_LDRAGON_TOP_1;
         gGlyphStrip_C8[2] = GLYPH_LDRAGON_TOP_2;
         gGlyphStrip_C8[3] = GLYPH_LDRAGON_TOP_3;
         for (i = 4; i < width + 1; i++) {
            gGlyphStrip_C8[i] = GLYPH_BLANK;
         }
         gGlyphStrip_C8[i] = GLYPH_TERMINATOR;

         gGlyphStrip_C9[1] = GLYPH_LDRAGON_MID_1;
         gGlyphStrip_C9[2] = GLYPH_LDRAGON_MID_2;
         gGlyphStrip_C9[3] = GLYPH_LDRAGON_MID_3;
         for (i = 4; i < width - 1; i++) {
            gGlyphStrip_C9[i] = GLYPH_CROSSED_BORDER_TOP;
         }
         gGlyphStrip_C9[i] = GLYPH_CROSSED_BORDER_TOP_RIGHT;
         gGlyphStrip_C9[i + 1] = GLYPH_BLANK;
         gGlyphStrip_C9[i + 2] = GLYPH_TERMINATOR;

         gGlyphStrip_CA[1] = GLYPH_BLANK;
         gGlyphStrip_CA[2] = GLYPH_LDRAGON_TAIL;
         for (i = 3; i < width - 1; i++) {
            gGlyphStrip_CA[i] = GLYPH_BG;
         }
         gGlyphStrip_CA[i] = GLYPH_CROSSED_BORDER_RIGHT;
         gGlyphStrip_CA[i + 1] = GLYPH_BLANK;
         gGlyphStrip_CA[i + 2] = GLYPH_TERMINATOR;
         if (height == 6) {
            gGlyphStrip_CA[i] = GLYPH_RDRAGON_TAIL_TIP;
         }

         gGlyphStrip_CB[1] = GLYPH_BLANK;
         gGlyphStrip_CB[2] = GLYPH_LDRAGON_TAIL_TIP;
         for (i = 3; i < width - 1; i++) {
            gGlyphStrip_CB[i] = GLYPH_BG;
         }
         gGlyphStrip_CB[i] = GLYPH_CROSSED_BORDER_RIGHT;
         gGlyphStrip_CB[i + 1] = GLYPH_BLANK;
         gGlyphStrip_CB[i + 2] = GLYPH_TERMINATOR;
         if (height == 6) {
            gGlyphStrip_CB[i] = GLYPH_RDRAGON_TAIL;
         }
         if (height == 7) {
            gGlyphStrip_CB[i] = GLYPH_RDRAGON_TAIL_TIP;
         }

         gGlyphStrip_CC[1] = GLYPH_BLANK;
         gGlyphStrip_CC[2] = GLYPH_CROSSED_BORDER_LEFT;
         for (i = 3; i < width - 1; i++) {
            gGlyphStrip_CC[i] = GLYPH_BG;
         }
         gGlyphStrip_CC[i] = GLYPH_CROSSED_BORDER_RIGHT;
         gGlyphStrip_CC[i + 1] = GLYPH_BLANK;
         gGlyphStrip_CC[i + 2] = GLYPH_TERMINATOR;

         gGlyphStrip_CD[1] = GLYPH_BLANK;
         gGlyphStrip_CD[2] = GLYPH_CROSSED_BORDER_LEFT;
         for (i = 3; i < width - 1; i++) {
            gGlyphStrip_CD[i] = GLYPH_BG;
         }
         gGlyphStrip_CD[i] = GLYPH_RDRAGON_TAIL_TIP;
         gGlyphStrip_CD[i + 1] = GLYPH_BLANK;
         gGlyphStrip_CD[i + 2] = GLYPH_TERMINATOR;

         gGlyphStrip_CE[1] = GLYPH_BLANK;
         gGlyphStrip_CE[2] = GLYPH_CROSSED_BORDER_LEFT;
         for (i = 3; i < width - 1; i++) {
            gGlyphStrip_CE[i] = GLYPH_BG;
         }
         gGlyphStrip_CE[i] = GLYPH_RDRAGON_TAIL;
         gGlyphStrip_CE[i + 1] = GLYPH_BLANK;
         gGlyphStrip_CE[i + 2] = GLYPH_TERMINATOR;

         gGlyphStrip_CF[1] = GLYPH_BLANK;
         gGlyphStrip_CF[2] = GLYPH_CROSSED_BORDER_BOTTOM_LEFT;
         for (i = 3; i < width - 2; i++) {
            gGlyphStrip_CF[i] = GLYPH_CROSSED_BORDER_BOTTOM;
         }
         gGlyphStrip_CF[i] = GLYPH_RDRAGON_MID_1;
         gGlyphStrip_CF[i + 1] = GLYPH_RDRAGON_MID_2;
         gGlyphStrip_CF[i + 2] = GLYPH_RDRAGON_MID_3;
         gGlyphStrip_CF[i + 3] = GLYPH_TERMINATOR;

         for (i = 0; i < width - 2; i++) {
            gGlyphStrip_D0[i] = GLYPH_BLANK;
         }
         gGlyphStrip_D0[i] = GLYPH_RDRAGON_BOTTOM_1;
         gGlyphStrip_D0[i + 1] = GLYPH_RDRAGON_BOTTOM_2;
         gGlyphStrip_D0[i + 2] = GLYPH_RDRAGON_BOTTOM_3;
         gGlyphStrip_D0[i + 3] = GLYPH_TERMINATOR;

         i = 0;
         gGlyphStripGroup_800f0bd4[i++] = 0;
         gGlyphStripGroup_800f0bd4[i++] = 0xc8;
         gGlyphStripGroup_800f0bd4[i++] = 0xc9;
         gGlyphStripGroup_800f0bd4[i++] = 0xca;
         gGlyphStripGroup_800f0bd4[i++] = 0xcb;
         if (height > 8) {
            for (i = 5; i < height - 3; i++) {
               gGlyphStripGroup_800f0bd4[i] = 0xcc;
            }
         }
         if (height > 7) {
            gGlyphStripGroup_800f0bd4[i++] = 0xcd;
         }
         if (height > 6) {
            gGlyphStripGroup_800f0bd4[i++] = 0xce;
         }
         gGlyphStripGroup_800f0bd4[i++] = 0xcf;
         gGlyphStripGroup_800f0bd4[i++] = 0xd0;
         gGlyphStripGroup_800f0bd4[i] = 0;
      }

      gGfxSubTextures[GFX_WINDOW_TBD_657 + windowId][0] = x;
      gGfxSubTextures[GFX_WINDOW_TBD_657 + windowId][1] = y;
      gGfxSubTextures[GFX_WINDOW_TBD_657 + windowId][2] = width * 8;
      gGfxSubTextures[GFX_WINDOW_TBD_657 + windowId][3] = yPadding + height * 9;
      gGfxSubTextures[GFX_WINDOW_TBD_707][0] = x;
      gGfxSubTextures[GFX_WINDOW_TBD_707][1] = y;
   } else {
      // usingMultipleTPages:
      width++;
      width = width - maxColumnsPerTPage;

      if (borderStyle != WBS_DRAGON) {
         if (borderStyle == WBS_CROSSED) {
            // WBS_CROSSED:

            for (i = 2; i < maxColumnsPerTPage; i++) {
               gGlyphStrip_C8[i] = GLYPH_CROSSED_BORDER_TOP;
               gGlyphStrip_C9[i] = GLYPH_BG;
               gGlyphStrip_CA[i] = GLYPH_CROSSED_BORDER_BOTTOM;
            }

            gGlyphStrip_C8[1] = GLYPH_CROSSED_BORDER_TOP_LEFT;
            gGlyphStrip_C8[i] = GLYPH_TERMINATOR;

            gGlyphStrip_C9[1] = GLYPH_CROSSED_BORDER_LEFT;
            gGlyphStrip_C9[i] = GLYPH_TERMINATOR;

            gGlyphStrip_CA[1] = GLYPH_CROSSED_BORDER_BOTTOM_LEFT;
            gGlyphStrip_CA[i] = GLYPH_TERMINATOR;

            if (width > 1) {
               for (i = 1; i < width; i++) {
                  gGlyphStrip_CB[i] = GLYPH_CROSSED_BORDER_TOP;
                  gGlyphStrip_CC[i] = GLYPH_BG;
                  gGlyphStrip_CD[i] = GLYPH_CROSSED_BORDER_BOTTOM;
               }
            }

            gGlyphStrip_CB[i] = GLYPH_CROSSED_BORDER_TOP_RIGHT;
            gGlyphStrip_CB[i + 1] = GLYPH_TERMINATOR;

            gGlyphStrip_CC[i] = GLYPH_CROSSED_BORDER_RIGHT;
            gGlyphStrip_CC[i + 1] = GLYPH_TERMINATOR;

            gGlyphStrip_CD[i] = GLYPH_CROSSED_BORDER_BOTTOM_RIGHT;
            gGlyphStrip_CD[i + 1] = GLYPH_TERMINATOR;
         } else {
            // WBS_ROUNDED:

            for (i = 2; i < maxColumnsPerTPage; i++) {
               gGlyphStrip_C8[i] = GLYPH_ROUNDED_BORDER_TOP;
               gGlyphStrip_C9[i] = GLYPH_BG;
               gGlyphStrip_CA[i] = GLYPH_ROUNDED_BORDER_BOTTOM;
            }

            gGlyphStrip_C8[1] = GLYPH_ROUNDED_BORDER_TOP_LEFT;
            gGlyphStrip_C8[i] = GLYPH_TERMINATOR;

            gGlyphStrip_C9[1] = GLYPH_ROUNDED_BORDER_LEFT;
            gGlyphStrip_C9[i] = GLYPH_TERMINATOR;

            gGlyphStrip_CA[1] = GLYPH_ROUNDED_BORDER_BOTTOM_LEFT;
            gGlyphStrip_CA[i] = GLYPH_TERMINATOR;

            if (width > 1) {
               for (i = 1; i < width; i++) {
                  gGlyphStrip_CB[i] = GLYPH_ROUNDED_BORDER_TOP;
                  gGlyphStrip_CC[i] = GLYPH_BG;
                  gGlyphStrip_CD[i] = GLYPH_ROUNDED_BORDER_BOTTOM;
               }
            }

            gGlyphStrip_CB[i] = GLYPH_ROUNDED_BORDER_TOP_RIGHT;
            gGlyphStrip_CB[i + 1] = GLYPH_TERMINATOR;

            gGlyphStrip_CC[i] = GLYPH_ROUNDED_BORDER_RIGHT;
            gGlyphStrip_CC[i + 1] = GLYPH_TERMINATOR;

            gGlyphStrip_CD[i] = GLYPH_ROUNDED_BORDER_BOTTOM_RIGHT;
            gGlyphStrip_CD[i + 1] = GLYPH_TERMINATOR;
         }

         gGlyphStripGroup_800f0bd4[0] = 0;
         gGlyphStripGroup_800f0bd4[1] = 0xc8;
         for (i = 2; i < height; i++) {
            gGlyphStripGroup_800f0bd4[i] = 0xc9;
         }
         gGlyphStripGroup_800f0bd4[i++] = 0xca;
         gGlyphStripGroup_800f0bd4[i] = 0;

         gGlyphStripGroup_800f0bf0[0] = 0;
         gGlyphStripGroup_800f0bf0[1] = 0xcb;
         for (i = 2; i < height; i++) {
            gGlyphStripGroup_800f0bf0[i] = 0xcc;
         }
         gGlyphStripGroup_800f0bf0[i++] = 0xcd;
         gGlyphStripGroup_800f0bf0[i++] = 0;

      } else {
         // WBS_DRAGON:

         gGlyphStrip_C8[1] = GLYPH_LDRAGON_TOP_1;
         gGlyphStrip_C8[2] = GLYPH_LDRAGON_TOP_2;
         gGlyphStrip_C8[3] = GLYPH_LDRAGON_TOP_3;
         for (i = 4; i < 32; i++) {
            gGlyphStrip_C8[i] = GLYPH_BLANK;
         }
         gGlyphStrip_C8[i] = GLYPH_TERMINATOR;

         gGlyphStrip_C9[1] = GLYPH_LDRAGON_MID_1;
         gGlyphStrip_C9[2] = GLYPH_LDRAGON_MID_2;
         gGlyphStrip_C9[3] = GLYPH_LDRAGON_MID_3;
         if (width == 1) {
            for (i = 4; i < 31; i++) {
               gGlyphStrip_C9[i] = GLYPH_CROSSED_BORDER_TOP;
            }
            gGlyphStrip_C9[i++] = GLYPH_CROSSED_BORDER_TOP_RIGHT;
         } else {
            for (i = 4; i < 32; i++) {
               gGlyphStrip_C9[i] = GLYPH_CROSSED_BORDER_TOP;
            }
         }
         gGlyphStrip_C9[i] = GLYPH_TERMINATOR;

         gGlyphStrip_CA[1] = GLYPH_BLANK;
         gGlyphStrip_CA[2] = GLYPH_LDRAGON_TAIL;
         if (width == 1) {
            for (i = 3; i < 31; i++) {
               gGlyphStrip_CA[i] = GLYPH_BG;
            }
            gGlyphStrip_CA[i++] = GLYPH_CROSSED_BORDER_RIGHT;
         } else {
            for (i = 3; i < 32; i++) {
               gGlyphStrip_CA[i] = GLYPH_BG;
            }
         }
         gGlyphStrip_CA[i] = GLYPH_TERMINATOR;

         gGlyphStrip_CB[1] = GLYPH_BLANK;
         gGlyphStrip_CB[2] = GLYPH_LDRAGON_TAIL_TIP;
         if (width == 1) {
            for (i = 3; i < 31; i++) {
               gGlyphStrip_CB[i] = GLYPH_BG;
            }
            gGlyphStrip_CB[i++] = GLYPH_CROSSED_BORDER_RIGHT;
         } else {
            for (i = 3; i < 32; i++) {
               gGlyphStrip_CB[i] = GLYPH_BG;
            }
         }
         gGlyphStrip_CB[i] = GLYPH_TERMINATOR;

         gGlyphStrip_CC[1] = GLYPH_BLANK;
         gGlyphStrip_CC[2] = GLYPH_CROSSED_BORDER_LEFT;
         if (width == 1) {
            for (i = 3; i < 31; i++) {
               gGlyphStrip_CC[i] = GLYPH_BG;
            }
            gGlyphStrip_CC[i++] = GLYPH_CROSSED_BORDER_RIGHT;
         } else {
            for (i = 3; i < 32; i++) {
               gGlyphStrip_CC[i] = GLYPH_BG;
            }
         }
         gGlyphStrip_CC[i] = GLYPH_TERMINATOR;

         gGlyphStrip_CD[1] = GLYPH_BLANK;
         gGlyphStrip_CD[2] = GLYPH_CROSSED_BORDER_LEFT;
         if (width == 1) {
            for (i = 3; i < 31; i++) {
               gGlyphStrip_CD[i] = GLYPH_BG;
            }
            gGlyphStrip_CD[i++] = GLYPH_RDRAGON_TAIL_TIP;
         } else {
            for (i = 3; i < 32; i++) {
               gGlyphStrip_CD[i] = GLYPH_BG;
            }
         }
         gGlyphStrip_CD[i] = GLYPH_TERMINATOR;

         gGlyphStrip_CE[1] = GLYPH_BLANK;
         gGlyphStrip_CE[2] = GLYPH_CROSSED_BORDER_LEFT;
         if (width == 1) {
            for (i = 3; i < 31; i++) {
               gGlyphStrip_CE[i] = GLYPH_BG;
            }
            gGlyphStrip_CE[i++] = GLYPH_RDRAGON_TAIL;
         } else {
            for (i = 3; i < 32; i++) {
               gGlyphStrip_CE[i] = GLYPH_BG;
            }
         }
         gGlyphStrip_CE[i] = GLYPH_TERMINATOR;

         gGlyphStrip_CF[1] = GLYPH_BLANK;
         gGlyphStrip_CF[2] = GLYPH_CROSSED_BORDER_BOTTOM_LEFT;
         if (width == 1) {
            for (i = 3; i < 30; i++) {
               gGlyphStrip_CF[i] = GLYPH_CROSSED_BORDER_BOTTOM;
            }
            gGlyphStrip_CF[i++] = GLYPH_RDRAGON_MID_1;
            gGlyphStrip_CF[i++] = GLYPH_RDRAGON_MID_2;
         } else if (width == 2) {
            for (i = 3; i < 31; i++) {
               gGlyphStrip_CF[i] = GLYPH_CROSSED_BORDER_BOTTOM;
            }
            gGlyphStrip_CF[i++] = GLYPH_RDRAGON_MID_1;
         } else {
            for (i = 3; i < 32; i++) {
               gGlyphStrip_CF[i] = GLYPH_CROSSED_BORDER_BOTTOM;
            }
         }
         gGlyphStrip_CF[i] = GLYPH_TERMINATOR;

         if (width == 1) {
            for (i = 1; i < 30; i++) {
               gGlyphStrip_D0[i] = GLYPH_BLANK;
            }
            gGlyphStrip_D0[i++] = GLYPH_RDRAGON_BOTTOM_1;
            gGlyphStrip_D0[i++] = GLYPH_RDRAGON_BOTTOM_2;
         } else if (width == 2) {
            for (i = 1; i < 31; i++) {
               gGlyphStrip_D0[i] = GLYPH_BLANK;
            }
            gGlyphStrip_D0[i++] = GLYPH_RDRAGON_BOTTOM_1;
         } else {
            for (i = 1; i < 32; i++) {
               gGlyphStrip_D0[i] = GLYPH_BLANK;
            }
         }
         gGlyphStrip_D0[i] = GLYPH_TERMINATOR;

         for (i = 1; i < width + 1; i++) {
            gGlyphStrip_D1[i] = GLYPH_BLANK;
         }
         gGlyphStrip_D1[i] = GLYPH_TERMINATOR;

         i = 1;
         if (width == 1) {
            gGlyphStrip_D2[i++] = GLYPH_BLANK;
         } else if (width == 2) {
            gGlyphStrip_D2[i++] = GLYPH_CROSSED_BORDER_TOP_RIGHT;
            gGlyphStrip_D2[i++] = GLYPH_BLANK;
         } else {
            for (i = 1; i < width - 1; i++) {
               gGlyphStrip_D2[i] = GLYPH_CROSSED_BORDER_TOP;
            }
            gGlyphStrip_D2[i++] = GLYPH_CROSSED_BORDER_TOP_RIGHT;
            gGlyphStrip_D2[i++] = GLYPH_BLANK;
         }
         gGlyphStrip_D2[i] = GLYPH_TERMINATOR;

         i = 1;
         if (width == 1) {
            gGlyphStrip_D3[i++] = GLYPH_BLANK;
         } else if (width == 2) {
            gGlyphStrip_D3[i++] = GLYPH_CROSSED_BORDER_RIGHT;
            gGlyphStrip_D3[i++] = GLYPH_BLANK;
         } else {
            for (i = 1; i < width - 1; i++) {
               gGlyphStrip_D3[i] = GLYPH_BG;
            }
            gGlyphStrip_D3[i++] = GLYPH_CROSSED_BORDER_RIGHT;
            gGlyphStrip_D3[i++] = GLYPH_BLANK;
         }
         gGlyphStrip_D3[i] = GLYPH_TERMINATOR;

         i = 1;
         if (width == 1) {
            gGlyphStrip_D4[i++] = GLYPH_BLANK;
         } else if (width == 2) {
            gGlyphStrip_D4[i++] = GLYPH_RDRAGON_TAIL_TIP;
            gGlyphStrip_D4[i++] = GLYPH_BLANK;
         } else {
            for (i = 1; i < width - 1; i++) {
               gGlyphStrip_D4[i] = GLYPH_BG;
            }
            gGlyphStrip_D4[i++] = GLYPH_RDRAGON_TAIL_TIP;
            gGlyphStrip_D4[i++] = GLYPH_BLANK;
         }
         gGlyphStrip_D4[i] = GLYPH_TERMINATOR;

         i = 1;
         if (width == 1) {
            gGlyphStrip_D5[i++] = GLYPH_BLANK;
         } else if (width == 2) {
            gGlyphStrip_D5[i++] = GLYPH_RDRAGON_TAIL;
            gGlyphStrip_D5[i++] = GLYPH_BLANK;
         } else {
            for (i = 1; i < width - 1; i++) {
               gGlyphStrip_D5[i] = GLYPH_BG;
            }
            gGlyphStrip_D5[i++] = GLYPH_RDRAGON_TAIL;
            gGlyphStrip_D5[i++] = GLYPH_BLANK;
         }
         gGlyphStrip_D5[i] = GLYPH_TERMINATOR;

         i = 1;
         if (width == 1) {
            gGlyphStrip_D6[i++] = GLYPH_RDRAGON_MID_3;
         } else if (width == 2) {
            gGlyphStrip_D6[i++] = GLYPH_RDRAGON_MID_2;
            gGlyphStrip_D6[i++] = GLYPH_RDRAGON_MID_3;
         } else if (width == 3) {
            gGlyphStrip_D6[i++] = GLYPH_RDRAGON_MID_1;
            gGlyphStrip_D6[i++] = GLYPH_RDRAGON_MID_2;
            gGlyphStrip_D6[i++] = GLYPH_RDRAGON_MID_3;
         } else {
            for (i = 1; i < width - 2; i++) {
               gGlyphStrip_D6[i] = GLYPH_CROSSED_BORDER_BOTTOM;
            }
            gGlyphStrip_D6[i++] = GLYPH_RDRAGON_MID_1;
            gGlyphStrip_D6[i++] = GLYPH_RDRAGON_MID_2;
            gGlyphStrip_D6[i++] = GLYPH_RDRAGON_MID_3;
         }
         gGlyphStrip_D6[i] = GLYPH_TERMINATOR;

         i = 1;
         if (width == 1) {
            gGlyphStrip_D7[i++] = GLYPH_RDRAGON_BOTTOM_3;
         } else if (width == 2) {
            gGlyphStrip_D7[i++] = GLYPH_RDRAGON_BOTTOM_2;
            gGlyphStrip_D7[i++] = GLYPH_RDRAGON_BOTTOM_3;
         } else if (width == 3) {
            gGlyphStrip_D7[i++] = GLYPH_RDRAGON_BOTTOM_1;
            gGlyphStrip_D7[i++] = GLYPH_RDRAGON_BOTTOM_2;
            gGlyphStrip_D7[i++] = GLYPH_RDRAGON_BOTTOM_3;
         } else {
            for (i = 1; i < width - 2; i++) {
               gGlyphStrip_D7[i] = GLYPH_BLANK;
            }
            gGlyphStrip_D7[i++] = GLYPH_RDRAGON_BOTTOM_1;
            gGlyphStrip_D7[i++] = GLYPH_RDRAGON_BOTTOM_2;
            gGlyphStrip_D7[i++] = GLYPH_RDRAGON_BOTTOM_3;
         }
         gGlyphStrip_D7[i] = GLYPH_TERMINATOR;

         gGlyphStripGroup_800f0bd4[0] = 0;
         gGlyphStripGroup_800f0bd4[1] = 0xc8;
         gGlyphStripGroup_800f0bd4[2] = 0xc9;
         gGlyphStripGroup_800f0bd4[3] = 0xca;
         gGlyphStripGroup_800f0bd4[4] = 0xcb;

         for (i = 5; i < height - 3; i++) {
            gGlyphStripGroup_800f0bd4[i] = 0xcc;
         }

         gGlyphStripGroup_800f0bd4[i] = 0xcd;
         gGlyphStripGroup_800f0bd4[i + 1] = 0xce;
         gGlyphStripGroup_800f0bd4[i + 2] = 0xcf;
         gGlyphStripGroup_800f0bd4[i + 3] = 0xd0;
         gGlyphStripGroup_800f0bd4[i + 4] = 0;

         gGlyphStripGroup_800f0bf0[0] = 0;
         gGlyphStripGroup_800f0bf0[1] = 0xd1;
         gGlyphStripGroup_800f0bf0[2] = 0xd2;

         for (i = 3; i < height - 3; i++) {
            gGlyphStripGroup_800f0bf0[i] = 0xd3;
         }

         gGlyphStripGroup_800f0bf0[i] = 0xd4;
         gGlyphStripGroup_800f0bf0[i + 1] = 0xd5;
         gGlyphStripGroup_800f0bf0[i + 2] = 0xd6;
         gGlyphStripGroup_800f0bf0[i + 3] = 0xd7;
         gGlyphStripGroup_800f0bf0[i + 4] = 0;
      }

      gGfxSubTextures[GFX_WINDOW_TBD_657 + windowId][0] = 0;
      gGfxSubTextures[GFX_WINDOW_TBD_657 + windowId][1] = y;
      gGfxSubTextures[GFX_WINDOW_TBD_657 + windowId][2] = (maxColumnsPerTPage - 1) * 8;
      gGfxSubTextures[GFX_WINDOW_TBD_657 + windowId][3] = yPadding + height * 9;

      gGfxSubTextures[GFX_WINDOW_TBD_658 + windowId][0] = 0;
      gGfxSubTextures[GFX_WINDOW_TBD_658 + windowId][1] = y;
      gGfxSubTextures[GFX_WINDOW_TBD_658 + windowId][2] = width * 8;
      gGfxSubTextures[GFX_WINDOW_TBD_658 + windowId][3] = yPadding + height * 9;

      gGfxSubTextures[GFX_WINDOW_TBD_707][0] = 0;
      gGfxSubTextures[GFX_WINDOW_TBD_707][1] = y;
      gGfxSubTextures[GFX_WINDOW_TBD_708][0] = 0;
      gGfxSubTextures[GFX_WINDOW_TBD_708][1] = y;
   }

   if (usingMultipleTPages) {
      if (type == 0) {
         DrawGlyphStripGroup(gGlyphStripGroups[50], GFX_WINDOW_TBD_707);
         DrawGlyphStripGroup(gGlyphStripGroups[51], GFX_WINDOW_TBD_708);

         gGfxTPageCells[GFX_WINDOW_TBD_657 + windowId] = 8;
         gGfxTPageIds[GFX_WINDOW_TBD_657 + windowId] = gTPageIds[8];
         gGfxTPageCells[GFX_WINDOW_TBD_658 + windowId] = 9;
         gGfxTPageIds[GFX_WINDOW_TBD_658 + windowId] = gTPageIds[9];
      } else {
         gGfxTPageCells[GFX_WINDOW_TBD_707] = 14;
         gGfxTPageIds[GFX_WINDOW_TBD_707] = gTPageIds[14];
         gGfxTPageCells[GFX_WINDOW_TBD_708] = 15;
         gGfxTPageIds[GFX_WINDOW_TBD_708] = gTPageIds[15];

         DrawGlyphStripGroup(gGlyphStripGroups[50], GFX_WINDOW_TBD_707);
         DrawGlyphStripGroup(gGlyphStripGroups[51], GFX_WINDOW_TBD_708);

         gGfxTPageCells[GFX_WINDOW_TBD_707] = 8;
         gGfxTPageIds[GFX_WINDOW_TBD_707] = gTPageIds[8];
         gGfxTPageCells[GFX_WINDOW_TBD_708] = 9;
         gGfxTPageIds[GFX_WINDOW_TBD_708] = gTPageIds[9];

         gGfxTPageCells[GFX_WINDOW_TBD_657 + windowId] = 8;
         gGfxTPageIds[GFX_WINDOW_TBD_657 + windowId] = gTPageIds[8];
         gGfxTPageCells[GFX_WINDOW_TBD_658 + windowId] = 9;
         gGfxTPageIds[GFX_WINDOW_TBD_658 + windowId] = gTPageIds[9];
      }
   } else {
      // !usingMultipleTPages:
      if (type == 0) {
         DrawGlyphStripGroup(gGlyphStripGroups[50], GFX_WINDOW_TBD_707);
         gGfxTPageCells[GFX_WINDOW_TBD_657 + windowId] = 8;
         gGfxTPageIds[GFX_WINDOW_TBD_657 + windowId] = gTPageIds[8];
      } else {
         if (type == 1) {
            gGfxTPageCells[GFX_WINDOW_TBD_707] = 9;
            gGfxTPageIds[GFX_WINDOW_TBD_707] = gTPageIds[9];
            DrawGlyphStripGroup(gGlyphStripGroups[50], GFX_WINDOW_TBD_707);
            gGfxTPageCells[GFX_WINDOW_TBD_707] = 8;
            gGfxTPageIds[GFX_WINDOW_TBD_707] = gTPageIds[8];
            gGfxTPageCells[GFX_WINDOW_TBD_657 + windowId] = 9;
            gGfxTPageIds[GFX_WINDOW_TBD_657 + windowId] = gTPageIds[9];
         } else {
            if (type == 2) {
               gGfxTPageCells[GFX_WINDOW_TBD_707] = 14;
               gGfxTPageIds[GFX_WINDOW_TBD_707] = gTPageIds[14];
               DrawGlyphStripGroup(gGlyphStripGroups[50], GFX_WINDOW_TBD_707);
               gGfxTPageCells[GFX_WINDOW_TBD_707] = 8;
               gGfxTPageIds[GFX_WINDOW_TBD_707] = gTPageIds[8];
               gGfxTPageCells[GFX_WINDOW_TBD_657 + windowId] = 14;
               gGfxTPageIds[GFX_WINDOW_TBD_657 + windowId] = gTPageIds[14];
            } else {
               gGfxTPageCells[GFX_WINDOW_TBD_707] = 15;
               gGfxTPageIds[GFX_WINDOW_TBD_707] = gTPageIds[15];
               DrawGlyphStripGroup(gGlyphStripGroups[50], GFX_WINDOW_TBD_707);
               gGfxTPageCells[GFX_WINDOW_TBD_707] = 8;
               gGfxTPageIds[GFX_WINDOW_TBD_707] = gTPageIds[8];
               gGfxTPageCells[GFX_WINDOW_TBD_657 + windowId] = 15;
               gGfxTPageIds[GFX_WINDOW_TBD_657 + windowId] = gTPageIds[15];
            }
         }
      }
   }
   if (dispX != 0) {
      gWindowDisplayX[windowId] = dispX;
   }
   if (dispY != 0) {
      gWindowDisplayY[windowId] = dispY;
   }
   if (usingMultipleTPages) {
      gWindowDisplayX[windowId + 1] = dispX + maxColumnsPerTPage * 8 - 8;
      gWindowDisplayY[windowId + 1] = dispY;
   }
}


s32 FUN_8001d384(u8 *dest, u8 *src) {
   // TBD: Unused? Maybe left-over kana glyph stuff?
   s32 ct = 0;
   s32 inByte, outByte;

   if (*src == 0xff) {
      return -1;
   }
   while (*src != 0xff) {
      inByte = *src++;
      outByte = inByte;
      if (inByte >= 0x9b) {
         outByte = inByte - 0x89;
         if (inByte >= 0xaa && inByte <= 0xae) {
            outByte = inByte - 0x84;
         }
      }
      if (inByte == 0x9a) {
         outByte = 0xf;
      }
      *dest++ = outByte;
      ct++;
   }
   return ct;
}

s32 FUN_8001d3fc(u8 *dest, u8 *src) {
   // TBD: Unused? Maybe left-over kana glyph stuff?
   s32 ct = 0;
   s32 inByte, outByte;

   if (*src == 0xff) {
      return -1;
   }
   while (*src != 0xff) {
      inByte = *src++;
      outByte = 1;
      if (inByte >= 0x9a) {
         outByte = 0x9a;
         if (inByte >= 0xaa && inByte <= 0xae) {
            outByte = 0x9b;
         }
      }
      *dest++ = outByte;
      ct++;
   }
   return ct;
}

void UpdateSkillStatusWindow(UnitStatus *unit) {
   s32 i;
   s32 a, def;
   Object *icon;

   for (a = 2; a < 23; a++) {
      gGlyphStrip_65[a] = GLYPH_BG;
      gGlyphStrip_66[a] = GLYPH_BG;
      gGlyphStrip_67[a] = GLYPH_BG;
      gGlyphStrip_68[a] = GLYPH_BG;
   }

   // Plot the 2x2 glyphs that make up the class icon
   gGlyphStrip_65[9] = gClassIconStartingGlyph[unit->class];
   gGlyphStrip_65[10] = gClassIconStartingGlyph[unit->class] + 1;
   gGlyphStrip_66[9] = gClassIconStartingGlyph[unit->class] + 2;
   gGlyphStrip_66[10] = gClassIconStartingGlyph[unit->class] + 3;

   // Kana names span two strip rows: upper half / lower half
   FUN_8001d3fc(&gGlyphStrip_67[9], gCharacterNames[unit->name]);
   a = FUN_8001d384(&gGlyphStrip_68[9], gCharacterNames[unit->name]) + 10;
   FUN_8001d3fc(&gGlyphStrip_67[a], gUnitTypeNames[unit->unitType]);
   FUN_8001d384(&gGlyphStrip_68[a], gUnitTypeNames[unit->unitType]);
   FUN_8001d3fc(&gGlyphStrip_5E[19], gItemNames[unit->weapon]);
   FUN_8001d384(&gGlyphStrip_5F[19], gItemNames[unit->weapon]);
   FUN_8001d3fc(&gGlyphStrip_60[19], gItemNames[unit->helmet]);
   FUN_8001d384(&gGlyphStrip_61[19], gItemNames[unit->helmet]);
   FUN_8001d3fc(&gGlyphStrip_62[19], gItemNames[unit->armor]);
   FUN_8001d384(&gGlyphStrip_63[19], gItemNames[unit->armor]);

   gGlyphStrip_5D[6] = GLYPH_BG;
   gGlyphStrip_5D[12] = GLYPH_BG;
   IntToLeftPaddedGlyphs(unit->hp, &gGlyphStrip_5D[6]);
   IntToGlyphs(unit->maxHp, &gGlyphStrip_5D[10]);

   gGlyphStrip_5E[6] = GLYPH_BG;
   gGlyphStrip_5E[11] = GLYPH_BG;
   IntToLeftPaddedGlyphs(unit->mp, &gGlyphStrip_5E[6]);
   IntToGlyphs(unit->maxMp, &gGlyphStrip_5E[10]);

   gGlyphStrip_61[7] = GLYPH_BG;
   gGlyphStrip_61[8] = GLYPH_BG;
   gGlyphStrip_61[9] = GLYPH_BG;

   a = unit->attack;
   if (unit->team == TEAM_PLAYER) {
      a += gItemEquipmentDisplayPower[unit->weapon];
   }
   i = IntToGlyphs(a, &gGlyphStrip_61[6]);
   if (unit->atkBoosted) {
      gGlyphStrip_61[6 + i] = GLYPH_CHAR_PLUS;
   }

   gGlyphStrip_62[7] = GLYPH_BG;
   gGlyphStrip_62[8] = GLYPH_BG;
   gGlyphStrip_62[9] = GLYPH_BG;

   a = unit->defense;
   if (unit->team == TEAM_PLAYER) {
      a += gItemEquipmentDisplayPower[unit->helmet];
      a += gItemEquipmentDisplayPower[unit->armor];
   }
   i = IntToGlyphs(a, &gGlyphStrip_62[6]);
   if (unit->defBoosted) {
      gGlyphStrip_62[6 + i] = GLYPH_CHAR_PLUS;
   }

   gGlyphStrip_63[7] = GLYPH_BG;
   gGlyphStrip_63[8] = GLYPH_BG;
   gGlyphStrip_63[9] = GLYPH_BG;

   i = IntToGlyphs(unit->agility, &gGlyphStrip_63[6]);
   if (unit->aglBoosted) {
      gGlyphStrip_63[6 + i] = GLYPH_CHAR_PLUS;
   }

   gGlyphStrip_5F[7] = GLYPH_BG;
   gGlyphStrip_5F[8] = GLYPH_BG;
   gGlyphStrip_5F[9] = GLYPH_BG;
   IntToGlyphs(unit->exp, &gGlyphStrip_5F[6]);

   gGlyphStrip_60[7] = GLYPH_BG;
   gGlyphStrip_60[8] = GLYPH_BG;

   // exp needed for next level
   IntToGlyphs(100 - unit->exp % 100, &gGlyphStrip_60[6]);

   gGlyphStrip_5C[16] = GLYPH_BG;
   IntToGlyphs(unit->level, &gGlyphStrip_5C[15]);

   icon = Obj_GetLastUnused();
   icon->functionIndex = OBJF_DISPLAY_ICON;
   icon->d.sprite.gfxIdx = GFX_ITEM_ICONS_OFS + unit->weapon;
   icon->x1.n = 141;
   icon->y1.n = 128;

   icon = Obj_GetLastUnused();
   icon->functionIndex = OBJF_DISPLAY_ICON;
   icon->d.sprite.gfxIdx = GFX_ITEM_ICONS_OFS + unit->helmet;
   icon->x1.n = 141;
   icon->y1.n = 146;

   icon = Obj_GetLastUnused();
   icon->functionIndex = OBJF_DISPLAY_ICON;
   icon->d.sprite.gfxIdx = GFX_ITEM_ICONS_OFS + unit->armor;
   icon->x1.n = 141;
   icon->y1.n = 164;
}

void Objf574_DisplayIcon(Object *obj) {
   obj->x3.n = obj->x1.n + 15;
   obj->y3.n = obj->y1.n + 15;
   AddObjPrim_Gui(gGraphicsPtr->ot, obj);
}

void ClearIcons(void) {
   Object *p;
   s32 i;

   p = gObjectArray;

   for (i = 0; i < OBJ_DATA_CT; i++) {
      if (p->functionIndex == OBJF_DISPLAY_ICON) {
         p->functionIndex = OBJF_NULL;
      }
      p++;
   }
}

void UpdateCompactUnitInfoWindow(UnitStatus *unit, UnitStatus *unused, u8 param_3) {
   s32 i, px, full, rem;
   s32 terrainBonus;
   Object *sprite;
   u8 *pTop;
   u8 *pMid;
   u8 *pBtm;

   // ?: 0: bottom;  1: top;  2: off-field (dojo?)
   if (param_3 != 0) {
      pTop = gGlyphStrip_57;
      pMid = gGlyphStrip_58;
      pBtm = gGlyphStrip_59;
   } else {
      pTop = gGlyphStrip_52;
      pMid = gGlyphStrip_53;
      pBtm = gGlyphStrip_54;
   }

   for (i = 2; i < 26; i++) {
      pTop[i] = GLYPH_BG;
      pMid[i] = GLYPH_BG;
   }

   pTop[2] = gClassIconStartingGlyph[unit->class];
   pTop[3] = gClassIconStartingGlyph[unit->class] + 1;
   pMid[2] = gClassIconStartingGlyph[unit->class] + 2;
   pMid[3] = gClassIconStartingGlyph[unit->class] + 3;

   if (param_3 != 2) {
      s32 t;
      sprite = unit->sprite;
      t = gTerrainBonus[OBJ_TERRAIN(sprite).s.terrain];
      terrainBonus = (s16)t / 100;
      IntToLeftPaddedGlyphs(terrainBonus, &pBtm[2]);
   } else {
      terrainBonus = 0;
      IntToLeftPaddedGlyphs(terrainBonus, &pBtm[2]);
   }

   pBtm[2] = GLYPH_BG;
   IntToLeftPaddedGlyphs(unit->hp, &pBtm[19]);
   pBtm[25] = GLYPH_BG;
   IntToGlyphs(unit->maxHp, &pBtm[23]);
   pBtm[5] = GLYPH_CHAR_PERCENT;

   // Kana names span two strip rows: upper half / lower half
   FUN_8001d3fc(&pTop[5], gCharacterNames[unit->name]);
   i = FUN_8001d384(&pMid[5], gCharacterNames[unit->name]);
   i += 5;

   pTop[i] = GLYPH_BG;
   pMid[i] = GLYPH_BG;
   i++;

   FUN_8001d3fc(&pTop[i], gUnitTypeNames[unit->unitType]);
   i += FUN_8001d384(&pMid[i], gUnitTypeNames[unit->unitType]);
   pTop[i] = GLYPH_BG;
   pMid[i] = GLYPH_BG;

   if (unit->level < 10) {
      IntToLeftPaddedGlyphs(unit->level, &pMid[i + 1]);
      pMid[i + 1] = GLYPH_CHAR_UPPERCASE_L;
      pMid[i + 2] = GLYPH_CHAR_UPPERCASE_V;
   } else {
      IntToLeftPaddedGlyphs(unit->level, &pMid[i + 2]);
      pMid[i + 1] = GLYPH_CHAR_UPPERCASE_L;
      pMid[i + 2] = GLYPH_CHAR_UPPERCASE_V;
   }

   // HP Bar (10 blocks/glyphs):

   // Start out with empty blocks:
   for (i = 0; i < 10; i++) {
      pBtm[i + 9] = GLYPH_HP_BAR_0_PX;
   }

   // Calculate required blocks:
   // e.g full hp: 10000 / 125 = 80px; 80px / 8 = 10 blocks, rem 0
   //     half hp:  5000 / 125 = 40px; 40px / 8 =  5 blocks, rem 0
   {
      s32 t = (u16)unit->hpFrac;
      px = (s16)t;
   }
   px /= 125;

   if (px == 0) {
      px = 1;
   }

   full = px / 8;
   rem = px % 8;

   // Plot full blocks:
   if (full != 0) {
      for (i = 0; i < full; i++) {
         pBtm[i + 9] = GLYPH_HP_BAR_8_PX;
      }
   }

   // Plot partial block for remainder:
   if (rem != 0) {
      pBtm[full + 9] = GLYPH_HP_BAR_1_PX + rem;
   }
}

void UpdateUnitInfoWindow(UnitStatus *unit) {
   s32 i, px, full, rem;
   s32 terrainBonus;
   Object *sprite;
   u8 *firstRow, *secondRow, *thirdRow, *fourthRow;

   firstRow = gGlyphStrip_52;
   secondRow = gGlyphStrip_53;
   thirdRow = gGlyphStrip_54;
   fourthRow = gGlyphStrip_55;

   for (i = 2; i < 26; i++) {
      firstRow[i] = GLYPH_BG;
      secondRow[i] = GLYPH_BG;
   }

   firstRow[2] = gClassIconStartingGlyph[unit->class];
   firstRow[3] = gClassIconStartingGlyph[unit->class] + 1;
   secondRow[2] = gClassIconStartingGlyph[unit->class] + 2;
   secondRow[3] = gClassIconStartingGlyph[unit->class] + 3;

   sprite = unit->sprite;
   thirdRow[3] = GLYPH_BG;
   thirdRow[4] = GLYPH_BG;
   thirdRow[5] = GLYPH_BG;
   {
      s32 t = gTerrainBonus[OBJ_TERRAIN(sprite).s.terrain];
      terrainBonus = (s16)t / 100;
   }
   IntToLeftPaddedGlyphs(terrainBonus, &fourthRow[2]);
   fourthRow[2] = GLYPH_BG;

   IntToLeftPaddedGlyphs(unit->hp, &thirdRow[19]);
   thirdRow[25] = GLYPH_BG;
   IntToGlyphs(unit->maxHp, &thirdRow[23]);
   IntToLeftPaddedGlyphs(unit->exp % 100, &fourthRow[19]);

   // Kana names span two strip rows: upper half / lower half
   FUN_8001d3fc(&firstRow[5], gCharacterNames[unit->name]);
   i = FUN_8001d384(&secondRow[5], gCharacterNames[unit->name]);
   i += 5;
   firstRow[i] = GLYPH_BG;
   secondRow[i] = GLYPH_BG;
   i++;
   FUN_8001d3fc(&firstRow[i], gUnitTypeNames[unit->unitType]);
   i += FUN_8001d384(&secondRow[i], gUnitTypeNames[unit->unitType]);
   firstRow[i] = GLYPH_BG;
   secondRow[i] = GLYPH_BG;

   if (unit->level < 10) {
      IntToLeftPaddedGlyphs(unit->level, &secondRow[i + 1]);
      secondRow[i + 1] = GLYPH_CHAR_UPPERCASE_L;
      secondRow[i + 2] = GLYPH_CHAR_UPPERCASE_V;
   } else {
      IntToLeftPaddedGlyphs(unit->level, &secondRow[i + 2]);
      secondRow[i + 1] = GLYPH_CHAR_UPPERCASE_L;
      secondRow[i + 2] = GLYPH_CHAR_UPPERCASE_V;
   }

   // HP Bar
   for (i = 0; i < 10; i++) {
      thirdRow[i + 9] = GLYPH_HP_BAR_0_PX;
   }

   {
      s32 t = (u16)unit->hpFrac;
      px = (s16)t;
   }
   px /= 125;
   if (px == 0) {
      px = 1;
   }

   full = px / 8;
   rem = px % 8;
   if (full != 0) {
      for (i = 0; i < full; i++) {
         thirdRow[i + 9] = GLYPH_HP_BAR_8_PX;
      }
   }
   if (rem != 0) {
      thirdRow[full + 9] = GLYPH_HP_BAR_1_PX + rem;
   }

   // EXP Bar
   for (i = 0; i < 10; i++) {
      fourthRow[i + 9] = GLYPH_HP_BAR_0_PX;
   }

   {
      u16 mod = unit->exp % 100;
      px = mod * 80 / 100;
      if (px == 0 && mod != 0) {
         px = 1;
      }
   }
   if ((unit->exp % 100) == 0) {
      px = 0;
   }

   full = px / 8;
   rem = px % 8;
   if (full != 0) {
      for (i = 0; i < full; i++) {
         fourthRow[i + 9] = GLYPH_EXP_BAR_8_PX;
      }
   }
   if (rem != 0) {
      fourthRow[full + 9] = GLYPH_EXP_BAR_0_PX + rem;
   }
}

void Objf002_MenuChoice(Object *obj) {
   if (gWindowChoice.s.windowId != (s8)gWindowActiveIdx) {
      gWindowChoice.s.choice = 0;
      gWindowChoice.s.windowId = gWindowActiveIdx;
      gWindowActivatedChoice.raw = gWindowChoice.raw;
   } else {
      gWindowChoice.s.windowId = gWindowActiveIdx;
   }
   if (gWindowActivatedChoice.raw == gWindowChoice.raw) {
      gWindowChoice.s.choice = 0;
      gWindowActivatedChoice.s.choice = 0;
   } else {
      gWindowChoice = gWindowActivatedChoice;
   }
}

void DisplayBasicWindow(s32 windowId) {
   DisplayCustomWindowWithSetChoice(windowId, 0, 1, 0, 0, 0, 0);
}

void DisplayBasicWindowWithSetChoice(s32 windowId, s32 choiceIdx) {
   DisplayCustomWindowWithSetChoice(windowId, 0, 1, 0, 0, 0, choiceIdx);
}

void DisplayCustomWindow(s32 windowId, u8 effect, u8 translucentHighlight, u8 otOfs,
                         u8 disableWraparound, u8 clut) {
   DisplayCustomWindowWithSetChoice(windowId, effect, translucentHighlight, otOfs,
                                    disableWraparound, clut, 0);
}

void DisplayCustomWindowWithSetChoice(s32 windowId, u8 effect, u8 translucentHighlight, u8 otOfs,
                                      u8 disableWraparound, u8 clut, u8 choiceIdx) {
   Object *window;
   s16 restored1, restored2, restored3;

   gWindowChoice.raw = 0;
   CloseWindow(windowId);
   window = Obj_GetLastUnusedSkippingTail(20);

   window->functionIndex = OBJF_WINDOW_TBD_004;
   window->d.objf004.windowId = windowId;
   window->d.objf004.effect = effect;
   window->d.objf004.translucentHighlight = translucentHighlight;
   window->d.objf004.otOfs = otOfs;
   window->d.objf004.disableWraparound = disableWraparound;
   window->d.objf004.clut = clut;
   window->state2 = choiceIdx;

   switch (windowId) {
   case 0x00:
   case 0x01:
      window->x1.n = 350;
      window->y1.n = 90;
      break;
   case 0x03:
      window->x1.n = 140;
      window->y1.n = 120;
      window->functionIndex = OBJF_WINDOW_TBD_005;
      break;
   case 0x1b:
      SaveRestorePos(&restored1, &restored2, 1);
      window->x1.n = restored1 - 64;
      window->y1.n = restored2 - 13;
      HI_H(window->d.objf004.dstHighlightY) = 1;
      break;
   case 0x1c:
      SaveRestoreHp(&restored1, &restored2, &restored3, 1);
      IntToLeftPaddedGlyphs2(restored2, &gGlyphStrip_50[1]);
      IntToLeftPaddedGlyphs2(restored1, &gGlyphStrip_50[5]);
      SaveRestorePos(&restored1, &restored2, 1);
      window->x1.n = restored1 + 27;
      window->y1.n = restored2 - 4;
      break;
   case 0x02:
   case 0x1d:
      window->x1.n = 102;
      window->y1.n = 184;
      break;
   case 0x04:
   case 0x1a:
   case 0x1e:
      window->x1.n = 10;
      window->y1.n = 16;
      break;
   case 0x1f:
      window->x1.n = 102;
      window->y1.n = 175;
      break;
   case 0x20:
      window->x1.n = 14;
      window->y1.n = 56;
      break;
   case 0x32:
   case 0x33:
   case 0x34:
   case 0x35:
   case 0x36:
   case 0x37:
   case 0x38:
   case 0x39:
   case 0x3a:
   case 0x3b:
   case 0x3c:
   case 0x3d:
   case 0x3e:
   case 0x3f:
   case 0x40:
   case 0x41:
   case 0x42:
   case 0x43:
   case 0x44:
   case 0x45:
   case 0x46:
      window->functionIndex = OBJF_WINDOW_TBD_005;
      window->x1.n = gWindowDisplayX[windowId];
      window->y1.n = gWindowDisplayY[windowId];
      window->d.objf004.choicesTopMargin = gWindowChoicesTopMargin;
      window->d.objf004.highlightHeight = gWindowChoiceHeight;
      window->d.objf004.choiceCt = gWindowChoicesCount;
      window->d.objf004.choiceHeight = gWindowChoiceHeight;
   }

   window->d.objf004.halfWidth =
       gGfxSubTextures[GFX_WINDOW_TBD_657 + window->d.objf004.windowId][2] / 2;
   window->d.objf004.halfHeight =
       gGfxSubTextures[GFX_WINDOW_TBD_657 + window->d.objf004.windowId][3] / 2;
}

void CloseWindow(s32 windowId) {
   s32 i;
   Object *p = &gObjectArray[0];

   for (i = 0; i < OBJ_DATA_CT; i++, p++) {
      if ((p->functionIndex == OBJF_WINDOW_TBD_004 || p->functionIndex == OBJF_WINDOW_TBD_005) &&
          p->d.objf004.windowId == windowId) {
         p->d.objf004.windowId = 99;
         p->functionIndex = OBJF_CLOSED_WINDOW;
         p->state = 99;
         p->state3 = 0;

         if (gWindowActivatedChoice.s.windowId == windowId) {
            gWindowActivatedChoice.raw = 0;
            gWindowChoice.raw = 0;
         }
         return;
      }
   }
}

s32 GetWindowChoice(s32 windowId) {
   s32 i;
   Object *p = &gObjectArray[0];

   for (i = 0; i < OBJ_DATA_CT; i++, p++) {
      if ((p->functionIndex == OBJF_WINDOW_TBD_004 || p->functionIndex == OBJF_WINDOW_TBD_005) &&
          p->d.objf004.windowId == windowId) {
         return p->state2 + 1;
      }
   }

   return 1;
}

s32 GetWindowChoice2(s32 windowId) {
   s32 i;
   Object *p = &gObjectArray[0];

   for (i = 0; i < OBJ_DATA_CT; i++, p++) {
      if ((p->functionIndex == OBJF_WINDOW_TBD_004 || p->functionIndex == OBJF_WINDOW_TBD_005) &&
          p->d.objf004.windowId == windowId && p->state == 2) {
         return p->state2 + 1;
      }
   }

   return 0;
}

void SlideWindowTo(s32 windowId, s16 x, s16 y) {
   // Initiates a slide to specified location; actual movement is done by Objf004_005_408_Window
   s32 i;
   Object *p = &gObjectArray[0];

   for (i = 0; i < OBJ_DATA_CT; i++, p++) {
      if ((p->functionIndex == OBJF_WINDOW_TBD_004 || p->functionIndex == OBJF_WINDOW_TBD_005) &&
          p->d.objf004.windowId == windowId) {
         // Convert specified top-left point to window center point
         p->x3.n = x + p->d.objf004.halfWidth;
         p->y3.n = y + p->d.objf004.halfHeight;
         return;
      }
   }
}

#undef OBJF
#define OBJF 004
void Objf004_005_408_Window(Object *obj) {
   // TODO: todo_x2c, todo_x30 (highlight location)
   // obj->state3: effectState
   // obj->x3: destX
   // obj->y3: destY
   Object *window;
   Object *highlight;
   s8 unused[8];

   window = OBJ.window;
   highlight = OBJ.highlight;

   switch (obj->state) {
   case 0:
      obj->x1.n += OBJ.halfWidth;

      if (OBJ.windowId == 0x1c) {
         OBJ.halfWidth >>= 1;
         obj->x1.n -= OBJ.halfWidth;
      }

      if (obj->functionIndex == OBJF_WINDOW_TBD_004) {
         DrawGlyphStripGroup(gGlyphStripGroups[OBJ.windowId], GFX_WINDOW_TBD_657 + OBJ.windowId);
      }

      OBJ.effectHalfHeight = OBJ.halfHeight + (OBJ.halfHeight >> 1);
      obj->y1.n += OBJ.halfHeight;

      window = Obj_GetUnused();
      window->functionIndex = OBJF_NOOP;
      window->d.sprite2.clut = OBJ.clut;
      window->d.sprite2.semiTrans = OBJ.translucentHighlight;
      if (OBJ.otOfs == 0) {
         OBJ.otOfs = 3;
      }
      window->d.sprite2.otOfs = OBJ.otOfs;
      window->d.sprite2.gfxIdx = GFX_WINDOW_TBD_657 + OBJ.windowId;
      OBJ.window = window;

      highlight = Obj_GetUnused();
      highlight->functionIndex = OBJF_NOOP;
      highlight->d.sprite.semiTrans = OBJ.translucentHighlight;
      highlight->d.sprite.otOfs = OBJ.otOfs;
      highlight->d.sprite.gfxIdx = GFX_WINDOW_TBD_657;
      highlight->d.sprite.hidden = 1;
      OBJ.highlight = highlight;

      obj->mem = OBJ.effect;
      if (obj->x3.n == 0) {
         obj->x3.n = obj->x1.n;
      }
      if (obj->y3.n == 0) {
         obj->y3.n = obj->y1.n;
      }

      obj->state++;
   // fallthrough
   case 1:
      //@3a5c
      switch (obj->mem) {
      case 0:
         OBJ.relQuadX0 = -OBJ.halfWidth;
         OBJ.relQuadX1 = OBJ.halfWidth;
         OBJ.relQuadX2 = -OBJ.halfWidth;
         OBJ.relQuadX3 = OBJ.halfWidth;
         OBJ.relQuadY0 = -OBJ.halfHeight;
         OBJ.relQuadY1 = -OBJ.halfHeight;
         OBJ.relQuadY2 = OBJ.halfHeight;
         OBJ.relQuadY3 = OBJ.halfHeight;

         if (highlight) {
            highlight->d.sprite.hidden = 0;
         }

         OBJ.highlightY = OBJ.choicesTopMargin << 16;
         obj->state++;
         obj->state3 = 0;
         OBJ.highlightY = (obj->state2 * OBJ.choiceHeight + OBJ.choicesTopMargin) << 16;
         break;
      case 1:
      case 2:

         switch (obj->state3) {
         case 0:
            OBJ.relQuadX1 = OBJ.halfWidth << 1;
            OBJ.clut = CLUT_PURPLES;
            obj->state3++;
         // fallthrough
         case 1:
            if (--OBJ.clut != 0) {
               OBJ.relQuadX1 += (OBJ.halfWidth - OBJ.relQuadX1) >> 1;
               OBJ.relQuadX0 = -OBJ.relQuadX1;
               OBJ.relQuadX2 = -OBJ.relQuadX1;
               OBJ.relQuadX3 = OBJ.relQuadX1;
               OBJ.relQuadY2 += (OBJ.halfHeight - OBJ.relQuadY2) >> 1;
               OBJ.relQuadY0 = -OBJ.relQuadY2;
               OBJ.relQuadY1 = -OBJ.relQuadY2;
               OBJ.relQuadY3 = OBJ.relQuadY2;
            } else {
               OBJ.relQuadX0 = -OBJ.halfWidth;
               OBJ.relQuadX1 = OBJ.halfWidth;
               OBJ.relQuadX2 = -OBJ.halfWidth;
               OBJ.relQuadX3 = OBJ.halfWidth;
               OBJ.relQuadY0 = -OBJ.halfHeight;
               OBJ.relQuadY1 = -OBJ.halfHeight;
               OBJ.relQuadY2 = OBJ.halfHeight;
               OBJ.relQuadY3 = OBJ.halfHeight;
               if (highlight) {
                  highlight->d.sprite.hidden = 0;
               }
               OBJ.highlightY = OBJ.choicesTopMargin << 16;
               obj->state++;
               obj->state3 = 0;
               OBJ.highlightY = (obj->state2 * OBJ.choiceHeight + OBJ.choicesTopMargin) << 16;
            }
         }
         break;
      case 3:
      case 4:
      case 5:

         switch (obj->state3) {
         case 0:
            OBJ.relQuadX0 = OBJ.halfWidth;
            OBJ.relQuadX1 = OBJ.halfWidth;
            OBJ.relQuadX2 = -OBJ.halfWidth;
            OBJ.relQuadX3 = -OBJ.halfWidth;
            OBJ.relQuadY0 = OBJ.halfHeight;
            OBJ.relQuadY1 = OBJ.halfHeight;
            OBJ.relQuadY2 = -OBJ.halfHeight;
            OBJ.relQuadY3 = -OBJ.halfHeight;
            OBJ.effectX = OBJ.halfWidth >> 3;
            OBJ.effectY = OBJ.halfHeight >> 3;
            OBJ.clut = CLUT_WINDOW;
            obj->state3++;
         // fallthrough
         case 1:
            if (--OBJ.clut != 0) {
               OBJ.relQuadX0 = OBJ.relQuadX0 - OBJ.effectX;
               OBJ.relQuadX3 = OBJ.relQuadX3 + OBJ.effectX;
               OBJ.relQuadY0 = OBJ.relQuadY0 - OBJ.effectY;
               OBJ.relQuadY1 = OBJ.relQuadY1 - OBJ.effectY;
               OBJ.relQuadY2 = OBJ.relQuadY2 + OBJ.effectY;
               OBJ.relQuadY3 = OBJ.relQuadY3 + OBJ.effectY;
            } else {

               OBJ.relQuadX0 = -OBJ.halfWidth;
               OBJ.relQuadX1 = OBJ.halfWidth;
               OBJ.relQuadX2 = -OBJ.halfWidth;
               OBJ.relQuadX3 = OBJ.halfWidth;
               OBJ.relQuadY0 = -OBJ.halfHeight;
               OBJ.relQuadY1 = -OBJ.halfHeight;
               OBJ.relQuadY2 = OBJ.halfHeight;
               OBJ.relQuadY3 = OBJ.halfHeight;

               if (highlight) {
                  highlight->d.sprite.hidden = 0;
               }
               OBJ.highlightY = OBJ.choicesTopMargin << 16;
               obj->state++;
               obj->state3 = 0;
               OBJ.highlightY = (obj->state2 * OBJ.choiceHeight + OBJ.choicesTopMargin) << 16;
            }
         }
         break;
      case 6:
      case 7:
         OBJ.relQuadX0 = -OBJ.halfWidth;
         OBJ.relQuadX1 = OBJ.halfWidth;
         OBJ.relQuadX2 = -OBJ.halfWidth;
         OBJ.relQuadX3 = OBJ.halfWidth;

         OBJ.effectPhase += 0xc0;
         OBJ.relQuadY0 = -(rcos((OBJ.effectPhase - 0x400) & 0xfff) * OBJ.effectHalfHeight >> 12);
         OBJ.relQuadY1 = OBJ.relQuadY0;
         OBJ.relQuadY2 = -OBJ.relQuadY0;
         OBJ.relQuadY3 = -OBJ.relQuadY0;

         //@3f60
         if (OBJ.effectPhase > 0x400 && -OBJ.relQuadY0 < OBJ.halfHeight) {
            OBJ.relQuadY0 = -OBJ.halfHeight;
            OBJ.relQuadY1 = -OBJ.halfHeight;
            OBJ.relQuadY2 = OBJ.halfHeight;
            OBJ.relQuadY3 = OBJ.halfHeight;
            if (highlight) {
               highlight->d.sprite.hidden = 0;
            }
            OBJ.highlightY = OBJ.choicesTopMargin << 16;
            obj->state++;
            //@3fdc
            OBJ.highlightY = (obj->state2 * OBJ.choiceHeight + OBJ.choicesTopMargin) << 16;
         }
         break;

      case 8:
      } // END of switch (obj->mem)  (via state:1)

      break;
   case 2:
      if (gWindowActiveIdx == OBJ.windowId) {
         if (gPadStateNewPresses & PAD_DOWN) {
            if (OBJ.choiceCt > 1 && !WindowIsOffScreen(obj)) {
               PerformAudioCommand(AUDIO_CMD_PLAY_SFX(237));
            }
            if (!OBJ.disableWraparound && obj->state2 + 1 == OBJ.choiceCt) {
               // Wrap-around to first choice
               obj->state2 = 0;
            } else {
               // Go to next choice
               obj->state2++;
            }
         }
         if (gPadStateNewPresses & PAD_UP) {
            if (OBJ.choiceCt > 1 && !WindowIsOffScreen(obj)) {
               PerformAudioCommand(AUDIO_CMD_PLAY_SFX(237));
            }
            if (!OBJ.disableWraparound && obj->state2 == 0) {
               // Wrap-around to last choice
               obj->state2 = OBJ.choiceCt - 1;
            } else {
               // Go to previous choice
               obj->state2--;
            }
         }
         obj->state2 = CLAMP(obj->state2, 0, OBJ.choiceCt - 1); //@4110
      }
      OBJ.dstHighlightY = (obj->state2 * OBJ.choiceHeight + OBJ.choicesTopMargin) << 16;

      if (gState.vsyncMode == 0) {
         OBJ.highlightY += (OBJ.dstHighlightY - OBJ.highlightY) >> 1;
      } else {
         OBJ.highlightY = OBJ.dstHighlightY;
      }

      if (gWindowActiveIdx == OBJ.windowId) {
         gHighlightedChoice = obj->state2 + 1;
         if (gPadStateNewPresses & PAD_CIRCLE) {
            gWindowActivatedChoice.s.windowId = OBJ.windowId;
            gWindowActivatedChoice.s.choice = obj->state2 + 1;
         }
         if (gPadStateNewPresses & PAD_X) {
            gWindowActivatedChoice.s.windowId = OBJ.windowId;
            gWindowActivatedChoice.s.choice = -1;
         }
      }
      break;

   case 99:

      switch (obj->mem) {
      case 0:
         if (highlight) {
            highlight->d.sprite.hidden = 1;
         }
         //@428c
         obj->state++;
         obj->state3 = 0;
         obj->functionIndex = OBJF_NULL;
#ifdef PC_PORT
         /* PC_PORT: window/highlight can be NULL in this teardown; PSX discards the NULL
          * store. Guard each write -- same gated guard as the US tree (src/ui/window.c). */
         if (window) window->functionIndex = OBJF_NULL;
         if (highlight) highlight->functionIndex = OBJF_NULL;
#else
         window->functionIndex = OBJF_NULL;
         highlight->functionIndex = OBJF_NULL;
#endif
         return;
      case 1:
      case 2:

         switch (obj->state3) {
         case 0:
            OBJ.relQuadX0 = -OBJ.halfWidth;
            OBJ.relQuadX1 = OBJ.halfWidth;
            OBJ.relQuadX2 = -OBJ.halfWidth;
            OBJ.relQuadX3 = OBJ.halfWidth;
            OBJ.relQuadY0 = -OBJ.halfHeight;
            OBJ.relQuadY1 = -OBJ.halfHeight;
            OBJ.relQuadY2 = OBJ.halfHeight;
            OBJ.relQuadY3 = OBJ.halfHeight;
            OBJ.clut = CLUT_PURPLES;
            obj->state3++;
         // fallthrough
         case 1:
            if (--OBJ.clut == CLUT_NULL) {
               if (highlight) {
                  highlight->d.sprite.hidden = 1;
               }
               obj->state++;
               obj->state3 = 0;
            } else {
               OBJ.relQuadY2 += (-OBJ.relQuadY2 >> 1);
               OBJ.relQuadY0 = -OBJ.relQuadY2;
               OBJ.relQuadY1 = -OBJ.relQuadY2;
               OBJ.relQuadY3 = OBJ.relQuadY2;

               OBJ.relQuadX1 = OBJ.relQuadX1 + OBJ.relQuadY2;
               OBJ.relQuadX0 = -OBJ.relQuadX1;
               OBJ.relQuadX2 = -OBJ.relQuadX1;
               OBJ.relQuadX3 = OBJ.relQuadX1;
            }
         }

         break;
      case 3:
      case 4:
      case 5:

         switch (obj->state3) {
         case 0:
            OBJ.relQuadX0 = -OBJ.halfWidth;
            OBJ.relQuadX1 = OBJ.halfWidth;
            OBJ.relQuadX2 = -OBJ.halfWidth;
            OBJ.relQuadX3 = OBJ.halfWidth;
            OBJ.relQuadY0 = -OBJ.halfHeight;
            OBJ.relQuadY1 = -OBJ.halfHeight;
            OBJ.relQuadY2 = OBJ.halfHeight;
            OBJ.relQuadY3 = OBJ.halfHeight;
            OBJ.effectX = OBJ.halfWidth >> 3;
            OBJ.effectY = OBJ.halfHeight >> 3;
            OBJ.clut = CLUT_WINDOW;
            obj->state3++;
         // fallthrough
         case 1:
            OBJ.relQuadX0 = OBJ.relQuadX0 + OBJ.effectX;
            OBJ.relQuadX3 = OBJ.relQuadX3 - OBJ.effectX;
            OBJ.relQuadY0 = OBJ.relQuadY0 + OBJ.effectY;
            OBJ.relQuadY1 = OBJ.relQuadY1 + OBJ.effectY;
            OBJ.relQuadY2 = OBJ.relQuadY2 - OBJ.effectY;
            OBJ.relQuadY3 = OBJ.relQuadY3 - OBJ.effectY;

            if (--OBJ.clut == CLUT_NULL) {
               if (highlight) {
                  highlight->d.sprite.hidden = 1;
               }
               obj->state++;
               obj->state3 = 0;
            }
         }

         break;
      case 7:
         //@457c
         switch (obj->state3) {
         case 0:
            OBJ.relQuadX0 = -OBJ.halfWidth;
            OBJ.relQuadX1 = OBJ.halfWidth;
            OBJ.relQuadX2 = -OBJ.halfWidth;
            OBJ.relQuadX3 = OBJ.halfWidth;
            OBJ.relQuadY0 = -OBJ.halfHeight;
            OBJ.relQuadY1 = -OBJ.halfHeight;
            OBJ.relQuadY2 = OBJ.halfHeight;
            OBJ.relQuadY3 = OBJ.halfHeight;
            obj->state3++;
         // fallthrough
         case 1:
            OBJ.effectPhase -= 0xc0;
            OBJ.relQuadY0 = -(rcos((OBJ.effectPhase - 0x400U) & 0xfff) * OBJ.effectHalfHeight >> 12);
            OBJ.relQuadY1 = OBJ.relQuadY0;
            OBJ.relQuadY2 = -OBJ.relQuadY0;
            OBJ.relQuadY3 = -OBJ.relQuadY0;

            if (OBJ.effectPhase <= 0) {
               OBJ.clut = CLUT_NULL;
               obj->state3 = 0;
               obj->state++;
            }

            if (highlight) {
               highlight->d.sprite.hidden = 1;
            }
         }

         break;
      case 8:
         break;
      } // END of SECOND switch (obj->mem) (via state:99)
      break;
   case 100:
      obj->functionIndex = OBJF_NULL;
      window->functionIndex = OBJF_NULL;
      highlight->functionIndex = OBJF_NULL;
      return;
   } // END of switch (obj->state)

   // switchD_8001f2ac_caseD_6: //@46a0
   obj->x1.n += (obj->x3.n - obj->x1.n) >> 2;
   obj->y1.n += (obj->y3.n - obj->y1.n) >> 2;

   window->d.sprite2.coords[0].x = obj->x1.n + OBJ.relQuadX0;
   window->d.sprite2.coords[0].y = obj->y1.n + OBJ.relQuadY0;
   window->d.sprite2.coords[1].x = obj->x1.n + OBJ.relQuadX1;
   window->d.sprite2.coords[1].y = obj->y1.n + OBJ.relQuadY1;
   window->d.sprite2.coords[2].x = obj->x1.n + OBJ.relQuadX2;
   window->d.sprite2.coords[2].y = obj->y1.n + OBJ.relQuadY2;
   window->d.sprite2.coords[3].x = obj->x1.n + OBJ.relQuadX3;
   window->d.sprite2.coords[3].y = obj->y1.n + OBJ.relQuadY3;

   if (highlight && OBJ.choiceCt != 0 && obj->state == 2) {
      //@479c
      highlight->y1.n = window->d.sprite2.coords[1].y + (OBJ.highlightY >> 16);
      highlight->y3.n = highlight->y1.n + OBJ.highlightHeight;
      highlight->x1.n = window->d.sprite2.coords[0].x;
      highlight->x3.n = window->d.sprite2.coords[1].x;

      gGfxSubTextures[GFX_WINDOW_TBD_657][0] =
          gGfxSubTextures[GFX_WINDOW_TBD_657 + OBJ.windowId][0];
      gGfxSubTextures[GFX_WINDOW_TBD_657][2] =
          gGfxSubTextures[GFX_WINDOW_TBD_657 + OBJ.windowId][2];
      gGfxTPageCells[GFX_WINDOW_TBD_657] = gGfxTPageCells[GFX_WINDOW_TBD_657 + OBJ.windowId];
      gGfxTPageIds[GFX_WINDOW_TBD_657] = gGfxTPageIds[GFX_WINDOW_TBD_657 + OBJ.windowId];
      gGfxSubTextures[GFX_WINDOW_TBD_657][1] =
          (OBJ.highlightY >> 16) + gGfxSubTextures[GFX_WINDOW_TBD_657 + OBJ.windowId][1];
      gGfxSubTextures[GFX_WINDOW_TBD_657][3] = OBJ.highlightHeight;
      if (OBJ.windowId == gWindowActiveIdx) {
         highlight->d.sprite.clut = CLUT_NULL;
      } else {
         highlight->d.sprite.clut = CLUT_INACTIVE_WINDOW;
      }
      AddObjPrim_Gui(gGraphicsPtr->ot, highlight);
   }

   AddObjPrim2(gGraphicsPtr->ot, window);

   if (gWindowActiveIdx == OBJ.windowId) {
      gHighlightedChoice = obj->state2 + 1;
      if (gPadStateNewPresses & PAD_CIRCLE) {
         gWindowActivatedChoice.s.windowId = OBJ.windowId;
         gWindowActivatedChoice.s.choice = obj->state2 + 1;
      }
      if (gPadStateNewPresses & PAD_X) {
         gWindowActivatedChoice.s.windowId = OBJ.windowId;
         gWindowActivatedChoice.s.choice = -1;
      }
   }
}

void DrawGlyphStrip(u8 *strip, s16 x, s16 y, u8 unused) {
   RECT rect;
   u8 current, first;

   rect.w = VRAM_GLYPH_W;
   first = *strip++;
   current = *strip++;

   while (current != GLYPH_TERMINATOR) {
      if (first != GLYPH_BLANK) {
         rect.h = 4;
      } else {
         rect.h = 9;
      }
      rect.x = VRAM_GLYPHS_X + (current % VRAM_GLYPHS_PER_ROW) * VRAM_GLYPH_W;
      rect.y = VRAM_GLYPHS_Y + (current / VRAM_GLYPHS_PER_ROW) * VRAM_GLYPH_H;
      if (first != GLYPH_BLANK) {
         rect.y += 5;
      }
      MoveImage(&rect, x, y);
      x += VRAM_GLYPH_W;
      current = *strip++;
   }
}

void ClearGlyphStripBottom(u8 *strip, s16 x, s16 y) {
   u8 current;
   u8 dbl;
   u8 first;
   u8 unused[8];
   RECT rect;

   dbl = 0;
   rect.w = (16 >> 2);
   rect.h = 1;
   rect.x = VRAM_GLYPHS_X;
   rect.y = VRAM_GLYPHS_Y;

   first = *strip++;
   current = *strip++;

   while (current != GLYPH_TERMINATOR) {
      if (current == GLYPH_TBD_254) {
         dbl = 1;
         current = *strip++;
      }
      if (current == GLYPH_TBD_253) {
         dbl = 0;
         current = *strip++;
      }

      if (dbl) {
         MoveImage(&rect, x, y + 9);
         x += VRAM_GLYPH_W;
         MoveImage(&rect, x, y + 9);
         x += VRAM_GLYPH_W;
      } else {
         MoveImage(&rect, x, y + 9);
         x += VRAM_GLYPH_W;
      }

      current = *strip++;
   }
}

void DrawGlyphStripGroup(u8 *group, s16 gfxIdx) {
   s16 x, y;
   u8 current, first;

   y = 0;
   x = 512;
   if (gGfxTPageCells[gfxIdx] == 9) {
      x += 64;
   }
   if (gGfxTPageCells[gfxIdx] == 14) {
      x += 384;
   }
   if (gGfxTPageCells[gfxIdx] == 15) {
      x += 448;
   }

   x += gGfxSubTextures[gfxIdx][0] >> 2;
   y += gGfxSubTextures[gfxIdx][1];

   first = *group++;
   current = *group++;

   while (current != 0) {
      DrawGlyphStrip(gGlyphStrips[current], x, y, first);
      first = !first; // hm

      if (gGlyphStrips[current][0] != GLYPH_BLANK) {
         y += 4;
      } else {
         y += 9;
      }
      current = *group++;
   }
   group -= 2;
   current = *group;
   ClearGlyphStripBottom(gGlyphStrips[current], x, y - 9);
   ClearGlyphStripBottom(gGlyphStrips[current], x, y - 8);
   DrawSync(0);
}

u16 DrawEmbossedSjisGlyph(u16 sjis, s32 x, s32 y, s32 fgc, s32 bgc) {
   extern u32 s_embossedGlyphData[34];

   s32 i, j;
   u8 bits;
   u8 *p;
   u32 fg, bg, mask;
   RECT rect;
   u8 unused[128];

   p = Krom2RawAdd(sjis);
   if (p == -1) {
      return 0xff;
   }

   for (i = 0; i < 34; i++) {
      s_embossedGlyphData[i] = 0;
   }

   for (i = 0; i < 30; i++, p++) {
      bg = 0;
      mask = 0;
      fg = 0;
      bits = *p;

      for (j = 0; j < 7; j++) {
         if (bits & 1) {
            fg |= fgc;
            bg |= bgc;
            mask |= 0xf;
         }
         bits >>= 1;
         fg <<= 4;
         bg <<= 4;
         mask <<= 4;
      }
      if (bits & 1) {
         fg |= fgc;
         bg |= bgc;
         mask |= 0xf;
      }

      s_embossedGlyphData[i] |= bg << 4;
      s_embossedGlyphData[i + 2] |= bg;
      s_embossedGlyphData[i + 2] |= (bg << 4);

      if ((bg >> 28) && !(i & 1)) {
         s_embossedGlyphData[i + 1] |= bgc;
         s_embossedGlyphData[i + 3] |= bgc;
      }

      s_embossedGlyphData[i] &= ~mask;
      s_embossedGlyphData[i] |= fg;
   }

   s_embossedGlyphData[32] = s_embossedGlyphData[33] = 0;

   rect.x = x;
   rect.y = y;
   rect.w = 16 >> 2;
   rect.h = 16;
   LoadImage(&rect, s_embossedGlyphData);
   DrawSync(0);
   return GetTPage(0, 0, x, y);
}

void Objf422_LowerMsgBoxTail(Object *obj) { Objf421_UpperMsgBoxTail(obj); }

#undef OBJF
#define OBJF 421
void Objf421_UpperMsgBoxTail(Object *obj) {
   s32 p, flag;
   SVECTOR position;

   RotTransPers(&OBJ.sprite->vec, (s32 *)&position, &p, &flag);
   OBJ.left = position.vx - 4;
   gTempGfxObj->d.sprite.gfxIdx = GFX_MSGBOX_TAIL_DOWN;

   if (obj->functionIndex == OBJF_UPPER_MSGBOX_TAIL) {
      position.vy -= 45;
      if (position.vy < 86) {
         s16 width = 18;
         if (OBJ.left < SCREEN_HALF_WIDTH) {
            gTempGfxObj->d.sprite.gfxIdx = GFX_MSGBOX_TAIL_LEFT;
            OBJ.left += width;
         } else {
            gTempGfxObj->d.sprite.gfxIdx = GFX_MSGBOX_TAIL_RIGHT;
            OBJ.left -= width;
         }
         position.vy = 99;
      }
      OBJ.top = 78;
      OBJ.bottom = position.vy;
   } else {
      // OBJF_LOWER_MSGBOX_TAIL:
      position.vy += -10;
      if (position.vy > 150) {
         position.vy = 150;
      }
      OBJ.bottom = position.vy;
      OBJ.top = 162;
   }

   gTempGfxObj->d.sprite.hidden = 0;
   gTempGfxObj->d.sprite.clut = CLUT_25;
   gTempGfxObj->d.sprite.otOfs = 2;

   if (gTempGfxObj->d.sprite.gfxIdx == GFX_MSGBOX_TAIL_DOWN) {
      gTempGfxObj->x1.n = OBJ.left;
      gTempGfxObj->x3.n = OBJ.left + 10;
   } else {
      gTempGfxObj->x1.n = OBJ.left - 2;
      gTempGfxObj->x3.n = OBJ.left + 14;
   }

   gTempGfxObj->y1.n = OBJ.top;
   gTempGfxObj->y3.n = OBJ.bottom;
   AddObjPrim_Gui(gGraphicsPtr->ot, gTempGfxObj);
}

#undef OBJF

#define OBJF 573
void Objf573_BattleItemsList(Object *obj) {
   s32 i;
   UnitStatus *unit;
   Object *icon;
   s32 tmp;

   unit = OBJ.unit;

   switch (obj->state) {
   case 0:
      CloseWindow(0x1e);
      gWindowChoiceHeight = 18;
      gWindowChoicesTopMargin = 9;
      DrawWindow(0x38, 0, 50, 136, 54, 70, 93, WBS_CROSSED, 2);

      for (i = 0; i < 2; i++) {
         if (gItemSpells[unit->items[i]] == SPELL_NULL)
            tmp = 1; // Color to indicate item is unusable
         else
            tmp = 0;
         DrawSjisText(28, i * 18 + 60, 20, 0, tmp, gItemNamesSjis[unit->items[i]]);
      }

      icon = Obj_GetUnused();
      icon->functionIndex = OBJF_DISPLAY_ICON;
      icon->d.sprite.gfxIdx = GFX_ITEM_ICONS_OFS + unit->items[0];
      icon->x1.n = 79;
      icon->y1.n = 103;

      icon = Obj_GetUnused();
      icon->functionIndex = OBJF_DISPLAY_ICON;
      icon->d.sprite.gfxIdx = GFX_ITEM_ICONS_OFS + unit->items[1];
      icon->x1.n = 79;
      icon->y1.n = 121;

      DisplayBasicWindow(0x38);
      gWindowActiveIdx = 0x38;
      gClearSavedPadState = 1;
      obj->state++;
      break;
   case 1:
      if (gWindowChoice.raw == 0x38ff) {
         // Canceled:
         CloseWindow(0x38);
         gSignal2 = 1;
         obj->functionIndex = OBJF_NULL;
         CloseWindow(0x3c);
         CloseWindow(0x3d);
         ClearIcons();
         return;
      }
      if (gWindowChoice.s.windowId == 0x38 && gWindowChoice.s.choice != 0) {
         gCurrentSpell = gItemSpells[unit->items[gWindowChoice.s.choice - 1]];
         if (gCurrentSpell != SPELL_NULL) {
            gState.activeItem = unit->items[gWindowChoice.s.choice - 1];
            gState.activeItemSlot = gWindowChoice.s.choice - 1;
            unit->items[gWindowChoice.s.choice - 1] = ITEM_NULL;
            CloseWindow(0x38);
            gSignal2 = 2;
            gClearSavedPadState = 0;
            obj->functionIndex = OBJF_NULL;
            CloseWindow(0x3c);
            CloseWindow(0x3d);
            ClearIcons();
            return;
         }
      }
      break;
   }

   switch (obj->state3) {
   case 0:
      OBJ.item = -1;
      DrawWindow(0x3c, 0, 0, 288, 36, 14, 192, WBS_CROSSED, 0);
      DisplayBasicWindow(0x3c);
      DisplayBasicWindow(0x3d);
      obj->state3++;
      break;
   case 1:
      tmp = unit->items[GetWindowChoice(0x38) - 1];
      if (OBJ.item != tmp) {
         OBJ.item = tmp;
         DrawWindow(60, 0, 0, 288, 36, 14, 192, WBS_CROSSED, 0);
         DrawSjisText(12, 10, 30, 2, 0, gItemDescriptions[tmp]);
      }
      break;
   }
}

#undef OBJF
#define OBJF 031
void Objf031_BattleSpellsList(Object *obj) {
   static u8 mpBuffer[9] = "\x82\x6c\x82\x6f\x81\x40\x81\x40\x00";
   UnitStatus *unit;
   s32 spellIdx;
   s32 i;
   s32 extraTopMargin, extraHeight, shorten;
   s32 numSpells;
   s32 spell;
   // obj->state2: page number

   unit = OBJ.unit;

   switch (obj->state) {
   case 0:
      DrawWindow(0x3e, 400, 200, 72, 36, 214, 140, WBS_CROSSED, 0);
      EmbedIntAsSjis(unit->mp, &mpBuffer[4], 2);
      DrawSjisText(404, 210, 20, 0, 0, mpBuffer);
      DisplayBasicWindow(0x3e);
      CloseWindow(0x1e);
      gWindowChoiceHeight = 17;

#ifdef PC_FEAT
      if (!gState.debug && (unit->weapon != ITEM_V_HEART_2 || gTacticalMode)) {
#else
      if (!gState.debug && unit->weapon != ITEM_V_HEART_2) {
#endif
         numSpells = 0;
         while (unit->spells[numSpells] != SPELL_NULL) {
            numSpells++;
         }

         extraTopMargin = numSpells;
         shorten = 0;
         if (numSpells >= 7) {
            shorten = 9;
            extraTopMargin = numSpells - 7;
            extraHeight = 1;
         } else {
            extraHeight = 0;
         }
         extraTopMargin /= 2;
         gWindowChoicesTopMargin = extraTopMargin + 9;

         DrawWindow(0x38, 0, 50, 144, extraHeight + (numSpells * 18 + 18 - shorten), 70,
                    10 + (10 - numSpells) * 18 / 2, WBS_CROSSED, numSpells);

         for (i = 0; i < numSpells; i++) {
            s32 color;
            if (unit->mp < gSpells[unit->spells[i]].mpCost)
               color = 1;
            else
               color = 0;
            DrawText(12, 60 + i * 17 + extraTopMargin, 20, 0, color, gSpellNames[unit->spells[i]]);
         }
         DisplayBasicWindow(0x38);
      }

      gWindowActiveIdx = 0x38;
      gClearSavedPadState = 1;

#ifdef PC_FEAT
      if (gState.debug || (unit->weapon == ITEM_V_HEART_2 && !gTacticalMode)) {
#else
      if (gState.debug || unit->weapon == ITEM_V_HEART_2) {
#endif
         gWindowChoicesTopMargin = 9;
         DrawWindow(0x38, 0, 50, 144, 190, 70, 8, WBS_CROSSED, 10);

         i = 0;
         spellIdx = obj->state2 * 10 + 1;

         while (spellIdx < obj->state2 * 10 + 1 + 10) {
            DrawText(12, i * 17 + 60, 20, 0, 0, gSpellNames[spellIdx]);
            i++;
            spellIdx++;
         }

         DisplayBasicWindow(0x38);
      }

      obj->state++;
      break;

   case 1:

#ifdef PC_FEAT
      if (gState.debug || (unit->weapon == ITEM_V_HEART_2 && !gTacticalMode)) {
#else
      if (gState.debug || unit->weapon == ITEM_V_HEART_2) {
#endif
         // Paged menu to select from all spells; for debug mode / vandalier
         if (gPadStateNewPresses & PAD_RIGHT) {
            if (obj->state2 != 6) {
               obj->state2++; // Next page
            }
            DrawWindow(0x38, 0, 50, 144, 190, 70, 8, WBS_CROSSED, 10);

            i = 0;
            spellIdx = obj->state2 * 10 + 1;

            while (spellIdx < obj->state2 * 10 + 1 + 10) {
               DrawText(12, i * 17 + 60, 20, 0, 0, gSpellNames[spellIdx]);
               i++;
               spellIdx++;
            }
         }
         if (gPadStateNewPresses & PAD_LEFT) {
            if (obj->state2 != 0) {
               obj->state2--; // Previous page
            }
            DrawWindow(0x38, 0, 50, 144, 190, 70, 8, WBS_CROSSED, 8);

            i = 0;
            spellIdx = obj->state2 * 10 + 1;

            while (spellIdx < obj->state2 * 10 + 1 + 10) {
               DrawText(12, i * 17 + 60, 20, 0, 0, gSpellNames[spellIdx]);
               i++;
               spellIdx++;
            }
         }
      }

      if (gWindowChoice.raw == 0x38ff) {
         // Canceled
         CloseWindow(0x38);
         CloseWindow(0x3e);
         gSignal2 = 1;
         obj->functionIndex = OBJF_NULL;
         CloseWindow(0x3c);
         CloseWindow(0x3d);
         return;
      }
      if (gWindowChoice.s.windowId == 0x38 && gWindowChoice.s.choice != 0) {
         gCurrentSpell = unit->spells[gWindowChoice.s.choice - 1];
#ifdef PC_FEAT
      if (gState.debug || (unit->weapon == ITEM_V_HEART_2 && !gTacticalMode)) {
#else
      if (gState.debug || unit->weapon == ITEM_V_HEART_2) {
#endif
            gCurrentSpell = gWindowChoice.s.choice + obj->state2 * 10;
         }
         if (unit->mp < gSpells[gCurrentSpell].mpCost) {
            gCurrentSpell = SPELL_NULL;
         }
         if (gCurrentSpell != SPELL_NULL) {
            CloseWindow(0x38);
            CloseWindow(0x3e);
            gSignal2 = 2;
            gClearSavedPadState = 0;
            obj->functionIndex = OBJF_NULL;
            CloseWindow(0x3c);
            CloseWindow(0x3d);
            return;
         }
      }
      break;
   }

   switch (obj->state3) {
   case 0:
      DrawWindow(0x3c, 0, 0, 288, 36, 14, 192, WBS_CROSSED, 0);
      DisplayBasicWindow(0x3c);
      DisplayBasicWindow(0x3d);
      obj->state3++;
      break;
   case 1:
      spellIdx = GetWindowChoice(0x38);
      spell = unit->spells[spellIdx - 1];
#ifdef PC_FEAT
      if (gState.debug || (unit->weapon == ITEM_V_HEART_2 && !gTacticalMode)) {
#else
      if (gState.debug || unit->weapon == ITEM_V_HEART_2) {
#endif
         spell = spellIdx + obj->state2 * 10;
      }
      if (OBJ.spell != spell) {
         OBJ.spell = spell;
         DrawWindow(60, 0, 0, 288, 36, 14, 192, WBS_CROSSED, 0);
         DrawSjisText_Internal(12, 10, 30, 2, 0, gSpellDescriptions[spell], 0);
      }
      break;
   }
}
