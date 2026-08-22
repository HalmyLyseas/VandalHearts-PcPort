#ifndef WINDOW_H
#define WINDOW_H

#include "common.h"

typedef enum WindowBorderStyle {
   WBS_CROSSED = 0,
   WBS_DRAGON = 1,
   WBS_ROUNDED = 2
} WindowBorderStyle;

typedef union WindowChoice {
   s8 bytes[2];
   s16 raw;
   struct {
      s8 choice, windowId;
   } s;
   struct {
      u8 choice, windowId;
   } u;
} WindowChoice;

extern u16 gWindowChoiceHeight, gWindowChoicesCount, gWindowChoicesTopMargin;
extern s16 gWindowActiveIdx;
extern u8 gHighlightedChoice;
/* The [16] bound is too small: these are indexed by raw windowId. DrawWindow -- the only writer
 * (ui/window.c:747/750/753/754) -- is called with ids 52..67, and the `usingMultipleTPages` path also
 * writes windowId + 1, so the real maximum index is 68. DisplayCustomWindowWithSetChoice reads
 * back with the same windowId (ui/window.c:1256/1257). Found by the AddressSanitizer sweep
 * flagged by an AddressSanitizer sweep across a chapter-1 battle.
 *
 * On real hardware the overrun is harmless, which is why it was never noticed: gWindowDisplayX is
 * 0x8012ed2c and gWindowDisplayY 0x8012ed54, so ids 52..68 write 0x8012ed94..0x8012edb4 and
 * 0x8012edbc..0x8012eddc respectively -- both entirely inside the unclaimed 88-byte gap between
 * gXaPauseInProgress (0x8012ed8c) and gPartyMemberUnitIdx (0x8012ede4). Nothing else lives there,
 * so the retail game is unaffected. */
extern s16 gWindowDisplayX[16];
extern s16 gWindowDisplayY[16];
extern WindowChoice gWindowChoice;
extern WindowChoice gWindowActivatedChoice;

s32 WindowIsOffScreen(struct Object *);
// void DrawSmallEquipmentWindow(u8);
void DrawWindow(s16, s16, s16, s16, s16, s16, s16, u8, u8);
s32 StringToGlyphs(u8 *, u8 *);
// void UpdateSkillStatusWindow(struct UnitStatus *);
void ClearIcons(void);
// void UpdateCompactUnitInfoWindow(struct UnitStatus *, struct UnitStatus *, u8);
// void UpdateUnitInfoWindow(struct UnitStatus *);
void DisplayBasicWindow(s32);
void DisplayBasicWindowWithSetChoice(s32, s32);
void DisplayCustomWindow(s32, u8, u8, u8, u8, u8);
// void DisplayCustomWindowWithSetChoice(s32, u8, u8, u8, u8, u8, u8);
void CloseWindow(s32);
s32 GetWindowChoice(s32);
s32 GetWindowChoice2(s32);
void SlideWindowTo(s32, s16, s16);
void DrawGlyphStripGroup(u8 *, s16);

#endif
