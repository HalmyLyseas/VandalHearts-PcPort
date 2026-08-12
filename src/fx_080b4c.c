/* Event/map ambience and the generic particle library (segment 0x80b4c): rain, ripples,
 * campfire, chimney smoke, dusk tint; the generic particles (Objf710/739) and their
 * emitters (projectile trails 764-769); item-spell dispatch (770-789); and assorted battle
 * FX (level-up, stat raised, faint, Phase Shift, Wyrmfang). Ambience objects are spawned
 * by event scripts (opcode 0x1d in EVDATA*.DAT names an objf index directly), which is why
 * several handlers here have no C spawn site. Objf790 (an ember-puff emitter, the only
 * spawner of OBJF_PARTICLE_739) appears in no retail event script -- cut content. */
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
   // 306: Extra Healing, Hyper Healing
   // 791: Healing Plus
   // 792: Ultra Healing
   // 793: Supreme Healing, Holy H2O
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

#undef OBJF
#define OBJF 289
void Objf289_ChimneySmokeRing(Object *obj) {
   s16 timer;
   s16 halfSize;
   s16 prevQuadIdx;
   s16 fade;
   POLY_FT4 *poly;
   s32 rgb;

   switch (obj->state) {
   case 0:
      obj->d.sprite.gfxIdx = GFX_SWIRLY_RING;
      obj->d.sprite.boxIdx = 7;
      obj->d.sprite.clut = CLUT_GRAYS;
      obj->d.sprite.semiTrans = 2;
      obj->d.sprite.coords[0].x = obj->d.sprite.coords[1].x = obj->x1.n - CV(0.125);
      obj->d.sprite.coords[2].x = obj->d.sprite.coords[3].x = obj->x1.n + CV(0.125);
      obj->d.sprite.coords[0].z = obj->d.sprite.coords[2].z = obj->z1.n - CV(0.125);
      obj->d.sprite.coords[1].z = obj->d.sprite.coords[3].z = obj->z1.n + CV(0.125);
      obj->state++;

   // fallthrough
   case 1:
      timer = obj->state2 * 2;

      halfSize = timer + CV(0.125);
      obj->d.sprite.coords[0].x = obj->d.sprite.coords[1].x = obj->x1.n - halfSize;
      obj->d.sprite.coords[2].x = obj->d.sprite.coords[3].x = obj->x1.n + halfSize;
      obj->d.sprite.coords[0].z = obj->d.sprite.coords[2].z = obj->z1.n - halfSize;
      obj->d.sprite.coords[1].z = obj->d.sprite.coords[3].z = obj->z1.n + halfSize;

      obj->y1.n += obj->y2.n;
      obj->d.sprite.coords[0].y = obj->d.sprite.coords[1].y = obj->d.sprite.coords[2].y =
          obj->d.sprite.coords[3].y = obj->y1.n;

      prevQuadIdx = gQuadIndex;
      AddObjPrim4(gGraphicsPtr->ot, obj);

      if (prevQuadIdx != gQuadIndex) {
         poly = &gGraphicsPtr->quads[gQuadIndex - 1];
         fade = (0x80 - timer) - timer;
         rgb = (fade << 16) + (fade << 8) + fade;
         *(u32 *)&poly->r0 = ((GPU_CODE_POLY_FT4 | GPU_CODE_SEMI_TRANS) << 24) + rgb;
      }

      if (++obj->state2 >= 32) {
         obj->functionIndex = OBJF_NULL;
      }

      break;
   }
}

#undef OBJF
#define OBJF 302
void Objf302_ChimneySmoke(Object *obj) {
   Object *ring;

   switch (obj->state) {
   case 0:
      obj->y1.n = GetMapModelElevation(obj->z1.s.hi, obj->x1.s.hi) + CV(0.25);
      obj->state++;
      obj->state2 = 0;

   // fallthrough
   case 1:
      if (++obj->state2 > 27) {
         ring = CreatePositionedObj(obj, OBJF_CHIMNEY_SMOKE_RING);
         ring->y2.n = 6;
         obj->state2 = 0;
      }
      break;
   }
}

#undef OBJF
#define OBJF 675
void Objf675_LeenaForcefield(Object *obj) {
   // Spawned by: EVDATA74.DAT->271->675, SetupMapExtras()->256->675
   static u8 cluts[4] = {CLUT_REDS, CLUT_BLUES, CLUT_PURPLES, CLUT_GREENS};

   Object *targetSprite;
   s32 i;
   s32 x, z;
   s32 currentSize;
   s32 halfSize;
   s32 theta;

   switch (obj->state) {
   case 0:
      obj->state++;

   // fallthrough
   case 1:
      targetSprite = OBJ.targetSprite;
      obj->x1.n = targetSprite->x1.n;
      obj->z1.n = targetSprite->z1.n;
      obj->y1.n = GetTerrainElevation(obj->z1.s.hi, obj->x1.s.hi);
      OBJ.targetSprite = NULL;
      OBJ.gfxIdx = GFX_RING;
      OBJ.boxIdx = 7;
      OBJ.clut = CLUT_BLUES;
      OBJ.semiTrans = 4;
      obj->y2.n = obj->y1.n;
      obj->state++;

   // fallthrough
   case 2:
      theta = obj->mem;
      x = obj->x1.n;
      z = obj->z1.n;

      obj->state3 += (CV(2.0) - obj->state3) >> 4;
      currentSize = obj->state3;
      OBJ.clut = cluts[(obj->mem >> 4) & 3];

      for (i = 0; i < 4; i++) {
         halfSize = currentSize * rcos(theta) >> 12;
         OBJ.coords[0].y = OBJ.coords[1].y = OBJ.coords[2].y = OBJ.coords[3].y =
             obj->y2.n + (CV(2.0) * rsin(theta) >> 12);

         OBJ.coords[0].x = x + halfSize;
         OBJ.coords[0].z = z;
         OBJ.coords[1].x = x;
         OBJ.coords[1].z = z + halfSize;
         OBJ.coords[2].x = x;
         OBJ.coords[2].z = z - halfSize;
         OBJ.coords[3].x = x - halfSize;
         OBJ.coords[3].z = z;

         obj->y1.n = OBJ.coords[0].y;
         AddObjPrim3(gGraphicsPtr->ot, obj, 0); //? Extra arg

         theta += DEG(22.5);
         if (theta >= DEG(90)) {
            theta -= DEG(90);
         }
      }

      obj->mem += DEG(1.58203125);
      if (obj->mem >= DEG(90)) {
         obj->mem = 0;
      }

      break;
   }
}

#undef OBJF
#define OBJF 676
void Objf676_687_Rainfall(Object *obj) {
   // Spawned by: EVDATA86.DAT, EVDATA88.DAT, SetupMapExtras()
   s32 i;
   Object *raindrop;

   switch (obj->state) {
   case 0:
      if (obj->functionIndex == OBJF_LIGHT_RAINFALL) {
         obj->state2 = 1;
      }
      obj->state++;

   // fallthrough
   case 1:
      if (--obj->mem <= 0) {
         for (i = 0; i < 8; i++) {
            raindrop = Obj_GetUnused();
            raindrop->functionIndex = OBJF_RAINFALL_DROP;
            raindrop->x1.n = rand() % CV(24.0);
            raindrop->z1.n = rand() % CV(12.0);
            raindrop->y1.n = rand() % CV(1.0) + CV(6.0);
         }
         if (obj->state2 != 0) {
            // OBJF_LIGHT_RAINFALL
            obj->mem += (rand() % 4 + 2);
         } else {
            // OBJF_HEAVY_RAINFALL
            obj->mem += (rand() % 2 + 1);
         }
      }

   case 2:
      break;
   }
}

