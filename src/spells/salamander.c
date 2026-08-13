/* Salamander, the crawling flame serpent (Objf334 FX1 driver, Objf335 head, Objf336
 * segments), plus Rolling Fire FX1 (Objf332, a stray) and the Wyrmfang flames
 * (Objf747_748/749 -- FX1 is in spells/shared_fx.c). Dispatched via gSpellsEx (see
 * spells/casting_main.c). Objf377 (breath head) is cut content: its breath phase is
 * signalled by no shipped head; suffixed _Unused, kept byte-exact.
 *
 * The head/quad initializer templates live in spells/dark_hurricane.c
 * (gSalamanderQuadTemplate / gSalamanderHeadGfxN / gSalamanderHeadGfxS): the retail TU
 * emitted them at the head of its .rodata, ahead of that file's jumptables, so after the
 * split they must stay in the first piece to keep the byte layout. */
#include "common.h"
#include "object.h"
#include "graphics.h"
#include "battle.h"
#include "field.h"
#include "state.h"
#include "units.h"
#include "audio.h"

/* Initializer templates address-locked in spells/dark_hurricane.c's .rodata (see the
 * note there). Struct-wrapped because Quad is an array typedef and arrays cannot be
 * copy-assigned. */
typedef struct { SVECTOR v[4]; } SalamanderQuad;
typedef struct { s16 v[8]; } SalamanderHeadGfx;
extern const SalamanderQuad gSalamanderQuadTemplate;
extern const SalamanderHeadGfx gSalamanderHeadGfxN;
extern const SalamanderHeadGfx gSalamanderHeadGfxS;

#undef OBJF
#define OBJF 332
void Objf332_RollingFire_FX1(Object *obj) {
   extern s16 gFlameAnimData_800ff8a4[];

   Object *dataStore;
   Cylinder *dsCylinder;

   switch (obj->state) {
   case 0:
      obj->x1.n = gTargetX * CV(1.0) + CV(0.5);
      obj->z1.n = gTargetZ * CV(1.0) + CV(0.5);
      obj->y1.n = GetTerrainElevation(gTargetZ, gTargetX);
      obj->d.sprite.animData = gFlameAnimData_800ff8a4;

      dataStore = Obj_GetUnused();
      dataStore->functionIndex = OBJF_NOOP;
      OBJ.dataStore = dataStore;
      dsCylinder = &dataStore->d.dataStore.cylinder;
      dsCylinder->bottom.vx = obj->x1.n;
      dsCylinder->bottom.vz = obj->z1.n;
      dsCylinder->bottom.vy = obj->y1.n;
      dsCylinder->top.vx = obj->x1.n;
      dsCylinder->top.vz = obj->z1.n;
      dsCylinder->top.vy = obj->y1.n + CV(4.0);
      dsCylinder->sideCt = 8;
      dsCylinder->semiTrans = 4;
      dsCylinder->gfxIdx = GFX_FLAME_1;
      dsCylinder->bottomRadius = CV(1.0);
      dsCylinder->topRadius = CV(1.0);
      dsCylinder->useColor = 1;
      dsCylinder->color.r = 0x80;
      dsCylinder->color.g = 0x80;
      dsCylinder->color.b = 0x80;

      obj->state2 = 0x200;
      obj->y2.n = 0x60;
      obj->y3.n = -4;
      obj->x2.n = 0;
      obj->x3.n = 0x10;
      obj->state++;

   // fallthrough
   case 1:
      PanCamera(obj->x1.n, obj->y1.n, obj->z1.n, 2);
      if (++obj->mem >= 4) {
         obj->mem = 0;
         obj->state++;
      }
      break;

   case 2:
      dataStore = OBJ.dataStore;
      dsCylinder = &dataStore->d.dataStore.cylinder;
      UpdateObjAnimation(obj);
      dsCylinder->gfxIdx = OBJ.gfxIdx;
      if (obj->mem >= 0x80) {
         dsCylinder->color.r = obj->mem - 1;
         dsCylinder->color.g = obj->mem - 1;
         dsCylinder->color.b = obj->mem - 1;
      }
      dsCylinder->bottom.vy = obj->y1.n;
      dsCylinder->top.vy = obj->y1.n + (CV(6.0) * rsin(obj->mem * 8) >> 12);
      dsCylinder->top.vx = obj->x1.n;
      dsCylinder->top.vz = obj->z1.n;
      dsCylinder->bottomRadius = CV(2.0) * rsin(obj->mem * 8) >> 12;
      dsCylinder->topRadius =
          dsCylinder->bottomRadius * 2 + (CV(2.0) * rsin(obj->mem * 0x20) >> 12);
      dsCylinder->theta = obj->x2.n;
      obj->x2.n += obj->x3.n;
      obj->x3.n += 1;
      dsCylinder->semiTrans = 4;
      RenderCylinder(dsCylinder);
      dsCylinder->top.vy = obj->y1.n + (CV(4.0) * rsin(obj->mem * 8) >> 12);
      dsCylinder->top.vx = obj->x1.n + (CV(0.5) * rcos(obj->mem * 0x20) >> 12);
      dsCylinder->top.vz = obj->z1.n + (CV(0.5) * rsin(obj->mem * 0x20) >> 12);
      dsCylinder->semiTrans = 2;
      RenderCylinder(dsCylinder);
      dsCylinder->theta = -dsCylinder->theta;
      dsCylinder->bottomRadius -= (dsCylinder->bottomRadius >> 1);
      dsCylinder->topRadius -= (dsCylinder->topRadius >> 1);
      RenderCylinder(dsCylinder);
      obj->state2 += obj->y2.n;
      obj->y2.n += obj->y3.n;
      obj->mem += 2;
      gCameraZoom.vz += (384 - gCameraZoom.vz) >> 4;
      gCameraRotation.vy -= 6;
      if (obj->mem >= 0x100) {
         obj->state++;
      }
      break;

   case 3:
      dataStore = OBJ.dataStore;
      dataStore->functionIndex = OBJF_NULL;
      obj->functionIndex = OBJF_NULL;
      gSignal3 = 1;
      break;
   }
}

