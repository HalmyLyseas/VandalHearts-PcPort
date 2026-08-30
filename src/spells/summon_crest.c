/* Summon-crest and lightning-family casting effects: Objf204_SummonCrest and its red/blue/green
 * shims Objf207/209/210 (also driven by the Objf323_713 cutscene object), Holy Lightning
 * (Objf208/212) and Rolling Thunder (Objf197/198). gSpellsEx: docs/decomp/spell-fx-dispatch.md. */
#include "common.h"
#include "object.h"
#include "graphics.h"
#include "field.h"
#include "battle.h"

extern void ApplyMaskEffect(s16, s16, s16, s16, s16, s16, s16, s16, s16, s16);

#undef OBJF
#define OBJF 204
void Objf204_SummonCrest(Object *obj) {
   Object *unitSprite;
   Object *fxSprite1;
   Object *fxSprite2;
   s16 tmp; // halfSize, intensity
   POLY_FT4 *poly;
   s32 spriteX, spriteY, spriteW, spriteH;

   switch (obj->state) {
   case 0:
      if (OBJ.unitSprite == NULL) {
         OBJ.unitSprite = unitSprite = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
      } else {
         unitSprite = OBJ.unitSprite;
      }
      unitSprite->d.sprite.hidden = 1;
      obj->state++;

   // fallthrough
   case 1:
      unitSprite = OBJ.unitSprite;
      fxSprite1 = Obj_GetUnused();
      fxSprite1->d.sprite.gfxIdx = GFX_MASK_EFFECT_4;
      fxSprite1->d.sprite.clut = OBJ.clut;
      fxSprite1->d.sprite.boxIdx = 7;

      tmp = rsin(OBJ.timer * 32) * 0x2a / ONE;
      gQuad_800fe63c[0].vx = -tmp;
      gQuad_800fe63c[0].vy = -tmp;
      gQuad_800fe63c[1].vx = tmp;
      gQuad_800fe63c[1].vy = -tmp;
      gQuad_800fe63c[2].vx = -tmp;
      gQuad_800fe63c[2].vy = tmp;
      gQuad_800fe63c[3].vx = tmp;
      gQuad_800fe63c[3].vy = tmp;

      ApplyMaskEffect(496 << 2, 384, 64, 64, OBJ.vramSrcX, OBJ.vramSrcY, 0, OBJ.timer % 64,
                      GFX_MASK_EFFECT_4, 0);
      fxSprite1->x1.n = unitSprite->x1.n;
      fxSprite1->y1.n = unitSprite->y1.n + CV(1.0);
      fxSprite1->z1.n = unitSprite->z1.n;
      AddObjPrim6(gGraphicsPtr->ot, fxSprite1, 0);
      GetUnitSpriteVramRect(unitSprite, &spriteX, &spriteY, &spriteW, &spriteH);
      ApplyMaskEffect(spriteX, spriteY, spriteW + 1, spriteH + 1, 452 << 2, 400, 0, 0,
                      GFX_MASK_EFFECT_1, 0);

      CopyObject(unitSprite, fxSprite1);
      fxSprite1->d.sprite.hidden = 0;
      fxSprite1->d.sprite.gfxIdx = GFX_MASK_EFFECT_1;
      fxSprite1->d.sprite.clut = OBJ.maskClut;
      fxSprite1->d.sprite.semiTrans = 2;

      if (unitSprite->d.sprite.gfxIdx >= 21 && unitSprite->d.sprite.gfxIdx <= 30) {
         fxSprite1->d.sprite.boxIdx = 1;
      } else {
         fxSprite1->d.sprite.boxIdx = 0;
      }

      tmp = rsin(OBJ.timer * 64) / 32;
      AddObjPrim6(gGraphicsPtr->ot, fxSprite1, 0);
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      setRGB0(poly, tmp, tmp, tmp);

      fxSprite2 = Obj_GetUnused();
      CopyObject(unitSprite, fxSprite2);
      fxSprite2->d.sprite.hidden = 0;
      RenderUnitSprite(gGraphicsPtr->ot, fxSprite2, 0);

      fxSprite2->functionIndex = OBJF_NULL;
      fxSprite1->functionIndex = OBJF_NULL;

      OBJ.timer++;
      if (OBJ.timer == 33) {
         obj->functionIndex = OBJF_NULL;
         unitSprite->d.sprite.hidden = 0;
         gSignal3 = 1;
      }

      break;
   }
}

