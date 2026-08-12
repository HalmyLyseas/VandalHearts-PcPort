/* Spell casting effects, part 2 (segment 0x5cbc4): Fire Gem, Delta Mirage, Explosion and
 * Stone Shower drivers with their sub-objects and drawing helpers. Dispatched data-driven
 * via gSpellsEx (see fx_060c38.c's header for the model). DrawLightningBoltSegment renders
 * one jagged segment of Objf147_LightningBolt's sky bolt; DrawDeltaMirageTriangle renders
 * the spell's rotating three-edged figure with its mask effect. */
#include "common.h"
#include "object.h"
#include "graphics.h"
#include "battle.h"
#include "field.h"

extern void ApplyMaskEffect(s16, s16, s16, s16, s16, s16, s16, s16, s16, s16);

#undef OBJF
#define OBJF 173
void Objf173_FireGem_Beam(Object *obj) {
   s16 x, y, z;
   s16 a;
   s32 i;
   Object *obj_s2;
   Object *obj_v1;
   SVECTOR positions[6];
   s16 local_38[6][2] = {{0, 1}, {3, 4}, {0, 2}, {3, 5}, {2, 1}, {5, 4}};

   x = obj->x1.n;
   y = obj->y1.n;
   z = obj->z1.n;

   switch (obj->state) {
   case 0:
      OBJ.todo_x2a = 0x800;
      obj->state++;

   // fallthrough
   case 1:
      obj_s2 = Obj_GetUnused();
      obj_s2->functionIndex = OBJF_NOOP;
      obj_s2->d.sprite.gfxIdx = GFX_COLOR_1 + OBJ.todo_x26;
      obj_s2->d.sprite.semiTrans = 1;

      a = rsin(OBJ.todo_x24);
      positions[0].vx = CV(1.25) * rcos(a) >> 12;
      positions[0].vz = CV(1.25) * rsin(a) >> 12;
      positions[1].vx = CV(1.25) * rcos(a + 0x555) >> 12;
      positions[1].vz = CV(1.25) * rsin(a + 0x555) >> 12;
      positions[2].vx = CV(1.25) * rcos(a + 0xaaa) >> 12;
      positions[2].vz = CV(1.25) * rsin(a + 0xaaa) >> 12;
      positions[3].vx = CV(1.25) * rcos(a + 0x2ab) >> 12;
      positions[3].vz = CV(1.25) * rsin(a + 0x2ab) >> 12;
      positions[4].vx = CV(1.25) * rcos(a + 0x7ff) >> 12;
      positions[4].vz = CV(1.25) * rsin(a + 0x7ff) >> 12;
      positions[5].vx = CV(1.25) * rcos(a + 0xd55) >> 12;
      positions[5].vz = CV(1.25) * rsin(a + 0xd55) >> 12;
      OBJ.todo_x24 += 0x20;

      obj_s2->d.sprite.coords[0].x = obj_s2->d.sprite.coords[1].x = x + positions[0].vx;
      obj_s2->d.sprite.coords[0].z = obj_s2->d.sprite.coords[1].z = z + positions[0].vz;
      // y += OBJ.todo_x2a;
      obj_s2->d.sprite.coords[0].y = obj_s2->d.sprite.coords[1].y = obj_s2->d.sprite.coords[2].y =
          obj_s2->d.sprite.coords[3].y = (y + OBJ.todo_x2a);
      obj_s2->d.sprite.coords[2].x = x + positions[1].vx;
      obj_s2->d.sprite.coords[2].z = z + positions[1].vz;
      obj_s2->d.sprite.coords[3].x = x + positions[2].vx;
      obj_s2->d.sprite.coords[3].z = z + positions[2].vz;
      AddObjPrim4(gGraphicsPtr->ot, obj_s2);

      obj_s2->d.sprite.coords[0].x = obj_s2->d.sprite.coords[1].x = x + positions[3].vx;
      obj_s2->d.sprite.coords[0].z = obj_s2->d.sprite.coords[1].z = z + positions[3].vz;
      obj_s2->d.sprite.coords[2].x = x + positions[4].vx;
      obj_s2->d.sprite.coords[2].z = z + positions[4].vz;
      obj_s2->d.sprite.coords[3].x = x + positions[5].vx;
      obj_s2->d.sprite.coords[3].z = z + positions[5].vz;
      AddObjPrim4(gGraphicsPtr->ot, obj_s2);

      if ((OBJ.todo_x2c == 0) && (++OBJ.todo_x2e % 3 == 0)) {
         OBJ.todo_x26++;
         if (OBJ.todo_x26 == 7) {
            OBJ.todo_x2c++;
            OBJ.todo_x26 = 6;
         }
      } else if ((OBJ.todo_x2e % 3 == 0) && (--OBJ.todo_x26 < 0)) {
         OBJ.todo_x2c = 0;
         OBJ.todo_x26 = 0;
      }

      OBJ.todo_x2a -= 0x40;
      if (OBJ.todo_x2a < 0) {
         OBJ.todo_x2a = 0;
         obj_v1 = Obj_GetUnused();
         obj_v1->functionIndex = OBJF_FLASHING_UNIT_SPRITE;
         obj_v1->x1.n = obj->x1.n;
         obj_v1->z1.n = obj->z1.n;
         OBJ.fx = obj_v1;
         obj->state++;
      }

      obj_s2->functionIndex = OBJF_NULL;
      break;

   case 2:
      obj_s2 = Obj_GetUnused();
      obj_s2->functionIndex = OBJF_NOOP;
      obj_s2->d.sprite.gfxIdx = GFX_COLOR_1 + OBJ.todo_x26;
      obj_s2->d.sprite.semiTrans = 1;

      a = rsin(OBJ.todo_x24);
      positions[0].vx = CV(1.25) * rcos(a) >> 12;
      positions[0].vz = CV(1.25) * rsin(a) >> 12;
      positions[1].vx = CV(1.25) * rcos(a + 0x555) >> 12;
      positions[1].vz = CV(1.25) * rsin(a + 0x555) >> 12;
      positions[2].vx = CV(1.25) * rcos(a + 0xaaa) >> 12;
      positions[2].vz = CV(1.25) * rsin(a + 0xaaa) >> 12;
      positions[3].vx = CV(1.25) * rcos(a + 0x2ab) >> 12;
      positions[3].vz = CV(1.25) * rsin(a + 0x2ab) >> 12;
      positions[4].vx = CV(1.25) * rcos(a + 0x7ff) >> 12;
      positions[4].vz = CV(1.25) * rsin(a + 0x7ff) >> 12;
      positions[5].vx = CV(1.25) * rcos(a + 0xd55) >> 12;
      positions[5].vz = CV(1.25) * rsin(a + 0xd55) >> 12;
      OBJ.todo_x24 += 0x20;

      for (i = 0; i < 6; i++) {
         obj_s2->d.sprite.coords[0].x = x;
         obj_s2->d.sprite.coords[0].z = z;
         obj_s2->d.sprite.coords[0].y = obj->y1.n + OBJ.todo_x28;
         obj_s2->d.sprite.coords[1].x = x;
         obj_s2->d.sprite.coords[1].z = z;
         obj_s2->d.sprite.coords[1].y = obj->y1.n + OBJ.todo_x28;
         obj_s2->d.sprite.coords[2].x = x + positions[local_38[i][0]].vx;
         obj_s2->d.sprite.coords[2].z = z + positions[local_38[i][0]].vz;
         obj_s2->d.sprite.coords[2].y = obj->y1.n + OBJ.todo_x2a;
         obj_s2->d.sprite.coords[3].x = x + positions[local_38[i][1]].vx;
         obj_s2->d.sprite.coords[3].z = z + positions[local_38[i][1]].vz;
         obj_s2->d.sprite.coords[3].y = obj->y1.n + OBJ.todo_x2a;

         obj_s2->x1.n = (obj_s2->d.sprite.coords[0].x + obj_s2->d.sprite.coords[1].x +
                         obj_s2->d.sprite.coords[2].x + obj_s2->d.sprite.coords[3].x) >>
                        2;
         obj_s2->z1.n = (obj_s2->d.sprite.coords[0].z + obj_s2->d.sprite.coords[1].z +
                         obj_s2->d.sprite.coords[2].z + obj_s2->d.sprite.coords[3].z) >>
                        2;
         obj_s2->y1.n = obj_s2->d.sprite.coords[3].y;

         AddObjPrim3(gGraphicsPtr->ot, obj_s2);
         if ((i + 1) % 2 != 0) {
            obj_s2->d.sprite.gfxIdx++;
         }
      }

      obj_s2->d.sprite.coords[0].x = x + positions[0].vx;
      obj_s2->d.sprite.coords[0].z = z + positions[0].vz;
      obj_s2->d.sprite.coords[0].y = obj->y1.n + OBJ.todo_x2a;
      obj_s2->d.sprite.coords[1].x = x + positions[0].vx;
      obj_s2->d.sprite.coords[1].z = z + positions[0].vz;
      obj_s2->d.sprite.coords[1].y = obj->y1.n + OBJ.todo_x2a;
      obj_s2->d.sprite.coords[2].x = x + positions[1].vx;
      obj_s2->d.sprite.coords[2].z = z + positions[1].vz;
      obj_s2->d.sprite.coords[2].y = obj->y1.n + OBJ.todo_x2a;
      obj_s2->d.sprite.coords[3].x = x + positions[2].vx;
      obj_s2->d.sprite.coords[3].z = z + positions[2].vz;
      obj_s2->d.sprite.coords[3].y = obj->y1.n + OBJ.todo_x2a;
      AddObjPrim4(gGraphicsPtr->ot, obj_s2);

      obj_s2->d.sprite.coords[0].x = x + positions[3].vx;
      obj_s2->d.sprite.coords[0].z = z + positions[3].vz;
      obj_s2->d.sprite.coords[0].y = obj->y1.n + OBJ.todo_x2a;
      obj_s2->d.sprite.coords[1].x = x + positions[3].vx;
      obj_s2->d.sprite.coords[1].z = z + positions[3].vz;
      obj_s2->d.sprite.coords[1].y = obj->y1.n + OBJ.todo_x2a;
      obj_s2->d.sprite.coords[2].x = x + positions[4].vx;
      obj_s2->d.sprite.coords[2].z = z + positions[4].vz;
      obj_s2->d.sprite.coords[2].y = obj->y1.n + OBJ.todo_x2a;
      obj_s2->d.sprite.coords[3].x = x + positions[5].vx;
      obj_s2->d.sprite.coords[3].z = z + positions[5].vz;
      obj_s2->d.sprite.coords[3].y = obj->y1.n + OBJ.todo_x2a;
      AddObjPrim4(gGraphicsPtr->ot, obj_s2);

      obj_s2->functionIndex = OBJF_NULL;

      switch (obj->state3) {
      case 0:
         if ((OBJ.todo_x2c == 0) && (++OBJ.todo_x2e % 3 == 0)) {
            OBJ.todo_x26++;
            if (OBJ.todo_x26 == 7) {
               OBJ.todo_x2c++;
               OBJ.todo_x26 = 6;
            }
         } else if ((OBJ.todo_x2e % 3 == 0) && (--OBJ.todo_x26 < 0)) {
            OBJ.todo_x2c = 0;
            OBJ.todo_x26 = 0;
         }
         OBJ.todo_x28 += CV(0.75);
         if (OBJ.todo_x28 > CV(8.0)) {
            OBJ.todo_x28 = CV(8.0);
            obj->state3++;
         }
         break;

      case 1:
         if ((OBJ.todo_x2c == 0) && (++OBJ.todo_x2e % 3 == 0)) {
            OBJ.todo_x26++;
            if (OBJ.todo_x26 == 7) {
               OBJ.todo_x2c++;
               OBJ.todo_x26 = 6;
            }
         } else if ((OBJ.todo_x2e % 3 == 0) && (--OBJ.todo_x26 < 0)) {
            OBJ.todo_x2c = 0;
            OBJ.todo_x26 = 0;
            OBJ.todo_x30++;
         }
         if (OBJ.todo_x30 == 2) {
            obj->state3++;
            OBJ.todo_x28 = 0;
         }
         break;

      case 2:
         OBJ.todo_x28 = CV(8.0) * (ONE - rsin(OBJ.todo_x30)) >> 12;
         if ((OBJ.todo_x2c == 0) && (++OBJ.todo_x2e % 3 == 0)) {
            OBJ.todo_x26++;
            if (OBJ.todo_x26 == 7) {
               OBJ.todo_x2c++;
               OBJ.todo_x26 = 6;
            }
         } else if ((OBJ.todo_x2e % 3 == 0) && (--OBJ.todo_x26 < 0)) {
            OBJ.todo_x2c = 0;
            OBJ.todo_x26 = 0;
         }
         OBJ.todo_x30 += 0x20;
         if (OBJ.todo_x30 > 0x400) {
            obj->functionIndex = OBJF_NULL;
            obj_v1 = OBJ.fx;
            obj_v1->state = 99;
         }
         break;
      }

      break;
   }
}

