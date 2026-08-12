/* Mood Ring spell effects: Objf094 (FX1 driver), MoodRing_RenderRing, and the
 * Objf095/097/096 ring/camera/target children. Dispatched via gSpellsEx (see
 * spells_casting_main.c). */
#include "common.h"
#include "object.h"
#include "graphics.h"
#include "battle.h"
#include "field.h"
#include "audio.h"

#undef OBJF
#define OBJF 094
void Objf094_MoodRing_FX1(Object *obj) {
   Object *unitSprite;
   Object *obj_s1; // ring, cam
   BVectorXZ *p;
   s32 i;

   switch (obj->state) {
   case 0:
      unitSprite = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);

      obj_s1 = Obj_GetUnused();
      obj_s1->functionIndex = OBJF_FOCUS_CAMERA;
      obj_s1->d.objf026.target = unitSprite;
      obj_s1->d.objf026.type = 0;
      obj_s1->d.objf026.zoom = 0x200;

      obj->x2.n = unitSprite->x1.n;
      obj->z2.n = unitSprite->z1.n;
      obj->y2.n = unitSprite->y1.n;

      p = (BVectorXZ *)gTargetCoords;
      for (i = 0; i < 8; i++) {
         obj_s1 = Obj_GetUnused();
         obj_s1->functionIndex = OBJF_MOOD_RING_RING;
         obj_s1->x1.n = obj->x2.n + (CV(8.0) * rcos(i * DEG(45)) >> 12);
         obj_s1->z1.n = obj->z2.n + (CV(8.0) * rsin(i * DEG(45)) >> 12);
         obj_s1->y1.n = obj->y2.n + CV(1.0);
         obj_s1->d.objf095.theta = i * DEG(45);
         obj_s1->d.objf095.radius = CV(8.0);
         obj_s1->x2.n = obj->x2.n;
         obj_s1->z2.n = obj->z2.n;
         obj_s1->y2.n = obj->y2.n;
         obj_s1->d.objf095.parent = obj;
         obj_s1->state2 = 56;

         if (p->x != 0xff) {
            unitSprite = GetUnitSpriteAtPosition(p->z, p->x);
            obj_s1->x3.n = unitSprite->x1.n;
            obj_s1->y3.n = unitSprite->y1.n + CV(0.5);
            obj_s1->z3.n = unitSprite->z1.n;
            p++;
         } else {
            obj_s1->x3.n = obj_s1->x1.n;
            obj_s1->y3.n = obj_s1->y1.n + CV(0.5);
            obj_s1->z3.n = obj_s1->z1.n;
         }
      }

      obj->state2 = 56;
      obj->state++;
      break;

   case 1:
      if (--obj->state2 < 0) {
         obj->state2 = 3;
         obj->state++;
      }
      break;

   case 2:
      OBJ.launchingOutward = 1;
      if (--obj->state2 < 0) {
         obj->state++;
      }
      break;

   case 3:
      obj->functionIndex = OBJF_NULL;
      gSignal3 = 1;
      break;
   }
}

void MoodRing_RenderRing(Object *obj, s16 halfSize, s16 theta, s16 clut) {
   SVECTOR vector;
   Object *top;
   Object *bottom;
   s32 a;

   if ((gCameraRotation.vy & 0xfff) < DEG(90) || (gCameraRotation.vy & 0xfff) > DEG(225)) {
      halfSize = -halfSize;
   }

   a = abs(halfSize) * 362 >> 8;
   SphericalToVector(&vector, a, theta, DEG(45));

   bottom = Obj_GetUnused();
   bottom->functionIndex = OBJF_NOOP;
   bottom->d.sprite.boxIdx = 3;
   bottom->d.sprite.gfxIdx = GFX_RING_BTM;
   bottom->d.sprite.clut = clut;

   top = Obj_GetUnused();
   top->functionIndex = OBJF_NOOP;
   top->d.sprite.boxIdx = 3;
   top->d.sprite.gfxIdx = GFX_RING_TOP;
   top->d.sprite.clut = clut;

   bottom->d.sprite.coords[0].x = top->d.sprite.coords[2].x = obj->x1.n - halfSize;
   bottom->d.sprite.coords[0].z = top->d.sprite.coords[2].z = obj->z1.n + halfSize;
   bottom->d.sprite.coords[0].y = top->d.sprite.coords[2].y = obj->y1.n;
   bottom->d.sprite.coords[1].x = top->d.sprite.coords[3].x = obj->x1.n + halfSize;
   bottom->d.sprite.coords[1].z = top->d.sprite.coords[3].z = obj->z1.n - halfSize;
   bottom->d.sprite.coords[1].y = top->d.sprite.coords[3].y = obj->y1.n;
   bottom->d.sprite.coords[2].x = bottom->d.sprite.coords[0].x + vector.vx;
   bottom->d.sprite.coords[2].z = bottom->d.sprite.coords[0].z + vector.vz;
   bottom->d.sprite.coords[2].y = bottom->d.sprite.coords[0].y + vector.vy;
   bottom->d.sprite.coords[3].x = bottom->d.sprite.coords[1].x + vector.vx;
   bottom->d.sprite.coords[3].z = bottom->d.sprite.coords[1].z + vector.vz;
   bottom->d.sprite.coords[3].y = bottom->d.sprite.coords[1].y + vector.vy;
   bottom->x1.n = bottom->d.sprite.coords[3].x;
   bottom->z1.n = bottom->d.sprite.coords[3].z;
   bottom->y1.n = bottom->d.sprite.coords[3].y;
   AddObjPrim3(gGraphicsPtr->ot, bottom, 0); //? Extra arg

   top->d.sprite.coords[0].x = top->d.sprite.coords[2].x - vector.vx;
   top->d.sprite.coords[0].z = top->d.sprite.coords[2].z - vector.vz;
   top->d.sprite.coords[0].y = top->d.sprite.coords[2].y - vector.vy;
   top->d.sprite.coords[1].x = top->d.sprite.coords[3].x - vector.vx;
   top->d.sprite.coords[1].z = top->d.sprite.coords[3].z - vector.vz;
   top->d.sprite.coords[1].y = top->d.sprite.coords[3].y - vector.vy;
   top->x1.n = top->d.sprite.coords[0].x;
   top->z1.n = top->d.sprite.coords[0].z;
   top->y1.n = top->d.sprite.coords[0].y;
   AddObjPrim3(gGraphicsPtr->ot, top, 0); //? Extra arg

   top->functionIndex = OBJF_NULL;
   bottom->functionIndex = OBJF_NULL;
}