#undef OBJF
#define OBJF 677
void Objf677_RainfallDrop(Object *obj) {
   switch (obj->state) {
   case 0:
      obj->d.sprite.gfxIdx = GFX_COLORS;
      obj->d.sprite.clut = CLUT_BLUES;
      obj->d.sprite.coords[0].x = obj->d.sprite.coords[1].x = obj->d.sprite.coords[2].x =
          obj->d.sprite.coords[3].x = obj->x1.n;
      obj->d.sprite.coords[0].z = obj->d.sprite.coords[1].z = obj->d.sprite.coords[2].z =
          obj->d.sprite.coords[3].z = obj->z1.n;
      obj->d.sprite.coords[0].x = obj->x1.n + 4;
      obj->d.sprite.coords[1].x = obj->x1.n + 4;
      obj->d.sprite.coords[2].x = obj->x1.n - 4;
      obj->d.sprite.coords[3].x = obj->x1.n - 4;
      obj->d.sprite.semiTrans = 4;
      obj->d.sprite.coords[0].y = obj->d.sprite.coords[2].y =
          obj->y1.n + (rand() % CV(0.25) + CV(1.0));
      obj->d.sprite.coords[1].y = obj->d.sprite.coords[3].y = obj->y1.n;
      obj->y2.n = CV(-0.5) - rand() % CV(0.25);
      obj->y3.n = GetTerrainElevation(obj->z1.s.hi, obj->x1.s.hi);
      obj->state++;
      break;

   case 1:
      obj->d.sprite.coords[0].y += obj->y2.n;
      obj->d.sprite.coords[1].y += obj->y2.n;
      obj->d.sprite.coords[2].y += obj->y2.n;
      obj->d.sprite.coords[3].y += obj->y2.n;
      AddObjPrim3(gGraphicsPtr->ot, obj);
      obj->y1.n += obj->y2.n;
      if (obj->y1.n <= obj->y3.n) {
         obj->state++;
      }
      break;

   case 2:
      if ((obj->x1.s.hi < gMapViewOriginX) || (obj->x1.s.hi > gMapSizeX + gMapViewOriginX - 1) ||
          (obj->z1.s.hi < gMapViewOriginZ) || (obj->z1.s.hi > gMapSizeZ + gMapViewOriginZ - 1)) {
         obj->functionIndex = OBJF_NULL;
         break;
      }

      if ((OBJ_TERRAIN(obj).s.terrain == TERRAIN_WATER)) {
         CreatePositionedObj(obj, OBJF_RIPPLE);
      }

      obj->functionIndex = OBJF_NULL;
      break;
   }
}

