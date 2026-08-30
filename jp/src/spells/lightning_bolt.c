/* The generic sky lightning bolt: Objf147_LightningBolt, spawned by Objf148_ThunderStrike
 * (spells/casting_main.c), plus DrawLightningBoltSegment, which renders one jagged segment.
 * Dispatched via gSpellsEx (docs/decomp/spell-fx-dispatch.md). */
#include "common.h"
#include "object.h"
#include "graphics.h"
#include "battle.h"
#include "field.h"

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

      OBJ.tipX = 0x80 - (rand() % 0x100);
      OBJ.tipZ = 0x80 - (rand() % 0x100);
      OBJ.tipY = -0x400;
      OBJ.joint1X = OBJ.tipX / 4;
      OBJ.joint1Y = OBJ.tipY / 4;
      OBJ.joint1Z = OBJ.tipZ / 4;
      OBJ.joint2X = OBJ.joint1X * 2;
      OBJ.joint2Y = OBJ.joint1Y * 2;
      OBJ.joint2Z = OBJ.joint1Z * 2;
      OBJ.joint3X = OBJ.joint1X * 3;
      OBJ.joint3Y = OBJ.joint1Y * 3;
      OBJ.joint3Z = OBJ.joint1Z * 3;

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

      OBJ.joint1X += 0x40 - (rand() % 0x80);
      OBJ.joint1Y += 8 - (rand() % 0x10);
      OBJ.joint1Z += 0x40 - (rand() % 0x80);
      OBJ.joint2X += 0x40 - (rand() % 0x80);
      OBJ.joint2Y += 8 - (rand() % 0x10);
      OBJ.joint2Z += 0x40 - (rand() % 0x80);
      OBJ.joint3X += 0x40 - (rand() % 0x80);
      OBJ.joint3Y += 8 - (rand() % 0x10);
      OBJ.joint3Z += 0x40 - (rand() % 0x80);

      UpdateObjAnimation(sprite);
      DrawLightningBoltSegment(obj, sprite, 0, 0, 0, OBJ.joint1X, OBJ.joint1Y, OBJ.joint1Z, 0);
      DrawLightningBoltSegment(obj, sprite, OBJ.joint1X, OBJ.joint1Y, OBJ.joint1Z, OBJ.joint2X,
                    OBJ.joint2Y, OBJ.joint2Z, 0);
      DrawLightningBoltSegment(obj, sprite, OBJ.joint2X, OBJ.joint2Y, OBJ.joint2Z, OBJ.joint3X,
                    OBJ.joint3Y, OBJ.joint3Z, 0);
      DrawLightningBoltSegment(obj, sprite, OBJ.joint3X, OBJ.joint3Y, OBJ.joint3Z, OBJ.tipX,
                    OBJ.tipY, OBJ.tipZ, 1);

      if (sprite->d.sprite.animFinished) {
         obj->functionIndex = OBJF_NULL;
         sprite->functionIndex = OBJF_NULL;
      }
      break;
   }
}