#undef OBJF
#define OBJF 207
void Objf207_SummonRedCrest(Object *obj) {
   Object *crest;

   crest = Obj_GetUnused();
   crest->functionIndex = OBJF_SUMMON_CREST;
   crest->x1.s.hi = obj->x1.s.hi;
   crest->z1.s.hi = obj->z1.s.hi;
   crest->d.objf204.maskClut = CLUT_MASK;
   crest->d.objf204.clut = CLUT_REDS;
   crest->d.objf204.vramSrcX = 384 << 2;
   crest->d.objf204.vramSrcY = 384;

   obj->functionIndex = OBJF_NULL;
}

#undef OBJF
#define OBJF 209
void Objf209_SummonBlueCrest(Object *obj) {
   Object *crest;

   crest = Obj_GetUnused();
   crest->functionIndex = OBJF_SUMMON_CREST;
   crest->x1.s.hi = obj->x1.s.hi;
   crest->z1.s.hi = obj->z1.s.hi;
   crest->d.objf204.maskClut = CLUT_MASK;
   crest->d.objf204.clut = CLUT_BLUES;
   crest->d.objf204.vramSrcX = 384 << 2;
   crest->d.objf204.vramSrcY = 384;

   obj->functionIndex = OBJF_NULL;
}

#undef OBJF
#define OBJF 210
void Objf210_SummonGreenCrest(Object *obj) {
   Object *crest;

   crest = Obj_GetUnused();
   crest->functionIndex = OBJF_SUMMON_CREST;
   crest->x1.s.hi = obj->x1.s.hi;
   crest->z1.s.hi = obj->z1.s.hi;
   crest->d.objf204.maskClut = CLUT_MASK;
   crest->d.objf204.clut = CLUT_GREENS;
   crest->d.objf204.vramSrcX = 384 << 2;
   crest->d.objf204.vramSrcY = 384;

   obj->functionIndex = OBJF_NULL;
}

