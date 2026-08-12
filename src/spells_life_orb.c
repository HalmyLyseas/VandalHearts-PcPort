/* Life Orb spell effects (Objf310-315, orb + rays) plus two strays kept by address
 * contiguity: Objf327 (Healing Circle FX2 -- FX1 is in spells_healing_circle.c) and
 * Objf322_370_371_372 (Magic Restoration FX2). The two RenderCylinder helpers close the
 * file, shared with the Map61 cutscene objects. Dispatched via gSpellsEx (see
 * spells_casting_main.c). */
#include "common.h"
#include "object.h"
#include "graphics.h"
#include "battle.h"
#include "field.h"

#undef OBJF
#define OBJF 327
void Objf327_HealingCircle_FX2(Object *obj) {
   // FX2 for Healing Circle, Healing Wave, Kingfoil, Life Orb

   // Any way to resolve this discrepancy?
   extern void ApplyMaskEffect(s16, s16, s16, s16, s16, s16, s16, s32, s16, s16);
   extern s16 gSparkleAnimData_800ffab4[36];

   Object *targetSprite;
   Object *fxSprite;
   Object *obj_s1;
   s32 fade;
   POLY_FT4 *poly;
   s32 spriteX, spriteY, spriteW, spriteH;

   switch (obj->state) {
   case 0:
      targetSprite = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
      obj->x1.n = targetSprite->x1.n;
      obj->z1.n = targetSprite->z1.n;
      obj->y1.n = targetSprite->y1.n;
      OBJ.targetSprite = targetSprite;
      obj->state2 = 0x40;
      obj->state++;

   // fallthrough
   case 1:
      targetSprite = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
      obj->x1.n = targetSprite->x1.n;
      obj->z1.n = targetSprite->z1.n;
      obj->y1.n = targetSprite->y1.n;
      targetSprite = OBJ.targetSprite;

      GetUnitSpriteVramRect(targetSprite, &spriteX, &spriteY, &spriteW, &spriteH);
      //? These increments and the need for an alternate ApplyMaskEffect declaration seem odd.
      spriteW++;
      spriteH++;
      ApplyMaskEffect(spriteX, spriteY, spriteW, spriteH, 416 << 2, 320, 0,
                      0x40 - (obj->state2 & 0x3f), GFX_MASK_EFFECT_1, 0);

      fxSprite = CreatePositionedObj(obj, OBJF_NOOP);

      if (targetSprite->d.sprite.gfxIdx >= 21 && targetSprite->d.sprite.gfxIdx <= 30) {
         fxSprite->d.sprite.boxIdx = 1;
      } else {
         fxSprite->d.sprite.boxIdx = 0;
      }

      fxSprite->d.sprite.gfxIdx = GFX_MASK_EFFECT_1;
      fxSprite->d.sprite.clut = CLUT_BLUES;
      fxSprite->d.sprite.semiTrans = 2;
      fxSprite->d.sprite.facingLeft = targetSprite->d.sprite.facingLeft;
      AddObjPrim6(gGraphicsPtr->ot, fxSprite, 0);
      fxSprite->functionIndex = OBJF_NULL;

      fade = rsin(obj->state2 * 0x20) >> 4;
      if (fade > 0xe0 || fade < 0) { //? Could also be u32 fade or 0xe0u instead.
         fade = 0xe0;
      }
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      setRGB0(poly, fade, fade, fade);
      obj->state2--;

      obj_s1 = Obj_GetUnused();
      obj_s1->functionIndex = OBJF_PARTICLE_710;
      obj_s1->x1.n = obj->x1.n + rand() % 0x120 - 0x8f;
      obj_s1->z1.n = obj->z1.n + rand() % 0x120 - 0x8f;
      obj_s1->y1.n = obj->y1.n + rand() % 0x80 + 0x41;
      obj_s1->state2 = (rand() >> 4) % 5;
      obj_s1->mem = 0x10;
      obj_s1->d.sprite.clut = CLUT_REDS;
      obj_s1->d.sprite.animData = gSparkleAnimData_800ffab4;

      if (obj->state2 == 0x20) {
         obj_s1 = Obj_GetUnused();
         obj_s1->functionIndex = OBJF_DISPLAY_DAMAGE_2;
         obj_s1->x1.n = obj->x1.n;
         obj_s1->z1.n = obj->z1.n;
      }
      if (obj->state2 <= 0) {
         obj->state++;
      }

      break;

   case 2:
      targetSprite = OBJ.targetSprite;
      targetSprite->d.sprite.semiTrans = 0;
      targetSprite->d.sprite.hidden = 0;
      obj->functionIndex = OBJF_NULL;
      gSignal3 = 1;
      break;
   }
}

