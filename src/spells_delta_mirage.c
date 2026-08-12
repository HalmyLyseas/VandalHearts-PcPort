/* Delta Mirage spell effects (Objf156/157) plus DrawDeltaMirageTriangle, which renders
 * the spell's rotating three-edged figure with its mask effect. Dispatched data-driven
 * via gSpellsEx (see spells_casting_main.c's header for the model). */
#include "common.h"
#include "object.h"
#include "graphics.h"
#include "battle.h"
#include "field.h"

extern void ApplyMaskEffect(s16, s16, s16, s16, s16, s16, s16, s16, s16, s16);

void DrawDeltaMirageTriangle(Object *deltaMirage) {
   s32 i;
   s16 *p;
   s16 current;
   s16 a, b, c, d, e;
   s16 x_1, z_1, x_2, z_2, x_3, z_3;
   POLY_FT4 *poly;
   Object *sprite;

   if (deltaMirage->state == 3) {
      a = 0x100 + (0x400 * rsin(DEG(90) - deltaMirage->d.objf156.collapsePhase * 2) >> 12);
      b = 0;
      c = 0x200 * rcos(DEG(90) - deltaMirage->d.objf156.collapsePhase * 2) >> 12;
   } else {
      a = 0x100;
      b = 0;
      c = 0x200;
   }

   ApplyMaskEffect(452 << 2, 400, 32, 32, 432 << 2, 256, deltaMirage->d.objf156.timer % 64, 0,
                   GFX_MASK_EFFECT_1, 0);

   sprite = Obj_GetUnused();
   sprite->functionIndex = OBJF_NOOP;
   sprite->d.sprite.gfxIdx = GFX_MASK_EFFECT_1;
   sprite->d.sprite.clut = CLUT_BLUES;
   sprite->d.sprite.semiTrans = 1;

   p = &deltaMirage->d.objf156.edgeAngle0;
   for (i = 0; i < 3 - deltaMirage->state2; i++, p++) {
      current = *p;

      switch (i) {
      case 2:
         d = deltaMirage->d.objf156.theta + 0xaaa;
         e = deltaMirage->d.objf156.theta;
         break;
      case 1:
         d = deltaMirage->d.objf156.theta + 0x555;
         e = deltaMirage->d.objf156.theta + 0xaaa;
         break;
      case 0:
      default:
         d = deltaMirage->d.objf156.theta;
         e = deltaMirage->d.objf156.theta + 0x555;
         break;
      }

      x_1 = c * rcos(d) >> 12;
      z_1 = c * rsin(d) >> 12;
      x_2 = (c * rcos(e) >> 12) - x_1;
      z_2 = (c * rsin(e) >> 12) - z_1;
      x_3 = x_2 * rsin(current) >> 12;
      z_3 = z_2 * rsin(current) >> 12;

      sprite->d.sprite.coords[0].x = deltaMirage->x1.n + x_1;
      sprite->d.sprite.coords[1].x = sprite->d.sprite.coords[0].x + x_3;
      sprite->d.sprite.coords[2].x = sprite->d.sprite.coords[0].x;
      sprite->d.sprite.coords[3].x = sprite->d.sprite.coords[1].x;

      sprite->d.sprite.coords[0].z = deltaMirage->z1.n + z_1;
      sprite->d.sprite.coords[1].z = sprite->d.sprite.coords[0].z + z_3;
      sprite->d.sprite.coords[2].z = sprite->d.sprite.coords[0].z;
      sprite->d.sprite.coords[3].z = sprite->d.sprite.coords[1].z;

      sprite->d.sprite.coords[0].y = deltaMirage->y1.n + a;
      sprite->d.sprite.coords[2].y = deltaMirage->y1.n + b;
      sprite->d.sprite.coords[1].y = sprite->d.sprite.coords[0].y;
      sprite->d.sprite.coords[3].y = sprite->d.sprite.coords[2].y;

      sprite->x1.n = (sprite->d.sprite.coords[0].x + sprite->d.sprite.coords[1].x) >> 1;
      sprite->z1.n = (sprite->d.sprite.coords[0].z + sprite->d.sprite.coords[1].z) >> 1;
      sprite->y1.n = (sprite->d.sprite.coords[0].y + sprite->d.sprite.coords[2].y) >> 1;

      AddObjPrim3(gGraphicsPtr->ot, sprite);
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      setRGB0(poly, 0xff, 0xff, 0xff);
   }

   sprite->functionIndex = OBJF_NULL;
}

