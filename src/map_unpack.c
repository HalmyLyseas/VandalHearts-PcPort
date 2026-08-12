/* Map-file decompression, terrain/texture event objects, the event-script index, and the
 * shared unit animation-set data (segment 0xa2ce0).
 *
 * UnpackMapFileData decodes the M_*.PRS map files: one control byte supplies eight mode
 * bits, each selecting a literal byte or one of three back-reference forms (10-bit
 * absolute index into the dictionary, 4-bit relative distance, or a run of up to 70
 * literals). The dictionary is a 1KB ring in scratchpad RAM at 0x1f800000 (primed with a
 * write cursor at 990), which the PC port replaces with a static buffer.
 * ProcessMapFileData reads the 4-byte length header and unpacks into gMapDataPtr; LoadMap
 * (game_setup.c) is the only caller.
 *
 * LoadEventData carves the freshly loaded EVDATA*.DAT out of gEvtEntityData: the leading
 * s16 is the entity count, and each entity's script is a run of s16 (opcode, argument)
 * pairs ending at opcode 99; walking the terminators yields one gEvtEntityDataPointers
 * entry per entity. events.c calls it and hands each pointer to SetupEventEntity
 * (unit_actor.c).
 *
 * Objf683_AdjustFaceElevation and Objf684_SlidingFace both index the 76-entry
 * sSlidingFaces table by obj->state2 and move one tile-model face's elevation -- the first
 * in a single step, the second a few units per frame; event opcodes 0x52/0x53 spawn them.
 * Objf758_Map44_Scn00_ShrinkDoorTex trims three map textures' height over 32 frames to
 * track a door being lowered.
 *
 * From line ~250 the file is pure data: 64 D_* byte arrays in the unit-sprite animation
 * format (s8 (frameIdx, delay) pairs closed by a zero pair, plus the 5-byte 0x3b/0x3c
 * records carrying a per-frame position offset), gathered into gAnimSet_80101fc0[56] and
 * two small sets nothing references. Entries come in front/back pairs because animation
 * is indexed animSet[animIdx + facingFront]; events.c passes these as event entities'
 * base/alt sets. */
#include "common.h"
#include "field.h"
#include "object.h"

typedef struct SlidingFace {
   s16 z, x, faceIdx, amount;
} SlidingFace;

static SlidingFace sSlidingFaces[76] = {
    {0, 0, 0, 0},   {5, 20, 9, 4}, {6, 20, 9, 4},  {7, 20, 9, 4},  {2, 0, 1, 3},  {2, 0, 2, -3},
    {2, 0, 1, -3},  {2, 0, 2, 3},  {0, 19, 1, 3},  {0, 19, 2, -3}, {0, 20, 1, 3}, {0, 20, 2, -3},
    {0, 19, 1, -3}, {0, 19, 2, 3}, {0, 20, 1, -3}, {0, 20, 2, 3},  {6, 2, 0, 4},  {6, 2, 1, 4},
    {6, 3, 0, 4},   {6, 3, 1, 4},  {6, 2, 0, -4},  {6, 2, 1, -4},  {6, 3, 0, -4}, {6, 3, 1, -4},
    {2, 1, 1, 3},   {2, 1, 1, -3}, {1, 1, 0, 4},   {1, 1, 1, 4},   {1, 2, 0, 4},  {1, 2, 1, 4},
    {0, 1, 0, -4},  {0, 1, 1, -4}, {0, 2, 0, -4},  {0, 2, 1, -4},  {1, 8, 1, 3},  {1, 8, 2, -3},
    {2, 8, 1, 3},   {2, 8, 2, -3}, {1, 8, 1, -3},  {1, 8, 2, 3},   {2, 8, 1, -3}, {2, 8, 2, 3},
    {2, 6, 1, 3},   {2, 6, 2, -3}, {2, 6, 1, -3},  {2, 6, 2, 3},   {4, 2, 0, 4},  {4, 2, 1, 4},
    {4, 3, 0, 4},   {4, 3, 1, 4},  {4, 2, 0, -4},  {4, 2, 1, -4},  {4, 3, 0, -4}, {4, 3, 1, -4},
    {0, 1, 1, 3},   {0, 1, 2, -3}, {0, 1, 1, -3},  {0, 1, 2, 3},   {3, 2, 1, 3},  {3, 2, 2, -3},
    {3, 2, 1, -3},  {3, 2, 2, 3},  {6, 2, 1, 3},   {6, 2, 2, -3},  {6, 2, 1, -3}, {6, 2, 2, 3},
    {1, 1, 1, 3},   {1, 1, 2, -3}, {6, 12, 1, 3},  {6, 12, 2, -3}, {6, 13, 1, 3}, {6, 13, 2, -3},
    {6, 12, 1, -3}, {6, 12, 2, 3}, {6, 13, 1, -3}, {6, 13, 2, 3},
};