#undef OBJF
#define OBJF 322
void Objf322_370_371_372_MagicRestoration_FX2(Object *obj) {
   // FX2 for Mage Oil, Mage Gem, Magic Charge
   Object *targetSprite;
   Object *fxSprite;
   Object *floatingText;
   u8 fade;
   POLY_FT4 *poly;
   MaskEffectPreset maskEffect;

   switch (obj->state) {
   case 0:

      switch (obj->functionIndex) {
      case OBJF_MAGE_OIL_FX2:
         OBJ.amount = 4;
         break;
      case OBJF_RESTORE_8_MP_UNK_FX2:
         OBJ.amount = 8;
         break;
      case OBJF_MAGIC_CHARGE_FX2:
         // Also used for Mage Gem
         OBJ.amount = 10;
         break;
      default:
         OBJ.amount = 10;
         break;
      }

      targetSprite = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
      targetSprite->d.sprite.hidden = 1;
      obj->state2 = 0x60;
      obj->state++;

   // fallthrough
   case 1:
      targetSprite = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
      obj->x1.n = targetSprite->x1.n;
      obj->z1.n = targetSprite->z1.n;
      obj->y1.n = targetSprite->y1.n;

      fxSprite = Obj_GetUnused();
      CopyObject(targetSprite, fxSprite);
      fxSprite->functionIndex = OBJF_NOOP;

      maskEffect.srcGfxIdx = GFX_TILED_VEINS;
      maskEffect.dstGfxIdx = GFX_MASK_EFFECT_1;
      maskEffect.width = 0;
      maskEffect.height = 0x60 - obj->state2;
      maskEffect.semiTrans = 2;
      fade = obj->state2 << 1;
      maskEffect.color.r = maskEffect.color.g = maskEffect.color.b = fade;
      maskEffect.clut = CLUT_PURPLES;
      RenderMaskEffect(fxSprite, &maskEffect);
      fxSprite->functionIndex = OBJF_NULL;

      targetSprite->d.sprite.hidden = 0;
      fade = (0x60 - obj->state2) << 1;
      RenderUnitSprite(gGraphicsPtr->ot, targetSprite, 0);
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      setRGB0(poly, fade, fade, fade);
      targetSprite->d.sprite.hidden = 1;

      if (--obj->state2 == 0x20) {
         floatingText = CreatePositionedObj(targetSprite, OBJF_FLOATING_DAMAGE_TEXT);
         floatingText->d.objf051.damage = OBJ.amount;
         floatingText->d.objf051.clut = CLUT_PURPLES;
      }
      if (obj->state2 <= 0) {
         targetSprite->d.sprite.hidden = 0;
         obj->state++;
      }
      break;

   case 2:
      obj->functionIndex = OBJF_NULL;
      gSignal3 = 1;
      break;
   }
}

#undef OBJF
#define OBJF 310
void Objf310_LifeOrb_Beam(Object *obj) {
   Object *sprite;
   POLY_FT4 *poly;
   s32 i;

   switch (obj->state) {
   case 0:
      sprite = Obj_GetUnused();
      sprite->functionIndex = OBJF_NOOP;
      sprite->d.sprite.gfxIdx = GFX_TILED_RED_SPARKLES_DYN_1;
      sprite->d.sprite.clut = CLUT_BLUES;
      sprite->d.sprite.semiTrans = 2;
      OBJ.sprite = sprite;
      OBJ.height = 0;
      OBJ.unused_0x26 = 0x10;
      OBJ.radius = CV(0.25);
      obj->state++;

   // fallthrough
   case 1:
      gGfxSubTextures[GFX_TILED_RED_SPARKLES_DYN_1][1] = 160 - obj->state2 % 0x20;
      sprite = OBJ.sprite;

      for (i = 0; i < 8; i++) {
         sprite->d.sprite.coords[0].x = sprite->d.sprite.coords[2].x =
             obj->x1.n + (OBJ.radius * rcos(i * DEG(45)) >> 12);
         sprite->d.sprite.coords[1].x = sprite->d.sprite.coords[3].x =
             obj->x1.n + (OBJ.radius * rcos((i + 1) * DEG(45)) >> 12);
         sprite->d.sprite.coords[0].z = sprite->d.sprite.coords[2].z =
             obj->z1.n + (OBJ.radius * rsin(i * DEG(45)) >> 12);
         sprite->d.sprite.coords[1].z = sprite->d.sprite.coords[3].z =
             obj->z1.n + (OBJ.radius * rsin((i + 1) * DEG(45)) >> 12);
         sprite->d.sprite.coords[0].y = sprite->d.sprite.coords[1].y = obj->y1.n + OBJ.height;
         sprite->d.sprite.coords[2].y = sprite->d.sprite.coords[3].y = obj->y1.n;
         sprite->x1.n = sprite->d.sprite.coords[0].x;
         sprite->z1.n = sprite->d.sprite.coords[0].z;
         sprite->y1.n = sprite->d.sprite.coords[0].y;
         AddObjPrim3(gGraphicsPtr->ot, sprite);
         poly = &gGraphicsPtr->quads[gQuadIndex - 1];
         setRGB0(poly, 0xff, 0xff, 0xff);
      }

      if (OBJ.height < CV(5.0)) {
         OBJ.height += CV(0.125);
      }

      if (--obj->state2 <= 0) {
         sprite->functionIndex = OBJF_NULL;
         obj->functionIndex = OBJF_NULL;
      }

      break;
   }
}

