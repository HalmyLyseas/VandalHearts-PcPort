/* Spell casting effects, part 9 of the FX corpus (segment 0x6e9c4) -- a short code split
 * carrying a large shared data block.
 *
 * Code: the Faerie spell (Objf115_Faerie_FX2 drives the faerie sprite orbiting in toward
 * the target, spawning Objf116_Faerie_Sparkle every frame and Objf117_Faerie_SparkleTrail
 * every few, planting Objf118_Faerie_Target on arrival, then raising gSignal3 when it
 * withdraws) and Objf211_Avalanche_Boulder (an 11x9 vertex shell rotated per frame and
 * drawn as textured quads with per-face directional shading). Both are dispatched
 * data-driven through gSpellsEx -- see fx_060c38.c for the model.
 *
 * Data: the block at 0x800ff18c holds shared sprite animation tables -- impact, smoke,
 * salamander, flame, two explosions, faerie and two sparkle loops. Only the faerie and
 * sparkle tables are used by this file; the rest are consumed by neighbouring FX units,
 * and gSmokeAnimData_800ff1b0 had to be made non-static for fx_070604.c: the code split at
 * Objf211 is provisional and cuts across the shared block (see the in-file comment).
 *
 * The PC_FEAT gate in the boulder is the Tactical-only ice re-skin. */
#include "common.h"
#include "object.h"
#include "graphics.h"
#include "battle.h"
#include "field.h"

extern void ApplyMaskEffect(s16, s16, s16, s16, s16, s16, s16, s16, s16, s16);
#ifdef PC_FEAT
extern int gTacticalMode;   /* Stage 3 1.3 -- Tactical-only Avalanche ice re-skin (pc_balance.c) */
#endif

static s16 sImpactAnimData_800ff18c[18] = {5, GFX_IMPACT_1, 2, GFX_IMPACT_2, 2, GFX_IMPACT_3,
                                           2, GFX_IMPACT_4, 2, GFX_IMPACT_5, 2, GFX_IMPACT_6,
                                           2, GFX_IMPACT_7, 2, GFX_NULL,     0, GFX_NULL};

// Making global until I resolve the premature split at Objf211:
s16 gSmokeAnimData_800ff1b0[24] = {7, GFX_PUFF_1, 1, GFX_PUFF_2,  1, GFX_PUFF_3, 1, GFX_PUFF_4,
                                   1, GFX_PUFF_5, 1, GFX_PUFF_6,  1, GFX_PUFF_7, 1, GFX_PUFF_8,
                                   1, GFX_PUFF_9, 1, GFX_PUFF_10, 1, GFX_NULL,   0, GFX_NULL};

static s16 sSalamanderAnimData_800ff1e0[6] = {0, GFX_SALAMANDER_S, 1, GFX_NULL, 1, GFX_NULL};

static s16 sFlameAnimData_800ff1ec[20] = {
    7, GFX_FLAME_1, 1, GFX_FLAME_2, 1, GFX_FLAME_3, 1, GFX_FLAME_4, 1, GFX_FLAME_5,
    1, GFX_FLAME_6, 1, GFX_FLAME_7, 1, GFX_FLAME_8, 1, GFX_NULL,    1, GFX_NULL};

static s16 sExplosionAnimData_800ff214[26] = {
    7, GFX_EXPLOSION_1, 1, GFX_EXPLOSION_2,  1, GFX_EXPLOSION_3,  1, GFX_EXPLOSION_4,
    1, GFX_EXPLOSION_5, 1, GFX_EXPLOSION_6,  1, GFX_EXPLOSION_7,  2, GFX_EXPLOSION_8,
    2, GFX_EXPLOSION_9, 2, GFX_EXPLOSION_10, 2, GFX_EXPLOSION_11, 2, GFX_NULL,
    0, GFX_NULL};