#undef OBJF
#define OBJF 208
void Objf208_HolyLightning_FX1(Object *obj) {
   Object *obj_s1; // unitSprite, bolt
   Object *fxSprite1;
   Object *fxSprite2;
   s32 spriteX, spriteY, spriteW, spriteH;

   switch (obj->state) {
   case 0:
      OBJ.unitSprite = obj_s1 = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
      obj->x1.n = obj_s1->x1.n;
      obj->y1.n = obj_s1->y1.n;
      obj->z1.n = obj_s1->z1.n;
      obj_s1->d.sprite.hidden = 1;

      obj_s1 = Obj_GetUnused();
      obj_s1->functionIndex = OBJF_HOLY_LIGHTNING_CASTING_BOLT;
      obj_s1->d.objf212.parent = obj;
      obj_s1->d.objf212.riseSpeedBias = 0;
      OBJ.bolts[8] = obj_s1;

      obj_s1 = Obj_GetUnused();
      obj_s1->functionIndex = OBJF_HOLY_LIGHTNING_CASTING_BOLT;
      obj_s1->d.objf212.parent = obj;
      obj_s1->d.objf212.riseSpeedBias = 4;
      OBJ.bolts[7] = obj_s1;

      obj_s1 = Obj_GetUnused();
      obj_s1->functionIndex = OBJF_HOLY_LIGHTNING_CASTING_BOLT;
      obj_s1->d.objf212.parent = obj;
      obj_s1->d.objf212.riseSpeedBias = 8;
      OBJ.bolts[6] = obj_s1;

      obj_s1 = Obj_GetUnused();
      obj_s1->functionIndex = OBJF_HOLY_LIGHTNING_CASTING_BOLT;
      obj_s1->d.objf212.parent = obj;
      obj_s1->d.objf212.riseSpeedBias = 12;
      OBJ.bolts[5] = obj_s1;

      obj_s1 = Obj_GetUnused();
      obj_s1->functionIndex = OBJF_HOLY_LIGHTNING_CASTING_BOLT;
      obj_s1->d.objf212.parent = obj;
      obj_s1->d.objf212.riseSpeedBias = 16;
      OBJ.bolts[4] = obj_s1;

      obj_s1 = Obj_GetUnused();
      obj_s1->functionIndex = OBJF_HOLY_LIGHTNING_CASTING_BOLT;
      obj_s1->d.objf212.parent = obj;
      obj_s1->d.objf212.riseSpeedBias = 20;
      OBJ.bolts[3] = obj_s1;

      obj_s1 = Obj_GetUnused();
      obj_s1->functionIndex = OBJF_HOLY_LIGHTNING_CASTING_BOLT;
      obj_s1->d.objf212.parent = obj;
      obj_s1->d.objf212.riseSpeedBias = 24;
      OBJ.bolts[2] = obj_s1;

      obj_s1 = Obj_GetUnused();
      obj_s1->functionIndex = OBJF_HOLY_LIGHTNING_CASTING_BOLT;
      obj_s1->d.objf212.parent = obj;
      obj_s1->d.objf212.riseSpeedBias = 28;
      OBJ.bolts[1] = obj_s1;

      obj_s1 = Obj_GetUnused();
      obj_s1->functionIndex = OBJF_HOLY_LIGHTNING_CASTING_BOLT;
      obj_s1->d.objf212.parent = obj;
      obj_s1->d.objf212.riseSpeedBias = 32;
      OBJ.bolts[0] = obj_s1;

      OBJ.timer = 0;
      obj->state++;

   // fallthrough
   case 1:
      obj_s1 = OBJ.unitSprite;
      GetUnitSpriteVramRect(obj_s1, &spriteX, &spriteY, &spriteW, &spriteH);
      ApplyMaskEffect(spriteX, spriteY, spriteW + 1, spriteH + 1, 432 << 2, 464, 0, OBJ.timer % 64,
                      GFX_MASK_EFFECT_1, 1);

      fxSprite1 = Obj_GetUnused();
      CopyObject(obj_s1, fxSprite1);
      fxSprite1->d.sprite.hidden = 0;
      fxSprite1->d.sprite.gfxIdx = GFX_MASK_EFFECT_1;
      fxSprite1->d.sprite.clut = CLUT_PURPLES;
      fxSprite1->d.sprite.semiTrans = 2;

      if (obj_s1->d.sprite.gfxIdx >= 21 && obj_s1->d.sprite.gfxIdx <= 30) {
         fxSprite1->d.sprite.boxIdx = 1;
      } else {
         fxSprite1->d.sprite.boxIdx = 0;
      }

      AddObjPrim6(gGraphicsPtr->ot, fxSprite1, 0);

      fxSprite2 = Obj_GetUnused();
      CopyObject(obj_s1, fxSprite2);
      fxSprite2->d.sprite.hidden = 0;
      RenderUnitSprite(gGraphicsPtr->ot, fxSprite2, 0);

      OBJ.timer++;
      if (OBJ.timer == 40) {
         gSignal3 = 1;
      } else if (OBJ.timer == 45) {
         obj_s1->d.sprite.hidden = 0; // unitSprite
         obj->functionIndex = OBJF_NULL;
         obj_s1 = OBJ.bolts[8];
         obj_s1->state2 = 99;
         obj_s1 = OBJ.bolts[7];
         obj_s1->state2 = 99;
         obj_s1 = OBJ.bolts[6];
         obj_s1->state2 = 99;
         obj_s1 = OBJ.bolts[5];
         obj_s1->state2 = 99;
         obj_s1 = OBJ.bolts[4];
         obj_s1->state2 = 99;
         obj_s1 = OBJ.bolts[3];
         obj_s1->state2 = 99;
         obj_s1 = OBJ.bolts[2];
         obj_s1->state2 = 99;
         obj_s1 = OBJ.bolts[1];
         obj_s1->state2 = 99;
         obj_s1 = OBJ.bolts[0];
         obj_s1->state2 = 99;
      }

      fxSprite2->functionIndex = OBJF_NULL;
      fxSprite1->functionIndex = OBJF_NULL;
      break;
   }
}

static s16 sLightningAnimData_800febe4[20] = {
    7, GFX_LIGHTNING_1, 1, GFX_LIGHTNING_2, 1, GFX_LIGHTNING_3, 1, GFX_LIGHTNING_4,
    1, GFX_LIGHTNING_5, 1, GFX_LIGHTNING_6, 1, GFX_LIGHTNING_7, 1, GFX_LIGHTNING_8,
    1, GFX_NULL,        1, GFX_NULL};