void Objf311_BubbleSwirl(Object *obj) {
   Object *bubble;

   switch (obj->state) {
   case 0:
      SnapToUnit(obj);
      obj->state2 = 128; // Number of bubbles
      obj->state++;

   // fallthrough
   case 1:
      bubble = Obj_GetUnused();
      bubble->functionIndex = OBJF_BUBBLE_SWIRL_BUBBLE;
      bubble->x2.n = obj->x1.n;
      bubble->z2.n = obj->z1.n;
      bubble->y2.n = obj->y1.n;
      if (--obj->state2 <= 0) {
         obj->state++;
      }
      break;

   case 2:
      obj->functionIndex = OBJF_NULL;
      gSignal3 = 1;
      break;
   }
}

#undef OBJF
#define OBJF 312
void Objf312_BubbleSwirl_Bubble(Object *obj) {
   switch (obj->state) {
   case 0:
      OBJ.gfxIdx = GFX_BUBBLE;
      OBJ.boxIdx = 5;
      OBJ.clut = CLUT_BLUES;
      OBJ.theta = rand() % DEG(360);
      OBJ.yVelocity = 0x10;
      OBJ.rVelocity = 1;
      OBJ.radius = 0x100;
      obj->state++;

   // fallthrough
   case 1:
      obj->x1.n = obj->x2.n + (OBJ.radius * rcos(OBJ.theta) >> 12);
      obj->z1.n = obj->z2.n + (OBJ.radius * rsin(OBJ.theta) >> 12);
      obj->y1.n = obj->y2.n;
      AddObjPrim6(gGraphicsPtr->ot, obj, 0);
      OBJ.theta = (OBJ.theta + 0x80) & 0xfff;
      obj->y2.n += OBJ.yVelocity;
      OBJ.radius += OBJ.rVelocity;
      if (obj->y2.n >= CV(5.0)) {
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 313
void Objf313_LifeOrb_FX1(Object *obj) {
   Object *targetSprite;
   Object *obj_v1;
   BVectorXZ *p;
   s32 i;

   gCameraRotation.vy += 0x10;

   switch (obj->state) {
   case 0:
      p = (BVectorXZ *)gTargetCoords;
      targetSprite = GetUnitSpriteAtPosition(p->z, p->x);
      obj->x1.n = targetSprite->x1.n;
      obj->z1.n = targetSprite->z1.n;
      obj->y1.n = GetTerrainElevation(obj->z1.s.hi, obj->x1.s.hi);
      obj->state2 = 0x40;
      obj->state3 = 0x80;
      obj->mem = 3;
      obj->state++;

   // fallthrough
   case 1:
      PanCamera(obj->x1.n, obj->y1.n, obj->z1.n, 3);
      if (--obj->mem <= 0) {
         for (i = 0; i < 12; i++) {
            obj_v1 = Obj_GetUnused();
            obj_v1->functionIndex = OBJF_INWARD_RAY;
            obj_v1->x2.n = obj->x1.n;
            obj_v1->z2.n = obj->z1.n;
            obj_v1->y2.n = obj->y1.n;
            obj->mem = 2 + rand() % 3;
         }
      }
      gLightColor.r = obj->state3;
      gLightColor.g = obj->state3;
      gLightColor.b = obj->state3;
      if (obj->state3 > 32) {
         obj->state3 -= 4;
      }
      if (--obj->state2 <= 0) {
         obj->state2 = 0x40;
         obj->state++;
         obj_v1 = Obj_GetUnused();
         obj_v1->functionIndex = OBJF_LIFE_ORB_BEAM;
         obj_v1->x1.n = obj->x1.n;
         obj_v1->z1.n = obj->z1.n;
         obj_v1->y1.n = obj->y1.n;
         obj_v1->state2 = 0x40;
         OBJ.beam = obj_v1;
      }
      break;

   case 2:
      obj_v1 = OBJ.beam;
      if (obj_v1->state2 < 0x40) {
         obj_v1->d.objf310.radius = (0x40 - obj_v1->state2) << 1;
      }
      if (--obj->mem <= 0) {
         for (i = 0; i < 4; i++) {
            obj_v1 = Obj_GetUnused();
            obj_v1->functionIndex = OBJF_INWARD_RAY;
            obj_v1->x2.n = obj->x1.n;
            obj_v1->z2.n = obj->z1.n;
            obj_v1->y2.n = obj->y1.n;
            obj->mem = 2 + rand() % 3;
         }
      }
      for (i = 0; i < 4; i++) {
         obj_v1 = Obj_GetUnused();
         if (obj_v1 != NULL) {
            obj_v1->functionIndex = OBJF_VERTICAL_RAY;
            obj_v1->x2.n = obj->x1.n;
            obj_v1->z2.n = obj->z1.n;
            obj_v1->y2.n = obj->y1.n;
         }
      }
      gLightColor.r = obj->state3;
      gLightColor.g = obj->state3;
      gLightColor.b = obj->state3;
      if (--obj->state2 <= 0) {
         obj->state++;
      }
      break;

   case 3:
      obj_v1 = OBJ.beam;
      if (obj_v1->state2 < 0x40) {
         obj_v1->d.objf310.radius = obj_v1->state2;
      }
      if (obj->state3 < 0x80) {
         obj->state3 += 4;
         gLightColor.r = obj->state3;
         gLightColor.g = obj->state3;
         gLightColor.b = obj->state3;
      } else {
         obj->functionIndex = OBJF_NULL;
         gSignal3 = 1;
         gLightColor.r = 0x80;
         gLightColor.g = 0x80;
         gLightColor.b = 0x80;
      }
      break;

   case 4:
      break;
   }
}

#undef OBJF
#define OBJF 314
void Objf314_InwardRay(Object *obj) {
   s32 radius;
   s32 rnd;

   switch (obj->state) {
   case 0:
      OBJ.theta1 = -(rand() % DEG(33.75));
      OBJ.theta2 = rand() % DEG(360);
      if (OBJ.dist == 0) {
         OBJ.dist = 0x500;
      }
      OBJ.unused_0x3A = rand() % 0x40 + 0x20;
      rnd = (rand() >> 3) % 0x1000;
      if (rnd >= 0x200) {
         obj->mem = 3;
      } else {
         obj->mem = 2;
      }
      OBJ.rotationSpeed_unused = -0x20 - rand() % 0x40;
      OBJ.gfxIdx = GFX_COLOR_15;
      obj->state2 = 0;

      radius = OBJ.dist * rcos(OBJ.theta1) >> 12;
      OBJ.coords[1].x = obj->x2.n + (radius * rcos(OBJ.theta2) >> 12);
      OBJ.coords[1].z = obj->z2.n + (radius * rsin(OBJ.theta2) >> 12);
      OBJ.coords[3].x = obj->x2.n + (radius * rcos(OBJ.theta2 + 16) >> 12);
      OBJ.coords[3].z = obj->z2.n + (radius * rsin(OBJ.theta2 + 16) >> 12);
      OBJ.coords[1].y = OBJ.coords[3].y = obj->y2.n + (OBJ.dist * rsin(OBJ.theta1) >> 12);

      OBJ.otOfs = 16;
      obj->state++;

   // fallthrough
   case 1:
      obj->state2++;
      OBJ.clut = 3 + obj->state2 % 3;

      OBJ.dist += (0 - OBJ.dist) >> obj->mem;
      radius = (OBJ.dist * rcos(OBJ.theta1)) >> 12;
      OBJ.coords[0].x = obj->x2.n + (radius * rcos(OBJ.theta2) >> 12);
      OBJ.coords[0].z = obj->z2.n + (radius * rsin(OBJ.theta2) >> 12);
      OBJ.coords[2].x = obj->x2.n + (radius * rcos(OBJ.theta2 + 16) >> 12);
      OBJ.coords[2].z = obj->z2.n + (radius * rsin(OBJ.theta2 + 16) >> 12);
      OBJ.coords[0].y = OBJ.coords[2].y = obj->y2.n + (OBJ.dist * rsin(OBJ.theta1) >> 12);

      obj->x1.n = OBJ.coords[0].x;
      obj->z1.n = OBJ.coords[0].z;
      obj->y1.n = OBJ.coords[0].y;

      AddObjPrim5(gGraphicsPtr->ot, obj);

      OBJ.coords[1].x = OBJ.coords[0].x;
      OBJ.coords[1].z = OBJ.coords[0].z;
      OBJ.coords[3].x = OBJ.coords[2].x;
      OBJ.coords[3].z = OBJ.coords[2].z;
      OBJ.coords[1].y = OBJ.coords[3].y = OBJ.coords[0].y;

      if (OBJ.dist <= 0x80) {
         obj->state++;
      }
      break;

   case 2:
      obj->functionIndex = OBJF_NULL;
      break;
   }
}

#undef OBJF
#define OBJF 315
void Objf315_VerticalRay(Object *obj) {
   switch (obj->state) {
   case 0:
      OBJ.theta = rand() % DEG(360);
      OBJ.radius = CV(0.125);
      OBJ.top = obj->y2.n;
      OBJ.height = rand() % CV(1.0);
      OBJ.rotationSpeed = 0x20;
      OBJ.speed = 0x40;
      OBJ.clut = 3 + rand() % 3;
      OBJ.gfxIdx = GFX_COLOR_15;
      obj->state++;

   // fallthrough
   case 1:
      OBJ.coords[0].x = OBJ.coords[2].x = obj->x2.n + (OBJ.radius * rcos(OBJ.theta) >> 12);
      OBJ.coords[0].z = OBJ.coords[2].z = obj->z2.n + (OBJ.radius * rsin(OBJ.theta) >> 12);
      OBJ.coords[1].x = OBJ.coords[3].x = obj->x2.n + (OBJ.radius * rcos(OBJ.theta + 16) >> 12);
      OBJ.coords[1].z = OBJ.coords[3].z = obj->z2.n + (OBJ.radius * rsin(OBJ.theta + 16) >> 12);
      OBJ.coords[0].y = OBJ.coords[1].y = OBJ.top;
      OBJ.coords[2].y = OBJ.top - OBJ.height;
      if (OBJ.coords[2].y < obj->y2.n) {
         OBJ.coords[2].y = obj->y2.n;
      }
      OBJ.coords[3].y = OBJ.coords[2].y;
      obj->x1.n = OBJ.coords[0].x;
      obj->z1.n = OBJ.coords[0].z;
      obj->y1.n = OBJ.coords[0].y;
      OBJ.theta += OBJ.rotationSpeed;
      AddObjPrim5(gGraphicsPtr->ot, obj, 0x400); //? Extra arg
      OBJ.top += OBJ.speed;
      if (OBJ.top >= obj->y2.n + CV(5.0)) {
         obj->state++;
      }
      break;

   case 2:
      obj->functionIndex = OBJF_NULL;
      break;
   }
}

void RenderCylinder(Cylinder *cylinder) {
   Object *face;
   s32 sideCt;
   s32 i;
   s16 increment;
   s16 startingTheta;
   s16 theta;
   POLY_FT4 *poly;

   face = Obj_GetUnused();
   face->d.sprite.semiTrans = cylinder->semiTrans;
   face->d.sprite.gfxIdx = cylinder->gfxIdx;
   if (cylinder->clut != 0) {
      face->d.sprite.clut = cylinder->clut;
   }

   sideCt = cylinder->sideCt;
   increment = DEG(360) / sideCt;

   if (cylinder->theta != 0) {
      startingTheta = cylinder->theta;
   } else {
      startingTheta = 0;
   }

   theta = startingTheta + increment;
   face->d.sprite.coords[0].x =
       cylinder->top.vx + (cylinder->topRadius * rcos(startingTheta) >> 12);
   face->d.sprite.coords[2].x =
       cylinder->bottom.vx + (cylinder->bottomRadius * rcos(startingTheta) >> 12);
   face->d.sprite.coords[0].z =
       cylinder->top.vz + (cylinder->topRadius * rsin(startingTheta) >> 12);
   face->d.sprite.coords[2].z =
       cylinder->bottom.vz + (cylinder->bottomRadius * rsin(startingTheta) >> 12);
   face->d.sprite.coords[0].y = face->d.sprite.coords[1].y = cylinder->top.vy;
   face->d.sprite.coords[2].y = face->d.sprite.coords[3].y = cylinder->bottom.vy;

   for (i = 0; i < sideCt; theta += increment, i++) {
      face->d.sprite.coords[1].x = cylinder->top.vx + (cylinder->topRadius * rcos(theta) >> 12);
      face->d.sprite.coords[3].x =
          cylinder->bottom.vx + (cylinder->bottomRadius * rcos(theta) >> 12);
      face->d.sprite.coords[1].z = cylinder->top.vz + (cylinder->topRadius * rsin(theta) >> 12);
      face->d.sprite.coords[3].z =
          cylinder->bottom.vz + (cylinder->bottomRadius * rsin(theta) >> 12);

      face->x1.n = face->d.sprite.coords[0].x;
      face->z1.n = face->d.sprite.coords[0].z;
      face->y1.n = face->d.sprite.coords[0].y;

      AddObjPrim3(gGraphicsPtr->ot, face);

      face->d.sprite.coords[0].x = face->d.sprite.coords[1].x;
      face->d.sprite.coords[2].x = face->d.sprite.coords[3].x;
      face->d.sprite.coords[0].z = face->d.sprite.coords[1].z;
      face->d.sprite.coords[2].z = face->d.sprite.coords[3].z;

      if (cylinder->useColor) {
         poly = &gGraphicsPtr->quads[gQuadIndex - 1];
         setRGB0(poly, cylinder->color.r, cylinder->color.g, cylinder->color.b);
      }
   }
}

void RenderCylinderWithRotatedTexture(Cylinder *cylinder) {
   Object *face;
   s32 sideCt;
   s32 i;
   s16 increment;
   s16 startingTheta;
   s16 theta;
   POLY_FT4 *poly;

   face = Obj_GetUnused();
   face->d.sprite.semiTrans = cylinder->semiTrans;
   face->d.sprite.gfxIdx = cylinder->gfxIdx;
   if (cylinder->clut != 0) {
      face->d.sprite.clut = cylinder->clut;
   }

   sideCt = cylinder->sideCt;
   increment = DEG(360) / sideCt;

   if (cylinder->theta != 0) {
      startingTheta = cylinder->theta;
   } else {
      startingTheta = 0;
   }

   theta = startingTheta + increment;
   face->d.sprite.coords[2].x =
       cylinder->top.vx + (cylinder->topRadius * rcos(startingTheta) >> 12);
   face->d.sprite.coords[3].x =
       cylinder->bottom.vx + (cylinder->bottomRadius * rcos(startingTheta) >> 12);
   face->d.sprite.coords[2].z =
       cylinder->top.vz + (cylinder->topRadius * rsin(startingTheta) >> 12);
   face->d.sprite.coords[3].z =
       cylinder->bottom.vz + (cylinder->bottomRadius * rsin(startingTheta) >> 12);
   face->d.sprite.coords[2].y = face->d.sprite.coords[0].y = cylinder->top.vy;
   face->d.sprite.coords[3].y = face->d.sprite.coords[1].y = cylinder->bottom.vy;

   for (i = 0; i < sideCt; theta += increment, i++) {
      face->d.sprite.coords[0].x = cylinder->top.vx + (cylinder->topRadius * rcos(theta) >> 12);
      face->d.sprite.coords[1].x =
          cylinder->bottom.vx + (cylinder->bottomRadius * rcos(theta) >> 12);
      face->d.sprite.coords[0].z = cylinder->top.vz + (cylinder->topRadius * rsin(theta) >> 12);
      face->d.sprite.coords[1].z =
          cylinder->bottom.vz + (cylinder->bottomRadius * rsin(theta) >> 12);

      face->x1.n = face->d.sprite.coords[1].x;
      face->z1.n = face->d.sprite.coords[1].z;
      face->y1.n = face->d.sprite.coords[1].y;

      AddObjPrim3(gGraphicsPtr->ot, face);

      face->d.sprite.coords[2].x = face->d.sprite.coords[0].x;
      face->d.sprite.coords[3].x = face->d.sprite.coords[1].x;
      face->d.sprite.coords[2].z = face->d.sprite.coords[0].z;
      face->d.sprite.coords[3].z = face->d.sprite.coords[1].z;

      if (cylinder->useColor) {
         poly = &gGraphicsPtr->quads[gQuadIndex - 1];
         setRGB0(poly, cylinder->color.r, cylinder->color.g, cylinder->color.b);
      }
   }
}

