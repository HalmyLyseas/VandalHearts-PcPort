#include "common.h"
#include "graphics.h"
#include "object.h"
#include "state.h"
#include "glyphs.h"

/* TODO: unmatched rodata — the localized string pool (world-map location + job class
 * names; JP rodata base 0x80015d8c). The preceding pointer tables belong to maps/map_38
 * + maps/map_39_40. Bytes verified exact against the original ROM; not yet named C. */
static const u8 D_800151C8[696] = {
   0x91, 0xba, 0x82, 0xf0, 0x8f, 0x6f, 0x82, 0xe9, 0x00, 0x00, 0x00, 0x00, 0x92, 0xac, 0x82, 0xf0,
   0x8f, 0x6f, 0x82, 0xe9, 0x00, 0x00, 0x00, 0x00, 0x93, 0x5d, 0x90, 0x45, 0x93, 0xb9, 0x8f, 0xea,
   0x00, 0x00, 0x00, 0x00, 0x83, 0x56, 0x83, 0x87, 0x83, 0x62, 0x83, 0x76, 0x00, 0x00, 0x00, 0x00,
   0x83, 0x52, 0x83, 0x6f, 0x83, 0x8b, 0x83, 0x5e, 0x82, 0xcc, 0x8b, 0x75, 0x00, 0x00, 0x00, 0x00,
   0x83, 0x4b, 0x83, 0x43, 0x83, 0x41, 0x83, 0x58, 0x8d, 0xd4, 0x00, 0x00, 0x83, 0x4f, 0x83, 0x89,
   0x83, 0x58, 0x83, 0x53, 0x81, 0x5b, 0x82, 0xcc, 0x92, 0xac, 0x00, 0x00, 0x83, 0x49, 0x83, 0x8d,
   0x83, 0x43, 0x8c, 0xce, 0x00, 0x00, 0x00, 0x00, 0x8e, 0xdc, 0x94, 0x4d, 0x82, 0xcc, 0x93, 0xb4,
   0x8c, 0x41, 0x00, 0x00, 0x83, 0x66, 0x83, 0x58, 0x83, 0x4e, 0x95, 0xbd, 0x92, 0x6e, 0x00, 0x00,
   0x95, 0xd3, 0x8b, 0xab, 0x82, 0xcc, 0x91, 0xba, 0x00, 0x00, 0x00, 0x00, 0x83, 0x67, 0x83, 0x8b,
   0x83, 0x6c, 0x81, 0x5b, 0x8e, 0x52, 0x96, 0xac, 0x00, 0x00, 0x00, 0x00, 0x8a, 0x4a, 0x91, 0xf1,
   0x91, 0xba, 0x83, 0x5c, 0x83, 0x8b, 0x83, 0x7b, 0x81, 0x5b, 0x00, 0x00, 0x83, 0x8f, 0x83, 0x8b,
   0x83, 0x67, 0x98, 0x70, 0x00, 0x00, 0x00, 0x00, 0x83, 0x5f, 0x83, 0x43, 0x83, 0x93, 0x8d, 0xd4,
   0x00, 0x00, 0x00, 0x00, 0x96, 0x66, 0x88, 0xd5, 0x93, 0x73, 0x8e, 0x73, 0x83, 0x50, 0x83, 0x89,
   0x83, 0x60, 0x00, 0x00, 0x83, 0x7d, 0x83, 0x54, 0x83, 0x43, 0x91, 0xe5, 0x90, 0x58, 0x97, 0xd1,
   0x00, 0x00, 0x00, 0x00, 0x83, 0x4a, 0x83, 0x6d, 0x81, 0x5b, 0x83, 0x58, 0x82, 0xcc, 0x92, 0xac,
   0x00, 0x00, 0x00, 0x00, 0x97, 0x76, 0x8d, 0xc7, 0x8c, 0x59, 0x96, 0xb1, 0x8f, 0x8a, 0x00, 0x00,
   0x83, 0x8a, 0x83, 0x68, 0x8a, 0x58, 0x93, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x83, 0x6f, 0x83, 0x57,
   0x83, 0x8b, 0x82, 0xcc, 0x8a, 0xd6, 0x8f, 0x8a, 0x00, 0x00, 0x00, 0x00, 0x83, 0x65, 0x83, 0x58,
   0x83, 0x5e, 0x91, 0xba, 0x00, 0x00, 0x00, 0x00, 0x82, 0xb3, 0x82, 0xa2, 0x89, 0xca, 0x82, 0xc4,
   0x82, 0xcc, 0x92, 0xac, 0x00, 0x00, 0x00, 0x00, 0x92, 0x51, 0x82, 0xab, 0x82, 0xcc, 0x91, 0xe4,
   0x92, 0x6e, 0x00, 0x00, 0x8f, 0xe9, 0x8d, 0xc7, 0x88, 0xe2, 0x90, 0xd5, 0x00, 0x00, 0x00, 0x00,
   0x83, 0x8b, 0x81, 0x5b, 0x82, 0xcc, 0x8b, 0x75, 0x00, 0x00, 0x00, 0x00, 0x83, 0x43, 0x83, 0x4f,
   0x83, 0x68, 0x83, 0x89, 0x8c, 0x6b, 0x92, 0x4a, 0x00, 0x00, 0x00, 0x00, 0x83, 0x86, 0x81, 0x5b,
   0x83, 0x58, 0x91, 0xba, 0x00, 0x00, 0x00, 0x00, 0x8d, 0x60, 0x92, 0xac, 0x83, 0x7c, 0x81, 0x5b,
   0x83, 0x63, 0x00, 0x00, 0x83, 0x89, 0x83, 0x43, 0x83, 0x93, 0x91, 0xe5, 0x8b, 0xb4, 0x00, 0x00,
   0x8e, 0xf1, 0x93, 0x73, 0x83, 0x56, 0x83, 0x85, 0x83, 0x81, 0x83, 0x8a, 0x83, 0x41, 0x00, 0x00,
   0x8e, 0x52, 0x91, 0xaf, 0x82, 0xcc, 0x92, 0x4a, 0x00, 0x00, 0x00, 0x00, 0x83, 0x94, 0x83, 0x40,
   0x83, 0x93, 0x83, 0x5f, 0x83, 0x8a, 0x83, 0x41, 0x83, 0x93, 0x00, 0x00, 0x83, 0x72, 0x83, 0x56,
   0x83, 0x87, 0x83, 0x62, 0x83, 0x76, 0x00, 0x00, 0x83, 0x53, 0x83, 0x62, 0x83, 0x68, 0x83, 0x6e,
   0x83, 0x93, 0x83, 0x68, 0x00, 0x00, 0x00, 0x00, 0x83, 0x58, 0x83, 0x79, 0x83, 0x8b, 0x83, 0x7d,
   0x83, 0x58, 0x83, 0x5e, 0x81, 0x5b, 0x00, 0x00, 0x83, 0x45, 0x83, 0x42, 0x83, 0x55, 0x81, 0x5b,
   0x83, 0x68, 0x00, 0x00, 0x83, 0x58, 0x83, 0x4a, 0x83, 0x43, 0x83, 0x8d, 0x81, 0x5b, 0x83, 0x68,
   0x00, 0x00, 0x00, 0x00, 0x83, 0x7a, 0x81, 0x5b, 0x83, 0x4e, 0x83, 0x69, 0x83, 0x43, 0x83, 0x67,
   0x00, 0x00, 0x00, 0x00, 0x83, 0x47, 0x81, 0x5b, 0x83, 0x58, 0x83, 0x4b, 0x83, 0x93, 0x83, 0x69,
   0x81, 0x5b, 0x00, 0x00, 0x83, 0x58, 0x83, 0x69, 0x83, 0x43, 0x83, 0x70, 0x81, 0x5b, 0x00, 0x00,
   0x83, 0x77, 0x83, 0x72, 0x81, 0x5b, 0x83, 0x41, 0x81, 0x5b, 0x83, 0x7d, 0x81, 0x5b, 0x00, 0x00,
   0x83, 0x41, 0x81, 0x5b, 0x83, 0x7d, 0x81, 0x5b, 0x00, 0x00, 0x00, 0x00, 0x83, 0x5c, 0x81, 0x5b,
   0x83, 0x68, 0x83, 0x7d, 0x83, 0x58, 0x83, 0x5e, 0x81, 0x5b, 0x00, 0x00, 0x83, 0x74, 0x83, 0x46,
   0x83, 0x5f, 0x81, 0x5b, 0x83, 0x43, 0x83, 0x93, 0x00, 0x00, 0x00, 0x00, 0x83, 0x6e, 0x83, 0x43,
   0x83, 0x7d, 0x83, 0x58, 0x83, 0x5e, 0x81, 0x5b, 0x00, 0x00, 0x00, 0x00, 0x83, 0x51, 0x83, 0x6a,
   0x83, 0x45, 0x83, 0x58, 0x00, 0x00, 0x00, 0x00, 0x83, 0x54, 0x83, 0x80, 0x83, 0x66, 0x83, 0x89,
   0x00, 0x00, 0x00, 0x00, 0x83, 0x68, 0x83, 0x8b, 0x83, 0x81, 0x83, 0x93, 0x00, 0x00, 0x00, 0x00,
   0x83, 0x7a, 0x83, 0x8b, 0x83, 0x4e, 0x83, 0x58, 0x00, 0x00, 0x00, 0x00, 0x83, 0x41, 0x83, 0x62,
   0x83, 0x56, 0x83, 0x85, 0x00, 0x00, 0x00, 0x00,
};