void DrawLightningBoltSegment(Object *fx, Object *sprite, s16 param_3, s16 param_4, s16 param_5, s16 param_6,
                   s16 param_7, s16 param_8, s16 param_9) {

   sprite->d.sprite.coords[0].x = fx->x1.n + param_3 + 0x100;
   sprite->d.sprite.coords[1].x = fx->x1.n + param_3 - 0x100;
   if (param_9) { // 1
      sprite->d.sprite.coords[2].x = fx->x1.n + param_6 + 0x110 - (rand() % 0x20);
      sprite->d.sprite.coords[3].x = fx->x1.n + param_6 - 0xf0 - (rand() % 0x20);
   } else {
      sprite->d.sprite.coords[2].x = fx->x1.n + param_6 + 0x100;
      sprite->d.sprite.coords[3].x = fx->x1.n + param_6 - 0x100;
   }
   sprite->d.sprite.coords[0].y = sprite->d.sprite.coords[1].y = fx->y1.n + param_4;
   sprite->d.sprite.coords[2].y = sprite->d.sprite.coords[3].y = fx->y1.n + param_7;
   sprite->d.sprite.coords[0].z = sprite->d.sprite.coords[1].z = fx->z1.n + param_5;
   if (param_9) { // 2
      sprite->d.sprite.coords[2].z = fx->z1.n + param_8 + 0x10 - (rand() % 0x20);
      sprite->d.sprite.coords[3].z = fx->z1.n + param_8 + 0x10 - (rand() % 0x20);
   } else {
      sprite->d.sprite.coords[2].z = fx->z1.n + param_8;
      sprite->d.sprite.coords[3].z = fx->z1.n + param_8;
   }
   sprite->d.sprite.semiTrans = 0;
   AddObjPrim4(gGraphicsPtr->ot, sprite);

   sprite->d.sprite.coords[0].x = fx->x1.n - (param_3 / 2) + 0x100;
   sprite->d.sprite.coords[1].x = fx->x1.n - (param_3 / 2) - 0x100;
   if (param_9) { // 3
      sprite->d.sprite.coords[2].x = fx->x1.n + param_6 + 0x110 - (rand() % 0x20);
      sprite->d.sprite.coords[3].x = fx->x1.n + param_6 - 0xf0 - (rand() % 0x20);
   } else {
      sprite->d.sprite.coords[2].x = fx->x1.n - (param_6 / 2) + 0x100;
      sprite->d.sprite.coords[3].x = fx->x1.n - (param_6 / 2) - 0x100;
   }
   sprite->d.sprite.coords[0].y = sprite->d.sprite.coords[1].y = fx->y1.n + param_4;
   sprite->d.sprite.coords[2].y = sprite->d.sprite.coords[3].y = fx->y1.n + param_7;
   sprite->d.sprite.coords[0].z = sprite->d.sprite.coords[1].z = fx->z1.n - param_5 / 2;
   if (param_9) { // 4
      sprite->d.sprite.coords[2].z = fx->z1.n + param_8 + 0x10 - (rand() % 0x20);
      sprite->d.sprite.coords[3].z = fx->z1.n + param_8 + 0x10 - (rand() % 0x20);
   } else {
      sprite->d.sprite.coords[2].z = sprite->d.sprite.coords[3].z = fx->z1.n - param_8 / 2;
   }
   sprite->d.sprite.semiTrans = 4;
   AddObjPrim4(gGraphicsPtr->ot, sprite);

   sprite->d.sprite.coords[0].x = fx->x1.n - param_3 + 0x100;
   sprite->d.sprite.coords[1].x = fx->x1.n - param_3 - 0x100;
   if (param_9) { // 5
      sprite->d.sprite.coords[2].x = fx->x1.n + param_6 + 0x110 - (rand() % 0x20);
      sprite->d.sprite.coords[3].x = fx->x1.n + param_6 - 0xf0 - (rand() % 0x20);
   } else {
      sprite->d.sprite.coords[2].x = fx->x1.n - param_6 + 0x100;
      sprite->d.sprite.coords[3].x = fx->x1.n - param_6 - 0x100;
   }
   sprite->d.sprite.coords[0].y = sprite->d.sprite.coords[1].y = fx->y1.n + param_4;
   do {
      // FIXME: This do-while fixes the reg-swaps, but should try to identify any actual macros.
      sprite->d.sprite.coords[2].y = sprite->d.sprite.coords[3].y = fx->y1.n + param_7;
   } while (0);
   sprite->d.sprite.coords[0].z = sprite->d.sprite.coords[1].z = fx->z1.n - param_5;
   if (param_9) { // 6
      sprite->d.sprite.coords[2].z = fx->z1.n + param_8 + 0x10 - (rand() % 0x20);
      sprite->d.sprite.coords[3].z = fx->z1.n + param_8 + 0x10 - (rand() % 0x20);
   } else {
      sprite->d.sprite.coords[2].z = sprite->d.sprite.coords[3].z = fx->z1.n - param_8;
   }
   sprite->d.sprite.semiTrans = 1;
   AddObjPrim4(gGraphicsPtr->ot, sprite);
}