static s16 sSmokeAnimData_800fec0c[20] = {
    4, GFX_PUFF_1, 2, GFX_PUFF_2, 2, GFX_PUFF_3, 2, GFX_PUFF_4, 2, GFX_PUFF_6,
    2, GFX_PUFF_7, 2, GFX_PUFF_8, 2, GFX_PUFF_9, 2, GFX_NULL,   1, GFX_NULL};

static s16 sRunesAnimData_800fec34[48] = {
    4, GFX_RUNE_1,  1, GFX_RUNE_2,  1, GFX_RUNE_3, 1, GFX_RUNE_4,  1, GFX_RUNE_5,  1, GFX_RUNE_6,
    1, GFX_RUNE_7,  1, GFX_RUNE_8,  1, GFX_RUNE_9, 1, GFX_RUNE_10, 1, GFX_RUNE_11, 1, GFX_RUNE_12,
    1, GFX_RUNE_11, 1, GFX_RUNE_10, 1, GFX_RUNE_9, 1, GFX_RUNE_8,  1, GFX_RUNE_7,  1, GFX_RUNE_6,
    1, GFX_RUNE_5,  1, GFX_RUNE_4,  1, GFX_RUNE_3, 1, GFX_RUNE_2,  1, GFX_NULL,    1, GFX_NULL};

static s16 sRockAnimData_800fec94[12] = {4, GFX_ROCK_1, 8, GFX_ROCK_2, 8, GFX_ROCK_3,
                                         8, GFX_ROCK_4, 8, GFX_NULL,   1, GFX_NULL};

#undef OBJF
#define OBJF 212
void Objf212_HolyLightning_CastingBolt(Object *obj) {
   Object *parent;
   Object *sprite;
   s16 halfHeight;

   switch (obj->state) {
   case 0:
      parent = OBJ.parent;
      obj->x1.n = parent->x1.n;
      obj->y1.n = parent->y1.n;
      obj->z1.n = parent->z1.n;
      OBJ.pulseTheta = 0;
      OBJ.theta = rand() % DEG(360);
      OBJ.pulseSpeed = rand() % 0x30 + 0x64;
      OBJ.baseHeight = rand() % 0xc0 + 0x40;
      OBJ.riseOfs = 0;
      OBJ.riseSpeed = rand() % 0x40 + 0x20 + OBJ.riseSpeedBias;

      sprite = Obj_GetUnused();
      sprite->functionIndex = OBJF_NOOP;
      sprite->d.sprite.animData = sLightningAnimData_800febe4;
      sprite->d.sprite.clut = CLUT_BLUES;
      OBJ.sprite = sprite;
      obj->state++;

   // fallthrough
   case 1:
      sprite = OBJ.sprite;
      halfHeight = 4 + rsin(OBJ.pulseTheta) * 0x18 / ONE;

      sprite->x1.n = obj->x1.n + CV(0.625) * rcos(OBJ.theta) / ONE;
      sprite->z1.n = obj->z1.n + CV(0.625) * rsin(OBJ.theta) / ONE;
      sprite->y1.n = obj->y1.n + OBJ.baseHeight + halfHeight + OBJ.riseOfs;

      gQuad_800fe63c[0].vx = -3;
      gQuad_800fe63c[0].vy = -halfHeight;
      gQuad_800fe63c[1].vx = 3;
      gQuad_800fe63c[1].vy = -halfHeight;
      gQuad_800fe63c[2].vx = -3;
      gQuad_800fe63c[2].vy = halfHeight;
      gQuad_800fe63c[3].vx = 3;
      gQuad_800fe63c[3].vy = halfHeight;

      UpdateObjAnimation(sprite);
      AddObjPrim6(gGraphicsPtr->ot, sprite, 0);

      OBJ.pulseTheta += OBJ.pulseSpeed;
      OBJ.riseOfs += OBJ.riseSpeed;

      switch (obj->state2) {
      case 0:
         if (OBJ.riseOfs >= CV(3.5)) {
            obj->state = 0;
            sprite->functionIndex = OBJF_NULL;
         }
         break;
      case 99:
         if (OBJ.riseOfs >= CV(3.5)) {
            obj->functionIndex = OBJF_NULL;
            sprite->functionIndex = OBJF_NULL;
         }
         break;
      }

      break;
   }
}

