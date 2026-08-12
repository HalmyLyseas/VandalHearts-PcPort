/* Spell casting effects, part 1 of the FX corpus (segment 0x60c38).
 *
 * Mostly _FX1 handlers: the "main" casting visual each spell plays at the caster/target,
 * dispatched data-driven through gSpellsEx[spellId][SPELL_EX_OBJF_MAIN/TARGET/DEFEAT]
 * (see battle_executors.c) -- there is deliberately no static spawn site for these. The
 * retail gSpellsEx data is the authoritative handler->spell map and was used to verify
 * every name in this file. Sub-objects (rays, rocks, glyphs, swirl strokes) are spawned
 * by their _FX1 driver and named <Spell>_<Thing>.
 *
 * Confirmed-unused handlers (in gObjFunctionPointers but referenced by no spell's
 * gSpellsEx triple and no code path): the spiked-ball toss pair (Objf182/183), the
 * explosion-strike pair (Objf186/187), and the single-target Magic Arrow (Objf190) --
 * cut content, kept byte-exact, suffixed _Unused. */
#include "common.h"
#include "object.h"
#include "graphics.h"
#include "battle.h"
#include "field.h"

extern void ApplyMaskEffect(s16, s16, s16, s16, s16, s16, s16, s16, s16, s16);

#undef OBJF
#define OBJF 172
void Objf172_HolyPressure_Cube(Object *obj) {
   s16 local_88[6][4] = {{0, 256, 0, 0},    {-135, 0, -135, 0}, {135, 0, -135, 0},
                         {-135, 0, 135, 0}, {135, 0, 135, 0},   {0, -256, 0, 0}};
   s16 local_58[8][3] = {{0, 1, 2}, {0, 2, 3}, {0, 3, 4}, {0, 4, 1},
                         {5, 1, 2}, {5, 2, 3}, {5, 3, 4}, {5, 4, 1}};

   s32 i, j;
   s16 a;
   Object *obj_s4;

   FntPrint("8side:%d\n", obj->state);

   switch (obj->state) {
   case 0:
      obj_s4 = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
      obj->y1.n = obj_s4->y1.n + CV(0.5);
      obj->x1.n = obj_s4->x1.n;
      obj->z1.n = obj_s4->z1.n;
      obj->state++;

   // fallthrough
   case 1:
      obj_s4 = Obj_GetUnused();
      obj_s4->functionIndex = OBJF_NOOP;
      obj_s4->d.sprite.semiTrans = 4;
      obj_s4->d.sprite.clut = CLUT_PURPLES;
      obj_s4->d.sprite.gfxIdx = GFX_TILED_TBD_162;

      a = OBJ.spin;
      for (i = 0; i < 2; i++) {
         switch (obj->state3) {
         case 0:
            for (j = 1; j < 5; j++) {
               local_88[j][0] = 0x100 * rsin(i * 0x20 + a + (j - 1) * 0x400 + 0x200) >> 12;
               local_88[j][2] = 0x100 * rcos(i * 0x20 + a + (j - 1) * 0x400 + 0x200) >> 12;
               local_88[j][0] = local_88[j][0] * rsin(obj->state2) >> 12;
               local_88[j][2] = local_88[j][2] * rsin(obj->state2) >> 12;
            }
            local_88[0][1] = 0x100 - (0x100 * rcos(obj->state2) >> 12);
            local_88[5][1] = -local_88[0][1];
            break;

         case 1:
            for (j = 1; j < 5; j++) {
               local_88[j][0] = 0x100 * rsin(i * 0x20 + a + (j - 1) * 0x400 + 0x200) / ONE;
               local_88[j][2] = 0x100 * rcos(i * 0x20 + a + (j - 1) * 0x400 + 0x200) / ONE;
            }
            break;

         case 2:
            for (j = 1; j < 5; j++) {
               local_88[j][0] = 0x100 * rsin(i * 0x20 + a + (j - 1) * 0x400 + 0x200) >> 12;
               local_88[j][2] = 0x100 * rcos(i * 0x20 + a + (j - 1) * 0x400 + 0x200) >> 12;
               local_88[j][0] = local_88[j][0] * rcos(obj->state2) >> 12;
               local_88[j][2] = local_88[j][2] * rcos(obj->state2) >> 12;
            }
            local_88[0][1] = 0x100;
            local_88[5][1] = -0x100;
            break;
         }

         for (j = 0; j < 8; j++) {
            obj_s4->d.sprite.coords[0].x = local_88[local_58[j][0]][0] + obj->x1.n;
            obj_s4->d.sprite.coords[0].z = local_88[local_58[j][0]][2] + obj->z1.n;
            obj_s4->d.sprite.coords[0].y = local_88[local_58[j][0]][1] + obj->y1.n;
            obj_s4->d.sprite.coords[1].x = obj_s4->d.sprite.coords[0].x;
            obj_s4->d.sprite.coords[1].z = obj_s4->d.sprite.coords[0].z;
            obj_s4->d.sprite.coords[1].y = obj_s4->d.sprite.coords[0].y;
            obj_s4->d.sprite.coords[2].x = local_88[local_58[j][1]][0] + obj->x1.n;
            obj_s4->d.sprite.coords[2].z = local_88[local_58[j][1]][2] + obj->z1.n;
            obj_s4->d.sprite.coords[2].y = local_88[local_58[j][1]][1] + obj->y1.n;
            obj_s4->d.sprite.coords[3].x = local_88[local_58[j][2]][0] + obj->x1.n;
            obj_s4->d.sprite.coords[3].z = local_88[local_58[j][2]][2] + obj->z1.n;
            obj_s4->d.sprite.coords[3].y = local_88[local_58[j][2]][1] + obj->y1.n;
            AddObjPrim4(gGraphicsPtr->ot, obj_s4);
         }
      } // for (i)

      obj_s4->functionIndex = OBJF_NULL;

      switch (obj->state3) {
      case 0:
         OBJ.spin += 0x40;
         obj->state2 += 0x20;
         if (obj->state2 >= 0x400) {
            obj->state3++;
            obj->state2 = 0;
         }
         break;

      case 1:
         OBJ.spin += 0x40;
         if (++OBJ.timer == 90) {
            obj->state3++;
         }
         break;

      case 2:
         OBJ.spin += 0x40;
         obj->state2 += 0x20;
         if (obj->state2 >= 0x400) {
            obj->functionIndex = OBJF_NULL;
         }
         break;
      }

      break;
   }
}

void Objf128_ThunderBall_FX2(Object *obj) {
   Object *unitSprite;
   Object *fx;

   unitSprite = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
   fx = Obj_GetUnused();
   fx->functionIndex = OBJF_THUNDER_STRIKE;
   fx->x1.n = unitSprite->x1.n;
   fx->y1.n = unitSprite->y1.n;
   fx->z1.n = unitSprite->z1.n;
   fx->d.objf148.endingFxType = 0;

   obj->functionIndex = OBJF_NULL;
}

void Objf129_ThunderBall_FX3(Object *obj) {
   Object *unitSprite;
   Object *fx;

   unitSprite = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
   fx = Obj_GetUnused();
   fx->functionIndex = OBJF_THUNDER_STRIKE;
   fx->x1.n = unitSprite->x1.n;
   fx->y1.n = unitSprite->y1.n;
   fx->z1.n = unitSprite->z1.n;
   fx->d.objf148.endingFxType = 2;

   obj->functionIndex = OBJF_NULL;
}

#undef OBJF
#define OBJF 148
/* Sky lightning bolt (Objf147) onto the target, then the lightning engulf after 25 frames.
 * Shared strike finisher: Thunder Ball / HelStone route here via Objf128/129, and the
 * spells_lightning.c Objf120/121 pair does the same for its own driver. */
void Objf148_ThunderStrike(Object *obj) {
   Object *obj_v1;

   switch (obj->state) {
   case 0:

      switch (obj->state2) {
      case 0:
         if (OBJ.timer == 0) {
            OBJ.clut = CLUT_REDS;
         }
         obj_v1 = Obj_GetUnused();
         obj_v1->functionIndex = OBJF_LIGHTNING_BOLT;
         obj_v1->x1.n = obj->x1.n;
         obj_v1->y1.n = obj->y1.n;
         obj_v1->z1.n = obj->z1.n;
         obj_v1->d.objf147.clut = OBJ.clut;
         OBJ.timer = 25;
         obj->state2++;

      // fallthrough
      case 1:
         if (--OBJ.timer == -1) {
            obj_v1 = Obj_GetUnused();
            obj_v1->functionIndex = OBJF_ENGULF_LIGHTNING_DAMAGE + OBJ.endingFxType;
            obj_v1->d.objf136.clut = OBJ.clut;
            obj_v1->x1.n = obj->x1.n;
            obj_v1->y1.n = obj->y1.n;
            obj_v1->z1.n = obj->z1.n;
            obj->functionIndex = OBJF_NULL;
         }
         break;
      }

      break;
   }
}

static s16 sColorsAnimData_800feea8[16] = {5, GFX_COLOR_10, 1, GFX_COLOR_11, 1, GFX_COLOR_12,
                                           1, GFX_COLOR_13, 1, GFX_COLOR_12, 1, GFX_COLOR_11,
                                           1, GFX_NULL,     1, GFX_NULL};

static s16 sExplosionAnimData_800feec8[12] = {4, GFX_EXPLOSION_1, 1, GFX_EXPLOSION_2,
                                              1, GFX_EXPLOSION_3, 1, GFX_EXPLOSION_2,
                                              1, GFX_NULL,        1, GFX_NULL};

static s16 sColorsAnimData_800feee0[64] = {
    4, GFX_COLOR_1,  1, GFX_COLOR_2,  1, GFX_COLOR_3,  1, GFX_COLOR_4,  1, GFX_COLOR_5,
    1, GFX_COLOR_6,  1, GFX_COLOR_7,  1, GFX_COLOR_8,  1, GFX_COLOR_9,  1, GFX_COLOR_10,
    1, GFX_COLOR_11, 1, GFX_COLOR_12, 1, GFX_COLOR_13, 1, GFX_COLOR_12, 1, GFX_COLOR_13,
    1, GFX_COLOR_14, 1, GFX_COLOR_15, 1, GFX_COLOR_14, 1, GFX_COLOR_13, 1, GFX_COLOR_12,
    1, GFX_COLOR_11, 1, GFX_COLOR_10, 1, GFX_COLOR_9,  1, GFX_COLOR_8,  1, GFX_COLOR_7,
    1, GFX_COLOR_6,  1, GFX_COLOR_5,  1, GFX_COLOR_4,  1, GFX_COLOR_3,  1, GFX_COLOR_2,
    1, GFX_NULL,     1, GFX_NULL};