// TBD: Which anims use those wiggly things?
static s16 sAnimData_800fecfc[20] = {5, GFX_TBD_278, 1, GFX_TBD_279, 1, GFX_TBD_280, 1, GFX_TBD_281,
                                     1, GFX_TBD_282, 1, GFX_TBD_283, 1, GFX_TBD_284, 1, GFX_TBD_285,
                                     1, GFX_NULL,    1, GFX_NULL};

#undef OBJF
#define OBJF 147
void Objf147_LightningBolt(Object *obj) {
   static s16 lightningAnimData[64] = {
       4, GFX_LIGHTNING_1, 2, GFX_LIGHTNING_2, 2, GFX_LIGHTNING_3, 2, GFX_LIGHTNING_4,
       2, GFX_LIGHTNING_4, 2, GFX_LIGHTNING_5, 2, GFX_LIGHTNING_5, 2, GFX_LIGHTNING_6,
       2, GFX_LIGHTNING_7, 2, GFX_LIGHTNING_8, 2, GFX_LIGHTNING_1, 2, GFX_LIGHTNING_2,
       2, GFX_LIGHTNING_3, 2, GFX_LIGHTNING_4, 2, GFX_LIGHTNING_4, 2, GFX_LIGHTNING_5,
       2, GFX_LIGHTNING_5, 2, GFX_LIGHTNING_6, 2, GFX_LIGHTNING_7, 2, GFX_LIGHTNING_8,
       2, GFX_LIGHTNING_1, 2, GFX_LIGHTNING_2, 2, GFX_LIGHTNING_3, 2, GFX_LIGHTNING_4,
       2, GFX_LIGHTNING_4, 2, GFX_LIGHTNING_5, 2, GFX_LIGHTNING_5, 2, GFX_LIGHTNING_6,
       2, GFX_LIGHTNING_7, 2, GFX_LIGHTNING_8, 2, GFX_NULL,        0, GFX_NULL};

   Object *sprite;

   switch (obj->state) {
   case 0:
      obj->y1.n += CV(4.0);

      OBJ.todo_x4c = 0x80 - (rand() % 0x100);
      OBJ.todo_x50 = 0x80 - (rand() % 0x100);
      OBJ.todo_x4e = -0x400;
      OBJ.todo_x34 = OBJ.todo_x4c / 4;
      OBJ.todo_x36 = OBJ.todo_x4e / 4;
      OBJ.todo_x38 = OBJ.todo_x50 / 4;
      OBJ.todo_x3a = OBJ.todo_x34 * 2;
      OBJ.todo_x3c = OBJ.todo_x36 * 2;
      OBJ.todo_x3e = OBJ.todo_x38 * 2;
      OBJ.todo_x40 = OBJ.todo_x34 * 3;
      OBJ.todo_x42 = OBJ.todo_x36 * 3;
      OBJ.todo_x44 = OBJ.todo_x38 * 3;

      if (OBJ.clut == CLUT_NULL) {
         OBJ.clut = CLUT_REDS;
      }

      sprite = Obj_GetUnused();
      OBJ.sprite = sprite;
      sprite->functionIndex = OBJF_NOOP;
      sprite->d.sprite.animData = lightningAnimData;
      sprite->d.sprite.animInitialized = 0;
      sprite->d.sprite.clut = OBJ.clut;

      obj->state++;
      break;

   case 1:
      sprite = OBJ.sprite;

      OBJ.todo_x34 += 0x40 - (rand() % 0x80);
      OBJ.todo_x36 += 8 - (rand() % 0x10);
      OBJ.todo_x38 += 0x40 - (rand() % 0x80);
      OBJ.todo_x3a += 0x40 - (rand() % 0x80);
      OBJ.todo_x3c += 8 - (rand() % 0x10);
      OBJ.todo_x3e += 0x40 - (rand() % 0x80);
      OBJ.todo_x40 += 0x40 - (rand() % 0x80);
      OBJ.todo_x42 += 8 - (rand() % 0x10);
      OBJ.todo_x44 += 0x40 - (rand() % 0x80);

      UpdateObjAnimation(sprite);
      DrawLightningBoltSegment(obj, sprite, 0, 0, 0, OBJ.todo_x34, OBJ.todo_x36, OBJ.todo_x38, 0);
      DrawLightningBoltSegment(obj, sprite, OBJ.todo_x34, OBJ.todo_x36, OBJ.todo_x38, OBJ.todo_x3a,
                    OBJ.todo_x3c, OBJ.todo_x3e, 0);
      DrawLightningBoltSegment(obj, sprite, OBJ.todo_x3a, OBJ.todo_x3c, OBJ.todo_x3e, OBJ.todo_x40,
                    OBJ.todo_x42, OBJ.todo_x44, 0);
      DrawLightningBoltSegment(obj, sprite, OBJ.todo_x40, OBJ.todo_x42, OBJ.todo_x44, OBJ.todo_x4c,
                    OBJ.todo_x4e, OBJ.todo_x50, 1);

      if (sprite->d.sprite.animFinished) {
         obj->functionIndex = OBJF_NULL;
         sprite->functionIndex = OBJF_NULL;
      }
      break;
   }
}