#undef OBJF
#define OBJF 197
void Objf197_RollingThunder_FX1(Object *obj) {
   Object *obj_s1;
   Object *fxSprite1;
   Object *fxSprite2;
   s32 spriteX, spriteY, spriteW, spriteH;

   switch (obj->state) {
   case 0:
      if (obj->mem == 0) {
         obj_s1 = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
      } else {
         obj_s1 = OBJ.variant_0x24.unitSpriteParam;
         OBJ.variant_0x24.unitSpriteParam = NULL;
      }

      OBJ.unitSprite = obj_s1;
      obj->x1.n = obj_s1->x1.n;
      obj->y1.n = obj_s1->y1.n;
      obj->z1.n = obj_s1->z1.n;
      obj_s1->d.sprite.hidden = 1;

      obj_s1 = Obj_GetUnused();
      obj_s1->functionIndex = OBJF_ROLLING_THUNDER_CASTING_BOLT;
      obj_s1->d.objf198.parent = obj;
      OBJ.bolts[8] = obj_s1;

      obj_s1 = Obj_GetUnused();
      obj_s1->functionIndex = OBJF_ROLLING_THUNDER_CASTING_BOLT;
      obj_s1->d.objf198.parent = obj;
      OBJ.bolts[7] = obj_s1;

      obj_s1 = Obj_GetUnused();
      obj_s1->functionIndex = OBJF_ROLLING_THUNDER_CASTING_BOLT;
      obj_s1->d.objf198.parent = obj;
      OBJ.bolts[6] = obj_s1;

      obj_s1 = Obj_GetUnused();
      obj_s1->functionIndex = OBJF_ROLLING_THUNDER_CASTING_BOLT;
      obj_s1->d.objf198.parent = obj;
      OBJ.bolts[5] = obj_s1;

      obj_s1 = Obj_GetUnused();
      obj_s1->functionIndex = OBJF_ROLLING_THUNDER_CASTING_BOLT;
      obj_s1->d.objf198.parent = obj;
      OBJ.bolts[4] = obj_s1;

      obj_s1 = Obj_GetUnused();
      obj_s1->functionIndex = OBJF_ROLLING_THUNDER_CASTING_BOLT;
      obj_s1->d.objf198.parent = obj;
      OBJ.bolts[3] = obj_s1;

      obj_s1 = Obj_GetUnused();
      obj_s1->functionIndex = OBJF_ROLLING_THUNDER_CASTING_BOLT;
      obj_s1->d.objf198.parent = obj;
      OBJ.bolts[2] = obj_s1;

      obj_s1 = Obj_GetUnused();
      obj_s1->functionIndex = OBJF_ROLLING_THUNDER_CASTING_BOLT;
      obj_s1->d.objf198.parent = obj;
      OBJ.bolts[1] = obj_s1;

      obj_s1 = Obj_GetUnused();
      obj_s1->functionIndex = OBJF_ROLLING_THUNDER_CASTING_BOLT;
      obj_s1->d.objf198.parent = obj;
      OBJ.bolts[0] = obj_s1;

      OBJ.variant_0x24.timer = 0;
      obj->state++;

   // fallthrough
   case 1:
      obj_s1 = OBJ.unitSprite;
      GetUnitSpriteVramRect(obj_s1, &spriteX, &spriteY, &spriteW, &spriteH);
      ApplyMaskEffect(spriteX, spriteY, spriteW + 1, spriteH + 1, 432 << 2, 464, 0,
                      OBJ.variant_0x24.timer % 64, GFX_MASK_EFFECT_1, 1);

      fxSprite1 = Obj_GetUnused();
      CopyObject(obj_s1, fxSprite1);
      fxSprite1->d.sprite.hidden = 0;
      fxSprite1->d.sprite.gfxIdx = GFX_MASK_EFFECT_1;
      fxSprite1->d.sprite.clut = CLUT_PURPLES;
      fxSprite1->d.sprite.semiTrans = 2;

      if (obj_s1->d.sprite.gfxIdx >= 21 && obj_s1->d.sprite.gfxIdx <= 30) {
         fxSprite1->d.sprite.boxIdx = 1;
      } else {
         fxSprite1->d.sprite.boxIdx = 0;
      }

      AddObjPrim6(gGraphicsPtr->ot, fxSprite1, 0);

      fxSprite2 = Obj_GetUnused();
      CopyObject(obj_s1, fxSprite2);
      fxSprite2->d.sprite.hidden = 0;
      RenderUnitSprite(gGraphicsPtr->ot, fxSprite2, 0);

      OBJ.variant_0x24.timer++;
      if (OBJ.variant_0x24.timer == 25) {
         gSignal3 = 1;
      } else if (OBJ.variant_0x24.timer == 30) {
         obj_s1->d.sprite.hidden = 0; // unitSprite
         obj->functionIndex = OBJF_NULL;
         obj_s1 = OBJ.bolts[8];
         obj_s1->state = 99;
         obj_s1 = OBJ.bolts[7];
         obj_s1->state = 99;
         obj_s1 = OBJ.bolts[6];
         obj_s1->state = 99;
         obj_s1 = OBJ.bolts[5];
         obj_s1->state = 99;
         obj_s1 = OBJ.bolts[4];
         obj_s1->state = 99;
         obj_s1 = OBJ.bolts[3];
         obj_s1->state = 99;
         obj_s1 = OBJ.bolts[2];
         obj_s1->state = 99;
         obj_s1 = OBJ.bolts[1];
         obj_s1->state = 99;
         obj_s1 = OBJ.bolts[0];
         obj_s1->state = 99;
      }

      fxSprite2->functionIndex = OBJF_NULL;
      fxSprite1->functionIndex = OBJF_NULL;
      break;
   }
}

