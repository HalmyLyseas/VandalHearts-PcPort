#include "common.h"
#include "graphics.h"
#include "object.h"
#include "state.h"
#include "glyphs.h"

/* TODO: unmatched rodata — a string pool (world-map location + job class names)
 * followed by what look like two pointer tables into it. Bytes verified exact
 * against the original ROM; not yet decompiled into real named C structures. */
static const u8 D_800151C8[888] = {
   0x57, 0x65, 0x69, 0x72, 0x64, 0x20, 0x6d, 0x61, 0x6e, 0x00, 0x00, 0x00, 0x59, 0x6f, 0x75, 0x6e,
   0x67, 0x20, 0x6d, 0x61, 0x6e, 0x00, 0x00, 0x00, 0x42, 0x61, 0x72, 0x74, 0x65, 0x6e, 0x64, 0x65,
   0x72, 0x00, 0x00, 0x00, 0x4c, 0x65, 0x61, 0x76, 0x65, 0x20, 0x62, 0x61, 0x72, 0x00, 0x00, 0x00,
   0x54, 0x72, 0x61, 0x6e, 0x73, 0x66, 0x6f, 0x72, 0x6d, 0x0a, 0x46, 0x6f, 0x72, 0x67, 0x65, 0x74,
   0x20, 0x69, 0x74, 0x00, 0x47, 0x72, 0x6f, 0x77, 0x74, 0x68, 0x0a, 0x37, 0x20, 0x50, 0x6f, 0x77,
   0x65, 0x72, 0x73, 0x0a, 0x48, 0x69, 0x6e, 0x74, 0x73, 0x0a, 0x4c, 0x65, 0x67, 0x65, 0x6e, 0x64,
   0x0a, 0x51, 0x75, 0x69, 0x74, 0x00, 0x00, 0x00, 0x47, 0x75, 0x69, 0x64, 0x61, 0x6e, 0x63, 0x65,
   0x0a, 0x41, 0x64, 0x76, 0x61, 0x6e, 0x63, 0x65, 0x0a, 0x4c, 0x65, 0x61, 0x76, 0x65, 0x20, 0x44,
   0x6f, 0x6a, 0x6f, 0x00, 0x4d, 0x61, 0x67, 0x69, 0x63, 0x20, 0x54, 0x72, 0x61, 0x69, 0x6e, 0x20,
   0x53, 0x74, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x00, 0x4f, 0x6c, 0x64, 0x20, 0x74, 0x6f, 0x77, 0x6e,
   0x20, 0x77, 0x61, 0x72, 0x65, 0x68, 0x6f, 0x75, 0x73, 0x65, 0x00, 0x00, 0x43, 0x61, 0x72, 0x6c,
   0x6f, 0x27, 0x73, 0x20, 0x6d, 0x61, 0x6e, 0x73, 0x69, 0x6f, 0x6e, 0x00, 0x47, 0x72, 0x6f, 0x67,
   0x27, 0x73, 0x20, 0x68, 0x6f, 0x75, 0x73, 0x65, 0x00, 0x00, 0x00, 0x00, 0x44, 0x6f, 0x76, 0x65,
   0x72, 0x20, 0x64, 0x69, 0x73, 0x74, 0x72, 0x69, 0x63, 0x74, 0x00, 0x00, 0x53, 0x65, 0x63, 0x75,
   0x72, 0x69, 0x74, 0x79, 0x20, 0x46, 0x6f, 0x72, 0x63, 0x65, 0x20, 0x48, 0x51, 0x00, 0x00, 0x00,
   0x47, 0x61, 0x64, 0x61, 0x72, 0x20, 0x53, 0x65, 0x61, 0x00, 0x00, 0x00, 0x53, 0x61, 0x6e, 0x64,
   0x20, 0x44, 0x75, 0x6e, 0x65, 0x73, 0x20, 0x6f, 0x66, 0x20, 0x53, 0x65, 0x61, 0x00, 0x00, 0x00,
   0x4c, 0x65, 0x61, 0x76, 0x65, 0x20, 0x56, 0x69, 0x6c, 0x6c, 0x61, 0x67, 0x65, 0x00, 0x00, 0x00,
   0x4c, 0x65, 0x61, 0x76, 0x65, 0x20, 0x54, 0x6f, 0x77, 0x6e, 0x00, 0x00, 0x43, 0x6f, 0x62, 0x61,
   0x6c, 0x74, 0x20, 0x42, 0x65, 0x61, 0x63, 0x68, 0x00, 0x00, 0x00, 0x00, 0x46, 0x6f, 0x72, 0x74,
   0x20, 0x47, 0x61, 0x72, 0x65, 0x74, 0x68, 0x00, 0x47, 0x6c, 0x61, 0x73, 0x67, 0x6f, 0x77, 0x20,
   0x43, 0x69, 0x74, 0x79, 0x00, 0x00, 0x00, 0x00, 0x4f, 0x72, 0x6f, 0x6d, 0x65, 0x20, 0x4c, 0x61,
   0x6b, 0x65, 0x00, 0x00, 0x53, 0x6d, 0x6f, 0x6b, 0x69, 0x6e, 0x67, 0x20, 0x42, 0x6f, 0x6e, 0x65,
   0x73, 0x20, 0x43, 0x61, 0x76, 0x65, 0x00, 0x00, 0x54, 0x73, 0x75, 0x6b, 0x75, 0x65, 0x20, 0x70,
   0x6c, 0x61, 0x69, 0x6e, 0x73, 0x00, 0x00, 0x00, 0x46, 0x72, 0x6f, 0x6e, 0x74, 0x69, 0x65, 0x72,
   0x20, 0x76, 0x69, 0x6c, 0x6c, 0x61, 0x67, 0x65, 0x00, 0x00, 0x00, 0x00, 0x54, 0x6f, 0x72, 0x6f,
   0x67, 0x20, 0x4d, 0x6f, 0x75, 0x6e, 0x74, 0x61, 0x69, 0x6e, 0x73, 0x00, 0x50, 0x69, 0x6f, 0x6e,
   0x65, 0x65, 0x72, 0x20, 0x74, 0x6f, 0x77, 0x6e, 0x20, 0x53, 0x6f, 0x72, 0x62, 0x6f, 0x00, 0x00,
   0x57, 0x61, 0x72, 0x74, 0x20, 0x42, 0x61, 0x79, 0x00, 0x00, 0x00, 0x00, 0x46, 0x6f, 0x72, 0x74,
   0x20, 0x44, 0x61, 0x69, 0x6e, 0x00, 0x00, 0x00, 0x52, 0x61, 0x69, 0x6c, 0x72, 0x6f, 0x61, 0x64,
   0x00, 0x00, 0x00, 0x00, 0x54, 0x72, 0x61, 0x64, 0x65, 0x20, 0x43, 0x69, 0x74, 0x79, 0x20, 0x4b,
   0x65, 0x72, 0x61, 0x63, 0x68, 0x69, 0x00, 0x00, 0x47, 0x72, 0x65, 0x61, 0x74, 0x20, 0x4d, 0x61,
   0x73, 0x61, 0x69, 0x20, 0x46, 0x6f, 0x72, 0x65, 0x73, 0x74, 0x00, 0x00, 0x4b, 0x68, 0x61, 0x6e,
   0x6f, 0x73, 0x20, 0x43, 0x69, 0x74, 0x79, 0x00, 0x50, 0x72, 0x69, 0x73, 0x6f, 0x6e, 0x20, 0x46,
   0x6f, 0x72, 0x74, 0x72, 0x65, 0x73, 0x73, 0x00, 0x52, 0x65, 0x65, 0x64, 0x20, 0x48, 0x69, 0x67,
   0x68, 0x77, 0x61, 0x79, 0x00, 0x00, 0x00, 0x00, 0x42, 0x61, 0x73, 0x69, 0x6c, 0x20, 0x47, 0x61,
   0x74, 0x65, 0x00, 0x00, 0x54, 0x65, 0x72, 0x61, 0x73, 0x75, 0x20, 0x56, 0x69, 0x6c, 0x6c, 0x61,
   0x67, 0x65, 0x00, 0x00, 0x52, 0x65, 0x6d, 0x6f, 0x74, 0x65, 0x20, 0x43, 0x69, 0x74, 0x79, 0x00,
   0x50, 0x6c, 0x61, 0x69, 0x6e, 0x20, 0x6f, 0x66, 0x20, 0x53, 0x6f, 0x72, 0x72, 0x6f, 0x77, 0x00,
   0x43, 0x61, 0x73, 0x74, 0x6c, 0x65, 0x20, 0x72, 0x75, 0x69, 0x6e, 0x73, 0x00, 0x00, 0x00, 0x00,
   0x4c, 0x6f, 0x72, 0x69, 0x73, 0x20, 0x42, 0x65, 0x61, 0x63, 0x68, 0x00, 0x59, 0x67, 0x64, 0x72,
   0x61, 0x20, 0x43, 0x61, 0x6e, 0x79, 0x6f, 0x6e, 0x00, 0x00, 0x00, 0x00, 0x56, 0x69, 0x6c, 0x6c,
   0x61, 0x67, 0x65, 0x20, 0x6f, 0x66, 0x20, 0x59, 0x75, 0x7a, 0x75, 0x00, 0x50, 0x6f, 0x72, 0x74,
   0x20, 0x43, 0x69, 0x74, 0x79, 0x20, 0x4d, 0x69, 0x6e, 0x61, 0x74, 0x6f, 0x00, 0x00, 0x00, 0x00,
   0x52, 0x68, 0x69, 0x6e, 0x65, 0x20, 0x42, 0x72, 0x69, 0x64, 0x67, 0x65, 0x00, 0x00, 0x00, 0x00,
   0x50, 0x61, 0x6c, 0x61, 0x63, 0x65, 0x20, 0x72, 0x75, 0x69, 0x6e, 0x73, 0x00, 0x00, 0x00, 0x00,
   0x43, 0x61, 0x70, 0x69, 0x74, 0x61, 0x6c, 0x20, 0x53, 0x68, 0x75, 0x6d, 0x65, 0x72, 0x69, 0x61,
   0x00, 0x00, 0x00, 0x00, 0x56, 0x61, 0x6c, 0x6c, 0x65, 0x79, 0x20, 0x6f, 0x66, 0x20, 0x54, 0x68,
   0x69, 0x65, 0x76, 0x65, 0x73, 0x00, 0x00, 0x00, 0x56, 0x61, 0x6e, 0x64, 0x61, 0x6c, 0x69, 0x65,
   0x72, 0x00, 0x00, 0x00, 0x41, 0x72, 0x63, 0x68, 0x62, 0x69, 0x73, 0x68, 0x6f, 0x70, 0x00, 0x00,
   0x45, 0x6e, 0x63, 0x68, 0x61, 0x6e, 0x74, 0x65, 0x72, 0x00, 0x00, 0x00, 0x53, 0x6f, 0x72, 0x63,
   0x65, 0x72, 0x65, 0x72, 0x00, 0x00, 0x00, 0x00, 0x53, 0x6b, 0x79, 0x20, 0x4c, 0x6f, 0x72, 0x64,
   0x00, 0x00, 0x00, 0x00, 0x48, 0x61, 0x77, 0x6b, 0x6e, 0x69, 0x67, 0x68, 0x74, 0x00, 0x00, 0x00,
   0x47, 0x75, 0x61, 0x72, 0x64, 0x73, 0x6d, 0x61, 0x6e, 0x00, 0x00, 0x00, 0x53, 0x77, 0x6f, 0x72,
   0x64, 0x73, 0x6d, 0x61, 0x6e, 0x00, 0x00, 0x00, 0x43, 0x68, 0x61, 0x6d, 0x70, 0x69, 0x6f, 0x6e,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

/* TODO: unmatched .sdata — a run of small (<=8 byte) NUL-padded strings
 * (job/character names). Bytes verified exact against the original ROM;
 * split into 8-byte chunks so each stays under -G8 and lands in .sdata
 * like the original; not yet decompiled into real named C structures. */
static u8 D_80122FB0[8] = { 0x4f, 0x6c, 0x64, 0x20, 0x6d, 0x61, 0x6e, 0x00 };
static u8 D_80122FB8[8] = { 0x57, 0x6f, 0x6d, 0x61, 0x6e, 0x00, 0x00, 0x00 };
static u8 D_80122FC0[8] = { 0x4d, 0x61, 0x6e, 0x00, 0x54, 0x61, 0x6c, 0x6b };
static u8 D_80122FC8[8] = { 0x20, 0x74, 0x6f, 0x00, 0x54, 0x61, 0x76, 0x65 };
static u8 D_80122FD0[8] = { 0x72, 0x6e, 0x00, 0x00, 0x44, 0x6f, 0x6a, 0x6f };
static u8 D_80122FD8[8] = { 0x00, 0x00, 0x00, 0x00, 0x53, 0x68, 0x6f, 0x70 };
static u8 D_80122FE0[8] = { 0x00, 0x00, 0x00, 0x00, 0x20, 0x20, 0x00, 0x00 };
static u8 D_80122FE8[8] = { 0x42, 0x69, 0x73, 0x68, 0x6f, 0x70, 0x00, 0x00 };
static u8 D_80122FF0[8] = { 0x4e, 0x69, 0x6e, 0x6a, 0x61, 0x00, 0x00, 0x00 };
static u8 D_80122FF8[8] = { 0x4d, 0x6f, 0x6e, 0x6b, 0x00, 0x00, 0x00, 0x00 };
static u8 D_80123000[8] = { 0x53, 0x6e, 0x69, 0x70, 0x65, 0x72, 0x00, 0x00 };
static u8 D_80123008[8] = { 0x42, 0x6f, 0x77, 0x6d, 0x61, 0x6e, 0x00, 0x00 };
static u8 D_80123010[8] = { 0x44, 0x72, 0x61, 0x67, 0x6f, 0x6f, 0x6e, 0x00 };
static u8 D_80123018[8] = { 0x44, 0x75, 0x65, 0x6c, 0x69, 0x73, 0x74, 0x00 };
static u8 D_80123020[8] = { 0x50, 0x61, 0x72, 0x61, 0x67, 0x6f, 0x6e, 0x00 };
static u8 D_80123028[8] = { 0x51, 0x75, 0x69, 0x74, 0x00, 0x00, 0x00, 0x00 };
static u8 D_80123030[8] = { 0x44, 0x65, 0x70, 0x6f, 0x74, 0x00, 0x00, 0x00 };
static u8 D_80123038[8] = { 0x44, 0x61, 0x72, 0x69, 0x75, 0x73, 0x00, 0x00 };
static u8 D_80123040[8] = { 0x5a, 0x6f, 0x68, 0x61, 0x72, 0x00, 0x00, 0x00 };
static u8 D_80123048[8] = { 0x53, 0x61, 0x72, 0x61, 0x00, 0x00, 0x00, 0x00 };
static u8 D_80123050[8] = { 0x41, 0x6d, 0x6f, 0x6e, 0x00, 0x00, 0x00, 0x00 };
static u8 D_80123058[8] = { 0x44, 0x6f, 0x6c, 0x61, 0x6e, 0x00, 0x00, 0x00 };
static u8 D_80123060[8] = { 0x47, 0x72, 0x6f, 0x67, 0x00, 0x00, 0x00, 0x00 };
static u8 D_80123068[8] = { 0x4b, 0x69, 0x72, 0x61, 0x00, 0x00, 0x00, 0x00 };
static u8 D_80123070[8] = { 0x48, 0x75, 0x78, 0x6c, 0x65, 0x79, 0x00, 0x00 };
static u8 D_80123078[8] = { 0x45, 0x6c, 0x65, 0x6e, 0x69, 0x00, 0x00, 0x00 };
static u8 D_80123080[8] = { 0x44, 0x69, 0x65, 0x67, 0x6f, 0x00, 0x00, 0x00 };
static u8 D_80123088[8] = { 0x43, 0x6c, 0x69, 0x6e, 0x74, 0x00, 0x00, 0x00 };
static u8 D_80123090[8] = { 0x41, 0x73, 0x68, 0x00, 0x00, 0x00, 0x00, 0x00 };


s32 CopySjisString(u8 *, u8 *);
s32 DecodeLineOfText(u8 *, u8 *);
void LoadText(s32, u8 *, u8 **);
s32 DrawSjisGlyph(u16, s32, s32, s32);
s32 MsgBox_DrawSjisGlyph(Object *, u16);
s32 MsgBox_DrawFontGlyph(Object *, s16);
s32 ParseDigits(u8 *, s32 *);
void MsgBox_Clear(Object *);
void Objf351_MsgBoxText(Object *);
u8 GetGlyphIdxForAsciiChar(u8);
void DrawFontGlyph(u8, s32, s32, s32);
void DrawText_Internal(s32, s32, s32, s32, s32, u8 *, s32);
void DrawSjisText_Internal(s32, s32, s32, s32, s32, u8 *, s32);
void DrawText(s32, s32, s32, s32, s32, u8 *);
void DrawSjisText(s32, s32, s32, s32, s32, u8 *);
void MsgBox_SetText(s32, s32, s32);
void MsgBox_SetText2(s32, s32, s32);
void Objf798_ResetInputState(Object *);

/* PERMUTER: 101 entries, not 100. src/dojo.c:511 deliberately sets `OBJ.partyIdx = 100` as a
 * "no selection yet" sentinel, and dojo.c:1007 then does
 * `gStringTable[32] = gStringTable[OBJ.partyIdx];` -- reading index 100 of a 100-entry table for
 * the one frame before a real party index is assigned. Found by the ASAN sweep during chapter 1.
 *
 * On hardware that reads `whiteShades` (0x801011bc, immediately after gStringTable's 400 bytes)
 * reinterpreted as a string pointer -- garbage, but the game evidently never displays string 32
 * on that frame. The PC port's generated string-table constructor normalizes this sentinel and all
 * retail NULL entries to a stable empty string, without relying on a platform fault handler.
 *
 * The extra entry is implicitly NULL, so behaviour is unchanged -- this only makes the existing
 * outcome deterministic instead of dependent on linker padding. The initializer list keeps
 * supplying exactly the 100 real entries, so nothing about the matching build shifts. */
#ifdef PC_PORT
/* The native port reconstructs these PSX-address pointers from the user's executable in
 * gen_string_table.py, then installs host pointers from a constructor. */
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

s32 DrawSjisGlyph(u16 sjis, s32 x, s32 y, s32 color) {
   // For anti-aliasing effect:
   static s32 whiteShades[5] = {0, 1, 2, 3, 4};
   static s32 redShades[5] = {0, 5, 6, 7, 8};

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

   if (sjis >= 0x83fd && sjis <= 0x843f) {
      return -1;
   }

   pInputData = (u8 *)Krom2RawAdd(sjis);
   if (pInputData == -1) {
      return -1;
   }

   if (y > 240) {
      return -2;
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

s32 MsgBox_DrawFontGlyph(Object *msg, s16 idx) {
   // x3: column, y3: row
   if (msg->x3.n < msg->d.objf351.pregapChars) {
      DrawFontGlyph(idx, 512 + msg->x3.n * (8 >> 2) + msg->x1.n,
                    msg->y3.n * (msg->d.objf351.lineSpacing + 15) + msg->y1.n +
                        sMsgBoxVramOffsets[msg->d.objf351.type * 2][1],
                    0);
      return 0;
   } else if (msg->x3.n < msg->d.objf351.maxCharsPerLine) {
      DrawFontGlyph(idx, 576 + (msg->x3.n - msg->d.objf351.pregapChars) * (8 >> 2),
                    msg->y3.n * (msg->d.objf351.lineSpacing + 15) + msg->y1.n +
                        sMsgBoxVramOffsets[msg->d.objf351.type * 2 + 1][1],
                    0);
      return 0;
   } else {
      return -1;
   }
}

#ifdef PC_FEAT
/* Language packs (platform/pc/src/pc_lang_font.c): UTF-8 sibling of MsgBox_DrawFontGlyph, with
 * IDENTICAL position math -- the glyph just comes from the pack's codepoint table instead of
 * sFontGlyphBitmaps. Consumes the WHOLE multi-byte sequence and returns its byte length, or 0 when
 * *p is not a pack-drawable sequence start (plain ASCII / no pack -- the retail path then runs
 * untouched). Past maxCharsPerLine it clips exactly like the retail path (nothing drawn) but still
 * consumes, so continuation bytes can never leak into the parser. */
static s32 MsgBox_DrawLangUtf8(Object *msg, u8 *p) {
   extern s32 PC_LangUtf8Glyph(u8 **pp, s32 px, s32 py, s32 color);
   extern s32 PC_LangUtf8SeqLen(u8 *pB);
   u8 *q = p;

   if (msg->x3.n < msg->d.objf351.pregapChars) {
      PC_LangUtf8Glyph(&q, 512 + msg->x3.n * (8 >> 2) + msg->x1.n,
                       msg->y3.n * (msg->d.objf351.lineSpacing + 15) + msg->y1.n +
                           sMsgBoxVramOffsets[msg->d.objf351.type * 2][1],
                       0);
   } else if (msg->x3.n < msg->d.objf351.maxCharsPerLine) {
      PC_LangUtf8Glyph(&q, 576 + (msg->x3.n - msg->d.objf351.pregapChars) * (8 >> 2),
                       msg->y3.n * (msg->d.objf351.lineSpacing + 15) + msg->y1.n +
                           sMsgBoxVramOffsets[msg->d.objf351.type * 2 + 1][1],
                       0);
   } else {
      return PC_LangUtf8SeqLen(p);
   }
   return (s32)(q - p);
}
#endif

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
      gState.field_0x31d = 0;
      obj->state3 = 1;
      OBJ.todo_x48 = 0;

      OBJ.textSpeed = textSpeeds[gState.textSpeed & 7];
      if (gState.vsyncMode != 2) {
         OBJ.textSpeed >>= 1;
      }

      OBJ.maxCharsPerLine = 26;
      OBJ.lineSpacing = 1;
      OBJ.maxRows = 3;

      buttonIconX = buttonIconPositions[OBJ.type - 1].x;
      buttonIconY = buttonIconPositions[OBJ.type - 1].y;

      switch (OBJ.type) {
      case 0:
      case 1:
         obj->x1.n = 64 >> 2;
         obj->y1.n = 8;
         OBJ.pregapChars = (240 - obj->x1.n * 4) / 8;
         break;
      case 2:
         obj->x1.n = 12 >> 2;
         obj->y1.n = 8;
         OBJ.pregapChars = (240 - obj->x1.n * 4) / 8;
         break;
      case 3:
         obj->x1.n = 80 >> 2;
         obj->y1.n = 22;
         OBJ.pregapChars = (248 - obj->x1.n * 4) / 8;
         OBJ.type = 1;
         break;
      case 4:
         obj->x1.n = 24 >> 2;
         obj->y1.n = 22;
         OBJ.pregapChars = (248 - obj->x1.n * 4) / 8;
         OBJ.type = 2;
         break;
      case 5:
         obj->x1.n = 80 >> 2;
         obj->y1.n = 22;
         OBJ.pregapChars = (248 - obj->x1.n * 4) / 8;
         OBJ.type = 1;
         break;
      case 6:
         obj->x1.n = 80 >> 2;
         obj->y1.n = 22;
         OBJ.pregapChars = (248 - obj->x1.n * 4) / 8;
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
      OBJ.todo_x44 = 0;

      OBJ.rect.x = obj->x1.n + 512;
      if (OBJ.type == 1) {
         OBJ.rect.y = obj->y1.n;
      } else if (OBJ.type == 2) {
         OBJ.rect.y = obj->y1.n + 100;
      }
      OBJ.rect.w = 64 + (OBJ.maxCharsPerLine - OBJ.pregapChars) * (12 >> 2) - obj->x1.n; //
      OBJ.rect.w = 64 + (OBJ.maxCharsPerLine - OBJ.pregapChars) * (8 >> 2) - obj->x1.n;
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
      if (OBJ.todo_x44 == 0 && (gPadStateNewPresses & PAD_CIRCLE)) {
         OBJ.todo_x44 = 1;
      }
      if (OBJ.todo_x45 != 0 && !(gPadState & PAD_CIRCLE)) {
         OBJ.todo_x44 = 0;
      }
      if ((gPadState & PAD_CIRCLE) && (OBJ.todo_x44 != 0)) {
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
               gState.field_0x31d = 1;
               obj->state = 5;
               return;
            }
         } else if ((*p >= 0x81 && *p <= 0x9f) || (*p >= 0xe0 && *p <= 0xfc)) {
            // SJIS
            if (obj->x3.n > maxCharsPerLine) {
               obj->x3.n = OBJ.indentChars;
               obj->y3.n++;
               if (obj->y3.n >= OBJ.maxRows) {
                  gState.field_0x31d = 1;
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
            if (sjis > 0x823f && OBJ.todo_x48 == 0) {
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
                     gState.field_0x31d = 1;
                     obj->state = 5;
                  } else {
                     gState.field_0x31d = 1;
                     obj->state = 4;
                  }
                  return;
               }
               break;

            case '$':
               // Command code, e.g. $T for a timed delay
               p++;
               OBJ.textPtr++;

               switch (*p) {
               case 'W':
               case 'w':
                  // Wait for button press
                  p++;
                  OBJ.textPtr++;
                  if (*p == '\n') {
                     OBJ.textPtr++;
                  }
                  gState.field_0x31d = 1;
                  obj->state = 4;
                  return;

               case 'F':
               case 'f':
                  //? Flag for e.g. event entity animation?
                  gState.field_0x31d = 1;
                  OBJ.textPtr++;
                  return;

               case 'P':
               case 'p':
                  gState.field_0x31d = 1;
                  obj->state = 6;
                  OBJ.textPtr++;
                  return;

               case 'S':
               case 's':
                  // Set text speed
                  p++;
                  n = ParseDigits(p, &parsedInt);
                  // ParseDigits returns numDigits+1 (which is used to skip past the 'S' here)
                  OBJ.textPtr += n;
                  p = OBJ.textPtr;
                  OBJ.textSpeed = parsedInt;
                  continue;

               case 'T':
               case 't':
                  // Delay
                  p++;
                  n = ParseDigits(p, &parsedInt);
                  OBJ.textPtr += n;
                  p = OBJ.textPtr;
                  obj->state3 = parsedInt;
                  obj->state = 3;
                  return;

               case 'O':
               case 'o':
                  OBJ.todo_x48++;
                  OBJ.todo_x48 %= 2;
                  p++;
                  OBJ.textPtr++;
                  continue;
               }

               break;

            case '#':
               p++;
               if (*p != '#') {
                  // Single # specifies an index into the string table
                  OBJ.readingFromStringTable = 1;
                  n = ParseDigits(p, &parsedInt);
                  p += n - 1;
                  OBJ.textResumePtr = p;
                  s_stringTable_80123348 = gStringTable;
                  OBJ.textPtr = gStringTable[parsedInt];
                  p = OBJ.textPtr;
                  continue;
               }
               // Double # indicates escaped #; fall-through to handle
               OBJ.textPtr++;

            // fallthrough
            default:
#ifdef PC_FEAT
               /* Language packs: pack dialogue is UTF-8. Offer the byte to the pack engine first --
                * a consumed sequence is ONE character for every purpose below (column, pacing,
                * letter wait-timer); on 0 the retail path runs verbatim. */
               {
                  s32 uLen = MsgBox_DrawLangUtf8(obj, p);
                  if (uLen != 0) {
                     if (gState.vsyncMode != 2) {
                        n = 6;
                     } else {
                        n = 3;
                     }
                     gState.msgTextWaitTimer[OBJ.type] = n;
                     p += uLen;
                     OBJ.textPtr += uLen;
                     obj->x3.n++;
                     OBJ.textSpeedAccum -= 0x100;
                     break;
                  }
               }
#endif
               MsgBox_DrawFontGlyph(obj, GetGlyphIdxForAsciiChar(*p));
               if (*p >= 'A' && *p <= 'z') {
                  if (gState.vsyncMode != 2) {
                     n = 6;
                  } else {
                     n = 3;
                  }
                  gState.msgTextWaitTimer[OBJ.type] = n;
               }
               p++;
               OBJ.textPtr++;
               obj->x3.n++;
               OBJ.textSpeedAccum -= 0x100;
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
         gState.field_0x31d = 1;
         OBJ.textSpeedAccum = 0;
         OBJ.todo_x44 = 0;
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

#ifdef PC_FEAT
/* Forward declaration for the hand-off below: the initialized definition sits after this function
 * (same TU, same PERMUTER-conditional size). */
#ifdef PERMUTER
static u8 sFontGlyphBitmaps[156][9];
#else
static u8 sFontGlyphBitmaps[128][9];
#endif
#endif

u8 GetGlyphIdxForAsciiChar(u8 asc) {
   static u8 mappings[128] = {
       128, 0,  0,  0,   0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,
       0,   0,  0,  0,   0,  0,  0,  0,  0,  0,  128, 97, 40, 42, 0,  99, 0,  39,  0,  0,  0,  102,
       95,  12, 94, 101, 2,  3,  4,  5,  6,  7,  8,   9,  10, 11, 96, 0,  41, 100, 0,  98, 0,  68,
       69,  70, 71, 72,  73, 74, 75, 76, 77, 78, 79,  80, 81, 82, 83, 84, 85, 86,  87, 88, 89, 90,
       91,  92, 93, 0,   0,  0,  0,  0,  0,  68, 69,  70, 71, 72, 73, 74, 75, 76,  77, 78, 79, 80,
       81,  82, 83, 84,  85, 86, 87, 88, 89, 90, 91,  92, 93, 0,  0,  0,  0,  0};

#ifdef PC_FEAT
   /* Language packs (platform/pc/src/pc_lang.c): both the code->glyph map above and the glyph
    * bitmaps are function/file-STATIC, so -- like battle_0201b8.c's terrainText -- they are handed
    * to the language layer once, on first use. A pack's charmap section can then assign free byte
    * codes to free glyph slots (writing pack bitmaps into them) and remap existing codes (the
    * mixed-case option reuses the lowercase art already present at indices 13-38). Every consumer
    * of this map -- DrawText, the message box, StringToGlyphs -- picks the changes up for free. */
   {
      static s32 langApplied = 0;
      if (!langApplied) {
         extern void PC_LangApplyCharmap(u8 *map128, u8 (*glyphs)[9], s32 glyphCount);
         langApplied = 1;
         PC_LangApplyCharmap(mappings, sFontGlyphBitmaps, sizeof(sFontGlyphBitmaps) / 9);
      }
   }
#endif

   return mappings[asc & 0x7f];
}

/* PC PORT NOTE (not a decompile change -- see exchange/12-phase-c-bootstrap.md Bug 10):
 * GetGlyphIdxForAsciiChar() maps the space character to glyph index 128, one past this
 * array's original 128-row bound, and DrawFontGlyph()'s own guard (`glyphIdx <= 128`)
 * deliberately allows that. On real hardware this out-of-bounds read is harmless: the
 * original linker happens to place all-zero bytes immediately after this array (confirmed
 * directly against SLUS_004.47, file offset 0xf1f64), so "row 128" reads as a blank glyph.
 * Our own compiler's completely different static-data layout doesn't reproduce that
 * placement -- without this, DrawFontGlyph(128, ...) reads whatever unrelated data our own
 * build happens to place there instead, visible as a stray glyph-shaped mark wherever a
 * space should render (a real, reported artifact, not a hypothetical). Two separately
 * declared globals aren't guaranteed adjacent in memory (that would just trade one undefined
 * behavior for another), so the fix widens the array itself by one row instead: C
 * zero-initializes any element an initializer list doesn't cover, so row 128 becomes a real,
 * guaranteed-contiguous, guaranteed-zero row -- the original 128 rows' data (from
 * assets/801012e4.inc) is completely unchanged.
 *
 * This widening is a PC-PORT-ONLY fix and MUST be gated: growing the array adds rows to
 * text.c's .data, which shifts every following symbol and breaks the stage-1 byte-exact
 * match. The matching (PSX) build keeps the original [128] size -- there the OOB "row 128"
 * read lands on the linker's all-zero bytes at file offset 0xf1f64 (the following data
 * segment), reproducing the blank glyph exactly. `PERMUTER` is defined only by the
 * platform/pc build, never by the matching build.
 *
 * The PC size is [156] rather than [129] to give LANGUAGE PACKS somewhere to put new
 * letterforms (see platform/pc/include/pc_lang.h). A pack's charmap may claim slots
 * 111-127 and 129-155 -- 44 in total, enough for a whole non-Latin alphabet. Two slots
 * inside that span are NOT free and must never be assigned: 128 is where the retail map
 * sends NUL and space (it has to stay blank), and 1 is the window-background tile in the
 * F_WD sheet, which indexes the same numbers. Rows past the initializer are zero-filled by
 * C, so with no pack loaded they are blank AND unreachable -- the retail map's largest
 * value is 128, so nothing can address them. */
#ifdef PERMUTER
static u8 sFontGlyphBitmaps[156][9] = {
#else
static u8 sFontGlyphBitmaps[128][9] = {
#endif
#include "assets/801012e4.inc"
};

void DrawFontGlyph(u8 glyphIdx, s32 x, s32 y, s32 color) {
   s32 i, j, k;
   RECT rect;
   u16 buffer[32];
   u16 *pOutputData;
   u8 *pInputData;
   u8 byte;
   s16 output;
   s32 colorPlusOne;

   pInputData = &sFontGlyphBitmaps[glyphIdx][0];
   pOutputData = &buffer[0];

   /* The upper bound follows the array size above: retail stops at the blank row 128, the PC
    * build extends to the last language-pack slot. Without a pack nothing reaches past 128
    * anyway, since that is the retail map's largest value. */
#ifdef PERMUTER
   if (y < 247 && glyphIdx <= 155) {
#else
   if (y < 247 && glyphIdx <= 128) {
#endif
      colorPlusOne = color + 1;
      for (i = 0; i < 9; i++) {
         byte = *pInputData++;
         for (j = 0; j < 2; j++) {
            output = 0;
            for (k = 0; k < 4; k++) {
               output >>= 4;
               if (byte & 0x80) {
                  output += colorPlusOne << 14;
               }
               byte <<= 1;
            }
            *pOutputData++ = output;
         }
      }
      rect.x = x;
      rect.y = y;
      rect.w = 8 >> 2;
      rect.h = 9;
      LoadImage(&rect, buffer);
      DrawSync(0);
   }
}

void DrawText_Internal(s32 x, s32 y, s32 maxCharsPerLine, s32 lineSpacing, s32 color_, u8 *text,
                       s32 gapType) {
   s32 readingFromStringTable;
   u8 *insertionPoint;
   u8 *p;
   s32 n;
   s32 pad;
   s32 column;
   s32 rowY;
   s32 unk_s4;
   s32 parsedInt;
   s32 color;
   s32 rowHeight;

   readingFromStringTable = 0;
   unk_s4 = 0;
   x = (x + (512 * 4)) / 4;
   y += 3;
   color = color_;
   rowHeight = lineSpacing + 15; // s8=s7+15
   p = text;
   rowY = 0;   // s2
   column = 0; // s3

   while (1) {
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
         if (unk_s4 == 0) {
            rowY += rowHeight;
         } else if (unk_s4 >= 0) {
            if (unk_s4 < 3) {
               rowY += (lineSpacing + 8);
               unk_s4 = 0;
            }
         }
         continue;

      case '#':
         p++;
         if (*p != '#') {
            // Single # specifies an index into the string table
            readingFromStringTable = 1;
            n = ParseDigits(p, &parsedInt);
            p += n - 1;
            insertionPoint = p;
            s_stringTable_80123348 = gStringTable;
            p = gStringTable[parsedInt];
            continue;
         }
         // Double # indicates escaped #; fall-through to handle

      default:
         pad = 1;
         if (gapType != 0) {
            if (column * (8 >> 2) + x > 571) {
               pad = 3;
            }
         } else {
            // gapType is 0 when drawing battle spell/item description
            if (column * (8 >> 2) + x > 569) {
               pad = 5;
            }
         }
#ifdef PC_FEAT
         /* Language packs (platform/pc/src/pc_lang_font.c): pointer strings from a pack are UTF-8.
          * Offer the byte to the pack's font engine first -- if it starts a valid multi-byte
          * sequence, the WHOLE sequence is consumed and drawn as ONE glyph, and the shared
          * column/wrap arithmetic below runs exactly as for one retail character. Returns 0
          * untouched for plain ASCII or when no pack font is loaded, so retail text is unaffected. */
         {
            extern s32 PC_LangUtf8Glyph(u8 **pp, s32 px, s32 py, s32 color);
            if (PC_LangUtf8Glyph(&p, column * (8 >> 2) + x + pad, rowY + y, color) == 0) {
               DrawFontGlyph(GetGlyphIdxForAsciiChar(*p++), column * (8 >> 2) + x + pad, rowY + y,
                             color);
            }
         }
#else
         DrawFontGlyph(GetGlyphIdxForAsciiChar(*p++), column * (8 >> 2) + x + pad, rowY + y, color);
#endif
         column++;
         if (column >= maxCharsPerLine) {
            column = 0;
            if (unk_s4 == 0) {
               rowY += rowHeight;
            } else if (unk_s4 >= 0) {
               if (unk_s4 < 3) {
                  rowY += (lineSpacing + 8);
                  unk_s4 = 0;
               }
            }
         }
         continue;
      }
   }
}

void DrawSjisText_Internal(s32 x, s32 y, s32 maxCharsPerLine, s32 lineSpacing, s32 color_, u8 *text,
                           s32 gapType) {
   s32 readingFromStringTable;
   u8 *insertionPoint;
   u8 *p;
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
         p++;
         if (*p != '#') {
            // Single # specifies an index into the string table
            readingFromStringTable = 1;
            n = ParseDigits(p, &parsedInt);
            p += n - 1;
            insertionPoint = p;
            s_stringTable_80123348 = gStringTable;
            p = gStringTable[parsedInt];
            continue;
         }
         // Double # indicates escaped #; break to handle outside switch
         break;

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
      DrawSjisGlyph(sjis, column * (12 >> 2) + x + pad, rowY + y, color);
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
   DrawText_Internal(x, y, maxCharsPerLine, lineSpacing, color, text, 1);
}

void DrawSjisText(s32 x, s32 y, s32 maxCharsPerLine, s32 lineSpacing, s32 color, u8 *text) {
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
