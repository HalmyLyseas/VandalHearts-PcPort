/* Dark Hurricane spell effects (Objf283 driver, Objf281_282 vortex, Objf280, Objf389/390/388
 * debris) and Objf285, the casting-pose FX spawned by Objf014 (units/actor.c) -- a stray.
 * Dispatched via gSpellsEx (docs/decomp/spell-fx-dispatch.md); _Unused = cut content. */
#include "common.h"
#include "object.h"
#include "graphics.h"
#include "battle.h"
#include "field.h"
#include "state.h"
#include "units.h"
#include "audio.h"

/* Address-locked rodata: Salamander's local-initializer templates (Objf335/377,
 * spells/salamander.c) sit at the head of this TU's .rodata, ahead of the jumptables, so they
 * stay here as named consts in this order: quad, N-start head table, S-start head table. */
typedef struct { SVECTOR v[4]; } SalamanderQuad;   /* Quad is an array typedef; arrays cannot be
                                                    * copy-assigned, so both templates are struct-
                                                    * wrapped (same bytes, same block-copy codegen). */
typedef struct { s16 v[8]; } SalamanderHeadGfx;
const SalamanderQuad gSalamanderQuadTemplate = {{{-32, -32, 0, 0}, {32, -32, 0, 0}, {-32, 32, 0, 0}, {32, 32, 0, 0}}};
const SalamanderHeadGfx gSalamanderHeadGfxN = {{GFX_SALAMANDER_N, GFX_SALAMANDER_NE, GFX_SALAMANDER_E, GFX_SALAMANDER_SE,
                                                GFX_SALAMANDER_S, GFX_SALAMANDER_SE, GFX_SALAMANDER_E, GFX_SALAMANDER_NE}};
const SalamanderHeadGfx gSalamanderHeadGfxS = {{GFX_SALAMANDER_S, GFX_SALAMANDER_SE, GFX_SALAMANDER_E, GFX_SALAMANDER_NE,
                                                GFX_SALAMANDER_N, GFX_SALAMANDER_NE, GFX_SALAMANDER_E, GFX_SALAMANDER_SE}};

static s16 sFlameAnimData_800ff5f8[20] = {
    0, GFX_FLAME_1, 2, GFX_FLAME_2, 2, GFX_FLAME_3, 2, GFX_FLAME_4, 2, GFX_FLAME_5,
    2, GFX_FLAME_6, 2, GFX_FLAME_7, 2, GFX_FLAME_8, 2, GFX_NULL,    1, GFX_NULL};

static s16 sFlameAnimData_800ff620[20] = {
    0, GFX_FLAME_1, 2, GFX_FLAME_2, 2, GFX_FLAME_3, 2, GFX_FLAME_4, 2, GFX_FLAME_5,
    2, GFX_FLAME_6, 2, GFX_FLAME_7, 2, GFX_FLAME_8, 2, GFX_NULL,    0, GFX_NULL};

static s16 sFlameAnimData_800ff648[20] = {
    7, GFX_FLAME_1, 2, GFX_FLAME_2, 2, GFX_FLAME_3, 2, GFX_FLAME_4, 2, GFX_FLAME_5,
    2, GFX_FLAME_6, 2, GFX_FLAME_7, 2, GFX_FLAME_8, 2, GFX_NULL,    1, GFX_NULL};

// Used by Objf379_EvilStream_Rock
s16 gRockAnimData_800ff670[12] = {5, GFX_ROCK_1, 2, GFX_ROCK_2, 2, GFX_ROCK_3,
                                  2, GFX_ROCK_4, 2, GFX_NULL,   1, GFX_NULL};

s16 gSmokeAnimData_800ff688[24] = {
    4, GFX_PUFF_1, 2, GFX_PUFF_2, 2, GFX_PUFF_3, 2, GFX_PUFF_4,  2, GFX_PUFF_5, 2, GFX_PUFF_6,
    2, GFX_PUFF_7, 2, GFX_PUFF_8, 2, GFX_PUFF_9, 2, GFX_PUFF_10, 2, GFX_NULL,   1, GFX_NULL};

