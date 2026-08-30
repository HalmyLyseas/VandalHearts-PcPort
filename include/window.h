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
/* The [16] bound is too small -- indexed by raw windowId, DrawWindow calls reach 68. Harmless on
 * hardware (lands in an unclaimed gap), but the PC build's independent globals put it on live XA
 * audio state. Widened to 70, PERMUTER-gated. See docs/memory-safety.md, "AddressSanitizer". */
#ifdef PERMUTER
extern s16 gWindowDisplayX[70];
extern s16 gWindowDisplayY[70];
#else
extern s16 gWindowDisplayX[16];
extern s16 gWindowDisplayY[16];
#endif
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
