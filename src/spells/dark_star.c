/* Dark Star spell effects: Objf194 (FX3 driver) and Objf193 (the star itself).
 * Dispatched via gSpellsEx (model described in spells/casting_main.c's header). */
#include "common.h"
#include "object.h"
#include "graphics.h"
#include "battle.h"

extern void ApplyMaskEffect(s16, s16, s16, s16, s16, s16, s16, s16, s16, s16);

#undef OBJF
#define OBJF 194
void Objf194_DarkStar_FX3(Object *obj) {
   Object *fx;

   fx = Obj_GetUnused();
   fx->functionIndex = OBJF_DARK_STAR_FX2;
   fx->x1.s.hi = obj->x1.s.hi;
   fx->z1.s.hi = obj->z1.s.hi;
   fx->d.objf193.endingFxType = 1;

   obj->functionIndex = OBJF_NULL;
}

#undef OBJF
#define OBJF 193
void Objf193_DarkStar_FX2(Object *obj) {
   static u8 faces[6][4] = {
       {0, 1, 3, 5}, {0, 2, 3, 4}, {1, 2, 4, 5}, {6, 7, 9, 11}, {6, 8, 9, 10}, {7, 8, 10, 11},
   };

   Object *targetSprite;
   Object *obj_s2;
   s16 x, z, y;
   s16 a, b, c;
   s16 tmp;
   s32 i, j;
   SVectorXZY *pCoord;
   SVECTOR vertices[12];

   switch (obj->state) {
   case 0:
      targetSprite = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
      OBJ.targetSprite = targetSprite;
      obj->state++;

   // fallthrough
   case 1:
      targetSprite = OBJ.targetSprite;

      switch (obj->state2) {
      case 0:
         if (OBJ.timer < 32) {
            obj->state3 = 0;
         } else if (OBJ.timer > 163) {
            obj->state3 = 2;
         } else {
            obj->state3 = 1;
         }

         switch (obj->state3) {
         case 0:
            OBJ.triRadius += 6;
            OBJ.orbitRadius += 8;
            gLightColor.r -= 3;
            gLightColor.g -= 3;
            gLightColor.b -= 3;

         // fallthrough
         case 1:
            break;

         case 2:
            OBJ.triRadius -= 6;
            OBJ.orbitRadius -= 8;
            gLightColor.r += 3;
            gLightColor.g += 3;
            gLightColor.b += 3;
            break;
         }

         a = OBJ.orbitRadius;
         ApplyMaskEffect(452 << 2, 400, 32, 32, 416 << 2, 384, OBJ.timer * 2 % 64, 0,
                         GFX_MASK_EFFECT_1, 0);

         obj_s2 = Obj_GetUnused();
         obj_s2->functionIndex = OBJF_NOOP;
         obj_s2->d.sprite.gfxIdx = GFX_MASK_EFFECT_1;
         obj_s2->d.sprite.clut = CLUT_BLUES;
         obj_s2->d.sprite.semiTrans = 0;

         x = targetSprite->x1.n;
         z = targetSprite->z1.n;
         y = targetSprite->y1.n;

         // 1 //

         b = a * rcos(OBJ.timer * 0x20) >> 12;
         c = a * rsin(OBJ.timer * 0x20) >> 12;

         tmp = OBJ.triRadius * rcos(OBJ.timer * 0x10) / ONE + b;
         obj_s2->d.sprite.coords[0].x = obj_s2->d.sprite.coords[1].x = x + tmp;
         obj_s2->d.sprite.coords[2].x = x + OBJ.triRadius * rcos(OBJ.timer * 0x10 + 0x556) / ONE + b;
         obj_s2->d.sprite.coords[3].x = x + OBJ.triRadius * rcos(OBJ.timer * 0x10 + 0xaac) / ONE + b;

         tmp = OBJ.triRadius * rsin(OBJ.timer * 8) / ONE + 0x80;
         obj_s2->d.sprite.coords[0].y = obj_s2->d.sprite.coords[1].y = y + tmp;
         obj_s2->d.sprite.coords[2].y = y + OBJ.triRadius * rsin(OBJ.timer * 8 + 0x556) / ONE + 0x80;
         obj_s2->d.sprite.coords[3].y = y + OBJ.triRadius * rsin(OBJ.timer * 8 + 0xaac) / ONE + 0x80;

         tmp = OBJ.triRadius * rsin(rcos(OBJ.timer * 0x10)) / ONE + c;
         obj_s2->d.sprite.coords[0].z = obj_s2->d.sprite.coords[1].z = z + tmp;
         obj_s2->d.sprite.coords[2].z =
             z + OBJ.triRadius * rsin(rcos(OBJ.timer * 0x10) + 0x556) / ONE + c;
         obj_s2->d.sprite.coords[3].z =
             z + OBJ.triRadius * rsin(rcos(OBJ.timer * 0x10) + 0xaac) / ONE + c;

         AddObjPrim4(gGraphicsPtr->ot, obj_s2);

         vertices[0].vx = obj_s2->d.sprite.coords[0].x;
         vertices[0].vz = obj_s2->d.sprite.coords[0].z;
         vertices[0].vy = obj_s2->d.sprite.coords[0].y;
         vertices[1].vx = obj_s2->d.sprite.coords[2].x;
         vertices[1].vz = obj_s2->d.sprite.coords[2].z;
         vertices[1].vy = obj_s2->d.sprite.coords[2].y;
         vertices[2].vx = obj_s2->d.sprite.coords[3].x;
         vertices[2].vz = obj_s2->d.sprite.coords[3].z;
         vertices[2].vy = obj_s2->d.sprite.coords[3].y;

         // 2 //

         b = a * rcos(OBJ.timer * 0x20 + 0x800) >> 12;
         c = a * rsin(OBJ.timer * 0x20 + 0x800) >> 12;

         tmp = OBJ.triRadius * rcos(OBJ.timer * 0x10) / ONE + c;
         obj_s2->d.sprite.coords[0].z = obj_s2->d.sprite.coords[1].z = z + tmp;
         obj_s2->d.sprite.coords[2].z = z + OBJ.triRadius * rcos(OBJ.timer * 0x10 + 0x556) / ONE + c;
         obj_s2->d.sprite.coords[3].z = z + OBJ.triRadius * rcos(OBJ.timer * 0x10 + 0xaac) / ONE + c;

         tmp = OBJ.triRadius * rsin(OBJ.timer * 8) / ONE + b;
         obj_s2->d.sprite.coords[0].x = obj_s2->d.sprite.coords[1].x = x + tmp;
         obj_s2->d.sprite.coords[2].x = x + OBJ.triRadius * rsin(OBJ.timer * 8 + 0x556) / ONE + b;
         obj_s2->d.sprite.coords[3].x = x + OBJ.triRadius * rsin(OBJ.timer * 8 + 0xaac) / ONE + b;

         tmp = OBJ.triRadius * rsin(rcos(OBJ.timer * 0x10)) / ONE + 0x80;
         obj_s2->d.sprite.coords[0].y = obj_s2->d.sprite.coords[1].y = y + tmp;
         obj_s2->d.sprite.coords[2].y =
             y + OBJ.triRadius * rsin(rcos(OBJ.timer * 0x10) + 0x556) / ONE + 0x80;
         obj_s2->d.sprite.coords[3].y =
             y + OBJ.triRadius * rsin(rcos(OBJ.timer * 0x10) + 0xaac) / ONE + 0x80;

         AddObjPrim4(gGraphicsPtr->ot, obj_s2);

         vertices[3].vx = obj_s2->d.sprite.coords[0].x;
         vertices[3].vz = obj_s2->d.sprite.coords[0].z;
         vertices[3].vy = obj_s2->d.sprite.coords[0].y;
         vertices[4].vx = obj_s2->d.sprite.coords[2].x;
         vertices[4].vz = obj_s2->d.sprite.coords[2].z;
         vertices[4].vy = obj_s2->d.sprite.coords[2].y;
         vertices[5].vx = obj_s2->d.sprite.coords[3].x;
         vertices[5].vz = obj_s2->d.sprite.coords[3].z;
         vertices[5].vy = obj_s2->d.sprite.coords[3].y;

         // 3 //

         b = a * rcos(OBJ.timer * 0x20 + 0x400) >> 12;
         c = a * rsin(OBJ.timer * 0x20 + 0x400) >> 12;

         tmp = OBJ.triRadius * rcos(OBJ.timer * 0x10) / ONE + 0x80;
         obj_s2->d.sprite.coords[0].y = obj_s2->d.sprite.coords[1].y = y + tmp;
         obj_s2->d.sprite.coords[2].y =
             y + OBJ.triRadius * rcos(OBJ.timer * 0x10 + 0x556) / ONE + 0x80;
         obj_s2->d.sprite.coords[3].y =
             y + OBJ.triRadius * rcos(OBJ.timer * 0x10 + 0xaac) / ONE + 0x80;

         tmp = OBJ.triRadius * rsin(OBJ.timer * 8) / ONE + c;
         obj_s2->d.sprite.coords[0].z = obj_s2->d.sprite.coords[1].z = z + tmp;
         obj_s2->d.sprite.coords[2].z = z + OBJ.triRadius * rsin(OBJ.timer * 8 + 0x556) / ONE + c;
         obj_s2->d.sprite.coords[3].z = z + OBJ.triRadius * rsin(OBJ.timer * 8 + 0xaac) / ONE + c;

         tmp = OBJ.triRadius * rsin(rcos(OBJ.timer * 0x10)) / ONE + b;
         obj_s2->d.sprite.coords[0].x = obj_s2->d.sprite.coords[1].x = x + tmp;
         obj_s2->d.sprite.coords[2].x =
             x + OBJ.triRadius * rsin(rcos(OBJ.timer * 0x10) + 0x556) / ONE + b;
         obj_s2->d.sprite.coords[3].x =
             x + OBJ.triRadius * rsin(rcos(OBJ.timer * 0x10) + 0xaac) / ONE + b;

         AddObjPrim4(gGraphicsPtr->ot, obj_s2);

         vertices[6].vx = obj_s2->d.sprite.coords[0].x;
         vertices[6].vz = obj_s2->d.sprite.coords[0].z;
         vertices[6].vy = obj_s2->d.sprite.coords[0].y;
         vertices[7].vx = obj_s2->d.sprite.coords[2].x;
         vertices[7].vz = obj_s2->d.sprite.coords[2].z;
         vertices[7].vy = obj_s2->d.sprite.coords[2].y;
         vertices[8].vx = obj_s2->d.sprite.coords[3].x;
         vertices[8].vz = obj_s2->d.sprite.coords[3].z;
         vertices[8].vy = obj_s2->d.sprite.coords[3].y;

         // 4 //

         b = a * rcos(OBJ.timer * 0x20 + 0xc18) >> 12;
         c = a * rsin(OBJ.timer * 0x20 + 0xc18) >> 12;

         tmp = OBJ.triRadius * rcos(OBJ.timer * 0x10) / ONE + c;
         obj_s2->d.sprite.coords[0].z = obj_s2->d.sprite.coords[1].z = z + tmp;
         obj_s2->d.sprite.coords[2].z = z + OBJ.triRadius * rcos(OBJ.timer * 0x10 + 0x556) / ONE + c;
         obj_s2->d.sprite.coords[3].z = z + OBJ.triRadius * rcos(OBJ.timer * 0x10 + 0xaac) / ONE + c;

         tmp = OBJ.triRadius * rsin(OBJ.timer * 8) / ONE + 0x80;
         obj_s2->d.sprite.coords[0].y = obj_s2->d.sprite.coords[1].y = y + tmp;
         obj_s2->d.sprite.coords[2].y = y + OBJ.triRadius * rsin(OBJ.timer * 8 + 0x556) / ONE + 0x80;
         obj_s2->d.sprite.coords[3].y = y + OBJ.triRadius * rsin(OBJ.timer * 8 + 0xaac) / ONE + 0x80;

         tmp = OBJ.triRadius * rsin(rcos(OBJ.timer * 0x10)) / ONE + b;
         obj_s2->d.sprite.coords[0].x = obj_s2->d.sprite.coords[1].x = x + tmp;
         obj_s2->d.sprite.coords[2].x =
             x + OBJ.triRadius * rsin(rcos(OBJ.timer * 0x10) + 0x556) / ONE + b;
         obj_s2->d.sprite.coords[3].x =
             x + OBJ.triRadius * rsin(rcos(OBJ.timer * 0x10) + 0xaac) / ONE + b;

         AddObjPrim4(gGraphicsPtr->ot, obj_s2);

         vertices[9].vx = obj_s2->d.sprite.coords[0].x;
         vertices[9].vz = obj_s2->d.sprite.coords[0].z;
         vertices[9].vy = obj_s2->d.sprite.coords[0].y;
         vertices[10].vx = obj_s2->d.sprite.coords[2].x;
         vertices[10].vz = obj_s2->d.sprite.coords[2].z;
         vertices[10].vy = obj_s2->d.sprite.coords[2].y;
         vertices[11].vx = obj_s2->d.sprite.coords[3].x;
         vertices[11].vz = obj_s2->d.sprite.coords[3].z;
         vertices[11].vy = obj_s2->d.sprite.coords[3].y;

         obj_s2->d.sprite.gfxIdx = GFX_COLOR_13;
         obj_s2->d.sprite.clut = CLUT_BLUES;
         obj_s2->d.sprite.semiTrans = 4;

         for (i = 0; i < 6; i++) {
            pCoord = &obj_s2->d.sprite.coords[0];
            for (j = 0; j < 4; j++) {
               pCoord->x = vertices[faces[i][j]].vx;
               pCoord->z = vertices[faces[i][j]].vz;
               pCoord->y = vertices[faces[i][j]].vy;
               pCoord++;
            }
            AddObjPrim4(gGraphicsPtr->ot, obj_s2);
         }

         obj_s2->functionIndex = OBJF_NULL;
         if (OBJ.timer == 196) {
            obj->state2++;
         }

         break;
      } // switch (state2)

      OBJ.timer++;
      if (OBJ.timer == 191) {
         obj_s2 = Obj_GetUnused();
         obj_s2->functionIndex = OBJF_DAMAGE_FX2 + OBJ.endingFxType;
         obj_s2->x1.s.hi = obj->x1.s.hi;
         obj_s2->z1.s.hi = obj->z1.s.hi;
      } else if (OBJ.timer == 196) {
         obj->functionIndex = OBJF_NULL;
      }

      break;
   }
}