extern void AdjustFaceElevation2(MapTileModel *tileModel, s16 faceIdx, s16 amount);

s32 UnpackMapFileData(u8 *input, u8 *output, s32 len) {
   // FIXME: Fake match (forced register)

   s32 i;
   u8 *pCache;
   s32 cacheOfs;
   // u8 *pInputData;
   u8 *pOutputStart;
   u32 modeBits;
   s32 b1, b2, b3;
   s32 value;
   s32 cacheIdx;
   s32 num;

   // pInputData = input;
   pOutputStart = output;
#ifdef PC_PORT
   { static u8 sPcScratchpad[0x400]; pCache = sPcScratchpad; }
#else
   pCache = (u8 *)0x1f800000;
#endif

   for (cacheIdx = 0; cacheIdx < 990; cacheIdx++) {
      pCache[cacheIdx] = 0;
   }

   cacheOfs = 990;
   // value = *input++;

   while (value = *input++, --len >= 0) {

      modeBits = value | 0xff00;
      do {
         if ((modeBits & 1) == 0) {
            // Mode lo-bit is clear; read and cache one byte from input
            value = *input++;
            if (--len < 0) {
               return output - pOutputStart;
            }
            *output++ = value;
            pCache[cacheOfs++] = value;
            cacheOfs &= 0x3ff;
         } else {
            b1 = *input++;
            if (--len < 0) {
               return output - pOutputStart;
            }

            if (b1 < 0x80) {
               // Hi-bit is clear; read from cache
               cacheIdx = *input++;
               if (--len < 0) {
                  return output - pOutputStart;
               }
               // b1: 0nnn_nnCC; b2: cccc_cccc
               // CCcccccccc: 10-bit cache index
               // nnnnn + 2: num bytes to read from cache - 1
               cacheIdx |= ((b1 & 0x03) << 8);
               num = (b1 >> 2) + 2;

               for (i = 0; i <= num; i++) {
                  value = pCache[(cacheIdx + i) & 0x3ff];
                  *output++ = value;
                  pCache[cacheOfs++] = value;
                  cacheOfs &= 0x3ff;
               }
            } else if (b1 < 0xc0) {
               register s32 num asm("t0"); // FIXME
               // Second-highest bit is clear; read from cache (relative)
               // b1: 10nn_cccc
               // (0x30: 0b0011_0000; 0x0f: 0b0000_1111)
               cacheIdx = b1 & 0x0f;
               num = ((b1 & 0x30) >> 4) + 1;
               for (i = 0; i <= num; i++) {
                  value = pCache[(cacheOfs - cacheIdx) & 0x3ff];
                  *output++ = value;
                  pCache[cacheOfs++] = value;
                  cacheOfs &= 0x3ff;
               }
            } else {
               // Both high bits set; lower six bits determine how many bytes to read from input
               // b1: 11nn_nnnn (0x3f: 0b0011_1111)
               num = (b1 & 0x3f) + 7;
               for (i = 0; i <= num; i++) {
                  value = *input++;
                  if (--len < 0) {
                     break;
                  }
                  *output++ = value;
                  pCache[cacheOfs++] = value;
                  cacheOfs &= 0x3ff;
               }
            }
         }
         modeBits >>= 1;
      } while ((modeBits & 0x100) != 0);
      // value = *input++;
   }

   return output - pOutputStart;
}

void ProcessMapFileData(u8 *input) {
   s32 len;

   len = input[0] + (input[1] << 8) + (input[2] << 16) + (input[3] << 24);
   UnpackMapFileData(&input[4], gMapDataPtr, len);
}

#undef OBJF
#define OBJF 683
void Objf683_AdjustFaceElevation(Object *obj) {
   SlidingFace *p;
   s32 z, x;
   s32 amount, faceIdx;

   p = &sSlidingFaces[obj->state2];
   z = p->z;
   x = p->x;
   faceIdx = p->faceIdx;
   amount = p->amount;
   AdjustFaceElevation2(&gMapRowPointers[z][x], faceIdx, amount * 16);
   obj->functionIndex = OBJF_NULL;
}

