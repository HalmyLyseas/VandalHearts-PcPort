/* Fire Gem's beam effect (Objf173; the FX1 driver is in spells/casting_main.c).
 * Dispatched data-driven via gSpellsEx (see spells/casting_main.c's header for the
 * model). */
#include "common.h"
#include "object.h"
#include "graphics.h"
#include "battle.h"
#include "field.h"


#undef OBJF
#define OBJF 173
void Objf173_FireGem_Beam(Object *obj) {
   s16 x, y, z;
   s16 a;
   s32 i;
   Object *obj_s2;
   Object *obj_v1;
   SVECTOR positions[6];
   s16 local_38[6][2] = {{0, 1}, {3, 4}, {0, 2}, {3, 5}, {2, 1}, {5, 4}};

   x = obj->x1.n;
   y = obj->y1.n;
   z = obj->z1.n;

   switch (obj->state) {
   case 0:
      OBJ.ringY = 0x800;
      obj->state++;

   // fallthrough
   case 1:
      obj_s2 = Obj_GetUnused();
      obj_s2->functionIndex = OBJF_NOOP;
      obj_s2->d.sprite.gfxIdx = GFX_COLOR_1 + OBJ.colorIdx;
      obj_s2->d.sprite.semiTrans = 1;

      a = rsin(OBJ.spinPhase);
      positions[0].vx = CV(1.25) * rcos(a) >> 12;
      positions[0].vz = CV(1.25) * rsin(a) >> 12;
      positions[1].vx = CV(1.25) * rcos(a + 0x555) >> 12;
      positions[1].vz = CV(1.25) * rsin(a + 0x555) >> 12;
      positions[2].vx = CV(1.25) * rcos(a + 0xaaa) >> 12;
      positions[2].vz = CV(1.25) * rsin(a + 0xaaa) >> 12;
      positions[3].vx = CV(1.25) * rcos(a + 0x2ab) >> 12;
      positions[3].vz = CV(1.25) * rsin(a + 0x2ab) >> 12;
      positions[4].vx = CV(1.25) * rcos(a + 0x7ff) >> 12;
      positions[4].vz = CV(1.25) * rsin(a + 0x7ff) >> 12;
      positions[5].vx = CV(1.25) * rcos(a + 0xd55) >> 12;
      positions[5].vz = CV(1.25) * rsin(a + 0xd55) >> 12;
      OBJ.spinPhase += 0x20;

      obj_s2->d.sprite.coords[0].x = obj_s2->d.sprite.coords[1].x = x + positions[0].vx;
      obj_s2->d.sprite.coords[0].z = obj_s2->d.sprite.coords[1].z = z + positions[0].vz;
      // y += OBJ.ringY;
      obj_s2->d.sprite.coords[0].y = obj_s2->d.sprite.coords[1].y = obj_s2->d.sprite.coords[2].y =
          obj_s2->d.sprite.coords[3].y = (y + OBJ.ringY);
      obj_s2->d.sprite.coords[2].x = x + positions[1].vx;
      obj_s2->d.sprite.coords[2].z = z + positions[1].vz;
      obj_s2->d.sprite.coords[3].x = x + positions[2].vx;
      obj_s2->d.sprite.coords[3].z = z + positions[2].vz;
      AddObjPrim4(gGraphicsPtr->ot, obj_s2);

      obj_s2->d.sprite.coords[0].x = obj_s2->d.sprite.coords[1].x = x + positions[3].vx;
      obj_s2->d.sprite.coords[0].z = obj_s2->d.sprite.coords[1].z = z + positions[3].vz;
      obj_s2->d.sprite.coords[2].x = x + positions[4].vx;
      obj_s2->d.sprite.coords[2].z = z + positions[4].vz;
      obj_s2->d.sprite.coords[3].x = x + positions[5].vx;
      obj_s2->d.sprite.coords[3].z = z + positions[5].vz;
      AddObjPrim4(gGraphicsPtr->ot, obj_s2);

      if ((OBJ.colorDescending == 0) && (++OBJ.colorTimer % 3 == 0)) {
         OBJ.colorIdx++;
         if (OBJ.colorIdx == 7) {
            OBJ.colorDescending++;
            OBJ.colorIdx = 6;
         }
      } else if ((OBJ.colorTimer % 3 == 0) && (--OBJ.colorIdx < 0)) {
         OBJ.colorDescending = 0;
         OBJ.colorIdx = 0;
      }

      OBJ.ringY -= 0x40;
      if (OBJ.ringY < 0) {
         OBJ.ringY = 0;
         obj_v1 = Obj_GetUnused();
         obj_v1->functionIndex = OBJF_FLASHING_UNIT_SPRITE;
         obj_v1->x1.n = obj->x1.n;
         obj_v1->z1.n = obj->z1.n;
         OBJ.fx = obj_v1;
         obj->state++;
      }

      obj_s2->functionIndex = OBJF_NULL;
      break;

   case 2:
      obj_s2 = Obj_GetUnused();
      obj_s2->functionIndex = OBJF_NOOP;
      obj_s2->d.sprite.gfxIdx = GFX_COLOR_1 + OBJ.colorIdx;
      obj_s2->d.sprite.semiTrans = 1;

      a = rsin(OBJ.spinPhase);
      positions[0].vx = CV(1.25) * rcos(a) >> 12;
      positions[0].vz = CV(1.25) * rsin(a) >> 12;
      positions[1].vx = CV(1.25) * rcos(a + 0x555) >> 12;
      positions[1].vz = CV(1.25) * rsin(a + 0x555) >> 12;
      positions[2].vx = CV(1.25) * rcos(a + 0xaaa) >> 12;
      positions[2].vz = CV(1.25) * rsin(a + 0xaaa) >> 12;
      positions[3].vx = CV(1.25) * rcos(a + 0x2ab) >> 12;
      positions[3].vz = CV(1.25) * rsin(a + 0x2ab) >> 12;
      positions[4].vx = CV(1.25) * rcos(a + 0x7ff) >> 12;
      positions[4].vz = CV(1.25) * rsin(a + 0x7ff) >> 12;
      positions[5].vx = CV(1.25) * rcos(a + 0xd55) >> 12;
      positions[5].vz = CV(1.25) * rsin(a + 0xd55) >> 12;
      OBJ.spinPhase += 0x20;

      for (i = 0; i < 6; i++) {
         obj_s2->d.sprite.coords[0].x = x;
         obj_s2->d.sprite.coords[0].z = z;
         obj_s2->d.sprite.coords[0].y = obj->y1.n + OBJ.beamHeight;
         obj_s2->d.sprite.coords[1].x = x;
         obj_s2->d.sprite.coords[1].z = z;
         obj_s2->d.sprite.coords[1].y = obj->y1.n + OBJ.beamHeight;
         obj_s2->d.sprite.coords[2].x = x + positions[local_38[i][0]].vx;
         obj_s2->d.sprite.coords[2].z = z + positions[local_38[i][0]].vz;
         obj_s2->d.sprite.coords[2].y = obj->y1.n + OBJ.ringY;
         obj_s2->d.sprite.coords[3].x = x + positions[local_38[i][1]].vx;
         obj_s2->d.sprite.coords[3].z = z + positions[local_38[i][1]].vz;
         obj_s2->d.sprite.coords[3].y = obj->y1.n + OBJ.ringY;

         obj_s2->x1.n = (obj_s2->d.sprite.coords[0].x + obj_s2->d.sprite.coords[1].x +
                         obj_s2->d.sprite.coords[2].x + obj_s2->d.sprite.coords[3].x) >>
                        2;
         obj_s2->z1.n = (obj_s2->d.sprite.coords[0].z + obj_s2->d.sprite.coords[1].z +
                         obj_s2->d.sprite.coords[2].z + obj_s2->d.sprite.coords[3].z) >>
                        2;
         obj_s2->y1.n = obj_s2->d.sprite.coords[3].y;

         AddObjPrim3(gGraphicsPtr->ot, obj_s2);
         if ((i + 1) % 2 != 0) {
            obj_s2->d.sprite.gfxIdx++;
         }
      }

      obj_s2->d.sprite.coords[0].x = x + positions[0].vx;
      obj_s2->d.sprite.coords[0].z = z + positions[0].vz;
      obj_s2->d.sprite.coords[0].y = obj->y1.n + OBJ.ringY;
      obj_s2->d.sprite.coords[1].x = x + positions[0].vx;
      obj_s2->d.sprite.coords[1].z = z + positions[0].vz;
      obj_s2->d.sprite.coords[1].y = obj->y1.n + OBJ.ringY;
      obj_s2->d.sprite.coords[2].x = x + positions[1].vx;
      obj_s2->d.sprite.coords[2].z = z + positions[1].vz;
      obj_s2->d.sprite.coords[2].y = obj->y1.n + OBJ.ringY;
      obj_s2->d.sprite.coords[3].x = x + positions[2].vx;
      obj_s2->d.sprite.coords[3].z = z + positions[2].vz;
      obj_s2->d.sprite.coords[3].y = obj->y1.n + OBJ.ringY;
      AddObjPrim4(gGraphicsPtr->ot, obj_s2);

      obj_s2->d.sprite.coords[0].x = x + positions[3].vx;
      obj_s2->d.sprite.coords[0].z = z + positions[3].vz;
      obj_s2->d.sprite.coords[0].y = obj->y1.n + OBJ.ringY;
      obj_s2->d.sprite.coords[1].x = x + positions[3].vx;
      obj_s2->d.sprite.coords[1].z = z + positions[3].vz;
      obj_s2->d.sprite.coords[1].y = obj->y1.n + OBJ.ringY;
      obj_s2->d.sprite.coords[2].x = x + positions[4].vx;
      obj_s2->d.sprite.coords[2].z = z + positions[4].vz;
      obj_s2->d.sprite.coords[2].y = obj->y1.n + OBJ.ringY;
      obj_s2->d.sprite.coords[3].x = x + positions[5].vx;
      obj_s2->d.sprite.coords[3].z = z + positions[5].vz;
      obj_s2->d.sprite.coords[3].y = obj->y1.n + OBJ.ringY;
      AddObjPrim4(gGraphicsPtr->ot, obj_s2);

      obj_s2->functionIndex = OBJF_NULL;

      switch (obj->state3) {
      case 0:
         if ((OBJ.colorDescending == 0) && (++OBJ.colorTimer % 3 == 0)) {
            OBJ.colorIdx++;
            if (OBJ.colorIdx == 7) {
               OBJ.colorDescending++;
               OBJ.colorIdx = 6;
            }
         } else if ((OBJ.colorTimer % 3 == 0) && (--OBJ.colorIdx < 0)) {
            OBJ.colorDescending = 0;
            OBJ.colorIdx = 0;
         }
         OBJ.beamHeight += CV(0.75);
         if (OBJ.beamHeight > CV(8.0)) {
            OBJ.beamHeight = CV(8.0);
            obj->state3++;
         }
         break;

      case 1:
         if ((OBJ.colorDescending == 0) && (++OBJ.colorTimer % 3 == 0)) {
            OBJ.colorIdx++;
            if (OBJ.colorIdx == 7) {
               OBJ.colorDescending++;
               OBJ.colorIdx = 6;
            }
         } else if ((OBJ.colorTimer % 3 == 0) && (--OBJ.colorIdx < 0)) {
            OBJ.colorDescending = 0;
            OBJ.colorIdx = 0;
            OBJ.phase++;
         }
         if (OBJ.phase == 2) {
            obj->state3++;
            OBJ.beamHeight = 0;
         }
         break;

      case 2:
         OBJ.beamHeight = CV(8.0) * (ONE - rsin(OBJ.phase)) >> 12;
         if ((OBJ.colorDescending == 0) && (++OBJ.colorTimer % 3 == 0)) {
            OBJ.colorIdx++;
            if (OBJ.colorIdx == 7) {
               OBJ.colorDescending++;
               OBJ.colorIdx = 6;
            }
         } else if ((OBJ.colorTimer % 3 == 0) && (--OBJ.colorIdx < 0)) {
            OBJ.colorDescending = 0;
            OBJ.colorIdx = 0;
         }
         OBJ.phase += 0x20;
         if (OBJ.phase > 0x400) {
            obj->functionIndex = OBJF_NULL;
            obj_v1 = OBJ.fx;
            obj_v1->state = 99;
         }
         break;
      }

      break;
   }
}