#undef OBJF
#define OBJF 167
void Objf167_RainbowSwirl(Object *obj) {
   // Swirling strips of translucent rainbow colors;
   // Used during FX1 of Spread Force and Rainbow Storm / Unicorn
   s16 rainbowCluts[4] = {3, 4, 8, 9};
   Object *sprite;
   s32 i;
   s16 a, b, c;
   s16 angle, angleOfs;
   s16 sVar2;
   s32 sum;

   switch (obj->state) {
   case 0:
      a = OBJ.spin;
      OBJ.theta1 = rand() % 0x1000;
      OBJ.span1 = rand() % 0x300 + 0x300;
      OBJ.speed1 = rand() % a + a;
      OBJ.y1 = -0x600;
      OBJ.theta2 = rand() % 0x1000;
      OBJ.span2 = rand() % 0x300 + 0x300;
      OBJ.speed2 = rand() % a + a;
      OBJ.y2 = -0x600;
      OBJ.theta3 = rand() % 0x1000;
      OBJ.span3 = rand() % 0x300 + 0x300;
      OBJ.speed3 = rand() % a + a;
      OBJ.y3 = -0x600;
      OBJ.theta4 = rand() % 0x1000;
      OBJ.span4 = rand() % 0x300 + 0x300;
      OBJ.speed4 = rand() % a + a;
      OBJ.y4 = -0x600;
      OBJ.theta5 = rand() % 0x1000;
      OBJ.span5 = rand() % 0x300 + 0x300;
      OBJ.speed5 = rand() % a + a;
      OBJ.y5 = -0x600;
      OBJ.theta6 = rand() % 0x1000;
      OBJ.span6 = rand() % 0x300 + 0x300;
      OBJ.speed6 = rand() % a + a;
      OBJ.y6 = -0x600;

      sprite = Obj_GetUnused();
      sprite->functionIndex = OBJF_NOOP;
      sprite->d.sprite.animData = sColorsAnimData_800feee0;
      sprite->d.sprite.animInitialized = 0;
      OBJ.sprite = sprite;

      obj->state++;
      break;

   case 1:
      sprite = OBJ.sprite;
      sprite->d.sprite.semiTrans = OBJ.semiTrans;
      sprite->x1.n = obj->x1.n;
      sprite->z1.n = obj->z1.n;
      sprite->y1.n = obj->y1.n;

      sum = 0;
      angle = OBJ.spin;
      OBJ.spin += 0x60;
      sVar2 = OBJ.arcWidth;
      b = OBJ.radius;

      for (i = 0; i < 6; i++) {
         switch (i) {
         case 0:
            a = OBJ.y1;
            c = OBJ.span1 + a;
            angleOfs = OBJ.theta1;
            break;
         case 1:
            a = OBJ.y2;
            c = OBJ.span2 + a;
            angleOfs = OBJ.theta2;
            break;
         case 2:
            a = OBJ.y3;
            c = OBJ.span3 + a;
            angleOfs = OBJ.theta3;
            break;
         case 3:
            a = OBJ.y4;
            c = OBJ.span4 + a;
            angleOfs = OBJ.theta4;
            break;
         case 4:
            a = OBJ.y5;
            c = OBJ.span5 + a;
            angleOfs = OBJ.theta5;
            break;
         case 5:
            a = OBJ.y6;
            c = OBJ.span6 + a;
            angleOfs = OBJ.theta6;
            break;
         }

         if (a >= 0x800) {
            a = 0x800;
         } else if (a <= 0) {
            a = 0;
         }
         if (c >= 0x800) {
            c = 0x800;
         } else if (c <= 0) {
            c = 0;
         }

         sum += c;

         sprite->d.sprite.coords[0].x = obj->x1.n + (b * rcos(angle + angleOfs) >> 12);
         sprite->d.sprite.coords[0].y = obj->y1.n + a;
         sprite->d.sprite.coords[0].z = obj->z1.n + (b * rsin(angle + angleOfs) >> 12);
         sprite->d.sprite.coords[1].x = obj->x1.n + (b * rcos(angle + angleOfs + sVar2) >> 12);
         sprite->d.sprite.coords[1].y = obj->y1.n + a;
         sprite->d.sprite.coords[1].z = obj->z1.n + (b * rsin(angle + angleOfs + sVar2) >> 12);
         sprite->d.sprite.coords[2].x = obj->x1.n + (b * rcos(angle + angleOfs) >> 12);
         sprite->d.sprite.coords[2].y = obj->y1.n + c;
         sprite->d.sprite.coords[2].z = obj->z1.n + (b * rsin(angle + angleOfs) >> 12);
         sprite->d.sprite.coords[3].x = obj->x1.n + (b * rcos(angle + angleOfs + sVar2) >> 12);
         sprite->d.sprite.coords[3].y = obj->y1.n + c;
         sprite->d.sprite.coords[3].z = obj->z1.n + (b * rsin(angle + angleOfs + sVar2) >> 12);

         sprite->x1.n = (sprite->d.sprite.coords[0].x + sprite->d.sprite.coords[3].x) / 2;
         sprite->y1.n = sprite->d.sprite.coords[3].y;
         sprite->z1.n = (sprite->d.sprite.coords[0].z + sprite->d.sprite.coords[3].z) / 2;

         UpdateObjAnimation(sprite);
         sprite->d.sprite.clut = rainbowCluts[rand() % 4];
         AddObjPrim3(gGraphicsPtr->ot, sprite);
      }

      OBJ.y1 += OBJ.speed1;
      OBJ.y2 += OBJ.speed2;
      OBJ.y3 += OBJ.speed3;
      OBJ.y4 += OBJ.speed4;
      OBJ.y5 += OBJ.speed5;
      OBJ.y6 += OBJ.speed6;

      if (sum == 0x3000) {
         obj->functionIndex = OBJF_NULL;
         sprite->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 168
void Objf168_RainbowStroke_RainbowSwirl(Object *obj) {
   s16 rainbowCluts[4] = {3, 4, 8, 9};
   Object *sprite;
   s32 i;
   s16 a, b, c;
   s16 angle, angleOfs;
   s16 sVar2;
   s32 sum;

   switch (obj->state) {
   case 0:
      a = OBJ.spin;
      OBJ.theta1 = rand() % 0x1000;
      OBJ.span1 = rand() % 0x300 + 0x300;
      OBJ.speed1 = rand() % a + a;
      OBJ.y1 = 0x600;
      OBJ.theta2 = rand() % 0x1000;
      OBJ.span2 = rand() % 0x300 + 0x300;
      OBJ.speed2 = rand() % a + a;
      OBJ.y2 = 0x600;
      OBJ.theta3 = rand() % 0x1000;
      OBJ.span3 = rand() % 0x300 + 0x300;
      OBJ.speed3 = rand() % a + a;
      OBJ.y3 = 0x600;
      OBJ.theta4 = rand() % 0x1000;
      OBJ.span4 = rand() % 0x300 + 0x300;
      OBJ.speed4 = rand() % a + a;
      OBJ.y4 = 0x600;
      OBJ.theta5 = rand() % 0x1000;
      OBJ.span5 = rand() % 0x300 + 0x300;
      OBJ.speed5 = rand() % a + a;
      OBJ.y5 = 0x600;
      OBJ.theta6 = rand() % 0x1000;
      OBJ.span6 = rand() % 0x300 + 0x300;
      OBJ.speed6 = rand() % a + a;
      OBJ.y6 = 0x600;

      sprite = Obj_GetUnused();
      sprite->functionIndex = OBJF_NOOP;
      sprite->d.sprite.animData = sColorsAnimData_800feee0;
      sprite->d.sprite.animInitialized = 0;
      OBJ.sprite = sprite;

      obj->state++;
      break;

   case 1:
      sprite = OBJ.sprite;
      sprite->d.sprite.semiTrans = OBJ.semiTrans;
      sprite->x1.n = obj->x1.n;
      sprite->z1.n = obj->z1.n;
      sprite->y1.n = obj->y1.n;

      sum = 0;
      angle = OBJ.spin;
      OBJ.spin += 0x60;
      sVar2 = OBJ.arcWidth;
      b = OBJ.radius;

      for (i = 0; i < 6; i++) {
         switch (i) {
         case 0:
            a = OBJ.y1;
            c = OBJ.span1 + a;
            angleOfs = OBJ.theta1;
            break;
         case 1:
            a = OBJ.y2;
            c = OBJ.span2 + a;
            angleOfs = OBJ.theta2;
            break;
         case 2:
            a = OBJ.y3;
            c = OBJ.span3 + a;
            angleOfs = OBJ.theta3;
            break;
         case 3:
            a = OBJ.y4;
            c = OBJ.span4 + a;
            angleOfs = OBJ.theta4;
            break;
         case 4:
            a = OBJ.y5;
            c = OBJ.span5 + a;
            angleOfs = OBJ.theta5;
            break;
         case 5:
            a = OBJ.y6;
            c = OBJ.span6 + a;
            angleOfs = OBJ.theta6;
            break;
         }

         if (a >= 0x800) {
            a = 0x800;
         } else if (a <= 0) {
            a = 0;
         }
         if (c >= 0x800) {
            c = 0x800;
         } else if (c <= 0) {
            c = 0;
         }

         sum += c;

         sprite->d.sprite.coords[0].x = obj->x1.n + (b * rcos(angle + angleOfs) >> 12);
         sprite->d.sprite.coords[0].y = obj->y1.n + a;
         sprite->d.sprite.coords[0].z = obj->z1.n + (b * rsin(angle + angleOfs) >> 12);
         sprite->d.sprite.coords[1].x = obj->x1.n + (b * rcos(angle + angleOfs + sVar2) >> 12);
         sprite->d.sprite.coords[1].y = obj->y1.n + a;
         sprite->d.sprite.coords[1].z = obj->z1.n + (b * rsin(angle + angleOfs + sVar2) >> 12);
         sprite->d.sprite.coords[2].x = obj->x1.n + (b * rcos(angle + angleOfs) >> 12);
         sprite->d.sprite.coords[2].y = obj->y1.n + c;
         sprite->d.sprite.coords[2].z = obj->z1.n + (b * rsin(angle + angleOfs) >> 12);
         sprite->d.sprite.coords[3].x = obj->x1.n + (b * rcos(angle + angleOfs + sVar2) >> 12);
         sprite->d.sprite.coords[3].y = obj->y1.n + c;
         sprite->d.sprite.coords[3].z = obj->z1.n + (b * rsin(angle + angleOfs + sVar2) >> 12);

         sprite->x1.n = (sprite->d.sprite.coords[0].x + sprite->d.sprite.coords[3].x) / 2;
         sprite->z1.n = (sprite->d.sprite.coords[0].z + sprite->d.sprite.coords[3].z) / 2;
         sprite->y1.n = sprite->d.sprite.coords[3].y;

         UpdateObjAnimation(sprite);
         sprite->d.sprite.clut = rainbowCluts[rand() % 4];
         AddObjPrim3(gGraphicsPtr->ot, sprite);
      }

      OBJ.y1 -= OBJ.speed1;
      OBJ.y2 -= OBJ.speed2;
      OBJ.y3 -= OBJ.speed3;
      OBJ.y4 -= OBJ.speed4;
      OBJ.y5 -= OBJ.speed5;
      OBJ.y6 -= OBJ.speed6;

      if (sum == 0) {
         obj->functionIndex = OBJF_NULL;
         sprite->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 169
void Objf169_EvilStream_FX1(Object *obj) {
   Object *unitSprite;
   Object *sprite;
   POLY_FT4 *poly;
   s16 a;

   switch (obj->state) {
   case 0:
      unitSprite = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
      obj->y1.n = GetTerrainElevation(obj->z1.s.hi, obj->x1.s.hi) + CV(1.0);
      obj->x1.n = unitSprite->x1.n;
      obj->z1.n = unitSprite->z1.n;
      if (OBJ.clut == CLUT_NULL) {
         OBJ.clut = CLUT_REDS;
      }
      OBJ.fade = 0x80;

      obj->state++;

   // fallthrough
   case 1:
      ApplyMaskEffect(448 << 2, 384, 64, 64, (384 << 2) + obj->state3 % 8 * 32, 0, 0,
                      (obj->state3 * 3) % 64, GFX_MASK_EFFECT_2, 0);

      sprite = Obj_GetUnused();
      sprite->functionIndex = OBJF_NOOP;
      sprite->d.sprite.gfxIdx = GFX_MASK_EFFECT_2;
      sprite->d.sprite.boxIdx = 7;
      sprite->x1.n = obj->x1.n;
      sprite->y1.n = obj->y1.n;
      sprite->z1.n = obj->z1.n;

      a = 0x80 * rsin(OBJ.growPhase) >> 12;
      gQuad_800fe63c[0].vx = -a;
      gQuad_800fe63c[0].vy = -a;
      gQuad_800fe63c[1].vx = a;
      gQuad_800fe63c[1].vy = -a;
      gQuad_800fe63c[2].vx = -a;
      gQuad_800fe63c[2].vy = a;
      gQuad_800fe63c[3].vx = a;
      gQuad_800fe63c[3].vy = a;

      sprite->d.sprite.semiTrans = 2;
      sprite->d.sprite.clut = OBJ.clut;
      AddObjPrim6(gGraphicsPtr->ot, sprite, 0);
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      setRGB0(poly, OBJ.fade, OBJ.fade, OBJ.fade);

      sprite->d.sprite.semiTrans = 3;
      sprite->d.sprite.clut = CLUT_MASK;
      AddObjPrim6(gGraphicsPtr->ot, sprite, 0);
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      setRGB0(poly, OBJ.fade, OBJ.fade, OBJ.fade);

      sprite->functionIndex = OBJF_NULL;

      a = 0x48 * rsin(OBJ.growPhase) >> 12;
      gQuad_800fe63c[0].vx = -a;
      gQuad_800fe63c[0].vy = -a;
      gQuad_800fe63c[1].vx = a;
      gQuad_800fe63c[1].vy = -a;
      gQuad_800fe63c[2].vx = -a;
      gQuad_800fe63c[2].vy = a;
      gQuad_800fe63c[3].vx = a;
      gQuad_800fe63c[3].vy = a;

      ApplyMaskEffect(496 << 2, 384, 64, 64, (384 << 2) + obj->state3 % 8 * 32, 0, 0, 0,
                      GFX_MASK_EFFECT_1, 0);

      sprite = Obj_GetUnused();
      sprite->functionIndex = OBJF_NOOP;
      sprite->d.sprite.gfxIdx = GFX_MASK_EFFECT_1;
      sprite->d.sprite.boxIdx = 7;
      sprite->x1.n = obj->x1.n;
      sprite->y1.n = obj->y1.n;
      sprite->z1.n = obj->z1.n;

      sprite->d.sprite.semiTrans = 2;
      sprite->d.sprite.clut = OBJ.clut;
      AddObjPrim6(gGraphicsPtr->ot, sprite, 0);
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      setRGB0(poly, 0x80 - OBJ.fade, 0x80 - OBJ.fade, 0x80 - OBJ.fade);

      sprite->d.sprite.semiTrans = 3;
      sprite->d.sprite.clut = CLUT_MASK;
      AddObjPrim6(gGraphicsPtr->ot, sprite, 0);
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      setRGB0(poly, 0x80 - OBJ.fade, 0x80 - OBJ.fade, 0x80 - OBJ.fade);

      obj->state3++;
      OBJ.fade -= 4;

      OBJ.growPhase += 0x20;
      if (OBJ.growPhase > 0x400) {
         obj->state++;
         OBJ.fade = 0x80;
      }

      sprite->functionIndex = OBJF_NULL;
      break;

   case 2:
      gQuad_800fe63c[0].vx = -0x48;
      gQuad_800fe63c[0].vy = -0x48;
      gQuad_800fe63c[1].vx = 0x48;
      gQuad_800fe63c[1].vy = -0x48;
      gQuad_800fe63c[2].vx = -0x48;
      gQuad_800fe63c[2].vy = 0x48;
      gQuad_800fe63c[3].vx = 0x48;
      gQuad_800fe63c[3].vy = 0x48;

      ApplyMaskEffect(496 << 2, 384, 64, 64, (384 << 2) + obj->state3 % 8 * 32, 0, 0, 0,
                      GFX_MASK_EFFECT_1, 0);

      sprite = Obj_GetUnused();
      sprite->functionIndex = OBJF_NOOP;
      sprite->d.sprite.gfxIdx = GFX_MASK_EFFECT_1;
      sprite->d.sprite.boxIdx = 7;
      sprite->x1.n = obj->x1.n;
      sprite->y1.n = obj->y1.n;
      sprite->z1.n = obj->z1.n;

      sprite->d.sprite.clut = OBJ.clut;
      AddObjPrim6(gGraphicsPtr->ot, sprite, 0);
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      setRGB0(poly, OBJ.fade, OBJ.fade, OBJ.fade);

      sprite->d.sprite.semiTrans = 3;
      sprite->d.sprite.clut = CLUT_MASK;
      AddObjPrim6(gGraphicsPtr->ot, sprite, 0);
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      setRGB0(poly, OBJ.fade, OBJ.fade, OBJ.fade);

      obj->state3++;

      OBJ.fade -= 0x10;
      if (OBJ.fade == 0) {
         obj->functionIndex = OBJF_NULL;
         gSignal3 = 1;
      }

      sprite->functionIndex = OBJF_NULL;
      break;
   }
}

#undef OBJF
#define OBJF 161
void Objf161_PiercingLight_FX1(Object *obj) {
   s16 cluts[4] = {CLUT_REDS, CLUT_BLUES, CLUT_GREENS, CLUT_PURPLES};
   Object *casterSprite;
   Object *targetSprite;
   Object *homingRay;

   switch (obj->state) {
   case 0:
      OBJ.unitSprite = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
      OBJ.clut = cluts[rand() % 4];
      obj->state++;
      break;

   case 1:
      casterSprite = OBJ.unitSprite;
      homingRay = Obj_GetUnused();
      homingRay->functionIndex = OBJF_HOMING_RAY;
      homingRay->x1.n = casterSprite->x1.n;
      homingRay->y1.n = casterSprite->y1.n + CV(0.5);
      homingRay->z1.n = casterSprite->z1.n;
      targetSprite = GetUnitSpriteAtPosition(gTargetZ, gTargetX);
      homingRay->d.objf171.dx = targetSprite->x1.n - casterSprite->x1.n;
      homingRay->d.objf171.dz = targetSprite->z1.n - casterSprite->z1.n;
      homingRay->d.objf171.dy = (targetSprite->y1.n - casterSprite->y1.n) + CV(0.4375);
      homingRay->d.objf171.clut = OBJ.clut;

      if (++obj->state2 == 17) {
         obj->state++;
      }

      gCameraZoom.vz += 11;
      gCameraRotation.vx += 8;
      break;

   case 2:
      gCameraZoom.vz += 11;
      gCameraRotation.vx += 8;
      if (obj->state2 == 36) {
         gSignal3 = 1;
      } else if (obj->state2 == 43) {
         obj->functionIndex = OBJF_NULL;
      }
      obj->state2++;
      break;
   }
}

#undef OBJF
#define OBJF 170
void Objf170_PiercingRay_Etc_FX1(Object *obj) {
   // FX1 for Plasma Wave and Piercing Ray / Aura Gem
   s16 cluts[5] = {CLUT_REDS, CLUT_BLUES, CLUT_GREENS, CLUT_PURPLES, CLUT_GRAYS};
   Object *casterSprite;
   Object *targetSprite;
   Object *homingRay;
   BVectorXZ *p;
   s16 i;

   switch (obj->state) {
   case 0:
      OBJ.unitSprite = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
      obj->state++;
      break;

   case 1:
      casterSprite = OBJ.unitSprite;
      p = (BVectorXZ *)gTargetCoords;
      for (i = 0; p->x != 0xff && i < 15; i++) {
         homingRay = Obj_GetUnused();
         homingRay->functionIndex = OBJF_HOMING_RAY;
         homingRay->x1.n = casterSprite->x1.n;
         homingRay->y1.n = casterSprite->y1.n + CV(0.5);
         homingRay->z1.n = casterSprite->z1.n;
         targetSprite = GetUnitSpriteAtPosition(p->z, p->x);
         homingRay->d.objf171.dx = targetSprite->x1.n - casterSprite->x1.n;
         homingRay->d.objf171.dz = targetSprite->z1.n - casterSprite->z1.n;
         homingRay->d.objf171.dy = (targetSprite->y1.n - casterSprite->y1.n) + CV(0.4375);
         homingRay->d.objf171.clut = cluts[i % 5];
         p++;
      }

      gCameraZoom.vz += 12;
      gCameraRotation.vx += 8;

      if (Obj_CountUnused() >= 50) {
         if (++obj->state2 == 17) {
            obj->state++;
         }
      } else {
         obj->state++;
      }
      break;

   case 2:
      gCameraZoom.vz += 12;
      gCameraRotation.vx += 8;
      if (++obj->state2 == 39) {
         obj->functionIndex = OBJF_NULL;
         gSignal3 = 1;
      }
      break;
   }
}

#undef OBJF
#define OBJF 171
void Objf171_HomingRay(Object *obj) {
   s16 colorGfx[16] = {GFX_COLOR_6,  GFX_COLOR_7,  GFX_COLOR_8,  GFX_COLOR_9,
                       GFX_COLOR_10, GFX_COLOR_11, GFX_COLOR_12, GFX_COLOR_13,
                       GFX_COLOR_13, GFX_COLOR_12, GFX_COLOR_11, GFX_COLOR_10,
                       GFX_COLOR_9,  GFX_COLOR_8,  GFX_COLOR_7,  GFX_COLOR_6};
   Object *sprite;
   s32 i;
   s16 colorIdx;

   switch (obj->state) {
   case 0:
      if (OBJ.clut == CLUT_NULL) {
         OBJ.clut = CLUT_REDS;
      }
      OBJ.amplitudeX = CV(1.5) - rand() % CV(3.0);
      OBJ.amplitudeY = CV(1.5) + rand() % CV(1.5);
      OBJ.amplitudeZ = CV(1.5) - rand() % CV(3.0);
      obj->state++;
      break;

   case 1:
      if (gQuadIndex < 1200) {
         sprite = Obj_GetUnused();
         sprite->functionIndex = OBJF_NOOP;
         sprite->d.sprite.animInitialized = 0;
         sprite->d.sprite.clut = OBJ.clut;

         colorIdx = 0;
         for (i = OBJ.tailIdx; i < OBJ.headIdx; i++) {
            sprite->d.sprite.coords[0].x = obj->x1.n + (OBJ.amplitudeX * rsin((i + 1) * 0x80) >> 11) +
                                           (i + 1) * OBJ.dx * 0x100 / ONE;
            sprite->d.sprite.coords[0].z = obj->z1.n + (OBJ.amplitudeZ * rsin((i + 1) * 0x80) >> 11) +
                                           (i + 1) * OBJ.dz * 0x100 / ONE;
            sprite->d.sprite.coords[0].y = obj->y1.n + (OBJ.amplitudeY * rsin((i + 1) * 0x80) >> 11) +
                                           (i + 1) * OBJ.dy * 0x100 / ONE;

            sprite->d.sprite.coords[1].x =
                obj->x1.n + (OBJ.amplitudeX * rsin(i * 0x80) >> 11) + (i * OBJ.dx * 0x100 / ONE);
            sprite->d.sprite.coords[1].z =
                obj->z1.n + (OBJ.amplitudeZ * rsin(i * 0x80) >> 11) + (i * OBJ.dz * 0x100 / ONE);
            sprite->d.sprite.coords[1].y =
                obj->y1.n + (OBJ.amplitudeY * rsin(i * 0x80) >> 11) + (i * OBJ.dy * 0x100 / ONE);

            sprite->d.sprite.coords[2].x = sprite->d.sprite.coords[0].x;
            sprite->d.sprite.coords[2].z = sprite->d.sprite.coords[0].z;
            sprite->d.sprite.coords[2].y = sprite->d.sprite.coords[0].y + 0x40;

            sprite->d.sprite.coords[3].x = sprite->d.sprite.coords[1].x;
            sprite->d.sprite.coords[3].z = sprite->d.sprite.coords[1].z;
            sprite->d.sprite.coords[3].y = sprite->d.sprite.coords[1].y + 0x40;

            sprite->x1.n = sprite->d.sprite.coords[0].x;
            sprite->y1.n = sprite->d.sprite.coords[0].y;
            sprite->z1.n = sprite->d.sprite.coords[0].z;

            sprite->d.sprite.gfxIdx = colorGfx[colorIdx++];
            AddObjPrim3(gGraphicsPtr->ot, sprite);
         } // for (i)

         sprite->functionIndex = OBJF_NULL;
      }

      switch (obj->state2) {
      case 0:
         if (++OBJ.headIdx == 16) {
            OBJ.tailIdx = 0;
            obj->state2++;
         }
         break;
      case 1:
         if (++OBJ.tailIdx == 16) {
            obj->functionIndex = OBJF_NULL;
         }
         break;
      }

      break;
   }
}

#undef OBJF
#define OBJF 151
void Objf151_FireGem_FX1(Object *obj) {
   Object *casterSprite;
   Object *targetSprite;
   Object *obj_s0;
   s16 i;
   BVectorXZ *p;

   switch (obj->state) {
   case 0:
      casterSprite = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);

      p = (BVectorXZ *)gTargetCoords;
      for (i = 0; p->x != 0xff && i < 15; i++) {
         targetSprite = GetUnitSpriteAtPosition(p->z, p->x);
         obj_s0 = Obj_GetUnused();
         obj_s0->functionIndex = OBJF_FIRE_GEM_BEAM;
         obj_s0->y1.n = GetTerrainElevation(p->z, p->x);
         obj_s0->x1.n = targetSprite->x1.n;
         obj_s0->z1.n = targetSprite->z1.n;
         p++;
      }

      obj_s0 = Obj_GetUnused();
      obj_s0->functionIndex = OBJF_FOCUS_CAMERA;
      obj_s0->d.objf026.target = targetSprite;

      obj->state++;
      break;

   case 1:
      gCameraZoom.vz += 2;
      gCameraRotation.vx += 2;
      gCameraRotation.vy += 16;
      if (++obj->state2 == 121) {
         obj->functionIndex = OBJF_NULL;
         gSignal3 = 1;
      }
      break;
   }
}

#undef OBJF
#define OBJF 175
void Objf175_RainbowStorm_FX1(Object *obj) {
   Object *obj_s0;
   Object *obj_s1;
   Object *targetSprite; // fixes regswap
   Object **pDataStoreAsObjs;
   BVectorXZ *p;
   s32 i;

   switch (obj->state) {
   case 0:
      obj_s0 = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
      obj->y1.n = GetTerrainElevation(obj->z1.s.hi, obj->x1.s.hi);
      obj->x1.n = obj_s0->x1.n;
      obj->z1.n = obj_s0->z1.n;

      obj_s0 = Obj_GetUnused();
      obj_s0->functionIndex = OBJF_NOOP;
      OBJ.dataStore = obj_s0;

      pDataStoreAsObjs = obj_s0->d.dataStore.objs;
      p = (BVectorXZ *)gTargetCoords;
      for (OBJ.iterator = 0; p->x != 0xff && OBJ.iterator < 15; OBJ.iterator++) {
         targetSprite = GetUnitSpriteAtPosition(p->z, p->x);
         obj_s1 = Obj_GetUnused();
         obj_s1->functionIndex = OBJF_FLASHING_UNIT_SPRITE;
         obj_s1->d.objf149.clut = CLUT_BLUES;
         obj_s1->x1.s.hi = targetSprite->x1.s.hi;
         obj_s1->z1.s.hi = targetSprite->z1.s.hi;
         pDataStoreAsObjs[OBJ.iterator] = obj_s1;
         p++;
      }

      obj->state2 = 0;
      obj->state3 = 0;
      obj->state++;

   // fallthrough
   case 1:
      if (OBJ.theta < 0x400) {
         obj_s1 = Obj_GetUnused();
         obj_s1->functionIndex = OBJF_RAINBOW_SWIRL;
         obj_s1->d.objf167.radius = 0x300 * (ONE - rsin(OBJ.theta)) / ONE;
         obj_s1->d.objf167.arcWidth = 0x60;
         obj_s1->d.objf167.spin = 0x20 + 0x28 * rcos(OBJ.theta) / ONE;
         obj_s1->d.objf167.semiTrans = 1;
         obj_s1->x1.n = obj->x1.n;
         obj_s1->y1.n = obj->y1.n;
         obj_s1->z1.n = obj->z1.n;
      }

      gCameraZoom.vz += 8;
      gCameraRotation.vx -= 2;
      OBJ.theta += 0x40;

      switch (obj->state2) {
      case 0:
         gLightColor.r -= 8;
         gLightColor.g -= 8;
         gLightColor.b -= 8;
         if (gLightColor.r <= 0x20) {
            obj->state2++;
         }
         break;
      case 1:
         if (OBJ.theta > 0xb00) {
            gLightColor.r += 8;
            gLightColor.g += 8;
            gLightColor.b += 8;
         }
         break;
      }

      if (OBJ.theta > 0xe00) {
         obj->functionIndex = OBJF_NULL;
         gSignal3 = 1;
         gLightColor.r = 0x80;
         gLightColor.g = 0x80;
         gLightColor.b = 0x80;

         obj_s0 = OBJ.dataStore;
         pDataStoreAsObjs = obj_s0->d.dataStore.objs;
         for (i = 0; i < OBJ.iterator; i++) {
            obj_s1 = pDataStoreAsObjs[i];
            obj_s1->state = 99;
         }
         obj_s0->functionIndex = OBJF_NULL;
      }

      break;
   }
}

#undef OBJF
#define OBJF 176
void Objf176_RainbowStroke_FX1(Object *obj) {
   Object *obj_s0;
   Object *obj_s1;
   Object *targetSprite; // fixes regswap (again)

   switch (obj->state) {
   case 0:
      obj_s0 = GetUnitSpriteAtPosition(gTargetZ, gTargetX);
      obj->y1.n = GetTerrainElevation(obj_s0->z1.s.hi, obj_s0->x1.s.hi);
      obj->x1.n = obj_s0->x1.n;
      obj->z1.n = obj_s0->z1.n;

      obj_s1 = Obj_GetUnused();
      obj_s1->functionIndex = OBJF_FOCUS_CAMERA;
      obj_s1->d.objf026.target = obj_s0;
      obj_s1->d.objf026.type = 2;
      OBJ.cam = obj_s1;

      targetSprite = GetUnitSpriteAtPosition(gTargetZ, gTargetX);
      obj_s1 = Obj_GetUnused();
      obj_s1->functionIndex = OBJF_FLASHING_UNIT_SPRITE;
      obj_s1->d.objf149.clut = CLUT_BLUES;
      obj_s1->x1.s.hi = targetSprite->x1.s.hi;
      obj_s1->z1.s.hi = targetSprite->z1.s.hi;
      OBJ.fx = obj_s1;

      obj->state++;

   // fallthrough
   case 1:
      if (OBJ.theta < 0x400) {
         obj_s1 = Obj_GetUnused();
         obj_s1->functionIndex = OBJF_RAINBOW_STROKE_RAINBOW_SWIRL;
         obj_s1->d.objf168.radius = 0xe0 * (ONE - rsin(OBJ.theta)) / ONE;
         obj_s1->d.objf168.arcWidth = 0x60;
         obj_s1->d.objf168.spin = 0x20 + 0x10 * rcos(OBJ.theta) / ONE;
         obj_s1->d.objf168.semiTrans = 0;
         obj_s1->x1.n = obj->x1.n;
         obj_s1->y1.n = obj->y1.n;
         obj_s1->z1.n = obj->z1.n;
      }

      gCameraZoom.vz += 6;
      gCameraRotation.vx -= 6;
      gCameraRotation.vy -= 12;

      OBJ.theta += 0x40;

      switch (obj->state2) {
      case 0:
         gLightColor.r -= 8;
         gLightColor.g -= 8;
         gLightColor.b -= 8;
         if (gLightColor.r <= 0x20) {
            obj->state2++;
         }
         break;
      case 1:
         if (OBJ.theta > 0xb00) {
            gLightColor.r += 8;
            gLightColor.g += 8;
            gLightColor.b += 8;
         }
         break;
      }

      if (OBJ.theta == 0x1c0) {
         obj_s0 = OBJ.cam;
         obj_s0->functionIndex = OBJF_NULL;
      }

      if (OBJ.theta > 0xe00) {
         obj->functionIndex = OBJF_NULL;
         gLightColor.r = 0x80;
         gLightColor.g = 0x80;
         gLightColor.b = 0x80;
         gSignal3 = 1;
         obj_s0 = OBJ.fx;
         obj_s0->state = 99;
      }

      break;
   }
}

#undef OBJF
#define OBJF 177
void Objf177_HolyPressure_FX1(Object *obj) {
   Object *obj_s2;
   Object *obj_s1;
   Object *targetSprite;
   Object **pDataStoreAsObjs;
   BVectorXZ *p;
   s32 i;

   switch (obj->state) {
   case 0:
      obj_s2 = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
      obj->x1.n = obj_s2->x1.n;
      obj->y1.n = obj_s2->y1.n;
      obj->z1.n = obj_s2->z1.n;

      p = (BVectorXZ *)gTargetCoords;
      obj_s2 = GetUnitSpriteAtPosition(p->z, p->x);
      obj_s1 = Obj_GetUnused();
      obj_s1->functionIndex = OBJF_FOCUS_CAMERA;
      obj_s1->d.objf026.target = obj_s2;
      obj_s1->d.objf026.type = 2;

      obj_s2 = Obj_GetUnused();
      obj_s2->functionIndex = OBJF_NOOP;
      OBJ.dataStore = obj_s2;
      pDataStoreAsObjs = obj_s2->d.dataStore.objs;

      for (OBJ.iterator = 0; p->x != 0xff && OBJ.iterator < 15; /*OBJ.iterator++*/) {
         targetSprite = GetUnitSpriteAtPosition(p->z, p->x);
         obj_s1 = Obj_GetUnused();
         obj_s1->functionIndex = OBJF_FLASHING_UNIT_SPRITE;
         obj_s1->d.objf149.clut = CLUT_GREENS;
         obj_s1->x1.n = targetSprite->x1.n;
         obj_s1->z1.n = targetSprite->z1.n;
         obj_s1->y1.n = targetSprite->y1.n;
         pDataStoreAsObjs[OBJ.iterator] = obj_s1;
         OBJ.iterator++;

         obj_s1 = Obj_GetUnused();
         obj_s1->functionIndex = OBJF_HOLY_PRESSURE_CUBE;
         obj_s1->x1.s.hi = targetSprite->x1.s.hi;
         obj_s1->z1.s.hi = targetSprite->z1.s.hi;

         p++;
      }

      obj->state++;

   // fallthrough
   case 1:
      gCameraZoom.vz += 2;
      gCameraRotation.vx -= 1;

      if (++OBJ.timer > 150) {
         obj->functionIndex = OBJF_NULL;
         gSignal3 = 1;

         obj_s2 = OBJ.dataStore;
         pDataStoreAsObjs = obj_s2->d.dataStore.objs;
         for (i = 0; i < OBJ.iterator; i++) {
            obj_s1 = pDataStoreAsObjs[i];
            FntPrint("%d\n", obj_s1->functionIndex);
            obj_s1->state = 99;
         }
         obj_s2->functionIndex = OBJF_NULL;
      }

      break;
   }
}

#undef OBJF
#define OBJF 160
void Objf160_IceStorm_Target(Object *obj) {
   Object *targetSprite;
   Object *fxSprite1;
   Object *fxSprite2;
   s32 spriteX, spriteY, spriteW, spriteH;
   POLY_FT4 *poly;

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

      fxSprite1 = Obj_GetUnused();
      CopyObject(targetSprite, fxSprite1);
      fxSprite2 = Obj_GetUnused();
      CopyObject(targetSprite, fxSprite2);
      fxSprite1->d.sprite.hidden = 0;
      fxSprite2->d.sprite.hidden = 0;

      GetUnitSpriteVramRect(targetSprite, &spriteX, &spriteY, &spriteW, &spriteH);
      ApplyMaskEffect(spriteX, spriteY, spriteW + 1, spriteH + 1, 384 << 2, 384, 0,
                      -(obj->state3++ % 64), GFX_MASK_EFFECT_1, 0);

      fxSprite1->d.sprite.gfxIdx = GFX_MASK_EFFECT_1;
      fxSprite1->d.sprite.clut = OBJ.clut;
      fxSprite1->d.sprite.semiTrans = 2;

      if (targetSprite->d.sprite.gfxIdx >= 21 && targetSprite->d.sprite.gfxIdx <= 30) {
         fxSprite1->d.sprite.boxIdx = 1;
      } else {
         fxSprite1->d.sprite.boxIdx = 0;
      }

      AddObjPrim6(gGraphicsPtr->ot, fxSprite1, 0);
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      setRGB0(poly, OBJ.fade, OBJ.fade, OBJ.fade);

      RenderUnitSprite(gGraphicsPtr->ot, fxSprite2, 0);

      fxSprite1->functionIndex = OBJF_NULL;
      fxSprite2->functionIndex = OBJF_NULL;

      switch (obj->state2) {
      case 0:
         OBJ.fade += 4;
         if (OBJ.fade > 0x40) {
            obj->state2++;
            OBJ.fade = 0x40;
         }
         break;
      case 1:
         OBJ.timer++;
         if (OBJ.timer == 88) {
            obj->state2++;
            OBJ.unused_0x2C = 0x40;
         }
         break;
      case 2:
         OBJ.fade -= 8;
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
#define OBJF 162
void Objf162_IceStorm_Splash(Object *obj) {
   static s16 splashAnimData[] = {
       7, GFX_SPLASH_1, 1, GFX_SPLASH_2, 1, GFX_SPLASH_3, 1, GFX_SPLASH_4, 1, GFX_SPLASH_5,
       1, GFX_SPLASH_6, 1, GFX_SPLASH_7, 1, GFX_SPLASH_8, 1, GFX_NULL,     0, GFX_NULL};

   POLY_FT4 *poly;
   Object *sprite;

   switch (obj->state) {
   case 0:
      sprite = Obj_GetUnused();
      sprite->functionIndex = OBJF_NOOP;
      sprite->d.sprite.animData = splashAnimData;
      sprite->d.sprite.semiTrans = 2;
      OBJ.sprite = sprite;

      OBJ.xOfs = 0x80 - rand() % 0x101;
      OBJ.zOfs = 0x80 - rand() % 0x101;
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
         sprite->y1.n = obj->y1.n;

         gQuad_800fe63c[0].vx = -12;
         gQuad_800fe63c[0].vy = -64;
         gQuad_800fe63c[1].vx = 12;
         gQuad_800fe63c[1].vy = -64;
         gQuad_800fe63c[2].vx = -12;
         gQuad_800fe63c[2].vy = 0;
         gQuad_800fe63c[3].vx = 12;
         gQuad_800fe63c[3].vy = 0;

         AddObjPrim6(gGraphicsPtr->ot, sprite, 0);
         poly = &gGraphicsPtr->quads[gQuadIndex - 1];
         setRGB0(poly, OBJ.fade, OBJ.fade, OBJ.fade);
         OBJ.fade -= 10;
      }
      break;
   }
}

#undef OBJF
#define OBJF 189
void Objf189_IceStorm_FX1(Object *obj) {
   Object *targetSprite;
   Object *obj_s1;
   s32 i, j;
   s32 a;
   s16 b, c, d;

   switch (obj->state) {
   case 0:
      targetSprite = GetUnitSpriteAtPosition(gTargetZ, gTargetX);
      obj->y1.n = GetTerrainElevation(gTargetZ, gTargetX);
      obj->x1.n = targetSprite->x1.n;
      obj->z1.n = targetSprite->z1.n;

      obj_s1 = CreatePositionedObj(obj, OBJF_ICE_STORM_TARGET);
      obj_s1->d.objf160.targetSprite = targetSprite;
      obj_s1->d.objf160.clut = CLUT_BLUES;

      obj_s1 = Obj_GetUnused();
      obj_s1->functionIndex = OBJF_ICE_STORM_CAMERA;
      obj_s1->d.objf279.targetSprite = targetSprite;

      OBJ.unused_0x24 = 0x280;
      obj->state++;

   // fallthrough
   case 1:
      obj_s1 = Obj_GetUnused();
      obj_s1->functionIndex = OBJF_NOOP;
      obj_s1->d.sprite.gfxIdx = GFX_TILED_ICE_DYN_1;
      obj_s1->d.sprite.clut = CLUT_BLUES;
      obj_s1->d.sprite.semiTrans = 1;

      OBJ.uvScroll += 2;
      a = OBJ.spin++;

      switch (obj->state2) {
      case 0:
         b = 0x80 + 0x200 * rcos(OBJ.theta1) / ONE;
         c = 0x600 * rsin(OBJ.theta1) / ONE;
         d = 0x40 + 0x3c0 * (ONE - rcos(OBJ.theta1)) / ONE;
         d /= 4;
         break;
      case 1:
         b = 0x80;
         c = 0x400 * (ONE - rsin(OBJ.theta2)) / ONE;
         d = 0x100;
         break;
      }

      for (i = 0; i < 4; i++) {
         for (j = 0; j < 4; j++) {
            gGfxSubTextures[GFX_TILED_ICE_DYN_1][0] =
                0x80 + ((i * 4 + j) * 2 + OBJ.uvScroll) % 0x20;
            gGfxSubTextures[GFX_TILED_ICE_DYN_1][1] =
                0x40 + ((i * 4 + j) * 2 + OBJ.uvScroll) % 0x20;
            gGfxSubTextures[GFX_TILED_ICE_DYN_1][2] = 2;
            gGfxSubTextures[GFX_TILED_ICE_DYN_1][3] = 31;

            obj_s1->d.sprite.coords[0].x = obj->x1.n + b * rcos(a * 0x40 + j * d) / ONE;
            obj_s1->d.sprite.coords[0].z = obj->z1.n + b * rsin(a * 0x40 + j * d) / ONE;
            obj_s1->d.sprite.coords[0].y = obj->y1.n + c / 2;

            obj_s1->d.sprite.coords[1].x = obj->x1.n + b * rcos(a * 0x40 + (j + 1) * d) / ONE;
            obj_s1->d.sprite.coords[1].z = obj->z1.n + b * rsin(a * 0x40 + (j + 1) * d) / ONE;
            obj_s1->d.sprite.coords[1].y = obj->y1.n + c / 2;

            obj_s1->d.sprite.coords[2].x = obj->x1.n + b * rcos(a * 0x40 + j * d) / ONE;
            obj_s1->d.sprite.coords[2].z = obj->z1.n + b * rsin(a * 0x40 + j * d) / ONE;
            obj_s1->d.sprite.coords[2].y = obj->y1.n;

            obj_s1->d.sprite.coords[3].x = obj->x1.n + b * rcos(a * 0x40 + (j + 1) * d) / ONE;
            obj_s1->d.sprite.coords[3].z = obj->z1.n + b * rsin(a * 0x40 + (j + 1) * d) / ONE;
            obj_s1->d.sprite.coords[3].y = obj->y1.n;

            AddObjPrim4(gGraphicsPtr->ot, obj_s1);

            obj_s1->d.sprite.coords[0].y = obj->y1.n + c;
            obj_s1->d.sprite.coords[1].y = obj->y1.n + c;
            obj_s1->d.sprite.coords[2].y = obj->y1.n + c / 2;
            obj_s1->d.sprite.coords[3].y = obj->y1.n + c / 2;
            obj_s1->y1.n = (obj_s1->d.sprite.coords[0].y + obj_s1->d.sprite.coords[1].y +
                            obj_s1->d.sprite.coords[2].y + obj_s1->d.sprite.coords[3].y) /
                           4;

            AddObjPrim3(gGraphicsPtr->ot, obj_s1);
         }
         a += 0x10;
      }

      obj_s1->functionIndex = OBJF_NULL;

      switch (obj->state2) {
      case 0:
         OBJ.theta1 += 0x10;
         if (OBJ.theta1 >= 0x400) {
            obj->state2++;
         }
         break;
      case 1:
         CreatePositionedObj(obj, OBJF_ICE_STORM_SPLASH);
         CreatePositionedObj(obj, OBJF_ICE_STORM_SPLASH);
         OBJ.theta2 += 0x10;
         if (OBJ.theta2 >= 0x400) {
            gSignal3 = 1;
            obj->functionIndex = OBJF_NULL;
         }
         break;
      }

      break;
   }
}

#undef OBJF
#define OBJF 155
void Objf155_Spellbind_Glyph(Object *obj) {
   POLY_FT4 *poly;
   s16 red, green, blue;
   s16 radius, theta, x_1, z_1, x_2, z_2, y;

   switch (obj->state) {
   case 0:
      OBJ.idx = OBJ.initialIdx;
      OBJ.theta = OBJ.idx * 0x100;
      OBJ.layerY = OBJ.idx / 16 * 0x40;
      OBJ.yOfs = 0x400;
      OBJ.ySpeed = 0x100;
      OBJ.gfxIdx = rand() % 12 + 0x89;
      OBJ.semiTrans = 1;
      obj->x2.n = obj->x1.n;
      obj->z2.n = obj->z1.n;
      obj->y2.n = obj->y1.n;
      OBJ.initialIdx = 0;
      obj->state++;

   // fallthrough
   case 1:

      switch (obj->state3) {
      case 0:
         radius = 0xa0;
         break;
      case 1:
         radius = 0xa0 + rsin(OBJ.wobblePhase) / 0x40;
         OBJ.wobblePhase += 0xc0;
         break;
      case 2:
         radius = 0xa0 + rsin(OBJ.wobblePhase) / 0x40;
         if (OBJ.wobblePhase & 0x800U) {
            OBJ.wobblePhase += 0x40;
         }
         break;
      }

      theta = OBJ.theta;
      x_1 = radius * rcos(theta) >> 12;
      z_1 = radius * rsin(theta) >> 12;
      x_2 = radius * rcos(theta + 0x100) >> 12;
      z_2 = radius * rsin(theta + 0x100) >> 12;
      OBJ.coords[0].x = OBJ.coords[2].x = obj->x2.n + x_1;
      OBJ.coords[1].x = OBJ.coords[3].x = obj->x2.n + x_2;
      OBJ.coords[0].z = OBJ.coords[2].z = obj->z2.n + z_1;
      OBJ.coords[1].z = OBJ.coords[3].z = obj->z2.n + z_2;

      y = obj->y2.n + OBJ.layerY + OBJ.yOfs;
      OBJ.coords[0].y = OBJ.coords[1].y = y + 0x40;
      OBJ.coords[2].y = OBJ.coords[3].y = y - 8;

      AddObjPrim4(gGraphicsPtr->ot, obj);

      switch (obj->state3) {
      case 0:
         OBJ.yOfs -= OBJ.ySpeed;
         OBJ.ySpeed += (-OBJ.ySpeed >> 2);
         if (OBJ.ySpeed <= 1) {
            obj->state3++;
         }
         break;

      case 1:
         red = abs(0x80 * rcos(obj->state2) / ONE);
         green = abs(0x80 * rcos(obj->state2 * 2) / ONE);
         blue = abs(0x80 * rcos(obj->state2 / 2) / ONE);
         obj->state2 += 0x80;
         poly = &gGraphicsPtr->quads[gQuadIndex - 1];
         setRGB0(poly, red, green, blue);

         if (obj->z3.n++ == (0x60 - OBJ.idx) * 2 + 0x1e) {
            obj->functionIndex = OBJF_NULL;
            if (OBJ.idx == 10) {
               gSignal3 = 1;
            }
         }
         break;

      case 2:
         OBJ.yOfs += OBJ.ySpeed;
         OBJ.ySpeed += (0x100 - OBJ.ySpeed) >> 2;
         if (OBJ.yOfs >= 0x400) {
            obj->functionIndex = OBJF_NULL;
            if (OBJ.idx == 10) {
               gSignal3 = 1;
            }
         }
         break;
      }

      break;
   }
}

#undef OBJF
#define OBJF 181
void Objf181_Spellbind_FX1(Object *obj) {
   Object *targetSprite;
   Object *obj_v1;

   switch (obj->state) {
   case 0:
      targetSprite = GetUnitSpriteAtPosition(gTargetZ, gTargetX);
      obj->y1.n = GetTerrainElevation(gTargetZ, gTargetX);
      obj->x1.n = targetSprite->x1.n;
      obj->z1.n = targetSprite->z1.n;

      obj_v1 = Obj_GetUnused();
      obj_v1->functionIndex = OBJF_FOCUS_CAMERA;
      obj_v1->d.objf026.target = targetSprite;

      obj->state++;

   // fallthrough
   case 1:
      if (obj->state3 == 0x60) {
         obj->functionIndex = OBJF_NULL;
      } else {
         obj_v1 = Obj_GetUnused();
         obj_v1->functionIndex = OBJF_SPELLBIND_GLYPH;
         obj_v1->y1.n = obj->y1.n;
         obj_v1->x1.n = obj->x1.n;
         obj_v1->z1.n = obj->z1.n;
         obj_v1->d.objf155.initialIdx = obj->state3;
         obj->state3++;
      }
      break;
   }
}

#undef OBJF
#define OBJF 179
void Objf179_ThunderFlash_Ray(Object *obj) {
   Object *obj_s2;
   s16 radius;

   switch (obj->state) {
   case 0:
      obj_s2 = OBJ.parent;
      obj->x1.n = obj_s2->x1.n;
      obj->z1.n = obj_s2->z1.n;
      obj->y1.n = obj_s2->y1.n;

      OBJ.maxRadius = rand() % 0x400 + 0x400;
      OBJ.thetaX = rand() % 0x1000;
      OBJ.thetaZ = rand() % 0x1000;
      OBJ.thetaY = rand() % 0x1000;
      OBJ.thetaXSpeed = 0x40 - rand() % 0x80;
      OBJ.thetaZSpeed = 0x40 - rand() % 0x80;
      OBJ.thetaYSpeed = 0x40 - rand() % 0x80;

      obj->state++;

   // fallthrough
   case 1:
      obj_s2 = Obj_GetUnused();
      obj_s2->functionIndex = OBJF_NOOP;
      obj_s2->d.sprite.gfxIdx = GFX_COLOR_11;
      obj_s2->d.sprite.clut = CLUT_GREENS;
      obj_s2->d.sprite.semiTrans = 1;

      radius = OBJ.maxRadius * OBJ.growth / 30;
      obj_s2->d.sprite.coords[0].x = obj->x1.n;
      obj_s2->d.sprite.coords[0].z = obj->z1.n;
      obj_s2->d.sprite.coords[0].y = obj->y1.n;
      obj_s2->d.sprite.coords[1].x = obj->x1.n;
      obj_s2->d.sprite.coords[1].z = obj->z1.n;
      obj_s2->d.sprite.coords[1].y = obj->y1.n;
      obj_s2->d.sprite.coords[2].x = obj->x1.n + radius * rcos(OBJ.thetaX) / ONE;
      obj_s2->d.sprite.coords[2].z = obj->z1.n + radius * rsin(OBJ.thetaZ) / ONE;
      obj_s2->d.sprite.coords[2].y = obj->y1.n + radius * rsin(OBJ.thetaY) / ONE;
      obj_s2->d.sprite.coords[3].x = obj->x1.n + radius * rcos(OBJ.thetaX + 0x30) / ONE;
      obj_s2->d.sprite.coords[3].z = obj->z1.n + radius * rsin(OBJ.thetaZ + 0x30) / ONE;
      obj_s2->d.sprite.coords[3].y = obj->y1.n + radius * rsin(OBJ.thetaY + 0x30) / ONE;

      AddObjPrim4(gGraphicsPtr->ot, obj_s2);
      obj_s2->functionIndex = OBJF_NULL;

      OBJ.thetaX += OBJ.thetaXSpeed;
      OBJ.thetaZ += OBJ.thetaZSpeed;
      OBJ.thetaY += OBJ.thetaYSpeed;

      switch (obj->state2) {
      case 0:
         OBJ.timer++;
         OBJ.growth++;
         if (OBJ.growth == 32) {
            obj->state2++;
         }
         break;
      case 1:
         OBJ.timer++;
         if (OBJ.timer == 98) {
            obj->state2++;
            OBJ.timer = 0;
         }
         break;
      case 2:
         OBJ.growth -= 2;
         if (OBJ.growth == 0) {
            obj->functionIndex = OBJF_NULL;
         }
         break;
      }

      break;
   }
}

#undef OBJF
#define OBJF 178
void Objf178_ThunderFlash_FX1(Object *obj) {
   Object *unitSprite;
   Object *obj_s1;
   s32 i;
   s16 a, b;
   s16 h_1, h_2;
   POLY_FT4 *poly;

   switch (obj->state) {
   case 0:
      unitSprite = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
      obj->y1.n = GetTerrainElevation(obj->z1.s.hi, obj->x1.s.hi);
      obj->x1.n = unitSprite->x1.n;
      obj->z1.n = unitSprite->z1.n;
      OBJ.maxRadius = 0x380;
      obj->state++;

   // fallthrough
   case 1:
      for (i = 0; i < 16; i++) {
         obj_s1 = Obj_GetUnused();
         obj_s1->functionIndex = OBJF_THUNDER_FLASH_RAY;
         obj_s1->d.objf179.parent = obj;
      }
      OBJ.unused_0x5E = 0;
      obj->state++;

   // fallthrough
   case 2:
      if (gLightColor.r > 32) {
         gLightColor.r -= 4;
         gLightColor.g -= 4;
         gLightColor.b -= 4;
      }
      OBJ.timer++;
      if (OBJ.timer == 106) {
         obj->state++;
      }
      break;

   case 3:
      obj_s1 = Obj_GetUnused();
      obj_s1->functionIndex = OBJF_NOOP;
      obj_s1->d.sprite.gfxIdx = GFX_COLOR_13;
      obj_s1->d.sprite.clut = CLUT_BLUES;
      obj_s1->d.sprite.semiTrans = 1;

      a = b = OBJ.maxRadius * (ONE - rcos(OBJ.theta)) / ONE;
      h_2 = 0x800;
      h_1 = 0;

      for (i = 0; i < 16; i++) {
         obj_s1->d.sprite.coords[0].x = obj->x1.n + a * rcos(i * 0x100) / ONE;
         obj_s1->d.sprite.coords[0].z = obj->z1.n + a * rsin(i * 0x100) / ONE;
         obj_s1->d.sprite.coords[0].y = obj->y1.n + h_1;
         obj_s1->d.sprite.coords[1].x = obj->x1.n + a * rcos((i + 1) * 0x100) / ONE;
         obj_s1->d.sprite.coords[1].z = obj->z1.n + a * rsin((i + 1) * 0x100) / ONE;
         obj_s1->d.sprite.coords[1].y = obj->y1.n + h_1;
         obj_s1->d.sprite.coords[2].x = obj->x1.n + b * rcos(i * 0x100) / ONE;
         obj_s1->d.sprite.coords[2].z = obj->z1.n + b * rsin(i * 0x100) / ONE;
         obj_s1->d.sprite.coords[2].y = obj->y1.n + h_2;
         obj_s1->d.sprite.coords[3].x = obj->x1.n + b * rcos((i + 1) * 0x100) / ONE;
         obj_s1->d.sprite.coords[3].z = obj->z1.n + b * rsin((i + 1) * 0x100) / ONE;
         obj_s1->d.sprite.coords[3].y = obj->y1.n + h_2;
         AddObjPrim4(gGraphicsPtr->ot, obj_s1);
      }

      poly = &gGraphicsPtr->quads[gQuadIndex];
#ifdef PC_PORT
      /* bugreport-02: type the textured quad explicitly. The fx code omits SetPolyFT4 and relies on a
       * stale POLY_FT4 slot code (0x2c on PSX); our backend's slot is stale-POLY_F4 -> texture dropped. */
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

      OBJ.fade += 8;
      OBJ.theta += 0x40;
      if (OBJ.theta == 0x400) {
         gSignal3 = 1;
         obj->state++;
      }

      obj_s1->functionIndex = OBJF_NULL;
      break;

   case 4:
      poly = &gGraphicsPtr->quads[gQuadIndex];
#ifdef PC_PORT
      /* bugreport-02: type the textured quad explicitly. The fx code omits SetPolyFT4 and relies on a
       * stale POLY_FT4 slot code (0x2c on PSX); our backend's slot is stale-POLY_F4 -> texture dropped. */
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

      OBJ.fade -= 16;
      gLightColor.r += 12;
      gLightColor.g += 12;
      gLightColor.b += 12;
      if (OBJ.fade < 0) {
         obj->functionIndex = OBJF_NULL;
         gLightColor.r = 0x80;
         gLightColor.g = 0x80;
         gLightColor.b = 0x80;
      }
      break;
   }
}

#undef OBJF
#define OBJF 180
void Objf180_SpreadForce_FX1(Object *obj) {
   Object *obj_s0;
   Object *targetSprite;
   Object *obj_s1;
   s32 i, j;
   Object **pDataStoreAsObjs;
   BVectorXZ *p;
   POLY_FT4 *poly;
   s16 a, b, c;
   s16 h_1, h_2;
   s16 unaff_s0;

   switch (obj->state) {
   case 0:
      obj_s0 = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
      obj->y1.n = GetTerrainElevation(obj->z1.s.hi, obj->x1.s.hi);
      obj->x1.n = obj_s0->x1.n;
      obj->z1.n = obj_s0->z1.n;
      OBJ.maxRadius = 0x280;
      obj->state++;

   // fallthrough
   case 1:
      OBJ.timer++;
      if (OBJ.timer % 2 == 0) {
         i = OBJ.timer / 2;
         obj_s1 = CreatePositionedObj(obj, OBJF_RAINBOW_SWIRL);
         obj_s1->d.objf167.radius = i * 0x14 + 0x30;
         obj_s1->d.objf167.arcWidth = 0x40;
         obj_s1->d.objf167.spin = i + 0x20;
         obj_s1->d.objf167.semiTrans = 1;
      }
      if (OBJ.timer < 17) {
         gLightColor.r -= 6;
         gLightColor.g -= 6;
         gLightColor.b -= 6;
      }
      if (OBJ.timer == 60) {
         obj->state++;
         obj_s0 = Obj_GetUnused();
         obj_s0->functionIndex = OBJF_NOOP;
         OBJ.dataStore = obj_s0;
         pDataStoreAsObjs = obj_s0->d.dataStore.objs;
         p = (BVectorXZ *)gTargetCoords;
         for (OBJ.iterator = 0; p->x != 0xff && OBJ.iterator < 15; OBJ.iterator++) {
            targetSprite = GetUnitSpriteAtPosition(p->z, p->x);
            obj_s1 = Obj_GetUnused();
            obj_s1->functionIndex = OBJF_FLASHING_UNIT_SPRITE;
            obj_s1->d.objf149.clut = CLUT_BLUES;
            obj_s1->x1.s.hi = targetSprite->x1.s.hi;
            obj_s1->z1.s.hi = targetSprite->z1.s.hi;
            pDataStoreAsObjs[OBJ.iterator] = obj_s1;
            p++;
         }
      }
      break;

   case 2:
      obj_s1 = Obj_GetUnused();
      obj_s1->functionIndex = OBJF_NOOP;
      obj_s1->d.sprite.gfxIdx = GFX_COLOR_13;
      obj_s1->d.sprite.clut = CLUT_BLUES;
      obj_s1->d.sprite.semiTrans = 1;
      a = OBJ.maxRadius * OBJ.growth / 0x80;

      switch (obj->state2) {
      case 0:
         unaff_s0 = 0xcd;
         break;
      case 1:
         unaff_s0 = OBJ.arcStep;
         break;
      }

      for (i = 0; i < 5; i++) {
         b = a * rcos(i * unaff_s0) / ONE;
         c = a * rcos((i + 1) * unaff_s0) / ONE;
         h_2 = a * rsin(i * unaff_s0) / ONE;
         h_1 = a * rsin((i + 1) * unaff_s0) / ONE;
         for (j = 0; j < 16; j++) {
            obj_s1->d.sprite.coords[0].x = obj->x1.n + c * rcos(j * 0x100) / ONE;
            obj_s1->d.sprite.coords[0].z = obj->z1.n + c * rsin(j * 0x100) / ONE;
            obj_s1->d.sprite.coords[0].y = obj->y1.n + h_1;
            obj_s1->d.sprite.coords[1].x = obj->x1.n + c * rcos((j + 1) * 0x100) / ONE;
            obj_s1->d.sprite.coords[1].z = obj->z1.n + c * rsin((j + 1) * 0x100) / ONE;
            obj_s1->d.sprite.coords[1].y = obj->y1.n + h_1;
            obj_s1->d.sprite.coords[2].x = obj->x1.n + b * rcos(j * 0x100) / ONE;
            obj_s1->d.sprite.coords[2].z = obj->z1.n + b * rsin(j * 0x100) / ONE;
            obj_s1->d.sprite.coords[2].y = obj->y1.n + h_2;
            obj_s1->d.sprite.coords[3].x = obj->x1.n + b * rcos((j + 1) * 0x100) / ONE;
            obj_s1->d.sprite.coords[3].z = obj->z1.n + b * rsin((j + 1) * 0x100) / ONE;
            obj_s1->d.sprite.coords[3].y = obj->y1.n + h_2;
            AddObjPrim4(gGraphicsPtr->ot, obj_s1);
         }
      }

      obj_s1->functionIndex = OBJF_NULL;

      switch (obj->state2) {
      case 0:
         OBJ.growth += 4;
         if (OBJ.growth == 0x80) {
            OBJ.arcStep = 0xcd;
            obj->state2++;
         }
         break;

      case 1:
         if (OBJ.arcStep <= 0x60) {
            gLightColor.r += 6;
            gLightColor.g += 6;
            gLightColor.b += 6;

            a = OBJ.fade;
            if (a > 0x80) {
               a = 0x80;
            }

            poly = &gGraphicsPtr->quads[gQuadIndex];
      #ifdef PC_PORT
      /* bugreport-02: type the textured quad explicitly. The fx code omits SetPolyFT4 and relies on a
       * stale POLY_FT4 slot code (0x2c on PSX); our backend's slot is stale-POLY_F4 -> texture dropped. */
      SetPolyFT4(poly);
#endif
      SetSemiTrans(poly, 1);
            poly->tpage = gTPageIds[32 + gGfxTPageCells[GFX_COLOR_15]];
            poly->clut = gGfxClutIds[GFX_COLOR_15];
            setUVWH(poly, gGfxSubTextures[GFX_COLOR_15][0], gGfxSubTextures[GFX_COLOR_15][1],
                    gGfxSubTextures[GFX_COLOR_15][2], gGfxSubTextures[GFX_COLOR_15][3]);
            setXYWH(poly, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
            setRGB0(poly, a, a, a);
            AddPrim(&gGraphicsPtr->ot[OT_SIZE - 1], poly);
            gQuadIndex++;

            OBJ.fade += 8;
         }

         OBJ.arcStep -= 6;
         if (OBJ.arcStep <= 0) {
            OBJ.arcStep = 0x80;
            gSignal3 = 1;

            obj_s0 = OBJ.dataStore;
            pDataStoreAsObjs = obj_s0->d.dataStore.objs;
            for (i = 0; i < OBJ.iterator; i++) {
               obj_s1 = pDataStoreAsObjs[i];
               obj_s1->state = 99;
            }
            obj_s0->functionIndex = OBJF_NULL;

            OBJ.fade = 0x80;
            obj->state++;
         }
         break;
      }

      break;

   case 3:
      poly = &gGraphicsPtr->quads[gQuadIndex];
#ifdef PC_PORT
      /* bugreport-02: type the textured quad explicitly. The fx code omits SetPolyFT4 and relies on a
       * stale POLY_FT4 slot code (0x2c on PSX); our backend's slot is stale-POLY_F4 -> texture dropped. */
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

      OBJ.fade -= 16;
      if (OBJ.fade <= 0) {
         obj->functionIndex = OBJF_NULL;
         gLightColor.r = 0x80;
         gLightColor.g = 0x80;
         gLightColor.b = 0x80;
      }
      break;
   }
}

#undef OBJF
#define OBJF 182
void Objf182_SpikedBallToss_Unused(Object *obj) {
   /* Toss spiked metal balls onto the target while the camera zoom oscillates.
    * CONFIRMED UNUSED: no spell's gSpellsEx triple and no code path references 182. */
   Object *obj_s0;
   Object *unitSprite;
   Object *targetSprite;

   switch (obj->state) {
   case 0:
      unitSprite = GetUnitSpriteAtPosition(gTargetZ, gTargetX);
      obj_s0 = Obj_GetUnused();
      obj_s0->functionIndex = OBJF_FOCUS_CAMERA;
      obj_s0->d.objf026.target = unitSprite;
      obj_s0->d.objf026.type = 2;
      OBJ.cam = obj_s0;
      OBJ.savedZoom = gCameraZoom.vz;
      obj->state++;

   // fallthrough
   case 1:
      unitSprite = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
      if (obj->state3 % 8 == 0) {
         obj_s0 = Obj_GetUnused();
         obj_s0->functionIndex = OBJF_SPIKED_BALL_UNUSED;
         obj_s0->x1.n = unitSprite->x1.n;
         obj_s0->y1.n = unitSprite->y1.n + CV(0.5);
         obj_s0->z1.n = unitSprite->z1.n;
         targetSprite = GetUnitSpriteAtPosition(gTargetZ, gTargetX);
         obj_s0->d.objf183.dx = targetSprite->x1.n - unitSprite->x1.n;
         obj_s0->d.objf183.dz = targetSprite->z1.n - unitSprite->z1.n;
         obj_s0->d.objf183.dy = targetSprite->y1.n - unitSprite->y1.n;
      }
      gCameraZoom.vz = OBJ.savedZoom + 0x100 * rsin(OBJ.theta) / ONE;
      OBJ.theta += 0x20;

      obj_s0 = OBJ.cam;
      if (obj_s0->functionIndex == OBJF_FOCUS_CAMERA) {
         obj_s0->d.objf026.zoom = gCameraZoom.vz;
      }

      obj->state3++;
      if (obj->state3 == 41) {
         obj->state++;
      }
      break;

   case 2:
      if (OBJ.theta <= DEG(180)) {
         gCameraZoom.vz = OBJ.savedZoom + 0x100 * rsin(OBJ.theta) / ONE;
         OBJ.theta += 0x20;
      }

      obj_s0 = OBJ.cam;
      if (obj_s0->functionIndex == OBJF_FOCUS_CAMERA) {
         obj_s0->d.objf026.zoom = gCameraZoom.vz;
      }

      obj->state2++;
      if (obj->state2 == 61) {
         obj->functionIndex = OBJF_NULL;
         gSignal3 = 1;
      }
      break;
   }
}

#undef OBJF
#define OBJF 183
void Objf183_SpikedBall_Unused(Object *obj) {
   static s16 spikedBallAnimData[] = {4, GFX_SPIKED_BALL_1, 2, GFX_SPIKED_BALL_2,
                                      2, GFX_NULL,          1, GFX_NULL};

   Object *obj_s0;
   s32 timer;

   switch (obj->state) {
   case 0:
      OBJ.wobbleX = 0x100 - rand() % 0x200;
      OBJ.wobbleY = 0x180 + rand() % 0x180;
      OBJ.wobbleZ = 0x100 - rand() % 0x200;

      obj_s0 = Obj_GetUnused();
      OBJ.sprite = obj_s0;
      obj_s0->functionIndex = OBJF_NOOP;
      obj_s0->d.sprite.animData = spikedBallAnimData;
      obj_s0->d.sprite.animInitialized = 0;
      obj_s0->d.sprite.boxIdx = 4;

      obj->state++;

   // fallthrough
   case 1:
      obj_s0 = OBJ.sprite;
      timer = OBJ.timer;
      obj_s0->x1.n =
          obj->x1.n + (OBJ.wobbleX * rsin(timer * 0x20) >> 11) + (timer * OBJ.dx / 0x40);
      obj_s0->z1.n =
          obj->z1.n + (OBJ.wobbleZ * rsin(timer * 0x20) >> 11) + (timer * OBJ.dz / 0x40);
      obj_s0->y1.n =
          obj->y1.n + (OBJ.wobbleY * rsin(timer * 0x20) >> 11) + (timer * OBJ.dy / 0x40);
      UpdateObjAnimation(obj_s0);
      AddObjPrim6(gGraphicsPtr->ot, obj_s0, 0);

      OBJ.timer++;
      if (OBJ.timer == 60) {
         obj_s0 = Obj_GetUnused();
         obj_s0->functionIndex = OBJF_RADIAL_FX_SPRITE;
         obj_s0->d.objf119.type = 0;
         obj_s0->d.objf119.boxIdx = 3;
         obj_s0->d.objf119.clut = CLUT_BLUES;
         obj_s0->d.objf119.semiTrans = 0;
         obj_s0->d.objf119.once = 1;
         obj_s0->d.objf119.radius = 0;
         obj_s0->x1.n = obj->x1.n + OBJ.dx;
         obj_s0->z1.n = obj->z1.n + OBJ.dz;
         obj_s0->y1.n = obj->y1.n + OBJ.dy + CV(0.8125);
      } else if (OBJ.timer == 65) {
         obj_s0->functionIndex = OBJF_NULL;
         obj->functionIndex = OBJF_NULL;
         obj_s0 = Obj_GetUnused();
         obj_s0->functionIndex = OBJF_BOUNCE_ZOOM;
         obj_s0->d.objf024.soft = 0;
      }
      break;
   }
}

#undef OBJF
#define OBJF 184
void Objf184_Avalanche_FX1(Object *obj) {
   Object *obj_a0;
   Object *boulder;
   s16 dy;

   switch (obj->state) {
   case 0:
      obj_a0 = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
      obj->x1.n = obj_a0->x1.n;
      obj->y1.n = obj_a0->y1.n;
      obj->z1.n = obj_a0->z1.n;

      boulder = Obj_GetUnused();
      boulder->functionIndex = OBJF_AVALANCHE_BOULDER;
      boulder->x1.n = obj->x1.n;
      boulder->z1.n = obj->z1.n;
      boulder->y1.n = obj->y1.n + CV(3.0);
      boulder->d.objf211.todo_x24 = 0x180;
      boulder->d.objf211.scale = 0;
      boulder->d.objf211.rotY = 0;
      boulder->d.objf211.rotZ = 0;
      boulder->d.objf211.rotX = 0;
      OBJ.boulder = boulder;

      obj->state++;

   // fallthrough
   case 1:
      obj_a0 = Obj_GetUnused();
      obj_a0->functionIndex = OBJF_AVALANCHE_ROCK;
      obj_a0->d.objf185.parent = obj;
      OBJ.rocks[5] = obj_a0;

      obj_a0 = Obj_GetUnused();
      obj_a0->functionIndex = OBJF_AVALANCHE_ROCK;
      obj_a0->d.objf185.parent = obj;
      OBJ.rocks[4] = obj_a0;

      obj_a0 = Obj_GetUnused();
      obj_a0->functionIndex = OBJF_AVALANCHE_ROCK;
      obj_a0->d.objf185.parent = obj;
      OBJ.rocks[3] = obj_a0;

      obj_a0 = Obj_GetUnused();
      obj_a0->functionIndex = OBJF_AVALANCHE_ROCK;
      obj_a0->d.objf185.parent = obj;
      OBJ.rocks[2] = obj_a0;

      obj_a0 = Obj_GetUnused();
      obj_a0->functionIndex = OBJF_AVALANCHE_ROCK;
      obj_a0->d.objf185.parent = obj;
      OBJ.rocks[1] = obj_a0;

      obj_a0 = Obj_GetUnused();
      obj_a0->functionIndex = OBJF_AVALANCHE_ROCK;
      obj_a0->d.objf185.parent = obj;
      OBJ.rocks[0] = obj_a0;

      obj->state++;

   // fallthrough
   case 2:
      gCameraZoom.vz += 3;
      gCameraRotation.vy += 0x10;

      boulder = OBJ.boulder;
      boulder->d.objf211.scale = OBJ.timer * 3;
      boulder->d.objf211.rotY += 12;
      boulder->d.objf211.rotZ -= 7;
      boulder->d.objf211.rotX += 3;
      boulder->x1.n = obj->x1.n;
      boulder->z1.n = obj->z1.n;
      boulder->y1.n = obj->y1.n + CV(3.0);

      OBJ.timer++;
      if (OBJ.timer == 80) {
         obj_a0 = Obj_GetUnused();
         obj_a0->functionIndex = OBJF_FOCUS_CAMERA;
         obj_a0->d.objf026.target = boulder;
         obj_a0->d.objf026.type = 3;
         OBJ.cam = obj_a0;
      }
      if (OBJ.timer >= 90) {
         obj_a0 = OBJ.rocks[5];
         obj_a0->state = 99;
         obj_a0 = OBJ.rocks[4];
         obj_a0->state = 99;
         obj_a0 = OBJ.rocks[3];
         obj_a0->state = 99;
         obj_a0 = OBJ.rocks[2];
         obj_a0->state = 99;
         obj_a0 = OBJ.rocks[1];
         obj_a0->state = 99;
         obj_a0 = OBJ.rocks[0];
         obj_a0->state = 99;

         obj->state++;
         obj_a0 = OBJ.cam;
         if (obj_a0->functionIndex == OBJF_FOCUS_CAMERA) {
            obj_a0->functionIndex = OBJF_NULL;
         }
      }

      break;

   case 3:
      boulder = OBJ.boulder;
      boulder->d.objf211.scale = 0x10e - OBJ.rise / 3;
      boulder->d.objf211.rotY += 12;
      boulder->d.objf211.rotZ -= 7;
      boulder->d.objf211.rotX += 3;
      boulder->x1.n = obj->x1.n;
      boulder->z1.n = obj->z1.n;
      boulder->y1.n = obj->y1.n + CV(3.0) + OBJ.rise * 0x20;

      gCameraRotation.vx += (DEG(90) - gCameraRotation.vx) >> 5;
      gCameraRotation.vy += DEG(1.40625);
      gCameraPos.vx = -(boulder->x1.n >> 3);
      gCameraPos.vz = -(boulder->z1.n >> 3);
      gCameraPos.vy = boulder->y1.n >> 3;

      OBJ.rise += 2;
      if (OBJ.rise == 180) {
         OBJ.fallTimer = 45;
         obj_a0 = GetUnitSpriteAtPosition(gTargetZ, gTargetX);
         OBJ.dx = obj_a0->x1.n - obj->x1.n;
         OBJ.dz = obj_a0->z1.n - obj->z1.n;
         dy = boulder->y1.n - obj_a0->y1.n;
         OBJ.dropHeight = dy;
         OBJ.dropSpan = dy;
         obj->x1.n = obj_a0->x1.n;
         obj->z1.n = obj_a0->z1.n;
         obj->state++;
      }
      break;

   case 4:
      boulder = OBJ.boulder;
      boulder->d.objf211.scale = 0x10e - OBJ.rise / 3;
      boulder->d.objf211.rotY += 12;
      boulder->d.objf211.rotZ -= 7;
      boulder->d.objf211.rotX += 3;
      OBJ.xOfs_unused = (OBJ.dx * (OBJ.dropSpan - OBJ.fallOfs)) / OBJ.dropSpan;
      OBJ.zOfs_unused = (OBJ.dz * (OBJ.dropSpan - OBJ.fallOfs)) / OBJ.dropSpan;
      OBJ.fallOfs = OBJ.dropHeight * rcos((0x2d - OBJ.fallTimer) * 0x1a) / ONE;
      boulder->y1.n = obj->y1.n + OBJ.fallOfs;
      OBJ.rise -= 4;
      AddObjPrim6(gGraphicsPtr->ot, boulder, 0);

      gCameraPos.vx = -(boulder->x1.n >> 3);
      gCameraPos.vz = -(boulder->z1.n >> 3);
      gCameraPos.vy = boulder->y1.n >> 3;
      gCameraRotation.vy += DEG(1.40625);

      OBJ.fallTimer--;
      if (OBJ.fallTimer == -1) {
         obj->state++;
      } else if (OBJ.fallTimer == 6) {
         gSignal3 = 1;
      }
      break;

   case 5:
      boulder = OBJ.boulder;
      boulder->functionIndex = OBJF_NULL;
      obj->functionIndex = OBJF_NULL;
      break;
   }
}

#undef OBJF
#define OBJF 185
void Objf185_Avalanche_Rock(Object *obj) {
   static s16 rockAnimData[] = {5, GFX_ROCK_1, 2, GFX_ROCK_2, 2, GFX_ROCK_3,
                                2, GFX_ROCK_4, 2, GFX_NULL,   1, GFX_NULL};

   Object *obj_s0;

   switch (obj->state) {
   case 0:
      obj_s0 = OBJ.parent;
      obj->x1.n = obj_s0->x1.n;
      obj->z1.n = obj_s0->z1.n;
      obj->y1.n = obj_s0->y1.n;

      obj_s0 = Obj_GetUnused();
      obj_s0->functionIndex = OBJF_NOOP;
      obj_s0->d.sprite.animData = rockAnimData;
      OBJ.sprite = obj_s0;

   // fallthrough
   case 1:
      obj_s0 = OBJ.sprite;
      obj_s0->d.sprite.animInitialized = 0;
      OBJ.height = 0;
      OBJ.theta = rand() % DEG(360);
      OBJ.velocity = rand() % 0x3c + 0x32;
      obj->state++;
      break;

   case 2:
      obj_s0 = OBJ.sprite;
      obj_s0->x1.n = obj->x1.n + CV(0.375) * rcos(OBJ.theta) / ONE;
      obj_s0->z1.n = obj->z1.n + CV(0.375) * rsin(OBJ.theta) / ONE;
      obj_s0->y1.n = obj->y1.n + OBJ.height;
      UpdateObjAnimation(obj_s0);
      AddObjPrim6(gGraphicsPtr->ot, obj_s0, 0);
      OBJ.height += OBJ.velocity;
      if (OBJ.height >= CV(3.0)) {
         obj->state = 1;
      }
      break;

   case 99:
      obj_s0 = OBJ.sprite;
      obj_s0->functionIndex = OBJF_NULL;
      obj->functionIndex = OBJF_NULL;
      break;
   }
}

#undef OBJF
#define OBJF 187
void Objf187_ExplosionStrikeSlay_Unused(Object *obj) {
   Object *fx;

   fx = CreatePositionedObj(obj, OBJF_EXPLOSION_STRIKE_UNUSED);
   fx->d.objf186.fxType = 2;
   obj->functionIndex = OBJF_NULL;
}

#undef OBJF
#define OBJF 186
void Objf186_ExplosionStrike_Unused(Object *obj) {
   Object *unitSprite;
   Object *fx;

   switch (obj->state) {
   case 0:
      unitSprite = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
      obj->x1.n = unitSprite->x1.n;
      obj->z1.n = unitSprite->z1.n;
      obj->y1.n = unitSprite->y1.n;

      fx = CreatePositionedObj(obj, OBJF_ENGULF_EXPLOSION_DAMAGE + OBJ.fxType);
      fx->d.objf140.clut = CLUT_REDS;

      fx = Obj_GetUnused();
      fx->functionIndex = OBJF_BOUNCE_ZOOM;

      obj->functionIndex = OBJF_NULL;
      break;
   }
}

#undef OBJF
#define OBJF 190
void Objf190_MagicArrowSingle_Unused(Object *obj) {
   /* Single-target Magic Arrow variant (17 arrows at one target + camera pull-back).
    * CONFIRMED UNUSED: no spell's gSpellsEx triple and no code path references 190. */
   Object *unitSprite;
   Object *targetSprite;
   Object *arrow;

   switch (obj->state) {
   case 0:
      unitSprite = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
      obj->x1.n = unitSprite->x1.n;
      obj->y1.n = unitSprite->y1.n;
      obj->z1.n = unitSprite->z1.n;
      obj->state++;

   // fallthrough
   case 1:
      arrow = Obj_GetUnused();
      arrow->functionIndex = OBJF_MAGIC_ARROW_ARROW;
      arrow->x1.n = obj->x1.n;
      arrow->y1.n = obj->y1.n + CV(0.25);
      arrow->z1.n = obj->z1.n;

      targetSprite = GetUnitSpriteAtPosition(gTargetZ, gTargetX);
      arrow->d.objf191.dx = targetSprite->x1.n - obj->x1.n;
      arrow->d.objf191.dz = targetSprite->z1.n - obj->z1.n;
      arrow->d.objf191.dy = (targetSprite->y1.n - obj->y1.n) + CV(0.25);

      obj->state3++;
      if (obj->state3 == 17) {
         obj->state++;
      }
      break;

   case 2:
      gCameraZoom.vz += 10;
      gCameraRotation.vx += 1;
      if (obj->state2 == 22) {
         gSignal3 = 1;
      } else if (obj->state2 == 34) {
         obj->functionIndex = OBJF_NULL;
      }
      obj->state2++;
      break;
   }
}

#undef OBJF
#define OBJF 199
void Objf199_MagicArrow_FX1(Object *obj) {
   Object *casterSprite;
   Object *targetSprite;
   Object *arrow;
   BVectorXZ *p;
   s16 i;

   switch (obj->state) {
   case 0:
      casterSprite = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
      OBJ.unitSprite = casterSprite;
      obj->state++;

   // fallthrough
   case 1:
      casterSprite = OBJ.unitSprite;

      p = (BVectorXZ *)gTargetCoords;
      for (i = 0; p->x != 0xff && i < 15; i++) {
         arrow = Obj_GetUnused();
         arrow->functionIndex = OBJF_MAGIC_ARROW_ARROW;
         arrow->x1.n = casterSprite->x1.n;
         arrow->y1.n = casterSprite->y1.n + CV(0.25);
         arrow->z1.n = casterSprite->z1.n;
         targetSprite = GetUnitSpriteAtPosition(p->z, p->x);
         arrow->d.objf191.dx = targetSprite->x1.n - casterSprite->x1.n;
         arrow->d.objf191.dz = targetSprite->z1.n - casterSprite->z1.n;
         arrow->d.objf191.dy = (targetSprite->y1.n - casterSprite->y1.n) + CV(0.25);
         p++;
      }

      gCameraZoom.vz += 9;
      gCameraRotation.vx += 8;
      gCameraRotation.vy += 0x10;

      if (Obj_CountUnused() < 50) {
         obj->state++;
      }

      obj->state2++;
      if (obj->state2 == 17) {
         obj->state++;
      }
      break;

   case 2:
      gCameraZoom.vz += 9;
      gCameraRotation.vx += 8;
      gCameraRotation.vy += 0x10;

      obj->state2++;
      if (obj->state2 == 35) {
         obj->functionIndex = OBJF_NULL;
         gSignal3 = 1;
      }
      break;
   }
}

#undef OBJF
#define OBJF 191
void Objf191_MagicArrow_Arrow(Object *obj) {
   Object *sprite;
   POLY_FT4 *poly;
   s32 timer, i, intensity;
   s16 a;

   switch (obj->state) {
   case 0:
      if (OBJ.clut_unused == CLUT_NULL) {
         OBJ.clut_unused = CLUT_REDS;
      }
      OBJ.wobbleX = 0x280 - rand() % 0x500;
      OBJ.wobbleY = 0x180 + rand() % 0x180;
      OBJ.wobbleZ = 0x280 - rand() % 0x500;
      obj->state++;
      break;

   case 1:
      sprite = Obj_GetUnused();
      sprite->functionIndex = OBJF_NOOP;
      sprite->d.sprite.gfxIdx = GFX_ARROW;
      sprite->d.sprite.semiTrans = 0;

      timer = OBJ.timer;
      sprite->d.sprite.coords[0].x =
          obj->x1.n + (OBJ.wobbleX * rsin(timer * 0x40) >> 12) + timer * OBJ.dx / 0x20;
      sprite->d.sprite.coords[0].z =
          obj->z1.n + (OBJ.wobbleZ * rsin(timer * 0x40) >> 12) + timer * OBJ.dz / 0x20;
      sprite->d.sprite.coords[0].y =
          obj->y1.n + (OBJ.wobbleY * rsin(timer * 0x40) >> 12) + timer * OBJ.dy / 0x20;

      sprite->d.sprite.coords[1].x =
          obj->x1.n + (OBJ.wobbleX * rsin((timer + 2) * 0x40) >> 12) + (timer + 2) * OBJ.dx / 0x20;
      sprite->d.sprite.coords[1].z =
          obj->z1.n + (OBJ.wobbleZ * rsin((timer + 2) * 0x40) >> 12) + (timer + 2) * OBJ.dz / 0x20;
      sprite->d.sprite.coords[1].y =
          obj->y1.n + (OBJ.wobbleY * rsin((timer + 2) * 0x40) >> 12) + (timer + 2) * OBJ.dy / 0x20;

      sprite->d.sprite.coords[2].x = sprite->d.sprite.coords[0].x;
      sprite->d.sprite.coords[2].z = sprite->d.sprite.coords[0].z;
      sprite->d.sprite.coords[2].y = sprite->d.sprite.coords[0].y + 0x80;
      sprite->d.sprite.coords[3].x = sprite->d.sprite.coords[1].x;
      sprite->d.sprite.coords[3].z = sprite->d.sprite.coords[1].z;
      sprite->d.sprite.coords[3].y = sprite->d.sprite.coords[1].y + 0x80;

      sprite->x1.n = (sprite->d.sprite.coords[0].x + sprite->d.sprite.coords[1].x) / 2;
      sprite->z1.n = (sprite->d.sprite.coords[0].z + sprite->d.sprite.coords[1].z) / 2;
      sprite->y1.n = (sprite->d.sprite.coords[0].y + sprite->d.sprite.coords[1].y +
                      sprite->d.sprite.coords[2].y + sprite->d.sprite.coords[3].y) /
                     4;

      AddObjPrim3(gGraphicsPtr->ot, sprite);

      if (timer >= 4) {
         i = 4;
         sprite->d.sprite.semiTrans = 2;
         for (; i > 0; i--) {
            a = timer - (i + 1);

            sprite->d.sprite.coords[0].x =
                obj->x1.n + (OBJ.wobbleX * rsin(a * 0x40) >> 12) + a * OBJ.dx / 0x20;
            sprite->d.sprite.coords[0].z =
                obj->z1.n + (OBJ.wobbleZ * rsin(a * 0x40) >> 12) + a * OBJ.dz / 0x20;
            sprite->d.sprite.coords[0].y =
                obj->y1.n + (OBJ.wobbleY * rsin(a * 0x40) >> 12) + a * OBJ.dy / 0x20;

            sprite->d.sprite.coords[1].x =
                obj->x1.n + (OBJ.wobbleX * rsin((a + 2) * 0x40) >> 12) + (a + 2) * OBJ.dx / 0x20;
            sprite->d.sprite.coords[1].z =
                obj->z1.n + (OBJ.wobbleZ * rsin((a + 2) * 0x40) >> 12) + (a + 2) * OBJ.dz / 0x20;
            sprite->d.sprite.coords[1].y =
                obj->y1.n + (OBJ.wobbleY * rsin((a + 2) * 0x40) >> 12) + (a + 2) * OBJ.dy / 0x20;

            sprite->d.sprite.coords[2].x = sprite->d.sprite.coords[0].x;
            sprite->d.sprite.coords[2].z = sprite->d.sprite.coords[0].z;
            sprite->d.sprite.coords[2].y = sprite->d.sprite.coords[0].y + 0x80;
            sprite->d.sprite.coords[3].x = sprite->d.sprite.coords[1].x;
            sprite->d.sprite.coords[3].z = sprite->d.sprite.coords[1].z;
            sprite->d.sprite.coords[3].y = sprite->d.sprite.coords[1].y + 0x80;

            sprite->x1.n = (sprite->d.sprite.coords[0].x + sprite->d.sprite.coords[1].x) / 2;
            sprite->z1.n = (sprite->d.sprite.coords[0].z + sprite->d.sprite.coords[1].z) / 2;
            sprite->y1.n = (sprite->d.sprite.coords[0].y + sprite->d.sprite.coords[1].y +
                            sprite->d.sprite.coords[2].y + sprite->d.sprite.coords[3].y) /
                           4;

            AddObjPrim3(gGraphicsPtr->ot, sprite);
            poly = &gGraphicsPtr->quads[gQuadIndex - 1];
            intensity = 0x40 / (i + 1);
            setRGB0(poly, intensity, intensity, intensity);
         }
      }

      OBJ.timer++;
      if (OBJ.timer == 31) {
         obj->functionIndex = OBJF_NULL;
      }
      sprite->functionIndex = OBJF_NULL;
      break;
   }
}