#undef OBJF
#define OBJF 684
void Objf684_SlidingFace(Object *obj) {
   SlidingFace *p;
   s32 z, x;
   s32 amount, faceIdx;
   MapTileModel *pTileModel;

   p = &sSlidingFaces[obj->state2];
   z = p->z;
   x = p->x;
   faceIdx = p->faceIdx;
   amount = p->amount;
   pTileModel = &gMapRowPointers[z][x];

   switch (obj->state) {
   case 0:
      obj->mem = amount * 16;
      if (obj->state2 < 4) {
         if (obj->mem < 0) {
            obj->state3 = -1;
         } else {
            obj->state3 = 1;
         }
      } else {
         if (obj->mem < 0) {
            obj->state3 = -2;
         } else {
            obj->state3 = 2;
         }
      }
      obj->state++;

   // fallthrough
   case 1:
      AdjustFaceElevation2(pTileModel, faceIdx, obj->state3);
      obj->mem -= obj->state3;
      if (obj->mem == 0) {
         obj->state++;
      }
      break;

   case 2:
      obj->functionIndex = OBJF_NULL;
      break;
   }
}

#undef OBJF
#define OBJF 758
void Objf758_Map44_Scn00_ShrinkDoorTex(Object *obj) {
   // The top face of the door is moved, causing the front face to shrink; this shrinks the
   // associated textures' height to match

   switch (obj->state) {
   case 0:
      obj->state3 = 32;
      obj->state++;

   // fallthrough
   case 1:
      gGfxSubTextures[GFX_MAP_TEXTURE_74][3]--; //.h
      gGfxSubTextures[GFX_MAP_TEXTURE_75][3]--;
      gGfxSubTextures[GFX_MAP_TEXTURE_76][3]--;
      obj->state++;
      break;

   case 2:
      obj->state--;
      if (--obj->state3 <= 0) {
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

s32 LoadEventData(void) {
   s16 *p;
   s16 entityCt;
   s32 i;

   p = gEvtEntityData;
   entityCt = *p++;
   gEvtEntityDataPointers[0] = p;

   for (i = 0; i < entityCt - 1; i++) {
      while (*p != 99) {
         p += 2;
      }
      p += 2;
      gEvtEntityDataPointers[i + 1] = p;
   }

   return entityCt;
}

// TODO: Trim padding
u8 D_80101DB4[] = {0x2b, 0x01, 0x00, 0x00};
u8 D_80101DB8[] = {0x2c, 0x01, 0x00, 0x00};
u8 D_80101DBC[] = {0x2b, 0x01, 0x00, 0x00};
u8 D_80101DC0[] = {0x2c, 0x01, 0x00, 0x00};
u8 D_80101DC4[] = {0x23, 0x05, 0x24, 0x03, 0x25, 0x05, 0x24, 0x03, 0x00, 0x01, 0x00, 0x00};
u8 D_80101DD0[] = {0x26, 0x05, 0x27, 0x03, 0x28, 0x05, 0x27, 0x03, 0x00, 0x01, 0x00, 0x00};
u8 D_80101DDC[] = {0x23, 0x05, 0x24, 0x05, 0x25, 0x05, 0x24, 0x05, 0x00, 0x01, 0x00, 0x00};
u8 D_80101DE8[] = {0x26, 0x05, 0x27, 0x05, 0x28, 0x05, 0x27, 0x05, 0x00, 0x01, 0x00, 0x00};
u8 D_80101DF4[] = {0x23, 0x05, 0x24, 0x05, 0x25, 0x05, 0x24, 0x05, 0x00, 0x01, 0x00, 0x00};
u8 D_80101E00[] = {0x26, 0x05, 0x27, 0x05, 0x28, 0x05, 0x27, 0x05, 0x00, 0x01, 0x00, 0x00};
u8 D_80101E0C[] = {0x23, 0x02, 0x24, 0x02, 0x25, 0x02, 0x24, 0x02, 0x00, 0x01, 0x00, 0x00};
u8 D_80101E18[] = {0x26, 0x02, 0x27, 0x02, 0x28, 0x02, 0x27, 0x02, 0x00, 0x01, 0x00, 0x00};
u8 D_80101E24[] = {0x29, 0x01, 0x00, 0x00};
u8 D_80101E28[] = {0x2a, 0x01, 0x00, 0x00};
u8 D_80101E2C[] = {0x29, 0x05, 0x2b, 0x05, 0x00, 0x00, 0x00, 0x00};
u8 D_80101E34[] = {0x2a, 0x05, 0x2c, 0x05, 0x00, 0x00, 0x00, 0x00};
u8 D_80101E3C[] = {0x29, 0x0a, 0x2b, 0x0a, 0x00, 0x00, 0x00, 0x00};
u8 D_80101E44[] = {0x2a, 0x0a, 0x2c, 0x0a, 0x00, 0x00, 0x00, 0x00};
u8 D_80101E4C[] = {0x29, 0x0f, 0x2b, 0x0f, 0x00, 0x00, 0x00, 0x00};
u8 D_80101E54[] = {0x2a, 0x0f, 0x2c, 0x0f, 0x00, 0x00, 0x00, 0x00};
u8 D_80101E5C[] = {0x23, 0x01, 0x00, 0x00};
u8 D_80101E60[] = {0x26, 0x01, 0x00, 0x00};
u8 D_80101E64[] = {0x24, 0x01, 0x00, 0x00};
u8 D_80101E68[] = {0x27, 0x01, 0x00, 0x00};
u8 D_80101E6C[] = {0x23, 0x01, 0x2b, 0x05, 0x00, 0x00, 0x00, 0x00};
u8 D_80101E74[] = {0x26, 0x01, 0x2c, 0x05, 0x00, 0x00, 0x00, 0x00};
u8 D_80101E7C[] = {0x24, 0x01, 0x2b, 0x00, 0x00, 0x00, 0x00, 0x00};
u8 D_80101E84[] = {0x27, 0x01, 0x2c, 0x00, 0x00, 0x00, 0x00, 0x00};
u8 D_80101E8C[] = {0x2b, 0x03, 0x25, 0x03, 0x00, 0x01, 0x00, 0x00};
u8 D_80101E94[] = {0x2c, 0x03, 0x28, 0x03, 0x00, 0x01, 0x00, 0x00};
u8 D_80101E9C[] = {0x2b, 0x06, 0x25, 0x06, 0x00, 0x01, 0x00, 0x00};
u8 D_80101EA4[] = {0x2c, 0x06, 0x28, 0x06, 0x00, 0x01, 0x00, 0x00};
u8 D_80101EAC[] = {0x23, 0x0a, 0x2b, 0x03, 0x24, 0x0a, 0x2b, 0x03, 0x00, 0x01, 0x00, 0x00};
u8 D_80101EB8[] = {0x26, 0x0a, 0x2c, 0x03, 0x27, 0x0a, 0x2c, 0x03, 0x00, 0x01, 0x00, 0x00};
u8 D_80101EC4[] = {0x24, 0x06, 0x2b, 0x06, 0x23, 0x06, 0x2b, 0x03, 0x00, 0x01, 0x00, 0x00};
u8 D_80101ED0[] = {0x27, 0x06, 0x2c, 0x06, 0x26, 0x06, 0x2c, 0x03, 0x00, 0x01, 0x00, 0x00};
u8 D_80101EDC[] = {0x3b, 0xfc, 0x02, 0x05, 0x02, 0x3b, 0x08, 0x04, 0x05, 0x02,
                   0x3b, 0x00, 0x00, 0x05, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00};
u8 D_80101EF0[] = {0x3b, 0xf8, 0x00, 0x06, 0x01, 0x3b, 0x00, 0x00, 0x06, 0x01, 0x3b, 0xfc,
                   0x00, 0x06, 0x01, 0x3b, 0x00, 0x00, 0x06, 0x01, 0x00, 0x00, 0x00, 0x00};
u8 D_80101F08[] = {0x05, 0x06, 0x2b, 0x01, 0x00, 0x01, 0x00, 0x00};
u8 D_80101F10[] = {0x06, 0x06, 0x2c, 0x01, 0x00, 0x01, 0x00, 0x00};
u8 D_80101F18[] = {0x23, 0x05, 0x24, 0x02, 0x25, 0x05, 0x24, 0x02, 0x00, 0x01, 0x00, 0x00};
u8 D_80101F24[] = {0x26, 0x05, 0x27, 0x02, 0x28, 0x05, 0x27, 0x02, 0x00, 0x01, 0x00, 0x00};
u8 D_80101F30[] = {0x23, 0x08, 0x24, 0x05, 0x25, 0x08, 0x24, 0x05, 0x00, 0x01, 0x00, 0x00};
u8 D_80101F3C[] = {0x26, 0x08, 0x27, 0x05, 0x28, 0x08, 0x27, 0x05, 0x00, 0x01, 0x00, 0x00};
u8 D_80101F48[] = {0x23, 0x04, 0x2b, 0x04, 0x24, 0x04, 0x2b, 0x04, 0x00, 0x00, 0x00, 0x00};
u8 D_80101F54[] = {0x26, 0x04, 0x2c, 0x04, 0x27, 0x04, 0x2c, 0x04, 0x00, 0x00, 0x00, 0x00};
u8 D_80101F60[] = {0x3b, 0xfc, 0x00, 0x25, 0x02, 0x3b, 0x02, 0x00, 0x25, 0x02, 0x3b, 0xfe,
                   0x00, 0x25, 0x02, 0x3b, 0x00, 0x00, 0x25, 0x02, 0x00, 0x00, 0x00, 0x00};
u8 D_80101F78[] = {0x3b, 0xfc, 0x00, 0x28, 0x02, 0x3b, 0x02, 0x00, 0x28, 0x02, 0x3b, 0xfe,
                   0x00, 0x28, 0x02, 0x3b, 0x00, 0x00, 0x28, 0x02, 0x00, 0x00, 0x00, 0x00};
u8 D_80101F90[] = {0x23, 0x14, 0x2b, 0x03, 0x24, 0x14, 0x2b, 0x03, 0x00, 0x00, 0x00, 0x00};
u8 D_80101F9C[] = {0x26, 0x14, 0x2c, 0x03, 0x27, 0x14, 0x2c, 0x03, 0x00, 0x00, 0x00, 0x00};
u8 D_80101FA8[] = {0x23, 0x01, 0x00, 0x00};
u8 D_80101FAC[] = {0x26, 0x01, 0x00, 0x00};
u8 D_80101FB0[] = {0x24, 0x01, 0x00, 0x00};
u8 D_80101FB4[] = {0x27, 0x01, 0x00, 0x00};
u8 D_80101FB8[] = {0x25, 0x01, 0x00, 0x00};
u8 D_80101FBC[] = {0x28, 0x01, 0x00, 0x00};

u8 *gAnimSet_80101fc0[] = {
    D_80101DB4, D_80101DB8, D_80101DBC, D_80101DC0, D_80101DC4, D_80101DD0, D_80101DDC, D_80101DE8,
    D_80101DF4, D_80101E00, D_80101E0C, D_80101E18, D_80101E24, D_80101E28, D_80101E2C, D_80101E34,
    D_80101E3C, D_80101E44, D_80101E4C, D_80101E54, D_80101E5C, D_80101E60, D_80101E64, D_80101E68,
    D_80101E6C, D_80101E74, D_80101E7C, D_80101E84, D_80101E8C, D_80101E94, D_80101E9C, D_80101EA4,
    D_80101EAC, D_80101EB8, D_80101EC4, D_80101ED0, D_80101EDC, D_80101EF0, D_80101F08, D_80101F10,
    D_80101F18, D_80101F24, D_80101F48, D_80101F54, D_80101F60, D_80101F78, D_80101F90, D_80101F9C,
    D_80101FA8, D_80101FAC, D_80101FB0, D_80101FB4, D_80101FB8, D_80101FBC, D_80101F30, D_80101F3C,
};

u8 D_801020A0[] = {0x09, 0x03, 0x0a, 0x03, 0x00, 0x00, 0x00, 0x00};
u8 D_801020A8[] = {0x0b, 0x03, 0x0c, 0x03, 0x00, 0x00, 0x00, 0x00};
u8 D_801020B0[] = {0x01, 0x1e, 0x02, 0x1e, 0x00, 0x00, 0x00, 0x00};
u8 D_801020B8[] = {0x02, 0x02, 0x03, 0x02, 0x00, 0x01, 0x00, 0x00};
u8 D_801020C0[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// Unused?
u8 *gAnimSet_801020c8[] = {
    D_801020A0, D_801020A8, D_801020B0, D_801020B0, D_801020B8, D_801020B8,
};

u8 D_801020E0[] = {0x15, 0x05, 0x16, 0x05, 0x00, 0x00, 0x00, 0x00};
u8 D_801020E8[] = {0x16, 0x02, 0x15, 0x02, 0x2b, 0x02, 0x17, 0x02, 0x18, 0x02, 0x00, 0x00};
u8 D_801020F4[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// Unused?
u8 *gAnimSet_801020fc[] = {
    D_801020E0,
    D_801020E0,
    D_801020E8,
    D_801020E8,
};