static s16 sExplosionAnimData_800ff248[26] = {
    7, GFX_EXPLOSION_1, 1, GFX_EXPLOSION_2,  1, GFX_EXPLOSION_3,  1, GFX_EXPLOSION_4,
    1, GFX_EXPLOSION_5, 1, GFX_EXPLOSION_6,  1, GFX_EXPLOSION_7,  1, GFX_EXPLOSION_8,
    1, GFX_EXPLOSION_9, 1, GFX_EXPLOSION_10, 1, GFX_EXPLOSION_11, 1, GFX_NULL,
    0, GFX_NULL};

static s16 sFaerieAnimData_800ff27c[8] = {4, GFX_FAERIE_1, 2, GFX_FAERIE_2,
                                          2, GFX_NULL,     1, GFX_NULL};

static s16 sSparkleAnimData_800ff28c[22] = {7, GFX_SPARKLE_1, 1, GFX_SPARKLE_2, 1, GFX_SPARKLE_3,
                                            1, GFX_SPARKLE_4, 1, GFX_SPARKLE_5, 1, GFX_SPARKLE_4,
                                            1, GFX_SPARKLE_3, 1, GFX_SPARKLE_2, 1, GFX_SPARKLE_1,
                                            1, GFX_NULL,      0, GFX_NULL};

static s16 sSparkleAnimData_800ff2b8[70] = {
    7, GFX_SPARKLE_1, 1, GFX_SPARKLE_2, 1, GFX_SPARKLE_3, 1, GFX_SPARKLE_4, 1, GFX_SPARKLE_5,
    1, GFX_SPARKLE_4, 1, GFX_SPARKLE_3, 1, GFX_SPARKLE_2, 1, GFX_SPARKLE_1, 1, GFX_SPARKLE_2,
    1, GFX_SPARKLE_3, 1, GFX_SPARKLE_4, 1, GFX_SPARKLE_5, 1, GFX_SPARKLE_4, 1, GFX_SPARKLE_3,
    1, GFX_SPARKLE_2, 1, GFX_SPARKLE_1, 1, GFX_SPARKLE_2, 1, GFX_SPARKLE_3, 1, GFX_SPARKLE_4,
    1, GFX_SPARKLE_5, 1, GFX_SPARKLE_4, 1, GFX_SPARKLE_3, 1, GFX_SPARKLE_2, 1, GFX_SPARKLE_1,
    1, GFX_SPARKLE_2, 1, GFX_SPARKLE_3, 1, GFX_SPARKLE_4, 1, GFX_SPARKLE_5, 1, GFX_SPARKLE_4,
    1, GFX_SPARKLE_3, 1, GFX_SPARKLE_2, 1, GFX_SPARKLE_1, 1, GFX_NULL,      0, GFX_NULL};

//...

