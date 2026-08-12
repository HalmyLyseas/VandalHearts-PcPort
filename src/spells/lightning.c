/* Lightning-family spell effects: Holy Lightning (Objf146/144/145 -- FX1 and the casting
 * bolt are in spells/summon_crest.c), Thunder Ball (Objf224/225/226 with the Objf128/129
 * wrappers' family), and Rolling Thunder (Objf200/195/196), dispatched via gSpellsEx
 * (model described in spells/casting_main.c's header).
 *
 * Cut content in this file (in gObjFunctionPointers but referenced by no spell's gSpellsEx
 * triple, no event script on the retail disc, and no code path): the quick thunder-strike
 * damage/slay pair (Objf120/121 -- shaped exactly like Thunder Ball's Objf128/129 wrappers)
 * and the lobbed energy-ball attack (Objf222/223, camera-following orb with a full-screen
 * flash and the gSignal3 main-handler handshake). Suffixed _Unused, kept byte-exact. */
#include "common.h"
#include "object.h"
#include "graphics.h"
#include "battle.h"


static s16 sExplosionAnimData_800fefb0[26] = {
    7, GFX_EXPLOSION_1, 2, GFX_EXPLOSION_2,  2, GFX_EXPLOSION_3,  2, GFX_EXPLOSION_4,
    2, GFX_EXPLOSION_5, 2, GFX_EXPLOSION_6,  2, GFX_EXPLOSION_7,  2, GFX_EXPLOSION_8,
    2, GFX_EXPLOSION_9, 2, GFX_EXPLOSION_10, 2, GFX_EXPLOSION_11, 2, GFX_NULL,
    0, GFX_NULL};

static s16 sGlobeAnimData_800fefe4[32] = {
    5, GFX_GLOBE_1, 1, GFX_GLOBE_2, 1, GFX_GLOBE_3, 1, GFX_GLOBE_4, 1, GFX_GLOBE_5, 1, GFX_GLOBE_6,
    1, GFX_GLOBE_7, 1, GFX_GLOBE_8, 1, GFX_GLOBE_7, 1, GFX_GLOBE_6, 1, GFX_GLOBE_5, 1, GFX_GLOBE_4,
    1, GFX_GLOBE_3, 1, GFX_GLOBE_2, 1, GFX_NULL,    1, GFX_NULL};

static s16 sGlobeAnimData_800ff024[32] = {
    4, GFX_GLOBE_1, 1, GFX_GLOBE_2, 1, GFX_GLOBE_3, 1, GFX_GLOBE_4, 1, GFX_GLOBE_5, 1, GFX_GLOBE_6,
    1, GFX_GLOBE_7, 1, GFX_GLOBE_8, 1, GFX_GLOBE_7, 1, GFX_GLOBE_6, 1, GFX_GLOBE_5, 1, GFX_GLOBE_4,
    1, GFX_GLOBE_3, 1, GFX_GLOBE_2, 1, GFX_NULL,    1, GFX_NULL};

static s16 sLightningAnimData_800ff064[32] = {
    4, GFX_LIGHTNING_1, 1, GFX_LIGHTNING_2, 1, GFX_LIGHTNING_3, 1, GFX_LIGHTNING_4,
    1, GFX_LIGHTNING_5, 1, GFX_LIGHTNING_6, 1, GFX_LIGHTNING_7, 1, GFX_LIGHTNING_8,
    1, GFX_LIGHTNING_7, 1, GFX_LIGHTNING_6, 1, GFX_LIGHTNING_5, 1, GFX_LIGHTNING_4,
    1, GFX_LIGHTNING_3, 1, GFX_LIGHTNING_2, 1, GFX_NULL,        1, GFX_NULL};

static s16 sExplosionAnimData_800ff0a4[26] = {
    0, GFX_EXPLOSION_1, 1, GFX_EXPLOSION_2,  1, GFX_EXPLOSION_3,  1, GFX_EXPLOSION_4,
    1, GFX_EXPLOSION_5, 1, GFX_EXPLOSION_6,  1, GFX_EXPLOSION_7,  1, GFX_EXPLOSION_8,
    1, GFX_EXPLOSION_9, 1, GFX_EXPLOSION_10, 1, GFX_EXPLOSION_11, 1, GFX_NULL,
    0, GFX_NULL};

#undef OBJF
#define OBJF 146
void Objf146_HolyLightning_FX3(Object *obj) {
   Object *fx2;

   fx2 = Obj_GetUnused();
   fx2->functionIndex = OBJF_HOLY_LIGHTNING_FX2;
   fx2->x1.s.hi = obj->x1.s.hi;
   fx2->z1.s.hi = obj->z1.s.hi;
   fx2->d.objf144.endingFxType = 2;

   obj->functionIndex = OBJF_NULL;
}

#undef OBJF
#define OBJF 144
void Objf144_HolyLightning_FX2(Object *obj) {
   Object *targetSprite;
   Object *obj_v1;
   s32 i;

   switch (obj->state) {
   case 0:

      switch (obj->state2) {
      case 0:
         targetSprite = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
         obj->x1.n = targetSprite->x1.n;
         obj->y1.n = targetSprite->y1.n;
         obj->z1.n = targetSprite->z1.n;

         for (i = 0; i < OBJ_DATA_CT; i++) {
            obj_v1 = &gObjectArray[i];
            if (obj_v1->functionIndex == OBJF_FOCUS_CAMERA) {
               obj_v1->functionIndex = OBJF_NULL;
            }
         }

         OBJ.unitSprite = targetSprite;

         obj_v1 = Obj_GetUnused();
         obj_v1->functionIndex = OBJF_FLASHING_UNIT_SPRITE;
         obj_v1->d.objf149.clut = CLUT_BLUES;
         obj_v1->x1.s.hi = targetSprite->x1.s.hi;
         obj_v1->z1.s.hi = targetSprite->z1.s.hi;
         OBJ.targetFlashingFx = obj_v1;

         OBJ.timer = 150;
         obj->state2++;

      // fallthrough
      case 1:
         if (OBJ.timer > 135) {
            gLightColor.r -= 6;
            gLightColor.g -= 6;
            gLightColor.b -= 6;
         } else if (OBJ.timer < 15) {
            gLightColor.r += 6;
            gLightColor.g += 6;
            gLightColor.b += 6;
         }
         gCameraZoom.vz += 1;
         gCameraRotation.vx += 2;
         gCameraRotation.vy += 0x10;

         // OBJ.timer--;
         if (OBJ.timer-- % 8 == 0) {
            obj_v1 = Obj_GetUnused();
            obj_v1->functionIndex = OBJF_HOLY_LIGHTNING_ELECTRIC_ORB;
            obj_v1->x1.n = obj->x1.n;
            obj_v1->y1.n = obj->y1.n;
            obj_v1->z1.n = obj->z1.n;
         }
         if (OBJ.timer == 10) {
            targetSprite = OBJ.unitSprite;
            obj_v1 = Obj_GetUnused();
            obj_v1->functionIndex = OBJF_FOCUS_CAMERA;
            obj_v1->d.objf026.target = targetSprite;
         } else if (OBJ.timer == 0) {
            obj_v1 = OBJ.targetFlashingFx;
            obj_v1->state = 99;
            obj->state2++;
         }
         break;

      case 2:
         obj_v1 = Obj_GetUnused();
         obj_v1->functionIndex = OBJF_ENGULF_LIGHTNING_DAMAGE + OBJ.endingFxType;
         obj_v1->d.objf136.clut = CLUT_BLUES;
         obj_v1->x1.n = obj->x1.n;
         obj_v1->y1.n = obj->y1.n;
         obj_v1->z1.n = obj->z1.n;

         obj->functionIndex = OBJF_NULL;
         gLightColor.r = 0x80;
         gLightColor.g = 0x80;
         gLightColor.b = 0x80;
         break;
      }

      break;
   }
}