s32 CopySjisString(u8 *, u8 *);
s32 DecodeLineOfText(u8 *, u8 *);
void LoadText(s32, u8 *, u8 **);
s32 DrawSjisGlyph(u16, s32, s32, s32);
s32 DrawSjisGlyphUD(u16, s32, s32, s32, s32);
s32 MsgBox_DrawSjisGlyph(Object *, u16);
s32 ParseDigits(u8 *, s32 *);
void MsgBox_Clear(Object *);
void Objf351_MsgBoxText(Object *);
void DrawSjisText_Internal(s32, s32, s32, s32, s32, u8 *, s32);
void DrawText(s32, s32, s32, s32, s32, u8 *);
void MsgBox_SetText(s32, s32, s32);
void MsgBox_SetText2(s32, s32, s32);
void Objf798_ResetInputState(Object *);

// JP-specific: custom glyph bitmaps for rare kanji (special-char fallback), and the SJIS codes
// that select them (gSpecialSjisCodes below). 27 glyphs x 30 bytes (12x15 4bpp).
static u8 gCustomGlyphs[27][30] = {
   {0x02, 0x00, 0xff, 0xfe, 0x07, 0x00, 0x18, 0x90, 0xf0, 0x60, 0x17, 0x30, 0x78, 0x0e, 0x00, 0x00, 0x7f, 0xfc, 0x44, 0x94, 0x48, 0xf4, 0x57, 0xc4, 0x44, 0x44, 0x47, 0xc4, 0x40, 0x3c},
   {0x41, 0x10, 0x27, 0xfc, 0x01, 0x10, 0x83, 0xf8, 0x42, 0x48, 0x03, 0xf8, 0x02, 0x48, 0x03, 0xf8, 0xe0, 0x40, 0x27, 0xfc, 0x24, 0x54, 0x25, 0xe4, 0x24, 0x1c, 0x58, 0x00, 0x87, 0xfe},
   {0x11, 0x20, 0x11, 0x20, 0x2f, 0x3e, 0x41, 0x20, 0x91, 0x20, 0x11, 0x20, 0x2f, 0x3c, 0x21, 0x20, 0x61, 0x20, 0xa3, 0xa0, 0x2d, 0x3e, 0x22, 0x20, 0x22, 0x20, 0x24, 0x20, 0x28, 0x20},
   {0x10, 0x00, 0x17, 0xfc, 0x24, 0x04, 0x44, 0x04, 0x94, 0x04, 0x15, 0xe4, 0x25, 0x24, 0x25, 0x24, 0x65, 0x24, 0xa5, 0x24, 0x25, 0xe4, 0x24, 0x04, 0x24, 0x04, 0x24, 0x04, 0x27, 0xfc},
   {0x00, 0x20, 0x7c, 0x20, 0x45, 0xfe, 0x45, 0x02, 0x45, 0x02, 0x7c, 0x78, 0x10, 0x00, 0x10, 0x00, 0x51, 0xfe, 0x5c, 0x20, 0x50, 0xa4, 0x50, 0xa2, 0x51, 0x22, 0x5c, 0x20, 0xe0, 0x60},
   {0x00, 0x00, 0x47, 0xfc, 0x24, 0x40, 0x07, 0xf8, 0x04, 0x40, 0x07, 0xf8, 0x14, 0x40, 0x27, 0xfc, 0x40, 0x04, 0x85, 0x54, 0x09, 0x44, 0x01, 0x18, 0x48, 0x84, 0x48, 0x12, 0x87, 0xf2},
   {0x20, 0x00, 0x27, 0xfe, 0x30, 0xa0, 0xa8, 0xa0, 0xab, 0xfc, 0xa2, 0xa4, 0xa2, 0xa4, 0x23, 0xfc, 0x20, 0x40, 0x2f, 0xfe, 0x20, 0xe0, 0x21, 0x50, 0x22, 0x48, 0x2c, 0x46, 0x20, 0x40},
   {0x00, 0x20, 0x7b, 0xfe, 0x48, 0x20, 0x49, 0xfc, 0x78, 0x00, 0x4b, 0xfe, 0x4a, 0x52, 0x7a, 0x8a, 0x4b, 0xfe, 0x49, 0x04, 0x49, 0xfc, 0x79, 0x04, 0x01, 0xfc, 0x50, 0x88, 0x8b, 0x06},
   {0x40, 0x00, 0x23, 0xf8, 0x02, 0x08, 0x82, 0x08, 0x43, 0xf8, 0x00, 0x00, 0x07, 0xfc, 0x00, 0x40, 0xe3, 0xf8, 0x20, 0x40, 0x20, 0x40, 0x27, 0xfc, 0x20, 0x00, 0x58, 0x00, 0x87, 0xfe},
   {0x00, 0x00, 0x7b, 0xfe, 0x02, 0x00, 0xfa, 0x7c, 0x02, 0x44, 0x7a, 0x44, 0x02, 0x7c, 0x7a, 0x00, 0x02, 0xee, 0x02, 0xaa, 0x7a, 0xaa, 0x4a, 0xaa, 0x4a, 0xee, 0x4a, 0x00, 0x7b, 0xfe},
   {0x00, 0x80, 0x3f, 0xfe, 0x20, 0x00, 0xa1, 0x90, 0x6e, 0x7e, 0x22, 0x10, 0x22, 0x3c, 0x6f, 0x90, 0xa2, 0x7e, 0x26, 0x24, 0x27, 0x3c, 0x2a, 0xa4, 0x4a, 0x24, 0x42, 0x3c, 0x82, 0x42},
   {0x00, 0x00, 0x7b, 0xde, 0x49, 0x4a, 0x4a, 0xd2, 0x49, 0x46, 0x7a, 0x4a, 0x48, 0x20, 0x48, 0xd8, 0x4b, 0x16, 0x78, 0x60, 0x49, 0x88, 0x48, 0x32, 0x49, 0xc4, 0x88, 0x18, 0x99, 0xe0},
   {0x7f, 0xfc, 0x40, 0x04, 0x40, 0x04, 0x7f, 0xfc, 0x40, 0x00, 0x48, 0x40, 0x48, 0x40, 0x48, 0x40, 0x48, 0x4c, 0x4f, 0x70, 0x48, 0x40, 0x48, 0x40, 0x88, 0x42, 0x8b, 0x42, 0xbc, 0x3e},
   {0x00, 0x00, 0x7f, 0xfc, 0x44, 0x44, 0x7f, 0xfc, 0x00, 0x00, 0x3f, 0xf8, 0x20, 0x08, 0x20, 0x08, 0x3f, 0xf8, 0x21, 0x00, 0x3f, 0xfc, 0x20, 0x80, 0x20, 0x42, 0x27, 0x32, 0xf8, 0x0e},
   {0x25, 0x28, 0x57, 0xbc, 0x89, 0x48, 0x07, 0xbe, 0xfb, 0x48, 0x25, 0x9a, 0x21, 0x2e, 0xf8, 0x00, 0x21, 0xf8, 0x21, 0x08, 0xa9, 0xf8, 0x69, 0x08, 0x21, 0xf8, 0x30, 0x90, 0xc3, 0x0c},
   {0x01, 0x00, 0x21, 0x08, 0x21, 0x08, 0x3f, 0xf8, 0x41, 0x04, 0x41, 0x04, 0x7f, 0xfc, 0x00, 0x00, 0x3f, 0xf8, 0x00, 0x00, 0xff, 0xfe, 0x01, 0x00, 0x11, 0x10, 0x21, 0x08, 0x47, 0x04},
   {0x00, 0x00, 0x7d, 0xfe, 0x50, 0x00, 0x7c, 0xfc, 0x50, 0x84, 0x7c, 0x84, 0x50, 0xfc, 0x50, 0x80, 0x7c, 0xfe, 0x04, 0xaa, 0xa5, 0xaa, 0xa5, 0xfe, 0x8a, 0xaa, 0x08, 0xaa, 0x30, 0x86},
   {0x00, 0x00, 0x7d, 0xfc, 0x44, 0x48, 0x44, 0x30, 0x45, 0xfe, 0x7c, 0x52, 0x10, 0x94, 0x11, 0x30, 0x50, 0x00, 0x5c, 0x20, 0x53, 0xfe, 0x50, 0x70, 0x50, 0xa8, 0x5d, 0x24, 0xe2, 0x22},
   {0x00, 0x00, 0x7b, 0xde, 0x4a, 0x52, 0x4b, 0xde, 0x4a, 0x52, 0x7b, 0xde, 0x12, 0xa2, 0x13, 0xfa, 0x52, 0xa2, 0x5a, 0xf2, 0x52, 0xa2, 0x52, 0xf2, 0x52, 0xa2, 0x5a, 0xfa, 0xe2, 0x06},
   {0x20, 0x80, 0x20, 0x80, 0x25, 0xf8, 0xa9, 0x08, 0xb3, 0x10, 0xa4, 0x90, 0xa0, 0x60, 0x20, 0x90, 0x21, 0x08, 0x26, 0x06, 0x51, 0xf8, 0x49, 0x08, 0x41, 0x08, 0x81, 0x08, 0x81, 0xf8},
   {0x00, 0x10, 0xf7, 0x94, 0x52, 0x92, 0xb5, 0x90, 0x52, 0x9e, 0x94, 0xf0, 0x08, 0x14, 0x36, 0x14, 0xc5, 0x98, 0x18, 0x18, 0xe2, 0x10, 0x0c, 0x12, 0x71, 0x2a, 0x06, 0x46, 0x78, 0x82},
   {0x00, 0x40, 0xf0, 0x40, 0x90, 0x40, 0x97, 0xfe, 0x90, 0x40, 0xf0, 0x80, 0x95, 0x08, 0x92, 0x10, 0xf1, 0x20, 0x90, 0xc0, 0x90, 0x48, 0x90, 0x84, 0xf1, 0x0c, 0x02, 0x72, 0x0f, 0x82},
   {0x00, 0x80, 0x00, 0x80, 0x3f, 0xfe, 0x20, 0x00, 0xa2, 0x08, 0x61, 0x10, 0x2f, 0xfe, 0x20, 0x40, 0x60, 0x40, 0xa7, 0xfc, 0x20, 0x40, 0x20, 0x40, 0x4f, 0xfe, 0x40, 0x40, 0x80, 0x40},
   {0x02, 0x90, 0xfa, 0x94, 0x25, 0x52, 0x25, 0x50, 0x4f, 0xfe, 0x72, 0x90, 0x56, 0xd0, 0x52, 0x94, 0xd6, 0xd4, 0xb2, 0x94, 0x26, 0xd8, 0x22, 0x88, 0x43, 0xda, 0x4e, 0x26, 0x80, 0x42},
   {0x00, 0x40, 0x00, 0xa0, 0xf1, 0x10, 0x92, 0x08, 0x9d, 0xf6, 0x90, 0x00, 0x93, 0xf8, 0x90, 0x08, 0x90, 0x10, 0x90, 0x80, 0xf2, 0x40, 0x0a, 0x04, 0x0a, 0x12, 0x12, 0x12, 0x01, 0xf0},
   {0x01, 0x00, 0x01, 0xf8, 0x01, 0x00, 0x3f, 0xfe, 0x21, 0x02, 0x21, 0xf0, 0x2f, 0x04, 0x21, 0xfc, 0x20, 0x40, 0x2f, 0xfe, 0x21, 0x10, 0x21, 0x10, 0x40, 0xe0, 0x41, 0xb0, 0x8e, 0x0e},
   {0x00, 0x80, 0x00, 0x80, 0x3f, 0xfe, 0x20, 0x40, 0xa0, 0x80, 0x67, 0xfc, 0x24, 0x44, 0x27, 0xfc, 0x64, 0x44, 0xa7, 0xfc, 0x21, 0x20, 0x22, 0x20, 0x4f, 0xfe, 0x40, 0x20, 0x80, 0x20},
};