#undef OBJF
#define OBJF 118
void Objf118_Faerie_Target(Object *obj) {
   Object *targetSprite;
   Object *obj_s2;
   Object *obj_s3;
   s32 spriteX, spriteY, spriteW, spriteH;
   POLY_FT4 *poly;

   FntPrint("w1 :%d\n", OBJ.timer);
   FntPrint("w10:%d\n", OBJ.fade);

   switch (obj->state) {
   case 0:
      targetSprite = OBJ.targetSprite;
      targetSprite->d.sprite.hidden = 1;
      obj->state++;

   // fallthrough
   case 1:
      targetSprite = OBJ.targetSprite;
      obj->x1.n = targetSprite->x1.n;
      obj->z1.n = targetSprite->z1.n;
      obj->y1.n = targetSprite->y1.n;

      obj_s2 = Obj_GetUnused();
      CopyObject(targetSprite, obj_s2);
      obj_s3 = Obj_GetUnused();
      CopyObject(targetSprite, obj_s3);
      obj_s2->d.sprite.hidden = 0;
      obj_s3->d.sprite.hidden = 0;
      GetUnitSpriteVramRect(targetSprite, &spriteX, &spriteY, &spriteW, &spriteH);
      ApplyMaskEffect(spriteX, spriteY, spriteW + 1, spriteH + 1, 400 << 2, 320, 0,
                      -(obj->state3++ % 0x40), GFX_MASK_EFFECT_1, 0);
      obj_s2->d.sprite.gfxIdx = GFX_MASK_EFFECT_1;
      obj_s2->d.sprite.clut = OBJ.clut;
      obj_s2->d.sprite.semiTrans = 2;

      if (targetSprite->d.sprite.gfxIdx >= 21 && targetSprite->d.sprite.gfxIdx <= 30) {
         obj_s2->d.sprite.boxIdx = 1;
      } else {
         obj_s2->d.sprite.boxIdx = 0;
      }

      AddObjPrim6(gGraphicsPtr->ot, obj_s2, 0);
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      setRGB0(poly, OBJ.fade, OBJ.fade, OBJ.fade);

      RenderUnitSprite(gGraphicsPtr->ot, obj_s3, 0);
      obj_s2->functionIndex = OBJF_NULL;
      obj_s3->functionIndex = OBJF_NULL;

      switch (obj->state2) {
      case 0:
         OBJ.fade += 4;
         if (OBJ.fade > 0x40) {
            obj->state2++;
         }
         break;
      case 1:
         if (++OBJ.timer == 88) {
            obj->state2++;
            OBJ.unused_0x2C = 0x40;
         }
         break;
      case 2:
         OBJ.fade -= 4;
         if (OBJ.fade < 0) {
            targetSprite = OBJ.targetSprite;
            targetSprite->d.sprite.hidden = 0;
            obj->functionIndex = OBJF_NULL;
         }
         break;
      }

      break;
   }
}

#undef OBJF
#define OBJF 117
void Objf117_Faerie_SparkleTrail(Object *obj) {
   Object *sprite;
   s32 rnd;
   s16 halfSize;
   POLY_FT4 *poly;

   switch (obj->state) {
   case 0:
      sprite = Obj_GetUnused();
      sprite->functionIndex = OBJF_NOOP;
      sprite->d.sprite.animData = sSparkleAnimData_800ff2b8;
      sprite->d.sprite.semiTrans = 2;
      OBJ.sprite = sprite;
      OBJ.fade = 0x180;
      obj->y2.n = obj->y1.n;
      obj->y3.n = -((rand() >> 2) % 15) - 2;
      obj->state2 = (rand() >> 2) % 3 + 1;
      obj->state++;

   // fallthrough
   case 1:
      sprite = OBJ.sprite;
      UpdateObjAnimation(sprite);

      if (sprite->d.sprite.animFinished) {
         sprite->functionIndex = OBJF_NULL;
         obj->functionIndex = OBJF_NULL;
      } else {
         sprite->x1.n = obj->x1.n;
         sprite->z1.n = obj->z1.n;
         sprite->y1.n = obj->y2.n;
         obj->y2.n += obj->y3.n;

         halfSize = obj->state2;
         gQuad_800fe63c[0].vx = -halfSize;
         gQuad_800fe63c[0].vy = -halfSize;
         gQuad_800fe63c[1].vx = halfSize;
         gQuad_800fe63c[1].vy = -halfSize;
         gQuad_800fe63c[2].vx = -halfSize;
         gQuad_800fe63c[2].vy = halfSize;
         gQuad_800fe63c[3].vx = halfSize;
         gQuad_800fe63c[3].vy = halfSize;

         AddObjPrim6(gGraphicsPtr->ot, sprite, 0);
         poly = &gGraphicsPtr->quads[gQuadIndex - 1];
         setRGB0(poly, OBJ.fade / 2, OBJ.fade / 2, OBJ.fade / 2);

         OBJ.fade += obj->y3.n;
         if (OBJ.fade <= 0) {
            OBJ.fade = 0;
         }
      }

      break;
   }
}