#undef OBJF
#define OBJF 198
void Objf198_RollingThunder_CastingBolt(Object *obj) {
   Object *parent;
   Object *sprite;
   s16 halfHeight;
   s16 a, b;

   switch (obj->state) {
   case 0:
      parent = OBJ.parent;
      obj->x1.n = parent->x1.n;
      obj->y1.n = parent->y1.n;
      obj->z1.n = parent->z1.n;
      OBJ.pulseTheta = 0;
      OBJ.theta = rand() % DEG(360);
      OBJ.pulseSpeed = rand() % 8 + 8;
      OBJ.baseHeight = rand() % 0xc0 + 0x40;
      OBJ.radiusTheta = 0;
      OBJ.rotationSpeed = rand() % 0x180 + 0x80;

      sprite = Obj_GetUnused();
      sprite->functionIndex = OBJF_NOOP;
      sprite->d.sprite.animData = sLightningAnimData_800febe4;
      sprite->d.sprite.clut = CLUT_BLUES;
      OBJ.sprite = sprite;
      obj->state++;

   // fallthrough
   case 1:
      sprite = OBJ.sprite;

      if (OBJ.pulseTheta <= DEG(90)) {
         a = OBJ.theta;
         halfHeight = 6 + 18 * rsin(OBJ.pulseTheta) / ONE;
         b = 16 + CV(3.0) * (ONE - rcos(OBJ.radiusTheta)) / ONE;

         sprite->x1.n = obj->x1.n + (b * rcos(a) / ONE);
         sprite->z1.n = obj->z1.n + (b * rsin(a) / ONE);
         sprite->y1.n = obj->y1.n + OBJ.baseHeight + halfHeight;

         gQuad_800fe63c[0].vx = -4;
         gQuad_800fe63c[0].vy = -halfHeight;
         gQuad_800fe63c[1].vx = 4;
         gQuad_800fe63c[1].vy = -halfHeight;
         gQuad_800fe63c[2].vx = -4;
         gQuad_800fe63c[2].vy = halfHeight;
         gQuad_800fe63c[3].vx = 4;
         gQuad_800fe63c[3].vy = halfHeight;

         UpdateObjAnimation(sprite);
         AddObjPrim6(gGraphicsPtr->ot, sprite, 0);

         OBJ.pulseTheta += OBJ.pulseSpeed;
         OBJ.theta += OBJ.rotationSpeed;
         OBJ.radiusTheta += 0x22;
      }

      break;

   case 99:
      sprite = OBJ.sprite;
      sprite->functionIndex = OBJF_NULL;
      obj->functionIndex = OBJF_NULL;
      break;
   }
}

