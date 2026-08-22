#include "common.h"
#include "graphics.h"
#include "object.h"
#include "battle.h"
#include "state.h"
#include "field.h"
#include "inline_gte.h"

#include "PsyQ/kernel.h"
#include "PsyQ/gtemac.h"

void LoadDecodedUnitSprites(Object *obj) {
   RECT rect;

   rect.x = obj->d.objf050.vramX;
   rect.y = obj->d.objf050.vramY;
   rect.w = 256 >> 2;
   rect.h = 352;
   LoadImage(&rect, &gScratch1_801317c0[0]);
}

// TBD: Simpler way around lui anomaly?

static inline void SetFunctionIndex(s16 value) { gUnitSpritesDecoder.functionIndex = value; }

static inline u16 GetState(void) { return gUnitSpritesDecoder.state; }
static inline void SetState(u16 value) { gUnitSpritesDecoder.state = value; }

static inline s32 GetRemainingBytes(void) { return gUnitSpritesDecoder.d.objf050.remainingBytes; }
static inline void SetRemainingBytes(s32 value) {
   gUnitSpritesDecoder.d.objf050.remainingBytes = value;
}

static inline u16 GetCacheOfs(void) { return gUnitSpritesDecoder.d.objf050.cacheOfs; }
static inline void SetCacheOfs(u16 value) { gUnitSpritesDecoder.d.objf050.cacheOfs = value; }

static inline u16 GetModeBits(void) { return gUnitSpritesDecoder.d.objf050.encodingBits; }
static inline void SetModeBits(u16 value) { gUnitSpritesDecoder.d.objf050.encodingBits = value; }

static inline u8 *GetSrc(void) { return gUnitSpritesDecoder.d.objf050.src; }
static inline void SetSrc(u8 *value) { gUnitSpritesDecoder.d.objf050.src = value; }

static inline u8 *GetDst(void) { return gUnitSpritesDecoder.d.objf050.dst; }
static inline void SetDst(u8 *value) { gUnitSpritesDecoder.d.objf050.dst = value; }

static inline u8 *GetBaseSrc(void) { return (u8 *)gUnitSpritesDecoder.d.objf050.baseSrcDataPtr; }
static inline u8 *GetBaseDst(void) { return (u8 *)gUnitSpritesDecoder.d.objf050.baseDstDataPtr; }

#undef OBJF
#define OBJF 050
void Objf050_UnitSpritesDecoder(Object *obj) {
   s32 i, j, k;
   u8 *pCache;
   s32 maxTicks;
   u16 cacheOfs;
   u16 modeBits;
   s32 remainingBytes;
   u8 *pSrc;
   u8 *pDst;
   s32 b;
   // s32 cachedBytesToRead;
   // s32 cacheIdx;
   s16 state;

#ifdef PC_PORT
   { static u8 sPcScratchpad[0x400]; pCache = sPcScratchpad; }
#else
   pCache = (u8 *)0x1f800000;
#endif
   state = GetState();

   switch (state) {
   case 0:
      remainingBytes = 0xb000;
      cacheOfs = 958;
      pSrc = GetBaseSrc();
      pDst = GetBaseDst();
      SetRemainingBytes(remainingBytes);
      SetModeBits(0);
      SetCacheOfs(cacheOfs);
      SetSrc(pSrc + 4);
      SetDst(pDst);

      for (j = 0; j < 958; j++) {
         pCache[j] = 0;
      }

      gDecodingSprites = 1;
      // gUnitSpritesDecoder.state++;
      SetState(GetState() + 1);

   // fallthrough
   case 1:
      if (!gIsEnemyTurn || GetRCnt(RCntCNT1) <= 470) {
         j = GetRCnt(RCntCNT1);
         if (gIsEnemyTurn) {
            maxTicks = j + ((256 - j) & 0xff) - 30;
         } else {
            maxTicks = j + ((256 - j) & 0xff);
         }
         if ((j >> 8) == 0) {
            maxTicks += 256;
         }

         remainingBytes = GetRemainingBytes();
         modeBits = GetModeBits();
         cacheOfs = GetCacheOfs();
         pSrc = GetSrc();
         pDst = GetDst();

         do {
            modeBits >>= 1;
            if ((modeBits & 0x100) == 0) {
               // Ran out of sentinel bits; read a byte from input and OR a sentinel hi-byte onto it
               modeBits = *pSrc++ | 0xff00;
            }
            if ((modeBits & 1) != 0) {
               // Mode lo-bit is set; read and cache one byte from input
               b = *pSrc++;
               *pDst++ = b;
               remainingBytes--;
               if (remainingBytes == 0) {
                  LoadDecodedUnitSprites(&gUnitSpritesDecoder);
                  SetFunctionIndex(OBJF_NULL);
                  gDecodingSprites = 0;
                  return;
               }
               pCache[cacheOfs++ & 0x3ff] = b;
            } else {
               // Mode lo-bit is clear; next two bytes decode to cache idx and length.
               // b1: cccc_cccc; b2: CCnn_nnnn;
               // CCcccccccc: 10-bit cache index
               // nnnnnn + 3: num bytes to read from cache (3..66)
               // (0x3f: 0b0011_1111; 0xc0: 0b1100_0000)
               j = *pSrc++;          // b1
               k = *pSrc++;          // b2
               j |= (k & 0xc0) << 2; // cacheIdx
               k = (k & 0x3f) + 3;   // cachedBytesToRead
               for (i = 0; i < k; i++) {
                  b = pCache[(j + i) & 0x3ff];
                  *pDst++ = b;
                  pCache[cacheOfs++ & 0x3ff] = b;
               }
               remainingBytes -= k;
               if (remainingBytes <= 0) {
                  LoadDecodedUnitSprites(&gUnitSpritesDecoder);
                  SetFunctionIndex(OBJF_NULL);
                  gDecodingSprites = 0;
                  return;
               }
            }
         } while (GetRCnt(RCntCNT1) <= maxTicks);

         SetRemainingBytes(remainingBytes);
         SetModeBits(modeBits);
         SetCacheOfs(cacheOfs);
         SetSrc(pSrc);
         SetDst(pDst);
      }
      break;

   case 2:
      gDecodingSprites = 0;
      SetFunctionIndex(OBJF_NULL);
      DrawSync(0);
      break;
   }
}