#undef OBJF
#define OBJF 145
void Objf145_HolyLightning_ElectricOrb(Object *obj) {
   static s16 lightningAnimData[] = {
       4, GFX_LIGHTNING_1, 2, GFX_LIGHTNING_2, 2, GFX_LIGHTNING_3, 2, GFX_LIGHTNING_4,
       2, GFX_LIGHTNING_4, 2, GFX_LIGHTNING_5, 2, GFX_LIGHTNING_5, 2, GFX_LIGHTNING_6,
       2, GFX_LIGHTNING_7, 2, GFX_LIGHTNING_8, 2, GFX_LIGHTNING_1, 2, GFX_LIGHTNING_2,
       2, GFX_LIGHTNING_3, 2, GFX_LIGHTNING_4, 2, GFX_LIGHTNING_4, 2, GFX_LIGHTNING_5,
       2, GFX_LIGHTNING_5, 2, GFX_LIGHTNING_6, 2, GFX_LIGHTNING_7, 2, GFX_LIGHTNING_8,
       2, GFX_NULL,        0, GFX_NULL};

   static s16 globeAnimData[] = {4, GFX_GLOBE_2, 1, GFX_GLOBE_3, 1, GFX_GLOBE_4, 1, GFX_GLOBE_5,
                                 1, GFX_GLOBE_6, 1, GFX_GLOBE_7, 1, GFX_GLOBE_8, 1, GFX_GLOBE_8,
                                 1, GFX_GLOBE_7, 1, GFX_GLOBE_6, 1, GFX_GLOBE_5, 1, GFX_GLOBE_4,
                                 1, GFX_GLOBE_3, 1, GFX_GLOBE_2, 1, GFX_NULL,    0, GFX_NULL};

   s16 cluts[4] = {CLUT_REDS, CLUT_BLUES, CLUT_PURPLES, CLUT_GREENS};
   Object *obj_s4;
   Object *orbSprite;
   POLY_FT4 *poly;
   s16 a, clut;

   switch (obj->state) {
   case 0:
      OBJ.x_3 = 0x40 - rand() % 0x80;
      OBJ.z_3 = 0x40 - rand() % 0x80;
      OBJ.x_4 = 0x200 - rand() % 0x400;
      OBJ.z_4 = 0x200 - rand() % 0x400;

      a = 0x300 - rsin(((abs(OBJ.x_4) + abs(OBJ.z_4)) / 2 + 0x200) * 2) / 0x40;
      obj->y1.n += a;
      OBJ.y_3 = -a;

      if (rand() % 2 != 0) {
         OBJ.z_1 = -((OBJ.z_4 - OBJ.z_3) * rsin(0x156)) / ONE;
         OBJ.x_2 = -((OBJ.x_4 - OBJ.x_3) * rsin(0x2aa)) / ONE;
      } else {
         OBJ.z_1 = -((OBJ.z_4 - OBJ.z_3) * rcos(0x2aa)) / ONE;
         OBJ.x_2 = -((OBJ.x_4 - OBJ.x_3) * rcos(0x156)) / ONE;
      }
      if (rand() % 2 != 0) {
         OBJ.y_1 = OBJ.y_3 * rsin(0x156) / ONE;
         OBJ.y_2 = OBJ.y_3 * rsin(0x2aa) / ONE;
      } else {
         OBJ.y_1 = OBJ.y_3 * rcos(0x2aa) / ONE;
         OBJ.y_2 = OBJ.y_3 * rcos(0x156) / ONE;
      }
      if (rand() % 2 != 0) {
         OBJ.z_1 = -((OBJ.z_4 - OBJ.z_3) * rsin(0x156)) / ONE;
         OBJ.z_2 = -((OBJ.z_4 - OBJ.z_3) * rsin(0x2aa)) / ONE;
      } else {
         OBJ.z_1 = -((OBJ.z_4 - OBJ.z_3) * rcos(0x2aa)) / ONE;
         OBJ.z_2 = -((OBJ.z_4 - OBJ.z_3) * rcos(0x156)) / ONE;
      }

      clut = cluts[rand() % ARRAY_COUNT(cluts)];

      obj_s4 = Obj_GetUnused();
      OBJ.lightningSprite = obj_s4;
      obj_s4->functionIndex = OBJF_NOOP;
      obj_s4->d.sprite.animData = lightningAnimData;
      obj_s4->d.sprite.animInitialized = 0;
      obj_s4->d.sprite.clut = clut;

      obj_s4 = Obj_GetUnused();
      OBJ.orbSprite = obj_s4;
      obj_s4->functionIndex = OBJF_NOOP;
      obj_s4->d.sprite.animData = globeAnimData;
      obj_s4->d.sprite.animInitialized = 0;
      obj_s4->d.sprite.boxIdx = 4;
      obj_s4->d.sprite.clut = clut;
      obj_s4->x1.n = obj->x1.n + OBJ.x_4;
      obj_s4->z1.n = obj->z1.n + OBJ.z_4;
      obj_s4->y1.n = obj->y1.n;

      obj->state++;
      break;

   case 1:
      orbSprite = OBJ.orbSprite;
      UpdateObjAnimation(orbSprite);
      clut = orbSprite->d.sprite.clut;

      orbSprite->d.sprite.semiTrans = 2;
      AddObjPrim6(gGraphicsPtr->ot, orbSprite, 0);
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      setRGB0(poly, OBJ.fade, OBJ.fade, OBJ.fade);

      orbSprite->d.sprite.semiTrans = 3;
      orbSprite->d.sprite.clut = CLUT_MASK;
      AddObjPrim6(gGraphicsPtr->ot, orbSprite, 0);
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      setRGB0(poly, OBJ.fade, OBJ.fade, OBJ.fade);

      orbSprite->d.sprite.clut = clut;
      obj_s4 = OBJ.lightningSprite;

      OBJ.x_1 += 0x18 - rand() % 0x30;
      OBJ.y_1 += 4 - rand() % 8;
      OBJ.z_1 += 0x18 - rand() % 0x30;
      OBJ.x_2 += 0x18 - rand() % 0x30;
      OBJ.y_2 += 4 - rand() % 8;
      OBJ.z_2 += 0x18 - rand() % 0x30;
      OBJ.unusedJitterX += 0x18 - rand() % 0x30;
      OBJ.unusedJitterY += 4 - rand() % 8;
      OBJ.unusedJitterZ += 0x18 - rand() % 0x30;

      obj_s4->d.sprite.coords[0].x = obj->x1.n + CV(0.75) + OBJ.x_4;
      obj_s4->d.sprite.coords[0].y = obj->y1.n;
      obj_s4->d.sprite.coords[0].z = obj->z1.n + OBJ.z_4;
      obj_s4->d.sprite.coords[1].x = obj->x1.n - CV(0.75) + OBJ.x_4;
      obj_s4->d.sprite.coords[1].y = obj->y1.n;
      obj_s4->d.sprite.coords[1].z = obj->z1.n + OBJ.z_4;
      obj_s4->d.sprite.coords[2].x = obj->x1.n + OBJ.x_1 + CV(0.75);
      obj_s4->d.sprite.coords[2].y = obj->y1.n + OBJ.y_1;
      obj_s4->d.sprite.coords[2].z = obj->z1.n + OBJ.z_1;
      obj_s4->d.sprite.coords[3].x = obj->x1.n + OBJ.x_1 - CV(0.75);
      obj_s4->d.sprite.coords[3].y = obj->y1.n + OBJ.y_1;
      obj_s4->d.sprite.coords[3].z = obj->z1.n + OBJ.z_1;

      obj_s4->x1.n = (obj_s4->d.sprite.coords[0].x + obj_s4->d.sprite.coords[3].x) / 2;
      obj_s4->y1.n = (obj_s4->d.sprite.coords[0].y + obj_s4->d.sprite.coords[3].y) / 2;
      obj_s4->z1.n = (obj_s4->d.sprite.coords[0].z + obj_s4->d.sprite.coords[3].z) / 2;

      UpdateObjAnimation(obj_s4);

      obj_s4->d.sprite.semiTrans = 2;
      AddObjPrim3(gGraphicsPtr->ot, obj_s4);
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      setRGB0(poly, OBJ.fade, OBJ.fade, OBJ.fade);

      obj_s4->d.sprite.coords[0].x = obj->x1.n + CV(0.75);
      obj_s4->d.sprite.coords[0].y = obj->y1.n;
      obj_s4->d.sprite.coords[0].z = obj->z1.n;
      obj_s4->d.sprite.coords[1].x = obj->x1.n - CV(0.75);
      obj_s4->d.sprite.coords[1].y = obj->y1.n;
      obj_s4->d.sprite.coords[1].z = obj->z1.n;
      obj_s4->d.sprite.coords[2].x = obj->x1.n - OBJ.x_1 / 2 + CV(0.75);
      obj_s4->d.sprite.coords[2].y = obj->y1.n + OBJ.y_1;
      obj_s4->d.sprite.coords[2].z = obj->z1.n - OBJ.z_1 / 2;
      obj_s4->d.sprite.coords[3].x = obj->x1.n - OBJ.x_1 / 2 - CV(0.75);
      obj_s4->d.sprite.coords[3].y = obj->y1.n + OBJ.y_1;
      obj_s4->d.sprite.coords[3].z = obj->z1.n - OBJ.z_1 / 2;

      obj_s4->x1.n = (obj_s4->d.sprite.coords[0].x + obj_s4->d.sprite.coords[3].x) / 2;
      obj_s4->y1.n = (obj_s4->d.sprite.coords[0].y + obj_s4->d.sprite.coords[3].y) / 2;
      obj_s4->z1.n = (obj_s4->d.sprite.coords[0].z + obj_s4->d.sprite.coords[3].z) / 2;

      obj_s4->d.sprite.semiTrans = 2;
      AddObjPrim3(gGraphicsPtr->ot, obj_s4);
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      setRGB0(poly, OBJ.fade / 3, OBJ.fade / 3, OBJ.fade / 3);

      obj_s4->d.sprite.coords[0].x = obj->x1.n + OBJ.x_1 + CV(0.75);
      obj_s4->d.sprite.coords[0].y = obj->y1.n + OBJ.y_1;
      obj_s4->d.sprite.coords[0].z = obj->z1.n + OBJ.z_1;
      obj_s4->d.sprite.coords[1].x = obj->x1.n + OBJ.x_1 - CV(0.75);
      obj_s4->d.sprite.coords[1].y = obj->y1.n + OBJ.y_1;
      obj_s4->d.sprite.coords[1].z = obj->z1.n + OBJ.z_1;
      obj_s4->d.sprite.coords[2].x = obj->x1.n + OBJ.x_2 + CV(0.75);
      obj_s4->d.sprite.coords[2].y = obj->y1.n + OBJ.y_2;
      obj_s4->d.sprite.coords[2].z = obj->z1.n + OBJ.z_2;
      obj_s4->d.sprite.coords[3].x = obj->x1.n + OBJ.x_2 - CV(0.75);
      obj_s4->d.sprite.coords[3].y = obj->y1.n + OBJ.y_2;
      obj_s4->d.sprite.coords[3].z = obj->z1.n + OBJ.z_2;

      obj_s4->x1.n = (obj_s4->d.sprite.coords[0].x + obj_s4->d.sprite.coords[3].x) / 2;
      obj_s4->y1.n = (obj_s4->d.sprite.coords[0].y + obj_s4->d.sprite.coords[3].y) / 2;
      obj_s4->z1.n = (obj_s4->d.sprite.coords[0].z + obj_s4->d.sprite.coords[3].z) / 2;

      obj_s4->d.sprite.semiTrans = 2;
      AddObjPrim3(gGraphicsPtr->ot, obj_s4);
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      setRGB0(poly, OBJ.fade, OBJ.fade, OBJ.fade);

      obj_s4->d.sprite.coords[0].x = obj->x1.n - OBJ.x_1 / 2 + CV(0.75);
      obj_s4->d.sprite.coords[0].y = obj->y1.n + OBJ.y_1;
      obj_s4->d.sprite.coords[0].z = obj->z1.n - OBJ.z_1 / 2;
      obj_s4->d.sprite.coords[1].x = obj->x1.n - OBJ.x_1 / 2 - CV(0.75);
      obj_s4->d.sprite.coords[1].y = obj->y1.n + OBJ.y_1;
      obj_s4->d.sprite.coords[1].z = obj->z1.n - OBJ.z_1 / 2;
      obj_s4->d.sprite.coords[2].x = obj->x1.n - OBJ.x_2 / 2 + CV(0.75);
      obj_s4->d.sprite.coords[2].y = obj->y1.n + OBJ.y_2;
      obj_s4->d.sprite.coords[2].z = obj->z1.n - OBJ.z_2 / 2;
      obj_s4->d.sprite.coords[3].x = obj->x1.n - OBJ.x_2 / 2 - CV(0.75);
      obj_s4->d.sprite.coords[3].y = obj->y1.n + OBJ.y_2;
      obj_s4->d.sprite.coords[3].z = obj->z1.n - OBJ.z_2 / 2;

      obj_s4->x1.n = (obj_s4->d.sprite.coords[0].x + obj_s4->d.sprite.coords[3].x) / 2;
      obj_s4->y1.n = (obj_s4->d.sprite.coords[0].y + obj_s4->d.sprite.coords[3].y) / 2;
      obj_s4->z1.n = (obj_s4->d.sprite.coords[0].z + obj_s4->d.sprite.coords[3].z) / 2;

      obj_s4->d.sprite.semiTrans = 2;
      AddObjPrim3(gGraphicsPtr->ot, obj_s4);
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      setRGB0(poly, OBJ.fade / 3, OBJ.fade / 3, OBJ.fade / 3);

      obj_s4->d.sprite.coords[0].x = obj->x1.n + OBJ.x_2 + CV(0.75);
      obj_s4->d.sprite.coords[0].y = obj->y1.n + OBJ.y_2;
      obj_s4->d.sprite.coords[0].z = obj->z1.n + OBJ.z_2;
      obj_s4->d.sprite.coords[1].x = obj->x1.n + OBJ.x_2 - CV(0.75);
      obj_s4->d.sprite.coords[1].y = obj->y1.n + OBJ.y_2;
      obj_s4->d.sprite.coords[1].z = obj->z1.n + OBJ.z_2;
      obj_s4->d.sprite.coords[2].x = obj->x1.n + OBJ.x_3 + CV(0.8125) - rand() % CV(0.125);
      obj_s4->d.sprite.coords[2].y = obj->y1.n + OBJ.y_3;
      obj_s4->d.sprite.coords[2].z = obj->z1.n + OBJ.z_3 + CV(0.0625) - rand() % CV(0.125);
      obj_s4->d.sprite.coords[3].x = obj->x1.n + OBJ.x_3 - CV(0.6875) - rand() % CV(0.125);
      obj_s4->d.sprite.coords[3].y = obj->y1.n + OBJ.y_3;
      obj_s4->d.sprite.coords[3].z = obj->z1.n + OBJ.z_3 + CV(0.0625) - rand() % CV(0.125);
      obj_s4->x1.n = (obj_s4->d.sprite.coords[0].x + obj_s4->d.sprite.coords[3].x) / 2;
      obj_s4->y1.n = (obj_s4->d.sprite.coords[0].y + obj_s4->d.sprite.coords[3].y) / 2;
      obj_s4->z1.n = (obj_s4->d.sprite.coords[0].z + obj_s4->d.sprite.coords[3].z) / 2;

      obj_s4->d.sprite.semiTrans = 2;
      AddObjPrim3(gGraphicsPtr->ot, obj_s4);
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      setRGB0(poly, OBJ.fade, OBJ.fade, OBJ.fade);

      obj_s4->d.sprite.coords[0].x = obj->x1.n - OBJ.x_2 / 2 + CV(0.75);
      obj_s4->d.sprite.coords[0].y = obj->y1.n + OBJ.y_2;
      obj_s4->d.sprite.coords[0].z = obj->z1.n - OBJ.z_2 / 2;
      obj_s4->d.sprite.coords[1].x = obj->x1.n - OBJ.x_2 / 2 - CV(0.75);
      obj_s4->d.sprite.coords[1].y = obj->y1.n + OBJ.y_2;
      obj_s4->d.sprite.coords[1].z = obj->z1.n - OBJ.z_2 / 2;
      obj_s4->d.sprite.coords[2].x = obj->x1.n - OBJ.x_3 / 2 + CV(0.8125) - rand() % CV(0.125);
      obj_s4->d.sprite.coords[2].y = obj->y1.n + OBJ.y_3;
      obj_s4->d.sprite.coords[2].z = obj->z1.n - OBJ.z_3 / 2 + CV(0.0625) - rand() % CV(0.125);
      obj_s4->d.sprite.coords[3].x = obj->x1.n - OBJ.x_3 / 2 - CV(0.6875) - rand() % CV(0.125);
      obj_s4->d.sprite.coords[3].y = obj->y1.n + OBJ.y_3;
      obj_s4->d.sprite.coords[3].z = obj->z1.n - OBJ.z_3 / 2 + CV(0.0625) - rand() % CV(0.125);

      obj_s4->x1.n = (obj_s4->d.sprite.coords[0].x + obj_s4->d.sprite.coords[3].x) / 2;
      obj_s4->y1.n = (obj_s4->d.sprite.coords[0].y + obj_s4->d.sprite.coords[3].y) / 2;
      obj_s4->z1.n = (obj_s4->d.sprite.coords[0].z + obj_s4->d.sprite.coords[3].z) / 2;

      obj_s4->d.sprite.semiTrans = 2;
      AddObjPrim3(gGraphicsPtr->ot, obj_s4);
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      setRGB0(poly, OBJ.fade / 3, OBJ.fade / 3, OBJ.fade / 3);

      switch (obj->state2) {
      case 0:
         OBJ.fade += 15;
         if (OBJ.fade > 191) {
            obj->state2++;
         }
         break;
      case 1:
         OBJ.fade -= 15;
         if (OBJ.fade < 0) {
            OBJ.fade = 0;
         }
         break;
      }

      if (obj_s4->d.sprite.animFinished) {
         obj->functionIndex = OBJF_NULL;
         obj_s4->functionIndex = OBJF_NULL;
         orbSprite->functionIndex = OBJF_NULL;
      }

      break;
   }
}

