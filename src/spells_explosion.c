/* Explosion spell effects: Objf158/159 (FX1/FX2) and the Objf220/221 fireball pair.
 * Dispatched data-driven via gSpellsEx (see spells_casting_main.c's header for the
 * model). */
#include "common.h"
#include "object.h"
#include "graphics.h"
#include "battle.h"
#include "field.h"

extern void ApplyMaskEffect(s16, s16, s16, s16, s16, s16, s16, s16, s16, s16);

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
         obj_s0->d.objf159.tilt = OBJ.timer;
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
      OBJ.theta1 = rand() % 0x1000;
      OBJ.span1 = rand() % 0x300 + 0x300;
      OBJ.speed1 = rand() % 0x40 + 0x40;
      OBJ.theta2 = rand() % 0x1000;
      OBJ.span2 = rand() % 0x300 + 0x300;
      OBJ.speed2 = rand() % 0x40 + 0x40;
      OBJ.theta3 = rand() % 0x1000;
      OBJ.span3 = rand() % 0x300 + 0x300;
      OBJ.speed3 = rand() % 0x40 + 0x40;
      OBJ.theta4 = rand() % 0x1000;
      OBJ.span4 = rand() % 0x300 + 0x300;
      OBJ.speed4 = rand() % 0x40 + 0x40;
      OBJ.theta5 = rand() % 0x1000;
      OBJ.span5 = rand() % 0x300 + 0x300;
      OBJ.speed5 = rand() % 0x40 + 0x40;
      OBJ.theta6 = rand() % 0x1000;
      OBJ.span6 = rand() % 0x300 + 0x300;
      OBJ.speed6 = rand() % 0x40 + 0x40;

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
      angle = OBJ.spin;
      OBJ.spin += 8;

      for (i = 0; i < 6; i++) {
         switch (i) {
         case 0:
            a = OBJ.radius1;
            b = a - OBJ.span1;
            angleOfs = OBJ.theta1;
            break;
         case 1:
            a = OBJ.radius2;
            b = a - OBJ.span2;
            angleOfs = OBJ.theta2;
            break;
         case 2:
            a = OBJ.radius3;
            b = a - OBJ.span3;
            angleOfs = OBJ.theta3;
            break;
         case 3:
            a = OBJ.radius4;
            b = a - OBJ.span4;
            angleOfs = OBJ.theta4;
            break;
         case 4:
            a = OBJ.radius5;
            b = a - OBJ.span5;
            angleOfs = OBJ.theta5;
            break;
         case 5:
            a = OBJ.radius6;
            b = a - OBJ.span6;
            angleOfs = OBJ.theta6;
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
         h_1 = a * OBJ.tilt / 180;
         h_2 = b * OBJ.tilt / 180;
         a = a * (180 - OBJ.tilt) / 180;
         b = b * (180 - OBJ.tilt) / 180;

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

      OBJ.radius1 += OBJ.speed1;
      OBJ.radius2 += OBJ.speed2;
      OBJ.radius3 += OBJ.speed3;
      OBJ.radius4 += OBJ.speed4;
      OBJ.radius5 += OBJ.speed5;
      OBJ.radius6 += OBJ.speed6;

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
            if (OBJ.theta <= 0x400) {
               a = (i * 0x80 + 0x80) * (ONE - rcos(OBJ.theta)) / ONE;
            } else {
               a = (i * 0x80 + 0x80) + (i * 0x80 + 0x80) * (ONE - rcos(OBJ.theta - 0x400)) / ONE;
            }

            if (i % 2 == 0) {
               b = a * rcos(j * 0x100 + OBJ.spin * i) >> 12;
               c = a * rsin(j * 0x100 + OBJ.spin * i) >> 12;
            } else {
               b = a * rcos(j * 0x100 - OBJ.spin * i) >> 12;
               c = a * rsin(j * 0x100 - OBJ.spin * i) >> 12;
            }

            obj_s6->x1.n = obj->x1.n + b;
            obj_s6->z1.n = obj->z1.n + c;

            if (OBJ.theta <= 0x400) {
               obj_s6->y1.n = obj->y1.n + (rsin(a * 4) >> 4);
            } else {
               obj_s6->y1.n =
                   obj->y1.n + (rsin(a * 4) >> 4) - 0x200 * rsin(OBJ.theta - 0x400) / ONE;
            }

            AddObjPrim6(gGraphicsPtr->ot, obj_s6, 0);
         }
      }

      OBJ.spin += 0x60;
      OBJ.theta += 0x24;
      if (OBJ.theta == 0x5e8) {
         obj_s6 = Obj_GetUnused();
         obj_s6->functionIndex = OBJF_ENGULF_FLAME_DAMAGE + OBJ.endingFxType;
         obj_s6->x1.n = obj->x1.n;
         obj_s6->z1.n = obj->z1.n;
      } else if (OBJ.theta >= 0x800) {
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

