/* Unit-hit effects and screen fades: fades (Objf070-075, Objf062), ring bursts (Objf076/077,
 * Objf130), damage/slay presentation (Objf078/079/131/149), the Objf132 engulf family with its
 * Objf133/137/141 emitters, melee impact dressing (Objf201-206, 213-215), Objf119 particles. */
#include "common.h"
#include "object.h"
#include "graphics.h"
#include "state.h"
#include "audio.h"

#undef OBJF
#define OBJF 119
void Objf119_RadialFxSprite(Object *obj) {
   // Spawned by: ?->182->183->119

   static s16 impactAnimData[18] = {4, GFX_IMPACT_1, 2, GFX_IMPACT_2, 2, GFX_IMPACT_3,
                                    2, GFX_IMPACT_4, 2, GFX_IMPACT_5, 2, GFX_IMPACT_6,
                                    2, GFX_IMPACT_7, 2, GFX_NULL,     0, GFX_NULL};

   static s16 explosionAnimData[26] = {4, GFX_EXPLOSION_1,  2, GFX_EXPLOSION_2,  2, GFX_EXPLOSION_3,
                                       2, GFX_EXPLOSION_4,  2, GFX_EXPLOSION_5,  2, GFX_EXPLOSION_6,
                                       2, GFX_EXPLOSION_7,  2, GFX_EXPLOSION_8,  2, GFX_EXPLOSION_9,
                                       2, GFX_EXPLOSION_10, 2, GFX_EXPLOSION_11, 2, GFX_NULL,
                                       0, GFX_NULL};

   static s16 smokeAnimData[24] = {4, GFX_PUFF_1, 2, GFX_PUFF_2,  2, GFX_PUFF_3, 2, GFX_PUFF_4,
                                   2, GFX_PUFF_5, 2, GFX_PUFF_6,  2, GFX_PUFF_7, 2, GFX_PUFF_8,
                                   2, GFX_PUFF_9, 2, GFX_PUFF_10, 2, GFX_NULL,   0, GFX_NULL};

   static s16 sparkleAnimData[18] = {4, GFX_SPARKLE_1, 2, GFX_SPARKLE_2, 2, GFX_SPARKLE_4,
                                     2, GFX_SPARKLE_5, 2, GFX_SPARKLE_4, 2, GFX_SPARKLE_3,
                                     2, GFX_SPARKLE_2, 2, GFX_NULL,      0, GFX_NULL};

   static s16 flameAnimData[20] = {4, GFX_FLAME_1, 2, GFX_FLAME_2, 2, GFX_FLAME_3, 2, GFX_FLAME_4,
                                   2, GFX_FLAME_5, 2, GFX_FLAME_6, 2, GFX_FLAME_7, 2, GFX_FLAME_8,
                                   2, GFX_NULL,    0, GFX_NULL};

   static s16 lightningAnimData[20] = {4, GFX_LIGHTNING_1, 2, GFX_LIGHTNING_2, 2, GFX_LIGHTNING_3,
                                       2, GFX_LIGHTNING_4, 2, GFX_LIGHTNING_5, 2, GFX_LIGHTNING_6,
                                       2, GFX_LIGHTNING_7, 2, GFX_LIGHTNING_8, 2, GFX_NULL,
                                       0, GFX_NULL};

   static s16 lightOrbAnimData[14] = {4, GFX_EXPLOSION_1, 2, GFX_EXPLOSION_2, 2, GFX_EXPLOSION_3,
                                      2, GFX_EXPLOSION_2, 2, GFX_EXPLOSION_1, 2, GFX_NULL,
                                      0, GFX_NULL};

   s32 rnd1, rnd2;
   Object *sprite;
   s16 *animData;

   switch (obj->state) {
   case 0:
      if (OBJ.boxIdx == 0) {
         OBJ.boxIdx = 4;
      }
      if (OBJ.clut == CLUT_NULL) {
         OBJ.clut = CLUT_BLUES;
      }

      rnd1 = rand();
      OBJ.theta = rnd1 % DEG(360);
      rnd2 = rand();
      OBJ.h = 0x80 - (rnd2 % 0x100);

      OBJ.sprite = sprite = Obj_GetUnused();
      sprite->functionIndex = OBJF_NOOP;

      switch (OBJ.type) {
      case 0:
         animData = impactAnimData;
         animData[0] = OBJ.boxIdx;
         sprite->d.sprite.animData = animData;
         break;
      case 1:
         animData = explosionAnimData;
         animData[0] = OBJ.boxIdx;
         sprite->d.sprite.animData = animData;
         break;
      case 2:
         animData = smokeAnimData;
         animData[0] = OBJ.boxIdx;
         sprite->d.sprite.animData = animData;
         break;
      case 3:
         animData = sparkleAnimData;
         animData[0] = OBJ.boxIdx;
         sprite->d.sprite.animData = animData;
         break;
      case 4:
         animData = flameAnimData;
         animData[0] = OBJ.boxIdx;
         sprite->d.sprite.animData = animData;
         break;
      case 5:
         animData = lightningAnimData;
         animData[0] = OBJ.boxIdx;
         sprite->d.sprite.animData = animData;
         break;
      case 6:
         animData = lightOrbAnimData;
         animData[0] = OBJ.boxIdx;
         sprite->d.sprite.animData = animData;
         break;
      } // switch (type) (via state:0)

      sprite->d.sprite.animInitialized = 0;
      sprite->d.sprite.clut = OBJ.clut;
      sprite->d.sprite.semiTrans = OBJ.semiTrans;
      obj->state++;
      break;

   case 1:
      if (OBJ.timer == 0) {
         obj->state++;
      } else {
         --OBJ.timer;
      }
      break;

   case 2:
      sprite = OBJ.sprite;
      sprite->x1.n = obj->x1.n + (OBJ.radius * rcos(OBJ.theta) >> 12);
      sprite->z1.n = obj->z1.n + (OBJ.radius * rsin(OBJ.theta) >> 12);
      sprite->y1.n = obj->y1.n + OBJ.h;
      UpdateObjAnimation(sprite);
      if (sprite->d.sprite.animFinished) {
         if (!OBJ.once) {
            obj->state = 0;
            sprite->functionIndex = OBJF_NULL;
         } else {
            obj->functionIndex = OBJF_NULL;
            sprite->functionIndex = OBJF_NULL;
         }
      } else {
         AddObjPrim6(gGraphicsPtr->ot, sprite, 0);
      }
      break;
   }
}

#undef OBJF
#define OBJF 062
void Objf062_StretchWarpSprite(Object *obj) {
   Object *unitSprite, *fxSprite;
   Quad quad;
   s32 i;

   switch (obj->state) {
   case 0:
      if (OBJ.sprite == NULL) {
         // If original sprite was not provided, retrieve by location
         OBJ.sprite = unitSprite = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
      } else {
         unitSprite = OBJ.sprite;
      }
      unitSprite->d.sprite.hidden = 1;
      if (OBJ.speed == 0) {
         OBJ.speed = DEG(270) / 0x10;
      }
      if (obj->mem != 0) {
         // Reversed (warp in)
         OBJ.phase = DEG(270);
      }
      obj->state++;

   // fallthrough
   case 1:
      unitSprite = OBJ.sprite;
      fxSprite = Obj_GetUnused();
      CopyObject(unitSprite, fxSprite);
      fxSprite->d.sprite.hidden = 0;
      for (i = 0; i < 4; i++) {
         quad[i].vx = gQuad_800fe53c[i].vx;
         quad[i].vz = gQuad_800fe53c[i].vz;
         quad[i].vy = gQuad_800fe53c[i].vy;
      }
      for (i = 0; i < 4; i++) {
         gQuad_800fe53c[i].vx = quad[i].vx * (rsin(OBJ.phase) + ONE) / ONE;
         gQuad_800fe53c[i].vz = quad[i].vz * (rsin(OBJ.phase) + ONE) / ONE;

         if (OBJ.phase <= DEG(180)) {
            gQuad_800fe53c[i].vy =
                quad[i].vy * (rsin(OBJ.phase + DEG(180)) / 2 + (ONE / 2)) / (ONE / 2);
         } else {
            gQuad_800fe53c[i].vy = quad[i].vy * (rsin(OBJ.phase + DEG(180)) * 3 + ONE) / ONE;
         }
      }
      RenderUnitSprite(gGraphicsPtr->ot, fxSprite, 0);
      for (i = 0; i < 4; i++) {
         gQuad_800fe53c[i].vx = quad[i].vx;
         gQuad_800fe53c[i].vz = quad[i].vz;
         gQuad_800fe53c[i].vy = quad[i].vy;
      }
      if (obj->mem == 0) {
         OBJ.phase += OBJ.speed;
         if (OBJ.phase >= DEG(270)) {
            obj->functionIndex = OBJF_NULL;
         }
      } else {
         OBJ.phase -= OBJ.speed;
         if (OBJ.phase <= 0) {
            unitSprite->d.sprite.hidden = 0;
            obj->functionIndex = OBJF_NULL;
         }
      }
      fxSprite->functionIndex = OBJF_NULL;
      break;
   }
}

#undef OBJF
#define OBJF 074
void Objf074_FadeInSprite(Object *obj) {
   Object *unitSprite, *fxSprite1, *fxSprite2;
   POLY_FT4 *poly;

   switch (obj->state) {
   case 0:
      OBJ.sprite = unitSprite = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
      unitSprite->d.sprite.hidden = 1;
      if (OBJ.speed == 0) {
         OBJ.speed = 4;
      }
      obj->state++;

   // fallthrough
   case 1:
      unitSprite = OBJ.sprite;
      fxSprite1 = Obj_GetUnused();
      CopyObject(unitSprite, fxSprite1);
      fxSprite1->d.sprite.hidden = 0;
      fxSprite1->x1.n = unitSprite->x1.n;
      fxSprite1->z1.n = unitSprite->z1.n;
      fxSprite1->y1.n = unitSprite->y1.n;
      fxSprite1->d.sprite.semiTrans = 2;

      fxSprite2 = Obj_GetUnused();
      CopyObject(fxSprite1, fxSprite2);
      fxSprite2->d.sprite.semiTrans = 3;
      fxSprite2->d.sprite.clut = CLUT_MASK;

      RenderUnitSprite(gGraphicsPtr->ot, fxSprite1, 0);
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      setRGB0(poly, OBJ.fade, OBJ.fade, OBJ.fade);

      RenderUnitSprite(gGraphicsPtr->ot, fxSprite2, 0);
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      setRGB0(poly, OBJ.fade, OBJ.fade, OBJ.fade);

      OBJ.fade += OBJ.speed;
      if (OBJ.fade > 0x80) {
         unitSprite->d.sprite.hidden = 0;
         obj->functionIndex = OBJF_NULL;
      }
      fxSprite1->functionIndex = OBJF_NULL;
      fxSprite2->functionIndex = OBJF_NULL;
      break;
   }
}