/* world/dojo.c sets `OBJ.partyIdx = 100` as a "no selection yet" sentinel, then does
 * `gStringTable[32] = gStringTable[OBJ.partyIdx];` -- reading index 100 of this 100-entry table
 * for the one frame before a real party index is assigned. On hardware that reads the shade
 * tables (0x801036c0, immediately after gStringTable's 400 bytes) reinterpreted as a string
 * pointer; the game never displays string 32 on that frame, so the garbage read is harmless.
 *
 * PERMUTER/PC_PORT: 101 entries, not 100 -- same gated widening as the US tree (that sentinel
 * read is a real OOB on PC, found there by the ASAN sweep). The extra entry is implicitly NULL
 * and the matching build below keeps exactly the 100 real entries, so nothing shifts. */
#ifdef PC_PORT
/* The native port reconstructs these PSX-address pointers from the user's executable
 * (gen_string_table.py, JP mode), then installs host pointers from a constructor. */
u8 *gStringTable[101] = {0};
#elif defined(PERMUTER)
u8 *gStringTable[101] = {
#include "assets/8010102c.inc"
};
#else
u8 *gStringTable[100] = {
#include "assets/8010102c.inc"
};
#endif

u8 **s_stringTable_80123348;

// Anti-alias shade tables, shared by DrawSjisGlyph + DrawSjisGlyphUD (JP: one copy @0x801036c0).
static s32 whiteShades[5] = {0, 1, 2, 3, 4};
static s32 redShades[5] = {0, 5, 6, 7, 8};