#undef OBJF
#define OBJF 116
void Objf116_Faerie_Sparkle(Object *obj) {
   Object *sprite;
   POLY_FT4 *poly;

   switch (obj->state) {
   case 0:
      sprite = Obj_GetUnused();
      sprite->functionIndex = OBJF_NOOP;
      sprite->d.sprite.animData = sSparkleAnimData_800ff28c;
      sprite->d.sprite.semiTrans = 2;
      OBJ.sprite = sprite;

      OBJ.xOfs = 0x40 - rand() % 0x81;
      OBJ.zOfs = 0x40 - rand() % 0x81;
      OBJ.yOfs = 0x40 - rand() % 0x81;
      OBJ.fade = 0x80;

      obj->state++;

   // fallthrough
   case 1:
      sprite = OBJ.sprite;
      UpdateObjAnimation(sprite);
      if (sprite->d.sprite.animFinished) {
         sprite->functionIndex = OBJF_NULL;
         obj->functionIndex = OBJF_NULL;
      } else {
         sprite->x1.n = obj->x1.n + OBJ.xOfs;
         sprite->z1.n = obj->z1.n + OBJ.zOfs;
         sprite->y1.n = obj->y1.n + OBJ.yOfs;

         gQuad_800fe63c[0].vx = -4;
         gQuad_800fe63c[0].vy = -4;
         gQuad_800fe63c[1].vx = 4;
         gQuad_800fe63c[1].vy = -4;
         gQuad_800fe63c[2].vx = -4;
         gQuad_800fe63c[2].vy = 4;
         gQuad_800fe63c[3].vx = 4;
         gQuad_800fe63c[3].vy = 4;

         AddObjPrim6(gGraphicsPtr->ot, sprite, 0);
         poly = &gGraphicsPtr->quads[gQuadIndex - 1];
         setRGB0(poly, OBJ.fade, OBJ.fade, OBJ.fade);
         OBJ.fade -= 14;
      }

      break;
   }
}

s32 s_unused_80123340;

#undef OBJF
#define OBJF 115
void Objf115_Faerie_FX2(Object *obj) {
   Object *obj_s1;

   switch (obj->state) {
   case 0:
      s_unused_80123340 = 0;
      obj_s1 = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
      OBJ.targetSprite = obj_s1;
      obj->y1.n = GetTerrainElevation(obj->z1.s.hi, obj->x1.s.hi);
      obj->x1.n = obj_s1->x1.n;
      obj->z1.n = obj_s1->z1.n;

      obj_s1 = Obj_GetUnused();
      obj_s1->functionIndex = OBJF_NOOP;
      obj_s1->d.sprite.animData = sFaerieAnimData_800ff27c;
      OBJ.faerieSprite = obj_s1;

      OBJ.dist = 0x200;
      obj->mem = 3;
      obj->state++;

   // fallthrough
   case 1:
      obj_s1 = OBJ.faerieSprite;
      obj_s1->x1.n = obj->x1.n + (OBJ.dist + CV(0.625)) * rcos(OBJ.theta) / ONE;
      obj_s1->z1.n = obj->z1.n + (OBJ.dist + CV(0.625)) * rsin(OBJ.theta) / ONE;
      obj_s1->y1.n = obj->y1.n + CV(1.5) + OBJ.dist;
      UpdateObjAnimation(obj_s1);
      AddObjPrim6(gGraphicsPtr->ot, obj_s1, 0);

      CreatePositionedObj(obj_s1, OBJF_FAERIE_SPARKLE);

      if (--obj->mem <= 0) {
         CreatePositionedObj(obj_s1, OBJF_FAERIE_SPARKLE_TRAIL);
         obj->mem = (rand() >> 2) % 2 + 1;
      }

      OBJ.theta += 0x60;

      switch (obj->state2) {
      case 0:
         OBJ.dist -= 10;
         if (OBJ.dist < 0) {
            OBJ.dist = 0;
            obj->state2++;
            obj_s1 = Obj_GetUnused();
            obj_s1->functionIndex = OBJF_FAERIE_TARGET;
            obj_s1->x1.n = obj->x1.n;
            obj_s1->z1.n = obj->z1.n;
            obj_s1->y1.n = obj->y1.n;
            obj_s1->d.objf118.targetSprite = OBJ.targetSprite;
            obj_s1->d.objf118.clut = CLUT_BLUES;
         }
         break;

      case 1:
         if (++OBJ.timer == 51) {
            obj->state2++;
         }
         break;

      case 2:
         OBJ.dist += 10;
         if (OBJ.dist == 220) {
            obj_s1 = Obj_GetUnused();
            obj_s1->functionIndex = OBJF_DISPLAY_DAMAGE_2;
            obj_s1->x1.s.hi = obj->x1.s.hi;
            obj_s1->z1.s.hi = obj->z1.s.hi;
         }
         if (OBJ.dist > 0x200) {
            obj->functionIndex = OBJF_NULL;
            gSignal3 = 1;
            obj_s1->functionIndex = OBJF_NULL;
         }
         break;
      }

      break;
   }
}