#undef OBJF
#define OBJF 678
void Objf678_Ripple(Object *obj) {
   s16 timer;
   s16 halfSize;
   s16 prevQuadIdx;
   s16 fade;
   POLY_FT4 *poly;
   s32 rgb;

   switch (obj->state) {
   case 0:
      obj->d.sprite.gfxIdx = GFX_RING;
      obj->d.sprite.clut = CLUT_GRAYS;
      obj->d.sprite.semiTrans = 2;
      obj->d.sprite.coords[0].x = obj->d.sprite.coords[1].x = obj->x1.n - CV(0.125);
      obj->d.sprite.coords[2].x = obj->d.sprite.coords[3].x = obj->x1.n + CV(0.125);
      obj->d.sprite.coords[0].z = obj->d.sprite.coords[2].z = obj->z1.n - CV(0.125);
      obj->d.sprite.coords[1].z = obj->d.sprite.coords[3].z = obj->z1.n + CV(0.125);
      obj->d.sprite.otOfs = 8;
      obj->y1.n = GetTerrainElevation(obj->z1.s.hi, obj->x1.s.hi) + CV(0.015625);
      obj->state++;

   // fallthrough
   case 1:
      timer = obj->state2 * 2;

      halfSize = timer + CV(0.125);
      obj->d.sprite.coords[0].x = obj->d.sprite.coords[1].x = obj->x1.n - halfSize;
      obj->d.sprite.coords[2].x = obj->d.sprite.coords[3].x = obj->x1.n + halfSize;
      obj->d.sprite.coords[0].z = obj->d.sprite.coords[2].z = obj->z1.n - halfSize;
      obj->d.sprite.coords[1].z = obj->d.sprite.coords[3].z = obj->z1.n + halfSize;

      obj->y1.n += obj->y2.n;
      obj->d.sprite.coords[0].y = obj->d.sprite.coords[1].y = obj->d.sprite.coords[2].y =
          obj->d.sprite.coords[3].y = obj->y1.n;

      prevQuadIdx = gQuadIndex;
      AddObjPrim4(gGraphicsPtr->ot, obj);

      if (prevQuadIdx != gQuadIndex) {
         poly = &gGraphicsPtr->quads[gQuadIndex - 1];
         fade = (0x80 - timer) - timer;
         rgb = (fade << 16) + (fade << 8) + fade;
         *(u32 *)&poly->r0 = ((GPU_CODE_POLY_FT4 | GPU_CODE_SEMI_TRANS) << 24) + rgb;
      }

      if (++obj->state2 >= 32) {
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

static Quad sQuad_800ff8f4 = {{-24, 0, -24, 0}, {24, 0, -24, 0}, {-24, 0, 24, 0}, {24, 0, 24, 0}};

#undef OBJF
#define OBJF 692
void Objf692_Campfire(Object *obj) {
   // Spawned by: EVDATA25.DAT, EVDATA67.DAT
   static s16 animData[12] = {4, GFX_CAMPFIRE_1, 2, GFX_CAMPFIRE_2, 2, GFX_CAMPFIRE_3,
                              2, GFX_CAMPFIRE_4, 2, GFX_NULL,       1, GFX_NULL};

   Object *sprite;
   Object *entitySprite;

   switch (obj->state) {
   case 0:
      sprite = Obj_GetUnused();
      sprite->functionIndex = OBJF_NOOP;
      OBJ.sprite = sprite;
      entitySprite = OBJ.entitySprite;
      sprite->x1.n = entitySprite->x1.n;
      sprite->z1.n = entitySprite->z1.n;
      sprite->d.sprite.animData = animData;
      obj->state++;

   // fallthrough
   case 1:
      sprite = OBJ.sprite;
      UpdateObjAnimation(sprite);
      AddObjPrim6(gGraphicsPtr->ot, sprite, 1);
      break;
   }
}

#undef OBJF
#define OBJF 763
void Objf763_BoulderRubble(Object *obj) {
   s32 i;

   for (i = 0; i < 32; i++) {
      CreatePositionedObj(obj, OBJF_ROCK_SPURT_PARTICLE);
      CreatePositionedObj(obj, OBJF_RUBBLE);
   }

   obj->functionIndex = OBJF_NULL;
}

#undef OBJF
#define OBJF 759
void Objf759_RockSpurtParticle(Object *obj) {
   static s16 rockAnimData[12] = {5, GFX_ROCK_1, 3, GFX_ROCK_2, 3, GFX_ROCK_3,
                                  3, GFX_ROCK_4, 3, GFX_NULL,   1, GFX_NULL};

   s16 elevation;
   s16 prevQuadIdx;
   POLY_FT4 *poly;
   s32 theta;
   s32 radius;

   switch (obj->state) {
   case 0:
      obj->d.sprite.animData = rockAnimData;
      theta = rand() % DEG(360);

      if (obj->state2 != 0) {
         radius = rand() % 15 + CV(0.375);
         obj->y2.n = rand() % CV(0.125) + CV(0.25);
         obj->y3.n = -2;
      } else {
         radius = rand() % 15 + 8;
         obj->y3.n = -8;
         obj->y2.n = rand() % CV(0.125) + CV(0.25);
      }

      obj->x2.n = radius * rcos(theta) >> 12;
      obj->z2.n = radius * rsin(theta) >> 12;
      obj->x3.n = -(obj->x2.n >> 2);
      obj->z3.n = -(obj->z2.n >> 2);
      obj->state++;
      obj->mem = rand() % 8;

   // fallthrough
   case 1:
      if (--obj->mem <= 0) {
         obj->mem = 32;
         UpdateObjAnimation(obj);
         obj->state++;
      }
      break;

   case 2:
      obj->x1.n += obj->x2.n;
      obj->z1.n += obj->z2.n;
      obj->y1.n += obj->y2.n;
      obj->y2.n += obj->y3.n;

      elevation = GetTerrainElevation(obj->z1.s.hi, obj->x1.s.hi);
      if (obj->y1.n < elevation) {
         obj->y1.n = elevation;
         obj->y2.n = -(obj->y2.n / 2);
      }

      UpdateObjAnimation(obj);

      prevQuadIdx = gQuadIndex;
      AddObjPrim6(gGraphicsPtr->ot, obj, 0);

      if (gQuadIndex != prevQuadIdx && obj->d.sprite.clut != 0) {
         poly = &gGraphicsPtr->quads[gQuadIndex - 1];
         poly->clut = obj->d.sprite.clut;
      }

      if (--obj->mem <= 0) {
         obj->functionIndex = OBJF_NULL;
      }

      break;
   }
}

/*void RotateQuadXY(Quad src, Quad dst, s16 theta) {
   //? Or just operate directly on SVECTOR* args (to avoid obscured array args)
   SVECTOR *pSrc = &src[0];
   SVECTOR *pDst = &dst[0];
   s32 i;

   for (i = 0; i < 4; i++) {
      pDst->vx = (pSrc->vx * rcos(theta) - pSrc->vy * rsin(theta)) >> 12;
      pDst->vy = (pSrc->vx * rsin(theta) + pSrc->vy * rcos(theta)) >> 12;
      pDst++;
      pSrc++;
   }
}*/

void RotateQuadXY(SVECTOR *src, SVECTOR *dst, s16 theta) {
   s32 i;

   for (i = 0; i < 4; i++) {
      dst->vx = (src->vx * rcos(theta) - src->vy * rsin(theta)) >> 12;
      dst->vy = (src->vx * rsin(theta) + src->vy * rcos(theta)) >> 12;
      dst++;
      src++;
   }
}

void RotateQuadYZ(SVECTOR *src, SVECTOR *dst, s16 theta) {
   s32 i;

   for (i = 0; i < 4; i++) {
      dst->vy = (src->vy * rcos(theta) - src->vz * rsin(theta)) >> 12;
      dst->vz = (src->vy * rsin(theta) + src->vz * rcos(theta)) >> 12;
      dst->vx = src->vx;
      dst++;
      src++;
   }
}

void RotateQuadXZ(SVECTOR *src, SVECTOR *dst, s16 theta) {
   s32 i;

   for (i = 0; i < 4; i++) {
      dst->vx = (src->vx * rcos(theta) - src->vz * rsin(theta)) >> 12;
      dst->vz = (src->vx * rsin(theta) + src->vz * rcos(theta)) >> 12;
      dst->vy = src->vy;
      dst++;
      src++;
   }
}

#undef OBJF
#define OBJF 272
void Objf272_StatRaised(Object *obj) {
   Object *unitSprite;
   s32 theta;
   s32 w, h;
   s32 tmp;

   switch (obj->state) {
   case 0:
      if (obj->mem == 0) {
         obj->mem = 0x40;
      }
      obj->state3 = obj->mem;
      OBJ.boxIdx = 7;
      OBJ.animRelativeOfs = -8;
      OBJ.animYOfs = 8;

      switch (OBJ.type) {
      case RAISED_STAT_LV:
         OBJ.statGfxIdx = GFX_LV;
         break;
      case RAISED_STAT_AT:
         OBJ.statGfxIdx = GFX_AT;
         break;
      case RAISED_STAT_DF:
         OBJ.statGfxIdx = GFX_DF;
         break;
      case RAISED_STAT_MV:
         OBJ.statGfxIdx = GFX_MV;
         break;
      default:
         OBJ.statGfxIdx = GFX_LV;
         break;
      }

      OBJ.upGfxIdx = GFX_UP; // "up!"
      obj->state++;

   // fallthrough
   case 1:
      unitSprite = OBJ.unitSprite;
      obj->x1.n = unitSprite->x1.n;
      obj->z1.n = unitSprite->z1.n;
      obj->y1.n = unitSprite->y1.n + CV(1.5);

      tmp = obj->state3 - obj->mem;
      if (tmp >= 12) {
         tmp = 12;
      }
      theta = tmp * DEG(11.25);
      w = abs(12 * rsin(theta) >> 12) + 12;
      h = abs(8 * rsin(theta) >> 12) + 8;
      rsin(theta);

      gQuad_800fe63c[0].vz = gQuad_800fe63c[1].vz = gQuad_800fe63c[2].vz = gQuad_800fe63c[3].vz = 0;

      OBJ.gfxIdx = OBJ.statGfxIdx;
      gQuad_800fe63c[0].vx = -w;
      gQuad_800fe63c[0].vy = -h / 2;
      gQuad_800fe63c[1].vx = 0;
      gQuad_800fe63c[1].vy = -h / 2;
      gQuad_800fe63c[2].vx = -w;
      gQuad_800fe63c[2].vy = h / 2;
      gQuad_800fe63c[3].vx = 0;
      gQuad_800fe63c[3].vy = h / 2;
      AddObjPrim6(gGraphicsPtr->ot, obj, 0);

      OBJ.gfxIdx = OBJ.upGfxIdx;
      gQuad_800fe63c[0].vx = 0;
      gQuad_800fe63c[0].vy = -h / 2;
      gQuad_800fe63c[1].vx = w;
      gQuad_800fe63c[1].vy = -h / 2;
      gQuad_800fe63c[2].vx = 0;
      gQuad_800fe63c[2].vy = h / 2;
      gQuad_800fe63c[3].vx = w;
      gQuad_800fe63c[3].vy = h / 2;
      AddObjPrim6(gGraphicsPtr->ot, obj, 0);

      if (--obj->mem <= 0) {
         obj->functionIndex = OBJF_NULL;
      }

      break;
   }
}

#undef OBJF
#define OBJF 380
void Objf380_LevelUpFx(Object *obj) {
   static Quad quad = {{-32, -32, 0, 0}, {32, -32, 0, 0}, {-32, 32, 0, 0}, {32, 32, 0, 0}};

   u8 cluts[4] = {CLUT_REDS, CLUT_BLUES, CLUT_PURPLES};

   Object *obj_s2;
   Object *unitSprite;
   Object *statRaised;
   s32 spriteX, spriteY, spriteW, spriteH;
   POLY_FT4 *poly;
   s32 gfxX, gfxY, gfxW, gfxH;
   s32 i;
   s16 halfSize;
   s16 theta;
   s16 radius;
   s16 fade;

   switch (obj->state) {
   case 0:
      SnapToUnit(obj);
      OBJ.timer = 0;

      obj_s2 = Obj_GetUnused();
      obj_s2->functionIndex = OBJF_NOOP;
      OBJ.sprite = obj_s2;
      obj_s2->d.sprite.gfxIdx = GFX_JP_TBD_245;
      obj_s2->d.sprite.semiTrans = 4;
      obj_s2->d.sprite.boxIdx = 7;
      obj_s2->d.sprite.clut = CLUT_REDS;
      obj_s2->x1.n = obj->x1.n;
      obj_s2->z1.n = obj->z1.n;
      obj_s2->y1.n = obj->y1.n;

      unitSprite = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
      unitSprite->d.sprite.hidden = 1;
      OBJ.unitSprite = unitSprite;

      statRaised = CreatePositionedObj(obj_s2, OBJF_STAT_RAISED);
      statRaised->d.objf272.unitSprite = unitSprite;

      obj->state2 = 0;
      obj->mem = 3;
      obj->state++;

   // fallthrough
   case 1:
      unitSprite = OBJ.unitSprite;
      GetUnitSpriteVramRect(unitSprite, &spriteX, &spriteY, &spriteW, &spriteH);
      ApplyMaskEffect(spriteX, spriteY, spriteW + 1, spriteH + 1, 400 << 2, 320, 0,
                      (OBJ.timer * 2) % 64, GFX_MASK_EFFECT_1, 0);
      OBJ.timer++;

      obj_s2 = Obj_GetUnused();
      CopyObject(unitSprite, obj_s2);
      obj_s2->d.sprite.hidden = 0;
      obj_s2->d.sprite.semiTrans = 2;
      RenderUnitSprite(gGraphicsPtr->ot, obj_s2, 0);
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];

      gfxX = gGfxSubTextures[GFX_MASK_EFFECT_1][0];
      gfxY = gGfxSubTextures[GFX_MASK_EFFECT_1][1];
      gfxW = gGfxSubTextures[GFX_MASK_EFFECT_1][2];
      gfxH = gGfxSubTextures[GFX_MASK_EFFECT_1][3];
      setUVWH(poly, gfxX, gfxY, gfxW, gfxH);

      poly->clut = gClutIds[cluts[OBJ.timer % 3]];
      poly->tpage = GetTPage(0, 1, 576, 256);

      fade = 0x60 + (0x20 * rsin(OBJ.timer * 0x80) >> 12);
      setRGB0(poly, fade, fade, fade);

      CopyObject(unitSprite, obj_s2);
      obj_s2->d.sprite.hidden = 0;
      RenderUnitSprite(gGraphicsPtr->ot, obj_s2, 0);
      obj_s2->functionIndex = OBJF_NULL;

      halfSize = 0x80 * rsin(obj->state2 * 2) >> 12;
      quad[0].vx = quad[2].vx = -halfSize;
      quad[1].vx = quad[3].vx = halfSize;
      quad[0].vy = quad[1].vy = -halfSize;
      quad[2].vy = quad[3].vy = halfSize;
      RotateQuadXY(quad, gQuad_800fe63c, obj->state2);

      obj->state2 += 0x20;

      if (--obj->mem <= 0) {
         for (i = 0; i < 2; i++) {
            theta = rand() % DEG(360);
            radius = rand() % 4 + 12;
            obj_s2 = Obj_GetUnused();
            obj_s2->functionIndex = OBJF_FAINT_SPARKLES;
            obj_s2->x2.n = radius * rcos(theta) >> 12;
            obj_s2->z2.n = radius * rsin(theta) >> 12;
            obj_s2->d.objf278.targetSprite = unitSprite;
         }
         obj->mem = rand() % 5;
         obj->state3++;
      }
      if (obj->state3 >= 30) {
         obj->state3 = 0;
         unitSprite = OBJ.unitSprite;
         unitSprite->d.sprite.hidden = 0;
         obj->state++;
      }
      break;

   case 2:
      obj->state++;

   // fallthrough
   case 3:
      if (++obj->state3 >= 40) {
         obj_s2 = OBJ.sprite;
         obj_s2->functionIndex = OBJF_NULL;
         obj->functionIndex = OBJF_NULL;
         gSignal3 = 1;
      }
      break;
   }
}

#undef OBJF
#define OBJF 278
void Objf278_FaintSparkles(Object *obj) {
   static s16 sparkleAnimData[14] = {7, GFX_SPARKLE_1, 3, GFX_SPARKLE_2, 3, GFX_SPARKLE_3,
                                     3, GFX_SPARKLE_4, 3, GFX_SPARKLE_5, 3, GFX_NULL,
                                     1, GFX_NULL};

   Object *targetSprite;
   Object *sprite;
   Quad quad;
   s32 i;

   targetSprite = OBJ.targetSprite;

   switch (obj->state) {
   case 0:
      sprite = Obj_GetUnused();
      sprite->functionIndex = OBJF_NOOP;
      sprite->d.sprite.gfxIdx = GFX_LIT_SPHERE_8;
      sprite->d.sprite.boxIdx = 7;
      if (OBJ.clut == CLUT_NULL) {
         sprite->d.sprite.clut = CLUT_BLUES;
      } else {
         sprite->d.sprite.clut = OBJ.clut;
      }
      sprite->d.sprite.semiTrans = 4;
      sprite->d.sprite.animData = sparkleAnimData;
      sprite->x1.n = obj->x1.n;
      sprite->z1.n = obj->z1.n;
      sprite->y1.n = obj->y1.n;
      OBJ.sprite = sprite;
      obj->state++;

   // fallthrough
   case 1:
      sprite = OBJ.sprite;
      obj->x1.n += obj->x2.n;
      obj->z1.n += obj->z2.n;
      obj->y1.n += obj->y2.n;
      sprite->x1.n = targetSprite->x1.n + obj->x1.n;
      sprite->z1.n = targetSprite->z1.n + obj->z1.n;
      sprite->y1.n =
          targetSprite->y1.n + obj->y1.n + (targetSprite->d.sprite.animYOfs << 3) + CV(0.5);

      for (i = 0; i < 4; i++) {
         quad[i] = gQuad_800fe63c[i];
      }
      for (i = 0; i < obj->state2; i++) {
         gQuad_800fe63c[0].vx = -i * 2;
         gQuad_800fe63c[1].vx = i * 2;
         gQuad_800fe63c[2].vx = -i * 2;
         gQuad_800fe63c[3].vx = i * 2;
         gQuad_800fe63c[0].vy = -i * 2;
         gQuad_800fe63c[1].vy = -i * 2;
         gQuad_800fe63c[2].vy = i * 2;
         gQuad_800fe63c[3].vy = i * 2;
         UpdateObjAnimation(sprite);
         AddObjPrim6(gGraphicsPtr->ot, sprite, 0);
      }
      for (i = 0; i < 4; i++) {
         gQuad_800fe63c[i] = quad[i];
      }

      if (++obj->state3 > 16) {
         if (--obj->mem <= 0) {
            obj->state++;
         } else {
            obj->state3 = 0;
         }
      }

      obj->state2 = abs(8 * rsin(obj->state3 * 16) >> 12) + 1;
      break;

   case 2:
      sprite = OBJ.sprite;
      sprite->functionIndex = OBJF_NULL;
      obj->functionIndex = OBJF_NULL;
      break;
   }
}

#undef OBJF
#define OBJF 685
void Objf685_RockSpurt(Object *obj) {
   Object *unitSprite;
   Object *particle;
   s16 clut;
   s32 i;

   unitSprite = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);

   if (gSpriteStripUnitIds[unitSprite->d.sprite.stripIdx - 2] == UNIT_ID_GOLD_GOLEM ||
       gSpriteStripUnitIds[unitSprite->d.sprite.stripIdx - 2] == UNIT_ID_CLAY_GOLEM) {
      clut = gSpriteStripClutIds[unitSprite->d.sprite.stripIdx];
   } else {
      clut = CLUT_NULL;
   }

   switch (obj->state) {
   case 0:
      PerformAudioCommand(AUDIO_CMD_PLAY_SFX(224));
      for (i = 0; i < 16; i++) {
         particle = CreatePositionedObj(unitSprite, OBJF_ROCK_SPURT_PARTICLE);
         particle->y1.n += CV(1.0);
         particle->d.sprite.clut = clut;
      }
      obj->mem = 32;
      obj->state++;
      break;

   case 1:
      if (--obj->mem <= 3) {
         PerformAudioCommand(AUDIO_CMD_PLAY_SFX(225));
         for (i = 0; i < 32; i++) {
            particle = CreatePositionedObj(unitSprite, OBJF_ROCK_SPURT_PARTICLE);
            particle->state2 = 1;
            particle->y1.n += CV(0.5);
            particle->d.sprite.clut = clut;
         }
      }
      if (obj->mem == 0) {
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 715
void Objf715_to_718_Spellbind_FX2_FX3(Object *obj) {
   Object *infoMarker; // objf052

   switch (obj->state) {
   case 0:
      infoMarker = CreatePositionedObj(obj, OBJF_ATTACK_INFO_MARKER);

      switch (obj->functionIndex) {
      case OBJF_SPELLBIND_FX2:
         infoMarker->d.objf052.type = ATK_MARKER_PARALYZED;
         infoMarker->functionIndex = OBJF_NULL;
         break;
      case OBJF_SPELLBIND_FX3:
         infoMarker->d.objf052.type = ATK_MARKER_MISS;
         break;
      case OBJF_TBD_717:
         infoMarker->d.objf052.type = ATK_MARKER_POISONED;
         OBJ.unused_0x26 = 8;
         infoMarker->functionIndex = OBJF_NULL;
         break;
      case OBJF_TBD_718:
         infoMarker->d.objf052.type = ATK_MARKER_MISS;
         OBJ.unused_0x26 = 8;
         break;
      }

      obj->mem = 31;
      obj->state++;
      break;

   case 1:
      if (--obj->mem <= 0) {
         obj->functionIndex = OBJF_NULL;
         gSignal3 = 1;
      }
      break;
   }
}

#undef OBJF
#define OBJF 739
void Objf739_Particle(Object *obj) {
   // Spawned by: 790
   obj->x1.n += obj->x2.n;
   obj->z1.n += obj->z2.n;
   obj->y1.n += obj->y2.n;

   obj->x2.n += obj->x3.n;
   obj->z2.n += obj->z3.n;
   obj->y2.n += obj->y3.n;

   UpdateObjAnimation(obj);

   if (obj->d.sprite.animFinished) {
      obj->d.sprite.hidden = 1;
   }

   AddObjPrim6(gGraphicsPtr->ot, obj, 0);

   if (--obj->mem <= 0) {
      obj->functionIndex = OBJF_NULL;
   }
}

static s16 sSparkleAnimData_800ff980[24] = {7, GFX_SPARKLE_1, 1, GFX_SPARKLE_2, 1, GFX_SPARKLE_3,
                                            1, GFX_SPARKLE_4, 1, GFX_SPARKLE_5, 1, GFX_SPARKLE_1,
                                            1, GFX_SPARKLE_2, 1, GFX_SPARKLE_3, 1, GFX_SPARKLE_4,
                                            1, GFX_SPARKLE_5, 1, GFX_NULL,      0, GFX_NULL};

#undef OBJF
#define OBJF 735
void Objf735_SparkleDust(Object *obj) {
   gQuad_800fe63c[0].vx = -2;
   gQuad_800fe63c[1].vx = 2;
   gQuad_800fe63c[2].vx = -2;
   gQuad_800fe63c[3].vx = 2;
   gQuad_800fe63c[0].vy = -4;
   gQuad_800fe63c[1].vy = -4;
   gQuad_800fe63c[2].vy = 4;
   gQuad_800fe63c[3].vy = 4;

   switch (obj->state) {
   case 0:
      obj->d.sprite.animData = sSparkleAnimData_800ff980;
      obj->d.sprite.clut = CLUT_REDS;
      obj->state++;

   // fallthrough
   case 1:
      UpdateObjAnimation(obj);
      if (obj->d.sprite.animFinished) {
         obj->state++;
      } else {
         AddObjPrim6(gGraphicsPtr->ot, obj, 0);
      }
      break;

   case 2:
      obj->d.sprite.gfxIdx = GFX_DOTS;
      obj->d.sprite.boxIdx = 7;
      obj->d.sprite.clut = CLUT_REDS;
      obj->y2.n = -8;
      obj->y3.n = -4;
      obj->mem = 32;
      obj->state++;

   // fallthrough
   case 3:
      obj->y1.n += obj->y2.n;
      AddObjPrim6(gGraphicsPtr->ot, obj, 0);
      if (--obj->mem <= 0) {
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 740
void Objf740_RemoveParalysis_Sparkles(Object *obj) {
   switch (obj->state) {
   case 0:
      obj->d.sprite.animData = sSparkleAnimData_800ff980;
      obj->d.sprite.clut = CLUT_REDS;
      obj->state++;

   // fallthrough
   case 1:
      obj->x1.n += obj->x2.n;
      obj->z1.n += obj->z2.n;
      obj->y1.n += obj->y2.n;

      obj->x2.n += obj->x3.n;
      obj->z2.n += obj->z3.n;
      obj->y2.n += obj->y3.n;

      UpdateObjAnimation(obj);
      AddObjPrim6(gGraphicsPtr->ot, obj, 0);

      if (--obj->mem <= 0) {
         obj->state++;
      }

      break;

   case 2:
      CreatePositionedObj(obj, OBJF_SPARKLE_DUST);
      obj->functionIndex = OBJF_NULL;
      break;
   }
}

#undef OBJF
#define OBJF 736
void Objf736_RemoveParalysis_Bubble(Object *obj) {
   Object *sparkles;
   s32 i;
   s32 theta;
   s32 radius;

   gQuad_800fe63c[0].vx = -2;
   gQuad_800fe63c[1].vx = 2;
   gQuad_800fe63c[2].vx = -2;
   gQuad_800fe63c[3].vx = 2;
   gQuad_800fe63c[0].vy = -4;
   gQuad_800fe63c[1].vy = -4;
   gQuad_800fe63c[2].vy = 4;
   gQuad_800fe63c[3].vy = 4;

   switch (obj->state) {
   case 0:
      obj->d.sprite.gfxIdx = GFX_EXPLOSION_1;
      obj->d.sprite.boxIdx = 5;
      obj->d.sprite.clut = CLUT_BLUES;
      obj->state++;

   // fallthrough
   case 1:
      obj->x1.n += obj->x2.n;
      obj->z1.n += obj->z2.n;
      obj->y1.n += obj->y2.n;

      obj->x2.n += obj->x3.n;
      obj->z2.n += obj->z3.n;
      obj->y2.n += obj->y3.n;

      obj->d.sprite.clut = 3 + (obj->mem % 2);
      AddObjPrim6(gGraphicsPtr->ot, obj, 0);

      if (--obj->mem <= 0) {
         obj->state++;
      }
      break;

   case 2:
      theta = rand() % DEG(360);
      radius = 8;

      for (i = 0; i < 4; i++) {
         sparkles = CreatePositionedObj(obj, OBJF_REMOVE_PARALYSIS_SPARKLES);
         sparkles->x2.n = radius * rcos(theta) >> 12;
         sparkles->z2.n = radius * rsin(theta) >> 12;
         sparkles->y2.n = 0;
         sparkles->x3.n = sparkles->x2.n >> 2;
         sparkles->z3.n = sparkles->z2.n >> 2;
         sparkles->y3.n = 0;
         sparkles->mem = 4;
         theta += DEG(90);
      }

      obj->functionIndex = OBJF_NULL;
      break;
   }
}

#undef OBJF
#define OBJF 737
void Objf737_RemoveParalysis(Object *obj) {
   Object *unitSprite;
   Object *obj_s1;
   s16 theta;
   s32 i;
   s32 radius;
   MaskEffectPreset unusedMaskEffect;

   switch (obj->state) {
   case 0:
      SnapToUnit(obj);
      obj->y1.n += CV(1.0);
      obj->state3 = rand() % 0x1000;

      if (obj->state2 == 0) {
         obj->state2 = 0x100;
      }

      unitSprite = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
      obj_s1 = Obj_GetUnused();
      CopyObject(unitSprite, obj_s1);
      unitSprite->d.sprite.hidden = 1;
      RenderUnitSprite(gGraphicsPtr->ot, obj_s1, 0);
      obj_s1->functionIndex = OBJF_NULL;

      obj->state++;

   // fallthrough
   case 1:
      SnapToUnit(obj);
      obj->y1.n += CV(1.0);
      if (--obj->mem <= 0) {
         theta = rand() % DEG(360);
         for (i = 0; i < 1; i++) {
            radius = 20;
            obj_s1 = CreatePositionedObj(obj, OBJF_REMOVE_PARALYSIS_BUBBLE);
            obj_s1->x2.n = radius * rcos(theta) >> 12;
            obj_s1->z2.n = radius * rsin(theta) >> 12;
            obj_s1->y2.n = radius;
            obj_s1->x3.n = obj_s1->x2.n >> 1;
            obj_s1->z3.n = obj_s1->z2.n >> 1;
            obj_s1->y3.n = obj_s1->y2.n >> 1;
            obj_s1->mem = 4;
            theta += DEG(45);
         }
         obj->mem = 4;
      }

      unusedMaskEffect.srcGfxIdx = GFX_TILED_VEINS;
      unusedMaskEffect.dstGfxIdx = GFX_MASK_EFFECT_1;
      unusedMaskEffect.width = 0;
      unusedMaskEffect.height = obj->state2 % 64;
      unusedMaskEffect.semiTrans = 2;
      unusedMaskEffect.clut = CLUT_BLUES;
      unusedMaskEffect.color.r = unusedMaskEffect.color.g = unusedMaskEffect.color.b =
          abs(128 * rsin(obj->state2 * 16) >> 12);

      unitSprite = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
      obj_s1 = Obj_GetUnused();
      CopyObject(unitSprite, obj_s1);
      obj_s1->d.sprite.hidden = 0;
      obj_s1->d.sprite.animRelativeOfs = 0;
      obj_s1->d.sprite.animYOfs = 0;
      RenderUnitSprite(gGraphicsPtr->ot, obj_s1, 0);
      obj_s1->functionIndex = OBJF_NULL;

      if (--obj->state2 <= 0) {
         unitSprite = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
         unitSprite->d.sprite.hidden = 0;
         obj->state3 = 48;
         obj->state++;
      }
      break;

   case 2:
      if (--obj->state3 <= 0) {
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 757
void Objf757_PushedObjectSplash(Object *obj) {
   Object *splash;
   s32 i;
   s32 theta;

   switch (obj->state) {
   case 0:
      obj->state3 = 3;
      obj->state++;

   // fallthrough
   case 1:
      if (--obj->mem <= 0) {
         for (i = 0; i < 3; i++) {
            theta = rand() % DEG(360);
            splash = CreatePositionedObj(obj, OBJF_SPLASH_WITH_DROPLETS);
            splash->x1.n += (CV(0.75) * rcos(theta) >> 12);
            splash->z1.n += (CV(0.75) * rsin(theta) >> 12);
            splash->y2.n = obj->y2.n;
         }
         obj->mem += (rand() % 8);
         obj->state3--;
      }
      if (obj->state3 == 0) {
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 760
void Objf760_EliteMeleeSparkles(Object *obj) {
   // Used for Dolf and Ash in their ultimate forms
   Object *sparkles; // objf278
   s32 i;
   s32 theta;
   s32 radius;

   switch (obj->state) {
   case 0:
      OBJ.unitSprite = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
      obj->state++;

   // fallthrough
   case 1:
      if (--obj->mem <= 0) {
         for (i = 0; i < 5; i++) {
            radius = rand() % 12 + 4;
            theta = rand() % DEG(360);
            sparkles = Obj_GetUnused();
            sparkles->functionIndex = OBJF_FAINT_SPARKLES;
            sparkles->d.objf278.targetSprite = OBJ.unitSprite;
            sparkles->x2.n = radius * rsin(theta) >> 12;
            sparkles->z2.n = radius * rcos(theta) >> 12;
            sparkles->d.objf278.clut = CLUT_REDS;
         }
         obj->mem += (rand() % 5);
      }
      if (++obj->state3 >= 32) {
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 762
void Objf762_Megaherb(Object *obj) {
   // Unused?
   Object *reveal;

   switch (obj->state) {
   case 0:
      SnapToUnit(obj);
      reveal = CreatePositionedObj(obj, OBJF_REVEAL_USED_ITEM);
      reveal->d.objf761.gfxIdx = GFX_ITEM_MEGAHERB;
      obj->state++;
      obj->mem = 96;

   // fallthrough
   case 1:
      if (--obj->mem <= 0) {
         gSignal3 = 1;
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 770
void Objf770_to_789_ItemSpell(Object *obj) {
   // TODO gfx constants where relevant
   static s16 icons[44] = {0x0,   0x0,   0x0,   0x14a, 0x14a, 0x139, 0x0,   0x97,  0x5e,
                           0xaa,  0x16b, 0xb5,  0xd2,  0x0,   0xd1,  0xaf,  0x17d, 0xe0,
                           0x0,   0x184, 0x2d8, 0x2d9, 0x2dc, 0x2da, 0x2db, 0x2de, 0x2dd,
                           0x2df, 0x2e0, 0x2e1, 0x2e2, 0x0,   0x0,   0x0,   0x32f, 0x330,
                           0x331, 0x332, 0x333, 0x334, 0x335, 0x336, 0x337, 0x0};
   Object *reveal;

   if ((gCurrentSpell >= 49 && gCurrentSpell <= 59) ||
       (gCurrentSpell >= 63 && gCurrentSpell <= 71)) {
      SnapToUnit(obj);
      reveal = CreatePositionedObj(obj, OBJF_REVEAL_USED_ITEM);
      reveal->d.objf761.gfxIdx = icons[gCurrentSpell - 29];
   }
   obj->functionIndex = OBJF_NULL;
}

#undef OBJF
#define OBJF 087
void Objf087_Map20_Scn30_ArrowSpawner(Object *obj) {
   Object *obj_v1;

   switch (obj->state) {
   case 0:
      obj_v1 = OBJ.variant_0x24.entitySpriteParam;
      obj->x1.n = obj_v1->x1.n;
      obj->z1.n = obj_v1->z1.n;
      obj->y1.n = CV(1.0);
      OBJ.entitySprite = obj_v1;
      OBJ.variant_0x24.velocity.z = CV(0.46875);
      OBJ.variant_0x24.velocity.y = CV(0.28125);
      OBJ.yAccel = CV(-0.046875);
      obj->y1.n += CV(0.5);
      obj->state++;

   // fallthrough
   case 1:
      obj_v1 = Obj_GetUnused();
      obj_v1->functionIndex = OBJF_MAP20_SCN30_ARROW;
      obj_v1->x1.n = obj->x1.n;
      obj_v1->z1.n = obj->z1.n;
      obj_v1->y1.n = obj->y1.n;
      obj_v1->z2.n = OBJ.variant_0x24.velocity.z;
      obj_v1->y2.n = OBJ.variant_0x24.velocity.y;
      obj_v1->y3.n = OBJ.yAccel;
      obj_v1->mem = 0x80;
      obj->state++;
      break;

   case 2:
      obj->functionIndex = OBJF_NULL;
      break;
   }
}

#undef OBJF
#define OBJF 098
void Objf098_Map20_Scn30_Arrow(Object *obj) {
   static const Quad baseQuad = {{0, CV(-0.125), CV(-0.5), 0},
                                 {0, CV(-0.125), CV(+0.5), 0},
                                 {0, CV(+0.125), CV(-0.5), 0},
                                 {0, CV(+0.125), CV(+0.5), 0}};

   Quad rotatedQuad;
   s32 i;
   s16 dy, dz;
   s16 theta;
   SVectorXZY *pPosition;

   switch (obj->state) {
   case 0:
      OBJ.gfxIdx = GFX_ARROW;

      obj->x1.n += obj->x2.n;
      obj->z1.n += obj->z2.n;
      obj->y1.n += obj->y2.n;

      obj->x2.n += obj->x3.n;
      obj->z2.n += obj->z3.n;
      obj->y2.n += obj->y3.n;

      if (OBJ.animFinished) {
         OBJ.hidden = 1;
      }

      dz = obj->z1.n - OBJ.pos.z;
      dy = obj->y1.n - OBJ.pos.y;
      theta = -ratan2(dy, dz);
      RotateQuadYZ(baseQuad, rotatedQuad, theta);

      for (i = 0; i < 4; i++) {
         rotatedQuad[i].vx = 0;
      }

      pPosition = &OBJ.coords[0];
      for (i = 0; i < 4; i++) {
         pPosition->x = obj->x1.n + rotatedQuad[i].vx;
         pPosition->z = obj->z1.n + rotatedQuad[i].vz;
         pPosition->y = obj->y1.n + rotatedQuad[i].vy;
         pPosition++;
      }

      AddObjPrim3(gGraphicsPtr->ot, obj);
      OBJ.pos.x = obj->x1.n;
      OBJ.pos.z = obj->z1.n;
      OBJ.pos.y = obj->y1.n;

      if (obj->y2.n <= 0 &&
          obj->y1.n <= GetTerrainElevation(obj->z1.s.hi, obj->x1.s.hi) + CV(0.5)) {
         obj->functionIndex = OBJF_NULL;
      }

      break;
   }
}

static s16 sSmokeAnimData_800ffa08[24] = {
    7, GFX_PUFF_1, 2, GFX_PUFF_2, 2, GFX_PUFF_3, 2, GFX_PUFF_4,  2, GFX_PUFF_5, 2, GFX_PUFF_6,
    2, GFX_PUFF_7, 2, GFX_PUFF_8, 2, GFX_PUFF_9, 2, GFX_PUFF_10, 2, GFX_NULL,   0, GFX_NULL};

static s16 sPoisonAnimData_800ffa38[36] = {
    7, GFX_POISON_1, 2, GFX_POISON_2, 2, GFX_POISON_3, 2, GFX_POISON_4, 2, GFX_POISON_5,
    2, GFX_POISON_6, 2, GFX_POISON_7, 2, GFX_POISON_8, 2, GFX_POISON_1, 2, GFX_POISON_2,
    2, GFX_POISON_3, 2, GFX_POISON_4, 2, GFX_POISON_5, 2, GFX_POISON_6, 2, GFX_POISON_7,
    2, GFX_POISON_8, 2, GFX_NULL,     0, GFX_NULL};

s16 gExplosionAnimData_800ffa80[26] = {7, GFX_EXPLOSION_1,  2, GFX_EXPLOSION_2,  2, GFX_EXPLOSION_3,
                                       2, GFX_EXPLOSION_4,  2, GFX_EXPLOSION_5,  2, GFX_EXPLOSION_6,
                                       2, GFX_EXPLOSION_7,  2, GFX_EXPLOSION_8,  2, GFX_EXPLOSION_9,
                                       2, GFX_EXPLOSION_10, 2, GFX_EXPLOSION_11, 2, GFX_NULL,
                                       0, GFX_NULL};

s16 gSparkleAnimData_800ffab4[36] = {
    7, GFX_SPARKLE_1, 2, GFX_SPARKLE_2, 2, GFX_SPARKLE_3, 2, GFX_SPARKLE_4, 2, GFX_SPARKLE_5,
    2, GFX_SPARKLE_4, 2, GFX_SPARKLE_3, 2, GFX_SPARKLE_2, 2, GFX_SPARKLE_1, 2, GFX_SPARKLE_2,
    2, GFX_SPARKLE_3, 2, GFX_SPARKLE_4, 2, GFX_SPARKLE_5, 2, GFX_SPARKLE_4, 2, GFX_SPARKLE_3,
    2, GFX_SPARKLE_2, 2, GFX_NULL,      0, GFX_NULL};

#undef OBJF
#define OBJF 764
void Objf764_to_769_ProjectileTrail(Object *obj) {
   Object *particle;
   s32 i;
   s32 skip;

   particle = CreatePositionedObj(obj, OBJF_PARTICLE_710);

   switch (obj->functionIndex) {
   case OBJF_PROJECTILE_TRAIL_POISON:
      particle->d.sprite.animData = sPoisonAnimData_800ffa38;
      particle->state2 = rand() % 4 + 2;
      particle->y2.n = -4;
      break;
   case OBJF_PROJECTILE_TRAIL_EXPLOSION:
      particle->state2 = rand() % 6 + 6;
      particle->d.sprite.animData = gExplosionAnimData_800ffa80;
      break;
   case OBJF_PROJECTILE_TRAIL_SMOKE:
      particle->state2 = rand() % 4 + 6;
      particle->d.sprite.animData = sSmokeAnimData_800ffa08;
      particle->d.sprite.semiTrans = 2;
      break;
   default: // OBJF_PROJECTILE_TRAIL_SPARKLES
      particle->state2 = rand() % 5;
      particle->d.sprite.animData = gSparkleAnimData_800ffab4;
      particle->d.sprite.semiTrans = 2;
      break;
   }

   particle->mem = 32;

   skip = rand() & 7;
   for (i = 0; i < skip; i++) {
      UpdateObjAnimation(particle);
   }

   obj->functionIndex = OBJF_NULL;
}

#undef OBJF
#define OBJF 790
/* Perpetual emitter of upward-drifting ember puffs (spawns OBJF_PARTICLE_739 every
 * 0-7 frames, forever). In the event-object index block but no retail EVDATA script
 * spawns it -- cut ambience; Objf739_Particle is unreachable along with it. */
void Objf790_EmberEmitter_Unused(Object *obj) {
   static s16 explosionAnimData[26] = {3, GFX_EXPLOSION_1,  1, GFX_EXPLOSION_2,  1, GFX_EXPLOSION_3,
                                       1, GFX_EXPLOSION_4,  2, GFX_EXPLOSION_5,  2, GFX_EXPLOSION_6,
                                       2, GFX_EXPLOSION_7,  2, GFX_EXPLOSION_8,  2, GFX_EXPLOSION_9,
                                       2, GFX_EXPLOSION_10, 2, GFX_EXPLOSION_11, 2, GFX_NULL,
                                       0, GFX_NULL};

   Object *obj_s2;
   s32 theta;
   s32 radius;
   s32 i;
   s32 skip;

   if (--obj->mem <= 0) {
      obj_s2 = CreatePositionedObj(obj, OBJF_PARTICLE_739);
      radius = rand() & 3;
      theta = rand() & 0xfff;
      obj_s2->x2.n = radius * rcos(theta) >> 12;
      obj_s2->z2.n = radius * rsin(theta) >> 12;
      obj_s2->y2.n = CV(0.24609375);
      obj_s2->d.sprite.animData = explosionAnimData;
      obj_s2->mem = 48;

      skip = rand() & 3;
      for (i = 0; i < skip; i++) {
         UpdateObjAnimation(obj_s2);
      }

      obj->mem = rand() & 7;
   }
}

static Quad sQuad_Size1 = {{-1, -1, 0, 0}, {1, -1, 0, 0}, {-1, 1, 0, 0}, {1, 1, 0, 0}};
static Quad sQuad_Size2 = {{-2, -2, 0, 0}, {2, -2, 0, 0}, {-2, 2, 0, 0}, {2, 2, 0, 0}};
static Quad sQuad_Size3 = {{-3, -3, 0, 0}, {3, -3, 0, 0}, {-3, 3, 0, 0}, {3, 3, 0, 0}};
static Quad sQuad_Size4 = {{-4, -4, 0, 0}, {4, -4, 0, 0}, {-4, 4, 0, 0}, {4, 4, 0, 0}};
static Quad sQuad_Size5 = {{-5, -5, 0, 0}, {5, -5, 0, 0}, {-5, 5, 0, 0}, {5, 5, 0, 0}};
static Quad sQuad_Size6 = {{-6, -6, 0, 0}, {6, -6, 0, 0}, {-6, 6, 0, 0}, {6, 6, 0, 0}};
static Quad sQuad_Size7 = {{-7, -7, 0, 0}, {7, -7, 0, 0}, {-7, 7, 0, 0}, {7, 7, 0, 0}};
static Quad sQuad_Size8 = {{-8, -8, 0, 0}, {8, -8, 0, 0}, {-8, 8, 0, 0}, {8, 8, 0, 0}};
static Quad sQuad_Size9 = {{-9, -9, 0, 0}, {9, -9, 0, 0}, {-9, 9, 0, 0}, {9, 9, 0, 0}};
static Quad sQuad_Size10 = {{-10, -10, 0, 0}, {10, -10, 0, 0}, {-10, 10, 0, 0}, {10, 10, 0, 0}};
static Quad sQuad_Size11 = {{-11, -11, 0, 0}, {11, -11, 0, 0}, {-11, 11, 0, 0}, {11, 11, 0, 0}};
static Quad sQuad_Size12 = {{-12, -12, 0, 0}, {12, -12, 0, 0}, {-12, 12, 0, 0}, {12, 12, 0, 0}};
static Quad sQuad_Size13 = {{-13, -13, 0, 0}, {13, -13, 0, 0}, {-13, 13, 0, 0}, {13, 13, 0, 0}};
static Quad sQuad_Size14 = {{-14, -14, 0, 0}, {14, -14, 0, 0}, {-14, 14, 0, 0}, {14, 14, 0, 0}};
static Quad sQuad_Size15 = {{-15, -15, 0, 0}, {15, -15, 0, 0}, {-15, 15, 0, 0}, {15, 15, 0, 0}};

static Quad *sSquareQuads[16] = {&sQuad_Size1,  &sQuad_Size1,  &sQuad_Size2,  &sQuad_Size3,
                                 &sQuad_Size4,  &sQuad_Size5,  &sQuad_Size6,  &sQuad_Size7,
                                 &sQuad_Size8,  &sQuad_Size9,  &sQuad_Size10, &sQuad_Size11,
                                 &sQuad_Size12, &sQuad_Size13, &sQuad_Size14, &sQuad_Size15};

#undef OBJF
#define OBJF 710
void Objf710_Particle(Object *obj) {
   obj->x1.n += obj->x2.n;
   obj->z1.n += obj->z2.n;
   obj->y1.n += obj->y2.n;

   obj->x2.n += obj->x3.n;
   obj->z2.n += obj->z3.n;
   obj->y2.n += obj->y3.n;

   UpdateObjAnimation(obj);

   if (obj->d.sprite.animFinished) {
      obj->d.sprite.hidden = 1;
   }

   obj->state2 = obj->state2 & 0xf;
   gSpriteBoxQuads[7] = sSquareQuads[obj->state2];
   AddObjPrim6(gGraphicsPtr->ot, obj, 0);
   gSpriteBoxQuads[7] = &gQuad_800fe63c;

   if (--obj->mem <= 0) {
      obj->functionIndex = OBJF_NULL;
   }
}

#undef OBJF
#define OBJF 797
void Objf797_Map47_Scn14_Dusk(Object *obj) {
   // Spawned by EVDATA14.DAT (Grog's house) to indicate passage of time while waiting
   gLightColor.r = 0x80;
   obj->functionIndex = OBJF_NULL;
}

#undef OBJF
#define OBJF 378
void Objf378_PhaseShift_FX1(Object *obj) {
   const s16 cornerThetas[4] = {DEG(45), DEG(135), DEG(-45), DEG(-135)};

   Object *runeSprite;  // sprite2 (xy)
   Object *crestSprite; // sprite2 (xy)
   Object *mapScaler;
   SVectorXY *pPosition;
   s32 i, j;
   s16 radius;
   s32 iRing;
   s16 quarterRadius;
   s32 startingTheta;
   s16 theta;

   switch (obj->state) {
   case 0:
      runeSprite = Obj_GetUnused();
      runeSprite->functionIndex = OBJF_NOOP;
      OBJ.runeSprite = runeSprite;

      crestSprite = Obj_GetUnused();
      crestSprite->functionIndex = OBJF_NOOP;
      crestSprite->d.sprite2.gfxIdx = GFX_CREST;

      obj->x2.n = CV(0.625);
      obj->y2.n = CV(0.46875);
      obj->state3 = 96;

      pPosition = &OBJ.positions[0];
      for (j = 0; j < ARRAY_COUNT(OBJ.positions); j++) {
         pPosition->x = 137 + j;
         pPosition->y = j * 512;
         pPosition++;
      }

      OBJ.crestSprite = crestSprite;
      obj->state++;

   // fallthrough
   case 1:
      crestSprite = OBJ.crestSprite;
      runeSprite = OBJ.runeSprite;

      switch (obj->state2) {
      case 0:
         radius = CV(0.25) * rsin(obj->mem * DEG(2.8125)) >> 12;
         if (obj->mem >= 32) {
            obj->state2++;
         }
         gLightColor.r += (0x20 - gLightColor.r) >> 4;
         gLightColor.g += (0x20 - gLightColor.g) >> 4;
         gLightColor.b += (0x20 - gLightColor.b) >> 4;
         break;

      case 1:
         radius = CV(0.25);
         if (obj->mem >= 96) {
            obj->state2++;
         }
         break;

      case 2:
         gLightColor.r += (0x80 - gLightColor.r) >> 4;
         gLightColor.g += (0x80 - gLightColor.g) >> 4;
         gLightColor.b += (0x80 - gLightColor.b) >> 4;
         radius = CV(0.25) * rsin(-obj->mem * DEG(2.8125)) >> 12;
         break;

      default:
         radius = CV(0.25);
         break;
      }

      startingTheta = (obj->mem * DEG(5.625));
      startingTheta &= 0xfff;
      pPosition = &crestSprite->d.sprite2.coords[0];
      for (j = 0; j < ARRAY_COUNT(cornerThetas); j++) {
         pPosition->x = obj->x2.n + (radius * rcos(startingTheta + cornerThetas[j]) >> 12);
         pPosition->y = obj->y2.n + (radius * rsin(startingTheta + cornerThetas[j]) >> 12);
         pPosition++;
      }

      AddObjPrim2(gGraphicsPtr->ot, crestSprite);

      for (iRing = 0; iRing < 3; iRing++) {
         startingTheta = -startingTheta;
         quarterRadius = radius >> 2;

         for (i = 0; i < 16; i++) {
            theta = i * (DEG(360) / 16);
            runeSprite->x1.n = obj->x2.n + (radius * rcos(startingTheta + theta) >> 12);
            runeSprite->y1.n = obj->y2.n + (radius * rsin(startingTheta + theta) >> 12);

            pPosition = &runeSprite->d.sprite2.coords[0];
            for (j = 0; j < ARRAY_COUNT(cornerThetas); j++) {
               pPosition->x = runeSprite->x1.n +
                              (quarterRadius * rcos(startingTheta + theta + cornerThetas[j]) >> 12);
               pPosition->y = runeSprite->y1.n +
                              (quarterRadius * rsin(startingTheta + theta + cornerThetas[j]) >> 12);
               pPosition++;
            }

            runeSprite->d.sprite2.gfxIdx = GFX_RUNE_1 + (i % 8);
            AddObjPrim2(gGraphicsPtr->ot, runeSprite);
         }

         radius += (radius >> 1);
      }

      gCameraZoom.vz += (512 - gCameraZoom.vz) >> 4;

      if (obj->mem == 96) {
         mapScaler = Obj_GetUnused();
         mapScaler->functionIndex = OBJF_PHASE_SHIFT_MAP_SCALER;
      }
      if (++obj->mem >= 128) {
         obj->state++;
      }
      break;

   case 2:
      crestSprite = OBJ.crestSprite;
      crestSprite->functionIndex = OBJF_NULL;
      runeSprite = OBJ.runeSprite;
      runeSprite->functionIndex = OBJF_NULL;
      obj->functionIndex = OBJF_NULL;
      break;
   }
}

#undef OBJF
#define OBJF 363
void Objf363_Wyrmfang_FX1(Object *obj) {
   switch (obj->state) {
   case 0:
      obj->y1.n = GetTerrainElevation(obj->z1.s.hi, obj->x1.s.hi);
      OBJ.cwFlames = CreatePositionedObj(obj, OBJF_WYRMFANG_FLAMES_CW);
      OBJ.ccwFlames = CreatePositionedObj(obj, OBJF_WYRMFANG_FLAMES_CCW);
      obj->state++;

   // fallthrough
   case 1:
      PanCamera(obj->x1.n, obj->y1.n, obj->z1.n, 4);
      gCameraZoom.vz += (768 - gCameraZoom.vz) >> 4;
      gCameraRotation.vy -= 8;

      gLightColor.r += (0x40 - gLightColor.r) >> 4;
      gLightColor.g += (0x40 - gLightColor.g) >> 4;
      gLightColor.b += (0x40 - gLightColor.b) >> 4;

      if (++obj->mem >= 32) {
         obj->state++;
         obj->state3 = 0;
         obj->mem = 0;
      }
      break;

   case 2:
      obj->state3 += 2;
      if (obj->state3 >= 0xff) {
         obj->state3 = 0xff;
      }

      gLightColor.r = obj->state3;

      PanCamera(obj->x1.n, obj->y1.n, obj->z1.n, 4);
      gCameraZoom.vz += (768 - gCameraZoom.vz) >> 4;
      gCameraRotation.vy -= 8;

      if (++obj->mem >= 160) {
         obj->state++;
      }
      break;

   case 3:
      gLightColor.r = 0x80;
      gLightColor.g = 0x80;
      gLightColor.b = 0x80;

      OBJ.cwFlames->functionIndex = OBJF_NULL;
      OBJ.ccwFlames->functionIndex = OBJF_NULL;
      obj->functionIndex = OBJF_NULL;

      gSignal3 = 1;
      break;
   }
}

#undef OBJF
#define OBJF 359
void Objf359_PhaseShift_MapScaler(Object *obj) {
   s16 radius;

   switch (obj->state) {
   case 0:
      obj->y2.n = gCameraPos.vy;
      gState.enableMapScaling = 1;
      obj->state++;

   // fallthrough
   case 1:
      obj->state2++;
      radius = abs(FIXED(0.5) * rsin(obj->mem * DEG(360.0 / 128)) >> 12);

      gMapScale.vy = FIXED(0.5) + (radius * rsin(obj->mem * DEG(11.25)) >> 12);
      gMapScale.vx = FIXED(0.5) + (radius * rsin(obj->mem * DEG(22.5)) >> 12);
      gMapScale.vz = FIXED(0.5) + (radius * rcos(obj->mem * DEG(22.5)) >> 12);

      gCameraZoom.vz += (512 - gCameraZoom.vz) >> 4;
      gCameraPos.vy = obj->y2.n + (gMapScale.vy >> 8);
      gCameraRotation.vy += DEG(360.0 / 128);

      if (++obj->mem >= 128) {
         obj->mem = 0;
         obj->state++;
      }
      break;

   case 2:
      gMapScale.vy += (FIXED(1.0) - gMapScale.vy) >> 3;
      gMapScale.vx += (FIXED(1.0) - gMapScale.vx) >> 3;
      gMapScale.vz += (FIXED(1.0) - gMapScale.vz) >> 3;
      gCameraPos.vy += (obj->y2.n - gCameraPos.vy) >> 3;

      if (++obj->mem >= 32) {
         obj->state++;
      }
      break;

   case 3:
      gMapScale.vx = FIXED(1.0);
      gMapScale.vz = FIXED(1.0);
      gMapScale.vy = FIXED(1.0);
      gState.enableMapScaling = 0;
      obj->functionIndex = OBJF_NULL;
      gSignal3 = 1;
      break;
   }
}