#undef OBJF
#define OBJF 075
void Objf075_FadeOutSprite(Object *obj) {
   Object *unitSprite, *fxSprite1, *fxSprite2;
   POLY_FT4 *poly;

   switch (obj->state) {
   case 0:
      OBJ.sprite = unitSprite = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
      unitSprite->d.sprite.hidden = 1;
      OBJ.fade = 0x80;
      if (OBJ.speed == 0) {
         OBJ.speed = 4;
      }
      obj->state++;

   // fallthrough
   case 1:
      unitSprite = OBJ.sprite;
      fxSprite1 = Obj_GetUnused();
      CopyObject(unitSprite, fxSprite1);
      fxSprite1->d.sprite.hidden = 0;
      fxSprite1->x1.n = unitSprite->x1.n;
      fxSprite1->z1.n = unitSprite->z1.n;
      fxSprite1->y1.n = unitSprite->y1.n;
      fxSprite1->d.sprite.semiTrans = 2;

      fxSprite2 = Obj_GetUnused();
      CopyObject(fxSprite1, fxSprite2);
      fxSprite2->d.sprite.semiTrans = 0;

      RenderUnitSprite(gGraphicsPtr->ot, fxSprite1, 0);
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      setRGB0(poly, OBJ.fade, OBJ.fade, OBJ.fade);

      OBJ.fade -= OBJ.speed;
      if (OBJ.fade < 0) {
         obj->functionIndex = OBJF_NULL;
      }
      fxSprite1->functionIndex = OBJF_NULL;
      fxSprite2->functionIndex = OBJF_NULL;
      break;
   }
}