#undef OBJF
#define OBJF 051
void Objf051_FloatingDamageText(Object *obj) {
   Object *unitSprite;
   Object *digitSprite;
   s32 i;
   s16 hundreds, tens;
   s16 digits[4];
   s16 a, b;
   s16 x_1, x_2, z_1, z_2;

   switch (obj->state) {
   case 0:
      OBJ.unitSprite = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);

      OBJ.hundreds = hundreds = OBJ.damage / 100;
      OBJ.damage -= OBJ.hundreds * 100;
      OBJ.tens = tens = OBJ.damage / 10;
      OBJ.damage -= OBJ.tens * 10;
      OBJ.ones = OBJ.damage;

      if (hundreds != 0) {
         OBJ.numDigits = 3;
      } else if (tens != 0) {
         OBJ.numDigits = 2;
      } else {
         OBJ.numDigits = 1;
      }

      digitSprite = Obj_GetUnused();
      OBJ.digitSprite = digitSprite;
      digitSprite->functionIndex = OBJF_NOOP;

      if (OBJ.clut == CLUT_NULL) {
         OBJ.clut = CLUT_REDS;
      }
      digitSprite->d.sprite.clut = OBJ.clut;

      obj->state++;
      break;

   case 1:
      digits[0] = OBJ.ones;
      digits[1] = OBJ.tens;
      digits[2] = OBJ.hundreds;

      unitSprite = OBJ.unitSprite;
      digitSprite = OBJ.digitSprite;

      obj->x1.n = unitSprite->x1.n;
      obj->z1.n = unitSprite->z1.n;
      obj->y1.n = unitSprite->y1.n + CV(0.875);

      switch ((gCameraRotation.vy & 0xfff) >> 10) {
      case CAM_DIR_SOUTH:
         digitSprite->z1.n -= CV(0.125);
         digitSprite->x1.n += CV(0.125);
         digitSprite->d.sprite.coords[0].x = CV(-0.15625);
         digitSprite->d.sprite.coords[0].z = CV(-0.15625);
         digitSprite->d.sprite.coords[1].x = CV(0.15625);
         digitSprite->d.sprite.coords[1].z = CV(0.15625);
         break;
      case CAM_DIR_EAST:
         digitSprite->x1.n += CV(0.125);
         digitSprite->z1.n += CV(0.125);
         digitSprite->d.sprite.coords[0].x = CV(0.15625);
         digitSprite->d.sprite.coords[0].z = CV(-0.15625);
         digitSprite->d.sprite.coords[1].x = CV(-0.15625);
         digitSprite->d.sprite.coords[1].z = CV(0.15625);
         break;
      case CAM_DIR_NORTH:
         digitSprite->z1.n += CV(0.125);
         digitSprite->x1.n -= CV(0.125);
         digitSprite->d.sprite.coords[0].x = CV(0.15625);
         digitSprite->d.sprite.coords[0].z = CV(0.15625);
         digitSprite->d.sprite.coords[1].x = CV(-0.15625);
         digitSprite->d.sprite.coords[1].z = CV(-0.15625);
         break;
      case CAM_DIR_WEST:
         digitSprite->z1.n -= CV(0.125);
         digitSprite->x1.n -= CV(0.125);
         digitSprite->d.sprite.coords[0].x = CV(-0.15625);
         digitSprite->d.sprite.coords[0].z = CV(0.15625);
         digitSprite->d.sprite.coords[1].x = CV(0.15625);
         digitSprite->d.sprite.coords[1].z = CV(-0.15625);
         break;
      }

      digitSprite->d.sprite.coords[0].y = CV(0.1875);
      digitSprite->d.sprite.coords[1].y = CV(0.1875);

      a = rsin(OBJ.phase) + 0x800;
      digitSprite->d.sprite.coords[0].x = (digitSprite->d.sprite.coords[0].x * a) >> 12;
      digitSprite->d.sprite.coords[1].x = (digitSprite->d.sprite.coords[1].x * a) >> 12;
      digitSprite->d.sprite.coords[0].z = (digitSprite->d.sprite.coords[0].z * a) >> 12;
      digitSprite->d.sprite.coords[1].z = (digitSprite->d.sprite.coords[1].z * a) >> 12;
      digitSprite->d.sprite.coords[0].y = (digitSprite->d.sprite.coords[0].y * a) >> 12;
      digitSprite->d.sprite.coords[1].y = (digitSprite->d.sprite.coords[1].y * a) >> 12;
      digitSprite->d.sprite.coords[2].x = digitSprite->d.sprite.coords[0].x;
      digitSprite->d.sprite.coords[3].x = digitSprite->d.sprite.coords[1].x;
      digitSprite->d.sprite.coords[2].z = digitSprite->d.sprite.coords[0].z;
      digitSprite->d.sprite.coords[3].z = digitSprite->d.sprite.coords[1].z;
      digitSprite->d.sprite.coords[2].y = -1 * digitSprite->d.sprite.coords[0].y;
      digitSprite->d.sprite.coords[3].y = -1 * digitSprite->d.sprite.coords[1].y;

      b = abs(digitSprite->d.sprite.coords[0].x) * 2;
      digitSprite->d.sprite.coords[0].x += obj->x1.n;
      digitSprite->d.sprite.coords[1].x += obj->x1.n;
      digitSprite->d.sprite.coords[2].x += obj->x1.n;
      digitSprite->d.sprite.coords[3].x += obj->x1.n;

      digitSprite->d.sprite.coords[0].z += obj->z1.n;
      digitSprite->d.sprite.coords[1].z += obj->z1.n;
      digitSprite->d.sprite.coords[2].z += obj->z1.n;
      digitSprite->d.sprite.coords[3].z += obj->z1.n;

      digitSprite->d.sprite.coords[0].y += obj->y1.n + (a - 0x800) / 16;
      digitSprite->d.sprite.coords[1].y += obj->y1.n + (a - 0x800) / 16;
      digitSprite->d.sprite.coords[2].y += obj->y1.n + (a - 0x800) / 16;
      digitSprite->d.sprite.coords[3].y += obj->y1.n + (a - 0x800) / 16;

      digitSprite->d.sprite.otOfs = 32;

      switch (OBJ.numDigits) {
      case 1:
         digitSprite->d.sprite.gfxIdx = GFX_DIGIT_0 + digits[0];
         AddObjPrim4(gGraphicsPtr->ot, digitSprite);
         break;

      case 2:

         switch ((gCameraRotation.vy & 0xfff) >> 10) {
         case CAM_DIR_SOUTH:
            x_1 = (b * 3) / 2;
            z_1 = (b * 3) / 2;
            x_2 = -1 * b;
            z_2 = -1 * b;
            break;
         case CAM_DIR_EAST:
            x_1 = -(b * 3) / 2;
            z_1 = (b * 3) / 2;
            x_2 = b;
            z_2 = -1 * b;
            break;
         case CAM_DIR_NORTH:
            x_1 = -(b * 3) / 2;
            z_1 = -(b * 3) / 2;
            x_2 = b;
            z_2 = b;
            break;
         case CAM_DIR_WEST:
            x_1 = (b * 3) / 2;
            z_1 = -(b * 3) / 2;
            x_2 = -1 * b;
            z_2 = b;
            break;
         }

         digitSprite->d.sprite.coords[0].x += x_1;
         digitSprite->d.sprite.coords[1].x += x_1;
         digitSprite->d.sprite.coords[2].x += x_1;
         digitSprite->d.sprite.coords[3].x += x_1;
         digitSprite->d.sprite.coords[0].z += z_1;
         digitSprite->d.sprite.coords[1].z += z_1;
         digitSprite->d.sprite.coords[2].z += z_1;
         digitSprite->d.sprite.coords[3].z += z_1;

         for (i = 0; i < 2; i++) {
            digitSprite->d.sprite.gfxIdx = GFX_DIGIT_0 + digits[i];
            digitSprite->d.sprite.coords[0].x += x_2;
            digitSprite->d.sprite.coords[1].x += x_2;
            digitSprite->d.sprite.coords[2].x += x_2;
            digitSprite->d.sprite.coords[3].x += x_2;
            digitSprite->d.sprite.coords[0].z += z_2;
            digitSprite->d.sprite.coords[1].z += z_2;
            digitSprite->d.sprite.coords[2].z += z_2;
            digitSprite->d.sprite.coords[3].z += z_2;
            AddObjPrim4(gGraphicsPtr->ot, digitSprite);
         }

         break;

      case 3:

         switch ((gCameraRotation.vy & 0xfff) >> 10) {
         case CAM_DIR_SOUTH:
            x_1 = b * 2;
            z_1 = b * 2;
            x_2 = -1 * b;
            z_2 = -1 * b;
            break;
         case CAM_DIR_EAST:
            x_1 = -(b * 2);
            z_1 = b * 2;
            x_2 = b;
            z_2 = -1 * b;
            break;
         case CAM_DIR_NORTH:
            x_1 = -(b * 2);
            z_1 = -(b * 2);
            x_2 = b;
            z_2 = b;
            break;
         case CAM_DIR_WEST:
            x_1 = b * 2;
            z_1 = -(b * 2);
            x_2 = -1 * b;
            z_2 = b;
            break;
         }

         digitSprite->d.sprite.coords[0].x += x_1;
         digitSprite->d.sprite.coords[1].x += x_1;
         digitSprite->d.sprite.coords[2].x += x_1;
         digitSprite->d.sprite.coords[3].x += x_1;
         digitSprite->d.sprite.coords[0].z += z_1;
         digitSprite->d.sprite.coords[1].z += z_1;
         digitSprite->d.sprite.coords[2].z += z_1;
         digitSprite->d.sprite.coords[3].z += z_1;

         for (i = 0; i < 3; i++) {
            digitSprite->d.sprite.gfxIdx = GFX_DIGIT_0 + digits[i];
            digitSprite->d.sprite.coords[0].x += x_2;
            digitSprite->d.sprite.coords[1].x += x_2;
            digitSprite->d.sprite.coords[2].x += x_2;
            digitSprite->d.sprite.coords[3].x += x_2;
            digitSprite->d.sprite.coords[0].z += z_2;
            digitSprite->d.sprite.coords[1].z += z_2;
            digitSprite->d.sprite.coords[2].z += z_2;
            digitSprite->d.sprite.coords[3].z += z_2;
            AddObjPrim4(gGraphicsPtr->ot, digitSprite);
         }

         break;
      }

      switch (obj->state2) {
      case 0:
         OBJ.phase += DEG(16.34765625);
         if (OBJ.phase >= DEG(135)) {
            obj->state2++;
         }
         break;

      case 1:
         if (OBJ.timer == 30) {
            obj->functionIndex = OBJF_NULL;
            digitSprite->functionIndex = OBJF_NULL;
         }
         OBJ.timer++;
         break;
      }

      break;
   }
}

#undef OBJF
#define OBJF 052
void Objf052_AttackInfoMarker(Object *obj) {
   s16 markers[4] = {GFX_MISS, GFX_PARALYZED, GFX_SUPPORT, GFX_POISONED};
   Object *unitSprite;
   Object *markerSprite;
   s16 a;

   switch (obj->state) {
   case 0:
      OBJ.unitSprite = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);

#ifdef PC_PORT
      /* If the marker's cell has no valid unit (stale / off-map coord -> NULL), retail's
       * case 1 would dereference the NULL sprite. Kill the marker instead of crashing.
       * Same gated guard as the US tree (src/core/graphics.c). */
      if (OBJ.unitSprite == NULL) {
         obj->functionIndex = OBJF_NULL;
         break;
      }
#endif

      markerSprite = Obj_GetUnused();
      OBJ.sprite = markerSprite;
      markerSprite->functionIndex = OBJF_NOOP;
      markerSprite->d.sprite.gfxIdx = markers[OBJ.type];

      if (OBJ.clut == CLUT_NULL) {
         OBJ.clut = CLUT_REDS;
      }
      markerSprite->d.sprite.clut = OBJ.clut;

      obj->state++;
      break;

   case 1:
      unitSprite = OBJ.unitSprite;
      markerSprite = OBJ.sprite;
      obj->x1.n = unitSprite->x1.n;
      obj->z1.n = unitSprite->z1.n;
      obj->y1.n = unitSprite->y1.n + CV(0.875);

      switch ((gCameraRotation.vy & 0xfff) >> 10) {
      case CAM_DIR_SOUTH:
         markerSprite->z1.n -= CV(0.125);
         markerSprite->x1.n += CV(0.125);
         markerSprite->d.sprite.coords[0].x = CV(-0.375);
         markerSprite->d.sprite.coords[0].z = CV(-0.375);
         markerSprite->d.sprite.coords[1].x = CV(0.375);
         markerSprite->d.sprite.coords[1].z = CV(0.375);
         break;
      case CAM_DIR_EAST:
         markerSprite->x1.n += CV(0.125);
         markerSprite->z1.n += CV(0.125);
         markerSprite->d.sprite.coords[0].x = CV(0.375);
         markerSprite->d.sprite.coords[0].z = CV(-0.375);
         markerSprite->d.sprite.coords[1].x = CV(-0.375);
         markerSprite->d.sprite.coords[1].z = CV(0.375);
         break;
      case CAM_DIR_NORTH:
         markerSprite->z1.n += CV(0.125);
         markerSprite->x1.n -= CV(0.125);
         markerSprite->d.sprite.coords[0].x = CV(0.375);
         markerSprite->d.sprite.coords[0].z = CV(0.375);
         markerSprite->d.sprite.coords[1].x = CV(-0.375);
         markerSprite->d.sprite.coords[1].z = CV(-0.375);
         break;
      case CAM_DIR_WEST:
         markerSprite->z1.n -= CV(0.125);
         markerSprite->x1.n -= CV(0.125);
         markerSprite->d.sprite.coords[0].x = CV(-0.375);
         markerSprite->d.sprite.coords[0].z = CV(0.375);
         markerSprite->d.sprite.coords[1].x = CV(0.375);
         markerSprite->d.sprite.coords[1].z = CV(-0.375);
         break;
      }

      markerSprite->d.sprite.coords[0].y = CV(0.1875);
      markerSprite->d.sprite.coords[1].y = CV(0.1875);

      if (OBJ.type == ATK_MARKER_SUPPORT) {
         markerSprite->d.sprite.coords[0].y = CV(0.125);
         markerSprite->d.sprite.coords[1].y = CV(0.125);
         markerSprite->d.sprite.coords[0].x /= 2;
         markerSprite->d.sprite.coords[0].z /= 2;
         markerSprite->d.sprite.coords[1].x /= 2;
         markerSprite->d.sprite.coords[1].z /= 2;
      }

      a = rsin(OBJ.angle) + 0x800;
      markerSprite->d.sprite.coords[0].x = markerSprite->d.sprite.coords[0].x * a / 0xc00;
      markerSprite->d.sprite.coords[1].x = markerSprite->d.sprite.coords[1].x * a / 0xc00;
      markerSprite->d.sprite.coords[0].z = markerSprite->d.sprite.coords[0].z * a / 0xc00;
      markerSprite->d.sprite.coords[1].z = markerSprite->d.sprite.coords[1].z * a / 0xc00;
      markerSprite->d.sprite.coords[0].y = markerSprite->d.sprite.coords[0].y * a / 0xc00;
      markerSprite->d.sprite.coords[1].y = markerSprite->d.sprite.coords[1].y * a / 0xc00;
      markerSprite->d.sprite.coords[0].x += obj->x1.n;
      markerSprite->d.sprite.coords[1].x += obj->x1.n;
      markerSprite->d.sprite.coords[0].z += obj->z1.n;
      markerSprite->d.sprite.coords[1].z += obj->z1.n;
      markerSprite->d.sprite.coords[2].x = markerSprite->d.sprite.coords[0].x;
      markerSprite->d.sprite.coords[3].x = markerSprite->d.sprite.coords[1].x;
      markerSprite->d.sprite.coords[2].z = markerSprite->d.sprite.coords[0].z;
      markerSprite->d.sprite.coords[3].z = markerSprite->d.sprite.coords[1].z;
      markerSprite->d.sprite.coords[2].y = -markerSprite->d.sprite.coords[0].y;
      markerSprite->d.sprite.coords[3].y = -markerSprite->d.sprite.coords[1].y;
      markerSprite->d.sprite.coords[0].y += obj->y1.n + ((a - 0x800) >> 4);
      markerSprite->d.sprite.coords[1].y += obj->y1.n + ((a - 0x800) >> 4);
      markerSprite->d.sprite.coords[2].y += obj->y1.n + ((a - 0x800) >> 4);
      markerSprite->d.sprite.coords[3].y += obj->y1.n + ((a - 0x800) >> 4);
      AddObjPrim4(gGraphicsPtr->ot, markerSprite);

      switch (obj->state2) {
      case 0:
         OBJ.angle += DEG(16.171875);
         if (OBJ.angle >= DEG(135)) {
            obj->state2++;
         }
         if (OBJ.type == ATK_MARKER_SUPPORT) {
            OBJ.timer = 20;
         } else {
            OBJ.timer = 30;
         }
         break;

      case 1:
         if (--OBJ.timer == 0) {
            obj->functionIndex = OBJF_NULL;
            markerSprite->functionIndex = OBJF_NULL;
         }
         break;
      }

      break;
   }
}