#undef OBJF
#define OBJF 211
void Objf211_Avalanche_Boulder(Object *obj) {
   extern SVECTOR s_vertices_80123b0c[11][9];

   Object *unitSprite;
   Object *obj_s2;
   POLY_FT4 *poly;
   s32 i, j, k;
   s16 iVar5s;
   s16 iVar8s;
   s16 iVar10s;
   s16 a, b;
   s32 tmp_0x200;
   s32 tmp_0x300;
   MATRIX matrix;
   VECTOR rotated[11][9];
   SVECTOR rotation;
   s32 flag;
   // s8 unused[8];

   switch (obj->state) {
   case 0:
      unitSprite = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
      obj->y1.n = GetTerrainElevation(obj->z1.s.hi, obj->x1.s.hi) + CV(2.0);
      obj->x1.n = unitSprite->x1.n;
      obj->z1.n = unitSprite->z1.n;

      tmp_0x300 = 0x300;
      // tmp_0x200 = 0x200;

      for (i = 0; i < 11; i++) {
         // Matches except for stack when using tmp_0x200, but trying to pad the stack shifts the
         // compiler temp (?); this gets around that but looks very fake (FIXME)
         s32 tmp;
         a = (tmp_0x300 - 0x100);
         tmp = a / 2;
         iVar8s = (tmp * rsin(i * 0xcd)) / ONE;
         // iVar8s = ((tmp_0x200 / 2) * rsin(i * 0xcd)) / ONE;
         iVar10s = (tmp_0x300 / 2) * rcos(i * 0xcd + 0x800) / ONE;
         for (j = 0; j < 9; j++) {
            s_vertices_80123b0c[i][j].vx = iVar8s * rcos(j * 0x200) / ONE;
            s_vertices_80123b0c[i][j].vy = iVar8s * rsin(j * 0x200) / ONE;
            s_vertices_80123b0c[i][j].vz = iVar10s;
         }
      }

      for (k = 0; k < 256; k++) {
         i = rand() % 11;
         j = rand() % 8;
         a = 0x140 * rsin(i * 0xcd) / ONE;

         if (i == 0) {
            iVar5s = 8 - rand() % 17;
            iVar8s = 8 - rand() % 17;
            iVar10s = 8 - rand() % 17;
            a = rand() % 8;

            for (j = 0; j < 8; j++) {
               s_vertices_80123b0c[i][j].vx =
                   (s_vertices_80123b0c[i][j].vx * 7 + s_vertices_80123b0c[i + 1][a].vx) / 8 +
                   iVar5s;
               s_vertices_80123b0c[i][j].vz =
                   (s_vertices_80123b0c[i][j].vz * 7 + s_vertices_80123b0c[i + 1][a].vz) / 8 +
                   iVar8s;
               s_vertices_80123b0c[i][j].vy =
                   (s_vertices_80123b0c[i][j].vy * 7 + s_vertices_80123b0c[i + 1][a].vy) / 8 +
                   iVar10s;
            }
         } else if (i == 10) {
            iVar5s = 8 - rand() % 17;
            iVar8s = 8 - rand() % 17;
            iVar10s = 8 - rand() % 17;
            a = rand() % 8;

            for (j = 0; j < 8; j++) {
               s_vertices_80123b0c[i][j].vx =
                   (s_vertices_80123b0c[i][j].vx * 7 + s_vertices_80123b0c[i - 1][a].vx) / 8 +
                   iVar5s;
               s_vertices_80123b0c[i][j].vz =
                   (s_vertices_80123b0c[i][j].vz * 7 + s_vertices_80123b0c[i - 1][a].vz) / 8 +
                   iVar8s;
               s_vertices_80123b0c[i][j].vy =
                   (s_vertices_80123b0c[i][j].vy * 7 + s_vertices_80123b0c[i - 1][a].vy) / 8 +
                   iVar10s;
            }
         } else {
            s_vertices_80123b0c[i][j].vx =
                (s_vertices_80123b0c[i][j].vx * 4 + s_vertices_80123b0c[i + 1][j].vx +
                 s_vertices_80123b0c[i - 1][j].vx + s_vertices_80123b0c[i][j + 1].vx +
                 s_vertices_80123b0c[i][j - 1].vx) /
                    8 +
                (a * (48 - rand() % 97) / 0x100);

            s_vertices_80123b0c[i][j].vy =
                (s_vertices_80123b0c[i][j].vy * 4 + s_vertices_80123b0c[i + 1][j].vy +
                 s_vertices_80123b0c[i - 1][j].vy + s_vertices_80123b0c[i][j + 1].vy +
                 s_vertices_80123b0c[i][j - 1].vy) /
                    8 +
                (a * (48 - rand() % 97) / 0x100);

            s_vertices_80123b0c[i][j].vz =
                (s_vertices_80123b0c[i][j].vz * 4 + s_vertices_80123b0c[i + 1][j].vz +
                 s_vertices_80123b0c[i - 1][j].vz + s_vertices_80123b0c[i][j + 1].vz +
                 s_vertices_80123b0c[i][j - 1].vz) /
                    8 +
                (a * (48 - rand() % 97) / 0x100);
         }
      }

      for (i = 0; i < 11; i++) {
         s_vertices_80123b0c[i][8].vx = s_vertices_80123b0c[i][0].vx;
         s_vertices_80123b0c[i][8].vy = s_vertices_80123b0c[i][0].vy;
         s_vertices_80123b0c[i][8].vz = s_vertices_80123b0c[i][0].vz;
      }

      obj->state++;

   // fallthrough
   case 1:
      obj_s2 = Obj_GetUnused();
      obj_s2->functionIndex = OBJF_NOOP;
      obj_s2->d.sprite.gfxIdx = GFX_TILED_STONE;
      obj_s2->d.sprite.clut = CLUT_GRAYS;
#ifdef PC_FEAT
      /* Tactical cosmetic re-skin: turn Avalanche's boulder to ice. Texture -> GFX_TILED_ICE (the
       * crystalline pattern Ice Storm uses, Objf189), palette -> CLUT_GRAYS for a frosty WHITE read
       * (Ice Storm pairs it with CLUT_BLUES, but at boulder scale blue looked like water; there is no
       * CLUT_WHITES, and GRAYS is the white/light ramp -- same one the retail stone used). Retail stays
       * grey stone (GFX_TILED_STONE + CLUT_GRAYS) in Normal mode. NOTE: only the boulder recolors -- the
       * tumbling GFX_ROCK_* debris and GFX_PUFF dust are baked-palette sprites with no settable CLUT, so
       * they stay rocky until snow sprites are drawn. */
      if (gTacticalMode) {
         obj_s2->d.sprite.gfxIdx = GFX_TILED_ICE;
         obj_s2->d.sprite.clut = CLUT_GRAYS;
      }
#endif

      iVar8s = OBJ.scale;
      PushMatrix();
      rotation.vx = OBJ.rotX;
      rotation.vz = OBJ.rotZ;
      rotation.vy = OBJ.rotY;
      RotMatrix(&rotation, &matrix);
      matrix.t[0] = 0;
      matrix.t[1] = 0;
      matrix.t[2] = 0;
      SetRotMatrix(&matrix);
      SetTransMatrix(&matrix);

      for (i = 0; i < 11; i++) {
         for (j = 0; j < 9; j++) {
            RotTrans(&s_vertices_80123b0c[i][j], &rotated[i][j], &flag);
         }
      }
      PopMatrix();

      for (i = 0; i < 10; i++) {
         for (j = 0; j < 8; j++) {
            obj_s2->d.sprite.coords[0].x = obj->x1.n + iVar8s * rotated[i + 1][j].vx / 128;
            obj_s2->d.sprite.coords[0].y = obj->y1.n + iVar8s * rotated[i + 1][j].vy / 128;
            obj_s2->d.sprite.coords[0].z = obj->z1.n + iVar8s * rotated[i + 1][j].vz / 128;
            obj_s2->d.sprite.coords[1].x = obj->x1.n + iVar8s * rotated[i + 1][j + 1].vx / 128;
            obj_s2->d.sprite.coords[1].y = obj->y1.n + iVar8s * rotated[i + 1][j + 1].vy / 128;
            obj_s2->d.sprite.coords[1].z = obj->z1.n + iVar8s * rotated[i + 1][j + 1].vz / 128;
            obj_s2->d.sprite.coords[2].x = obj->x1.n + iVar8s * rotated[i][j].vx / 128;
            obj_s2->d.sprite.coords[2].y = obj->y1.n + iVar8s * rotated[i][j].vy / 128;
            obj_s2->d.sprite.coords[2].z = obj->z1.n + iVar8s * rotated[i][j].vz / 128;
            obj_s2->d.sprite.coords[3].x = obj->x1.n + iVar8s * rotated[i][j + 1].vx / 128;
            obj_s2->d.sprite.coords[3].y = obj->y1.n + iVar8s * rotated[i][j + 1].vy / 128;
            obj_s2->d.sprite.coords[3].z = obj->z1.n + iVar8s * rotated[i][j + 1].vz / 128;

            iVar10s = 128;
            if (obj_s2->d.sprite.coords[0].x - obj_s2->d.sprite.coords[1].x < 0) {
               iVar10s -= 16;
            }
            if (obj_s2->d.sprite.coords[2].x - obj_s2->d.sprite.coords[3].x < 0) {
               iVar10s -= 16;
            }
            if (obj_s2->d.sprite.coords[0].z - obj_s2->d.sprite.coords[1].z < 0) {
               iVar10s -= 16;
            }
            if (obj_s2->d.sprite.coords[2].z - obj_s2->d.sprite.coords[3].z < 0) {
               iVar10s -= 16;
            }
            if (obj_s2->d.sprite.coords[0].y - obj_s2->d.sprite.coords[2].y < 0) {
               iVar10s -= 16;
            }
            if (obj_s2->d.sprite.coords[1].y - obj_s2->d.sprite.coords[3].y < 0) {
               iVar10s -= 16;
            }

            AddObjPrim4(gGraphicsPtr->ot, obj_s2);
            poly = &gGraphicsPtr->quads[gQuadIndex - 1];
#ifdef PC_FEAT
            /* Tactical ice re-skin: lift the boulder's grey directional shading toward white so the ice
             * reads snowier. Retail shades faces in [32..128] (128 = GPU-neutral, darker faces down to
             * 32); +64 pushes lit faces above neutral (~192) while keeping the per-face contrast. iVar10s
             * is recomputed to 128 each face, so this never accumulates. Boulder-only + Tactical-only --
             * no other SFX or Normal-mode Avalanche is touched (pairs with the gfxIdx/clut swap above). */
            if (gTacticalMode) {
               iVar10s += 64;
            }
#endif
            setRGB0(poly, iVar10s, iVar10s, iVar10s);
         }
      }

      obj_s2->functionIndex = OBJF_NULL;
      break;
   }
   //{ s8 unused[8]; }
}