#undef OBJF
#define OBJF 072
void Objf072_FadeFromWhite(Object *obj) {
   POLY_FT4 *poly;

   switch (obj->state) {
   case 0:
      OBJ.fade = 0x80;
      if (OBJ.speed == 0) {
         OBJ.speed = 4;
      }
      obj->state++;

   // fallthrough
   case 1:
      poly = &gGraphicsPtr->quads[gQuadIndex];
#ifdef PC_PORT
      /* bugreport-02: type the textured quad explicitly. The fx code omits SetPolyFT4 and relies on a
       * stale POLY_FT4 slot code (0x2c on PSX); our backend's slot is stale-POLY_F4 -> texture dropped
       * (rendered as a flat semi-transparent fill). Same gated fix as the US tree. */
      SetPolyFT4(poly);
#endif
      SetSemiTrans(poly, 1);
      poly->tpage = gTPageIds[32 + gGfxTPageCells[GFX_COLOR_15]];
      poly->clut = gGfxClutIds[GFX_COLOR_15];
      setUVWH(poly, gGfxSubTextures[GFX_COLOR_15][0], gGfxSubTextures[GFX_COLOR_15][1],
              gGfxSubTextures[GFX_COLOR_15][2], gGfxSubTextures[GFX_COLOR_15][3]);
      setXYWH(poly, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
      setRGB0(poly, OBJ.fade, OBJ.fade, OBJ.fade);
      AddPrim(&gGraphicsPtr->ot[OT_SIZE - 1], poly);
      gQuadIndex++;

      OBJ.fade -= OBJ.speed;
      if (OBJ.fade < 0) {
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 073
void Objf073_FadeToWhite(Object *obj) {
   POLY_FT4 *poly;

   switch (obj->state) {
   case 0:
      if (OBJ.speed == 0) {
         OBJ.speed = 4;
      }
      obj->state++;

   // fallthrough
   case 1:
      poly = &gGraphicsPtr->quads[gQuadIndex];
#ifdef PC_PORT
      /* bugreport-02: type the textured quad explicitly. The fx code omits SetPolyFT4 and relies on a
       * stale POLY_FT4 slot code (0x2c on PSX); our backend's slot is stale-POLY_F4 -> texture dropped
       * (rendered as a flat semi-transparent fill). Same gated fix as the US tree. */
      SetPolyFT4(poly);
#endif
      SetSemiTrans(poly, 1);
      poly->tpage = gTPageIds[32 + gGfxTPageCells[GFX_COLOR_15]];
      poly->clut = gGfxClutIds[GFX_COLOR_15];
      setUVWH(poly, gGfxSubTextures[GFX_COLOR_15][0], gGfxSubTextures[GFX_COLOR_15][1],
              gGfxSubTextures[GFX_COLOR_15][2], gGfxSubTextures[GFX_COLOR_15][3]);
      setXYWH(poly, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
      setRGB0(poly, OBJ.fade, OBJ.fade, OBJ.fade);
      AddPrim(&gGraphicsPtr->ot[OT_SIZE - 1], poly);
      gQuadIndex++;

      OBJ.fade += OBJ.speed;
      if (OBJ.fade > 0x80) {
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 070
void Objf070_FadeFromBlack(Object *obj) {
   POLY_FT4 *poly;

   switch (obj->state) {
   case 0:
      OBJ.fade = 0x80;
      if (OBJ.speed == 0) {
         OBJ.speed = 4;
      }
      obj->state++;

   // fallthrough
   case 1:
      poly = &gGraphicsPtr->quads[gQuadIndex];
#ifdef PC_PORT
      /* bugreport-02: type the textured quad explicitly. The fx code omits SetPolyFT4 and relies on a
       * stale POLY_FT4 slot code (0x2c on PSX); our backend's slot is stale-POLY_F4 -> texture dropped
       * (rendered as a flat semi-transparent fill). Same gated fix as the US tree. */
      SetPolyFT4(poly);
#endif
      SetSemiTrans(poly, 1);
      poly->tpage = gTPageIds[64 + gGfxTPageCells[GFX_COLOR_15]];
      poly->clut = gGfxClutIds[GFX_COLOR_15];
      setUVWH(poly, gGfxSubTextures[GFX_COLOR_15][0], gGfxSubTextures[GFX_COLOR_15][1],
              gGfxSubTextures[GFX_COLOR_15][2], gGfxSubTextures[GFX_COLOR_15][3]);
      setXYWH(poly, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
      setRGB0(poly, OBJ.fade, OBJ.fade, OBJ.fade);
      AddPrim(&gGraphicsPtr->ot[OT_SIZE - 2], poly);
      gQuadIndex++;

      OBJ.fade -= OBJ.speed;
      if (OBJ.fade < 0) {
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 071
void Objf071_FadeToBlack(Object *obj) {
   POLY_FT4 *poly;

   switch (obj->state) {
   case 0:
      if (OBJ.speed == 0) {
         OBJ.speed = 4;
      }
      obj->state++;

   // fallthrough
   case 1:
      poly = &gGraphicsPtr->quads[gQuadIndex];
#ifdef PC_PORT
      /* bugreport-02: type the textured quad explicitly. The fx code omits SetPolyFT4 and relies on a
       * stale POLY_FT4 slot code (0x2c on PSX); our backend's slot is stale-POLY_F4 -> texture dropped
       * (rendered as a flat semi-transparent fill). Same gated fix as the US tree. */
      SetPolyFT4(poly);
#endif
      SetSemiTrans(poly, 1);
      poly->tpage = gTPageIds[64 + gGfxTPageCells[GFX_COLOR_15]];
      poly->clut = gGfxClutIds[GFX_COLOR_15];
      setUVWH(poly, gGfxSubTextures[GFX_COLOR_15][0], gGfxSubTextures[GFX_COLOR_15][1],
              gGfxSubTextures[GFX_COLOR_15][2], gGfxSubTextures[GFX_COLOR_15][3]);
      setXYWH(poly, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
      setRGB0(poly, OBJ.fade, OBJ.fade, OBJ.fade);
      AddPrim(&gGraphicsPtr->ot[OT_SIZE - 2], poly);
      gQuadIndex++;

      OBJ.fade += OBJ.speed;
      if (OBJ.fade > 0x80) {
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 076
void Objf076_RingBurstOnUnit(Object *obj) {
   Object *unitSprite, *circle;

   switch (obj->state) {
   case 0:
      unitSprite = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
      obj->x1.n = unitSprite->x1.n;
      obj->z1.n = unitSprite->z1.n;
      obj->y1.n = unitSprite->y1.n + CV(0.75);

      switch (unitSprite->d.sprite.direction) {
      case ANGLE_WEST:
         OBJ.dirSign = -1;
         OBJ.axis = 1;
         obj->x1.n += CV(0.5625);
         break;
      case ANGLE_EAST:
         OBJ.dirSign = 1;
         OBJ.axis = 1;
         obj->x1.n -= CV(0.5625);
         break;
      case ANGLE_SOUTH:
         OBJ.dirSign = -1;
         OBJ.axis = 0;
         obj->z1.n += CV(0.5625);
         break;
      case ANGLE_NORTH:
         OBJ.dirSign = 1;
         OBJ.axis = 0;
         obj->z1.n -= CV(0.5625);
         break;
      }

      obj->state++;

   // fallthrough
   case 1:
      circle = Obj_GetUnused();
      circle->functionIndex = OBJF_EXPANDING_RING;
      circle->x1.n = obj->x1.n;
      circle->z1.n = obj->z1.n;
      circle->y1.n = obj->y1.n;
      circle->d.objf077.axis = OBJ.axis;
      circle->d.objf077.dirSign = OBJ.dirSign;
      if (++OBJ.timer == 3) {
         obj->state++;
         OBJ.timer = 0;
      }
      break;

   case 2:
      if (++OBJ.timer >= 16) {
         obj->functionIndex = OBJF_NULL;
         gSignal3 = 1;
      }
      break;
   }
}

#undef OBJF
#define OBJF 077
void Objf077_ExpandingRing(Object *obj) {
   Object *ringSprite;
   s16 a, b;
   s16 fade;
   POLY_FT4 *poly;

   switch (obj->state) {
   case 0:
      OBJ.fade = 0xff;
      obj->state++;

   // fallthrough
   case 1:
      ringSprite = Obj_GetUnused();
      ringSprite->functionIndex = OBJF_NOOP;
      ringSprite->d.sprite.gfxIdx = GFX_RING;

      a = 0xe0 * rsin(OBJ.angle) / ONE;
      b = 0xe0 * rsin(OBJ.angle * 2) / ONE;
      OBJ.angle += 0x80;

      fade = OBJ.fade;
      if (fade < 0) {
         fade = 0;
      }

      ringSprite->d.sprite.coords[0].x = obj->x1.n;
      ringSprite->d.sprite.coords[1].x = obj->x1.n;
      ringSprite->d.sprite.coords[2].x = obj->x1.n;
      ringSprite->d.sprite.coords[3].x = obj->x1.n;
      ringSprite->d.sprite.coords[0].z = obj->z1.n;
      ringSprite->d.sprite.coords[1].z = obj->z1.n;
      ringSprite->d.sprite.coords[2].z = obj->z1.n;
      ringSprite->d.sprite.coords[3].z = obj->z1.n;

      if (OBJ.axis == 0) {
         ringSprite->d.sprite.coords[0].x -= b;
         ringSprite->d.sprite.coords[1].x += b;
         ringSprite->d.sprite.coords[2].x -= b;
         ringSprite->d.sprite.coords[3].x += b;

         ringSprite->d.sprite.coords[0].z += (a * OBJ.dirSign);
         ringSprite->d.sprite.coords[1].z = ringSprite->d.sprite.coords[0].z;
         ringSprite->d.sprite.coords[2].z = ringSprite->d.sprite.coords[0].z;
         ringSprite->d.sprite.coords[3].z = ringSprite->d.sprite.coords[0].z;
      } else {
         ringSprite->d.sprite.coords[0].z -= b;
         ringSprite->d.sprite.coords[1].z += b;
         ringSprite->d.sprite.coords[2].z -= b;
         ringSprite->d.sprite.coords[3].z += b;

         ringSprite->d.sprite.coords[0].x += (a * OBJ.dirSign);
         ringSprite->d.sprite.coords[1].x = ringSprite->d.sprite.coords[0].x;
         ringSprite->d.sprite.coords[2].x = ringSprite->d.sprite.coords[0].x;
         ringSprite->d.sprite.coords[3].x = ringSprite->d.sprite.coords[0].x;
      }

      ringSprite->d.sprite.coords[0].y = obj->y1.n - b;
      ringSprite->d.sprite.coords[1].y = ringSprite->d.sprite.coords[0].y;
      ringSprite->d.sprite.coords[2].y = obj->y1.n + b;
      ringSprite->d.sprite.coords[3].y = ringSprite->d.sprite.coords[2].y;

      ringSprite->d.sprite.clut = CLUT_BLUES;
      ringSprite->d.sprite.semiTrans = 2;
      AddObjPrim4(gGraphicsPtr->ot, ringSprite);
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      setRGB0(poly, fade, fade, fade);

      ringSprite->d.sprite.clut = CLUT_MASK;
      ringSprite->d.sprite.semiTrans = 3;
      AddObjPrim3(gGraphicsPtr->ot, ringSprite);
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      setRGB0(poly, fade, fade, fade);

      OBJ.fade -= 24;
      if (OBJ.fade <= 0) {
         obj->functionIndex = OBJF_NULL;
      }
      ringSprite->functionIndex = OBJF_NULL;
      break;
   }
}

#undef OBJF
#define OBJF 078
void Objf078_Damage_FX2(Object *obj) {
   Object *newObj;

   switch (obj->state) {
   case 0:
      newObj = Obj_GetUnused();
      newObj->functionIndex = OBJF_BOUNCE_ZOOM;
      newObj = Obj_GetUnused();
      newObj->functionIndex = OBJF_DISPLAY_DAMAGE_2;
      newObj->x1.s.hi = obj->x1.s.hi;
      newObj->z1.s.hi = obj->z1.s.hi;
      OBJ.timer = 30;
      obj->state++;

   // fallthrough
   case 1:
      if (--OBJ.timer == -1) {
         obj->functionIndex = OBJF_NULL;
         gSignal3 = 1;
      }
      break;
   }
}

#undef OBJF
#define OBJF 079
void Objf079_Slay_FX3(Object *obj) {
   Object *newObj;

   switch (obj->state) {
   case 0:
      newObj = Obj_GetUnused();
      newObj->functionIndex = OBJF_BOUNCE_ZOOM;
      newObj = Obj_GetUnused();
      newObj->functionIndex = OBJF_STRETCH_WARP_SPRITE;
      newObj->x1.s.hi = obj->x1.s.hi;
      newObj->z1.s.hi = obj->z1.s.hi;
      newObj = Obj_GetUnused();
      newObj->functionIndex = OBJF_DISPLAY_DAMAGE_2;
      newObj->x1.s.hi = obj->x1.s.hi;
      newObj->z1.s.hi = obj->z1.s.hi;
      OBJ.timer = 30;
      obj->state++;

   // fallthrough
   case 1:
      if (--OBJ.timer == -1) {
         obj->functionIndex = OBJF_NULL;
         gSignal3 = 1;
      }
      break;
   }
}

#undef OBJF
#define OBJF Unk8006183c
/* Unreachable: absent from gObjFunctionPointers[], so no functionIndex dispatches here. It
 * hides the real unit sprite and redraws it plus an enlarged translucent tinted copy (a
 * selection/charge aura) until state 99 restores the sprite. Keeps its address name. */
void Objf_Unk_8006183c(Object *obj) {
   Object *unitSprite, *fxSprite;
   Quad quad;
   s32 i;

   switch (obj->state) {
   case 0:
      OBJ.unitSprite = unitSprite = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
      unitSprite->d.sprite.hidden = 1;
      if (OBJ.clut == CLUT_NULL) {
         OBJ.clut = CLUT_BLUES;
      }
      if (OBJ.scale == 0) {
         OBJ.scale = 110;
      }
      obj->state++;

   // fallthrough
   case 1:
      unitSprite = OBJ.unitSprite;
      fxSprite = Obj_GetUnused();
      CopyObject(unitSprite, fxSprite);
      fxSprite->d.sprite.hidden = 0;
      fxSprite->x1.n = unitSprite->x1.n;
      fxSprite->z1.n = unitSprite->z1.n;
      fxSprite->y1.n = unitSprite->y1.n;
      fxSprite->d.sprite.semiTrans = 0;
      fxSprite->d.sprite.clut = CLUT_NULL;
      RenderUnitSprite(gGraphicsPtr->ot, fxSprite, 0);

      switch ((gCameraRotation.vy & 0xfff) >> 10) {
      case CAM_DIR_SOUTH:
         fxSprite->z1.n += CV(0.0625);
         fxSprite->x1.n -= CV(0.0625);
         break;
      case CAM_DIR_EAST:
         fxSprite->x1.n -= CV(0.0625);
         fxSprite->z1.n -= CV(0.0625);
         break;
      case CAM_DIR_NORTH:
         fxSprite->z1.n -= CV(0.0625);
         fxSprite->x1.n += CV(0.0625);
         break;
      case CAM_DIR_WEST:
         fxSprite->z1.n += CV(0.0625);
         fxSprite->x1.n += CV(0.0625);
         break;
      }

      for (i = 0; i < 4; i++) {
         quad[i].vx = gQuad_800fe53c[i].vx;
         quad[i].vz = gQuad_800fe53c[i].vz;
         quad[i].vy = gQuad_800fe53c[i].vy;
      }
      for (i = 0; i < 4; i++) {
         gQuad_800fe53c[i].vx *= OBJ.scale;
         gQuad_800fe53c[i].vz *= OBJ.scale;
         gQuad_800fe53c[i].vy *= OBJ.scale;
      }
      for (i = 0; i < 4; i++) {
         gQuad_800fe53c[i].vx /= 100;
         gQuad_800fe53c[i].vz /= 100;
         gQuad_800fe53c[i].vy /= 100;
      }

      fxSprite->y1.n -= (abs(gQuad_800fe53c[0].vy) - abs(quad[0].vy));
      fxSprite->d.sprite.semiTrans = 1;
      fxSprite->d.sprite.clut = OBJ.clut;
      RenderUnitSprite(gGraphicsPtr->ot, fxSprite, 0);

      for (i = 0; i < 4; i++) {
         gQuad_800fe53c[i].vx = quad[i].vx;
         gQuad_800fe53c[i].vz = quad[i].vz;
         gQuad_800fe53c[i].vy = quad[i].vy;
      }

      fxSprite->functionIndex = OBJF_NULL;
      break;

   case 99:
      obj->functionIndex = OBJF_NULL;
      unitSprite = OBJ.unitSprite;
      unitSprite->d.sprite.hidden = 0;
      break;
   }
}

#undef OBJF
#define OBJF 802
void Objf802_ExplosionRingSprite(Object *obj) {
   static s16 explosionAnimData[26] = {7, GFX_EXPLOSION_1,  2, GFX_EXPLOSION_2,  2, GFX_EXPLOSION_3,
                                       2, GFX_EXPLOSION_4,  2, GFX_EXPLOSION_5,  2, GFX_EXPLOSION_6,
                                       2, GFX_EXPLOSION_7,  2, GFX_EXPLOSION_8,  2, GFX_EXPLOSION_9,
                                       2, GFX_EXPLOSION_10, 2, GFX_EXPLOSION_11, 2, GFX_NULL,
                                       0, GFX_NULL};

   Object *sprite;
   s32 rnd1, rnd2;
   s16 a;

   gQuad_800fe63c[0].vx = -12;
   gQuad_800fe63c[0].vy = -24;
   gQuad_800fe63c[0].vz = 0;
   gQuad_800fe63c[1].vx = 12;
   gQuad_800fe63c[1].vy = -24;
   gQuad_800fe63c[1].vz = 0;
   gQuad_800fe63c[2].vx = -12;
   gQuad_800fe63c[2].vy = 0;
   gQuad_800fe63c[2].vz = 0;
   gQuad_800fe63c[3].vx = 12;
   gQuad_800fe63c[3].vy = 0;
   gQuad_800fe63c[3].vz = 0;

   switch (obj->state) {
   case 0:
      rnd1 = rand();
      OBJ.theta = rnd1 % DEG(360);
      rnd2 = rand();
      OBJ.h = (rnd2 % CV(0.375)) - CV(0.8125);

      OBJ.sprite = sprite = Obj_GetUnused();
      sprite->functionIndex = OBJF_NOOP;
      sprite->d.sprite.animData = explosionAnimData;
      sprite->d.sprite.animInitialized = 0;
      sprite->d.sprite.clut = OBJ.clut;
      sprite->d.sprite.semiTrans = OBJ.semiTrans;
      obj->state++;

   // fallthrough
   case 1:
      if (--OBJ.timer <= 0) {
         obj->state++;
      }
      break;

   case 2:
      sprite = OBJ.sprite;
      a = OBJ.theta;

      gQuad_800fe63c[0].vy = -6 - ((OBJ.maxHeight - 6) * abs(rsin(OBJ.pulsePhase)) / ONE);
      gQuad_800fe63c[1].vy = gQuad_800fe63c[0].vy;

      gQuad_800fe63c[0].vx = -3 - ((OBJ.maxHeight / 2 - 3) * abs(rsin(OBJ.pulsePhase)) / ONE);
      gQuad_800fe63c[2].vx = gQuad_800fe63c[0].vx;
      gQuad_800fe63c[1].vx = -gQuad_800fe63c[0].vx;
      gQuad_800fe63c[3].vx = -gQuad_800fe63c[0].vx;

      OBJ.pulsePhase += OBJ.pulseSpeed;

      sprite->x1.n = obj->x1.n + (OBJ.radius * rcos(a) / ONE);
      sprite->z1.n = obj->z1.n + (OBJ.radius * rsin(a) / ONE);
      sprite->y1.n = obj->y1.n + OBJ.h;

      UpdateObjAnimation(sprite);
      AddObjPrim6(gGraphicsPtr->ot, sprite, 0);
      if (sprite->d.sprite.animFinished) {
         obj->functionIndex = OBJF_NULL;
         sprite->functionIndex = OBJF_NULL;
      }
      break;
   }
}

// Definition order is swapped, so these must not be function-scoped. I guess the others shouldn't
// be either, but I'm fine with whatever works, for now.
static s16 sFlameAnimData_800fead4[20] = {
    7, GFX_FLAME_1, 1, GFX_FLAME_2, 1, GFX_FLAME_3, 1, GFX_FLAME_4, 1, GFX_FLAME_5,
    1, GFX_FLAME_6, 1, GFX_FLAME_7, 1, GFX_FLAME_8, 1, GFX_NULL,    0, GFX_NULL};

static s16 sLightningAnimData_800feafc[20] = {
    7, GFX_LIGHTNING_1, 2, GFX_LIGHTNING_2, 2, GFX_LIGHTNING_3, 2, GFX_LIGHTNING_4,
    2, GFX_LIGHTNING_5, 2, GFX_LIGHTNING_6, 2, GFX_LIGHTNING_7, 2, GFX_LIGHTNING_8,
    2, GFX_NULL,        0, GFX_NULL};

#undef OBJF
#define OBJF 803
void Objf803_LightningRingSprite(Object *obj) {
   Object *sprite;
   s32 rnd1, rnd2;

   gQuad_800fe63c[0].vx = -12;
   gQuad_800fe63c[0].vy = -24;
   gQuad_800fe63c[0].vz = 0;
   gQuad_800fe63c[1].vx = 12;
   gQuad_800fe63c[1].vy = -24;
   gQuad_800fe63c[1].vz = 0;
   gQuad_800fe63c[2].vx = -12;
   gQuad_800fe63c[2].vy = 0;
   gQuad_800fe63c[2].vz = 0;
   gQuad_800fe63c[3].vx = 12;
   gQuad_800fe63c[3].vy = 0;
   gQuad_800fe63c[3].vz = 0;

   switch (obj->state) {
   case 0:
      rnd1 = rand();
      OBJ.theta = rnd1 % DEG(360);
      rnd2 = rand();
      OBJ.h = (rnd2 % 0x20) - 0xd0;

      OBJ.sprite = sprite = Obj_GetUnused();
      sprite->functionIndex = OBJF_NOOP;
      sprite->d.sprite.animData = sLightningAnimData_800feafc;
      sprite->d.sprite.animInitialized = 0;
      sprite->d.sprite.clut = OBJ.clut;
      sprite->d.sprite.semiTrans = OBJ.semiTrans;
      obj->state++;

   // fallthrough
   case 1:
      if (--OBJ.timer <= 0) {
         obj->state++;
      }
      break;

   case 2:
      sprite = OBJ.sprite;

      gQuad_800fe63c[0].vy = -12 - ((OBJ.maxHeight - 12) * abs(rsin(OBJ.pulsePhase)) / ONE);
      gQuad_800fe63c[1].vy = gQuad_800fe63c[0].vy;

      OBJ.pulsePhase += OBJ.pulseSpeed;

      sprite->x1.n = obj->x1.n + (OBJ.radius * rcos(OBJ.theta) / ONE);
      sprite->z1.n = obj->z1.n + (OBJ.radius * rsin(OBJ.theta) / ONE);
      sprite->y1.n = obj->y1.n + OBJ.h;

      UpdateObjAnimation(sprite);
      AddObjPrim6(gGraphicsPtr->ot, sprite, 0);
      if (sprite->d.sprite.animFinished) {
         obj->functionIndex = OBJF_NULL;
         sprite->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 801
void Objf801_FlameRingSprite(Object *obj) {
   Object *sprite;
   s32 rnd1, rnd2;

   gQuad_800fe63c[0].vx = -12;
   gQuad_800fe63c[0].vy = -32;
   gQuad_800fe63c[0].vz = 0;
   gQuad_800fe63c[1].vx = 12;
   gQuad_800fe63c[1].vy = -32;
   gQuad_800fe63c[1].vz = 0;
   gQuad_800fe63c[2].vx = -12;
   gQuad_800fe63c[2].vy = 0;
   gQuad_800fe63c[2].vz = 0;
   gQuad_800fe63c[3].vx = 12;
   gQuad_800fe63c[3].vy = 0;
   gQuad_800fe63c[3].vz = 0;

   Noop_DebugPrintValue(10, 10, obj->state);

   switch (obj->state) {
   case 0:
      rnd1 = rand();
      OBJ.theta = rnd1 % DEG(360);
      rnd2 = rand();
      OBJ.h = (rnd2 % 0x46) - 0xd0;

      sprite = Obj_GetUnused();
      sprite->functionIndex = OBJF_NOOP;
      OBJ.sprite = sprite;
      sprite->d.sprite.animData = sFlameAnimData_800fead4;
      sprite->d.sprite.clut = CLUT_NULL;
      sprite->d.sprite.semiTrans = 2;
      obj->state++;

   // fallthrough
   case 1:
      if (--OBJ.timer <= 0) {
         obj->state++;
      }
      break;

   case 2:
      sprite = OBJ.sprite;

      gQuad_800fe63c[0].vy = -16 - ((OBJ.maxHeight - 16) * abs(rsin(OBJ.pulsePhase)) / ONE);
      gQuad_800fe63c[1].vy = gQuad_800fe63c[0].vy;

      OBJ.pulsePhase += OBJ.pulseSpeed;

      sprite->x1.n = obj->x1.n + (OBJ.radius * rcos(OBJ.theta) >> 12);
      sprite->z1.n = obj->z1.n + (OBJ.radius * rsin(OBJ.theta) >> 12);
      sprite->y1.n = obj->y1.n + OBJ.h;

      UpdateObjAnimation(sprite);
      AddObjPrim6(gGraphicsPtr->ot, sprite, 0);
      if (sprite->d.sprite.animFinished) {
         obj->functionIndex = OBJF_NULL;
         sprite->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 133
void Objf133_FlameRingEmitter(Object *obj) {
   Object *newObj;

   switch (obj->state) {
   case 0:
      newObj = CreatePositionedObj(obj, OBJF_FLAME_RING_SPRITE);
      newObj->d.objf801.radius = OBJ.radius;
      newObj->d.objf801.pulseSpeed = OBJ.pulseSpeed;
      newObj->d.objf801.maxHeight = OBJ.maxHeight;
      newObj->d.objf801.semiTrans = OBJ.semiTrans;
      newObj->d.objf801.clut = OBJ.clut;
      newObj->d.objf801.timer = OBJ.timer;
      obj->mem--;
      if (obj->mem <= 0) {
         obj->state++;
      }
      break;
   case 1:
      obj->functionIndex = OBJF_NULL;
      break;
   }
}

#undef OBJF
#define OBJF 141
void Objf141_ExplosionRingEmitter(Object *obj) {
   Object *newObj;

   switch (obj->state) {
   case 0:
      newObj = CreatePositionedObj(obj, OBJF_EXPLOSION_RING_SPRITE);
      newObj->d.objf802.radius = OBJ.radius;
      newObj->d.objf802.pulseSpeed = OBJ.pulseSpeed;
      newObj->d.objf802.maxHeight = OBJ.maxHeight;
      newObj->d.objf802.semiTrans = OBJ.semiTrans;
      newObj->d.objf802.clut = OBJ.clut;
      newObj->d.objf802.timer = OBJ.timer;
      obj->mem--;
      if (obj->mem <= 0) {
         obj->state++;
      }
      break;
   case 1:
      obj->functionIndex = OBJF_NULL;
      break;
   }
}

#undef OBJF
#define OBJF 137
void Objf137_LightningRingEmitter(Object *obj) {
   Object *newObj;

   switch (obj->state) {
   case 0:
      newObj = CreatePositionedObj(obj, OBJF_LIGHTNING_RING_SPRITE);
      newObj->d.objf803.radius = OBJ.radius;
      newObj->d.objf803.pulseSpeed = OBJ.pulseSpeed;
      newObj->d.objf803.maxHeight = OBJ.maxHeight;
      newObj->d.objf803.semiTrans = OBJ.semiTrans;
      newObj->d.objf803.clut = OBJ.clut;
      newObj->d.objf803.timer = OBJ.timer;
      obj->mem--;
      if (obj->mem <= 0) {
         obj->state++;
      }
      break;
   case 1:
      obj->functionIndex = OBJF_NULL;
      break;
   }
}

#undef OBJF
#define OBJF 132
/* One handler for the engulf family: 132/134 flame, 136/138 and 799/800 lightning, 140/142
 * explosion; the first of each pair restores the sprite, the second stretch-fades it away.
 * Spawners pick OBJF_ENGULF_<element>_DAMAGE + endingFxType; 135/139 are NULL slots. */
void Objf132_EngulfUnit(Object *obj) {
   static struct {
      s16 clut, objf, radius, to_x28, to_x2a;
   } fxInfo[3] = {{3, OBJF_FLAME_RING_EMITTER, 0x70, 0x60, 0x100},
                  {8, OBJF_LIGHTNING_RING_EMITTER, 0x80, 0x50, 0x100},
                  {3, OBJF_EXPLOSION_RING_EMITTER, 0x70, 0x20, 0xaa}};

   Object *unitSprite, *fxObj1, *fxObj2, *dmgObj;
   POLY_FT4 *poly;
   s32 i, fadeSpeed, stretchSpeed;
   Quad quad;

   switch (obj->state) {
   case 0:

      switch (obj->functionIndex) {
      case OBJF_ENGULF_FLAME_DAMAGE:
      case OBJF_ENGULF_FLAME_SLAY:
         i = 0;
         break;

      case OBJF_ENGULF_LIGHTNING_SLAY:
      case OBJF_ENGULF_LIGHTNING_DAMAGE:
      case OBJF_ENGULF_LIGHTNING_DAMAGE_B:
      case OBJF_ENGULF_LIGHTNING_SLAY_B:
         i = 1;
         break;

      case OBJF_ENGULF_EXPLOSION_SLAY:
         i = 2;
         break;

      case OBJF_EXPLOSION_RING_EMITTER:
      default:
         i = 2;
         break;
      }

      OBJ.unitSprite = unitSprite = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
      unitSprite->d.sprite.hidden = 1;
      OBJ.fade = 0x80;
      OBJ.increasing = 0;
      if (OBJ.clut == CLUT_NULL) {
         OBJ.clut = fxInfo[i].clut;
      }

      fxObj1 = Obj_GetUnused();
      fxObj1->functionIndex = fxInfo[i].objf;
      fxObj1->x1.n = unitSprite->x1.n;
      fxObj1->z1.n = unitSprite->z1.n;
      fxObj1->y1.n = unitSprite->y1.n + CV(0.8125);
      fxObj1->mem = 30;
      fxObj1->d.objf133.radius = fxInfo[i].radius;
      fxObj1->d.objf133.pulseSpeed = fxInfo[i].to_x2a;
      fxObj1->d.objf133.maxHeight = fxInfo[i].to_x28;
      fxObj1->d.objf133.semiTrans = 2;
      fxObj1->d.objf133.clut = OBJ.clut;
      fxObj1->d.objf133.timer = 0;

      obj->state++;
      break;

   case 1:
      unitSprite = OBJ.unitSprite;
      fxObj1 = Obj_GetUnused();
      CopyObject(unitSprite, fxObj1);
      fxObj2 = Obj_GetUnused();
      CopyObject(unitSprite, fxObj2);

      fxObj1->d.sprite.hidden = 0;
      fxObj2->d.sprite.hidden = 0;
      fxObj2->d.sprite.clut = OBJ.clut;
      fxObj1->d.sprite.clut = OBJ.clut;
      fxObj1->d.sprite.semiTrans = 2;

      RenderUnitSprite(gGraphicsPtr->ot, fxObj1, 0);
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      setRGB0(poly, OBJ.fade, OBJ.fade, OBJ.fade);

      RenderUnitSprite(gGraphicsPtr->ot, fxObj2, 0);
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      setRGB0(poly, OBJ.fade, OBJ.fade, OBJ.fade);

      if (!OBJ.increasing) {
         OBJ.fade -= 12;
         if (OBJ.fade <= 0) {
            OBJ.fade = 0;
            OBJ.increasing = 1;
         }
      } else {
         OBJ.fade += 12;
         if (OBJ.fade >= 0x80) {
            OBJ.fade = 0x80;
            OBJ.increasing = 0;
         }
      }

      OBJ.phase++;
      if (OBJ.phase == 30) {
         dmgObj = Obj_GetUnused();
         dmgObj->functionIndex = OBJF_DISPLAY_DAMAGE_2;
         dmgObj->x1.s.hi = obj->x1.s.hi;
         dmgObj->z1.s.hi = obj->z1.s.hi;

         if (obj->functionIndex == OBJF_ENGULF_FLAME_DAMAGE || obj->functionIndex == OBJF_ENGULF_LIGHTNING_DAMAGE ||
             obj->functionIndex == OBJF_ENGULF_EXPLOSION_DAMAGE || obj->functionIndex == OBJF_ENGULF_LIGHTNING_DAMAGE_B) {
            unitSprite->d.sprite.hidden = 0;
         }
      } else if (OBJ.phase == 60) {
         fxObj1->functionIndex = OBJF_NULL;
         fxObj2->functionIndex = OBJF_NULL;
         OBJ.phase = 0;

         if (obj->functionIndex == OBJF_ENGULF_FLAME_DAMAGE || obj->functionIndex == OBJF_ENGULF_LIGHTNING_DAMAGE ||
             obj->functionIndex == OBJF_ENGULF_EXPLOSION_DAMAGE || obj->functionIndex == OBJF_ENGULF_LIGHTNING_DAMAGE_B) {
            obj->functionIndex = OBJF_NULL;
            gSignal3 = 1;
            return;
         }

         obj->state++;
         break;
      }

      fxObj1->functionIndex = OBJF_NULL;
      fxObj2->functionIndex = OBJF_NULL;
      break;

   case 2:

      switch (obj->functionIndex) {
      case OBJF_ENGULF_LIGHTNING_SLAY:
         fadeSpeed = 8;
         stretchSpeed = 0xc0;
         break;
      case OBJF_ENGULF_FLAME_SLAY:
         fadeSpeed = 12;
         stretchSpeed = 0xe0;
         break;
      case OBJF_ENGULF_UNUSED_139:
         fadeSpeed = 8;
         stretchSpeed = 0xc0;
         break;
      default:
         fadeSpeed = 8;
         stretchSpeed = 0xc0;
      }

      unitSprite = OBJ.unitSprite;
      fxObj1 = Obj_GetUnused();
      CopyObject(unitSprite, fxObj1);
      fxObj2 = Obj_GetUnused();
      CopyObject(unitSprite, fxObj2);
      fxObj1->d.sprite.hidden = 0;
      fxObj2->d.sprite.hidden = 0;
      fxObj2->d.sprite.clut = OBJ.clut;
      fxObj1->d.sprite.clut = OBJ.clut;
      fxObj1->d.sprite.semiTrans = 2;

      for (i = 0; i < 4; i++) {
         quad[i].vx = gQuad_800fe53c[i].vx;
         quad[i].vz = gQuad_800fe53c[i].vz;
         quad[i].vy = gQuad_800fe53c[i].vy;
      }

      for (i = 0; i < 4; i++) {
         gQuad_800fe53c[i].vx = (quad[i].vx * (rsin(OBJ.phase) + ONE) / ONE);
         gQuad_800fe53c[i].vz = (quad[i].vz * (rsin(OBJ.phase) + ONE) / ONE);

         if (OBJ.phase <= DEG(180)) {
            gQuad_800fe53c[i].vy =
                quad[i].vy * (rsin(OBJ.phase + DEG(180)) / 2 + (ONE / 2)) / (ONE / 2);
         } else {
            gQuad_800fe53c[i].vy = quad[i].vy * (rsin(OBJ.phase + DEG(180)) * 3 + ONE) / ONE;
         }
      }

      RenderUnitSprite(gGraphicsPtr->ot, fxObj1, 0);
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      setRGB0(poly, OBJ.fade, OBJ.fade, OBJ.fade);

      RenderUnitSprite(gGraphicsPtr->ot, fxObj2, 0);
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      setRGB0(poly, OBJ.fade, OBJ.fade, OBJ.fade);

      for (i = 0; i < 4; i++) {
         gQuad_800fe53c[i].vx = quad[i].vx;
         gQuad_800fe53c[i].vz = quad[i].vz;
         gQuad_800fe53c[i].vy = quad[i].vy;
      }

      if (!OBJ.increasing) {
         OBJ.fade -= fadeSpeed;
         if (OBJ.fade <= 0) {
            OBJ.fade = 0;
            OBJ.increasing = 1;
         }
      } else {
         OBJ.fade += fadeSpeed;
         if (OBJ.fade >= 0x80) {
            OBJ.fade = 0x80;
            OBJ.increasing = 0;
         }
      }

      OBJ.phase += stretchSpeed;
      if (OBJ.phase > DEG(270)) {
         obj->functionIndex = OBJF_NULL;
         gSignal3 = 1;
      }

      fxObj1->functionIndex = OBJF_NULL;
      fxObj2->functionIndex = OBJF_NULL;
      break;
   }
}

#undef OBJF
#define OBJF 149
void Objf149_FlashingUnitSprite(Object *obj) {
   Object *unitSprite, *fxSprite1, *fxSprite2;
   POLY_FT4 *poly;

   switch (obj->state) {
   case 0:
      OBJ.unitSprite = unitSprite = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
      OBJ.fade = 0x80;
      OBJ.increasing = 0;
      unitSprite->d.sprite.hidden = 1;
      if (OBJ.clut == CLUT_NULL) {
         OBJ.clut = CLUT_REDS;
      }
      if (OBJ.fadeSpeed == 0) {
         OBJ.fadeSpeed = 16;
      }
      obj->state++;
      break;

   case 1:
      unitSprite = OBJ.unitSprite;
      fxSprite1 = Obj_GetUnused();
      CopyObject(unitSprite, fxSprite1);
      fxSprite2 = Obj_GetUnused();
      CopyObject(unitSprite, fxSprite2);
      fxSprite1->d.sprite.hidden = 0;
      fxSprite2->d.sprite.hidden = 0;
      fxSprite1->d.sprite.semiTrans = 2;
      fxSprite1->d.sprite.clut = OBJ.clut;

      RenderUnitSprite(gGraphicsPtr->ot, fxSprite1, 0);
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      setRGB0(poly, 0x80 - OBJ.fade, 0x80 - OBJ.fade, 0x80 - OBJ.fade);

      RenderUnitSprite(gGraphicsPtr->ot, fxSprite2, 0);
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      setRGB0(poly, OBJ.fade, OBJ.fade, OBJ.fade);

      if (!OBJ.increasing) {
         OBJ.fade -= OBJ.fadeSpeed;
         if (OBJ.fade <= 0) {
            OBJ.fade = 0;
            OBJ.increasing = 1;
         }
      } else {
         OBJ.fade += OBJ.fadeSpeed;
         if (OBJ.fade >= 0x80) {
            OBJ.fade = 0x80;
            OBJ.increasing = 0;
         }
      }

      fxSprite2->functionIndex = OBJF_NULL;
      fxSprite1->functionIndex = OBJF_NULL;
      break;

   case 99:
      unitSprite = OBJ.unitSprite;
      unitSprite->d.sprite.hidden = 0;
      obj->functionIndex = OBJF_NULL;
      break;
   }
}

// Also used for blood (by setting poly color to red)
static s16 sImpactAnimData_800feb44[18] = {5, GFX_IMPACT_1, 2, GFX_IMPACT_2, 2, GFX_IMPACT_3,
                                           2, GFX_IMPACT_4, 2, GFX_IMPACT_5, 2, GFX_IMPACT_6,
                                           2, GFX_IMPACT_7, 2, GFX_NULL,     0, GFX_NULL};

// Used instead of blood for "rocky" enemy types like golems
static s16 sRockAnimData_800feb68[20] = {5, GFX_ROCK_1, 2, GFX_ROCK_2, 2, GFX_ROCK_3, 2, GFX_ROCK_4,
                                         2, GFX_ROCK_1, 2, GFX_ROCK_2, 2, GFX_ROCK_3, 2, GFX_ROCK_4,
                                         2, GFX_NULL,   1, GFX_NULL};

// Used for clouds of sand or dust, etc.
static s16 sCloudAnimData_800feb90[24] = {
    7, GFX_PUFF_1, 1, GFX_PUFF_2, 1, GFX_PUFF_3, 1, GFX_PUFF_4,  1, GFX_PUFF_5, 1, GFX_PUFF_6,
    1, GFX_PUFF_7, 1, GFX_PUFF_8, 1, GFX_PUFF_9, 1, GFX_PUFF_10, 1, GFX_NULL,   0, GFX_NULL};

#undef OBJF
#define OBJF 215
void Objf215_Cloud(Object *obj) {
   Object *sprite;
   s16 a;

   switch (obj->state) {
   case 0:
      sprite = Obj_GetUnused();
      sprite->functionIndex = OBJF_NOOP;
      sprite->d.sprite.animData = sCloudAnimData_800feb90;
      sprite->d.sprite.animInitialized = 0;
      OBJ.sprite = sprite;

      a = rand() % DEG(360);
      OBJ.position1.x = rsin(a) >> 5;
      OBJ.position1.z = rcos(a) >> 5;
      OBJ.position1.y = (rand() % 0x60) + 0x20;

      a = rand() % DEG(360);
      OBJ.position2.x = rsin(a) >> 5;
      OBJ.position2.z = rcos(a) >> 5;
      OBJ.position2.y = (rand() % 0x60) + 0x20;

      a = rand() % DEG(360);
      OBJ.position3.x = rsin(a) >> 5;
      OBJ.position3.z = rcos(a) >> 5;
      OBJ.position3.y = (rand() % 0x60) + 0x20;

      a = rand() % DEG(360);
      OBJ.position4.x = rsin(a) >> 5;
      OBJ.position4.z = rcos(a) >> 5;
      OBJ.position4.y = (rand() % 0x60) + 0x20;

      a = rand() % DEG(360);
      OBJ.position5.x = rsin(a) >> 5;
      OBJ.position5.z = rcos(a) >> 5;
      OBJ.position5.y = (rand() % 0x60) + 0x20;

      a = rand() % DEG(360);
      OBJ.position6.x = rsin(a) >> 5;
      OBJ.position6.z = rcos(a) >> 5;
      OBJ.position6.y = (rand() % 0x60) + 0x20;

      a = rand() % DEG(360);
      OBJ.position7_x = rsin(a) >> 5;
      OBJ.position7_z = rcos(a) >> 5;
      OBJ.position7_y = (rand() % 0x60) + 0x20;

      a = rand() % DEG(360);
      OBJ.position8_x = rsin(a) >> 5;
      OBJ.position8_z = rcos(a) >> 5;
      OBJ.position8_y = (rand() % 0x60) + 0x20;

      OBJ.halfSize = (rand() % 6) + 4;
      obj->state++;

   // fallthrough
   case 1:
      a = OBJ.halfSize;
      gQuad_800fe63c[0].vx = -a;
      gQuad_800fe63c[0].vy = -a;
      gQuad_800fe63c[1].vx = a;
      gQuad_800fe63c[1].vy = -a;
      gQuad_800fe63c[2].vx = -a;
      gQuad_800fe63c[2].vy = a;
      gQuad_800fe63c[3].vx = a;
      gQuad_800fe63c[3].vy = a;

      sprite = OBJ.sprite;
      UpdateObjAnimation(sprite);

      sprite->x1.n =
          obj->x1.n + OBJ.position1.x + (OBJ.position1.x * (ONE - rcos(OBJ.theta)) / ONE);
      sprite->z1.n =
          obj->z1.n + OBJ.position1.z + (OBJ.position1.z * (ONE - rcos(OBJ.theta)) / ONE);
      sprite->y1.n = obj->y1.n + (OBJ.position1.y * rsin(OBJ.theta)) / ONE;
      AddObjPrim6(gGraphicsPtr->ot, sprite, 0);

      sprite->x1.n =
          obj->x1.n + OBJ.position2.x + (OBJ.position2.x * (ONE - rcos(OBJ.theta)) / ONE);
      sprite->z1.n =
          obj->z1.n + OBJ.position2.z + (OBJ.position2.z * (ONE - rcos(OBJ.theta)) / ONE);
      sprite->y1.n = obj->y1.n + (OBJ.position2.y * rsin(OBJ.theta)) / ONE;
      AddObjPrim6(gGraphicsPtr->ot, sprite, 0);

      sprite->x1.n =
          obj->x1.n + OBJ.position3.x + (OBJ.position3.x * (ONE - rcos(OBJ.theta)) / ONE);
      sprite->z1.n =
          obj->z1.n + OBJ.position3.z + (OBJ.position3.z * (ONE - rcos(OBJ.theta)) / ONE);
      sprite->y1.n = obj->y1.n + (OBJ.position3.y * rsin(OBJ.theta)) / ONE;
      AddObjPrim6(gGraphicsPtr->ot, sprite, 0);

      sprite->x1.n =
          obj->x1.n + OBJ.position4.x + (OBJ.position4.x * (ONE - rcos(OBJ.theta)) / ONE);
      sprite->z1.n =
          obj->z1.n + OBJ.position4.z + (OBJ.position4.z * (ONE - rcos(OBJ.theta)) / ONE);
      sprite->y1.n = obj->y1.n + (OBJ.position4.y * rsin(OBJ.theta)) / ONE;
      AddObjPrim6(gGraphicsPtr->ot, sprite, 0);

      sprite->x1.n =
          obj->x1.n + OBJ.position5.x + (OBJ.position5.x * (ONE - rcos(OBJ.theta)) / ONE);
      sprite->z1.n =
          obj->z1.n + OBJ.position5.z + (OBJ.position5.z * (ONE - rcos(OBJ.theta)) / ONE);
      sprite->y1.n = obj->y1.n + (OBJ.position5.y * rsin(OBJ.theta)) / ONE;
      AddObjPrim6(gGraphicsPtr->ot, sprite, 0);

      sprite->x1.n =
          obj->x1.n + OBJ.position6.x + (OBJ.position6.x * (ONE - rcos(OBJ.theta)) / ONE);
      sprite->z1.n =
          obj->z1.n + OBJ.position6.z + (OBJ.position6.z * (ONE - rcos(OBJ.theta)) / ONE);
      sprite->y1.n = obj->y1.n + (OBJ.position6.y * rsin(OBJ.theta)) / ONE;
      AddObjPrim6(gGraphicsPtr->ot, sprite, 0);

      sprite->x1.n =
          obj->x1.n + OBJ.position7_x + (OBJ.position6.x * (ONE - rcos(OBJ.theta)) / ONE);
      sprite->z1.n =
          obj->z1.n + OBJ.position7_z + (OBJ.position6.z * (ONE - rcos(OBJ.theta)) / ONE);
      sprite->y1.n = obj->y1.n + (OBJ.position7_y * rsin(OBJ.theta)) / ONE;
      AddObjPrim6(gGraphicsPtr->ot, sprite, 0);

      sprite->x1.n =
          obj->x1.n + OBJ.position8_x + (OBJ.position6.x * (ONE - rcos(OBJ.theta)) / ONE);
      sprite->z1.n =
          obj->z1.n + OBJ.position8_z + (OBJ.position6.z * (ONE - rcos(OBJ.theta)) / ONE);
      sprite->y1.n = obj->y1.n + (OBJ.position8_y * rsin(OBJ.theta)) / ONE;
      AddObjPrim6(gGraphicsPtr->ot, sprite, 0);

      OBJ.theta += (DEG(90) / 10);
      if (OBJ.theta >= DEG(90)) {
         obj->functionIndex = OBJF_NULL;
         sprite->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 213
void Objf213_DustCloudSpawner(Object *obj) {
   // Spawned by: OBJF_MAP_OBJECT_CRATE->213; SetupMapExtras()->362->213 (lowering bridge, etc.)
   Object *particle;

   switch (obj->state) {
   case 0:
      OBJ.timer = 10;
      obj->state++;
   // fallthrough
   case 1:
      if (OBJ.timer > 2) {
         particle = Obj_GetUnused();
         particle->functionIndex = OBJF_DUST_CLOUD;
         particle->x1.n = obj->x1.n;
         particle->z1.n = obj->z1.n;
         particle->y1.n = obj->y1.n;
      }
      if (--OBJ.timer == -1) {
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 214
void Objf214_DustCloud(Object *obj) {
   SVectorXZY *pPosition;
   Object *sprite;
   s32 i;
   s16 a, theta;
   s16 qx, qz;

   pPosition = &OBJ.positions[0];

   switch (obj->state) {
   case 0:
      sprite = Obj_GetUnused();
      sprite->functionIndex = OBJF_NOOP;
      sprite->d.sprite.animData = sCloudAnimData_800feb90;
      sprite->d.sprite.animInitialized = 0;

      OBJ.halfSize = rand() % 4 + 4;
      OBJ.sprite = sprite;

      for (i = 0; i < ARRAY_COUNT(OBJ.positions); i++, pPosition++) {
         a = rand() % DEG(360);
         pPosition->x = rsin(a) >> 5;
         pPosition->z = rcos(a) >> 5;
         pPosition->y = (rand() % 0x40) + 0x20;
      }

      obj->state++;

   // fallthrough
   case 1:
      a = OBJ.halfSize;
      gQuad_800fe63c[0].vx = -a;
      gQuad_800fe63c[0].vy = -a;
      gQuad_800fe63c[1].vx = a;
      gQuad_800fe63c[1].vy = -a;
      gQuad_800fe63c[2].vx = -a;
      gQuad_800fe63c[2].vy = a;
      gQuad_800fe63c[3].vx = a;
      gQuad_800fe63c[3].vy = a;

      sprite = OBJ.sprite;
      UpdateObjAnimation(sprite);

      theta = OBJ.theta;

      for (i = 0; i < ARRAY_COUNT(OBJ.positions); i++, pPosition++) {
         qx = pPosition->x >> 2;
         qz = pPosition->z >> 2;

         sprite->x1.n =
             obj->x1.n + pPosition->x + qx + (pPosition->x * (ONE - rcos(theta / 2)) / ONE);

         sprite->z1.n =
             obj->z1.n + pPosition->z + qz + (pPosition->z * (ONE - rcos(theta / 2)) / ONE);

         sprite->y1.n = obj->y1.n + (pPosition->y * rsin(theta) / ONE);

         AddObjPrim6(gGraphicsPtr->ot, sprite, 0);
      }

      OBJ.theta += (DEG(180) / 10);
      if (OBJ.theta >= (DEG(180) / 10 * 10)) {
         obj->functionIndex = OBJF_NULL;
         sprite->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 130
void Objf130_TorusSweepOnUnit(Object *obj) {
   Object *sprite, *unitSprite;
   s32 i, j;
   s16 a, b, c, d, e; // TBD

   POLY_FT4 *poly;

   switch (obj->state) {
   case 0:
      unitSprite = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
      obj->x1.n = unitSprite->x1.n;
      obj->z1.n = unitSprite->z1.n;
      obj->y1.n = unitSprite->y1.n;

      switch (unitSprite->d.sprite.direction) {
      case ANGLE_WEST:
         OBJ.dirX = -1;
         OBJ.dirZ = 0;
         break;
      case ANGLE_EAST:
         OBJ.dirX = 1;
         OBJ.dirZ = 0;
         break;
      case ANGLE_SOUTH:
         OBJ.dirX = 0;
         OBJ.dirZ = -1;
         break;
      case ANGLE_NORTH:
         OBJ.dirX = 0;
         OBJ.dirZ = 1;
         break;
      }

      obj->state++;

   // fallthrough
   case 1:
      sprite = Obj_GetUnused();
      sprite->functionIndex = OBJF_NOOP;
      sprite->d.sprite.gfxIdx = GFX_COLOR_13;
      sprite->d.sprite.clut = CLUT_BLUES;
      sprite->d.sprite.semiTrans = 1;

      switch (OBJ.dirX) {
      case 0:
         a = (OBJ.dirZ == 1) ? DEG(180) : 0;
         break;
      case 1:
         a = (OBJ.dirX == 1) ? DEG(180) : 0;
         break;
      }

      for (i = OBJ.bandStart; i < OBJ.bandEnd; i++) {
         b = (rsin(i * 401) * 13) >> 8;
         c = (rsin((i + 1) * 401) * 13) >> 8;
         d = (rcos(a + i * 401) * 13) >> 8;
         e = (rcos(a + (i + 1) * 401) * 13) >> 8;

         for (j = 0; j < 16; j++) {

            switch (OBJ.dirX) {
            case 0:
               sprite->d.sprite.coords[0].x = obj->x1.n + (b * rcos(j * 128) >> 12);
               sprite->d.sprite.coords[0].z = obj->z1.n + d;

               sprite->d.sprite.coords[1].x = obj->x1.n + (b * rcos((j + 1) * 128) >> 12);
               sprite->d.sprite.coords[1].z = obj->z1.n + d;

               sprite->d.sprite.coords[2].x = obj->x1.n + (c * rcos(j * 128) >> 12);
               sprite->d.sprite.coords[2].z = obj->z1.n + e;

               sprite->d.sprite.coords[3].x = obj->x1.n + (c * rcos((j + 1) * 128) >> 12);
               sprite->d.sprite.coords[3].z = obj->z1.n + e;
               break;
            case 1:
               sprite->d.sprite.coords[0].z = obj->z1.n + (b * rcos(j * 128) >> 12);
               sprite->d.sprite.coords[0].x = obj->x1.n + d;

               sprite->d.sprite.coords[1].z = obj->z1.n + (b * rcos((j + 1) * 128) >> 12);
               sprite->d.sprite.coords[1].x = obj->x1.n + d;

               sprite->d.sprite.coords[2].z = obj->z1.n + (c * rcos(j * 128) >> 12);
               sprite->d.sprite.coords[2].x = obj->x1.n + e;

               sprite->d.sprite.coords[3].z = obj->z1.n + (c * rcos((j + 1) * 128) >> 12);
               sprite->d.sprite.coords[3].x = obj->x1.n + e;
               break;
            }

            sprite->d.sprite.coords[0].y = obj->y1.n + (b * rsin(j * 128) >> 12);
            sprite->d.sprite.coords[1].y = obj->y1.n + (b * rsin((j + 1) * 128) >> 12);

            sprite->d.sprite.coords[2].y = obj->y1.n + (c * rsin(j * 128) >> 12);
            sprite->d.sprite.coords[3].y = obj->y1.n + (c * rsin((j + 1) * 128) >> 12);

            AddObjPrim4(gGraphicsPtr->ot, sprite);
            poly = &gGraphicsPtr->quads[gQuadIndex - 1];
            setRGB0(poly, 0xff, 0xff, 0xff);
         }
      }

      sprite->functionIndex = OBJF_NULL;

      switch (obj->state2) {
      case 0:
         OBJ.bandEnd++;
         if (OBJ.bandEnd == 6) {
            obj->state2++;
            OBJ.bandEnd = 5;
         }
         break;
      case 1:
         OBJ.bandStart++;
         if (OBJ.bandStart == 5) {
            obj->functionIndex = OBJF_NULL;
            gSignal3 = 1;
         }
         break;
      }

      break;
   }
}

#undef OBJF
#define OBJF 131
void Objf131_SlayUnit(Object *obj) {
   Object *warpFx;
   s16 initSpeed;

   switch (obj->state) {
   case 0:
      warpFx = Obj_GetUnused();
      warpFx->functionIndex = OBJF_STRETCH_WARP_SPRITE;
      warpFx->x1.s.hi = obj->x1.s.hi;
      warpFx->z1.s.hi = obj->z1.s.hi;
      warpFx->d.objf062.speed = OBJ.speed;
      if (OBJ.unitSprite != NULL) {
         warpFx->d.objf062.sprite = OBJ.unitSprite;
      }

      if (OBJ.speed == 0) {
         initSpeed = 0xc0;
      } else {
         initSpeed = OBJ.speed;
      }
      OBJ.speed = (0xc00 / initSpeed);

      obj->state++;

   // fallthrough
   case 1:
      if (--OBJ.speed == -1) {
         obj->functionIndex = OBJF_NULL;
         gSignal3 = 1;
      }

      break;
   }
}

#undef OBJF
#define OBJF 201
void Objf201_UnitStruck(Object *obj) {
   static s16 impactAnimData[18] = {3, GFX_IMPACT_1, 2, GFX_IMPACT_2, 2, GFX_IMPACT_3,
                                    2, GFX_IMPACT_4, 2, GFX_IMPACT_5, 2, GFX_IMPACT_6,
                                    2, GFX_IMPACT_7, 2, GFX_NULL,     0, GFX_NULL};

   Object *obj_s0;
   Object *obj_s2;
   Object *obj_s3;
   POLY_FT4 *poly;

   switch (obj->state) {
   case 0:
      obj_s0 = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
      obj->x1.n = obj_s0->x1.n;
      obj->z1.n = obj_s0->z1.n;
      obj->y1.n = obj_s0->y1.n;
      OBJ.unitSprite = obj_s0;
      obj_s0->d.sprite.hidden = 1;

      obj_s0 = Obj_GetUnused();
      obj_s0->functionIndex = OBJF_NOOP;
      obj_s0->d.sprite.animData = impactAnimData;
      obj_s0->x1.n = obj->x1.n;
      obj_s0->z1.n = obj->z1.n;
      obj_s0->y1.n = obj->y1.n + CV(0.875);

      switch (obj_s0->d.sprite.direction) {
      case ANGLE_WEST:
         obj_s0->x1.n += CV(0.125);
         break;
      case ANGLE_EAST:
         obj_s0->x1.n -= CV(0.125);
         break;
      case ANGLE_SOUTH:
         obj_s0->z1.n += CV(0.125);
         break;
      case ANGLE_NORTH:
         obj_s0->z1.n -= CV(0.125);
         break;
      }

      OBJ.impactSprite = obj_s0;
      obj->state++;

   // fallthrough
   case 1:
      obj_s0 = OBJ.unitSprite;
      obj_s2 = Obj_GetUnused();
      CopyObject(obj_s0, obj_s2);
      obj_s3 = Obj_GetUnused();
      CopyObject(obj_s0, obj_s3);
      obj_s2->d.sprite.hidden = 0;
      obj_s3->d.sprite.hidden = 0;
      obj_s2->d.sprite.semiTrans = 2;
      obj_s2->d.sprite.clut = CLUT_MASK;

      RenderUnitSprite(gGraphicsPtr->ot, obj_s2, 0);
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      setRGB0(poly, 0x80 - OBJ.fade, 0x80 - OBJ.fade, 0x80 - OBJ.fade);

      RenderUnitSprite(gGraphicsPtr->ot, obj_s3, 0);
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      setRGB0(poly, OBJ.fade, OBJ.fade, OBJ.fade);

      if (OBJ.flashState == 0) {
         OBJ.fade -= 0x20;
         if (OBJ.fade <= 0) {
            OBJ.fade = 0;
            OBJ.flashState = 1;
         }
      } else if (OBJ.flashState == 1) {
         OBJ.fade += 0x20;
         if (OBJ.fade >= 0x80) {
            OBJ.fade = 0x80;
            OBJ.flashState = 2;
         }
      }

      obj_s0 = OBJ.impactSprite;
      UpdateObjAnimation(obj_s0);
      AddObjPrim6(gGraphicsPtr->ot, obj_s0, 0);

      if (obj_s0->d.sprite.animFinished) {
         obj->functionIndex = OBJF_NULL;
         obj_s0 = OBJ.impactSprite;
         obj_s0->functionIndex = OBJF_NULL;
         obj_s0 = OBJ.unitSprite;
#ifdef PC_PORT
         if (obj_s0)
#endif
         obj_s0->d.sprite.hidden = 0;
      }

      obj_s3->functionIndex = OBJF_NULL;
      obj_s2->functionIndex = OBJF_NULL;
      break;
   }
}

#undef OBJF
#define OBJF 202
void Objf202_746_UnitBlocking(Object *obj) {
   Object *obj_s0;
   Object *obj_s2;
   Object *obj_s3;
   POLY_FT4 *poly;
   s32 i;

   switch (obj->state) {
   case 0:
      if (obj->functionIndex == OBJF_ENTITY_BLOCKING) {
         obj_s0 = OBJ.variant_0x24.entitySpriteParam;
         obj_s0->d.sprite.direction = DIR_WEST;
         OBJ.variant_0x24.entitySpriteParam = NULL;
      } else {
         obj_s0 = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
      }

      obj->x1.n = obj_s0->x1.n;
      obj->z1.n = obj_s0->z1.n;
      obj->y1.n = obj_s0->y1.n;
      OBJ.unitSprite = obj_s0;
      obj_s0->d.sprite.hidden = 1;
      OBJ.variant_0x24.timer = 14;

      for (i = 0; i < 8; i++) {
         obj_s3 = Obj_GetUnused();
         obj_s3->functionIndex = OBJF_BLOCKING_IMPACT_PARTICLE;
         obj_s3->x1.n = obj->x1.n;
         obj_s3->z1.n = obj->z1.n;
         obj_s3->y1.n = obj->y1.n + CV(0.875);
         obj_s3->d.objf203.direction = obj_s0->d.sprite.direction;
      }

      obj->state++;

   // fallthrough
   case 1:
      obj_s0 = OBJ.unitSprite;
      obj_s2 = Obj_GetUnused();
      CopyObject(obj_s0, obj_s2);
      obj_s3 = Obj_GetUnused();
      CopyObject(obj_s0, obj_s3);
      obj_s2->d.sprite.hidden = 0;
      obj_s3->d.sprite.hidden = 0;
      obj_s2->d.sprite.semiTrans = 2;
      obj_s2->d.sprite.clut = CLUT_MASK;

      RenderUnitSprite(gGraphicsPtr->ot, obj_s2, 0);
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      setRGB0(poly, 0x80 - OBJ.fade, 0x80 - OBJ.fade, 0x80 - OBJ.fade);

      RenderUnitSprite(gGraphicsPtr->ot, obj_s3, 0);
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      setRGB0(poly, OBJ.fade, OBJ.fade, OBJ.fade);

      if (OBJ.flashState == 0) {
         OBJ.fade -= 0x20;
         if (OBJ.fade <= 0) {
            OBJ.fade = 0;
            OBJ.flashState = 1;
         }
      } else if (OBJ.flashState == 1) {
         OBJ.fade += 0x20;
         if (OBJ.fade >= 0x80) {
            OBJ.fade = 0x80;
            OBJ.flashState = 2;
         }
      }

      if (--OBJ.variant_0x24.timer == -1) {
         obj->functionIndex = OBJF_NULL;
         obj_s0 = OBJ.unusedSprite;
         /* unusedSprite/unitSprite may be NULL here; the PSX silently discards a store through
          * a null pointer, so writing functionIndex@0x8 is harmless on hardware. On PC, skip
          * the store -- store-discard is exact. */
#ifdef PC_PORT
         if (obj_s0)
#endif
         obj_s0->functionIndex = OBJF_NULL;
         obj_s0 = OBJ.unitSprite;
         obj_s0->d.sprite.hidden = 0;
      }

      obj_s3->functionIndex = OBJF_NULL;
      obj_s2->functionIndex = OBJF_NULL;
      break;
   }
}

#undef OBJF
#define OBJF 203
void Objf203_BlockingImpactParticle(Object *obj) {
   Object *sprite;
   s16 a;
   s8 unused[32];

   switch (obj->state) {
   case 0:
      sprite = Obj_GetUnused();
      sprite->functionIndex = OBJF_NOOP;
      sprite->d.sprite.clut = CLUT_BLUES;
      sprite->d.sprite.animData = sImpactAnimData_800feb44;
      sprite->d.sprite.animInitialized = 0;
      OBJ.sprite = sprite;

      a = rand() % DEG(360);

      switch (OBJ.direction) {
      case ANGLE_WEST:
         obj->x1.n += CV(0.125);
         OBJ.zOfs = rsin(a) / 24;
         OBJ.yOfs = rcos(a) / 24;
         OBJ.xOfs = CV(0.75) - rand() % CV(1.5);
         break;
      case ANGLE_EAST:
         obj->x1.n -= CV(0.125);
         OBJ.zOfs = rsin(a) / 24;
         OBJ.yOfs = rcos(a) / 24;
         OBJ.xOfs = CV(0.75) - rand() % CV(1.5);
         break;
      case ANGLE_SOUTH:
         obj->z1.n += CV(0.125);
         OBJ.xOfs = rsin(a) / 24;
         OBJ.yOfs = rcos(a) / 24;
         OBJ.zOfs = CV(0.75) - rand() % CV(1.5);
         break;
      case ANGLE_NORTH:
         obj->z1.n -= CV(0.125);
         OBJ.xOfs = rsin(a) / 24;
         OBJ.yOfs = rcos(a) / 24;
         OBJ.zOfs = CV(0.75) - rand() % CV(1.5);
         break;
      }

      OBJ.yOfs = abs(OBJ.yOfs) * 3;
      obj->state++;

   // fallthrough
   case 1:
      sprite = OBJ.sprite;
      sprite->x1.n = obj->x1.n + (OBJ.xOfs * rsin(OBJ.theta) / ONE);
      sprite->z1.n = obj->z1.n + (OBJ.zOfs * rsin(OBJ.theta) / ONE);
      sprite->y1.n = obj->y1.n + (OBJ.yOfs * rsin(OBJ.theta) / ONE) - (OBJ.theta / 4);

      UpdateObjAnimation(sprite);
      AddObjPrim6(gGraphicsPtr->ot, sprite, 0);

      OBJ.theta += 0x4e;
      if (OBJ.theta >= DEG(90)) {
         obj->functionIndex = OBJF_NULL;
         sprite->functionIndex = OBJF_NULL;
      }

      break;
   }
}

#undef OBJF
#define OBJF 205
void Objf205_BloodSpurt(Object *obj) {
   s32 particleObjf;
   Object *unitSprite;
   Object *obj_s2;
   Object *obj_s4;
   POLY_FT4 *poly;

   //? Rock spurt stuff may be left-over code since separate objf exists (Objf685_RockSpurt)
   particleObjf = (obj->functionIndex == OBJF_ROCK_SPURT) ? OBJF_ROCK_SPURT_PARTICLE_2
                                                          : OBJF_BLOOD_SPURT_PARTICLE;

   switch (obj->state) {
   case 0:
      PerformAudioCommand(AUDIO_CMD_PLAY_SFX(223));
      if (OBJ.unitSprite == NULL) {
         OBJ.unitSprite = unitSprite = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
      } else {
         unitSprite = OBJ.unitSprite;
      }
      obj->x1.n = unitSprite->x1.n;
      obj->z1.n = unitSprite->z1.n;
      obj->y1.n = unitSprite->y1.n;
      unitSprite->d.sprite.hidden = 1;
      OBJ.timer = 22;
      obj->state++;

   // fallthrough
   case 1:
      unitSprite = OBJ.unitSprite;
      obj_s4 = Obj_GetUnused();
      CopyObject(unitSprite, obj_s4);
      obj_s2 = Obj_GetUnused();
      CopyObject(unitSprite, obj_s2);
      obj_s4->d.sprite.hidden = 0;
      obj_s2->d.sprite.hidden = 0;

      if (obj->functionIndex == OBJF_BLOOD_SPURT) {
         obj_s4->d.sprite.semiTrans = 2;

         RenderUnitSprite(gGraphicsPtr->ot, obj_s4, 0);
         poly = &gGraphicsPtr->quads[gQuadIndex - 1];
         setRGB0(poly, OBJ.fade, 0, 0);

         RenderUnitSprite(gGraphicsPtr->ot, obj_s2, 0);
         poly = &gGraphicsPtr->quads[gQuadIndex - 1];
         setRGB0(poly, OBJ.fade + 0x7f, 0x80 - OBJ.fade, 0x80 - OBJ.fade);
      } else {
         RenderUnitSprite(gGraphicsPtr->ot, obj_s4, 0);
      }

      if (OBJ.flashState == 0) {
         OBJ.fade += 12;
         if (OBJ.fade > 0x80) {
            OBJ.fade = 0x80;
            OBJ.flashState = 1;
         }
      } else if (OBJ.flashState == 1) {
         OBJ.fade -= 10;
         if (OBJ.fade <= 0) {
            OBJ.fade = 0;
            OBJ.flashState = 2;
         }
      }

      obj_s2->functionIndex = OBJF_NULL;
      obj_s4->functionIndex = OBJF_NULL;

      obj_s2 = Obj_GetUnused();
      if (gState.mapNum == 32) {
         obj_s2->functionIndex = OBJF_BLOOD_SPURT_PARTICLE_OFFSET;
      } else {
         obj_s2->functionIndex = particleObjf;
      }
      obj_s2->x1.n = unitSprite->x1.n;
      obj_s2->z1.n = unitSprite->z1.n;
      obj_s2->y1.n = unitSprite->y1.n + CV(0.25) + rand() % CV(0.125);

      if (OBJ.timer % 2 == 0) {
         obj_s2 = Obj_GetUnused();
         if (gState.mapNum == 32) {
            obj_s2->functionIndex = OBJF_BLOOD_SPURT_PARTICLE_OFFSET;
         } else {
            obj_s2->functionIndex = particleObjf;
         }
         obj_s2->x1.n = unitSprite->x1.n;
         obj_s2->z1.n = unitSprite->z1.n;
         obj_s2->y1.n = unitSprite->y1.n + CV(0.25) + rand() % CV(0.125);
      }

      if (--OBJ.timer == -1) {
         obj->functionIndex = OBJF_NULL;
         unitSprite = OBJ.unitSprite;
         unitSprite->d.sprite.hidden = 0;
      }

      break;
   }
}

#undef OBJF
#define OBJF 206
void Objf206_686_BloodSpurtParticle(Object *obj) {
   Object *sprite;
   s32 i;
   s32 rnd;
   s16 a, b;
   POLY_FT4 *poly;
   s16 red, green, blue;
   s16 prevQuadIdx;
   s16 shade;

   switch (obj->state) {
   case 0:
      sprite = Obj_GetUnused();
      sprite->functionIndex = OBJF_NOOP;

      if (obj->functionIndex == OBJF_BLOOD_SPURT_PARTICLE ||
          obj->functionIndex == OBJF_BLOOD_SPURT_PARTICLE_OFFSET) {
         sprite->d.sprite.animData = sImpactAnimData_800feb44;
         sprite->d.sprite.clut = CLUT_REDS;
         sprite->d.sprite.animInitialized = 0;
      } else {
         sprite->d.sprite.animData = sRockAnimData_800feb68;
         sprite->d.sprite.clut = CLUT_2;
         sprite->d.sprite.animInitialized = 0;
         rnd = rand() % 3;
         for (i = 0; i < rnd; i++) {
            UpdateObjAnimation(sprite);
         }
      }

      OBJ.sprite = sprite;
      OBJ.shade = rand() % 0x60;

      a = rand() % DEG(360);
      OBJ.position1.x = (rand() % CV(0.25) + CV(1.5703125)) * rsin(a) / ONE;
      OBJ.position1.z = (rand() % CV(0.25) + CV(1.5703125)) * rcos(a) / ONE;
      OBJ.position1.y = rand() % CV(2.0) + CV(1.0);

      a = rand() % DEG(360);
      OBJ.position2.x = (rand() % CV(0.75) + CV(1.25)) * rsin(a) / ONE;
      OBJ.position2.z = (rand() % CV(0.75) + CV(1.25)) * rcos(a) / ONE;
      OBJ.position2.y = rand() % CV(2.0) + CV(1.0);

      a = rand() % DEG(360);
      OBJ.position3.x = (rand() % CV(0.75) + CV(1.25)) * rsin(a) / ONE;
      OBJ.position3.z = (rand() % CV(0.75) + CV(1.25)) * rcos(a) / ONE;
      OBJ.position3.y = rand() % CV(2.0) + CV(1.0);

      a = rand() % DEG(360);
      OBJ.position4.x = (rand() % CV(0.75) + CV(1.25)) * rsin(a) / ONE;
      OBJ.position4.z = (rand() % CV(0.75) + CV(1.25)) * rcos(a) / ONE;
      OBJ.position4.y = rand() % CV(2.0) + CV(1.0);

      a = rand() % DEG(360);
      OBJ.position5.x = (rand() % CV(0.75) + CV(1.25)) * rsin(a) / ONE;
      OBJ.position5.z = (rand() % CV(0.75) + CV(1.25)) * rcos(a) / ONE;
      OBJ.position5.y = rand() % CV(2.0) + CV(1.0);

      a = rand() % DEG(360);
      OBJ.position6.x = (rand() % CV(0.75) + CV(1.25)) * rsin(a) / ONE;
      OBJ.position6.z = (rand() % CV(0.75) + CV(1.25)) * rcos(a) / ONE;
      OBJ.position6.y = rand() % CV(2.0) + CV(1.0);

      a = rand() % DEG(360);
      OBJ.position7_x = (rand() % CV(0.75) + CV(1.25)) * rsin(a) / ONE;
      OBJ.position7_z = (rand() % CV(0.75) + CV(1.25)) * rcos(a) / ONE;
      OBJ.position7_y = rand() % CV(2.0) + CV(1.0);

      a = rand() % DEG(360);
      OBJ.position8_x = (rand() % CV(0.75) + CV(1.25)) * rsin(a) / ONE;
      OBJ.position8_z = (rand() % CV(0.75) + CV(1.25)) * rcos(a) / ONE;
      OBJ.position8_y = rand() % CV(2.0) + CV(1.0);

      obj->state++;

   // fallthrough
   case 1:
      sprite = OBJ.sprite;
      UpdateObjAnimation(sprite);

      a = ONE - rcos(OBJ.theta / 2);
      b = rsin(OBJ.theta);
      shade = OBJ.shade + 0x40;

      if (obj->functionIndex == OBJF_BLOOD_SPURT_PARTICLE ||
          obj->functionIndex == OBJF_BLOOD_SPURT_PARTICLE_OFFSET) {
         red = shade;
         green = 0;
         blue = 0;
      } else {
         red = 0x80;
         green = 0x80;
         blue = 0x80;
      }

      sprite->x1.n = obj->x1.n + OBJ.position1.x * a / ONE;
      sprite->z1.n = obj->z1.n + OBJ.position1.z * a / ONE;
      sprite->y1.n = obj->y1.n + OBJ.position1.y * b / ONE;

      prevQuadIdx = gQuadIndex;
      AddObjPrim6(gGraphicsPtr->ot, sprite, 0);

      if (prevQuadIdx != gQuadIndex) {
         poly = &gGraphicsPtr->quads[gQuadIndex - 1];
         setRGB0(poly, red, green, blue);
      }

      sprite->x1.n = obj->x1.n + OBJ.position2.x * a / ONE;
      sprite->z1.n = obj->z1.n + OBJ.position2.z * a / ONE;
      sprite->y1.n = obj->y1.n + OBJ.position2.y * b / ONE;

      prevQuadIdx = gQuadIndex;
      AddObjPrim6(gGraphicsPtr->ot, sprite, 0);

      if (prevQuadIdx != gQuadIndex) {
         poly = &gGraphicsPtr->quads[gQuadIndex - 1];
         setRGB0(poly, red, green, blue);
      }

      sprite->x1.n = obj->x1.n + OBJ.position3.x * a / ONE;
      sprite->z1.n = obj->z1.n + OBJ.position3.z * a / ONE;
      sprite->y1.n = obj->y1.n + OBJ.position3.y * b / ONE;

      prevQuadIdx = gQuadIndex;
      AddObjPrim6(gGraphicsPtr->ot, sprite, 0);

      if (prevQuadIdx != gQuadIndex) {
         poly = &gGraphicsPtr->quads[gQuadIndex - 1];
         setRGB0(poly, red, green, blue);
      }

      sprite->x1.n = obj->x1.n + OBJ.position4.x * a / ONE;
      sprite->z1.n = obj->z1.n + OBJ.position4.z * a / ONE;
      sprite->y1.n = obj->y1.n + OBJ.position4.y * b / ONE;

      prevQuadIdx = gQuadIndex;
      AddObjPrim6(gGraphicsPtr->ot, sprite, 0);

      if (prevQuadIdx != gQuadIndex) {
         poly = &gGraphicsPtr->quads[gQuadIndex - 1];
         setRGB0(poly, red, green, blue);
      }

      sprite->x1.n = obj->x1.n + OBJ.position5.x * a / ONE;
      sprite->z1.n = obj->z1.n + OBJ.position5.z * a / ONE;
      sprite->y1.n = obj->y1.n + OBJ.position5.y * b / ONE;

      prevQuadIdx = gQuadIndex;
      AddObjPrim6(gGraphicsPtr->ot, sprite, 0);

      if (prevQuadIdx != gQuadIndex) {
         poly = &gGraphicsPtr->quads[gQuadIndex - 1];
         setRGB0(poly, red, green, blue);
      }

      sprite->x1.n = obj->x1.n + OBJ.position6.x * a / ONE;
      sprite->z1.n = obj->z1.n + OBJ.position6.z * a / ONE;
      sprite->y1.n = obj->y1.n + OBJ.position6.y * b / ONE;

      prevQuadIdx = gQuadIndex;
      AddObjPrim6(gGraphicsPtr->ot, sprite, 0);

      if (prevQuadIdx != gQuadIndex) {
         poly = &gGraphicsPtr->quads[gQuadIndex - 1];
         setRGB0(poly, red, green, blue);
      }

      sprite->x1.n = obj->x1.n + OBJ.position7_x * a / ONE;
      sprite->z1.n = obj->z1.n + OBJ.position7_z * a / ONE;
      sprite->y1.n = obj->y1.n + OBJ.position7_y * b / ONE;

      prevQuadIdx = gQuadIndex;
      AddObjPrim6(gGraphicsPtr->ot, sprite, 0);

      if (prevQuadIdx != gQuadIndex) {
         poly = &gGraphicsPtr->quads[gQuadIndex - 1];
         setRGB0(poly, red, green, blue);
      }

      sprite->x1.n = obj->x1.n + OBJ.position8_x * a / ONE;
      sprite->z1.n = obj->z1.n + OBJ.position8_z * a / ONE;
      sprite->y1.n = obj->y1.n + OBJ.position8_y * b / ONE;

      prevQuadIdx = gQuadIndex;
      AddObjPrim6(gGraphicsPtr->ot, sprite, 0);

      if (prevQuadIdx != gQuadIndex) {
         poly = &gGraphicsPtr->quads[gQuadIndex - 1];
         setRGB0(poly, red, green, blue);
      }

      OBJ.theta += (DEG(180) / 32);
      if (OBJ.theta >= DEG(180)) {
         obj->functionIndex = OBJF_NULL;
         sprite->functionIndex = OBJF_NULL;
      }

      break;
   }
}