#undef OBJF
#define OBJF 059
void Objf059_DebugVram(Object *obj) {
   extern s32 D_8012340C;

   s16 widths[5] = {256, 320, 384, 512, 640};
   s16 heights[2] = {240, 480};
   Graphics *g;

   switch (obj->state) {
   case 0:
      OBJ.widthIdx = 1;
      OBJ.heightIdx = 0;
      OBJ.width = widths[OBJ.widthIdx];
      OBJ.height = heights[OBJ.heightIdx];
      obj->state++;
      break;

   case 1:
      if (gSavedPad2StateNewPresses & PAD_SELECT) {
         D_8012340C = 0;
         obj->functionIndex = OBJF_NULL;
         SetDefDispEnv(&gGraphicBuffers[0].dispEnv, 0, 272, SCREEN_WIDTH, SCREEN_HEIGHT);
         SetDefDispEnv(&gGraphicBuffers[1].dispEnv, 0, 16, SCREEN_WIDTH, SCREEN_HEIGHT);
      } else {
         if (gSavedPad2State & PAD_UP) {
            gGraphicsPtr->dispEnv.disp.y -= 8;
         }
         if (gSavedPad2State & PAD_DOWN) {
            gGraphicsPtr->dispEnv.disp.y += 8;
         }
         if (gSavedPad2State & PAD_LEFT) {
            gGraphicsPtr->dispEnv.disp.x -= 8;
         }
         if (gSavedPad2State & PAD_RIGHT) {
            gGraphicsPtr->dispEnv.disp.x += 8;
         }
         if (gSavedPad2StateNewPresses & PAD_L1) {
            OBJ.widthIdx++;
            OBJ.widthIdx %= 5;
            OBJ.width = widths[OBJ.widthIdx];
         }
         if (gSavedPad2StateNewPresses & PAD_L2) {
            OBJ.heightIdx++;
            OBJ.heightIdx %= 2;
            OBJ.height = heights[OBJ.heightIdx];
         }
         gGraphicsPtr->dispEnv.disp.w = widths[OBJ.widthIdx];
         gGraphicsPtr->dispEnv.disp.h = heights[OBJ.heightIdx];

         g = (gGraphicsPtr == &gGraphicBuffers[0]) ? &gGraphicBuffers[1] : &gGraphicBuffers[0];
         g->dispEnv.disp.x = gGraphicsPtr->dispEnv.disp.x;
         g->dispEnv.disp.y = gGraphicsPtr->dispEnv.disp.y;
         g->dispEnv.disp.w = gGraphicsPtr->dispEnv.disp.w;
         g->dispEnv.disp.h = gGraphicsPtr->dispEnv.disp.h;
      }
      break;
   }
}