#undef OBJF
#define OBJF 224
void Objf224_ThunderBall_FX1(Object *obj) {
   Object *unitSprite;
   Object *orb;

   switch (obj->state) {
   case 0:
      OBJ.savedZoom = gCameraZoom.vz;
      unitSprite = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);

      orb = Obj_GetUnused();
      orb->functionIndex = OBJF_THUNDER_BALL_INITIAL_ORB;
      orb->x1.n = unitSprite->x1.n;
      orb->y1.n = unitSprite->y1.n;
      orb->z1.n = unitSprite->z1.n;
      orb->d.objf225.dx = 0; // unused
      orb->d.objf225.dz = 0; // unused
      orb->d.objf225.dy = CV(3.0);

      obj->state++;
      break;

   case 1:
      obj->state3++;
      if (gLightColor.r < 36) {
         obj->state++;
      } else {
         gLightColor.r -= 4;
         gLightColor.g -= 4;
         gLightColor.b -= 4;
      }
      break;

   case 2:
      obj->state3++;
      if (obj->state3 > 56) {
         gLightColor.r += 8;
         gLightColor.g += 8;
         gLightColor.b += 8;
      }
      if (obj->state3 == 68) {
         gLightColor.r = 0x80;
         gLightColor.g = 0x80;
         gLightColor.b = 0x80;
         gSignal3 = 1;
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 225
void Objf225_ThunderBall_InitialOrb(Object *obj) {
   Object *obj_s2;
   Object *obj_s1;
   POLY_FT4 *poly;
   BVectorXZ *p;

   switch (obj->state) {
   case 0:
      obj_s2 = Obj_GetUnused();
      OBJ.orbSprite = obj_s2;
      obj_s2->functionIndex = OBJF_NOOP;
      obj_s2->d.sprite.animData = sGlobeAnimData_800ff024;
      obj_s2->d.sprite.animInitialized = 0;
      obj_s2->d.sprite.boxIdx = 4;
      obj_s2->d.sprite.clut = CLUT_BLUES;

      obj_s2 = Obj_GetUnused();
      OBJ.lightningSprite = obj_s2;
      obj_s2->functionIndex = OBJF_NOOP;
      obj_s2->d.sprite.animData = sLightningAnimData_800ff064;
      obj_s2->d.sprite.animInitialized = 0;
      obj_s2->d.sprite.boxIdx = 4;
      obj_s2->d.sprite.clut = CLUT_PURPLES;

      obj->state++;

   // fallthrough
   case 1:
      obj_s2 = OBJ.orbSprite;
      obj_s1 = OBJ.lightningSprite;
      obj_s2->x1.n = obj->x1.n;
      obj_s2->z1.n = obj->z1.n;
      obj_s2->y1.n = obj->y1.n + OBJ.timer * OBJ.dy / 48 + CV(0.875);
      obj_s1->x1.n = obj_s2->x1.n;
      obj_s1->z1.n = obj_s2->z1.n;
      obj_s1->y1.n = obj_s2->y1.n;

      if (obj->state2 == 0) {
         UpdateObjAnimation(obj_s1);
         AddObjPrim6(gGraphicsPtr->ot, obj_s1, 0);
      }
      UpdateObjAnimation(obj_s2);
      AddObjPrim6(gGraphicsPtr->ot, obj_s2, 0);
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      setRGB0(poly, 0xff, 0xff, 0xff);
      gCameraZoom.vz += 10;

      switch (obj->state2) {
      case 0:
         OBJ.timer++;
         if (OBJ.timer == 37) {
            obj_s1->functionIndex = OBJF_NULL;
            obj->state2++;
            obj_s2->d.sprite.animData = sExplosionAnimData_800ff0a4;
            obj_s2->d.sprite.animInitialized = 0;
            obj_s2->d.sprite.boxIdx = 4;
            obj_s2->d.sprite.clut = CLUT_REDS;
         }
         break;

      case 1:
         OBJ.timer++;
         if (OBJ.timer == 46) {
            for (p = (BVectorXZ *)gTargetCoords; p->x != 0xff; p++) {
               obj_s1 = GetUnitSpriteAtPosition(p->z, p->x);
               obj_s2 = Obj_GetUnused();
               obj_s2->functionIndex = OBJF_THUNDER_BALL_CHILD_ORB;
               obj_s2->x1.n = obj->x1.n;
               obj_s2->z1.n = obj->z1.n;
               obj_s2->y1.n = obj->y1.n + OBJ.dy + CV(0.875);
               obj_s2->d.objf226.dx = obj_s1->x1.n - obj_s2->x1.n;
               obj_s2->d.objf226.dz = obj_s1->z1.n - obj_s2->z1.n;
               obj_s2->d.objf226.dy = (obj_s1->y1.n - obj_s2->y1.n) + CV(0.5);
            }
         } else if (OBJ.timer == 48) {
            obj_s2->functionIndex = OBJF_NULL;
            obj->functionIndex = OBJF_NULL;
         }
         break;
      }

      break;
   }
}

#undef OBJF
#define OBJF 226
void Objf226_ThunderBall_ChildOrb(Object *obj) {
   Object *obj_s2;
   Object *lightningSprite;
   POLY_FT4 *poly;
   s32 timer;

   switch (obj->state) {
   case 0:
      OBJ.wobbleX = CV(1.0) - rand() % CV(2.0);
      OBJ.wobbleZ = CV(1.0) - rand() % CV(2.0);

      obj_s2 = Obj_GetUnused();
      OBJ.orbSprite = obj_s2;
      obj_s2->functionIndex = OBJF_NOOP;
      obj_s2->d.sprite.animData = sGlobeAnimData_800ff024;
      obj_s2->d.sprite.animInitialized = 0;
      obj_s2->d.sprite.boxIdx = 4;
      obj_s2->d.sprite.clut = CLUT_BLUES;

      obj_s2 = Obj_GetUnused();
      OBJ.lightningSprite = obj_s2;
      obj_s2->functionIndex = OBJF_NOOP;
      obj_s2->d.sprite.animData = sLightningAnimData_800ff064;
      obj_s2->d.sprite.animInitialized = 0;
      obj_s2->d.sprite.boxIdx = 4;
      obj_s2->d.sprite.clut = CLUT_PURPLES;

      obj->state++;

   // fallthrough
   case 1:
      obj_s2 = OBJ.orbSprite;
      lightningSprite = OBJ.lightningSprite;
      timer = OBJ.timer;

      obj_s2->x1.n =
          obj->x1.n + (OBJ.wobbleX * rsin(timer * 0x40) >> 11) + (timer * OBJ.dx / 0x20);
      obj_s2->z1.n =
          obj->z1.n + (OBJ.wobbleZ * rsin(timer * 0x40) >> 11) + (timer * OBJ.dz / 0x20);
      obj_s2->y1.n = obj->y1.n + (CV(1.5) * rsin(timer * 0x40) >> 11) + (timer * OBJ.dy / 0x20);

      lightningSprite->x1.n = obj_s2->x1.n;
      lightningSprite->z1.n = obj_s2->z1.n;
      lightningSprite->y1.n = obj_s2->y1.n;

      if (OBJ.timer < 22) {
         UpdateObjAnimation(lightningSprite);
         AddObjPrim6(gGraphicsPtr->ot, lightningSprite, 0);
      }
      UpdateObjAnimation(obj_s2);
      AddObjPrim6(gGraphicsPtr->ot, obj_s2, 0);
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      setRGB0(poly, 0xff, 0xff, 0xff);

      OBJ.timer++;
      if (OBJ.timer == 22) {
         lightningSprite->functionIndex = OBJF_NULL;
         obj_s2->d.sprite.animData = sExplosionAnimData_800ff0a4;
         obj_s2->d.sprite.animInitialized = 0;
         obj_s2->d.sprite.boxIdx = 4;
         obj_s2->d.sprite.clut = CLUT_REDS;
      } else if (OBJ.timer == 32) {
         obj_s2->functionIndex = OBJF_NULL;
         obj->functionIndex = OBJF_NULL;
      }

      break;
   }
}

#undef OBJF
#define OBJF 222
void Objf222_EnergyBallLob_Unused(Object *obj) {
   Object *obj_s0;
   Object *obj_s1;
   Object *obj_s2;
   POLY_FT4 *poly;

   switch (obj->state) {
   case 0:
      OBJ.savedZoom = gCameraZoom.vz;
      obj_s0 = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
      obj_s1 = Obj_GetUnused();
      obj_s1->functionIndex = OBJF_ENERGY_BALL_UNUSED;
      obj_s1->x1.n = obj_s0->x1.n;
      obj_s1->y1.n = obj_s0->y1.n;
      obj_s1->z1.n = obj_s0->z1.n;
      obj_s2 = GetUnitSpriteAtPosition(gTargetZ, gTargetX);
      obj_s1->d.objf223.dx = obj_s2->x1.n - obj_s0->x1.n;
      obj_s1->d.objf223.dz = obj_s2->z1.n - obj_s0->z1.n;
      obj_s1->d.objf223.dy = (obj_s2->y1.n - obj_s0->y1.n) + CV(2.0);
      OBJ.fx = obj_s1;

      obj_s1 = Obj_GetUnused();
      obj_s1->functionIndex = OBJF_FOCUS_CAMERA;
      obj_s1->d.objf026.target = obj_s2;
      obj_s1->d.objf026.type = 2;
      OBJ.cam = obj_s1;

      obj->state++;
      break;

   case 1:
      obj_s0 = OBJ.fx;
      obj_s2 = obj_s0->d.objf223.orbSprite;
      gCameraZoom.vz = OBJ.savedZoom + (obj_s2->y1.n >> 4) + (obj_s2->y1.n >> 5);
      obj_s1 = OBJ.cam;
      if (obj_s1->functionIndex == OBJF_FOCUS_CAMERA) {
         obj_s1->d.objf026.target = obj_s2;
         obj_s1->d.objf026.zoom = gCameraZoom.vz;
      } else {
         obj_s1 = Obj_GetUnused();
         obj_s1->functionIndex = OBJF_FOCUS_CAMERA;
         obj_s1->d.objf026.target = obj_s2;
         OBJ.cam = obj_s1;
         obj_s1->d.objf026.zoom = gCameraZoom.vz;
      }

      if (obj->state3++ > 51) {
         poly = &gGraphicsPtr->quads[gQuadIndex];
#ifdef PC_PORT
         /* bugreport-02: type the textured quad explicitly (fx omits SetPolyFT4; PSX relies on a stale
          * POLY_FT4 slot code, ours is stale-POLY_F4 -> texture dropped). */
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

         OBJ.fade += 16;
      }
      if (obj->state3 == 61) {
         obj_s1->functionIndex = OBJF_NULL;
         obj->state++;
         gSignal3 = 1;
         obj_s1->d.objf026.todo_x2c = 0x80;
      }
      break;

   case 2:
      poly = &gGraphicsPtr->quads[gQuadIndex];
#ifdef PC_PORT
      /* bugreport-02: type the textured quad explicitly (fx omits SetPolyFT4; PSX relies on a stale
       * POLY_FT4 slot code, ours is stale-POLY_F4 -> texture dropped). */
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
      if (OBJ.fade < 0) {
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 223
void Objf223_EnergyBall_Unused(Object *obj) {
   Object *obj_s2;
   Object *obj_s1;
   s32 timer;
   POLY_FT4 *poly;

   switch (obj->state) {
   case 0:
      obj_s2 = Obj_GetUnused();
      OBJ.orbSprite = obj_s2;
      obj_s2->functionIndex = OBJF_NOOP;
      obj_s2->d.sprite.animData = sGlobeAnimData_800ff024;
      obj_s2->d.sprite.animInitialized = 0;
      obj_s2->d.sprite.boxIdx = 4;
      obj_s2->d.sprite.clut = CLUT_BLUES;

      obj_s2 = Obj_GetUnused();
      OBJ.lightningSprite = obj_s2;
      obj_s2->functionIndex = OBJF_NOOP;
      obj_s2->d.sprite.animData = sLightningAnimData_800ff064;
      obj_s2->d.sprite.animInitialized = 0;
      obj_s2->d.sprite.boxIdx = 4;
      obj_s2->d.sprite.clut = CLUT_PURPLES;

      obj->state++;

   // fallthrough
   case 1:
      obj_s2 = OBJ.orbSprite;
      obj_s1 = OBJ.lightningSprite;

      timer = OBJ.timer;
      obj_s2->x1.n = obj->x1.n + timer * OBJ.dx / 0x40;
      obj_s2->z1.n = obj->z1.n + timer * OBJ.dz / 0x40;
      obj_s2->y1.n = obj->y1.n + (0x200 * rsin(timer * 0x20) >> 11) + (timer * OBJ.dy / 0x40);

      obj_s1->x1.n = obj_s2->x1.n;
      obj_s1->z1.n = obj_s2->z1.n;
      obj_s1->y1.n = obj_s2->y1.n;

      if (OBJ.timer < 52) {
         UpdateObjAnimation(obj_s1);
         AddObjPrim6(gGraphicsPtr->ot, obj_s1, 0);
      }
      UpdateObjAnimation(obj_s2);
      AddObjPrim6(gGraphicsPtr->ot, obj_s2, 0);
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      setRGB0(poly, 0xff, 0xff, 0xff);

      OBJ.timer++;
      if (OBJ.timer == 53) {
         obj_s1->functionIndex = OBJF_NULL;
         obj_s2->d.sprite.animData = sExplosionAnimData_800ff0a4;
         obj_s2->d.sprite.animInitialized = 0;
         obj_s2->d.sprite.boxIdx = 4;
         obj_s2->d.sprite.clut = CLUT_REDS;
      } else if (OBJ.timer == 64) {
         obj_s2->functionIndex = OBJF_NULL;
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 120
void Objf120_ThunderStrikeDamage_Unused(Object *obj) {
   Object *unitSprite;
   Object *fx;

   unitSprite = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
   fx = Obj_GetUnused();
   fx->functionIndex = OBJF_THUNDER_STRIKE;
   fx->x1.n = unitSprite->x1.n;
   fx->y1.n = unitSprite->y1.n;
   fx->z1.n = unitSprite->z1.n;
   fx->d.objf148.timer = 4;
   fx->d.objf148.endingFxType = 0;

   obj->functionIndex = OBJF_NULL;
}

#undef OBJF
#define OBJF 121
void Objf121_ThunderStrikeSlay_Unused(Object *obj) {
   Object *unitSprite;
   Object *fx;

   unitSprite = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
   fx = Obj_GetUnused();
   fx->functionIndex = OBJF_THUNDER_STRIKE;
   fx->x1.n = unitSprite->x1.n;
   fx->y1.n = unitSprite->y1.n;
   fx->z1.n = unitSprite->z1.n;
   fx->d.objf148.timer = 4;
   fx->d.objf148.endingFxType = 2;

   obj->functionIndex = OBJF_NULL;
}

#undef OBJF
#define OBJF 200
void Objf200_RollingThunder_FX3(Object *obj) {
   Object *unitSprite;
   Object *fx;

   unitSprite = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
   fx = Obj_GetUnused();
   fx->functionIndex = OBJF_ROLLING_THUNDER_FX2;
   fx->x1.n = unitSprite->x1.n;
   fx->y1.n = unitSprite->y1.n;
   fx->z1.n = unitSprite->z1.n;
   fx->d.objf148.endingFxType = 2;

   obj->functionIndex = OBJF_NULL;
}

#undef OBJF
#define OBJF 195
void Objf195_RollingThunder_FX2(Object *obj) {
   static u8 cluts[] = {CLUT_REDS, CLUT_BLUES, CLUT_PURPLES, CLUT_GREENS};

   s32 i;
   Object *obj_v1;

   switch (obj->state) {
   case 0:
      for (i = 0; i < 4; i++) {
         obj_v1 = Obj_GetUnused();
         obj_v1->functionIndex = OBJF_ROLLING_THUNDER_ORB_PAIR;
         obj_v1->x1.s.hi = obj->x1.s.hi;
         obj_v1->z1.s.hi = obj->z1.s.hi;
         obj_v1->d.objf196.phase = i * 0x400;
         obj_v1->d.objf196.theta = DEG(45);
         obj_v1->d.objf196.clut = cluts[i];
      }
      obj->state++;

   // fallthrough
   case 1:
      OBJ.timer++;
      if (OBJ.timer == 191) {
         obj_v1 = Obj_GetUnused();
         obj_v1->functionIndex = OBJF_ENGULF_LIGHTNING_DAMAGE + OBJ.endingFxType;
         obj_v1->d.objf136.clut = CLUT_BLUES;
         obj_v1->x1.n = obj->x1.n;
         obj_v1->y1.n = obj->y1.n;
         obj_v1->z1.n = obj->z1.n;
      } else if (OBJ.timer == 196) {
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 196
void Objf196_RollingThunder_OrbPair(Object *obj) {
   Object *targetSprite;
   Object *lightningSprite;
   Object *orbSprite;
   POLY_FT4 *poly;
   s8 unused[96]; //? Object is also 96 bytes
   s16 x_3, y_3, z_3;
   s16 x_1, y_1, z_1;
   s16 x_2, y_2, z_2;

   switch (obj->state) {
   case 0:
      OBJ.targetSprite = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);

      orbSprite = Obj_GetUnused();
      orbSprite->functionIndex = OBJF_NOOP;
      orbSprite->d.sprite.clut = OBJ.clut;
      orbSprite->d.sprite.animData = sGlobeAnimData_800fefe4;
      OBJ.orbSprite = orbSprite;

      orbSprite = Obj_GetUnused(); // Re-used for lightning here
      orbSprite->functionIndex = OBJF_NOOP;
      orbSprite->d.sprite.clut = OBJ.clut;
      orbSprite->d.sprite.animData = sLightningAnimData_800ff064;
      OBJ.lightningSprite = orbSprite;

      obj->state++;

   // fallthrough
   case 1:

      switch (obj->state2) {
      case 0:
         OBJ.radius += 0x10;
         if (OBJ.radius == 0x100) {
            obj->state2++;
         }
         break;
      case 1:
         if (OBJ.timer == 180) {
            obj->state2++;
         }
         break;
      case 2:
         OBJ.radius -= 0x10;
         if (OBJ.radius == 0) {
            obj->state2++;
         }
         break;
      }

      targetSprite = OBJ.targetSprite;
      orbSprite = OBJ.orbSprite;
      lightningSprite = OBJ.lightningSprite;

      OBJ.wobble = rsin(rcos(OBJ.timer * 8));
      x_3 = OBJ.radius * rcos(OBJ.phase + OBJ.wobble) >> 12;
      z_3 = OBJ.radius * rsin(OBJ.phase + OBJ.wobble) >> 12;
      y_3 = OBJ.radius * rsin(OBJ.theta) >> 12;
      x_3 = rcos(OBJ.theta) * x_3 / ONE;
      z_3 = rcos(OBJ.theta) * z_3 / ONE;
      OBJ.theta += DEG(5.625);

      UpdateObjAnimation(orbSprite);

      orbSprite->x1.n = targetSprite->x1.n + x_3;
      orbSprite->z1.n = targetSprite->z1.n + z_3;
      orbSprite->y1.n = targetSprite->y1.n + CV(0.5) + y_3;
      AddObjPrim6(gGraphicsPtr->ot, orbSprite, 0);
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      setRGB0(poly, 0xff, 0xff, 0xff);

      x_1 = orbSprite->x1.n;
      y_1 = orbSprite->y1.n;
      z_1 = orbSprite->z1.n;

      orbSprite->x1.n = targetSprite->x1.n - x_3;
      orbSprite->z1.n = targetSprite->z1.n - z_3;
      orbSprite->y1.n = targetSprite->y1.n + CV(0.5) - y_3;
      AddObjPrim6(gGraphicsPtr->ot, orbSprite, 0);
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      setRGB0(poly, 0xff, 0xff, 0xff);

      x_2 = orbSprite->x1.n;
      y_2 = orbSprite->y1.n;
      z_2 = orbSprite->z1.n;

      x_3 = CV(0.5) - rand() % CV(1.0);
      z_3 = CV(0.5) - rand() % CV(1.0);
      y_3 = CV(0.5) - rand() % CV(1.0);

      UpdateObjAnimation(lightningSprite);

      lightningSprite->d.sprite.otOfs = 2;
      lightningSprite->d.sprite.semiTrans = 2;
      lightningSprite->d.sprite.coords[0].x = x_1;
      lightningSprite->d.sprite.coords[1].x = x_1;
      lightningSprite->d.sprite.coords[2].x = (x_1 + x_2) / 2 + x_3;
      lightningSprite->d.sprite.coords[3].x = (x_1 + x_2) / 2 + x_3;
      lightningSprite->d.sprite.coords[0].y = y_1 + CV(0.25);
      lightningSprite->d.sprite.coords[1].y = y_1 - CV(0.25);
      lightningSprite->d.sprite.coords[2].y = (y_1 + y_2) / 2 + y_3 + CV(0.25);
      lightningSprite->d.sprite.coords[3].y = (y_1 + y_2) / 2 + y_3 - CV(0.25);
      lightningSprite->d.sprite.coords[0].z = z_1;
      lightningSprite->d.sprite.coords[1].z = z_1;
      lightningSprite->d.sprite.coords[2].z = (z_1 + z_2) / 2 + z_3;
      lightningSprite->d.sprite.coords[3].z = (z_1 + z_2) / 2 + z_3;
      AddObjPrim4(gGraphicsPtr->ot, lightningSprite); // 1

      lightningSprite->d.sprite.coords[0].x = x_2;
      lightningSprite->d.sprite.coords[1].x = x_2;
      lightningSprite->d.sprite.coords[2].x = (x_1 + x_2) / 2 + x_3;
      lightningSprite->d.sprite.coords[3].x = (x_1 + x_2) / 2 + x_3;
      lightningSprite->d.sprite.coords[0].y = y_2 + CV(0.25);
      lightningSprite->d.sprite.coords[1].y = y_2 - CV(0.25);
      lightningSprite->d.sprite.coords[2].y = (y_1 + y_2) / 2 + y_3 + CV(0.25);
      lightningSprite->d.sprite.coords[3].y = (y_1 + y_2) / 2 + y_3 - CV(0.25);
      lightningSprite->d.sprite.coords[0].z = z_2;
      lightningSprite->d.sprite.coords[1].z = z_2;
      lightningSprite->d.sprite.coords[2].z = (z_1 + z_2) / 2 + z_3;
      lightningSprite->d.sprite.coords[3].z = (z_1 + z_2) / 2 + z_3;
      AddObjPrim4(gGraphicsPtr->ot, lightningSprite); // 2

      lightningSprite->d.sprite.semiTrans = 1;
      lightningSprite->d.sprite.coords[0].x = x_1;
      lightningSprite->d.sprite.coords[1].x = x_1;
      lightningSprite->d.sprite.coords[2].x = (x_1 + x_2) / 2 - x_3;
      lightningSprite->d.sprite.coords[3].x = (x_1 + x_2) / 2 - x_3;
      lightningSprite->d.sprite.coords[0].y = y_1 + CV(0.25);
      lightningSprite->d.sprite.coords[1].y = y_1 - CV(0.25);
      lightningSprite->d.sprite.coords[2].y = (y_1 + y_2) / 2 - y_3 + CV(0.25);
      lightningSprite->d.sprite.coords[3].y = (y_1 + y_2) / 2 - y_3 - CV(0.25);
      lightningSprite->d.sprite.coords[0].z = z_1;
      lightningSprite->d.sprite.coords[1].z = z_1;
      lightningSprite->d.sprite.coords[2].z = (z_1 + z_2) / 2 - z_3;
      lightningSprite->d.sprite.coords[3].z = (z_1 + z_2) / 2 - z_3;
      AddObjPrim4(gGraphicsPtr->ot, lightningSprite); // 3

      lightningSprite->d.sprite.coords[0].x = x_2;
      lightningSprite->d.sprite.coords[1].x = x_2;
      lightningSprite->d.sprite.coords[2].x = (x_1 + x_2) / 2 - x_3;
      lightningSprite->d.sprite.coords[3].x = (x_1 + x_2) / 2 - x_3;
      lightningSprite->d.sprite.coords[0].y = y_2 + CV(0.25);
      lightningSprite->d.sprite.coords[1].y = y_2 - CV(0.25);
      lightningSprite->d.sprite.coords[2].y = (y_1 + y_2) / 2 - y_3 + CV(0.25);
      lightningSprite->d.sprite.coords[3].y = (y_1 + y_2) / 2 - y_3 - CV(0.25);
      lightningSprite->d.sprite.coords[0].z = z_2;
      lightningSprite->d.sprite.coords[1].z = z_2;
      lightningSprite->d.sprite.coords[2].z = (z_1 + z_2) / 2 - z_3;
      lightningSprite->d.sprite.coords[3].z = (z_1 + z_2) / 2 - z_3;
      AddObjPrim4(gGraphicsPtr->ot, lightningSprite); // 4

      lightningSprite->d.sprite.semiTrans = 4;
      lightningSprite->d.sprite.coords[0].x = x_1;
      lightningSprite->d.sprite.coords[1].x = x_1;
      lightningSprite->d.sprite.coords[2].x = (x_1 + x_2) / 2 - x_3 / 2;
      lightningSprite->d.sprite.coords[3].x = (x_1 + x_2) / 2 - x_3 / 2;
      lightningSprite->d.sprite.coords[0].y = y_1 + CV(0.25);
      lightningSprite->d.sprite.coords[1].y = y_1 - CV(0.25);
      lightningSprite->d.sprite.coords[2].y = (y_1 + y_2) / 2 - y_3 / 2 + CV(0.25);
      lightningSprite->d.sprite.coords[3].y = (y_1 + y_2) / 2 - y_3 / 2 - CV(0.25);
      lightningSprite->d.sprite.coords[0].z = z_1;
      lightningSprite->d.sprite.coords[1].z = z_1;
      lightningSprite->d.sprite.coords[2].z = (z_1 + z_2) / 2 - z_3 / 2;
      lightningSprite->d.sprite.coords[3].z = (z_1 + z_2) / 2 - z_3 / 2;
      AddObjPrim4(gGraphicsPtr->ot, lightningSprite); // 5

      lightningSprite->d.sprite.coords[0].x = x_2;
      lightningSprite->d.sprite.coords[1].x = x_2;
      lightningSprite->d.sprite.coords[2].x = (x_1 + x_2) / 2 - x_3 / 2;
      lightningSprite->d.sprite.coords[3].x = (x_1 + x_2) / 2 - x_3 / 2;
      lightningSprite->d.sprite.coords[0].y = y_2 + CV(0.25);
      lightningSprite->d.sprite.coords[1].y = y_2 - CV(0.25);
      lightningSprite->d.sprite.coords[2].y = (y_1 + y_2) / 2 - y_3 / 2 + CV(0.25);
      lightningSprite->d.sprite.coords[3].y = (y_1 + y_2) / 2 - y_3 / 2 - CV(0.25);
      lightningSprite->d.sprite.coords[0].z = z_2;
      lightningSprite->d.sprite.coords[1].z = z_2;
      lightningSprite->d.sprite.coords[2].z = (z_1 + z_2) / 2 - z_3 / 2;
      lightningSprite->d.sprite.coords[3].z = (z_1 + z_2) / 2 - z_3 / 2;
      AddObjPrim4(gGraphicsPtr->ot, lightningSprite); // 6

      OBJ.timer++;
      if (OBJ.timer == 197) {
         orbSprite->functionIndex = OBJF_NULL;
         lightningSprite->functionIndex = OBJF_NULL;
         obj->functionIndex = OBJF_NULL;
      }

      break;
   }
}

