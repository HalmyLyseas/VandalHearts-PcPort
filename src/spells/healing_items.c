/* Healing-item and healing-spell target effects: Objf306_791_792_793 (one handler in the
 * FX2 slot of Extra/Hyper/Plus/Ultra Healing) and its Objf386 sparkle child. Dispatched
 * via gSpellsEx (see spells/casting_main.c). */
#include "common.h"
#include "object.h"
#include "graphics.h"
#include "battle.h"
#include "field.h"
#include "state.h"
#include "units.h"
#include "audio.h"

#undef OBJF
#define OBJF 306
void Objf306_791_792_793_Healing_FX2(Object *obj) {
   // 306: Extra Healing, Hyper Healing; 791: Healing Plus;
   // 792: Ultra Healing; 793: Supreme Healing, Holy H2O
   Object *unitSprite;
   Object *obj_s0;
   MaskEffectPreset maskEffect;

   switch (obj->state) {
   case 0:
      unitSprite = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
      obj->x1.n = unitSprite->x1.n;
      obj->z1.n = unitSprite->z1.n;
      obj->y1.n = unitSprite->y1.n;
      OBJ.unitSprite = unitSprite;
      unitSprite->d.sprite.hidden = 1;
      obj->state++;

   // fallthrough
   case 1:
      unitSprite = OBJ.unitSprite;
      maskEffect.srcGfxIdx = GFX_TILED_FLAMES;
      maskEffect.dstGfxIdx = GFX_MASK_EFFECT_1;
      maskEffect.width = 0;
      maskEffect.height = OBJ.timer % 0x40;
      maskEffect.semiTrans = 2;
      maskEffect.clut = CLUT_BLUES;
      maskEffect.color.r = maskEffect.color.g = maskEffect.color.b =
          0x80 * rsin(OBJ.timer * 0x20) >> 12;

      switch (obj->functionIndex) {
      case OBJF_ULTRA_HEALING_FX2:
         maskEffect.srcGfxIdx = GFX_TILED_MAGMA;
         maskEffect.clut = CLUT_REDS;
         break;

      case OBJF_HEALING_FX2:
         // Probably left-over, since this has a dedicated function (Objf100_Healing_FX2)
         break;

      case OBJF_SUPREME_HEALING_FX2: // or Holy H2O
         maskEffect.srcGfxIdx = GFX_TILED_CLOUDS;
         maskEffect.clut = CLUT_GRAYS;
         break;

      case OBJF_EXTRA_HEALING_FX2: // or Hyper Healing
         maskEffect.srcGfxIdx = GFX_TILED_RED_SPARKLES;
         maskEffect.clut = CLUT_REDS;
         break;

      case OBJF_HEALING_PLUS_FX2:
         maskEffect.clut = CLUT_REDS;
         break;
      }

      RenderMaskEffect(unitSprite, &maskEffect);

      obj_s0 = Obj_GetUnused();
      CopyObject(unitSprite, obj_s0);
      obj_s0->d.sprite.hidden = 0;
      RenderUnitSprite(gGraphicsPtr->ot, obj_s0, 0);
      obj_s0->functionIndex = OBJF_NULL;

      if (--obj->mem <= 0) {
         obj_s0 = CreatePositionedObj(unitSprite, OBJF_HEALING_SPARKLE_386);

         switch (obj->functionIndex) {
         case OBJF_ULTRA_HEALING_FX2:
            obj_s0->d.objf386.clut = CLUT_PURPLES;
            obj_s0->y3.n = (rand() % 8 + 6) * 2;
            obj->mem = (rand() + 2) & 3;
            obj->mem = (rand() + 1) & 5;
            break;

         case OBJF_SUPREME_HEALING_FX2: // or Holy H2O
            obj_s0->d.objf386.clut = CLUT_GREENS;
            obj_s0->y3.n = (rand() % 10 + 7) * 2;
            obj->mem = (rand() + 1) & 3;
            break;

         case OBJF_HEALING_PLUS_FX2:
         case OBJF_HEALING_FX2:
         case OBJF_EXTRA_HEALING_FX2: // or Hyper Healing
         default:
            obj_s0->d.objf386.clut = CLUT_BLUES;
            obj->mem = (rand() + 3) & 5;
            obj_s0->y3.n = (rand() % 8 + 5) * 2;
            break;
         }
      }

      if (OBJ.timer == 30) {
         obj_s0 = Obj_GetUnused();
         obj_s0->functionIndex = OBJF_DISPLAY_DAMAGE_2;
         obj_s0->x1.s.hi = unitSprite->x1.s.hi;
         obj_s0->z1.s.hi = unitSprite->z1.s.hi;
      }

      if (++OBJ.timer == 61) {
         unitSprite->d.sprite.hidden = 0;
         obj->functionIndex = OBJF_NULL;
         gSignal3 = 1;
      }

      break;
   }
}

#undef OBJF
#define OBJF 386
void Objf386_HealingSparkle(Object *obj) {
   static s16 sparkleAnimData[20] = {
       5, GFX_SPARKLE_1, 1, GFX_SPARKLE_2, 1, GFX_SPARKLE_3, 1, GFX_SPARKLE_4, 1, GFX_SPARKLE_5,
       1, GFX_SPARKLE_4, 1, GFX_SPARKLE_3, 1, GFX_SPARKLE_2, 1, GFX_NULL,      1, GFX_NULL};

   s32 skip;
   s32 i;
   s16 theta;

   switch (obj->state) {
   case 0:
      obj->x2.n = obj->x1.n;
      obj->z2.n = obj->z1.n;
      obj->y2.n = obj->y1.n;
      OBJ.animData = sparkleAnimData;

      skip = rand() & 7;
      for (i = 0; i < skip; i++) {
         UpdateObjAnimation(obj);
      }

      obj->state2 = rand() & 0xfff;
      obj->mem = (rand() + 0x20) & 0x3f;

      if (obj->y3.n == 0) {
         obj->y3.n = (rand() % 6 + 4) * 2;
      }

      OBJ.timer = 20;
      obj->state++;

   // fallthrough
   case 1:
      theta = obj->state2;
      obj->x1.n = obj->x2.n + (rcos(theta) >> 5);
      obj->z1.n = obj->z2.n + (rsin(theta) >> 5);
      obj->y1.n = obj->y2.n;

      UpdateObjAnimation(obj);
      AddObjPrim6(gGraphicsPtr->ot, obj, 0);

      obj->y2.n += obj->y3.n;
      obj->state2 += obj->mem;

      if (--OBJ.timer <= 0) {
         obj->functionIndex = OBJF_NULL;
      }

      break;
   }
}