// JP-specific: rare kanji absent from the PS1 BIOS charset, and their custom
// 12x15 4bpp glyph bitmaps (30 bytes each). Indexed in lockstep by DrawSjisGlyph.
static u16 gSpecialSjisCodes[30] = {
   0xe5e1, 0xe7b0, 0x9c70, 0x9c6a, 0xe748, 0x9cdf, 0x9cc9, 0xe6dc, 0xe797,
   0xe690, 0xe19b, 0xe450, 0x9b9b, 0xe3a9, 0xe873, 0xe24d, 0xe978, 0xe6f8,
   0xe757, 0xe080, 0x9d43, 0xe1bf, 0xe179, 0x9f72, 0x9a58, 0xe569, 0xe183, 0x0000, 0x0000, 0x0000,
};

s32 CopySjisString(u8 *src, u8 *dst) {
   s32 n = 0;

   while (*src != '\0') {
      if ((*src >= 0x81 && *src <= 0x9f) || (*src >= 0xe0 && *src <= 0xfc)) {
         *dst++ = *src++;
         *dst++ = *src++;
         n += 2;
      } else {
         *dst++ = *src++;
         n += 1;
      }
   }

   return n;
}

s32 DecodeLineOfText(u8 *src, u8 *dst) {
   u8 b1, b2;
   s32 n = 0;

   while (1) {
#ifdef PC_PORT
      /* Corrupt-input bound (2026-08-21, JP debug-menu event-map warp, gdb first-chance trace):
       * with UNLOADED scratch data there is no CR/LF terminator, and the decode marched ~4.7KB
       * past the caller's 1KB stack buffer into the guard page -- smearing the whole stack on
       * the way (the "no dump, no core" crash). Real lines are <100 bytes; 1000 stays inside
       * LoadText's buffer[1024] incl. the "\n\0" tail. LoadText treats a capped line as END. */
      if (n >= 1000) {
         break;
      }
#endif
      b1 = ~src[0];
      b2 = ~src[1];
      if (b1 == '\r' && b2 == '\n') {
         break;
      }
      if ((b1 >= 0x81 && b1 <= 0x9f) || (b1 >= 0xe0 && b1 <= 0xfc)) {
         *dst++ = b1;
         *dst++ = b2;
         src += 2;
         n += 2;
      } else {
         *dst++ = b1;
         src++;
         n += 1;
      }
   }

   *dst++ = '\n';
   *dst++ = '\0';
   return n + 2;
}

