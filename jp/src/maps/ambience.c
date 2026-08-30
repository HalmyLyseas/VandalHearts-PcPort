/* Event-spawned map ambience: chimney smoke (Objf289/302), the Map 36 forcefield (Objf675),
 * rain (Objf676_687/677), water ripples (Objf678) and the campfire (Objf692). All are spawned
 * by event opcode 0x1d (docs/decomp/event-scripts.md), so no handler here has a C spawn site. */
#include "common.h"
#include "object.h"
#include "graphics.h"
#include "battle.h"
#include "field.h"
#include "state.h"
#include "units.h"
#include "audio.h"

#undef OBJF
#define OBJF 289
void Objf289_ChimneySmokeRing(Object *obj) {
   s16 timer;
   s16 halfSize;
   s16 prevQuadIdx;
   s16 fade;
   POLY_FT4 *poly;
   s32 rgb;

   switch (obj->state) {
   case 0:
      obj->d.sprite.gfxIdx = GFX_SWIRLY_RING;
      obj->d.sprite.boxIdx = 7;
      obj->d.sprite.clut = CLUT_GRAYS;
      obj->d.sprite.semiTrans = 2;
      obj->d.sprite.coords[0].x = obj->d.sprite.coords[1].x = obj->x1.n - CV(0.125);
      obj->d.sprite.coords[2].x = obj->d.sprite.coords[3].x = obj->x1.n + CV(0.125);
      obj->d.sprite.coords[0].z = obj->d.sprite.coords[2].z = obj->z1.n - CV(0.125);
      obj->d.sprite.coords[1].z = obj->d.sprite.coords[3].z = obj->z1.n + CV(0.125);
      obj->state++;

   // fallthrough
   case 1:
      timer = obj->state2 * 2;

      halfSize = timer + CV(0.125);
      obj->d.sprite.coords[0].x = obj->d.sprite.coords[1].x = obj->x1.n - halfSize;
      obj->d.sprite.coords[2].x = obj->d.sprite.coords[3].x = obj->x1.n + halfSize;
      obj->d.sprite.coords[0].z = obj->d.sprite.coords[2].z = obj->z1.n - halfSize;
      obj->d.sprite.coords[1].z = obj->d.sprite.coords[3].z = obj->z1.n + halfSize;

      obj->y1.n += obj->y2.n;
      obj->d.sprite.coords[0].y = obj->d.sprite.coords[1].y = obj->d.sprite.coords[2].y =
          obj->d.sprite.coords[3].y = obj->y1.n;

      prevQuadIdx = gQuadIndex;
      AddObjPrim4(gGraphicsPtr->ot, obj);

      if (prevQuadIdx != gQuadIndex) {
         poly = &gGraphicsPtr->quads[gQuadIndex - 1];
         fade = (0x80 - timer) - timer;
         rgb = (fade << 16) + (fade << 8) + fade;
         *(u32 *)&poly->r0 = ((GPU_CODE_POLY_FT4 | GPU_CODE_SEMI_TRANS) << 24) + rgb;
      }

      if (++obj->state2 >= 32) {
         obj->functionIndex = OBJF_NULL;
      }

      break;
   }
}

#undef OBJF
#define OBJF 302
void Objf302_ChimneySmoke(Object *obj) {
   Object *ring;

   switch (obj->state) {
   case 0:
      obj->y1.n = GetMapModelElevation(obj->z1.s.hi, obj->x1.s.hi) + CV(0.25);
      obj->state++;
      obj->state2 = 0;

   // fallthrough
   case 1:
      if (++obj->state2 > 27) {
         ring = CreatePositionedObj(obj, OBJF_CHIMNEY_SMOKE_RING);
         ring->y2.n = 6;
         obj->state2 = 0;
      }
      break;
   }
}