#undef OBJF
#define OBJF 333
/* Sparkle spiralling inward onto a unit (~248 frames), laying rune-textured Objf323
 * ribbon segments every frame, then gSignal3. Cut: nothing dispatches 333 (Objf323's
 * other user is EVDATA29.DAT / Map20 -- see its comment in events/fx_scenes.c). */
void Objf333_RuneSpiral_Unused(Object *obj) {
   extern s16 gSparkleAnimData_800ff38c[14];
   Object *obj_s3;
   s16 a, b;

   switch (obj->state) {
   case 0:
      obj_s3 = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
      obj->x2.n = obj_s3->x1.n;
      obj->z2.n = obj_s3->z1.n;
      obj->y2.n = obj_s3->y1.n;
      obj->d.sprite.animData = gSparkleAnimData_800ff38c;
      // TODO: Object
      obj->d.sprite.coords[0].x = 0x400;
      obj->d.sprite.coords[0].z = -4;
      obj->d.sprite.coords[0].y = 0;
      obj->d.sprite.coords[1].x = 0x40;
      obj->x1.n = obj->x2.n + (CV(4.0) * rcos(0) >> 12);
      obj->z1.n = obj->z2.n + (CV(4.0) * rsin(0) >> 12);
      obj->y1.n = obj->y2.n + CV(1.0);
      obj->state++;

   // fallthrough
   case 1:
      obj_s3 = Obj_GetUnused();
      obj_s3->functionIndex = OBJF_SUMMON_RUNE_COLUMN_323;
      obj_s3->d.objf323.gfxIdx = GFX_RUNE_1 + rand() % 10;
      obj_s3->d.objf323.semiTrans = 2;
      obj_s3->state2 = 0xff;
      obj_s3->state3 = 8;
      obj_s3->d.objf323.coords[1].x = obj->x1.n;
      obj_s3->d.objf323.coords[3].x = obj->x1.n;
      obj_s3->d.objf323.coords[1].z = obj->z1.n;
      obj_s3->d.objf323.coords[3].z = obj->z1.n;
      obj_s3->d.objf323.coords[1].y = obj->y1.n;
      obj_s3->d.objf323.coords[3].y = obj->y1.n - CV(0.25);
      a = obj->d.sprite.coords[0].x + obj->d.sprite.coords[0].z;
      obj->d.sprite.coords[0].x = a;
      b = obj->d.sprite.coords[0].y + obj->d.sprite.coords[1].x;
      obj->d.sprite.coords[0].y = b;
      obj->x1.n = obj->x2.n + (a * rcos(b) >> 12);
      obj->z1.n = obj->z2.n + (a * rsin(b) >> 12);
      obj->y1.n = obj->y2.n;
      UpdateObjAnimation(obj);
      obj->d.sprite.boxIdx = 5;
      AddObjPrim6(gGraphicsPtr->ot, obj, 0);
      obj_s3->d.objf323.coords[0].x = obj->x1.n;
      obj_s3->d.objf323.coords[2].x = obj->x1.n;
      obj_s3->d.objf323.coords[0].z = obj->z1.n;
      obj_s3->d.objf323.coords[2].z = obj->z1.n;
      obj_s3->d.objf323.coords[0].y = obj->y1.n;
      obj_s3->d.objf323.coords[2].y = obj->y1.n - CV(0.25);
      obj_s3->x1.n = obj_s3->d.objf323.coords[0].x;
      obj_s3->z1.n = obj_s3->d.objf323.coords[0].z;
      obj_s3->y1.n = obj_s3->d.objf323.coords[0].y;
      if (a < 0x20) {
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
#define OBJF 348
void Objf348_BlueFlameDome_Unused(Object *obj) {
   Object *dataStore;
   Object *sprite;
   Cylinder *dsCylinder;
   s32 i;
   s16 iVar6;
   s16 iVar2;
   s16 uVar1;
   s32 half;
   s16 theta_0xc0;

   gGfxSubTextures[GFX_TILED_FLAMES_DYN_1][1] =
       gGfxSubTextures[GFX_TILED_FLAMES][1] + (obj->mem * 2 % 0x20);

   switch (obj->state) {
   case 0:
      SnapToUnit(obj);
      dataStore = Obj_GetUnused();
      dataStore->functionIndex = OBJF_NOOP;
      OBJ.dataStore = dataStore;
      dsCylinder = &dataStore->d.dataStore.cylinder;
      dsCylinder->bottom.vx = dsCylinder->top.vx = obj->x1.n;
      dsCylinder->bottom.vz = dsCylinder->top.vz = obj->z1.n;
      dsCylinder->bottom.vy = obj->y1.n;
      dsCylinder->top.vy = obj->y1.n + CV(4.0);
      dsCylinder->sideCt = 16;
      dsCylinder->topRadius = CV(1.0);
      dsCylinder->bottomRadius = CV(1.0);
      dsCylinder->gfxIdx = GFX_TILED_FLAMES_DYN_1;
      dsCylinder->semiTrans = 2;
      dsCylinder->useColor = 1;
      dsCylinder->clut = CLUT_BLUES;
      dsCylinder->color.r = 0x80;
      dsCylinder->color.g = 0x80;
      dsCylinder->color.b = 0x80;
      dsCylinder->theta = 0;

      for (i = 0; i < 8; i++) {
         OBJ.targetRadius[i] = (rand() >> 2) % 0x100 + 0x200;
         OBJ.radius[i] = 0;
         OBJ.unused_0x44[i] = (rand() >> 2) % 0x180;
      }

      obj->state++;

   // fallthrough
   case 1:
      dataStore = OBJ.dataStore;
      dsCylinder = &dataStore->d.dataStore.cylinder;
      dsCylinder->theta = obj->mem << 2;
      dsCylinder->color.r = (0x100 - obj->mem) >> 1;
      dsCylinder->color.g = (0x100 - obj->mem) >> 1;
      dsCylinder->color.b = (0x100 - obj->mem) >> 1;

      //@6750
      uVar1 = obj->mem;
      iVar2 = (obj->mem * (0x100 - obj->mem)) >> 4;
      iVar6 = obj->mem;

      half = iVar6 >> 1;
      if (half >= 0x80) {
         half = 0x80;
      }

      for (i = 0; i < 8; i++) {
         dsCylinder->bottom.vy = obj->y1.n + (iVar2 * rsin(i * half) >> 12);
         dsCylinder->top.vy = obj->y1.n + (iVar2 * rsin((i + 1) * half) >> 12);
         dsCylinder->bottomRadius = uVar1 * rcos(i * half) >> 12;
         dsCylinder->topRadius = uVar1 * rcos((i + 1) * half) >> 12;
         RenderCylinder(dsCylinder);
      }

      sprite = Obj_GetUnused();
      sprite->d.sprite.gfxIdx = GFX_TILED_FLAMES;
      sprite->d.sprite.semiTrans = 4;
      theta_0xc0 = 0xc0;

      for (i = 0; i < 8; i++) {
         OBJ.radius[i] += (OBJ.targetRadius[i] - OBJ.radius[i]) >> 3;
         iVar6 = OBJ.radius[i];

         sprite->d.sprite.coords[1].x = sprite->d.sprite.coords[0].x =
             obj->x1.n + (iVar6 * rcos(i * DEG(45)) >> 12);
         sprite->d.sprite.coords[1].z = sprite->d.sprite.coords[0].z =
             obj->z1.n + (iVar6 * rsin(i * DEG(45)) >> 12);
         sprite->d.sprite.coords[1].y = sprite->d.sprite.coords[0].y =
             obj->y1.n + (iVar6 * rsin(theta_0xc0) >> 12);

         iVar6 = uVar1 - 0x80;
         if (iVar6 < 0) {
            iVar6 = 0;
         }

         sprite->d.sprite.coords[3].x = obj->x1.n + (iVar6 * rcos(i * DEG(45)) >> 12);
         sprite->d.sprite.coords[3].z = obj->z1.n + (iVar6 * rsin(i * DEG(45)) >> 12);
         sprite->d.sprite.coords[3].y = obj->y1.n;
         sprite->d.sprite.coords[2].x = obj->x1.n + (uVar1 * rcos(i * DEG(45)) >> 12);
         sprite->d.sprite.coords[2].z = obj->z1.n + (uVar1 * rsin(i * DEG(45)) >> 12);
         sprite->d.sprite.coords[2].y = obj->y1.n;
         AddObjPrim4(gGraphicsPtr->ot, sprite);
      }

      if (++obj->mem >= 0x100) {
         obj->state++;
      }

      break;

   case 2:
      dataStore = OBJ.dataStore;
      dataStore->functionIndex = OBJF_NULL;
      obj->functionIndex = OBJF_NULL;
      gSignal3 = 1;
      break;
   }
}

#undef OBJF
#define OBJF 334
void Objf334_Salamander_FX1(Object *obj) {
   Object *previous;
   Object *current;
   s32 i;

   switch (obj->state) {
   case 0:
      SnapToUnit(obj);
      previous = CreatePositionedObj(obj, OBJF_SALAMANDER_HEAD);
      previous->d.objf335.link = obj;
      previous->d.objf335.parent = obj;

      for (i = 0; i < 32; i++) {
         current = Obj_GetUnused();
         current->functionIndex = OBJF_SALAMANDER_SEGMENT;
         current->x1.n = obj->x1.n;
         current->z1.n = obj->z1.n;
         current->y1.n = obj->y1.n;
         current->d.objf336.link = previous;
         current->d.objf336.theta1 = 0;
         current->d.objf336.theta2 = 0;
         current->d.objf336.radius = 0;
         current->d.objf336.theta5 = i * 0x111;
         current->d.objf336.parent = obj;
         current->mem = 0;
         previous = current;
      }

      OBJ.rockSpawnCounter = 0;
      obj->state++;

   // fallthrough
   case 1:
      obj->mem = 0;
      obj->state++;
      break;

   case 2:
      if (gLightColor.r <= 0xd0) {
         gLightColor.r++;
         // gLightColor.g -= 1; // No match; needs to be decrement op.
         gLightColor.g--;
         gLightColor.b--;
      }
      if (obj->mem == 1) {
         obj->state++;
      }
      break;

   case 3:
      gLightColor.r = (0x80 - gLightColor.r) >> 2;
      gLightColor.g = (0x80 - gLightColor.g) >> 2;
      gLightColor.b = (0x80 - gLightColor.b) >> 2;
      if (++obj->mem >= 24) {
         gLightColor.b = 0x80;
         gLightColor.g = 0x80;
         gLightColor.r = 0x80;
         obj->state3 = 1;
         obj->functionIndex = OBJF_NULL;
         gSignal3 = 1;
      }
      break;
   }
}

#undef OBJF
#define OBJF 335
void Objf335_Salamander_Head(Object *obj) {
   /* GCC 2.6.3 rejects `Quad quad = gSalamanderQuadTemplate;` (invalid initializer for an
    * automatic aggregate from an expression), so the retail initializer copies are written
    * as entry assignments -- same block-copy codegen, template bytes stay address-locked
    * in spells/dark_hurricane.c. */
   SalamanderQuad quad;
   SalamanderHeadGfx headGfx;

   Object *parent;
   Object *sprite;
   s16 unaff_s1;
   s32 i;
   s16 dir;
   s16 theta;

   quad = gSalamanderQuadTemplate;
   headGfx = gSalamanderHeadGfxN;

   parent = OBJ.parent;

   switch (obj->state) {
   case 0:
      sprite = Obj_GetUnused();
      sprite->functionIndex = OBJF_NOOP;
      OBJ.sprite = sprite;
      sprite->d.sprite.gfxIdx = GFX_GLOBE_8;
      sprite->d.sprite.boxIdx = 3;

      obj->x2.n = obj->x1.n;
      obj->y2.n = obj->y1.n;
      obj->z2.n = obj->z1.n;
      obj->y1.n = GetTerrainElevation(obj->z1.s.hi, obj->x1.s.hi) + CV(2.0);
      obj->state3 = 0x50;
      obj->z3.n = 0;
      OBJ.unused_0x4E = 0;
      obj->state++;

   // fallthrough
   case 1:
      sprite = OBJ.sprite;
      OBJ.position4.x = OBJ.position3.x;
      OBJ.position3.x = OBJ.position2.x;
      OBJ.position2.x = OBJ.position1.x;
      OBJ.position1.x = obj->x1.n;
      OBJ.position4.y = OBJ.position3.y;
      OBJ.position3.y = OBJ.position2.y;
      OBJ.position2.y = OBJ.position1.y;
      OBJ.position1.y = obj->y1.n;
      OBJ.position4.z = OBJ.position3.z;
      OBJ.position3.z = OBJ.position2.z;
      OBJ.position2.z = OBJ.position1.z;
      OBJ.position1.z = obj->z1.n;

      switch (obj->z3.n) {
      case 0:
         unaff_s1 = 0x40 + (0x30 * rsin(OBJ.theta1 * 2) >> 12);
         OBJ.theta1 += obj->state3;
         OBJ.theta2 = CV(1.0) * rsin(OBJ.theta1) >> 12;

         if (++obj->y3.n >= 0x100) {
            obj->y3.n = 0;
            obj->x3.n = 0x100;
            gCameraRotation.vy &= 0xfff;
            obj->z3.n++;
         }

         obj->x1.n -= (unaff_s1 * rcos(OBJ.theta1) >> 12);
         obj->z1.n -= (unaff_s1 * rsin(OBJ.theta1) >> 12);
         obj->y1.n -= (unaff_s1 * rsin(OBJ.theta2) >> 12);

         gCameraRotation.vy += 0x10;
         gCameraZoom.vz = 460;
         break;

      case 1:
         OBJ.theta1 = 0;
         OBJ.theta2 += obj->x3.n;
         gCameraRotation.vy += (0 - gCameraRotation.vy) >> 2;
         unaff_s1 = 0xa0;

         if (++obj->y3.n >= 0x40) {
            obj->y3.n = 0;
            obj->z3.n++;
         }

         i = unaff_s1 * rcos(OBJ.theta2) >> 12;
         obj->x1.n -= (i * rcos(OBJ.theta1) >> 12);
         obj->z1.n -= (i * rsin(OBJ.theta1) >> 12);
         obj->y1.n -= (unaff_s1 * rsin(OBJ.theta2) >> 12);
         break;

      case 2:
         obj->y3.n = 0;
         obj->mem = 99;
         parent->mem++;
         obj->state++;
         break;

      case 3:
         OBJ.theta2 = 0;
         unaff_s1 = 0x40;

         if (++obj->y3.n >= 0x80) {
            obj->y3.n = 0;
            obj->z3.n++;
         }

         obj->x1.n -= (unaff_s1 * rcos(OBJ.theta1) >> 12);
         obj->z1.n -= (unaff_s1 * rsin(OBJ.theta1) >> 12);
         obj->y1.n -= (unaff_s1 * rsin(OBJ.theta2) >> 12);
         break;

      case 4:
      default:
         unaff_s1 = 0x40;
         obj->y3.n = 0;
         obj->mem = 99;
         parent->mem++;
         obj->state++;
         break;
      }

      if (OBJ.theta1 >= DEG(360)) {
         obj->state3 = -obj->state3;
      } else if (OBJ.theta1 <= 0) {
         obj->state3 = -obj->state3;
      }

      OBJ.theta4 = OBJ.theta2;
      OBJ.theta3 = OBJ.theta1;
      OBJ.radius = unaff_s1 + 0xa0;
      PanCamera(obj->x1.n, obj->y1.n, obj->z1.n, 2);

      switch (obj->z3.n) {
      case 0:
         dir = (((gCameraRotation.vy - OBJ.theta1) & 0xfff) / DEG(45) - 1) & 7;
         sprite->d.sprite.gfxIdx = headGfx.v[dir];
         sprite->d.sprite.boxIdx = 7;
         sprite->d.sprite.semiTrans = 2;

         for (i = 0; i < 4; i++) {
            gQuad_800fe63c[i] = quad.v[i];
         }

         if (dir > 4) {
            sprite->d.sprite.facingLeft = 1;
         } else {
            sprite->d.sprite.facingLeft = 0;
         }

         break;

      case 1:
      case 2:
         sprite->d.sprite.gfxIdx = GFX_SALAMANDER_E;
         dir = (((gCameraRotation.vy - OBJ.theta1) & 0xfff) / DEG(45) - 1) & 7;

         if (dir > 4) {
            sprite->d.sprite.facingLeft = 1;
         } else {
            sprite->d.sprite.facingLeft = 0;
         }

         sprite->d.sprite.boxIdx = 7;

         theta = OBJ.theta2;
         for (i = 0; i < 4; i++) {
            gQuad_800fe63c[i].vx = (quad.v[i].vx * rcos(theta) - quad.v[i].vy * rsin(theta)) >> 12;
            gQuad_800fe63c[i].vy = (quad.v[i].vx * rsin(theta) + quad.v[i].vy * rcos(theta)) >> 12;
         }
         break;

      case 3:
         break;
      }

      sprite->d.sprite.semiTrans = 2;
      AddObjPrim6(gGraphicsPtr->ot, sprite, 0);
      sprite->d.sprite.semiTrans = 0;
      AddObjPrim6(gGraphicsPtr->ot, sprite, 0);
      sprite->x1.n = obj->x1.n;
      sprite->z1.n = obj->z1.n;
      sprite->y1.n = obj->y1.n;

      break;

   case 2:
      gCameraZoom.vz += (256 - gCameraZoom.vz) >> 2;
      if (++obj->y3.n >= 0x10) {
         sprite = OBJ.sprite;
         sprite->functionIndex = OBJF_NULL;
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 336
void Objf336_Salamander_Segment(Object *obj) {
   static s16 animData[20] = {7, GFX_FLAME_1, 1, GFX_FLAME_2, 1, GFX_FLAME_3, 1, GFX_FLAME_4,
                              1, GFX_FLAME_5, 1, GFX_FLAME_6, 1, GFX_FLAME_7, 1, GFX_FLAME_8,
                              1, GFX_NULL,    1, GFX_NULL};
   static Quad quad = {{-16, -48, 0, 0}, {16, -48, 0, 0}, {-16, 16, 0, 0}, {16, 16, 0, 0}};

   SVECTOR vector_unused;
   /* Retail shared one rodata template between this dead initializer and the live quads
    * in Objf335/377 -- same workaround (see Objf335). */
   SalamanderQuad quad_unused;

   Object *fx1;
   Object *sprite;
   Object *link;
   Object *flamingRock;
   s32 ct;
   s32 i;
   s32 randomAngle;
   s16 theta;
   s32 dx, dy, dz;

   quad_unused = gSalamanderQuadTemplate;

   fx1 = OBJ.parent;

   switch (obj->state) {
   case 0:
      obj->y1.n += CV(2.0);

      sprite = Obj_GetUnused();
      sprite->functionIndex = OBJF_NOOP;
      OBJ.sprite = sprite;
      sprite->d.sprite.gfxIdx = GFX_GLOBE_5;
      sprite->d.sprite.boxIdx = 3;
      sprite->d.sprite.animData = animData;

      ct = (rand() >> 2) % 0x10;
      for (i = 0; i < ct; i++) {
         UpdateObjAnimation(sprite);
      }

      sprite->d.sprite.semiTrans = 2;
      obj->state++;

   // fallthrough
   case 1:
      sprite = OBJ.sprite;
      link = OBJ.link;

      if (obj->mem < 9) {
         obj->mem = link->mem;
      }

      OBJ.position4.x = OBJ.position3.x;
      OBJ.position3.x = OBJ.position2.x;
      OBJ.position2.x = OBJ.position1.x;
      OBJ.position1.x = obj->x1.n;
      OBJ.position4.y = OBJ.position3.y;
      OBJ.position3.y = OBJ.position2.y;
      OBJ.position2.y = OBJ.position1.y;
      OBJ.position1.y = obj->y1.n;
      OBJ.position4.z = OBJ.position3.z;
      OBJ.position3.z = OBJ.position2.z;
      OBJ.position2.z = OBJ.position1.z;
      OBJ.position1.z = obj->z1.n;

      OBJ.radius = 0x40 + (0x20 * rsin(OBJ.theta5) >> 12);
      OBJ.theta5 = (OBJ.theta5 + 0x40) & 0xfff;
      obj->x1.n = link->d.objf336.position1.x;
      obj->z1.n = link->d.objf336.position1.z;
      obj->y1.n = link->d.objf336.position1.y;
      OBJ.radius = link->d.objf336.radius;
      OBJ.theta2 = 0;
      OBJ.theta4 = link->d.objf336.theta4;
      OBJ.theta3 = link->d.objf336.theta3 + OBJ.theta1;
      vector_unused.vy = OBJ.radius * rsin(OBJ.theta4) >> 12;
      sprite->x1.n = obj->x1.n;
      sprite->z1.n = obj->z1.n;
      sprite->y1.n = obj->y1.n;

      if (obj->mem != 9) {
         dx = link->x1.n - obj->x1.n;
         dz = link->z1.n - obj->z1.n;
         dy = link->y1.n - obj->y1.n;
         theta = ratan2(dy, SquareRoot0(dx * dx + dz * dz));
         for (i = 0; i < 4; i++) {
            gQuad_800fe63c[i].vx = (quad[i].vx * rcos(theta) - quad[i].vy * rsin(theta)) >> 12;
            gQuad_800fe63c[i].vy = (quad[i].vx * rsin(theta) + quad[i].vy * rcos(theta)) >> 12;
         }
         UpdateObjAnimation(sprite);
      }

      AddObjPrim6(gGraphicsPtr->ot, sprite, 0);

      if (obj->mem == 99) {
         if (fx1->d.objf334.rockSpawnCounter < 2) {
            fx1->d.objf334.rockSpawnCounter++;
         } else {
            if (Obj_CountUnused() > 100) {
               randomAngle = rand() % DEG(360);
               flamingRock = CreatePositionedObj(obj, OBJF_FLAMING_ROCK);
               flamingRock->y2.n = (rand() & 0x3f) + 0x10;
               flamingRock->x2.n = (0x40 * rsin(randomAngle) >> 12);
               flamingRock->z2.n = (0x40 * rcos(randomAngle) >> 12);
               flamingRock->y3.n = -12;
               fx1->d.objf334.rockSpawnCounter = 0;
            }
         }
         obj->state++;
      }

      break;

   case 2:
      sprite = OBJ.sprite;
      sprite->functionIndex = OBJF_NULL;
      obj->functionIndex = OBJF_NULL;
      break;
   }
}

#undef OBJF
#define OBJF 377
/* An objf335/336-compatible Salamander chain link (identical struct layout + chaining
 * math) drawing the directional head; when its link reports mem == 2 it emits Flame
 * Breath particles -- but no shipped head handler ever writes mem = 2, and nothing
 * spawns 377: a cut breath-phase variant of the Salamander. */
void Objf377_SalamanderBreathHead_Unused(Object *obj) {
   /* Same GCC 2.6.3 workaround as Objf335: entry assignments in the retail initializer
    * order (headGfx first, then quad). */
   SVECTOR vector;
   SalamanderHeadGfx headGfx;
   SalamanderQuad quad;

   Object *sprite;
   Object *obj_s4; //? objf unknown - treating as objf335
   Object *obj_s1;
   s16 dir;
   s32 i;
   s32 iVar3, x, z;

   headGfx = gSalamanderHeadGfxS;
   quad = gSalamanderQuadTemplate;

   switch (obj->state) {
   case 0:
      sprite = Obj_GetUnused();
      sprite->functionIndex = OBJF_NOOP;
      OBJ.sprite = sprite;
      sprite->d.sprite.gfxIdx = GFX_GLOBE_5;
      sprite->d.sprite.boxIdx = 3;
      obj->state++;

   // fallthrough
   case 1:
      sprite = OBJ.sprite;
      obj_s4 = OBJ.link;

      dir = (((gCameraRotation.vy - OBJ.theta3) & 0xfff) / DEG(45)) & 7;
      sprite->d.sprite.gfxIdx = headGfx.v[dir];

      if (dir > 4) {
         sprite->d.sprite.facingLeft = 1;
      } else {
         sprite->d.sprite.facingLeft = 0;
      }

      if (obj->mem == 0) {
         if (obj_s4->mem != 2) {
            sprite->d.sprite.boxIdx = 7;
            for (i = 0; i < 4; i++) {
               gQuad_800fe63c[i] = quad.v[i];
            }
            OBJ.radius = CV(0.5) + (CV(0.125) * rsin(OBJ.theta5) >> 12);
            OBJ.theta5 = (OBJ.theta5 + DEG(5.625)) & 0xfff;
            OBJ.theta2 = obj_s4->d.objf335.theta2;
            gCameraZoom.vz = 512;
            PanCamera(obj->x1.n, obj->y1.n, obj->z1.n, 2);
            gCameraRotation.vy += (16 * rsin(OBJ.theta4 >> 1) >> 12);
         } else {
            obj_s1 = CreatePositionedObj(obj, OBJF_FLAME_BREATH_PARTICLE);
            obj_s1->d.sprite.boxIdx = 3;
            obj_s1->x2.n = CV(0.28125) * rcos(OBJ.theta3) >> 12;
            obj_s1->z2.n = CV(0.28125) * rsin(OBJ.theta3) >> 12;
            obj_s1->x2.n += rand() % 13 - 6;
            obj_s1->z2.n += rand() % 13 - 6;
            obj_s1->y2.n = CV(0.15625) * rsin(DEG(67.5) * rsin(obj->state2) >> 12) >> 12;
            obj_s1->x3.n = obj_s1->x2.n >> 2;
            obj_s1->z3.n = obj_s1->z2.n >> 2;
            obj_s1->y3.n = obj_s1->y2.n >> 2;
            obj->state2 += 0x20;
            gCameraRotation.vy += DEG(1.40625) * rsin(OBJ.theta4 >> 1) >> 12;
            PanCamera(obj->x1.n, obj->y1.n, obj->z1.n, 2);
            gCameraZoom.vz = 512;
         }
      }

      OBJ.theta3 = obj_s4->d.objf335.theta3 + OBJ.theta1;
      OBJ.theta4 = obj_s4->d.objf335.theta4 + OBJ.theta2;

      iVar3 = OBJ.radius * rsin(OBJ.theta4) >> 12;
      vector.vy = iVar3;
      iVar3 = OBJ.radius * rcos(OBJ.theta4) >> 12;
      x = iVar3 * rcos(OBJ.theta3) >> 12;
      z = iVar3 * rsin(OBJ.theta3) >> 12;

      vector.vx = x;
      vector.vz = z;

      obj->x1.n = obj_s4->x1.n + (vector.vx);
      obj->z1.n = obj_s4->z1.n + (vector.vz);
      obj->y1.n = obj_s4->y1.n + (vector.vy);

      sprite->x1.n = obj->x1.n;
      sprite->z1.n = obj->z1.n;
      sprite->y1.n = obj->y1.n;
      AddObjPrim6(gGraphicsPtr->ot, sprite, 0);
      break;

   case 2:
      sprite = OBJ.sprite;
      sprite->functionIndex = OBJF_NULL;
      obj->functionIndex = OBJF_NULL;
      break;
   }
}

#undef OBJF
#define OBJF 331
void Objf331_RedXMark_Unused(Object *obj) {
   Object *parent = OBJ.parent;

   switch (obj->state) {
   case 0:
      OBJ.gfxIdx = GFX_RED_X_BTM_RIGHT;
      OBJ.boxIdx = 4;
      obj->state++;

   // fallthrough
   case 1:
      if (obj->state2 == 1) {
         obj->y1.n = parent->y1.n + (CV(1.5) * rsin(DEG(-45)) >> 12);
         obj->x1.n = parent->x1.n + CV(0.5);
         obj->z1.n = parent->z1.n;
      } else {
         obj->y1.n = parent->y1.n + (CV(1.5) * rsin(DEG(-135)) >> 12);
         obj->x1.n = parent->x1.n - CV(0.5);
         obj->z1.n = parent->z1.n;
      }

      AddObjPrim6(gGraphicsPtr->ot, obj, 0);
      break;
   }
}

#undef OBJF
#define OBJF 747
void Objf747_748_Wyrmfang_Flames(Object *obj) {
   Object *flame;

   if (obj->functionIndex == OBJF_WYRMFANG_FLAMES_CW) {
      OBJ.theta += DEG(8.4375);
      OBJ.radius += 8;
   } else {
      OBJ.theta -= DEG(8.4375);
      OBJ.radius += 8;
   }
   if (OBJ.radius > CV(6.0)) {
      OBJ.radius = 0;
   }

   OBJ.amplitudeTheta = (OBJ.amplitudeTheta + 280) & 0x7ff;
   OBJ.amplitude = 1200 * rcos((OBJ.amplitudeTheta - DEG(90)) & 0xfff) >> 12;

   switch (obj->state) {
   case 0:
      OBJ.timer = 1;
      if (obj->functionIndex == OBJF_WYRMFANG_FLAMES_CW) {
         OBJ.radius = 0;
      } else {
         OBJ.radius = CV(2.0);
      }
      obj->state++;

   // fallthrough
   case 1:
      if (--OBJ.timer == 0) {
         OBJ.timer = 1;
         flame = CreatePositionedObj(obj, OBJF_WYRMFANG_FLAME);
         flame->d.objf749.theta = OBJ.theta;
         flame->d.objf749.radius = OBJ.radius;
         flame->d.objf749.amplitude = OBJ.amplitude;
      }
      break;
   }
}

// Used by: Objf332_RollingFire_FX1, Objf749_Wyrmfang_Flame
s16 gFlameAnimData_800ff8a4[20] = {0, GFX_FLAME_1, 2, GFX_FLAME_2, 2, GFX_FLAME_3, 2, GFX_FLAME_4,
                                   2, GFX_FLAME_5, 2, GFX_FLAME_6, 2, GFX_FLAME_7, 2, GFX_FLAME_8,
                                   2, GFX_NULL,    1, GFX_NULL};

#undef OBJF
#define OBJF 749
void Objf749_Wyrmfang_Flame(Object *obj) {
   Object *sprite;

   sprite = OBJ.sprite;
   if (sprite != NULL) {
      UpdateObjAnimation(sprite);
      AddObjPrim3(gGraphicsPtr->ot, sprite);
   }

   switch (obj->state) {
   case 0:
      sprite = Obj_GetUnused();
      sprite->functionIndex = OBJF_NOOP;
      sprite->d.sprite.animData = gFlameAnimData_800ff8a4;
      sprite->d.sprite.semiTrans = 1;
      OBJ.sprite = sprite;
      sprite->d.sprite.coords[0].x =
          obj->x1.n + (rcos((OBJ.theta - DEG(11.25)) & 0xfff) * OBJ.radius >> 12);
      sprite->d.sprite.coords[0].z =
          obj->z1.n + (rcos((OBJ.theta + DEG(78.75)) & 0xfff) * OBJ.radius >> 12);
      sprite->d.sprite.coords[2].x = sprite->d.sprite.coords[0].x;
      sprite->d.sprite.coords[2].z = sprite->d.sprite.coords[0].z;
      sprite->d.sprite.coords[1].x =
          obj->x1.n + (rcos((OBJ.theta + DEG(11.25)) & 0xfff) * OBJ.radius >> 12);
      sprite->d.sprite.coords[1].z =
          obj->z1.n + (rcos((OBJ.theta + DEG(101.25)) & 0xfff) * OBJ.radius >> 12);
      sprite->d.sprite.coords[3].x = sprite->d.sprite.coords[1].x;
      sprite->d.sprite.coords[3].z = sprite->d.sprite.coords[1].z;
      sprite->d.sprite.coords[0].y = obj->y1.n;
      sprite->d.sprite.coords[1].y = obj->y1.n;
      sprite->d.sprite.coords[2].y = obj->y1.n;
      sprite->d.sprite.coords[3].y = obj->y1.n;
      sprite->x1.n = sprite->d.sprite.coords[2].x;
      sprite->z1.n = sprite->d.sprite.coords[2].z;
      sprite->y1.n = sprite->d.sprite.coords[2].y;
      obj->state++;

   // fallthrough
   case 1:
      OBJ.yTheta += DEG(4.21875);
      if (OBJ.yTheta > DEG(180)) {
         OBJ.yTheta = 0;
         obj->state++;
      }
      sprite->d.sprite.coords[0].y =
          obj->y1.n + (rcos((OBJ.yTheta - DEG(90)) & 0xfff) * OBJ.amplitude >> 12);
      sprite->d.sprite.coords[1].y = sprite->d.sprite.coords[0].y;
      break;

   case 2:
      obj->functionIndex = OBJF_NULL;
      sprite->functionIndex = OBJF_NULL;
      break;
   }
}