#undef OBJF
#define OBJF 284
void Objf284_ConvergingSparkle_Unused(Object *obj) {
   static s16 sparkleAnimData[14] = {7, GFX_SPARKLE_1, 3, GFX_SPARKLE_2, 3, GFX_SPARKLE_3,
                                     3, GFX_SPARKLE_4, 3, GFX_SPARKLE_5, 3, GFX_NULL,
                                     1, GFX_NULL};

   s32 i;
   SVECTOR *p;
   Quad unused;

   switch (obj->state) {
   case 0:
      OBJ.animData = sparkleAnimData;
      OBJ.boxIdx = 7;
      obj->mem = 4 + rand() % 16;
      obj->x3.n = (obj->x2.n - obj->x1.n) / obj->mem;
      obj->z3.n = (obj->z2.n - obj->z1.n) / obj->mem;
      obj->y3.n = (obj->y2.n - obj->y1.n) / obj->mem;

      p = &OBJ.quad[0];
      for (i = 0; i < 4; i++) {
         p->vx = gQuad_800fe53c[i].vx;
         p->vy = gQuad_800fe53c[i].vy;
         p->vz = gQuad_800fe53c[i].vz;
         p++;
      }

      obj->state2 = 3;
      obj->state++;

   // fallthrough
   case 1:
      OBJ.clut = 3 + obj->mem % 3;
      OBJ.quad[0].vx = -obj->state2;
      OBJ.quad[0].vy = -obj->state2;
      OBJ.quad[1].vx = obj->state2;
      OBJ.quad[1].vy = -obj->state2;
      OBJ.quad[2].vx = -obj->state2;
      OBJ.quad[2].vy = obj->state2;
      OBJ.quad[3].vx = obj->state2;
      OBJ.quad[3].vy = obj->state2;

      p = &OBJ.quad[0];
      for (i = 0; i < 4; i++) {
         gQuad_800fe63c[i].vx = p->vx;
         gQuad_800fe63c[i].vy = p->vy;
         gQuad_800fe63c[i].vz = 0;
         p++;
      }

      UpdateObjAnimation(obj);
      AddObjPrim6(gGraphicsPtr->ot, obj, 0);

      obj->x1.n += (obj->x2.n - obj->x1.n) >> 3;
      obj->z1.n += (obj->z2.n - obj->z1.n) >> 3;
      obj->y1.n += (obj->y2.n - obj->y1.n) >> 3;

      if (--obj->mem <= 0) {
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 286
void Objf286_ShrinkingGroundArc_Unused(Object *obj) {
   s32 radius;

   switch (obj->state) {
   case 0:
      obj->state2 = rand() % DEG(360);
      obj->mem = CV(0.75) + rand() % CV(0.25);
      obj->d.sprite.gfxIdx = GFX_COLOR_15;
      obj->state3 = 8;
      obj->d.sprite.semiTrans = 2;
      obj->state++;

   // fallthrough
   case 1:
      radius = obj->mem;
      obj->d.sprite.coords[0].x = obj->x2.n + (radius * rcos(obj->state2) >> 12);
      obj->d.sprite.coords[0].z = obj->z2.n + (radius * rsin(obj->state2) >> 12);
      obj->d.sprite.coords[1].x = obj->x2.n + (radius * rcos(obj->state2 + 0x10) >> 12);
      obj->d.sprite.coords[1].z = obj->z2.n + (radius * rsin(obj->state2 + 0x10) >> 12);

      radius = obj->mem + ((0 - obj->mem) >> 2);
      obj->d.sprite.coords[2].x = obj->x2.n + (radius * rcos(obj->state2) >> 12);
      obj->d.sprite.coords[2].z = obj->z2.n + (radius * rsin(obj->state2) >> 12);
      obj->d.sprite.coords[3].x = obj->x2.n + (radius * rcos(obj->state2 + 0x10) >> 12);
      obj->d.sprite.coords[3].z = obj->z2.n + (radius * rsin(obj->state2 + 0x10) >> 12);

      obj->d.sprite.coords[0].y = obj->d.sprite.coords[1].y = obj->d.sprite.coords[2].y =
          obj->d.sprite.coords[3].y = obj->y2.n;

      obj->x1.n = obj->d.sprite.coords[0].x;
      obj->z1.n = obj->d.sprite.coords[0].z;
      obj->y1.n = obj->d.sprite.coords[0].y;
      obj->d.sprite.otOfs = -8;
      AddObjPrim5(gGraphicsPtr->ot, obj);

      obj->mem += (0 - obj->mem) >> 3;
      if (obj->mem <= 4) {
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 285
void Objf285_CastingFx(Object *obj) {
   Object *ray;
   s32 i;

   switch (obj->state) {
   case 0:
      SnapToUnit(obj);
      obj->y1.n += CV(0.5);
      obj->state++;
      obj->state2 = 4;
      obj->mem = 0xc0;
      PerformAudioCommand(AUDIO_CMD_PLAY_SFX(230));

   // fallthrough
   case 1:
      if (--obj->state2 <= 0) {
         for (i = 0; i < 3; i++) {
            ray = Obj_GetUnused();
            ray->functionIndex = OBJF_INWARD_RAY;
            ray->x2.n = obj->x1.n;
            ray->z2.n = obj->z1.n;
            ray->y2.n = obj->y1.n;
         }
         obj->state2 = (rand() >> 2) % 3;
      }
      if (gSignal3 == 1) {
         obj->mem = 0x20;
         obj->state++;
      }
      break;

   case 2:
      if (--obj->mem <= 0) {
         PerformAudioCommand(AUDIO_CMD_PLAY_SFX(231));
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 287
void Objf287_CastingFxSpawner_Unused(Object *obj) {
   Object *unitSprite;
   Object *castingFx;

   switch (obj->state) {
   case 0:
      unitSprite = SnapToUnit(obj);
      obj->y1.n += CV(0.5);
      OBJ.unitSprite = unitSprite;
      obj->state3 = 0;
      obj->state++;

      castingFx = Obj_GetUnused();
      castingFx->functionIndex = OBJF_CASTING_FX;
      castingFx->mem = 0x30;
      castingFx->x1.n = obj->x1.n;
      castingFx->z1.n = obj->z1.n;
      castingFx->y1.n = obj->y1.n;

   // fallthrough
   case 1:
      if (gSignal3 == 1) {
         obj->state++;
      }
      break;

   case 2:
      obj->functionIndex = OBJF_NULL;
      break;
   }
}

#undef OBJF
#define OBJF 283
void Objf283_DarkHurricane_Cloud(Object *obj) {
   Object *parent;

   switch (obj->state) {
   case 0:
      OBJ.gfxIdx = GFX_PUFF_1;
      OBJ.boxIdx = 4;
      OBJ.animData = gSmokeAnimData_800ff688;
      obj->mem = 0x100;
      obj->state2 = rand() % DEG(360);
      obj->state++;

   // fallthrough
   case 1:
      parent = OBJ.parent;
      obj->x1.n = parent->x1.n + (obj->mem * rcos(obj->state2) >> 12);
      obj->z1.n = parent->z1.n + (obj->mem * rsin(obj->state2) >> 12);
      obj->y1.n = parent->y1.n + CV(0.5) - (obj->mem / 2);
      obj->mem -= 0x10;
      obj->state2 += 0x40;
      UpdateObjAnimation(obj);
      AddObjPrim6(gGraphicsPtr->ot, obj, 0);
      if (obj->mem <= 0x20) {
         obj->functionIndex = OBJF_NULL;
      }
      if (parent->state == 99) {
         obj->state++;
      }
      break;

   case 2:
      parent = OBJ.parent;
      obj->x3.n = parent->x1.n + (obj->mem * rcos(obj->state2) >> 12);
      obj->z3.n = parent->z1.n + (obj->mem * rsin(obj->state2) >> 12);
      obj->y3.n = parent->y1.n + CV(0.5) - (obj->mem / 2);
      obj->x2.n = obj->x3.n - obj->x1.n;
      obj->z2.n = obj->z3.n - obj->z1.n;
      obj->y2.n = obj->y3.n - obj->y1.n;
      obj->mem = 0;
      obj->state++;

   // fallthrough
   case 3:
      obj->x1.n += obj->x2.n;
      obj->z1.n += obj->z2.n;
      obj->y1.n += obj->y2.n;
      UpdateObjAnimation(obj);
      AddObjPrim6(gGraphicsPtr->ot, obj, 0);
      if (++obj->mem >= 0x40) {
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 281
void Objf281_282_DarkHurricane_FX2_FX3(Object *obj) {
   Object *fxSprite;
   Object *targetSprite;
   Object *obj_v1;
   s16 theta;
   s32 i;
   Quad quad;

   switch (obj->state) {
   case 0:
      obj_v1 = Obj_GetUnused();
      obj_v1->functionIndex = OBJF_NOOP;
      OBJ.sprite = obj_v1;

      targetSprite = SnapToUnit(obj);
      OBJ.targetSprite = targetSprite;

      obj->state3 = 0;
      obj->y1.n = CV(8.0);
      obj->y3.n = -8;
      OBJ.theta = 0;
      obj->state++;

   // fallthrough
   case 1:
      GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
      targetSprite = OBJ.targetSprite;
      targetSprite->d.sprite.hidden = 1;
      fxSprite = OBJ.sprite;
      CopyObject(targetSprite, fxSprite);
      fxSprite->functionIndex = OBJF_NOOP;
      fxSprite->d.sprite.hidden = 0;
      obj->state3++;

      switch (obj->state2) {
      case 0:
         obj->state2++;

      // fallthrough
      case 1:
         obj->y1.n += obj->y2.n;
         obj->y2.n += obj->y3.n;
         OBJ.theta += (obj->y2.n * 8);
         theta = OBJ.theta;

         if (GetTerrainElevation(obj->z1.s.hi, obj->x1.s.hi) > obj->y1.n) {
            obj_v1 = Obj_GetUnused();
            obj_v1->functionIndex = OBJF_BOUNCE_ZOOM;
            obj_v1 = Obj_GetUnused();
            obj_v1->functionIndex = OBJF_DISPLAY_DAMAGE_2;
            obj_v1->x1.n = obj->x1.n;
            obj_v1->z1.n = obj->z1.n;
            obj->y1.n = GetTerrainElevation(obj->z1.s.hi, obj->x1.s.hi);
            obj->y2.n = -(obj->y2.n / 2);
            obj->state2++;
            obj->state3 = 0;
         }
         break;

      case 2:
         obj->y1.n += obj->y2.n;
         obj->y2.n += obj->y3.n;
         OBJ.theta += (0 - OBJ.theta) >> 2;
         theta = OBJ.theta;

         if (GetTerrainElevation(obj->z1.s.hi, obj->x1.s.hi) > obj->y1.n) {
            obj->y1.n = GetTerrainElevation(obj->z1.s.hi, obj->x1.s.hi);
            obj->y2.n = 0;
            obj->y3.n = 0;
            fxSprite->functionIndex = OBJF_NULL;
            targetSprite->d.sprite.hidden = 0;
            obj->state3 = 0;

            if (obj->functionIndex == OBJF_DARK_HURRICANE_FX2) {
               obj->state += 1;
            } else if (obj->functionIndex == OBJF_DARK_HURRICANE_FX3) {
               obj->state += 2;
            }
         }

         break;
      }

      fxSprite->y1.n = obj->y1.n;
      fxSprite->x1.n = obj->x1.n;
      fxSprite->z1.n = obj->z1.n;

      for (i = 0; i < 4; i++) {
         quad[i] = gQuad_800fe53c[i];
      }

      for (i = 0; i < 4; i++) {
         gQuad_800fe53c[i].vx = (quad[i].vx * rcos(theta) - (quad[i].vy + 8) * rsin(theta)) >> 12;
         gQuad_800fe53c[i].vy = (quad[i].vx * rsin(theta) + (quad[i].vy + 8) * rcos(theta)) >> 12;
         gQuad_800fe53c[i].vz = 0;
      }

      RenderUnitSprite(gGraphicsPtr->ot, fxSprite, 0);

      for (i = 0; i < 4; i++) {
         gQuad_800fe53c[i] = quad[i];
      }

      break;

   case 2:
      if (++obj->state3 >= 15) {
         obj->functionIndex = OBJF_NULL;
         gSignal3 = 1;
      }
      break;

   case 3:
      if (++obj->state3 >= 10) {
         obj->state3 = 0;
         obj->state++;
      }
      break;

   case 4:
      obj_v1 = Obj_GetUnused();
      obj_v1->functionIndex = OBJF_STRETCH_WARP_SPRITE;
      obj_v1->x1.n = obj->x1.n;
      obj_v1->z1.n = obj->z1.n;
      obj_v1->y1.n = obj->y1.n;
      obj->state++;

   // fallthrough
   case 5:
      if (++obj->state3 >= 15) {
         obj->functionIndex = OBJF_NULL;
         gSignal3 = 1;
      }
      break;
   }
}

#undef OBJF
#define OBJF 280
void Objf280_DarkHurricane_Target(Object *obj) {
   Object *fxSprite; // copy of unit sprite to toss around
   Object *targetSprite;
   Object *obj_s1;
   Object *vortex;
   s32 i;
   s16 theta;
   Quad quad;

   switch (obj->state) {
   case 0:
      obj_s1 = Obj_GetUnused();
      obj_s1->functionIndex = OBJF_NOOP;
      OBJ.sprite = obj_s1;
      obj_s1 = OBJ.unitSprite = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
      obj->state3 = rand() % 0x20;
      obj->x1.n = obj_s1->x1.n;
      obj->z1.n = obj_s1->z1.n;
      obj->y1.n = GetTerrainElevation(obj->z1.s.hi, obj->x1.s.hi);
      obj->y3.n = -8;
      OBJ.theta = 0;
      obj->state++;

   // fallthrough
   case 1:
      targetSprite = OBJ.unitSprite;
      targetSprite->d.sprite.hidden = 1;
      fxSprite = OBJ.sprite;
      CopyObject(targetSprite, fxSprite);
      fxSprite->functionIndex = OBJF_NOOP;
      fxSprite->d.sprite.hidden = 0;
      obj->state3++;
      OBJ.theta += obj->y2.n * 8;
      theta = OBJ.theta;

      switch (obj->state2) {
      case 0:
         vortex = OBJ.parent;
         if (vortex->state == 99) {
            obj->y2.n = CV(-0.5);
            obj->state2++;
            obj->state3 = 0;
         } else {
            obj->y1.n += CV(0.03125);
            if (obj->y1.n >= CV(4.0)) {
               obj->y1.n = CV(4.0);
            }
            obj->x1.n = vortex->x1.n + (CV(0.5) * rcos(obj->state3 * 0x80) >> 12);
            obj->z1.n = vortex->z1.n + (CV(0.5) * rsin(obj->state3 * 0x80) >> 12);
            obj->y2.n = CV(1.75);
         }
         break;

      case 1:
         obj->y1.n += obj->y2.n;
         obj->y2.n += obj->y3.n;
         fxSprite->functionIndex = OBJF_NULL;
         obj->functionIndex = OBJF_NULL;
         break;
      }

      fxSprite->y1.n = obj->y1.n;
      fxSprite->x1.n = obj->x1.n;
      fxSprite->z1.n = obj->z1.n;

      for (i = 0; i < 4; i++) {
         quad[i] = gQuad_800fe53c[i];
      }

      for (i = 0; i < 4; i++) {
         gQuad_800fe53c[i].vx =
             (quad[i].vx * rcos(theta) - (quad[i].vy + 8) * rsin(theta)) >> 1 >> 12;
         gQuad_800fe53c[i].vy =
             (quad[i].vx * rsin(theta) + (quad[i].vy + 8) * rcos(theta)) >> 1 >> 12;
         gQuad_800fe53c[i].vz = 0;
      }

      RenderUnitSprite(gGraphicsPtr->ot, fxSprite, 0);

      for (i = 0; i < 4; i++) {
         gQuad_800fe53c[i] = quad[i];
      }

      break;
   }
}

/* UNREACHABLE: not in gObjFunctionPointers[]. Salamander head + per-frame screen-space
 * red-spark wedge for 128 frames, then gSignal3 -- driver-shaped test code. */
void Objf_Unk_80089298(Object *obj) {
   Object *sprite;
   POLY_FT4 *poly;
   s32 randomAngle;

   switch (obj->state) {
   case 0:
      SnapToUnit(obj);
      obj->d.sprite.gfxIdx = GFX_SALAMANDER_S;
      obj->d.sprite.boxIdx = 4;
      obj->d.sprite.clut = CLUT_REDS;
      obj->state2 = 0x80;
      obj->state++;

   // fallthrough
   case 1:
      AddObjPrim6(gGraphicsPtr->ot, obj, 0);
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      obj->x2.n = (poly->x0 + poly->x1) / 2;
      obj->y2.n = (poly->y0 + poly->y2) / 2;

      sprite = Obj_GetUnused();
      sprite->d.sprite.gfxIdx = GFX_TILED_RED_SPARKLES;
      sprite->d.sprite.boxIdx = 4;
      sprite->d.sprite.clut = CLUT_BLUES;
      sprite->x1.n = obj->x1.n;
      sprite->z1.n = obj->z1.n;
      sprite->y1.n = obj->y1.n;
      sprite->d.sprite.semiTrans = 1;
      AddObjPrim6(gGraphicsPtr->ot, sprite, 0);
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      poly->x0 = poly->x1 = obj->x2.n;
      poly->y0 = poly->y1 = obj->y2.n;

      randomAngle = (rand() >> 2) % DEG(360);
      poly->x2 = poly->x0 + (0x100 * rcos(randomAngle) >> 12);
      poly->y2 = poly->y0 + (0x100 * rsin(randomAngle) >> 12);
      poly->x3 = poly->x0 + (0x100 * rcos(randomAngle + 0x20) >> 12);
      poly->y3 = poly->y0 + (0x100 * rsin(randomAngle + 0x20) >> 12);

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
#define OBJF 389
void Objf389_DarkHurricane_Vortex(Object *obj) {
   Object *parent;
   Object *sprite;

   parent = OBJ.parent;

   switch (obj->state) {
   case 0:
      sprite = Obj_GetUnused();
      sprite->functionIndex = OBJF_NOOP;
      sprite->d.sprite.gfxIdx = GFX_EXPLOSION_1;
      sprite->d.sprite.boxIdx = 5;
      OBJ.sprite = sprite;

      obj->x2.n = obj->x1.n;
      obj->z2.n = obj->z1.n;
      obj->y2.n = obj->y1.n;
      obj->state2 = 0;
      obj->state++;

   // fallthrough
   case 1:
      obj->state2 = parent->state2;
      obj->x1.n = obj->x2.n + (0x80 * rcos(obj->state3 * 0x10) >> 12);
      obj->z1.n = obj->z2.n + (0x80 * rcos(obj->state3 * 0x10) >> 12);
      obj->y1.n = obj->y2.n;
      obj->state3++;

      sprite = OBJ.sprite;
      sprite->x1.n = obj->x1.n;
      sprite->z1.n = obj->z1.n;
      sprite->y1.n = obj->y1.n;

      OBJ.theta += 0x40;

      if (parent->state == 99) {
         obj->state3 = 0;
         obj->state = 99;
      }
      break;

   case 99:
      if (++obj->state3 >= 2) {
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 390
void Objf390_DarkHurricane_VortexLayer(Object *obj) {
   Object *parent;
   Object *sprite;
   s32 i;
   s32 startingTheta;
   s32 parentTheta;
   s32 radius;
   s32 parentRadius;
   POLY_FT4 *poly;
   s32 increment;

   parent = OBJ.parent;

   switch (obj->state) {
   case 0:
      sprite = Obj_GetUnused();
      sprite->functionIndex = OBJF_NOOP;
      sprite->d.sprite.gfxIdx = GFX_EXPLOSION_1;
      sprite->d.sprite.boxIdx = 5;
      OBJ.sprite = sprite;

      OBJ.height = 0;
      OBJ.radius2 = 0;
      OBJ.theta2 = (rand() >> 2) % DEG(360);
      OBJ.maxRadius = OBJ.radius;
      OBJ.radius = 0;
      obj->state3 = 0;
      obj->state++;

   // fallthrough
   case 1:

      switch (obj->state2) {
      case 0:
         OBJ.height += 2;
         OBJ.radius2 = 0x10 + (0x30 * rcos(obj->state3 * 0x40) >> 12);
         OBJ.radius += 8;
         if (OBJ.radius > OBJ.maxRadius) {
            OBJ.radius = OBJ.maxRadius;
         }
         if (OBJ.height >= CV(0.375)) {
            OBJ.height = CV(0.375);
         }
         if (parent->state2 == 1) {
            obj->state2++;
            OBJ.radius = OBJ.maxRadius;
         }
         break;

      case 1:
         obj->state3++;
         OBJ.radius2 = 0x10 + (0x30 * rcos(obj->state3 * 0x40) >> 12);
         OBJ.height = 0x60 + (0x20 * rsin(obj->state3 * 0x20) >> 12);
         if (parent->state2 == 2) {
            obj->state2++;
         }
         break;

      case 2:
         OBJ.height += 2;
         OBJ.radius += (0 - OBJ.radius) >> 3;
         OBJ.radius2 += (0 - OBJ.radius2) >> 2;
         if (parent->state2 == 3) {
            obj->state2++;
         }
         break;
      }

      obj->x1.n = parent->x1.n + (OBJ.radius2 * rcos(OBJ.theta2) >> 12);
      obj->z1.n = parent->z1.n + (OBJ.radius2 * rsin(OBJ.theta2) >> 12);
      obj->y1.n = parent->y1.n + OBJ.height;

      OBJ.theta2 += DEG(2.8125);

      sprite = OBJ.sprite;
      startingTheta = OBJ.theta = parent->d.objf389.theta;
      parentTheta = startingTheta;
      radius = OBJ.radius;
      parentRadius = parent->d.objf389.radius;

      sprite->d.sprite.gfxIdx = GFX_TILED_LINES;
      sprite->d.sprite.clut = CLUT_GRAYS;
      sprite->d.sprite.semiTrans = 1;

      sprite->d.sprite.coords[0].x = obj->x1.n + (radius * rcos(startingTheta) >> 12);
      sprite->d.sprite.coords[0].z = obj->z1.n + (radius * rsin(startingTheta) >> 12);
      // Connect to previous layer (parent)
      sprite->d.sprite.coords[2].x = parent->x1.n + (parentRadius * rcos(parentTheta) >> 12);
      sprite->d.sprite.coords[2].z = parent->z1.n + (parentRadius * rsin(parentTheta) >> 12);
      sprite->d.sprite.coords[0].y = sprite->d.sprite.coords[1].y = obj->y1.n;
      sprite->d.sprite.coords[2].y = sprite->d.sprite.coords[3].y = parent->y1.n;

      increment = DEG(360) / 16;

      for (i = 0; i < 16; i++) {
         sprite->d.sprite.coords[1].x =
             obj->x1.n + (radius * rcos(startingTheta + increment * (i + 1)) >> 12);
         sprite->d.sprite.coords[1].z =
             obj->z1.n + (radius * rsin(startingTheta + increment * (i + 1)) >> 12);
         sprite->d.sprite.coords[3].x =
             parent->x1.n + (parentRadius * rcos(parentTheta + increment * (i + 1)) >> 12);
         sprite->d.sprite.coords[3].z =
             parent->z1.n + (parentRadius * rsin(parentTheta + increment * (i + 1)) >> 12);

         sprite->x1.n = sprite->d.sprite.coords[3].x;
         sprite->z1.n = sprite->d.sprite.coords[3].z;
         sprite->y1.n = sprite->d.sprite.coords[3].y;

         AddObjPrim3(gGraphicsPtr->ot, sprite);
         poly = &gGraphicsPtr->quads[gQuadIndex - 1];
         setRGB0(poly, 0x64, 0x64, 0x64);

         sprite->d.sprite.coords[0].x = sprite->d.sprite.coords[1].x;
         sprite->d.sprite.coords[0].z = sprite->d.sprite.coords[1].z;
         sprite->d.sprite.coords[2].x = sprite->d.sprite.coords[3].x;
         sprite->d.sprite.coords[2].z = sprite->d.sprite.coords[3].z;
      }

      if (parent->state == 99) {
         obj->state = 99;
      }
      break;

   case 99:
      obj->functionIndex = OBJF_NULL;
      break;
   }
}

#undef OBJF
#define OBJF 388
void Objf388_DarkHurricane_FX1(Object *obj) {
   Object *obj_s2;
   Object *obj_a0;
   BVectorXZ *p;
   s32 i;

   switch (obj->state) {
   case 0:
      obj->z1.s.hi = gTargetZ;
      obj->x1.s.hi = gTargetX;
      obj->y1.n = GetTerrainElevation(gTargetZ, gTargetX);

      obj_s2 = Obj_GetUnused();
      obj_s2->functionIndex = OBJF_DARK_HURRICANE_VORTEX;
      obj_s2->x1.n = obj->x1.n;
      obj_s2->z1.n = obj->z1.n;
      obj_s2->y1.n = GetTerrainElevation(obj->z1.s.hi, obj->x1.s.hi);
      obj_s2->d.objf389.parent = obj;
      OBJ.vortex = obj_s2;

      p = (BVectorXZ *)gTargetCoords;
      obj_a0 = GetUnitSpriteAtPosition(p->z, p->x);
      obj->x2.n = obj_a0->x1.n;
      obj->z2.n = obj_a0->z1.n;
      obj->y2.n = obj_a0->y1.n + CV(0.5);
      while (p->x != 0xff) {
         obj_a0 = Obj_GetUnused();
         obj_a0->functionIndex = OBJF_DARK_HURRICANE_TARGET;
         obj_a0->z1.s.hi = p->z;
         obj_a0->x1.s.hi = p->x;
         obj_a0->d.objf280.parent = obj_s2;
         p++;
      }

      for (i = 0; i < 8; i++) {
         obj_a0 = Obj_GetUnused();
         obj_a0->functionIndex = OBJF_DARK_HURRICANE_VORTEX_LAYER;
         obj_a0->d.objf390.parent = obj_s2;
         obj_a0->d.objf390.radius = (i + 1) * 0x40;
         obj_a0->d.objf390.unused_0x38 = i * 0x20;
         obj_s2 = obj_a0;
      }

      obj->state++;
      obj->state3 = 0x100;
      obj->mem = 0;
      obj->state2 = 0;
      break;

   case 1:
      gCameraZoom.vz += (400 - gCameraZoom.vz) >> 3;
      gCameraRotation.vy += 0x10;
      PanCamera(obj->x2.n, obj->y2.n, obj->z2.n, 2);

      if (--obj->mem <= 0) {
         obj_s2 = Obj_GetUnused();
         obj_s2->functionIndex = OBJF_DARK_HURRICANE_CLOUD;
         obj_s2->d.objf283.parent = OBJ.vortex;
         obj->mem = rand() % 3;
      }

      obj->state3--;

      switch (obj->state2) {
      case 0:
         if (obj->state3 <= 0xc0) {
            obj->state2++;
         }
         break;
      case 1:
         if (obj->state3 <= 0x20) {
            obj->state2++;
         }
         break;
      case 2:
         if (obj->state3 <= 8) {
            obj->state2++;
         }
         break;
      }

      if (obj->state3 <= 0) {
         obj->state3 = 0;
         obj->state = 99;
      }
      break;

   case 99:
      if (++obj->state3 >= 2) {
         obj->functionIndex = OBJF_NULL;
         gSignal3 = 1;
      }
      break;
   }
}

#undef OBJF
#define OBJF 392
/* Non-drawing 9x9 lattice vertex: flat layout -> neighbour relaxation -> sphere mapping,
 * driven by a controller object whose handler does not exist in retail. Cut content. */
void Objf392_MorphMeshNode_Unused(Object *obj) {
   Object *obj_s1;
   Object *obj1;
   Object *obj2;
   Object *obj3;
   Object *obj4;
   s32 radius;
   s8 unused[24];

   // Not enough context to identify which objf to use for this.
   obj_s1 = OBJ.controller;

   if (obj_s1->d.objfUnkUsedBy392.killFlag == 99) {
      obj->functionIndex = OBJF_NULL;
      return;
   }

   obj1 = OBJ.neighborY0;
   obj2 = OBJ.neighborY1;
   obj3 = OBJ.neighborX0;
   obj4 = OBJ.neighborX1;

   switch (obj_s1->d.objfUnkUsedBy392.mode) {
   case 0:
      obj->x1.n = obj->x3.n * 12 - 0x30;
      obj->y1.n = obj->y3.n * 12 - 0x30;
      if (obj->y1.n == 0) {
         obj->state3 = 0x200 + (8 - obj->x1.n) * 0x71;
      } else if (obj->y1.n == 8) {
         obj->state3 = -0x200 - (8 - obj->x1.n) * 0x71;
      }
      if (obj->x1.n == 0) {
         obj->state3 = obj->y1.n * 0x71 + 0x600;
      } else if (obj->x1.n == 8) {
         obj->state3 = -0x188;
      }
      break;

   case 1:
      if (obj1 != NULL && obj2 != NULL) {
         obj->y1.n += ((obj1->y1.n + obj2->y1.n) / 2 - obj->y1.n) >> 2;
      }
      if (obj3 != NULL && obj4 != NULL) {
         obj->x1.n += ((obj3->x1.n + obj4->x1.n) / 2 - obj->x1.n) >> 2;
      }
      if (obj3 == NULL || obj4 == NULL || obj1 == NULL || obj2 == NULL) {
         obj->x1.n += 4 * rcos(0x800 * rsin(obj_s1->d.objfUnkUsedBy392.theta) >> 12) >> 12;
         obj->y1.n += 4 * rsin(0x800 * rsin(obj_s1->d.objfUnkUsedBy392.theta) >> 12) >> 12;
      }
      break;

   case 2:
      if (obj->y3.n == 0 || obj->y3.n == 8) {
         radius = 0;
      } else {
         radius = 0x100 * rsin(obj->y3.n * 0x100) >> 12;
      }
      obj->x1.n = -(radius * rcos(obj->x3.n * 0x200) >> 12);
      obj->z1.n = -(radius * rsin(obj->x3.n * 0x200) >> 12);
      obj->y1.n = (8 - obj->y3.n) * 0x40;
      break;
   }
}

#undef OBJF
#define OBJF Unk8008a364
/* UNREACHABLE: not in gObjFunctionPointers[]. Camera-spinning disc that re-samples the
 * framebuffer through its own quad (GetTPage(2, ...) + flipped verts); never terminates
 * -- an abandoned mirror/reflection experiment. */
void Objf_Unk_8008a364(Object *obj) {
   Object *sprite;
   POLY_FT4 *poly;
   s32 clipX, clipY;
   s32 swap;
   u8 unused[32];

   switch (obj->state) {
   case 0:
      SnapToUnit(obj);
      sprite = Obj_GetUnused();
      sprite->functionIndex = OBJF_NOOP;
      sprite->x1.n = obj->x1.n;
      sprite->z1.n = obj->z1.n;
      sprite->y1.n = obj->y1.n + CV(2.0);
      OBJ.sprite = sprite;
      sprite->d.sprite.gfxIdx = GFX_LARGE_RED_CIRCLE;
      sprite->d.sprite.boxIdx = 3;
      obj->state++;

   // fallthrough
   case 1:
      sprite = OBJ.sprite;
      clipX = gGraphicsPtr->drawEnv.clip.x;
      clipY = gGraphicsPtr->drawEnv.clip.y;
      gCameraRotation.vy += 1;

      AddObjPrim6(gGraphicsPtr->ot, sprite, 0);
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      poly->u0 = poly->x2;
      poly->v0 = poly->y2;
      poly->u1 = poly->x3;
      poly->v1 = poly->y3;
      poly->u2 = poly->x2 + 0xe0;
      poly->v2 = 0xff;
      poly->u3 = poly->x3 + 0x20;
      poly->v3 = 0xff;

      swap = poly->x0;
      poly->x0 = poly->x2;
      poly->x2 = swap;
      swap = poly->x1;
      poly->x1 = poly->x3;
      poly->x3 = swap;
      swap = poly->y0;
      poly->y0 = poly->y2;
      poly->y2 = swap;
      swap = poly->y1;
      poly->y1 = poly->y3;
      poly->y3 = swap;

      poly->tpage = GetTPage(2, 0, clipX, clipY);
      break;
   }
}