#undef OBJF
#define OBJF 675
void Objf675_LeenaForcefield(Object *obj) {
   // Spawned by: EVDATA74.DAT->271->675, SetupMapExtras()->256->675
   static u8 cluts[4] = {CLUT_REDS, CLUT_BLUES, CLUT_PURPLES, CLUT_GREENS};

   Object *targetSprite;
   s32 i;
   s32 x, z;
   s32 currentSize;
   s32 halfSize;
   s32 theta;

   switch (obj->state) {
   case 0:
      obj->state++;

   // fallthrough
   case 1:
      targetSprite = OBJ.targetSprite;
      obj->x1.n = targetSprite->x1.n;
      obj->z1.n = targetSprite->z1.n;
      obj->y1.n = GetTerrainElevation(obj->z1.s.hi, obj->x1.s.hi);
      OBJ.targetSprite = NULL;
      OBJ.gfxIdx = GFX_RING;
      OBJ.boxIdx = 7;
      OBJ.clut = CLUT_BLUES;
      OBJ.semiTrans = 4;
      obj->y2.n = obj->y1.n;
      obj->state++;

   // fallthrough
   case 2:
      theta = obj->mem;
      x = obj->x1.n;
      z = obj->z1.n;

      obj->state3 += (CV(2.0) - obj->state3) >> 4;
      currentSize = obj->state3;
      OBJ.clut = cluts[(obj->mem >> 4) & 3];

      for (i = 0; i < 4; i++) {
         halfSize = currentSize * rcos(theta) >> 12;
         OBJ.coords[0].y = OBJ.coords[1].y = OBJ.coords[2].y = OBJ.coords[3].y =
             obj->y2.n + (CV(2.0) * rsin(theta) >> 12);

         OBJ.coords[0].x = x + halfSize;
         OBJ.coords[0].z = z;
         OBJ.coords[1].x = x;
         OBJ.coords[1].z = z + halfSize;
         OBJ.coords[2].x = x;
         OBJ.coords[2].z = z - halfSize;
         OBJ.coords[3].x = x - halfSize;
         OBJ.coords[3].z = z;

         obj->y1.n = OBJ.coords[0].y;
         AddObjPrim3(gGraphicsPtr->ot, obj, 0); //? Extra arg

         theta += DEG(22.5);
         if (theta >= DEG(90)) {
            theta -= DEG(90);
         }
      }

      obj->mem += DEG(1.58203125);
      if (obj->mem >= DEG(90)) {
         obj->mem = 0;
      }

      break;
   }
}

#undef OBJF
#define OBJF 676
void Objf676_687_Rainfall(Object *obj) {
   // Spawned by: EVDATA86.DAT, EVDATA88.DAT, SetupMapExtras()
   s32 i;
   Object *raindrop;

   switch (obj->state) {
   case 0:
      if (obj->functionIndex == OBJF_LIGHT_RAINFALL) {
         obj->state2 = 1;
      }
      obj->state++;

   // fallthrough
   case 1:
      if (--obj->mem <= 0) {
         for (i = 0; i < 8; i++) {
            raindrop = Obj_GetUnused();
            raindrop->functionIndex = OBJF_RAINFALL_DROP;
            raindrop->x1.n = rand() % CV(24.0);
            raindrop->z1.n = rand() % CV(12.0);
            raindrop->y1.n = rand() % CV(1.0) + CV(6.0);
         }
         if (obj->state2 != 0) {
            // OBJF_LIGHT_RAINFALL
            obj->mem += (rand() % 4 + 2);
         } else {
            // OBJF_HEAVY_RAINFALL
            obj->mem += (rand() % 2 + 1);
         }
      }

   case 2:
      break;
   }
}

#undef OBJF
#define OBJF 677
void Objf677_RainfallDrop(Object *obj) {
   switch (obj->state) {
   case 0:
      obj->d.sprite.gfxIdx = GFX_COLORS;
      obj->d.sprite.clut = CLUT_BLUES;
      obj->d.sprite.coords[0].x = obj->d.sprite.coords[1].x = obj->d.sprite.coords[2].x =
          obj->d.sprite.coords[3].x = obj->x1.n;
      obj->d.sprite.coords[0].z = obj->d.sprite.coords[1].z = obj->d.sprite.coords[2].z =
          obj->d.sprite.coords[3].z = obj->z1.n;
      obj->d.sprite.coords[0].x = obj->x1.n + 4;
      obj->d.sprite.coords[1].x = obj->x1.n + 4;
      obj->d.sprite.coords[2].x = obj->x1.n - 4;
      obj->d.sprite.coords[3].x = obj->x1.n - 4;
      obj->d.sprite.semiTrans = 4;
      obj->d.sprite.coords[0].y = obj->d.sprite.coords[2].y =
          obj->y1.n + (rand() % CV(0.25) + CV(1.0));
      obj->d.sprite.coords[1].y = obj->d.sprite.coords[3].y = obj->y1.n;
      obj->y2.n = CV(-0.5) - rand() % CV(0.25);
      obj->y3.n = GetTerrainElevation(obj->z1.s.hi, obj->x1.s.hi);
      obj->state++;
      break;

   case 1:
      obj->d.sprite.coords[0].y += obj->y2.n;
      obj->d.sprite.coords[1].y += obj->y2.n;
      obj->d.sprite.coords[2].y += obj->y2.n;
      obj->d.sprite.coords[3].y += obj->y2.n;
      AddObjPrim3(gGraphicsPtr->ot, obj);
      obj->y1.n += obj->y2.n;
      if (obj->y1.n <= obj->y3.n) {
         obj->state++;
      }
      break;

   case 2:
      if ((obj->x1.s.hi < gMapViewOriginX) || (obj->x1.s.hi > gMapSizeX + gMapViewOriginX - 1) ||
          (obj->z1.s.hi < gMapViewOriginZ) || (obj->z1.s.hi > gMapSizeZ + gMapViewOriginZ - 1)) {
         obj->functionIndex = OBJF_NULL;
         break;
      }

      if ((OBJ_TERRAIN(obj).s.terrain == TERRAIN_WATER)) {
         CreatePositionedObj(obj, OBJF_RIPPLE);
      }

      obj->functionIndex = OBJF_NULL;
      break;
   }
}