#undef OBJF
#define OBJF 095
void Objf095_MoodRing_Ring(Object *obj) {
   Object *parent;
   s16 a;

   switch (obj->state) {
   case 0:
      OBJ.gfxIdx_unused = GFX_RING; // Just guessing
      OBJ.boxIdx_unused = 3;        // for these fields
      OBJ.unused_0x44 = 0x40;
      obj->state2 = 0x200;
      obj->state3 = 0;
      obj->state++;

   // fallthrough
   case 1:
      OBJ.theta += 0x40;
      OBJ.radius -= (OBJ.radius >> 4);
      a = obj->x2.n + (rcos(OBJ.theta) * OBJ.radius >> 12);
      obj->x1.n = a;
      a = obj->z2.n + (rsin(OBJ.theta) * OBJ.radius >> 12);
      obj->z1.n = a;
      MoodRing_RenderRing(obj, obj->state2, 0, 3 + obj->state3 % 3);
      obj->state2 -= (obj->state2 - 0x20) >> 4;
      obj->state3++;
      parent = OBJ.parent;
      if (parent->d.objf094.launchingOutward == 1) {
         obj->state3 = 0x10;
         obj->state++;
      }
      break;

   case 2:
      obj->state2 = 0x80;
      obj->x1.n -= (obj->x1.n - obj->x3.n) >> 2;
      obj->z1.n -= (obj->z1.n - obj->z3.n) >> 2;
      obj->y1.n -= (obj->y1.n - obj->y3.n) >> 2;
      MoodRing_RenderRing(obj, obj->state2, 0, 3 + obj->state3 % 3);
      if (--obj->state3 == 0) {
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 097
void Objf097_MoodRing_FX3(Object *obj) {
   OBJ.fatal = 1;
   obj->functionIndex = OBJF_MOOD_RING_FX2;
}

#undef OBJF
#define OBJF 096
void Objf096_MoodRing_FX2(Object *obj) {
   Object *targetSprite;

   switch (obj->state) {
   case 0:
      targetSprite = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
      OBJ.targetSprite = targetSprite;
      SnapToUnit(obj);
      obj->y2.n = obj->y1.n;
      obj->state2 = 0x80;
      obj->state3 = 0;
      obj->state++;

   // fallthrough
   case 1:
      targetSprite = OBJ.targetSprite;
      obj->y1.n = targetSprite->y1.n + CV(0.75);
      MoodRing_RenderRing(obj, obj->state2 + 0x60, 0, 3 + obj->state3 % 3);
      obj->y1.n = targetSprite->y1.n + CV(0.5);
      MoodRing_RenderRing(obj, obj->state2 + 0x60, 0, 3 + obj->state3 % 3);
      obj->y1.n = targetSprite->y1.n + CV(0.25);
      MoodRing_RenderRing(obj, obj->state2 + 0x60, 0, 3 + obj->state3 % 3);
      obj->state3++;
      obj->state2 -= 0x10;
      if (obj->state2 <= 0) {
         if (OBJ.fatal) {
            CreatePositionedObj(obj, OBJF_SLAY_FX3);
         } else {
            CreatePositionedObj(obj, OBJF_DAMAGE_FX2);
         }
         obj->state++;
      }
      break;

   case 2:
      obj->state2++;
      if (obj->state2 >= 0x20) {
         obj->state++;
      }
      break;

   case 3:
      obj->state++;

   // fallthrough
   case 4:
   case 5:
      obj->state++;
      break;

   case 6:
      obj->functionIndex = OBJF_NULL;
      break;
   }
}

