/* Perfect Guard's target effect, Objf192_PerfectGuard_FX2: the rotating cube of faces
 * around the shielded unit (s_faces_80123a4c holds the six face templates). Dispatched
 * via gSpellsEx (model described in spells/casting_main.c's header). */
#include "common.h"
#include "object.h"
#include "graphics.h"
#include "battle.h"

#undef OBJF
#define OBJF 192
void Objf192_PerfectGuard_FX2(Object *obj) {
   extern SVECTOR s_faces_80123a4c[6][4];

   Object *targetSprite;
   Object *obj_s2;
   POLY_FT4 *poly;
   s32 i, j;
   s32 flag;
   s32 fade;
   s16 xOfs, zOfs;
   s16 a;
   MATRIX matrix;
   VECTOR local_1c0[6][4];
   SVECTOR local_40;

   switch (obj->state) {
   case 0:
      targetSprite = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
      obj->x1.n = targetSprite->x1.n;
      obj->y1.n = targetSprite->y1.n + CV(0.5);
      obj->z1.n = targetSprite->z1.n;

      for (i = 0; i < 6; i++) {
         s_faces_80123a4c[i][0].vx = -0xa0;
         s_faces_80123a4c[i][0].vy = -0xa0;
         s_faces_80123a4c[i][0].vz = -0xa0;
         s_faces_80123a4c[i][1].vx = 0xa0;
         s_faces_80123a4c[i][1].vy = -0xa0;
         s_faces_80123a4c[i][1].vz = -0xa0;
         s_faces_80123a4c[i][2].vx = -0xa0;
         s_faces_80123a4c[i][2].vy = -0xa0;
         s_faces_80123a4c[i][2].vz = 0xa0;
         s_faces_80123a4c[i][3].vx = 0xa0;
         s_faces_80123a4c[i][3].vy = -0xa0;
         s_faces_80123a4c[i][3].vz = 0xa0;
      }

      obj->state++;

   // fallthrough
   case 1:
      obj_s2 = Obj_GetUnused();
      obj_s2->functionIndex = OBJF_NOOP;
      obj_s2->d.sprite.semiTrans = 2;

      gGfxSubTextures[GFX_TILED_DIAMONDS_DYN_1][0] = OBJ.uvScroll % 0x20;
      gGfxSubTextures[GFX_TILED_DIAMONDS_DYN_1][1] = 0x80 + OBJ.uvScroll % 0x20;
      gGfxSubTextures[GFX_TILED_DIAMONDS_DYN_1][2] = 0x20;
      gGfxSubTextures[GFX_TILED_DIAMONDS_DYN_1][3] = 0x20;
      OBJ.uvScroll++;

      // 1 //
      switch (obj->state3) {
      case 1:

         switch (obj->state2) {
         case 0:
            s_faces_80123a4c[1][0].vx = 0x140 * rsin(OBJ.theta) / ONE - 0xa0;
            s_faces_80123a4c[1][0].vy = 0x140 * rsin(OBJ.theta) / ONE - 0xa0;
            s_faces_80123a4c[1][2].vx = 0x140 * rsin(OBJ.theta) / ONE - 0xa0;
            s_faces_80123a4c[1][2].vy = 0x140 * rsin(OBJ.theta) / ONE - 0xa0;

            OBJ.theta += 0x80;
            if (OBJ.theta > 0x400) {
               OBJ.theta = 0;
               obj->state2++;
            }
            break;

         case 1:
            s_faces_80123a4c[2][0].vz = 0x140 * rsin(OBJ.theta) / ONE - 0xa0;
            s_faces_80123a4c[2][0].vy = 0x140 * rsin(OBJ.theta) / ONE - 0xa0;
            s_faces_80123a4c[2][1].vz = 0x140 * rsin(OBJ.theta) / ONE - 0xa0;
            s_faces_80123a4c[2][1].vy = 0x140 * rsin(OBJ.theta) / ONE - 0xa0;

            OBJ.theta += 0x80;
            if (OBJ.theta > 0x400) {
               OBJ.theta = 0;
               obj->state2++;
            }
            break;

         case 2:
            s_faces_80123a4c[3][1].vx = 0xa0 - 0x140 * rsin(OBJ.theta) / ONE;
            s_faces_80123a4c[3][1].vy = 0x140 * rsin(OBJ.theta) / ONE - 0xa0;
            s_faces_80123a4c[3][3].vx = 0xa0 - 0x140 * rsin(OBJ.theta) / ONE;
            s_faces_80123a4c[3][3].vy = 0x140 * rsin(OBJ.theta) / ONE - 0xa0;

            OBJ.theta += 0x80;
            if (OBJ.theta > 0x400) {
               OBJ.theta = 0;
               obj->state2++;
            }
            break;

         case 3:
            s_faces_80123a4c[4][2].vz = 0xa0 - 0x140 * rsin(OBJ.theta) / ONE;
            s_faces_80123a4c[4][2].vy = 0x140 * rsin(OBJ.theta) / ONE - 0xa0;
            s_faces_80123a4c[4][3].vz = 0xa0 - 0x140 * rsin(OBJ.theta) / ONE;
            s_faces_80123a4c[4][3].vy = 0x140 * rsin(OBJ.theta) / ONE - 0xa0;

            s_faces_80123a4c[5][2].vz = 0xa0 - 0x140 * rsin(OBJ.theta) / ONE;
            s_faces_80123a4c[5][2].vy = 0x140 * rsin(OBJ.theta) / ONE - 0xa0;
            s_faces_80123a4c[5][3].vz = 0xa0 - 0x140 * rsin(OBJ.theta) / ONE;
            s_faces_80123a4c[5][3].vy = 0x140 * rsin(OBJ.theta) / ONE - 0xa0;

            OBJ.theta += 0x80;
            if (OBJ.theta > 0x400) {
               OBJ.theta = 0;
               obj->state2++;
            }
            break;

         case 4:
            s_faces_80123a4c[5][0].vz = 0x140 * rsin(OBJ.theta) / ONE - 0xa0;
            s_faces_80123a4c[5][0].vy = 0x140 * rsin(OBJ.theta) / ONE - 0xa0;
            s_faces_80123a4c[5][1].vz = 0x140 * rsin(OBJ.theta) / ONE - 0xa0;
            s_faces_80123a4c[5][1].vy = 0x140 * rsin(OBJ.theta) / ONE - 0xa0;

            OBJ.theta += 0x80;
            if (OBJ.theta > 0x400) {
               OBJ.theta = 0;
               obj->state2 = 0;
               OBJ.openFinished++;
            }
            break;

         case 5:
            break;
         }

         break;

      case 2:
         PushMatrix();
         local_40.vx = 0;
         local_40.vz = 0;
         local_40.vy = rcos(OBJ.spinTimer * 0x40);
         RotMatrix(&local_40, &matrix);
         matrix.t[0] = 0;
         matrix.t[1] = 0;
         matrix.t[2] = 0;
         SetRotMatrix(&matrix);
         SetTransMatrix(&matrix);
         for (i = 0; i < 6; i++) {
            for (j = 0; j < 4; j++) {
               RotTrans(&s_faces_80123a4c[i][j], &local_1c0[i][j], &flag);
            }
         }
         PopMatrix();
         break;

      case 3:

         switch (obj->state2) {
         case 0:
            s_faces_80123a4c[5][0].vz = 0x140 * rsin(OBJ.theta) / ONE - 0xa0;
            s_faces_80123a4c[5][0].vy = 0x140 * rsin(OBJ.theta) / ONE - 0xa0;
            s_faces_80123a4c[5][1].vz = 0x140 * rsin(OBJ.theta) / ONE - 0xa0;
            s_faces_80123a4c[5][1].vy = 0x140 * rsin(OBJ.theta) / ONE - 0xa0;

            OBJ.theta -= 0x80;
            if (OBJ.theta < 0) {
               OBJ.theta = 0x400;
               obj->state2++;
            }
            break;

         case 1:
            s_faces_80123a4c[4][2].vz = 0xa0 - 0x140 * rsin(OBJ.theta) / ONE;
            s_faces_80123a4c[4][2].vy = 0x140 * rsin(OBJ.theta) / ONE - 0xa0;
            s_faces_80123a4c[4][3].vz = 0xa0 - 0x140 * rsin(OBJ.theta) / ONE;
            s_faces_80123a4c[4][3].vy = 0x140 * rsin(OBJ.theta) / ONE - 0xa0;

            s_faces_80123a4c[5][2].vz = 0xa0 - 0x140 * rsin(OBJ.theta) / ONE;
            s_faces_80123a4c[5][2].vy = 0x140 * rsin(OBJ.theta) / ONE - 0xa0;
            s_faces_80123a4c[5][3].vz = 0xa0 - 0x140 * rsin(OBJ.theta) / ONE;
            s_faces_80123a4c[5][3].vy = 0x140 * rsin(OBJ.theta) / ONE - 0xa0;

            OBJ.theta -= 0x80;
            if (OBJ.theta < 0) {
               OBJ.theta = 0x400;
               obj->state2++;
            }
            break;

         case 2:
            s_faces_80123a4c[3][1].vx = 0xa0 - 0x140 * rsin(OBJ.theta) / ONE;
            s_faces_80123a4c[3][1].vy = 0x140 * rsin(OBJ.theta) / ONE - 0xa0;
            s_faces_80123a4c[3][3].vx = 0xa0 - 0x140 * rsin(OBJ.theta) / ONE;
            s_faces_80123a4c[3][3].vy = 0x140 * rsin(OBJ.theta) / ONE - 0xa0;

            OBJ.theta -= 0x80;
            if (OBJ.theta < 0) {
               OBJ.theta = 0x400;
               obj->state2++;
            }
            break;

         case 3:
            s_faces_80123a4c[2][0].vz = 0x140 * rsin(OBJ.theta) / ONE - 0xa0;
            s_faces_80123a4c[2][0].vy = 0x140 * rsin(OBJ.theta) / ONE - 0xa0;
            s_faces_80123a4c[2][1].vz = 0x140 * rsin(OBJ.theta) / ONE - 0xa0;
            s_faces_80123a4c[2][1].vy = 0x140 * rsin(OBJ.theta) / ONE - 0xa0;

            OBJ.theta -= 0x80;
            if (OBJ.theta < 0) {
               OBJ.theta = 0x400;
               obj->state2++;
            }
            break;

         case 4:
            s_faces_80123a4c[1][0].vx = 0x140 * rsin(OBJ.theta) / ONE - 0xa0;
            s_faces_80123a4c[1][0].vy = 0x140 * rsin(OBJ.theta) / ONE - 0xa0;
            s_faces_80123a4c[1][2].vx = 0x140 * rsin(OBJ.theta) / ONE - 0xa0;
            s_faces_80123a4c[1][2].vy = 0x140 * rsin(OBJ.theta) / ONE - 0xa0;

            OBJ.theta -= 0x80;
            if (OBJ.theta < 0) {
               OBJ.theta = 0x400;
               OBJ.closeFinished++;
            }
            break;

         case 5:
            break;
         }

         break;

      case 0:
      case 4:
         a = 0x400 * (ONE - rsin(OBJ.theta)) / ONE;
         xOfs = a * rsin(OBJ.theta * 8) / ONE;
         zOfs = a * rcos(OBJ.theta * 8) / ONE;
         break;
      }

      // 2 //
      switch (obj->state3) {
      case 0:
         for (i = 0; i < 6; i++) {
            obj_s2->d.sprite.coords[0].x = obj->x1.n + s_faces_80123a4c[i][0].vx + xOfs;
            obj_s2->d.sprite.coords[0].y = obj->y1.n + s_faces_80123a4c[i][0].vy;
            obj_s2->d.sprite.coords[0].z = obj->z1.n + s_faces_80123a4c[i][0].vz + zOfs;
            obj_s2->d.sprite.coords[1].x = obj->x1.n + s_faces_80123a4c[i][1].vx + xOfs;
            obj_s2->d.sprite.coords[1].y = obj->y1.n + s_faces_80123a4c[i][1].vy;
            obj_s2->d.sprite.coords[1].z = obj->z1.n + s_faces_80123a4c[i][1].vz + zOfs;
            obj_s2->d.sprite.coords[2].x = obj->x1.n + s_faces_80123a4c[i][2].vx + xOfs;
            obj_s2->d.sprite.coords[2].y = obj->y1.n + s_faces_80123a4c[i][2].vy;
            obj_s2->d.sprite.coords[2].z = obj->z1.n + s_faces_80123a4c[i][2].vz + zOfs;
            obj_s2->d.sprite.coords[3].x = obj->x1.n + s_faces_80123a4c[i][3].vx + xOfs;
            obj_s2->d.sprite.coords[3].y = obj->y1.n + s_faces_80123a4c[i][3].vy;
            obj_s2->d.sprite.coords[3].z = obj->z1.n + s_faces_80123a4c[i][3].vz + zOfs;

            obj_s2->x1.n = (obj_s2->d.sprite.coords[1].x + obj_s2->d.sprite.coords[2].x +
                            obj_s2->d.sprite.coords[3].x + obj_s2->d.sprite.coords[0].x) /
                           4;
            obj_s2->y1.n = (obj_s2->d.sprite.coords[1].y + obj_s2->d.sprite.coords[2].y +
                            obj_s2->d.sprite.coords[3].y + obj_s2->d.sprite.coords[0].y) /
                           4;
            obj_s2->z1.n = (obj_s2->d.sprite.coords[1].z + obj_s2->d.sprite.coords[2].z +
                            obj_s2->d.sprite.coords[3].z + obj_s2->d.sprite.coords[0].z) /
                           4;

            obj_s2->d.sprite.gfxIdx = GFX_TILED_DIAMONDS_DYN_1;
            AddObjPrim3(gGraphicsPtr->ot, obj_s2);
         }

         obj_s2->functionIndex = OBJF_NULL;
         break;

      case 2:
         for (i = 0; i < 6; i++) {
            obj_s2->d.sprite.coords[0].x = obj->x1.n + local_1c0[i][0].vx;
            obj_s2->d.sprite.coords[0].y = obj->y1.n + local_1c0[i][0].vy;
            obj_s2->d.sprite.coords[0].z = obj->z1.n + local_1c0[i][0].vz;
            obj_s2->d.sprite.coords[1].x = obj->x1.n + local_1c0[i][1].vx;
            obj_s2->d.sprite.coords[1].y = obj->y1.n + local_1c0[i][1].vy;
            obj_s2->d.sprite.coords[1].z = obj->z1.n + local_1c0[i][1].vz;
            obj_s2->d.sprite.coords[2].x = obj->x1.n + local_1c0[i][2].vx;
            obj_s2->d.sprite.coords[2].y = obj->y1.n + local_1c0[i][2].vy;
            obj_s2->d.sprite.coords[2].z = obj->z1.n + local_1c0[i][2].vz;
            obj_s2->d.sprite.coords[3].x = obj->x1.n + local_1c0[i][3].vx;
            obj_s2->d.sprite.coords[3].y = obj->y1.n + local_1c0[i][3].vy;
            obj_s2->d.sprite.coords[3].z = obj->z1.n + local_1c0[i][3].vz;

            obj_s2->x1.n = (obj_s2->d.sprite.coords[1].x + obj_s2->d.sprite.coords[2].x +
                            obj_s2->d.sprite.coords[3].x + obj_s2->d.sprite.coords[0].x) /
                           4;
            obj_s2->y1.n = (obj_s2->d.sprite.coords[1].y + obj_s2->d.sprite.coords[2].y +
                            obj_s2->d.sprite.coords[3].y + obj_s2->d.sprite.coords[0].y) /
                           4;
            obj_s2->z1.n = (obj_s2->d.sprite.coords[1].z + obj_s2->d.sprite.coords[2].z +
                            obj_s2->d.sprite.coords[3].z + obj_s2->d.sprite.coords[0].z) /
                           4;

            obj_s2->d.sprite.gfxIdx = GFX_TILED_DIAMONDS_DYN_1;
            AddObjPrim3(gGraphicsPtr->ot, obj_s2);
         }

         obj_s2->functionIndex = OBJF_NULL;
         break;

      case 1:
      case 3:

         for (i = 0; i < 6; i++) {
            obj_s2->d.sprite.coords[0].x = obj->x1.n + s_faces_80123a4c[i][0].vx;
            obj_s2->d.sprite.coords[0].y = obj->y1.n + s_faces_80123a4c[i][0].vy;
            obj_s2->d.sprite.coords[0].z = obj->z1.n + s_faces_80123a4c[i][0].vz;
            obj_s2->d.sprite.coords[1].x = obj->x1.n + s_faces_80123a4c[i][1].vx;
            obj_s2->d.sprite.coords[1].y = obj->y1.n + s_faces_80123a4c[i][1].vy;
            obj_s2->d.sprite.coords[1].z = obj->z1.n + s_faces_80123a4c[i][1].vz;
            obj_s2->d.sprite.coords[2].x = obj->x1.n + s_faces_80123a4c[i][2].vx;
            obj_s2->d.sprite.coords[2].y = obj->y1.n + s_faces_80123a4c[i][2].vy;
            obj_s2->d.sprite.coords[2].z = obj->z1.n + s_faces_80123a4c[i][2].vz;
            obj_s2->d.sprite.coords[3].x = obj->x1.n + s_faces_80123a4c[i][3].vx;
            obj_s2->d.sprite.coords[3].y = obj->y1.n + s_faces_80123a4c[i][3].vy;
            obj_s2->d.sprite.coords[3].z = obj->z1.n + s_faces_80123a4c[i][3].vz;

            obj_s2->x1.n = (obj_s2->d.sprite.coords[1].x + obj_s2->d.sprite.coords[2].x +
                            obj_s2->d.sprite.coords[3].x + obj_s2->d.sprite.coords[0].x) /
                           4;
            obj_s2->y1.n = (obj_s2->d.sprite.coords[1].y + obj_s2->d.sprite.coords[2].y +
                            obj_s2->d.sprite.coords[3].y + obj_s2->d.sprite.coords[0].y) /
                           4;
            obj_s2->z1.n = (obj_s2->d.sprite.coords[1].z + obj_s2->d.sprite.coords[2].z +
                            obj_s2->d.sprite.coords[3].z + obj_s2->d.sprite.coords[0].z) /
                           4;

            obj_s2->d.sprite.gfxIdx = GFX_TILED_DIAMONDS_DYN_1;
            AddObjPrim3(gGraphicsPtr->ot, obj_s2);

            if (obj->state3 == 3) {
               fade = ~obj->mem;
               poly = &gGraphicsPtr->quads[gQuadIndex - 1];
               setRGB0(poly, fade, fade, fade);
               obj->mem++;
               if (obj->mem >= 0xff) {
                  obj->mem = 0xff;
               }
            }
         }

         obj_s2->functionIndex = OBJF_NULL;
         break;

      default:
         obj_s2->functionIndex = OBJF_NULL;
         break;
      }

      // 3 //
      switch (obj->state3) {
      case 0:
         OBJ.theta += 0x20;
         if (OBJ.theta >= 0x400) {
            obj->state3++;
            OBJ.theta = 0;
         }
         break;

      case 1:
         if (OBJ.openFinished != 0) {
            obj->state3++;
         }
         break;

      case 2:
         OBJ.spinTimer++;
         if (OBJ.spinTimer == 0x40) {
            obj->state3++;
            OBJ.theta = 0x400;
         }
         break;

      case 3:
         if (OBJ.closeFinished != 0) {
            obj->state3++;
            OBJ.theta = 0x20;
            obj_s2 = Obj_GetUnused();
            obj_s2->functionIndex = OBJF_ATTACK_INFO_MARKER;
            obj_s2->x1.s.hi = obj->x1.s.hi;
            obj_s2->z1.s.hi = obj->z1.s.hi;
            obj_s2->d.objf052.type = ATK_MARKER_SUPPORT;
            obj_s2->d.objf052.clut = CLUT_GREENS;
         }
         break;

      case 4:
         OBJ.theta--;
         if (OBJ.theta <= 0) {
            gSignal3 = 1;
            obj->functionIndex = OBJF_NULL;
         }
         break;
      }

      break;
   }
}