void LoadText(s32 cdf, u8 *pText, u8 **textPointers) {
   s32 readingEntry;
   s32 entryNum;
   u8 *pInputData;
   u8 buffer[1024];
   s32 n;

   readingEntry = 0;
   LoadCdFile(cdf, 0);
   entryNum = 1;
   pInputData = (u8 *)gScratch1_801317c0;

   while (entryNum <= 100) {
      n = DecodeLineOfText(pInputData, buffer);

#ifdef PC_PORT
      if (n >= 1000) {
         /* Capped decode = corrupt/unloaded text data (see DecodeLineOfText). Don't just stop:
          * leaving textPointers[] unfilled hands consumers garbage pointers that can sit at a
          * page edge and fault inside the renderers (2026-08-22, debug-menu warp, 2nd gdb
          * trace). Point every entry at a safe empty string instead, so draws render nothing. */
         pText[0] = '\0';
         for (n = 1; n <= 100; n++) {
            textPointers[n] = pText;
         }
         break;
      }
#endif
      if (buffer[0] == '\n' && readingEntry == 0) {
         readingEntry = 1;
         textPointers[entryNum++] = pText;
         pInputData += n;
      } else if (buffer[0] == '\n' && readingEntry == 1) {
         readingEntry = 0;
         *pText++ = '\0';
      } else if (buffer[0] == 'E' && buffer[1] == 'N' && buffer[2] == 'D') {
         *pText++ = '\0';
         break;
      } else if (((buffer[0] >= 0x81 && buffer[0] <= 0x9f) ||
                  (buffer[0] >= 0xe0 && buffer[0] <= 0xfc)) &&
                 (buffer[1] == 0x94)) {
         //? Presumably this is to treat lines starting with SJIS 8194 (#) as comments, but won't
         //  it also include a bunch of false positives?
         pInputData += n;
      } else {
         pText += CopySjisString(buffer, pText);
         pInputData += n;
      }
   }
}


// JP-only: draws HALF a glyph (top 8 rows if `up`, bottom 7 rows otherwise) for
// the U/D control codes (raised/lowered kana). Shares DrawSjisGlyph's char lookup.
s32 DrawSjisGlyphUD(u16 sjis, s32 x, s32 y, s32 color, s32 up) {
   u32 buffer[32];
   u16 glyph[48];
   RECT rect;
   s32 *colors;
   u8 *pInputData;
   u32 *pOutputData;
   u16 *src;
   s32 i;
   u32 word0, word1;

   if (color != 0) {
      colors = redShades;
   } else {
      colors = whiteShades;
   }

   pInputData = (u8 *)Krom2RawAdd(sjis);
   if (pInputData == (u8 *)-1) {
      u16 *codes = gSpecialSjisCodes;
      u8 *glyphs = gCustomGlyphs;
      while (*codes != 0) {
         if (sjis == *codes) {
            pInputData = glyphs;
            break;
         }
         codes++;
         glyphs += 30;
      }
      if (pInputData == (u8 *)-1) {
         return -1;
      }
   }

   pOutputData = &buffer[0];

   for (i = 0; i < 15; i++) {
      u32 b;
      u8 c;
      s32 bit0, bit1, bit2, bit3;

      b = *pInputData; // byte1
      bit0 = b & 1;
      bit1 = (b >> 1) & 1;
      word0 = colors[bit0 * 3 + bit1] << 20;
      bit2 = (b >> 2) & 1;
      word0 |= colors[bit1 * 2 + bit2 * 2] << 16;
      bit3 = (b >> 3) & 1;
      word0 |= colors[bit3 * 3 + bit2] << 12;
      bit0 = (b >> 4) & 1;
      { u32 t5 = b >> 5; bit1 = t5 & 1; }
      word0 |= colors[bit0 * 3 + bit1] << 8;
      { u32 t6 = b >> 6; bit2 = t6 & 1; }
      word0 |= colors[bit1 * 2 + bit2 * 2] << 4;
      bit3 = (b >> 7) & 1;
      word0 |= colors[bit3 * 3 + bit2];

      pInputData++;
      c = *pInputData; // byte2
      bit0 = c & 1;
      bit1 = (c >> 1) & 1;
      word1 = colors[bit0 * 3 + bit1] << 12;
      bit2 = (c >> 2) & 1;
      word1 |= colors[bit1 * 2 + bit2 * 2] << 8;
      bit3 = (c >> 3) & 1;
      word1 |= colors[bit3 * 3 + bit2] << 4;
      bit0 = (c >> 4) & 1;
      bit1 = (c >> 5) & 1;
      word1 |= colors[bit0 * 3 + bit1];
      bit2 = (c >> 6) & 1;
      word0 |= colors[bit1 * 2 + bit2 * 2] << 28;
      bit3 = (c >> 7) & 1;
      word0 |= colors[bit3 * 3 + bit2] << 24;

      *pOutputData++ = word0;
      *pOutputData++ = word1;
      pInputData++;
   }
   *pOutputData++ = 0;
   *pOutputData++ = 0;

   src = (u16 *)&buffer[0];
   for (i = 0; i < 15; i++) {
      glyph[i * 3] = src[0];
      glyph[i * 3 + 1] = src[1];
      glyph[i * 3 + 2] = src[2];
      src += 4;
   }

   if (up != 0) {
      rect.x = x;
      rect.y = y;
      rect.w = 12 >> 2;
      rect.h = 8;
      LoadImage(&rect, (u_long *)&glyph[0]);
   } else {
      rect.x = x;
      rect.y = y;
      rect.w = 12 >> 2;
      rect.h = 7;
      LoadImage(&rect, (u_long *)&glyph[24]);
   }
   DrawSync(0);
   return 0;
}

s32 DrawSjisGlyph(u16 sjis, s32 x, s32 y, s32 color) {
   // For anti-aliasing effect:
   u16 buffer[45];
   s32 *colors;
   RECT rect;
   u8 *pInputData;
   u16 *pOutputData;
   s32 i;
   u8 byte;
   u16 output1, output2, output3;
   u16 bit0, bit1, bit2, bit3, bit4, bit5, bit6, bit7;
   u16 tmp1, tmp2; //?

   if (color != 0) {
      colors = redShades;
   } else {
      colors = whiteShades;
   }

   pInputData = (u8 *)Krom2RawAdd(sjis);
   if (pInputData == (u8 *)-1) {
      // JP: chars absent from the PS1 BIOS charset fall back to a game-supplied
      // custom glyph, matched by SJIS code.
      u16 *codes = gSpecialSjisCodes;
      u8 *glyphs = gCustomGlyphs;
      while (*codes != 0) {
         if (sjis == *codes) {
            pInputData = glyphs;
            break;
         }
         codes++;
         glyphs += 30;
      }
      if (pInputData == (u8 *)-1) {
         return -1;
      }
   }

   pOutputData = &buffer[0];

   for (i = 0; i < 15; i++) {
      byte = *pInputData; // byte1

      bit0 = byte & 1;
      byte >>= 1;
      bit1 = byte & 1;
      byte >>= 1;
      bit2 = byte & 1;
      byte >>= 1;
      bit3 = byte & 1;
      byte >>= 1;

      output2 = colors[bit0 * 3 + bit1] << 4;
      output2 |= colors[bit1 * 2 + bit2 * 2];
      output1 = tmp1 = colors[bit3 * 3 + bit2] << 12;

      bit4 = byte & 1;
      byte >>= 1;
      bit5 = byte & 1;
      byte >>= 1;
      bit6 = byte & 1;
      byte >>= 1;
      bit7 = byte & 1;
      byte >>= 1;

      output1 |= colors[bit4 * 3 + bit5] << 8;
      output1 |= colors[bit5 * 2 + bit6 * 2] << 4;
      output1 |= colors[bit7 * 3 + bit6];

      pInputData++;
      byte = *pInputData; // byte2

      bit0 = byte & 1;
      byte >>= 1;
      bit1 = byte & 1;
      byte >>= 1;
      bit2 = byte & 1;
      byte >>= 1;
      bit3 = byte & 1;
      byte >>= 1;

      output3 = tmp2 = colors[bit0 * 3 + bit1] << 12;
      output3 |= colors[bit1 * 2 + bit2 * 2] << 8;
      output3 |= colors[bit3 * 3 + bit2] << 4;

      bit4 = byte & 1;
      byte >>= 1;
      bit5 = byte & 1;
      byte >>= 1;
      bit6 = byte & 1;
      byte >>= 1;
      bit7 = byte & 1;
      byte >>= 1;

      output3 |= colors[bit4 * 3 + bit5];
      output2 |= colors[bit5 * 2 + bit6 * 2] << 12;
      output2 |= colors[bit7 * 3 + bit6] << 8;

      *pOutputData++ = output1;
      *pOutputData++ = output2;
      *pOutputData++ = output3;
      pInputData++;
   }

   rect.x = x;
   rect.y = y;
   rect.w = 12 >> 2;
   rect.h = 15;
   LoadImage(&rect, buffer);
   DrawSync(0);
   return 0;
}