#undef OBJF
#define OBJF 156
void Objf156_DeltaMirage_FX1(Object *obj) {
   s32 i;
   Object *obj_v1;
   Object *unitSprite;

   switch (obj->state) {
   case 0:
      unitSprite = GetUnitSpriteAtPosition(gTargetZ, gTargetX);
      obj->y1.n = GetTerrainElevation(gTargetZ, gTargetX);
      obj->x1.n = unitSprite->x1.n;
      obj->z1.n = unitSprite->z1.n;
      OBJ.theta = gCameraRotation.vy - 0x656;

      obj_v1 = Obj_GetUnused();
      obj_v1->functionIndex = OBJF_FOCUS_CAMERA;
      obj_v1->d.objf026.target = unitSprite;

      obj->state++;
      obj->state2 = 2;

   // fallthrough
   case 1:
      DrawDeltaMirageTriangle(obj);

      switch (obj->state2) {
      case 0:
         OBJ.edgeAngle2 += 0x20;
         if (OBJ.edgeAngle2 > DEG(90)) {
            OBJ.edgeAngle2 = DEG(90);
            obj->state++;
         }
         break;
      case 1:
         OBJ.edgeAngle1 += 0x20;
         if (OBJ.edgeAngle1 > DEG(90)) {
            OBJ.edgeAngle1 = DEG(90);
            obj->state2--;
         }
         break;
      case 2:
         OBJ.edgeAngle0 += 0x20;
         if (OBJ.edgeAngle0 > DEG(90)) {
            OBJ.edgeAngle0 = DEG(90);
            obj->state2--;
         }
         break;
      }

      OBJ.timer++;
      if (gLightColor.r > 0x20) {
         gLightColor.r -= 4;
         gLightColor.g -= 4;
         gLightColor.b -= 4;
      }

      break;

   case 2:
      for (i = 0; i < 0x40; i++) {
         obj_v1 = Obj_GetUnused();
         obj_v1->functionIndex = OBJF_DELTA_MIRAGE_RAY;
         obj_v1->d.objf157.parent = obj;
      }
      OBJ.collapsePhase = 0x200;
      obj->state++;

   // fallthrough
   case 3:
      DrawDeltaMirageTriangle(obj);
      OBJ.theta += 0x10;
      OBJ.timer++;
      OBJ.collapsePhase -= 4;
      if (OBJ.collapsePhase == 0x28) {
         gSignal3 = 1;
      }
      if (OBJ.collapsePhase < 0x20) {
         gLightColor.r += 12;
         gLightColor.g += 12;
         gLightColor.b += 12;
      }
      if (OBJ.collapsePhase < 0) {
         obj->functionIndex = OBJF_NULL;
         gLightColor.r = 0x80;
         gLightColor.g = 0x80;
         gLightColor.b = 0x80;
      }
      break;
   }
}

#undef OBJF
#define OBJF 157
void Objf157_DeltaMirage_Ray(Object *obj) {
   Object *obj_s0;

   switch (obj->state) {
   case 0:
      obj_s0 = OBJ.parent;
      obj->x1.n = obj_s0->x1.n;
      obj->z1.n = obj_s0->z1.n;
      obj->y1.n = obj_s0->y1.n;

      OBJ.maxLength = 0x400 + (rand() % 0x400);
      OBJ.thetaX = rand() % 0x1000;
      OBJ.thetaZ = rand() % 0x1000;
      OBJ.thetaY = rand() % 0x1000;
      OBJ.dThetaX = 0x60 - (rand() % 0xc1);
      OBJ.dThetaZ = 0x60 - (rand() % 0xc1);
      OBJ.dThetaY = 0x60 - (rand() % 0xc1);

      obj->state++;

   // fallthrough
   case 1:
      obj_s0 = Obj_GetUnused();
      obj_s0->functionIndex = OBJF_NOOP;
      obj_s0->d.sprite.gfxIdx = GFX_COLOR_14;
      obj_s0->d.sprite.clut = CLUT_BLUES;
      obj_s0->d.sprite.semiTrans = 1;

      OBJ.length = OBJ.maxLength * OBJ.lengthScale / 0x30;
      obj_s0->d.sprite.coords[0].x = obj->x1.n;
      obj_s0->d.sprite.coords[0].z = obj->z1.n;
      obj_s0->d.sprite.coords[0].y = obj->y1.n;
      obj_s0->d.sprite.coords[1].x = obj->x1.n;
      obj_s0->d.sprite.coords[1].z = obj->z1.n;
      obj_s0->d.sprite.coords[1].y = obj->y1.n;
      obj_s0->d.sprite.coords[2].x = obj->x1.n + OBJ.length * rcos(OBJ.thetaX) / ONE;
      obj_s0->d.sprite.coords[2].z = obj->z1.n + OBJ.length * rsin(OBJ.thetaZ) / ONE;
      obj_s0->d.sprite.coords[2].y = obj->y1.n + OBJ.length * rsin(OBJ.thetaY) / ONE;
      obj_s0->d.sprite.coords[3].x = obj->x1.n + OBJ.length * rcos(OBJ.thetaX + 0x10) / ONE;
      obj_s0->d.sprite.coords[3].z = obj->z1.n + OBJ.length * rsin(OBJ.thetaZ + 0x10) / ONE;
      obj_s0->d.sprite.coords[3].y = obj->y1.n + OBJ.length * rsin(OBJ.thetaY + 0x10) / ONE;

      AddObjPrim4(gGraphicsPtr->ot, obj_s0);

      OBJ.thetaX += OBJ.dThetaX;
      OBJ.thetaZ += OBJ.dThetaZ;
      OBJ.thetaY += OBJ.dThetaY;

      obj_s0->functionIndex = OBJF_NULL;

      switch (obj->state2) {
      case 0:
         OBJ.timer++;
         OBJ.lengthScale++;
         if (OBJ.lengthScale == 0x31) {
            obj->state2++;
            OBJ.lengthScale = 0x30;
         }
         break;
      case 1:
         OBJ.timer++;
         if (OBJ.timer == 0x81) {
            obj->state2++;
         }
         break;
      case 2:
         OBJ.timer++;
         OBJ.lengthScale -= 3;
         if (OBJ.lengthScale == 0) {
            obj->functionIndex = OBJF_NULL;
         }
         break;
      }

      break;
   }
}

