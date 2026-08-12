/* Stone Shower spell effects (Objf163/165/166/164: driver, falling rocks, impacts).
 * Dispatched data-driven via gSpellsEx (see spells_casting_main.c's header for the
 * model). */
#include "common.h"
#include "object.h"
#include "graphics.h"
#include "battle.h"
#include "field.h"

#undef OBJF
#define OBJF 163
void Objf163_StoneShower_FX1(Object *obj) {
   Object *obj_s0;
   Object *sprite;
   POLY_FT4 *poly;
   Object **pDataStoreAsObjs;
   s32 i;
   s16 radius;

   switch (obj->state) {
   case 0:
      obj_s0 = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
      obj->y1.n = GetTerrainElevation(obj->z1.s.hi, obj->x1.s.hi);
      obj->x1.n = obj_s0->x1.n;
      obj->z1.n = obj_s0->z1.n;

      obj_s0 = Obj_GetUnused();
      obj_s0->functionIndex = OBJF_NOOP;
      OBJ.dataStore = obj_s0;

      obj->state++;

   // fallthrough
   case 1:
      sprite = Obj_GetUnused();
      sprite->functionIndex = OBJF_NOOP;
      sprite->d.sprite.semiTrans = 1;
      sprite->d.sprite.gfxIdx = GFX_COLOR_15;

      radius = OBJ.radius;
      for (i = 0; i < 32; i++) {
         sprite->d.sprite.coords[0].x = obj->x1.n + (radius * rsin(i * 0x80) >> 12);
         sprite->d.sprite.coords[0].z = obj->z1.n + (radius * rcos(i * 0x80) >> 12);
         sprite->d.sprite.coords[0].y = obj->y1.n + CV(10.0);
         sprite->d.sprite.coords[1].x = obj->x1.n + (radius * rsin(i * 0x80 + 0x80) >> 12);
         sprite->d.sprite.coords[1].z = obj->z1.n + (radius * rcos(i * 0x80 + 0x80) >> 12);
         sprite->d.sprite.coords[1].y = sprite->d.sprite.coords[0].y;
         sprite->d.sprite.coords[2].x = sprite->d.sprite.coords[0].x;
         sprite->d.sprite.coords[2].z = sprite->d.sprite.coords[0].z;
         sprite->d.sprite.coords[2].y = obj->y1.n;
         sprite->d.sprite.coords[3].x = sprite->d.sprite.coords[1].x;
         sprite->d.sprite.coords[3].z = sprite->d.sprite.coords[1].z;
         sprite->d.sprite.coords[3].y = sprite->d.sprite.coords[2].y;

         sprite->x1.n = (sprite->d.sprite.coords[0].x + sprite->d.sprite.coords[1].x +
                         sprite->d.sprite.coords[2].x + sprite->d.sprite.coords[3].x) >>
                        2;
         sprite->z1.n = (sprite->d.sprite.coords[0].z + sprite->d.sprite.coords[1].z +
                         sprite->d.sprite.coords[2].z + sprite->d.sprite.coords[3].z) >>
                        2;
         sprite->y1.n = obj->y1.n + CV(0.875);

         AddObjPrim3(gGraphicsPtr->ot, sprite);
         poly = &gGraphicsPtr->quads[gQuadIndex - 1];
         setRGB0(poly, radius + 0x3f, radius + 0x3f, radius + 0x3f);
      }

      sprite->functionIndex = OBJF_NULL;

      switch (obj->state2) {
      case 0:
         gCameraZoom.vz += 2;
         gCameraRotation.vx += 4;
         gCameraRotation.vy += 16;

         OBJ.radius += 8;
         if (OBJ.radius >= 0xc0) {
            sprite->functionIndex = OBJF_NULL;
            obj->state2++;
            OBJ.radius = 0xc0;
         }
         break;

      case 1:
         gCameraZoom.vz += 2;
         gCameraRotation.vx += 4;
         gCameraRotation.vy += 16;

         obj_s0 = OBJ.dataStore;
         pDataStoreAsObjs = obj_s0->d.dataStore.objs;
         for (i = 0; i < 15; i++) {
            obj_s0 = Obj_GetUnused();
            obj_s0->functionIndex = OBJF_STONE_SHOWER_ROCK;
            obj_s0->d.objf164.parent = obj;
            obj_s0->d.objf164.downward = 0;
            obj_s0->d.objf164.delay = rand() % 5;
            pDataStoreAsObjs[i] = obj_s0;
         }
         obj->state2++;
         break;

      case 2:
         gCameraZoom.vz += 2;
         gCameraRotation.vx += 2;
         gCameraRotation.vy += 16;

         OBJ.timer++;
         if (OBJ.timer >= 90) {
            sprite->functionIndex = OBJF_NULL;
            obj->state2++;
         }
         break;

      case 3:
         gCameraZoom.vz += 2;
         gCameraRotation.vx += 4;
         gCameraRotation.vy += 16;

         OBJ.radius -= 8;
         if (OBJ.radius < 0) {
            obj_s0 = OBJ.dataStore;
            obj_s0->functionIndex = OBJF_NULL;
            pDataStoreAsObjs = obj_s0->d.dataStore.objs;
            for (i = 0; i < 15; i++) {
               obj_s0 = pDataStoreAsObjs[i];
               obj_s0->state = 99;
            }
            obj->functionIndex = OBJF_NULL;
            gSignal3 = 1;
         }
         break;
      }

      break;
   }
}