void CopyObject(Object *src, Object *dst) {
   s32 i;

#ifdef PC_PORT
   /* PC_PORT width fix: 24 u32 words = 96 bytes = the PSX/-m32 sizeof(Object). On LP64 the
    * union's pointers grow and fields past the boundary (Object_Sprite.animYOfs) fall OUTSIDE
    * the copy -- the level-up hop never lifted. Copy the real struct size. Same gated fix as
    * the US tree (src/core/graphics.c). */
   for (i = 0; i < (s32)(sizeof(Object) / sizeof(u32)); i++) {
      ((u32 *)dst)[i] = ((u32 *)src)[i];
#else
   for (i = 0; i < 24; i++) {
      ((u32 *)dst)[i] = ((u32 *)src)[i];
#endif
   }
}

void SetupLightNormals(void) {
   VECTOR v[26];
   s32 i;

   v[0].vx = 0;
   v[0].vy = 1;
   v[0].vz = 0;
   v[1].vx = 0;
   v[1].vy = -1;
   v[1].vz = 0;
   v[2].vx = 1;
   v[2].vy = 0;
   v[2].vz = 0;
   v[3].vx = 0;
   v[3].vy = 0;
   v[3].vz = 1;
   v[4].vx = -1;
   v[4].vy = 0;
   v[4].vz = 0;
   v[5].vx = 0;
   v[5].vy = 0;
   v[5].vz = -1;
   v[6].vx = 1;
   v[6].vy = 0;
   v[6].vz = -1;
   v[7].vx = 1;
   v[7].vy = 0;
   v[7].vz = 1;
   v[8].vx = -1;
   v[8].vy = 0;
   v[8].vz = 1;
   v[9].vx = -1;
   v[9].vy = 0;
   v[9].vz = -1;
   v[10].vx = 1;
   v[10].vy = 1;
   v[10].vz = 0;
   v[11].vx = 0;
   v[11].vy = 1;
   v[11].vz = 1;
   v[12].vx = -1;
   v[12].vy = 1;
   v[12].vz = 0;
   v[13].vx = 0;
   v[13].vy = 1;
   v[13].vz = -1;
   v[14].vx = 1;
   v[14].vy = 1;
   v[14].vz = -1;
   v[15].vx = 1;
   v[15].vy = 1;
   v[15].vz = 1;
   v[16].vx = -1;
   v[16].vy = 1;
   v[16].vz = 1;
   v[17].vx = -1;
   v[17].vy = 1;
   v[17].vz = -1;
   v[18].vx = 1;
   v[18].vy = -1;
   v[18].vz = 0;
   v[19].vx = 0;
   v[19].vy = -1;
   v[19].vz = 1;
   v[20].vx = -1;
   v[20].vy = -1;
   v[20].vz = 0;
   v[21].vx = 0;
   v[21].vy = -1;
   v[21].vz = -1;
   v[22].vx = 1;
   v[22].vy = -1;
   v[22].vz = -1;
   v[23].vx = 1;
   v[23].vy = -1;
   v[23].vz = 1;
   v[24].vx = -1;
   v[24].vy = -1;
   v[24].vz = 1;
   v[25].vx = -1;
   v[25].vy = -1;
   v[25].vz = -1;

   for (i = 0; i < ARRAY_COUNT(v); i++) {
      VectorNormalS(&v[i], &gSideLightNormals[i]);
   }
}

void UpdateLight(s16 intensity) {
   static s32 indices[10][2] = {{3, 5},   {7, 6},   {8, 9},   {11, 13}, {15, 14},
                                {16, 17}, {19, 21}, {23, 22}, {24, 25}, {-1, -1}};

   MATRIX mtx;
   VECTOR vec;
   SVECTOR svec = {0x1000, 0, 0, 0};
   CVECTOR unused;
   CVECTOR color1 = {128, 128, 128, 0};
   CVECTOR color2;
   s32 flag;
   s16 i;
   s32 idx1, idx2;
   s32 j;

   PushMatrix();
   RotMatrix(&gLightRotation, &mtx);
   mtx.t[0] = 0;
   mtx.t[1] = 0;
   mtx.t[2] = 0;
   SetRotMatrix(&mtx);
   SetTransMatrix(&mtx);
   RotTrans(&svec, &vec, &flag);
   mtx.m[0][0] = -vec.vx;
   mtx.m[0][1] = -vec.vy;
   mtx.m[0][2] = -vec.vz;
   mtx.m[1][0] = 0;
   mtx.m[1][1] = 0;
   mtx.m[1][2] = 0;
   mtx.m[2][0] = 0;
   mtx.m[2][1] = 0;
   mtx.m[2][2] = 0;
   mtx.t[0] = 0;
   mtx.t[1] = 0;
   mtx.t[2] = 0;
   SetLightMatrix(&mtx);

   for (i = 0; i < 26; i++) {
      gte_NormalColorCol(&gSideLightNormals[i], &color1, &color2);
      gSideLightColor[i].r = gLightColor.r * (intensity + (color2.r * (128 - intensity) >> 7)) >> 7;
      gSideLightColor[i].g = gLightColor.g * (intensity + (color2.g * (128 - intensity) >> 7)) >> 7;
      gSideLightColor[i].b = gLightColor.b * (intensity + (color2.b * (128 - intensity) >> 7)) >> 7;
   }
   PopMatrix();

   if (gState.mapNum == 32) {
      for (j = 0; indices[j][0] != -1; j++) {
         idx1 = indices[j][0];
         idx2 = indices[j][1];
         gSideLightColor[idx2].r = gSideLightColor[idx1].r;
         gSideLightColor[idx2].g = gSideLightColor[idx1].g;
         gSideLightColor[idx2].b = gSideLightColor[idx1].b;
      }
   }
}

void UpdateMapLighting(void) {
   s16 ix, iz;
   s8 unused[56];

   for (iz = 0; iz < gMapMaxZ + 1; iz++) {
      for (ix = 0; ix < gMapMaxX + 1; ix++) {
         UpdateMapTileLighting(&gMapRowPointers[iz][ix]);
      }
   }
}

void UpdateMapTileLighting(MapTileModel *tileModel) {
   VECTOR vec_sp10;
   VECTOR vec_sp20;
   VECTOR vec_sp30;
   SVECTOR svec_sp40;

   s16 var_a1, var_a0, var_t0, var_a2;
   s16 i, j;
   s32 greatest;
   s32 iVar6;
   s16 iVar5;
   s32 ct1, ct2;

   PushMatrix();

   for (i = 0; i < tileModel->faceCt; i++) {
      var_a1 = tileModel->faces[i][0];
      var_a0 = tileModel->faces[i][1];
      var_t0 = tileModel->faces[i][2];
      var_a2 = tileModel->faces[i][3];

      if (var_a1 == var_a0) {
         var_a1 = tileModel->faces[i][2];
         var_a0 = tileModel->faces[i][0];
         var_t0 = tileModel->faces[i][3];
      } else if (var_a1 == var_t0) {
         var_a1 = tileModel->faces[i][1];
         var_a0 = tileModel->faces[i][0];
         var_t0 = tileModel->faces[i][3];
      } else if (var_a1 == var_a2) {
         var_a1 = tileModel->faces[i][1];
         var_a0 = tileModel->faces[i][0];
         var_t0 = tileModel->faces[i][2];
      } else if (var_a0 == var_t0) {
         var_a1 = tileModel->faces[i][1];
         var_a0 = tileModel->faces[i][0];
         var_t0 = tileModel->faces[i][3];
      }

      vec_sp20.vx = tileModel->vertices[var_a0].vx - tileModel->vertices[var_a1].vx;
      vec_sp20.vy = tileModel->vertices[var_a0].vy - tileModel->vertices[var_a1].vy;
      vec_sp20.vz = tileModel->vertices[var_a0].vz - tileModel->vertices[var_a1].vz;

      vec_sp30.vx = tileModel->vertices[var_t0].vx - tileModel->vertices[var_a1].vx;
      vec_sp30.vy = tileModel->vertices[var_t0].vy - tileModel->vertices[var_a1].vy;
      vec_sp30.vz = tileModel->vertices[var_t0].vz - tileModel->vertices[var_a1].vz;

      gte_OuterProduct0(&vec_sp20, &vec_sp30, &vec_sp10);
      VectorNormalS(&vec_sp10, &svec_sp40);

      greatest = -0x1000000;
      iVar5 = 0;
      for (j = 0; j < 26; j++) {
         iVar6 = gSideLightNormals[j].vx * svec_sp40.vx + gSideLightNormals[j].vy * svec_sp40.vy +
                 gSideLightNormals[j].vz * svec_sp40.vz;
         if (iVar6 > greatest) {
            iVar5 = j;
            greatest = iVar6;
         } else if (iVar6 == greatest) {
            ct1 = 0;
            ct2 = 0;

            var_a1 = gSideLightNormals[j].vx;
            var_a0 = gSideLightNormals[j].vy;
            var_t0 = gSideLightNormals[j].vz;

            if (svec_sp40.vx > 0 && var_a1 > 0) {
               ct1++;
            } else if (svec_sp40.vx < 0 && var_a1 < 0) {
               ct1++;
            }
            if (svec_sp40.vy > 0 && var_a0 > 0) {
               ct1++;
            } else if (svec_sp40.vy < 0 && var_a0 < 0) {
               ct1++;
            }
            if (svec_sp40.vz > 0 && var_t0 > 0) {
               ct1++;
            } else if (svec_sp40.vz < 0 && var_t0 < 0) {
               ct1++;
            }

            if (svec_sp40.vx == var_a1) {
               ct1++;
            }
            if (svec_sp40.vy == var_a0) {
               ct1++;
            }
            if (svec_sp40.vz == var_t0) {
               ct1++;
            }

            var_a1 = gSideLightNormals[iVar5].vx;
            var_a0 = gSideLightNormals[iVar5].vy;
            var_t0 = gSideLightNormals[iVar5].vz;

            if (svec_sp40.vx > 0 && var_a1 > 0) {
               ct2++;
            } else if (svec_sp40.vx < 0 && var_a1 < 0) {
               ct2++;
            }
            if (svec_sp40.vy > 0 && var_a0 > 0) {
               ct2++;
            } else if (svec_sp40.vy < 0 && var_a0 < 0) {
               ct2++;
            }
            if (svec_sp40.vz > 0 && var_t0 > 0) {
               ct2++;
            } else if (svec_sp40.vz < 0 && var_t0 < 0) {
               ct2++;
            }

            if (svec_sp40.vx == var_a1) {
               ct2++;
            }
            if (svec_sp40.vy == var_a0) {
               ct2++;
            }
            if (svec_sp40.vz == var_t0) {
               ct2++;
            }

            if (ct1 > ct2) {
               iVar5 = j;
            }
         }

         tileModel->shades[i] = iVar5;
      }
   }

   PopMatrix();
}

void ApplyMaskEffect(s16 spriteX, s16 spriteY, s16 spriteW, s16 spriteH, s16 textureX, s16 textureY,
                     s16 horizontalScroll, s16 verticalScroll, s16 gfxIdx, s16 bgColor) {
   u32 spriteData[512];
   u32 textureData[32][4];
   u32 tiledTextureData[64][8];
   RECT rect;
   s32 i;
   u32 j;
   u32 x, y; //
   u32 hShift;
   u32 inputWord1, inputWord2, inputWord3, inputWord4;
   u32 outputWord1, outputWord2, outputWord3, outputWord4;
   u32 *p;
   u32 c;

   p = &spriteData[0];

   if (horizontalScroll < 0) {
      horizontalScroll += 64;
   }
   if (verticalScroll < 0) {
      verticalScroll += 64;
   }

   rect.x = textureX / 4;
   rect.y = textureY;
   rect.w = 32 / 4;
   rect.h = 32;
   StoreImage(&rect, textureData);

   rect.x = spriteX / 4;
   rect.y = spriteY;
   rect.w = spriteW / 4;
   rect.h = spriteH;

   // (There are eight 4-bit pixels per u32)
   x = horizontalScroll / 8;
   y = verticalScroll;
   c = bgColor;
   hShift = horizontalScroll % 8 * 4;
   DrawSync(0);
   StoreImage(&rect, p);

   // Scroll and tile texture
   for (i = 0; i < 32; i++) {
      inputWord1 = textureData[(i + y) & 0x1f][(x + 0) & 3];
      inputWord2 = textureData[(i + y) & 0x1f][(x + 1) & 3];
      inputWord3 = textureData[(i + y) & 0x1f][(x + 2) & 3];
      inputWord4 = textureData[(i + y) & 0x1f][(x + 3) & 3];

      if (c != 0) {
         // Replace texture's transparent pixels with given bgColor
         if ((inputWord1 & 0xf0000000) == 0) {
            inputWord1 |= (c << 28);
         }
         if ((inputWord1 & 0x0f000000) == 0) {
            inputWord1 |= (c << 24);
         }
         if ((inputWord1 & 0x00f00000) == 0) {
            inputWord1 |= (c << 20);
         }
         if ((inputWord1 & 0x000f0000) == 0) {
            inputWord1 |= (c << 16);
         }
         if ((inputWord1 & 0x0000f000) == 0) {
            inputWord1 |= (c << 12);
         }
         if ((inputWord1 & 0x00000f00) == 0) {
            inputWord1 |= (c << 8);
         }
         if ((inputWord1 & 0x000000f0) == 0) {
            inputWord1 |= (c << 4);
         }
         if ((inputWord1 & 0x0000000f) == 0) {
            inputWord1 |= c;
         }

         if ((inputWord2 & 0xf0000000) == 0) {
            inputWord2 |= (c << 28);
         }
         if ((inputWord2 & 0x0f000000) == 0) {
            inputWord2 |= (c << 24);
         }
         if ((inputWord2 & 0x00f00000) == 0) {
            inputWord2 |= (c << 20);
         }
         if ((inputWord2 & 0x000f0000) == 0) {
            inputWord2 |= (c << 16);
         }
         if ((inputWord2 & 0x0000f000) == 0) {
            inputWord2 |= (c << 12);
         }
         if ((inputWord2 & 0x00000f00) == 0) {
            inputWord2 |= (c << 8);
         }
         if ((inputWord2 & 0x000000f0) == 0) {
            inputWord2 |= (c << 4);
         }
         if ((inputWord2 & 0x0000000f) == 0) {
            inputWord2 |= c;
         }

         if ((inputWord3 & 0xf0000000) == 0) {
            inputWord3 |= (c << 28);
         }
         if ((inputWord3 & 0x0f000000) == 0) {
            inputWord3 |= (c << 24);
         }
         if ((inputWord3 & 0x00f00000) == 0) {
            inputWord3 |= (c << 20);
         }
         if ((inputWord3 & 0x000f0000) == 0) {
            inputWord3 |= (c << 16);
         }
         if ((inputWord3 & 0x0000f000) == 0) {
            inputWord3 |= (c << 12);
         }
         if ((inputWord3 & 0x00000f00) == 0) {
            inputWord3 |= (c << 8);
         }
         if ((inputWord3 & 0x000000f0) == 0) {
            inputWord3 |= (c << 4);
         }
         if ((inputWord3 & 0x0000000f) == 0) {
            inputWord3 |= c;
         }

         if ((inputWord4 & 0xf0000000) == 0) {
            inputWord4 |= (c << 28);
         }
         if ((inputWord4 & 0x0f000000) == 0) {
            inputWord4 |= (c << 24);
         }
         if ((inputWord4 & 0x00f00000) == 0) {
            inputWord4 |= (c << 20);
         }
         if ((inputWord4 & 0x000f0000) == 0) {
            inputWord4 |= (c << 16);
         }
         if ((inputWord4 & 0x0000f000) == 0) {
            inputWord4 |= (c << 12);
         }
         if ((inputWord4 & 0x00000f00) == 0) {
            inputWord4 |= (c << 8);
         }
         if ((inputWord4 & 0x000000f0) == 0) {
            inputWord4 |= (c << 4);
         }
         if ((inputWord4 & 0x0000000f) == 0) {
            inputWord4 |= c;
         }
      }

      if (hShift != 0) {
         outputWord1 = (inputWord1 >> hShift) + (inputWord2 << (32 - hShift));
         outputWord2 = (inputWord2 >> hShift) + (inputWord3 << (32 - hShift));
         outputWord3 = (inputWord3 >> hShift) + (inputWord4 << (32 - hShift));
         outputWord4 = (inputWord4 >> hShift) + (inputWord1 << (32 - hShift));
      } else {
         outputWord1 = inputWord1;
         outputWord2 = inputWord2;
         outputWord3 = inputWord3;
         outputWord4 = inputWord4;
      }
      // t1,a2,a0,v1
      // a1,a2,a3,t0
      tiledTextureData[i][0] = outputWord1;
      tiledTextureData[i][4] = outputWord1;
      tiledTextureData[i + 32][0] = outputWord1;
      tiledTextureData[i + 32][4] = outputWord1;
      tiledTextureData[i][1] = outputWord2;
      tiledTextureData[i][5] = outputWord2;
      tiledTextureData[i + 32][1] = outputWord2;
      tiledTextureData[i + 32][5] = outputWord2;
      tiledTextureData[i][2] = outputWord3;
      tiledTextureData[i][6] = outputWord3;
      tiledTextureData[i + 32][2] = outputWord3;
      tiledTextureData[i + 32][6] = outputWord3;
      tiledTextureData[i][3] = outputWord4;
      tiledTextureData[i][7] = outputWord4;
      tiledTextureData[i + 32][3] = outputWord4;
      tiledTextureData[i + 32][7] = outputWord4;
   }

   x = spriteW / 8;
   DrawSync(0);

   // Replace the sprite's non-transparent pixels with those of the tiled texture
   for (i = 0; i < spriteH; i++) {
      y = i * x;
      for (j = 0; j < x; j++) {
         u32 mask = p[y + j];

         if (mask & 0xf0000000) {
            mask |= 0xf0000000;
         }
         if (mask & 0x0f000000) {
            mask |= 0x0f000000;
         }
         if (mask & 0x00f00000) {
            mask |= 0x00f00000;
         }
         if (mask & 0x000f0000) {
            mask |= 0x000f0000;
         }
         if (mask & 0x0000f000) {
            mask |= 0x0000f000;
         }
         if (mask & 0x00000f00) {
            mask |= 0x00000f00;
         }
         if (mask & 0x000000f0) {
            mask |= 0x000000f0;
         }
         if (mask & 0x0000000f) {
            mask |= 0x0000000f;
         }

         p[y + j] = mask & tiledTextureData[i][j];
      }
   }

   // Rows of four 64x64 slots for masked results
   x = (gfxIdx - GFX_MASK_EFFECT_1) % 4 * 64;
   y = (gfxIdx - GFX_MASK_EFFECT_1) / 4 * 64;
   rect.x = 576 + x / 4;
   rect.y = 256 + y;
   rect.w = spriteW / 4;
   rect.h = spriteH;
   LoadImage(&rect, p);
   gGfxSubTextures[gfxIdx][0] = x;
   gGfxSubTextures[gfxIdx][1] = y;
   gGfxSubTextures[gfxIdx][2] = spriteW - 1;
   gGfxSubTextures[gfxIdx][3] = spriteH - 1;
   DrawSync(0);
}

s16 D_80123358;
s16 D_8012335C;

void RenderField(void) {
   s32 ix, iz;
   s32 endX, endZ, startX, startZ, xEdgeStartX, zEdgeX, zEdgeStartZ, xEdgeZ;

   if (gOverheadMapState == 1) {
      gOverheadMapState = 2;
      D_80123358 = gMapViewOriginX;
      D_8012335C = gMapViewOriginZ;
   }

   if (gOverheadMapState != 0) {
      for (iz = gMapMinZ; iz <= gMapMaxZ; iz++) {
         if (gState.mapNum == 32 && (iz == gMapMinZ || iz == gMapMaxZ)) {
            continue;
         }
         for (ix = gMapMinX; ix <= gMapMaxX; ix++) {
            RenderOverheadMapTile(gGraphicsPtr->ot, &gMapRowPointers[iz][ix], 128);
         }
      }
      return;
   }

   switch ((gCameraRotation.vy & 0xfff) >> 10) {
   case CAM_DIR_SOUTH:
      endX = gMapViewOriginX + gMapSizeX - 2;
      startX = gMapViewOriginX;
      endZ = gMapViewOriginZ + gMapSizeZ - 1;
      startZ = gMapViewOriginZ + 1;
      xEdgeStartX = gMapViewOriginX;
      xEdgeZ = gMapViewOriginZ;
      zEdgeX = gMapViewOriginX + gMapSizeX - 1;
      zEdgeStartZ = startZ;
      break;
   case CAM_DIR_EAST:
      endX = gMapViewOriginX + gMapSizeX - 2;
      startX = gMapViewOriginX;
      endZ = gMapViewOriginZ + gMapSizeZ - 2;
      startZ = gMapViewOriginZ;
      xEdgeStartX = gMapViewOriginX;
      xEdgeZ = gMapViewOriginZ + gMapSizeZ - 1;
      zEdgeX = gMapViewOriginX + gMapSizeX - 1;
      zEdgeStartZ = startZ;
      break;
   case CAM_DIR_NORTH:
      endX = gMapViewOriginX + gMapSizeX - 1;
      startX = gMapViewOriginX + 1;
      endZ = gMapViewOriginZ + gMapSizeZ - 2;
      startZ = gMapViewOriginZ;
      xEdgeStartX = gMapViewOriginX;
      xEdgeZ = gMapViewOriginZ + gMapSizeZ - 1;
      zEdgeX = gMapViewOriginX;
      zEdgeStartZ = startZ;
      break;
   case CAM_DIR_WEST:
      endX = gMapViewOriginX + gMapSizeX - 1;
      startX = gMapViewOriginX + 1;
      endZ = gMapViewOriginZ + gMapSizeZ - 1;
      startZ = gMapViewOriginZ + 1;
      xEdgeStartX = gMapViewOriginX;
      xEdgeZ = gMapViewOriginZ;
      zEdgeX = gMapViewOriginX;
      zEdgeStartZ = startZ;
      break;
   }

   for (iz = startZ; iz <= endZ; iz++) {
      for (ix = startX; ix <= endX; ix++) {
         if (gYellowTargetGridPtr[iz][ix] != 0) {
            RenderMapTile(gGraphicsPtr->ot, &gMapRowPointers[iz][ix], GRID_COLOR_YELLOW);
         } else if (gRedAttackGridPtr[iz][ix] != 0) {
            RenderMapTile(gGraphicsPtr->ot, &gMapRowPointers[iz][ix], GRID_COLOR_RED);
         } else if (gBlueMovementGridPtr[iz][ix] != 0) {
#ifdef PC_FEAT
            /* Stage 3 (1.1B): a reachable tile that is ALSO enemy-threatened -> warn in orange, so
             * the player still sees their own extra movement AND the danger (bugreport-02 #3), and the
             * warning stays distinct from the native yellow attack/AoE target grid (bugreport-04).
             * Hidden while control is suppressed (mid-animation): during a unit walk / crate push the
             * board is changing, so a not-yet-recomputed overlay would be stale -- show nothing and
             * let it reappear correct when control returns (bugreport-02 #2 3rd check). */
            if (gShowThreatGrid && !gPlayerControlSuppressed && gThreatGridPtr[iz][ix] != 0) {
               RenderMapTile(gGraphicsPtr->ot, &gMapRowPointers[iz][ix], GRID_COLOR_ORANGE);
            } else
#endif
            RenderMapTile(gGraphicsPtr->ot, &gMapRowPointers[iz][ix], GRID_COLOR_BLUE);
#ifdef PC_FEAT
         /* Stage 3 (1.1B): pure enemy threat sits BELOW the active unit's move/target grids so
          * casting AoE and movement previews stay visible on top (bugreport-02 #4); and it is hidden
          * mid-animation (see above) so a stale danger zone is never drawn. */
         } else if (gShowThreatGrid && !gPlayerControlSuppressed && gThreatGridPtr[iz][ix] != 0) {
            RenderMapTile(gGraphicsPtr->ot, &gMapRowPointers[iz][ix], GRID_COLOR_PURPLE);
#endif
         } else {
            RenderMapTile(gGraphicsPtr->ot, &gMapRowPointers[iz][ix], GRID_COLOR_NONE);
         }
      }
   }

   iz = xEdgeZ;
   for (ix = xEdgeStartX; ix < xEdgeStartX + gMapSizeX; ix++) {
      if (gYellowTargetGridPtr[iz][ix] != 0) {
         RenderEdgeMapTile(gGraphicsPtr->ot, &gMapRowPointers[iz][ix], GRID_COLOR_YELLOW);
      } else if (gRedAttackGridPtr[iz][ix] != 0) {
         RenderEdgeMapTile(gGraphicsPtr->ot, &gMapRowPointers[iz][ix], GRID_COLOR_RED);
      } else if (gBlueMovementGridPtr[iz][ix] != 0) {
#ifdef PC_FEAT
         /* Stage 3 (1.1B): mirror the interior loop's threat handling on the map's edge row/column.
          * Without it, a genuinely enemy-threatened boundary tile lost its purple at camera angles
          * where the edge loop (not the interior loop) draws that row -- the overlay flickered off on
          * rotation (bugreport-01 follow-up 2026-07-26). A reachable+threatened edge tile warns orange
          * (distinct from the native yellow attack/AoE grid -- bugreport-04). */
         if (gShowThreatGrid && !gPlayerControlSuppressed && gThreatGridPtr[iz][ix] != 0) {
            RenderEdgeMapTile(gGraphicsPtr->ot, &gMapRowPointers[iz][ix], GRID_COLOR_ORANGE);
         } else
#endif
         RenderEdgeMapTile(gGraphicsPtr->ot, &gMapRowPointers[iz][ix], GRID_COLOR_BLUE);
#ifdef PC_FEAT
      } else if (gShowThreatGrid && !gPlayerControlSuppressed && gThreatGridPtr[iz][ix] != 0) {
         RenderEdgeMapTile(gGraphicsPtr->ot, &gMapRowPointers[iz][ix], GRID_COLOR_PURPLE);
#endif
      } else {
         RenderEdgeMapTile(gGraphicsPtr->ot, &gMapRowPointers[iz][ix], GRID_COLOR_NONE);
      }
   }

   ix = zEdgeX;
   for (iz = zEdgeStartZ; iz < zEdgeStartZ + gMapSizeZ - 1; iz++) {
      if (gYellowTargetGridPtr[iz][ix] != 0) {
         RenderEdgeMapTile(gGraphicsPtr->ot, &gMapRowPointers[iz][ix], GRID_COLOR_YELLOW);
      } else if (gRedAttackGridPtr[iz][ix] != 0) {
         RenderEdgeMapTile(gGraphicsPtr->ot, &gMapRowPointers[iz][ix], GRID_COLOR_RED);
      } else if (gBlueMovementGridPtr[iz][ix] != 0) {
#ifdef PC_FEAT
         /* Stage 3 (1.1B): mirror the interior loop's threat handling on the map's edge row/column.
          * Without it, a genuinely enemy-threatened boundary tile lost its purple at camera angles
          * where the edge loop (not the interior loop) draws that row -- the overlay flickered off on
          * rotation (bugreport-01 follow-up 2026-07-26). A reachable+threatened edge tile warns orange
          * (distinct from the native yellow attack/AoE grid -- bugreport-04). */
         if (gShowThreatGrid && !gPlayerControlSuppressed && gThreatGridPtr[iz][ix] != 0) {
            RenderEdgeMapTile(gGraphicsPtr->ot, &gMapRowPointers[iz][ix], GRID_COLOR_ORANGE);
         } else
#endif
         RenderEdgeMapTile(gGraphicsPtr->ot, &gMapRowPointers[iz][ix], GRID_COLOR_BLUE);
#ifdef PC_FEAT
      } else if (gShowThreatGrid && !gPlayerControlSuppressed && gThreatGridPtr[iz][ix] != 0) {
         RenderEdgeMapTile(gGraphicsPtr->ot, &gMapRowPointers[iz][ix], GRID_COLOR_PURPLE);
#endif
      } else {
         RenderEdgeMapTile(gGraphicsPtr->ot, &gMapRowPointers[iz][ix], GRID_COLOR_NONE);
      }
   }
}

void RenderMapTile(u32 *ot, MapTileModel *tileModel, s32 gridColor) {
   s32 faceIdx;
   POLY_FT4 *poly;
   u8 *pFaceVerts;
   s32 gfx;
   u8 texX, texY, texW, texH;
   s32 opz;
   s32 otz;
   s16 shading;
   u8 r1, g1, b1;
   s16 r2, g2, b2;

   poly = &gGraphicsPtr->quads[gQuadIndex];
   for (faceIdx = 0; faceIdx < tileModel->faceCt; faceIdx++) {
      gfx = tileModel->gfx[faceIdx];
      if (gfx != GFX_NULL) {
         pFaceVerts = &tileModel->faces[faceIdx][0];
         gte_ldv3(&tileModel->vertices[pFaceVerts[0]], &tileModel->vertices[pFaceVerts[1]],
                  &tileModel->vertices[pFaceVerts[2]]);
         gte_rtpt();
         gte_nclip();
         gte_stopz(&opz);
         if (opz > 0) {
            gte_stsxy3(&poly->x0, &poly->x1, &poly->x3);
            gte_ldv0(&tileModel->vertices[pFaceVerts[3]]);
            gte_rtps();
            gte_stsxy(&poly->x2);
            gte_avsz4();
            gte_stotz(&otz);
            if (otz < 5) {
               otz = 5;
            }
            poly->clut = gGfxClutIds[gfx];
            poly->tpage = gGfxTPageIds[gfx];
            texX = gGfxSubTextures[gfx][0];
            texY = gGfxSubTextures[gfx][1];
            texW = gGfxSubTextures[gfx][2];
            texH = gGfxSubTextures[gfx][3];
            setUVWH(poly, texX, texY, texW, texH);

            // Darken distant faces
            shading = otz - 150;
            if (shading < 0) {
               shading = 0;
            } else if (shading > 256) {
               shading = 256;
            }
            r2 = gSideLightColor[tileModel->shades[faceIdx]].r;
            r1 = gSideLightColor[tileModel->shades[faceIdx]].r;
            g2 = gSideLightColor[tileModel->shades[faceIdx]].g;
            g1 = gSideLightColor[tileModel->shades[faceIdx]].g;
            b2 = gSideLightColor[tileModel->shades[faceIdx]].b;
            b1 = gSideLightColor[tileModel->shades[faceIdx]].b;
            r2 *= shading;
            g2 *= shading;
            b2 *= shading;
            poly->r0 = r1 - (r2 >> 8);
            poly->g0 = g1 - (g2 >> 8);
            poly->b0 = b1 - (b2 >> 8);

            if (faceIdx == 0 && gridColor != GRID_COLOR_NONE) {
               if (gridColor == GRID_COLOR_BLUE && gShowBlueMovementGrid) {
                  if (!gState.previewingRange) {
                     poly->r0 = 40;
                     poly->g0 = 40;
                     poly->b0 = gGridColorOscillation;
                  } else {
                     // Use a lighter blue when previewing enemy range
                     poly->r0 = 70;
                     poly->b0 = gGridColorOscillation;
                     poly->g0 = 70;
                  }
               } else if (gridColor == GRID_COLOR_RED) {
                  poly->r0 = gGridColorOscillation;
                  poly->g0 = 40;
                  poly->b0 = 40;
               } else if (gridColor == GRID_COLOR_YELLOW) {
                  poly->r0 = gGridColorOscillation;
                  poly->g0 = gGridColorOscillation;
                  poly->b0 = 0;
#ifdef PC_FEAT
               } else if (gridColor == GRID_COLOR_PURPLE) {
                  /* Stage 3 (1.1) enemy threat: pulsing purple, distinct from blue/red/yellow. It drives
                   * TWO channels (red+blue), so at the shared oscillation's 255 peak it reads as a neon
                   * fluorescent magenta -- eye-tiring over a large danger zone (bugreport 2026-07-26).
                   * Soften it: halve the pulse amplitude (peak ~148 not 255) and keep green below r/b so
                   * it desaturates into a calmer purple that still stays clearly purple at the trough. */
                  int soft = 40 + ((gGridColorOscillation - 40) >> 1);   /* ~47..148 vs 55..255 */
                  poly->r0 = soft;
                  poly->g0 = 30 + (soft >> 2);                            /* muted, not neon */
                  poly->b0 = soft;
               } else if (gridColor == GRID_COLOR_ORANGE) {
                  /* Stage 3 (1.3): reachable-AND-threatened warning. Orange (red, ~half green, no blue)
                   * reads clearly apart from the native YELLOW attack/AoE grid (where r==g). Pulse
                   * softened the same way as PURPLE -- halved amplitude (peak ~148 not 255) -- so it's
                   * flat, not neon (bugreport-04). */
                  int soft = 40 + ((gGridColorOscillation - 40) >> 1);   /* ~47..148 vs 55..255 */
                  poly->r0 = soft;
                  poly->g0 = soft >> 1;                                   /* ~23..74 -> orange hue */
                  poly->b0 = 0;
#endif
               }
            }

            poly->code = GPU_CODE_POLY_FT4;
            AddPrim(ot + OT_SIZE - otz, poly);
            poly = &gGraphicsPtr->quads[++gQuadIndex];
         }
      }
   }
}

void RenderEdgeMapTile(u32 *ot, MapTileModel *tileModel, s32 gridColor) {
   s32 faceIdx;
   POLY_FT4 *poly;
   u8 *pFaceVerts;
   s32 gfx;
   u8 texX, texY, texW, texH;
   s32 opz;
   s32 otz;
   s16 shading;
   u8 r1, g1, b1;
   s16 r2, g2, b2;

   poly = &gGraphicsPtr->quads[gQuadIndex];
   for (faceIdx = 0; faceIdx < tileModel->faceCt; faceIdx++) {
      pFaceVerts = &tileModel->faces[faceIdx][0];
      gte_ldv3(&tileModel->vertices[pFaceVerts[0]], &tileModel->vertices[pFaceVerts[1]],
               &tileModel->vertices[pFaceVerts[2]]);
      gte_rtpt();
      gte_nclip();
      gte_stopz(&opz);
      if (opz > 0) {
         gte_stsxy3(&poly->x0, &poly->x1, &poly->x3);
         gte_ldv0(&tileModel->vertices[pFaceVerts[3]]);
         gte_rtps();
         gte_stsxy(&poly->x2);
         gte_avsz4();
         gte_stotz(&otz);
         if (otz < 5) {
            otz = 5;
         }
         gfx = tileModel->gfx[faceIdx];
         poly->clut = gGfxClutIds[gfx];
         poly->tpage = gGfxTPageIds[gfx];
         texX = gGfxSubTextures[gfx][0];
         texY = gGfxSubTextures[gfx][1];
         texW = gGfxSubTextures[gfx][2];
         texH = gGfxSubTextures[gfx][3];
         setUVWH(poly, texX, texY, texW, texH);

         // Darken distant faces
         shading = otz - 150;
         if (shading < 0) {
            shading = 0;
         } else if (shading > 256) {
            shading = 256;
         }
         r2 = gSideLightColor[tileModel->shades[faceIdx]].r;
         r1 = gSideLightColor[tileModel->shades[faceIdx]].r;
         g2 = gSideLightColor[tileModel->shades[faceIdx]].g;
         g1 = gSideLightColor[tileModel->shades[faceIdx]].g;
         b2 = gSideLightColor[tileModel->shades[faceIdx]].b;
         b1 = gSideLightColor[tileModel->shades[faceIdx]].b;
         r2 *= shading;
         g2 *= shading;
         b2 *= shading;
         poly->r0 = r1 - (r2 >> 8);
         poly->g0 = g1 - (g2 >> 8);
         poly->b0 = b1 - (b2 >> 8);

         if (faceIdx == 0 && gridColor != GRID_COLOR_NONE) {
            if (gridColor == GRID_COLOR_BLUE && gShowBlueMovementGrid) {
               if (!gState.previewingRange) {
                  poly->r0 = 40;
                  poly->g0 = 40;
                  poly->b0 = gGridColorOscillation;
               } else {
                  // Use a lighter blue when previewing enemy range
                  poly->g0 = 70;
                  poly->b0 = gGridColorOscillation;
                  poly->r0 = 70;
               }
            } else if (gridColor == GRID_COLOR_RED) {
               poly->r0 = gGridColorOscillation;
               poly->g0 = 40;
               poly->b0 = 40;
            } else if (gridColor == GRID_COLOR_YELLOW) {
               poly->r0 = gGridColorOscillation;
               poly->g0 = gGridColorOscillation;
               poly->b0 = 0;
#ifdef PC_FEAT
            } else if (gridColor == GRID_COLOR_PURPLE) {
               /* Stage 3 (1.1): enemy-threat tint on an edge tile -- the same softened purple as the
                * interior RenderMapTile (halved pulse, muted green; see the note there). */
               int soft = 40 + ((gGridColorOscillation - 40) >> 1);
               poly->r0 = soft;
               poly->g0 = 30 + (soft >> 2);
               poly->b0 = soft;
            } else if (gridColor == GRID_COLOR_ORANGE) {
               /* Stage 3 (1.3): reachable-AND-threatened warning on an edge tile -- orange, matching
                * the interior RenderMapTile (softened/halved pulse; see note there; bugreport-04). */
               int soft = 40 + ((gGridColorOscillation - 40) >> 1);
               poly->r0 = soft;
               poly->g0 = soft >> 1;
               poly->b0 = 0;
#endif
            }
         }

         poly->code = GPU_CODE_POLY_FT4;
         AddPrim(ot + OT_SIZE - otz, poly);
         poly = &gGraphicsPtr->quads[++gQuadIndex];
      }
   }
}

void RenderOverheadMapTile(u32 *ot, MapTileModel *tileModel, s32 brightness) {
   s32 faceIdx;
   POLY_FT4 *poly;
   u8 *pFaceVerts;
   s32 gfx;
   s32 opz;
   s32 otz;
   s32 shade; // TODO: rename shade stuff

   poly = &gGraphicsPtr->quads[gQuadIndex];
   for (faceIdx = 0; faceIdx < tileModel->faceCt; faceIdx++) {
      gfx = tileModel->gfx[faceIdx];
      if (gfx != GFX_NULL) {
         shade = tileModel->shades[faceIdx];
         if ((gState.mapNum == 31 || gState.mapNum == 32) && (shade > 0) &&
             (shade < 10 || shade > 18)) {
            continue;
         }
         pFaceVerts = &tileModel->faces[faceIdx][0];
         gte_ldv3(&tileModel->vertices[pFaceVerts[0]], &tileModel->vertices[pFaceVerts[1]],
                  &tileModel->vertices[pFaceVerts[2]]);
         gte_rtpt();
         gte_nclip();
         gte_stopz(&opz);
         if (opz > 0) {
            gte_stsxy3(&poly->x0, &poly->x1, &poly->x3);
            gte_ldv0(&tileModel->vertices[pFaceVerts[3]]);
            gte_rtps();
            gte_stsxy(&poly->x2);
            gte_avsz4();
            gte_stotz(&otz);
            if (otz > OT_SIZE - 1) {
               otz = OT_SIZE - 1;
            }
            if (otz < 5) {
               otz = 5;
            }
            poly->clut = gGfxClutIds[gfx];
            poly->tpage = gGfxTPageIds[gfx];

            setUVWH(poly, gGfxSubTextures[gfx][0], gGfxSubTextures[gfx][1], gGfxSubTextures[gfx][2],
                    gGfxSubTextures[gfx][3]);
            setRGB0(poly, brightness, brightness, brightness);

            poly->code = GPU_CODE_POLY_FT4;
            AddPrim(ot + OT_SIZE - otz, poly);
            poly = &gGraphicsPtr->quads[++gQuadIndex];
         }
      }
   }
}

s32 GetBestViewOfTarget(s8 z, s8 x, s32 param_3) {
   s16 angleToCheck;
   s16 cameraAngle;
   s16 obstructionCt; //
   s16 bestObstructionCt;
   s16 bestAngle;
   u8 dir;
   s32 result;

   cameraAngle = gCameraRotation.vy & 0xfff;
   if (gTargetX > x) {
      dir = DIR_WEST;
   }
   if (gTargetX < x) {
      dir = DIR_EAST;
   }
   if (gTargetZ > z) {
      dir = DIR_SOUTH;
   }
   if (gTargetZ < z) {
      dir = DIR_NORTH;
   }

   obstructionCt = CountViewObstructions(z, x, cameraAngle, dir, param_3);
   bestObstructionCt = obstructionCt;

   if (obstructionCt == 0) {
      result = (cameraAngle >> 10) << 10;
   } else {
      bestAngle = cameraAngle;

      angleToCheck = (cameraAngle + DEG(90)) & 0xfff;
      obstructionCt = CountViewObstructions(z, x, angleToCheck, dir, param_3);
      if (bestObstructionCt > obstructionCt) {
         bestAngle = angleToCheck;
         bestObstructionCt = obstructionCt;
      }

      angleToCheck = (cameraAngle - DEG(90)) & 0xfff;
      obstructionCt = CountViewObstructions(z, x, angleToCheck, dir, param_3);
      if (bestObstructionCt > obstructionCt) {
         bestAngle = angleToCheck;
         bestObstructionCt = obstructionCt;
      }

      angleToCheck = (cameraAngle + DEG(180)) & 0xfff;
      obstructionCt = CountViewObstructions(z, x, angleToCheck, dir, param_3);
      if (bestObstructionCt > obstructionCt) {
         bestAngle = angleToCheck;
         bestObstructionCt = obstructionCt;
      }
      // Truncate to one of the four primary directions
      result = bestAngle & 0xc00;
   }

   return result + DEG(45);
}

/* GetBestViewOfTarget variant starting the obstruction search from the OPPOSITE yaw
 * (camera + 180deg); the battle camera (battle/presentation.c) picks among the three offset
 * variants for shot variety. */
s32 GetBestViewOfTargetPlus180(s8 z, s8 x, s32 param_3) {
   s16 angleToCheck;
   s16 cameraAngle;
   s16 obstructionCt;
   s16 bestObstructionCt;
   s16 bestAngle;
   u8 dir;
   s32 result;

   cameraAngle = (gCameraRotation.vy + DEG(180)) & 0xfff;
   if (gTargetX > x) {
      dir = DIR_WEST;
   }
   if (gTargetX < x) {
      dir = DIR_EAST;
   }
   if (gTargetZ > z) {
      dir = DIR_SOUTH;
   }
   if (gTargetZ < z) {
      dir = DIR_NORTH;
   }

   obstructionCt = CountViewObstructions(z, x, cameraAngle, dir, param_3);
   bestObstructionCt = obstructionCt;

   if (obstructionCt == 0) {
      result = (cameraAngle >> 10) << 10;
   } else {
      bestAngle = cameraAngle;

      angleToCheck = (cameraAngle + DEG(90)) & 0xfff;
      obstructionCt = CountViewObstructions(z, x, angleToCheck, dir, param_3);
      if (bestObstructionCt > obstructionCt) {
         bestAngle = angleToCheck;
         bestObstructionCt = obstructionCt;
      }

      angleToCheck = (cameraAngle - DEG(90)) & 0xfff;
      obstructionCt = CountViewObstructions(z, x, angleToCheck, dir, param_3);
      if (bestObstructionCt > obstructionCt) {
         bestAngle = angleToCheck;
         bestObstructionCt = obstructionCt;
      }

      angleToCheck = (cameraAngle + DEG(180)) & 0xfff;
      obstructionCt = CountViewObstructions(z, x, angleToCheck, dir, param_3);
      if (bestObstructionCt > obstructionCt) {
         bestAngle = angleToCheck;
         bestObstructionCt = obstructionCt;
      }

      result = bestAngle & 0xc00;
   }

   return result + DEG(45);
}

s32 GetBestViewOfTargetMinus90(s8 z, s8 x, s32 param_3) {
   s16 angleToCheck;
   s16 cameraAngle;
   s16 obstructionCt;
   s16 bestObstructionCt;
   s16 bestAngle;
   u8 dir;
   s32 result;

   cameraAngle = (gCameraRotation.vy - DEG(90)) & 0xfff;
   if (gTargetX > x) {
      dir = DIR_WEST;
   }
   if (gTargetX < x) {
      dir = DIR_EAST;
   }
   if (gTargetZ > z) {
      dir = DIR_SOUTH;
   }
   if (gTargetZ < z) {
      dir = DIR_NORTH;
   }

   obstructionCt = CountViewObstructions(z, x, cameraAngle, dir, param_3);
   bestObstructionCt = obstructionCt;

   if (obstructionCt == 0) {
      result = (cameraAngle >> 10) << 10;
   } else {
      bestAngle = cameraAngle;

      angleToCheck = (cameraAngle + DEG(180)) & 0xfff;
      obstructionCt = CountViewObstructions(z, x, angleToCheck, dir, param_3);
      if (bestObstructionCt > obstructionCt) {
         bestAngle = angleToCheck;
         bestObstructionCt = obstructionCt;
      }

      angleToCheck = (cameraAngle - DEG(90)) & 0xfff;
      obstructionCt = CountViewObstructions(z, x, angleToCheck, dir, param_3);
      if (bestObstructionCt > obstructionCt) {
         bestAngle = angleToCheck;
         bestObstructionCt = obstructionCt;
      }

      angleToCheck = (cameraAngle + DEG(90)) & 0xfff;
      obstructionCt = CountViewObstructions(z, x, angleToCheck, dir, param_3);
      if (bestObstructionCt > obstructionCt) {
         bestAngle = angleToCheck;
         bestObstructionCt = obstructionCt;
      }

      result = bestAngle & 0xc00;
   }

   return result + DEG(45);
}

s32 GetBestViewOfTargetPlus90(s8 z, s8 x, s32 param_3) {
   s16 angleToCheck;
   s16 cameraAngle;
   s16 obstructionCt;
   s16 bestObstructionCt;
   s16 bestAngle;
   u8 dir;
   s32 result;

   cameraAngle = (gCameraRotation.vy + DEG(90)) & 0xfff;
   if (gTargetX > x) {
      dir = DIR_WEST;
   }
   if (gTargetX < x) {
      dir = DIR_EAST;
   }
   if (gTargetZ > z) {
      dir = DIR_SOUTH;
   }
   if (gTargetZ < z) {
      dir = DIR_NORTH;
   }

   obstructionCt = CountViewObstructions(z, x, cameraAngle, dir, param_3);
   bestObstructionCt = obstructionCt;

   if (obstructionCt == 0) {
      result = (cameraAngle >> 10) << 10;
   } else {
      bestAngle = cameraAngle;

      angleToCheck = (cameraAngle - DEG(180)) & 0xfff;
      obstructionCt = CountViewObstructions(z, x, angleToCheck, dir, param_3);
      if (bestObstructionCt > obstructionCt) {
         bestAngle = angleToCheck;
         bestObstructionCt = obstructionCt;
      }

      angleToCheck = (cameraAngle + DEG(90)) & 0xfff;
      obstructionCt = CountViewObstructions(z, x, angleToCheck, dir, param_3);
      if (bestObstructionCt > obstructionCt) {
         bestAngle = angleToCheck;
         bestObstructionCt = obstructionCt;
      }

      angleToCheck = (cameraAngle - DEG(90)) & 0xfff;
      obstructionCt = CountViewObstructions(z, x, angleToCheck, dir, param_3);
      if (bestObstructionCt > obstructionCt) {
         bestAngle = angleToCheck;
         bestObstructionCt = obstructionCt;
      }

      result = bestAngle & 0xc00;
   }

   return result + DEG(45);
}

//? Count, followed by count tile offsets within viewable range? (CHECKME)
static s8 sViewObstructionOfsFlat[] = {
    33, 1,  0,  1,  0,  -1, 1,   1,  -1, 1,   2,  -1, 2,   1,  -2, 2,   2,  -2, 2,   3,
    -2, 4,  2,  -3, 4,  3,  -3,  4,  4,  -3,  6,  3,  -4,  6,  4,  -4,  6,  5,  -4,  8,
    4,  -5, 8,  5,  -5, 8,  6,   -5, 10, 5,   -6, 10, 6,   -6, 10, 7,   -6, 12, 6,   -7,
    12, 7,  -7, 12, 8,  -7, 14,  7,  -8, 13,  8,  -8, 14,  9,  -8, 15,  8,  -9, 15,  9,
    -9, 16, 10, -9, 17, 9,  -10, 17, 10, -10, 17, 11, -10, 19, 10, -11, 19, 11, -11, 19,
};

static s8 sViewOfsLevelW[] = {
    41, 2,  0,  1,  0, -1,  1,  1,  -1,  1,  2,  -1,  1,  3,  -1,  2,  1,  -2,  2,  2,  -2,
    2,  3,  -2, 2,  4, -2,  4,  2,  -3,  4,  3,  -3,  4,  4,  -3,  4,  5,  -3,  6,  3,  -4,
    6,  4,  -4, 6,  5, -4,  6,  6,  -4,  8,  4,  -5,  8,  5,  -5,  8,  6,  -5,  8,  7,  -5,
    10, 5,  -6, 10, 6, -6,  10, 7,  -6,  10, 8,  -6,  12, 6,  -7,  12, 7,  -7,  12, 8,  -7,
    12, 9,  -7, 14, 7, -8,  13, 8,  -8,  13, 9,  -8,  14, 8,  -9,  15, 9,  -9,  15, 10, -9,
    16, 11, -9, 18, 9, -10, 17, 10, -10, 17, 11, -10, 18, 10, -11, 19, 11, -11, 19,
};

static s8 sViewOfsLevelE[] = {
    41, 1, 0,  1,  -1, -1, 1,  0, -1,  1,  1, -1,  1,  2,  -1,  2,  0,  -2,  2,  1, -2,
    2,  2, -2, 2,  3,  -2, 4,  1, -3,  4,  2, -3,  4,  3,  -3,  4,  4,  -3,  6,  2, -4,
    6,  3, -4, 6,  4,  -4, 6,  5, -4,  8,  3, -5,  8,  4,  -5,  8,  5,  -5,  8,  6, -5,
    10, 4, -6, 10, 5,  -6, 10, 6, -6,  10, 7, -6,  12, 5,  -7,  12, 6,  -7,  12, 7, -7,
    12, 8, -7, 14, 6,  -8, 13, 7, -8,  13, 8, -8,  14, 9,  -8,  15, 7,  -9,  15, 8, -9,
    15, 9, -9, 16, 10, -9, 17, 8, -10, 17, 9, -10, 17, 10, -10, 17, 10, -11, 19,
};

static s8 sViewOfsLevelS[] = {
    41, 1, 1,  1,  1, 0,  1,  2,  0,  2,  0, -1,  1,  1,  -1,  1,  2,  -1,  2,  3,  -1,
    4,  1, -2, 2,  2, -2, 2,  3,  -2, 4,  4, -2,  6,  2,  -3,  4,  3,  -3,  4,  4,  -3,
    6,  5, -3, 7,  3, -4, 6,  4,  -4, 6,  5, -4,  8,  6,  -4,  9,  4,  -5,  8,  5,  -5,
    8,  6, -5, 9,  7, -5, 11, 5,  -6, 10, 6, -6,  10, 7,  -6,  11, 8,  -6,  13, 6,  -7,
    12, 7, -7, 12, 8, -7, 13, 9,  -7, 15, 7, -8,  13, 8,  -8,  14, 9,  -8,  15, 10, -8,
    17, 8, -9, 15, 9, -9, 16, 10, -9, 17, 9, -10, 17, 10, -10, 18, 11, -10, 19,
};
static s8 sViewOfsLevelN[] = {
    39, 1,  0,  1,  1,   -1, 1,  2,   -1, 2,  0,   -2, 1,  1,   -2, 1,  2,   -2, 2,  3,
    -2, 4,  1,  -3, 2,   2,  -3, 2,   3,  -3, 4,   4,  -3, 6,   2,  -4, 4,   3,  -4, 4,
    4,  -4, 6,  5,  -4,  8,  3,  -5,  6,  4,  -5,  6,  5,  -5,  8,  6,  -5,  10, 4,  -6,
    8,  5,  -6, 8,  6,   -6, 10, 7,   -6, 12, 5,   -7, 10, 6,   -7, 10, 7,   -7, 11, 8,
    -7, 14, 7,  -8, 12,  8,  -8, 13,  9,  -8, 15,  7,  -9, 14,  8,  -9, 14,  9,  -9, 15,
    10, -9, 17, 9,  -10, 16, 10, -10, 17, 11, -10, 19, 10, -11, 18, 11, -11, 19,
};
static s8 sViewOfsAboveW[] = {
    41, 2,  0,  0,  0, -1,  1,  1,  -1,  0,  2,  -1,  0,  3,  -1,  2,  1,  -2,  2,  2,  -2,
    2,  3,  -2, 2,  4, -2,  4,  2,  -3,  4,  3,  -3,  3,  4,  -3,  4,  5,  -3,  5,  3,  -4,
    6,  4,  -4, 5,  5, -4,  6,  6,  -4,  7,  4,  -5,  8,  5,  -5,  7,  6,  -5,  8,  7,  -5,
    9,  5,  -6, 10, 6, -6,  9,  7,  -6,  9,  8,  -6,  11, 6,  -7,  12, 7,  -7,  11, 8,  -7,
    12, 9,  -7, 14, 7, -8,  13, 8,  -8,  13, 9,  -8,  14, 8,  -9,  15, 9,  -9,  15, 10, -9,
    16, 11, -9, 18, 9, -10, 17, 10, -10, 17, 11, -10, 18, 10, -11, 19, 11, -11, 19,
};
static s8 sViewOfsAboveE[] = {
    41, 1, 0,  1,  -1, -1, 0,  0, -1,  0,  1, -1,  1,  2,  -1,  2,  0,  -2,  2,  1, -2,
    2,  2, -2, 2,  3,  -2, 4,  1, -3,  3,  2, -3,  4,  3,  -3,  4,  4,  -3,  6,  2, -4,
    5,  3, -4, 6,  4,  -4, 6,  5, -4,  8,  3, -5,  7,  4,  -5,  8,  5,  -5,  8,  6, -5,
    10, 4, -6, 9,  5,  -6, 9,  6, -6,  10, 7, -6,  12, 5,  -7,  11, 6,  -7,  11, 7, -7,
    12, 8, -7, 14, 6,  -8, 13, 7, -8,  13, 8, -8,  14, 9,  -8,  15, 7,  -9,  15, 8, -9,
    15, 9, -9, 16, 10, -9, 17, 8, -10, 17, 9, -10, 17, 10, -10, 17, 10, -11, 19,
};
static s8 sViewOfsAboveS[] = {
    41, 1, 1,  0,  1, 0,  0,  2,  0,  1,  0, -1,  1,  1,  -1,  1,  2,  -1,  2,  3,  -1,
    3,  1, -2, 2,  2, -2, 2,  3,  -2, 4,  4, -2,  5,  2,  -3,  4,  3,  -3,  4,  4,  -3,
    6,  5, -3, 7,  3, -4, 6,  4,  -4, 6,  5, -4,  8,  6,  -4,  9,  4,  -5,  8,  5,  -5,
    8,  6, -5, 9,  7, -5, 11, 5,  -6, 10, 6, -6,  10, 7,  -6,  11, 8,  -6,  13, 6,  -7,
    12, 7, -7, 12, 8, -7, 13, 9,  -7, 15, 7, -8,  13, 8,  -8,  14, 9,  -8,  15, 10, -8,
    17, 8, -9, 15, 9, -9, 16, 10, -9, 17, 9, -10, 17, 10, -10, 18, 11, -10, 19,
};
static s8 sViewOfsAboveN[] = {
    39, 1,  0,  1,  1,   -1, 0,  2,   -1, 2,  0,   -2, 0,  1,   -2, 0,  2,   -2, 2,  3,
    -2, 4,  1,  -3, 2,   2,  -3, 2,   3,  -3, 4,   4,  -3, 6,   2,  -4, 4,   3,  -4, 4,
    4,  -4, 5,  5,  -4,  8,  3,  -5,  5,  4,  -5,  6,  5,  -5,  7,  6,  -5,  10, 4,  -6,
    7,  5,  -6, 8,  6,   -6, 9,  7,   -6, 12, 5,   -7, 9,  6,   -7, 10, 7,   -7, 11, 8,
    -7, 14, 7,  -8, 12,  8,  -8, 13,  9,  -8, 15,  7,  -9, 13,  8,  -9, 14,  9,  -9, 15,
    10, -9, 17, 9,  -10, 16, 10, -10, 17, 11, -10, 19, 10, -11, 18, 11, -11, 19,
};
static s8 sViewOfsBelowW[] = {
    41, 2,  0,  2,  0, -1,  1,  1,  -1,  0,  2,  -1,  2,  3,  -1,  3,  1,  -2,  2,  2,  -2,
    2,  3,  -2, 3,  4, -2,  5,  2,  -3,  4,  3,  -3,  4,  4,  -3,  5,  5,  -3,  7,  3,  -4,
    6,  4,  -4, 6,  5, -4,  7,  6,  -4,  9,  4,  -5,  8,  5,  -5,  8,  6,  -5,  9,  7,  -5,
    11, 5,  -6, 10, 6, -6,  10, 7,  -6,  11, 8,  -6,  13, 6,  -7,  12, 7,  -7,  12, 8,  -7,
    13, 9,  -7, 15, 7, -8,  13, 8,  -8,  14, 9,  -8,  15, 8,  -9,  15, 9,  -9,  16, 10, -9,
    17, 11, -9, 19, 9, -10, 17, 10, -10, 17, 11, -10, 19, 10, -11, 19, 11, -11, 19,
};
static s8 sViewOfsBelowE[] = {
    41, 1, 0,  1,  -1, -1, 2,  0, -1,  1,  1, -1,  1,  2,  -1,  2,  0,  -2,  3,  1, -2,
    2,  2, -2, 2,  3,  -2, 4,  1, -3,  5,  2, -3,  4,  3,  -3,  4,  4,  -3,  6,  2, -4,
    7,  3, -4, 6,  4,  -4, 6,  5, -4,  8,  3, -5,  9,  4,  -5,  8,  5,  -5,  8,  6, -5,
    10, 4, -6, 11, 5,  -6, 10, 6, -6,  10, 7, -6,  12, 5,  -7,  13, 6,  -7,  12, 7, -7,
    12, 8, -7, 14, 6,  -8, 14, 7, -8,  13, 8, -8,  14, 9,  -8,  15, 7,  -9,  16, 8, -9,
    15, 9, -9, 16, 10, -9, 17, 8, -10, 18, 9, -10, 17, 10, -10, 17, 10, -11, 19,
};
static s8 sViewOfsBelowS[] = {
    41, 1, 1,  2,  1, 0,  1,  2,  0,  3,  0, -1,  1,  1,  -1,  1,  2,  -1,  2,  3,  -1,
    5,  1, -2, 2,  2, -2, 2,  3,  -2, 4,  4, -2,  7,  2,  -3,  4,  3,  -3,  4,  4,  -3,
    6,  5, -3, 8,  3, -4, 6,  4,  -4, 6,  5, -4,  8,  6,  -4,  10, 4,  -5,  8,  5,  -5,
    8,  6, -5, 10, 7, -5, 12, 5,  -6, 10, 6, -6,  10, 7,  -6,  12, 8,  -6,  14, 6,  -7,
    12, 7, -7, 12, 8, -7, 14, 9,  -7, 16, 7, -8,  13, 8,  -8,  14, 9,  -8,  15, 10, -8,
    18, 8, -9, 15, 9, -9, 16, 10, -9, 17, 9, -10, 17, 10, -10, 18, 11, -10, 19,
};
static s8 sViewOfsBelowN[] = {
    39, 1,  0,  1,  1,   -1, 1,  2,   -1, 2,  0,   -2, 2,  1,   -2, 2,  2,   -2, 2,  3,
    -2, 4,  1,  -3, 3,   2,  -3, 3,   3,  -3, 4,   4,  -3, 6,   2,  -4, 5,   3,  -4, 5,
    4,  -4, 6,  5,  -4,  8,  3,  -5,  7,  4,  -5,  7,  5,  -5,  8,  6,  -5,  10, 4,  -6,
    9,  5,  -6, 9,  6,   -6, 10, 7,   -6, 12, 5,   -7, 11, 6,   -7, 11, 7,   -7, 12, 8,
    -7, 14, 7,  -8, 13,  8,  -8, 14,  9,  -8, 15,  7,  -9, 15,  8,  -9, 15,  9,  -9, 16,
    10, -9, 17, 9,  -10, 17, 10, -10, 17, 11, -10, 19, 10, -11, 19, 11, -11, 19,
};

//? Arrays for each of the four directions, for level/downward/upward viewing ranges (CHECKME)
static s8 *sViewObstructionTables[] = {sViewOfsLevelS, sViewOfsLevelW, sViewOfsLevelN, sViewOfsLevelE, sViewOfsAboveS, sViewOfsAboveW,
                           sViewOfsAboveN, sViewOfsAboveE, sViewOfsBelowS, sViewOfsBelowW, sViewOfsBelowN, sViewOfsBelowE};

/* Counts map tiles that block the camera's view of tile (z,x) at the given yaw, walking
 * an offset table selected by the elevation relation to the target and facing
 * (sViewObstructionTables; sViewObstructionOfsFlat is the alternate table when param_5 is set). Backs all four
 * GetBestViewOfTarget* pickers. */
s32 CountViewObstructions(s8 z, s8 x, s32 angle, u8 dir, s32 param_5) {
   s32 result;
   s16 angleDir;
   s8 x_t4, z_t3, y_t2;
   s8 elevation;
   s8 **psVar2;
   s8 *p;
   s32 i;
   s32 count;
   s32 zero; //?

   result = 0;
   zero = 0;
   elevation = gTerrainPtr[z][x].s.elevation;
   angleDir = (angle & 0xfff) >> 10;
   dir = (dir + angleDir) & 3;

   psVar2 = &sViewObstructionTables[0];
   if (elevation > gTerrainPtr[gTargetZ][gTargetX].s.elevation) {
      psVar2 += 4;
   }
   if (elevation < gTerrainPtr[gTargetZ][gTargetX].s.elevation) {
      psVar2 += 8;
   }
   psVar2 += dir;

   p = *psVar2;
   if (param_5) {
      p = &sViewObstructionOfsFlat[0];
   }

   count = *p++;
   for (i = 0; i < count; i++) {
      switch (angleDir) {
      case DIR_SOUTH:
         x_t4 = x + *p++;
         z_t3 = z + *p++;
         y_t2 = *p++;
         break;
      case DIR_WEST:
         z_t3 = z + *p++;
         x_t4 = x - *p++;
         y_t2 = *p++;
         break;
      case DIR_NORTH:
         x_t4 = x - *p++;
         z_t3 = z - *p++;
         y_t2 = *p++;
         break;
      case DIR_EAST:
         z_t3 = z - *p++;
         x_t4 = x + *p++;
         y_t2 = *p++;
         break;
      }

      if (x_t4 >= gMapMinX && x_t4 <= gMapMaxX && z_t3 >= gMapMinZ && z_t3 <= gMapMaxZ) {
         y_t2 = gMapRowPointers[z_t3][x_t4].height - elevation - y_t2 + zero +
                gCrateGrid_Ptr[z_t3][x_t4] * 2;
         if (y_t2 > 0) {
            result++;
         }
      }
   }

   return result;
}

/* UNREFERENCED empty stub -- kept for byte/ordering exactness. */
void Noop_800c4658(void) {}