static u8 sMsgBoxVramOffsets[6][4] = {{0, 0, -8, 108}, {0, 0, 72, 108},   {0, 0, -8, 108},
                                      {0, 0, 72, 108}, {0, 100, -8, 108}, {0, 100, 72, 108}};

s32 MsgBox_DrawSjisGlyph(Object *msg, u16 sjis) {
   // x3: column, y3: row
   if (msg->x3.n < msg->d.objf351.pregapChars) {
      DrawSjisGlyph(sjis, 512 + msg->x3.n * (12 >> 2) + msg->x1.n,
                    msg->y3.n * (msg->d.objf351.lineSpacing + 15) + msg->y1.n +
                        sMsgBoxVramOffsets[msg->d.objf351.type * 2][1],
                    0);
      return 0;
   } else if (msg->x3.n < msg->d.objf351.maxCharsPerLine) {
      DrawSjisGlyph(sjis, 576 + (msg->x3.n - msg->d.objf351.pregapChars) * (12 >> 2),
                    msg->y3.n * (msg->d.objf351.lineSpacing + 15) + msg->y1.n +
                        sMsgBoxVramOffsets[msg->d.objf351.type * 2 + 1][1],
                    0);
      return 0;
   } else {
      return -1;
   }
}

s32 ParseDigits(u8 *str, s32 *output) {
   s32 value;
   s32 n;

   value = 0;
   n = 1;

   while (*str >= '0' && *str <= '9') {
      value *= 10;
      value += (*str - '0');
      str++;
      n++;
   }

   *output = value;
   return n;
}

void MsgBox_Clear(Object *msg) {
   Object *buttonIcon;

   buttonIcon = msg->d.objf351.buttonIcon;
   buttonIcon->functionIndex = OBJF_NULL;
   msg->functionIndex = OBJF_NULL;
   gState.msgBoxFinished = 1;
}