#undef OBJF
#define OBJF 165
void Objf165_StoneShower_FX2(Object *obj) {
   s32 i;
   Object *obj_s0;
   Object *endingFx;
   Object **pDataStoreAsObjs;
   s16 waiting;

   switch (obj->state) {
   case 0:
      obj_s0 = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
      obj->y1.n = GetTerrainElevation(obj->z1.s.hi, obj->x1.s.hi);
      obj->x1.n = obj_s0->x1.n;
      obj->z1.n = obj_s0->z1.n;

      obj_s0 = Obj_GetUnused();
      obj_s0->functionIndex = OBJF_NOOP;
      OBJ.dataStore = obj_s0;

      obj->state++;

   // fallthrough
   case 1:
      obj_s0 = OBJ.dataStore;
      pDataStoreAsObjs = obj_s0->d.dataStore.objs;
      for (i = 0; i < 15; i++) {
         obj_s0 = Obj_GetUnused();
         obj_s0->functionIndex = OBJF_STONE_SHOWER_ROCK;
         obj_s0->d.objf164.parent = obj;
         obj_s0->d.objf164.downward = 1;
         obj_s0->d.objf164.delay = 0;
         pDataStoreAsObjs[i] = obj_s0;
      }
      obj->state++;

   // fallthrough
   case 2:
      if (OBJ.timer == 5) {
         endingFx = Obj_GetUnused();
         endingFx->functionIndex = OBJF_ENGULF_EXPLOSION_DAMAGE + OBJ.endingFxType;
         endingFx->x1.s.hi = obj->x1.s.hi;
         endingFx->z1.s.hi = obj->z1.s.hi;
      }
      if (OBJ.endingFxType == 0) {
         waiting = (OBJ.timer < 60);
      } else {
         waiting = (OBJ.timer < 98);
      }
      if (!waiting) {
         obj->state++;
      } else {
         OBJ.timer++;
      }
      break;

   case 3:
      obj_s0 = OBJ.dataStore;
      obj_s0->functionIndex = OBJF_NULL;
      pDataStoreAsObjs = obj_s0->d.dataStore.objs;
      for (i = 0; i < 15; i++) {
         obj_s0 = pDataStoreAsObjs[i];
         obj_s0->state = 99;
      }
      gSignal3 = 1;
      obj->functionIndex = OBJF_NULL;
      break;
   }
}

void Objf166_StoneShower_FX3(Object *obj) {
   Object *obj_s0;

   obj_s0 = Obj_GetUnused();
   obj_s0->functionIndex = OBJF_STONE_SHOWER_FX2;
   obj_s0->x1.s.hi = obj->x1.s.hi;
   obj_s0->z1.s.hi = obj->z1.s.hi;
   obj_s0->d.objf165.endingFxType = 2;

   obj->functionIndex = OBJF_NULL;
}

#undef OBJF
#define OBJF 164
void Objf164_StoneShower_Rock(Object *obj) {
   static s16 rockAnimData[12] = {5, GFX_ROCK_1, 2, GFX_ROCK_2, 2, GFX_ROCK_3,
                                  2, GFX_ROCK_4, 2, GFX_NULL,   1, GFX_NULL};

   Object *obj_s1;
   s16 a;

   switch (obj->state) {
   case 0:
      obj_s1 = OBJ.parent;
      obj->x1.n = obj_s1->x1.n;
      obj->z1.n = obj_s1->z1.n;
      obj->y1.n = obj_s1->y1.n;

      obj_s1 = Obj_GetUnused();
      obj_s1->functionIndex = OBJF_NOOP;
      obj_s1->d.sprite.animData = rockAnimData;
      OBJ.sprite = obj_s1;

      obj->state2 = 0;
      obj->state++;

   // fallthrough
   case 1:
      if (!OBJ.downward) {
         OBJ.startY = 0;
         OBJ.endY = 0x800;
         OBJ.ySpeed = rand() % 0x50 + 0x32;
      } else {
         OBJ.startY = 0x400;
         OBJ.endY = 0;
         OBJ.ySpeed = -0x50 - (rand() % 0x78);
      }

      OBJ.theta = rand() % 0x1000;
      OBJ.radius = rand() % 0xa0 + 0x20;
      OBJ.yOfs = OBJ.startY;
      obj_s1 = OBJ.sprite;
      obj_s1->d.sprite.animInitialized = 0;
      obj->state++;

   // fallthrough
   case 2:
      if (obj->state2++ == OBJ.delay) {
         obj->state++;
         obj->state2 = 0;
      }
      break;

   case 3:
      obj_s1 = OBJ.sprite;
      a = OBJ.theta;
      obj_s1->x1.n = obj->x1.n + OBJ.radius * rcos(a) / ONE;
      obj_s1->z1.n = obj->z1.n + OBJ.radius * rsin(a) / ONE;
      obj_s1->y1.n = obj->y1.n + OBJ.yOfs;
      UpdateObjAnimation(obj_s1);
      AddObjPrim6(gGraphicsPtr->ot, obj_s1, 0);

      OBJ.yOfs += OBJ.ySpeed;
      if (!OBJ.downward) {
         if (OBJ.yOfs >= OBJ.endY) {
            obj->state = 1;
         }
      } else {
         if (OBJ.yOfs <= OBJ.endY) {
            obj->state = 1;
            obj_s1 = Obj_GetUnused();
            obj_s1->functionIndex = OBJF_BOUNCE_ZOOM;
            obj_s1->d.objf024.soft = 1;
         }
      }
      break;

   case 99:
      obj_s1 = OBJ.sprite;
      obj_s1->functionIndex = OBJF_NULL;
      obj->functionIndex = OBJF_NULL;
      break;
   }
}