#undef OBJF
#define OBJF 678
void Objf678_Ripple(Object *obj) {
   s16 timer;
   s16 halfSize;
   s16 prevQuadIdx;
   s16 fade;
   POLY_FT4 *poly;
   s32 rgb;

   switch (obj->state) {
   case 0:
      obj->d.sprite.gfxIdx = GFX_RING;
      obj->d.sprite.clut = CLUT_GRAYS;
      obj->d.sprite.semiTrans = 2;
      obj->d.sprite.coords[0].x = obj->d.sprite.coords[1].x = obj->x1.n - CV(0.125);
      obj->d.sprite.coords[2].x = obj->d.sprite.coords[3].x = obj->x1.n + CV(0.125);
      obj->d.sprite.coords[0].z = obj->d.sprite.coords[2].z = obj->z1.n - CV(0.125);
      obj->d.sprite.coords[1].z = obj->d.sprite.coords[3].z = obj->z1.n + CV(0.125);
      obj->d.sprite.otOfs = 8;
      obj->y1.n = GetTerrainElevation(obj->z1.s.hi, obj->x1.s.hi) + CV(0.015625);
      obj->state++;

   // fallthrough
   case 1:
      timer = obj->state2 * 2;

      halfSize = timer + CV(0.125);
      obj->d.sprite.coords[0].x = obj->d.sprite.coords[1].x = obj->x1.n - halfSize;
      obj->d.sprite.coords[2].x = obj->d.sprite.coords[3].x = obj->x1.n + halfSize;
      obj->d.sprite.coords[0].z = obj->d.sprite.coords[2].z = obj->z1.n - halfSize;
      obj->d.sprite.coords[1].z = obj->d.sprite.coords[3].z = obj->z1.n + halfSize;

      obj->y1.n += obj->y2.n;
      obj->d.sprite.coords[0].y = obj->d.sprite.coords[1].y = obj->d.sprite.coords[2].y =
          obj->d.sprite.coords[3].y = obj->y1.n;

      prevQuadIdx = gQuadIndex;
      AddObjPrim4(gGraphicsPtr->ot, obj);

      if (prevQuadIdx != gQuadIndex) {
         poly = &gGraphicsPtr->quads[gQuadIndex - 1];
         fade = (0x80 - timer) - timer;
         rgb = (fade << 16) + (fade << 8) + fade;
         *(u32 *)&poly->r0 = ((GPU_CODE_POLY_FT4 | GPU_CODE_SEMI_TRANS) << 24) + rgb;
      }

      if (++obj->state2 >= 32) {
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

static Quad sQuad_800ff8f4 = {{-24, 0, -24, 0}, {24, 0, -24, 0}, {-24, 0, 24, 0}, {24, 0, 24, 0}};

#undef OBJF
#define OBJF 692
void Objf692_Campfire(Object *obj) {
   // Spawned by: EVDATA25.DAT, EVDATA67.DAT
   static s16 animData[12] = {4, GFX_CAMPFIRE_1, 2, GFX_CAMPFIRE_2, 2, GFX_CAMPFIRE_3,
                              2, GFX_CAMPFIRE_4, 2, GFX_NULL,       1, GFX_NULL};

   Object *sprite;
   Object *entitySprite;

   switch (obj->state) {
   case 0:
      sprite = Obj_GetUnused();
      sprite->functionIndex = OBJF_NOOP;
      OBJ.sprite = sprite;
      entitySprite = OBJ.entitySprite;
      sprite->x1.n = entitySprite->x1.n;
      sprite->z1.n = entitySprite->z1.n;
      sprite->d.sprite.animData = animData;
      obj->state++;

   // fallthrough
   case 1:
      sprite = OBJ.sprite;
      UpdateObjAnimation(sprite);
      AddObjPrim6(gGraphicsPtr->ot, sprite, 1);
      break;
   }
}