#undef OBJF
#define OBJF 351
void Objf351_MsgBoxText(Object *obj) {
   static s16 buttonIconAnimData1[12] = {2, GFX_MSGBOX_BUTTON_1,
                                         3, GFX_MSGBOX_BUTTON_2,
                                         3, GFX_MSGBOX_BUTTON_3,
                                         3, GFX_MSGBOX_BUTTON_4,
                                         3, GFX_NULL,
                                         1, GFX_NULL};

   static s16 buttonIconAnimData2[12] = {2, GFX_MSGBOX_BUTTON_1,
                                         6, GFX_MSGBOX_BUTTON_2,
                                         6, GFX_MSGBOX_BUTTON_3,
                                         6, GFX_MSGBOX_BUTTON_4,
                                         6, GFX_NULL,
                                         1, GFX_NULL};

   static SVectorXY buttonIconPositions[6] = {{289, 58},  {232, 203}, {281, 64},
                                              {236, 197}, {285, 129}, {281, 99}};

   static s32 textSpeeds[8] = {0x80, 0x100, 0x400, 0x400, 0x400, 0x400, 0x400, 0x400};

   s16 buttonIconX, buttonIconY;
   Object *buttonIcon;
   RECT rect;
   u8 *p;
   s32 maxCharsPerLine;
   s32 parsedInt;
   s32 n;
   u32 sjis;

   maxCharsPerLine = OBJ.maxCharsPerLine;

   switch (obj->state) {
   case 0:
      gState.msgBoxFinished = 0;
      gState.msgBoxPagePaused = 0;
      obj->state3 = 1;
      OBJ.speakAnimSuppressed = 0;

      OBJ.textSpeed = textSpeeds[gState.textSpeed & 7];
      if (gState.vsyncMode != 2) {
         OBJ.textSpeed >>= 1;
      }

      if (OBJ.maxCharsPerLine == 0) {
         OBJ.maxCharsPerLine = 19;
      }
      OBJ.lineSpacing = 1;
      OBJ.maxRows = 3;

      buttonIconX = buttonIconPositions[OBJ.type - 1].x;
      buttonIconY = buttonIconPositions[OBJ.type - 1].y;

      switch (OBJ.type) {
      case 0:
      case 1:
         obj->x1.n = 60 >> 2;
         obj->y1.n = 8;
         OBJ.pregapChars = (240 - obj->x1.n * 4) / 12;
         break;
      case 2:
         obj->x1.n = 12 >> 2;
         obj->y1.n = 8;
         OBJ.pregapChars = (240 - obj->x1.n * 4) / 12;
         break;
      case 3:
         obj->x1.n = 68 >> 2;
         obj->y1.n = 22;
         OBJ.pregapChars = (248 - obj->x1.n * 4) / 12;
         OBJ.type = 1;
         break;
      case 4:
         obj->x1.n = 20 >> 2;
         obj->y1.n = 22;
         OBJ.pregapChars = (248 - obj->x1.n * 4) / 12;
         OBJ.type = 2;
         break;
      case 5:
         obj->x1.n = 68 >> 2;
         obj->y1.n = 22;
         OBJ.pregapChars = (248 - obj->x1.n * 4) / 12;
         OBJ.type = 1;
         break;
      case 6:
         obj->x1.n = 68 >> 2;
         obj->y1.n = 22;
         OBJ.pregapChars = (248 - obj->x1.n * 4) / 12;
         OBJ.type = 1;
         break;
      }

      obj->x3.n = 0; // current column
      obj->y3.n = 0; // current row
      OBJ.textSpeedAccum = 0;
      if (OBJ.textSpeed == 0) {
         OBJ.textSpeed = 0x100;
      }
      if (OBJ.pregapChars > OBJ.maxCharsPerLine) {
         OBJ.pregapChars = OBJ.maxCharsPerLine;
      }

      buttonIcon = Obj_GetUnused();
      buttonIcon->functionIndex = OBJF_NOOP;
      OBJ.buttonIcon = buttonIcon;
      if (gState.vsyncMode == 2) {
         buttonIcon->d.sprite.animData = buttonIconAnimData1;
      } else {
         buttonIcon->d.sprite.animData = buttonIconAnimData2;
      }
      buttonIcon->d.sprite.gfxIdx = GFX_TBD_42;
      buttonIcon->d.sprite.semiTrans = 1;
      buttonIcon->d.sprite.clut = CLUT_25;
      buttonIcon->x1.n = buttonIconX;
      buttonIcon->y1.n = buttonIconY;
      buttonIcon->x3.n = buttonIconX + 16;
      buttonIcon->y3.n = buttonIconY + 16;

      OBJ.textPtr = gState.currentTextPointers[OBJ.textPtrIdx];
      OBJ.fastForward = 0;

      OBJ.rect.x = obj->x1.n + 512;
      if (OBJ.type == 1) {
         OBJ.rect.y = obj->y1.n;
      } else if (OBJ.type == 2) {
         OBJ.rect.y = obj->y1.n + 100;
      }
      OBJ.rect.w = 64 + (OBJ.maxCharsPerLine - OBJ.pregapChars) * (12 >> 2) - obj->x1.n;
      OBJ.rect.h = (OBJ.lineSpacing + 15) * OBJ.maxRows;
      rect.x = OBJ.rect.x;
      rect.y = OBJ.rect.y;
      rect.w = OBJ.rect.w;
      rect.h = OBJ.rect.h;
      ClearImage(&rect, 0, 0, 0);
      obj->state++;
      break;

   case 1:
      if (--obj->state3 > 0) {
         break;
      }
      obj->state++;

   // fallthrough
   case 2:
      p = OBJ.textPtr;
      if (gPadStateNewPresses & PAD_X) {
         OBJ.textSpeedAccum += 0x4000;
      }
      if (OBJ.fastForward == 0 && (gPadStateNewPresses & PAD_CIRCLE)) {
         OBJ.fastForward = 1;
      }
      if (OBJ.todo_x45 != 0 && !(gPadState & PAD_CIRCLE)) {
         OBJ.fastForward = 0;
      }
      if ((gPadState & PAD_CIRCLE) && (OBJ.fastForward != 0)) {
         OBJ.textSpeedAccum += 0x200;
         if (OBJ.textSpeedAccum > 0x4000) {
            OBJ.textSpeedAccum = 0x4000;
         }
      } else {
         OBJ.textSpeedAccum += OBJ.textSpeed;
      }

      while ((OBJ.textSpeedAccum >> 8) > 0) {
         if (*p == '\0') {
            // NUL
            if (OBJ.readingFromStringTable) {
               // End of string table string; resume from after insertion point
               OBJ.textPtr = OBJ.textResumePtr;
               p = OBJ.textPtr;
               OBJ.readingFromStringTable = 0;
            } else {
               obj->state = 5;
               return;
            }
         } else if ((*p >= 0x81 && *p <= 0x9f) || (*p >= 0xe0 && *p <= 0xfc)) {
            // SJIS
            if (obj->x3.n > maxCharsPerLine) {
               obj->x3.n = OBJ.indentChars;
               obj->y3.n++;
               if (obj->y3.n >= OBJ.maxRows) {
                  obj->state = 4;
                  return;
               }
            }
            sjis = (p[0] << 8) | p[1];
            MsgBox_DrawSjisGlyph(obj, sjis);
            OBJ.textPtr += 2;
            p += 2;
            obj->x3.n++;
            OBJ.textSpeedAccum -= 0x100;
            if (sjis > 0x823f && OBJ.speakAnimSuppressed == 0) {
               if (gState.vsyncMode != 2) {
                  n = 6;
               } else {
                  n = 3;
               }
               gState.msgTextWaitTimer[OBJ.type] = n;
            }
         } else {
            // ASCII
            switch (*p) {
            case '\n':
               obj->x3.n = OBJ.indentChars;
               obj->y3.n++;
               OBJ.textPtr++;
               p++;
               if (obj->y3.n >= OBJ.maxRows) {
                  if (*p == '\0') {
                     obj->state = 5;
                  } else {
                     obj->state = 4;
                  }
                  return;
               }
               break;

            // JP flattens the US nested `$` switch: the `$` byte falls to `default`
            // (consumed there), and its command letters are top-level cases. Safe because
            // none of these bytes is a valid SJIS lead byte.
            case 'W':
            case 'w':
               // Wait for button press
               OBJ.textPtr++;
               if (p[1] == '\n') {
                  OBJ.textPtr++;
               }
               obj->state = 4;
               return;

            case 'F':
            case 'f':
               gState.msgBoxPagePaused = 1;
               OBJ.textPtr++;
               return;

            case 'P':
            case 'p':
               obj->state = 6;
               OBJ.textPtr++;
               return;

            case 'S':
            case 's':
               // Set text speed
               n = ParseDigits(p + 1, &parsedInt);
               OBJ.textPtr += n;
               p = OBJ.textPtr;
               OBJ.textSpeed = parsedInt;
               continue;

            case 'T':
            case 't':
               // Delay
               n = ParseDigits(p + 1, &parsedInt);
               OBJ.textPtr += n;
               obj->state3 = parsedInt;
               obj->state = 3;
               return;

            case 'O':
            case 'o':
               OBJ.speakAnimSuppressed++;
               OBJ.speakAnimSuppressed %= 2;
               p++;
               OBJ.textPtr++;
               break;

            case '#':
               // JP: no `##` escape guard; a # always indexes the string table.
               OBJ.readingFromStringTable = 1;
               p++;
               n = ParseDigits(p, &parsedInt);
               p += n - 1;
               OBJ.textResumePtr = p;
               s_stringTable_80123348 = gStringTable;
               OBJ.textPtr = gStringTable[parsedInt];
               p = OBJ.textPtr;
               continue;

            // JP has no ASCII glyph path: any other byte just advances.
            default:
               p++;
               OBJ.textPtr++;
               break;
            }
         }
      }
      break;

   case 3:
      if (--obj->state3 <= 0 || (gPadStateNewPresses & PAD_X) ||
          (gPadStateNewPresses & PAD_CIRCLE)) {
         obj->state = 2;
      }
      break;

   case 4:
      buttonIcon = OBJ.buttonIcon;
      UpdateObjAnimation(buttonIcon);
      AddObjPrim_Gui(gGraphicsPtr->ot, buttonIcon);
      if ((gPadStateNewPresses & PAD_CIRCLE) || (gPadStateNewPresses & PAD_X)) {
         rect.x = OBJ.rect.x;
         rect.y = OBJ.rect.y;
         rect.w = OBJ.rect.w;
         rect.h = OBJ.rect.h;
         ClearImage(&rect, 0, 0, 0);
         obj->state3 = 1;
         obj->state = 1;
         obj->x3.n = 0;
         obj->y3.n = 0;
         OBJ.textSpeedAccum = 0;
         OBJ.fastForward = 0;
      }
      break;

   case 5:
      if ((gPadStateNewPresses & PAD_CIRCLE) || (gPadStateNewPresses & PAD_X)) {
         obj->state++;
      }
      break;

   case 6:
      MsgBox_Clear(obj);
      break;
   }
}