void DrawDeltaMirageTriangle(Object *deltaMirage) {
   s32 i;
   s16 *p;
   s16 current;
   s16 a, b, c, d, e;
   s16 x_1, z_1, x_2, z_2, x_3, z_3;
   POLY_FT4 *poly;
   Object *sprite;

   if (deltaMirage->state == 3) {
      a = 0x100 + (0x400 * rsin(DEG(90) - deltaMirage->d.objf156.todo_x36 * 2) >> 12);
      b = 0;
      c = 0x200 * rcos(DEG(90) - deltaMirage->d.objf156.todo_x36 * 2) >> 12;
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

   p = &deltaMirage->d.objf156.todo_x26;
   for (i = 0; i < 3 - deltaMirage->state2; i++, p++) {
      current = *p;

      switch (i) {
      case 2:
         d = deltaMirage->d.objf156.todo_x24 + 0xaaa;
         e = deltaMirage->d.objf156.todo_x24;
         break;
      case 1:
         d = deltaMirage->d.objf156.todo_x24 + 0x555;
         e = deltaMirage->d.objf156.todo_x24 + 0xaaa;
         break;
      case 0:
      default:
         d = deltaMirage->d.objf156.todo_x24;
         e = deltaMirage->d.objf156.todo_x24 + 0x555;
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
      OBJ.todo_x24 = gCameraRotation.vy - 0x656;

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
         OBJ.todo_x2a += 0x20;
         if (OBJ.todo_x2a > DEG(90)) {
            OBJ.todo_x2a = DEG(90);
            obj->state++;
         }
         break;
      case 1:
         OBJ.todo_x28 += 0x20;
         if (OBJ.todo_x28 > DEG(90)) {
            OBJ.todo_x28 = DEG(90);
            obj->state2--;
         }
         break;
      case 2:
         OBJ.todo_x26 += 0x20;
         if (OBJ.todo_x26 > DEG(90)) {
            OBJ.todo_x26 = DEG(90);
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
      OBJ.todo_x36 = 0x200;
      obj->state++;

   // fallthrough
   case 3:
      DrawDeltaMirageTriangle(obj);
      OBJ.todo_x24 += 0x10;
      OBJ.timer++;
      OBJ.todo_x36 -= 4;
      if (OBJ.todo_x36 == 0x28) {
         gSignal3 = 1;
      }
      if (OBJ.todo_x36 < 0x20) {
         gLightColor.r += 12;
         gLightColor.g += 12;
         gLightColor.b += 12;
      }
      if (OBJ.todo_x36 < 0) {
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

      OBJ.todo_x24 = 0x400 + (rand() % 0x400);
      OBJ.todo_x26 = rand() % 0x1000;
      OBJ.todo_x28 = rand() % 0x1000;
      OBJ.todo_x2a = rand() % 0x1000;
      OBJ.todo_x2c = 0x60 - (rand() % 0xc1);
      OBJ.todo_x2e = 0x60 - (rand() % 0xc1);
      OBJ.todo_x30 = 0x60 - (rand() % 0xc1);

      obj->state++;

   // fallthrough
   case 1:
      obj_s0 = Obj_GetUnused();
      obj_s0->functionIndex = OBJF_NOOP;
      obj_s0->d.sprite.gfxIdx = GFX_COLOR_14;
      obj_s0->d.sprite.clut = CLUT_BLUES;
      obj_s0->d.sprite.semiTrans = 1;

      OBJ.todo_x34 = OBJ.todo_x24 * OBJ.todo_x36 / 0x30;
      obj_s0->d.sprite.coords[0].x = obj->x1.n;
      obj_s0->d.sprite.coords[0].z = obj->z1.n;
      obj_s0->d.sprite.coords[0].y = obj->y1.n;
      obj_s0->d.sprite.coords[1].x = obj->x1.n;
      obj_s0->d.sprite.coords[1].z = obj->z1.n;
      obj_s0->d.sprite.coords[1].y = obj->y1.n;
      obj_s0->d.sprite.coords[2].x = obj->x1.n + OBJ.todo_x34 * rcos(OBJ.todo_x26) / ONE;
      obj_s0->d.sprite.coords[2].z = obj->z1.n + OBJ.todo_x34 * rsin(OBJ.todo_x28) / ONE;
      obj_s0->d.sprite.coords[2].y = obj->y1.n + OBJ.todo_x34 * rsin(OBJ.todo_x2a) / ONE;
      obj_s0->d.sprite.coords[3].x = obj->x1.n + OBJ.todo_x34 * rcos(OBJ.todo_x26 + 0x10) / ONE;
      obj_s0->d.sprite.coords[3].z = obj->z1.n + OBJ.todo_x34 * rsin(OBJ.todo_x28 + 0x10) / ONE;
      obj_s0->d.sprite.coords[3].y = obj->y1.n + OBJ.todo_x34 * rsin(OBJ.todo_x2a + 0x10) / ONE;

      AddObjPrim4(gGraphicsPtr->ot, obj_s0);

      OBJ.todo_x26 += OBJ.todo_x2c;
      OBJ.todo_x28 += OBJ.todo_x2e;
      OBJ.todo_x2a += OBJ.todo_x30;

      obj_s0->functionIndex = OBJF_NULL;

      switch (obj->state2) {
      case 0:
         OBJ.todo_x32++;
         OBJ.todo_x36++;
         if (OBJ.todo_x36 == 0x31) {
            obj->state2++;
            OBJ.todo_x36 = 0x30;
         }
         break;
      case 1:
         OBJ.todo_x32++;
         if (OBJ.todo_x32 == 0x81) {
            obj->state2++;
         }
         break;
      case 2:
         OBJ.todo_x32++;
         OBJ.todo_x36 -= 3;
         if (OBJ.todo_x36 == 0) {
            obj->functionIndex = OBJF_NULL;
         }
         break;
      }

      break;
   }
}

#undef OBJF
#define OBJF 158
void Objf158_Explosion_FX1(Object *obj) {
   Object *unitSprite;
   Object *obj_s0;
   s32 spriteX, spriteY, spriteW, spriteH;
   s16 width, height;

   switch (obj->state) {
   case 0:
      unitSprite = GetUnitSpriteAtPosition(gTargetZ, gTargetX);
      obj->x1.n = unitSprite->x1.n;
      obj->y1.n = unitSprite->y1.n;
      obj->z1.n = unitSprite->z1.n;
      OBJ.unitSprite = unitSprite;
      unitSprite->d.sprite.hidden = 1;

      obj_s0 = Obj_GetUnused();
      obj_s0->functionIndex = OBJF_FOCUS_CAMERA;
      obj_s0->d.objf026.target = unitSprite;
      obj_s0->d.objf026.type = 3;

      obj->state++;

   // fallthrough
   case 1:
      width = 0x40 * rsin(OBJ.timer * 0x30) / ONE;
      if (width == 0x40) {
         width = 0x3f;
      } else if (width == -0x40) {
         width = -0x3f;
      }
      height = 0x10 * rcos(OBJ.timer * 0x30) / ONE;

      unitSprite = OBJ.unitSprite;
      GetUnitSpriteVramRect(unitSprite, &spriteX, &spriteY, &spriteW, &spriteH);
      ApplyMaskEffect(spriteX, spriteY, spriteW + 1, spriteH + 1, 432 << 2, 256, width, height,
                      GFX_MASK_EFFECT_1, 0);

      obj_s0 = Obj_GetUnused();
      CopyObject(unitSprite, obj_s0);
      obj_s0->d.sprite.hidden = 0;
      obj_s0->d.sprite.gfxIdx = GFX_MASK_EFFECT_1;
      obj_s0->d.sprite.clut = CLUT_REDS;
      obj_s0->d.sprite.semiTrans = 0;

      if (unitSprite->d.sprite.gfxIdx >= 21 && unitSprite->d.sprite.gfxIdx <= 30) {
         obj_s0->d.sprite.boxIdx = 1;
      } else {
         obj_s0->d.sprite.boxIdx = 0;
      }

      AddObjPrim6(gGraphicsPtr->ot, obj_s0, 0);
      obj_s0->functionIndex = OBJF_NULL;

      OBJ.timer += 2;
      if (OBJ.timer % 4 == 0) {
         obj_s0 = Obj_GetUnused();
         obj_s0->functionIndex = OBJF_EXPLOSION_RAYS;
         obj_s0->x1.n = obj->x1.n;
         obj_s0->y1.n = obj->y1.n + CV(0.25);
         obj_s0->z1.n = obj->z1.n;
         obj_s0->d.objf159.todo_x5a = OBJ.timer;
      }
      if (OBJ.timer == 150) {
         gSignal3 = 1;
      } else if (OBJ.timer == 180) {
         obj->functionIndex = OBJF_NULL;
         unitSprite->d.sprite.hidden = 0;
      }
      break;
   }
}

#undef OBJF
#define OBJF 159
void Objf159_Explosion_Rays(Object *obj) {
   static s16 colorsAnimData[64] = {
       4, GFX_COLOR_1,  3, GFX_COLOR_2,  3, GFX_COLOR_3,  3, GFX_COLOR_4,  3, GFX_COLOR_5,
       3, GFX_COLOR_6,  3, GFX_COLOR_7,  3, GFX_COLOR_8,  3, GFX_COLOR_9,  3, GFX_COLOR_10,
       3, GFX_COLOR_11, 3, GFX_COLOR_12, 3, GFX_COLOR_13, 3, GFX_COLOR_12, 3, GFX_COLOR_13,
       3, GFX_COLOR_14, 3, GFX_COLOR_15, 3, GFX_COLOR_14, 3, GFX_COLOR_13, 3, GFX_COLOR_12,
       3, GFX_COLOR_11, 3, GFX_COLOR_10, 3, GFX_COLOR_9,  3, GFX_COLOR_8,  3, GFX_COLOR_7,
       3, GFX_COLOR_6,  3, GFX_COLOR_5,  3, GFX_COLOR_4,  3, GFX_COLOR_3,  3, GFX_COLOR_2,
       3, GFX_NULL,     1, GFX_NULL};

   Object *sprite;
   s32 i;
   s16 a, b;
   s16 angle, angleOfs;
   s16 h_1, h_2;
   s32 sum;

   switch (obj->state) {
   case 0:
      OBJ.todo_x24 = rand() % 0x1000;
      OBJ.todo_x26 = rand() % 0x300 + 0x300;
      OBJ.todo_x28 = rand() % 0x40 + 0x40;
      OBJ.todo_x2c = rand() % 0x1000;
      OBJ.todo_x2e = rand() % 0x300 + 0x300;
      OBJ.todo_x30 = rand() % 0x40 + 0x40;
      OBJ.todo_x34 = rand() % 0x1000;
      OBJ.todo_x36 = rand() % 0x300 + 0x300;
      OBJ.todo_x38 = rand() % 0x40 + 0x40;
      OBJ.todo_x3c = rand() % 0x1000;
      OBJ.todo_x3e = rand() % 0x300 + 0x300;
      OBJ.todo_x40 = rand() % 0x40 + 0x40;
      OBJ.todo_x44 = rand() % 0x1000;
      OBJ.todo_x46 = rand() % 0x300 + 0x300;
      OBJ.todo_x48 = rand() % 0x40 + 0x40;
      OBJ.todo_x4c = rand() % 0x1000;
      OBJ.todo_x4e = rand() % 0x300 + 0x300;
      OBJ.todo_x50 = rand() % 0x40 + 0x40;

      sprite = Obj_GetUnused();
      sprite->functionIndex = OBJF_NOOP;
      sprite->d.sprite.animData = colorsAnimData;
      sprite->d.sprite.animInitialized = 0;
      sprite->d.sprite.semiTrans = 1;
      sprite->x1.n = obj->x1.n;
      sprite->z1.n = obj->z1.n;
      sprite->y1.n = obj->y1.n + CV(0.5);
      OBJ.sprite = sprite;

      obj->state++;

   // fallthrough
   case 1:
      sprite = OBJ.sprite;
      sum = 0;
      angle = OBJ.todo_x58;
      OBJ.todo_x58 += 8;

      for (i = 0; i < 6; i++) {
         switch (i) {
         case 0:
            a = OBJ.todo_x2a;
            b = a - OBJ.todo_x26;
            angleOfs = OBJ.todo_x24;
            break;
         case 1:
            a = OBJ.todo_x32;
            b = a - OBJ.todo_x2e;
            angleOfs = OBJ.todo_x2c;
            break;
         case 2:
            a = OBJ.todo_x3a;
            b = a - OBJ.todo_x36;
            angleOfs = OBJ.todo_x34;
            break;
         case 3:
            a = OBJ.todo_x42;
            b = a - OBJ.todo_x3e;
            angleOfs = OBJ.todo_x3c;
            break;
         case 4:
            a = OBJ.todo_x4a;
            b = a - OBJ.todo_x46;
            angleOfs = OBJ.todo_x44;
            break;
         case 5:
            a = OBJ.todo_x52;
            b = a - OBJ.todo_x4e;
            angleOfs = OBJ.todo_x4c;
            break;
         }

         if (a >= 0x600) {
            a = 0x600;
         }
         if (b >= 0x600) {
            b = 0x600;
         } else if (b <= 0) {
            b = 0;
         }

         sum += b;
         h_1 = a * OBJ.todo_x5a / 180;
         h_2 = b * OBJ.todo_x5a / 180;
         a = a * (180 - OBJ.todo_x5a) / 180;
         b = b * (180 - OBJ.todo_x5a) / 180;

         sprite->d.sprite.coords[0].x = obj->x1.n + (a * rcos(angle + angleOfs) >> 12);
         sprite->d.sprite.coords[0].z = obj->z1.n + (a * rsin(angle + angleOfs) >> 12);
         sprite->d.sprite.coords[0].y = obj->y1.n + h_1;
         sprite->d.sprite.coords[1].x = obj->x1.n + (a * rcos(angle + angleOfs + 0x40) >> 12);
         sprite->d.sprite.coords[1].z = obj->z1.n + (a * rsin(angle + angleOfs + 0x40) >> 12);
         sprite->d.sprite.coords[1].y = obj->y1.n + h_1;
         sprite->d.sprite.coords[2].x = obj->x1.n + (b * rcos(angle + angleOfs) >> 12);
         sprite->d.sprite.coords[2].z = obj->z1.n + (b * rsin(angle + angleOfs) >> 12);
         sprite->d.sprite.coords[2].y = obj->y1.n + h_2;
         sprite->d.sprite.coords[3].x = obj->x1.n + (b * rcos(angle + angleOfs + 0x40) >> 12);
         sprite->d.sprite.coords[3].z = obj->z1.n + (b * rsin(angle + angleOfs + 0x40) >> 12);
         sprite->d.sprite.coords[3].y = obj->y1.n + h_2;

         sprite->x1.n = (sprite->d.sprite.coords[0].x + sprite->d.sprite.coords[3].x) >> 1;
         sprite->y1.n = (sprite->d.sprite.coords[0].y + sprite->d.sprite.coords[3].y) >> 1;
         sprite->z1.n = (sprite->d.sprite.coords[0].z + sprite->d.sprite.coords[3].z) >> 1;

         UpdateObjAnimation(sprite);
         AddObjPrim3(gGraphicsPtr->ot, sprite);
      }

      OBJ.todo_x2a += OBJ.todo_x28;
      OBJ.todo_x32 += OBJ.todo_x30;
      OBJ.todo_x3a += OBJ.todo_x38;
      OBJ.todo_x42 += OBJ.todo_x40;
      OBJ.todo_x4a += OBJ.todo_x48;
      OBJ.todo_x52 += OBJ.todo_x50;

      if (sum == 0x2400) {
         obj->functionIndex = OBJF_NULL;
         sprite->functionIndex = OBJF_NULL;
      }
      break;
   }
}

static s16 sAnimData_800fee24[28] = {
    4,  GFX_COLOR_1, 16, GFX_COLOR_2, 16, GFX_COLOR_3, 16, GFX_COLOR_4, 16, GFX_COLOR_5,
    16, GFX_COLOR_6, 16, GFX_COLOR_7, 16, GFX_COLOR_6, 16, GFX_COLOR_5, 16, GFX_COLOR_4,
    16, GFX_COLOR_3, 16, GFX_COLOR_2, 16, GFX_NULL,    1,  GFX_NULL};

#undef OBJF
#define OBJF 220
void Objf220_Explosion_FX2(Object *obj) {
   static s16 explosionAnimData[26] = {0, GFX_EXPLOSION_1,  4, GFX_EXPLOSION_2,  4, GFX_EXPLOSION_3,
                                       4, GFX_EXPLOSION_4,  4, GFX_EXPLOSION_5,  4, GFX_EXPLOSION_6,
                                       4, GFX_EXPLOSION_7,  4, GFX_EXPLOSION_8,  4, GFX_EXPLOSION_9,
                                       4, GFX_EXPLOSION_10, 4, GFX_EXPLOSION_11, 4, GFX_NULL,
                                       0, GFX_NULL};

   Object *obj_s6;
   Object *unitSprite;
   s32 i, j;
   s16 a, b, c;

   switch (obj->state) {
   case 0:
      unitSprite = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
      obj->x1.n = unitSprite->x1.n;
      obj->z1.n = unitSprite->z1.n;
      obj->y1.n = unitSprite->y1.n;

      obj_s6 = Obj_GetUnused();
      obj_s6->functionIndex = OBJF_NOOP;
      obj_s6->d.sprite.animData = explosionAnimData;
      OBJ.sprite = obj_s6;

      obj->state++;

   // fallthrough
   case 1:
      obj_s6 = OBJ.sprite;
      UpdateObjAnimation(obj_s6);

      for (i = 0; i < 6; i++) {
         for (j = 0; j < 16; j++) {
            if (OBJ.todo_x24 <= 0x400) {
               a = (i * 0x80 + 0x80) * (ONE - rcos(OBJ.todo_x24)) / ONE;
            } else {
               a = (i * 0x80 + 0x80) + (i * 0x80 + 0x80) * (ONE - rcos(OBJ.todo_x24 - 0x400)) / ONE;
            }

            if (i % 2 == 0) {
               b = a * rcos(j * 0x100 + OBJ.todo_x26 * i) >> 12;
               c = a * rsin(j * 0x100 + OBJ.todo_x26 * i) >> 12;
            } else {
               b = a * rcos(j * 0x100 - OBJ.todo_x26 * i) >> 12;
               c = a * rsin(j * 0x100 - OBJ.todo_x26 * i) >> 12;
            }

            obj_s6->x1.n = obj->x1.n + b;
            obj_s6->z1.n = obj->z1.n + c;

            if (OBJ.todo_x24 <= 0x400) {
               obj_s6->y1.n = obj->y1.n + (rsin(a * 4) >> 4);
            } else {
               obj_s6->y1.n =
                   obj->y1.n + (rsin(a * 4) >> 4) - 0x200 * rsin(OBJ.todo_x24 - 0x400) / ONE;
            }

            AddObjPrim6(gGraphicsPtr->ot, obj_s6, 0);
         }
      }

      OBJ.todo_x26 += 0x60;
      OBJ.todo_x24 += 0x24;
      if (OBJ.todo_x24 == 0x5e8) {
         obj_s6 = Obj_GetUnused();
         obj_s6->functionIndex = OBJF_ENGULF_FLAME_DAMAGE + OBJ.endingFxType;
         obj_s6->x1.n = obj->x1.n;
         obj_s6->z1.n = obj->z1.n;
      } else if (OBJ.todo_x24 >= 0x800) {
         obj->functionIndex = OBJF_NULL;
         obj_s6->functionIndex = OBJF_NULL;
      }
      break;
   }
}

void Objf221_Explosion_FX3(Object *obj) {
   Object *obj_s0;

   obj_s0 = Obj_GetUnused();
   obj_s0->functionIndex = OBJF_EXPLOSION_FX2;
   obj_s0->x1.s.hi = obj->x1.s.hi;
   obj_s0->z1.s.hi = obj->z1.s.hi;
   obj_s0->d.objf220.endingFxType = 2;

   obj->functionIndex = OBJF_NULL;
}

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

      radius = OBJ.todo_x24;
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

         OBJ.todo_x24 += 8;
         if (OBJ.todo_x24 >= 0xc0) {
            sprite->functionIndex = OBJF_NULL;
            obj->state2++;
            OBJ.todo_x24 = 0xc0;
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
            obj_s0->d.objf164.todo_x32 = rand() % 5;
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

         OBJ.todo_x24 -= 8;
         if (OBJ.todo_x24 < 0) {
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
         obj_s0->d.objf164.todo_x32 = 0;
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
         OBJ.todo_x4c = 0;
         OBJ.todo_x4e = 0x800;
         OBJ.todo_x28 = rand() % 0x50 + 0x32;
      } else {
         OBJ.todo_x4c = 0x400;
         OBJ.todo_x4e = 0;
         OBJ.todo_x28 = -0x50 - (rand() % 0x78);
      }

      OBJ.todo_x26 = rand() % 0x1000;
      OBJ.todo_x30 = rand() % 0xa0 + 0x20;
      OBJ.todo_x24 = OBJ.todo_x4c;
      obj_s1 = OBJ.sprite;
      obj_s1->d.sprite.animInitialized = 0;
      obj->state++;

   // fallthrough
   case 2:
      if (obj->state2++ == OBJ.todo_x32) {
         obj->state++;
         obj->state2 = 0;
      }
      break;

   case 3:
      obj_s1 = OBJ.sprite;
      a = OBJ.todo_x26;
      obj_s1->x1.n = obj->x1.n + OBJ.todo_x30 * rcos(a) / ONE;
      obj_s1->z1.n = obj->z1.n + OBJ.todo_x30 * rsin(a) / ONE;
      obj_s1->y1.n = obj->y1.n + OBJ.todo_x24;
      UpdateObjAnimation(obj_s1);
      AddObjPrim6(gGraphicsPtr->ot, obj_s1, 0);

      OBJ.todo_x24 += OBJ.todo_x28;
      if (!OBJ.downward) {
         if (OBJ.todo_x24 >= OBJ.todo_x4e) {
            obj->state = 1;
         }
      } else {
         if (OBJ.todo_x24 <= OBJ.todo_x4e) {
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