void DrawSjisText_Internal(s32 x, s32 y, s32 maxCharsPerLine, s32 lineSpacing, s32 color_, u8 *text,
                           s32 gapType) {
   s32 readingFromStringTable;
   u8 *insertionPoint;
   u8 *p;
#ifdef PC_PORT
   s32 guardSteps = 0;   /* unterminated-input bound; see the loop-top check */
#endif
   s32 n;
   s32 pad;
   s32 column;
   s32 rowY;
   s32 unk_s3;
   s32 parsedInt;
   s32 color;
   s32 rowHeight;
   u32 sjis;

   readingFromStringTable = 0;
   unk_s3 = 0;
   x = (x + (512 * 4)) / 4;
   color = color_;
   rowHeight = lineSpacing + 15;
   p = text;
   rowY = 0;
   column = 0;

   while (1) {
#ifdef PC_PORT
      /* Unterminated-input bound (2026-08-22, debug-menu warp, gdb first-chance in this loop):
       * a garbage string walked ~60 glyphs past the screen edge and off a mapped page. Real
       * draws are far below 4096 glyph/control steps per call; bail instead. */
      if (++guardSteps > 4096) {
         return;
      }
#endif
      switch (*p) {
      case '\0':
         if (readingFromStringTable) {
            // End of string table string; resume from after insertion point
            p = insertionPoint;
            readingFromStringTable = 0;
            continue;
         }
         // End of text
         return;

      case '\n':
         p++;
         column = 0;
         if (unk_s3 == 0) {
            rowY += rowHeight;
         } else if (unk_s3 >= 0) {
            if (unk_s3 < 3) {
               rowY += (lineSpacing + 8);
               unk_s3 = 0;
            }
         }
         continue;

      case '#':
         // JP: no `##` escape guard; a # always indexes the string table.
         readingFromStringTable = 1;
         p++;
         n = ParseDigits(p, &parsedInt);
         p += n - 1;
         insertionPoint = p;
         s_stringTable_80123348 = gStringTable;
         p = gStringTable[parsedInt];
         continue;

      case 'U':
         p++;
         unk_s3 = 1;
         continue;

      case 'D':
         p++;
         unk_s3 = 2;
         continue;
      }

      pad = 0;
      if (gapType != 0) {
         if (column * (12 >> 2) + x > 573) {
            pad = 2;
         } else {
            pad = 0;
         }
      } else {
         if (column * (12 >> 2) + x > 571) {
            pad = 4;
         }
      }
      sjis = (p[0] << 8) | p[1];
      if (unk_s3 == 0) {
         DrawSjisGlyph(sjis, column * (12 >> 2) + x + pad, rowY + y, color);
      } else if (unk_s3 == 1) {
         DrawSjisGlyphUD(sjis, column * (12 >> 2) + x + pad, rowY + y, color, 1);
      } else if (unk_s3 == 2) {
         DrawSjisGlyphUD(sjis, column * (12 >> 2) + x + pad, rowY + y, color, 0);
      }
      p += 2;
      column++;
      if (column >= maxCharsPerLine) {
         column = 0;
         if (unk_s3 == 0) {
            rowY += rowHeight;
         } else if (unk_s3 >= 0) {
            if (unk_s3 < 3) {
               rowY += (lineSpacing + 8);
               unk_s3 = 0;
            }
         }
      }
   }
}

void DrawText(s32 x, s32 y, s32 maxCharsPerLine, s32 lineSpacing, s32 color, u8 *text) {
   // JP has no ASCII path: DrawText routes through the SJIS renderer (== DrawSjisText).
   DrawSjisText_Internal(x, y, maxCharsPerLine, lineSpacing, color, text, 1);
}


void MsgBox_SetText(s32 type, s32 textPtrIdx, s32 textSpeed) {
   s32 i;
   Object *p;
   Object *msg;

   p = gObjectArray;
   for (i = 0; i < OBJ_DATA_CT; i++) {
      if (p->functionIndex == OBJF_MSGBOX_TEXT) {
         MsgBox_Clear(p);
      }
      p++;
   }

   msg = Obj_GetUnused();
   msg->functionIndex = OBJF_MSGBOX_TEXT;
   msg->d.objf351.type = type;
   msg->d.objf351.textPtrIdx = textPtrIdx;
   msg->d.objf351.textSpeed = textSpeed;
}

void MsgBox_SetText2(s32 type, s32 textPtrIdx, s32 textSpeed) {
   s32 i;
   Object *p;
   Object *msg;

   p = gObjectArray;
   for (i = 0; i < OBJ_DATA_CT; i++) {
      if (p->functionIndex == OBJF_MSGBOX_TEXT) {
         MsgBox_Clear(p);
      }
      p++;
   }

   msg = Obj_GetLastUnused();
   msg->functionIndex = OBJF_MSGBOX_TEXT;
   msg->d.objf351.type = type;
   msg->d.objf351.textPtrIdx = textPtrIdx;
   msg->d.objf351.textSpeed = textSpeed;
}

#undef OBJF
#define OBJF 798
void Objf798_ResetInputState(Object *obj) {
   gPadStateNewPresses = 0;
   gPadState = 0;
}
