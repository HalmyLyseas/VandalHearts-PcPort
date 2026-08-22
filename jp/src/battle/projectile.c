#include "common.h"
#include "object.h"
#include "units.h"
#include "graphics.h"
#include "battle.h"
#include "field.h"

void CalculateProjectileDeltas(s32, s32, s32, s32, s32, s32, s32 *, s32 *, s32 *);
s32 ArcProjectileTo(Object *, s32, s32, s32, s32);
void RotateProjectile(s16, s16, s16);
void Objf022_029_Projectile(Object *);
void Objf023_Camera_RangedTarget(Object *);
s32 CalculateProjectileHeight(s8, s8, s8, s8);
void PopulateRangedAttackGridInDir(s16, s16, s16, s16, s32, s32);

void CalculateProjectileDeltas(s32 currentXorZ, s32 currentY, s32 destXorZ, s32 destY, s32 rise,
                               s32 height, s32 *deltaXorZ, s32 *deltaY, s32 *substeps) {
   // TODO: Better match? Got close (~25) w/o register overrides or do-while, but moving on for now.
   s32 a, b;

   do {
      currentXorZ >>= 8;
      destY >>= 8;
      rise >>= 8;
      destXorZ >>= 8;
      currentY >>= 8;
      height >>= 8;
      destY -= height;
   } while (0);

   *deltaY = -(SquareRoot12(((abs(height) * rise) >> 8) << 5) >> 4);
   b = -(*deltaY / rise);
   a = ((destY - currentY) / rise * 2) << 12;
   *substeps = b + (SquareRoot12(-a) >> 12);
   *deltaXorZ = ((destXorZ - currentXorZ) / *substeps) << 8;
   *deltaY = -(*deltaY << 8);
}

s32 s_currentX_80123174;
s32 s_destX_80123178;
s32 s_currentZ_8012317c;
s32 s_destZ_80123180;
s32 s_currentY_80123184;
s32 s_destY_80123188;
s16 s_isubstep_8012318c;

s32 ArcProjectileTo(Object *sprite, s32 destX, s32 destZ, s32 height, s32 cont) {
   static s32 deltaX = 0;
   static s32 deltaY = 0;
   static s32 deltaZ = 0;
   static s32 substeps = 0;
   static s32 rise = 3000;
   static const s32 unused = 0; //<- to fix sdata spacing

   if (cont == 0) {
      s_currentX_80123174 = sprite->x1.n << 8;
      s_destX_80123178 = destX << 8;
      s_currentY_80123184 = sprite->y1.n << 8;
      s_destY_80123188 = (-gMapRowPointers[destZ >> 8][destX >> 8].vertices[0].vy + 16) * 0x800;
      s_currentZ_8012317c = sprite->z1.n << 8;
      s_destZ_80123180 = destZ << 8;
      if (s_currentX_80123174 != s_destX_80123178) {
         CalculateProjectileDeltas(s_currentX_80123174, s_currentY_80123184, s_destX_80123178,
                                   s_destY_80123188, rise, height, &deltaX, &deltaY, &substeps);
         deltaZ = 0;
      } else {
         CalculateProjectileDeltas(s_currentZ_8012317c, s_currentY_80123184, s_destZ_80123180,
                                   s_destY_80123188, rise, height, &deltaZ, &deltaY, &substeps);
         deltaX = 0;
      }
      s_isubstep_8012318c = 0;
      return 0;
   } else {
      s_currentX_80123174 += deltaX;
      s_currentZ_8012317c += deltaZ;
      s_currentY_80123184 += deltaY;
      deltaY -= rise;
      sprite->y2.n = deltaY >> 2;
      if (deltaZ == 0) {
         sprite->x2.n = deltaX >> 2;
      } else {
         sprite->x2.n = deltaZ >> 2;
      }
      sprite->x1.n = s_currentX_80123174 >> 8;
      sprite->z1.n = s_currentZ_8012317c >> 8;
      sprite->y1.n = s_currentY_80123184 >> 8;
      s_isubstep_8012318c++;
      // For cont:1, return 1 when movement completes
      return s_isubstep_8012318c == substeps;
   }
}

void RotateProjectile(s16 rotx, s16 roty, s16 rotz) {
   s32 flag;
   Quad quad;
   VECTOR transformedVtx[4];
   MATRIX matrix;
   SVECTOR rotation;
   VECTOR translation;

   // .sdata: Initialized in pieces to fit
   quad[0] = ((SVECTOR){-15, -6, 0, 0});
   quad[1] = ((SVECTOR){15, -6, 0, 0});
   quad[2] = ((SVECTOR){-15, 6, 0, 0});
   quad[3] = ((SVECTOR){15, 6, 0, 0});

   // .rdata (rhs)
   translation = ((VECTOR){0, 0, 0, 0});

   rotation.vz = rotz;
   rotation.vx = rotx;
   rotation.vy = roty;

   PushMatrix();
   RotMatrix(&rotation, &matrix);
   TransMatrix(&matrix, &translation);
   SetRotMatrix(&matrix);
   SetTransMatrix(&matrix);

   RotTrans(&quad[0], &transformedVtx[0], &flag);
   RotTrans(&quad[1], &transformedVtx[1], &flag);
   RotTrans(&quad[2], &transformedVtx[2], &flag);
   RotTrans(&quad[3], &transformedVtx[3], &flag);

   gQuad_800fe63c[0].vx = (s16)transformedVtx[0].vx;
   gQuad_800fe63c[0].vy = (s16)transformedVtx[0].vy;
   gQuad_800fe63c[0].vz = (s16)transformedVtx[0].vz;

   gQuad_800fe63c[1].vx = (s16)transformedVtx[1].vx;
   gQuad_800fe63c[1].vy = (s16)transformedVtx[1].vy;
   gQuad_800fe63c[1].vz = (s16)transformedVtx[1].vz;

   gQuad_800fe63c[2].vx = (s16)transformedVtx[2].vx;
   gQuad_800fe63c[2].vy = (s16)transformedVtx[2].vy;
   gQuad_800fe63c[2].vz = (s16)transformedVtx[2].vz;

   gQuad_800fe63c[3].vx = (s16)transformedVtx[3].vx;
   gQuad_800fe63c[3].vy = (s16)transformedVtx[3].vy;
   gQuad_800fe63c[3].vz = (s16)transformedVtx[3].vz;

   PopMatrix();
}

#undef OBJF
#define OBJF 022
void Objf022_029_Projectile(Object *obj) {
   Object *targetSprite;
   Object *projectileSprite;
   Object *particles;
   s32 unitIdx;
   s32 projectileHeight;
   UnitStatus *target, *attacker;
   s16 mdist, xmdist, zmdist;
   s8 unused[24];

   projectileSprite = OBJ.sprite;
   target = &gUnits[gMapUnitsPtr[gTargetZ][gTargetX].s.unitIdx];
   unitIdx = OBJ_MAP_UNIT(obj).s.unitIdx;
   attacker = &gUnits[unitIdx];

   if (obj->functionIndex == OBJF_PROJECTILE) {
      targetSprite = target->sprite;
   } else {
      // OBJF_PROJECTILE_INDIRECT: Unused? maybe to open chest w/ ranged attacker?
      targetSprite = gTempGfxObj;
      targetSprite->x1.s.hi = gTargetX;
      targetSprite->z1.s.hi = gTargetZ;
      targetSprite->x1.s.lo = CV(0.5);
      targetSprite->z1.s.lo = CV(0.5);
      targetSprite->y1.n = GetTerrainElevation(gTargetZ, gTargetX);
   }

   switch (obj->state) {
   case 0:
      projectileSprite = Obj_GetUnused();
      projectileSprite->functionIndex = OBJF_NOOP;

      if (attacker->unitId == UNIT_ID_GRENADIER) {
         projectileSprite->d.sprite.gfxIdx = GFX_BOMB;
         projectileSprite->d.sprite.boxIdx = 4;
      } else if (attacker->unitId == UNIT_ID_BASILISK || attacker->unitId == UNIT_ID_M_CANNON ||
                 attacker->unitId == UNIT_ID_EGGWORM || attacker->unitId == UNIT_ID_BUGABOO) {
         projectileSprite->d.sprite.gfxIdx = GFX_EXPLOSION_3;
         projectileSprite->d.sprite.boxIdx = 4;
      } else {
         projectileSprite->d.sprite.gfxIdx = GFX_ARROW;
         projectileSprite->d.sprite.boxIdx = 7;
      }

      OBJ.sprite = projectileSprite;
      projectileSprite->x1.n = obj->x1.n;
      projectileSprite->z1.n = obj->z1.n;
      projectileSprite->y1.n = obj->y1.n;

      projectileHeight =
          CalculateProjectileHeight(projectileSprite->x1.s.hi, projectileSprite->z1.s.hi,
                                    targetSprite->x1.s.hi, targetSprite->z1.s.hi);
      ArcProjectileTo(projectileSprite, targetSprite->x1.n, targetSprite->z1.n, projectileHeight,
                      0);
      OBJ.xMid = obj->x1.n + ((targetSprite->x1.n - obj->x1.n) / 2);
      OBJ.zMid = obj->z1.n + ((targetSprite->z1.n - obj->z1.n) / 2);

      OBJ.xzMidDist = (targetSprite->x1.n - obj->x1.n) / 2;
      SET_ABS(OBJ.xzMidDist);

      OBJ.zMidDist = (targetSprite->z1.n - obj->z1.n) / 2;
      SET_ABS(OBJ.zMidDist);

      OBJ.xzMidDist += OBJ.zMidDist;

      if (projectileSprite->x1.n == targetSprite->x1.n) {
         OBJ.yRotOfs = DEG(90);
         OBJ.zRotOfs = DEG(180);
      }
      obj->state++;
      ArcProjectileTo(projectileSprite, 0, 0, 0, 1);
      break;
   case 1:
      particles = Obj_GetUnused();
      particles->x1.n = projectileSprite->x1.n;
      particles->y1.n = projectileSprite->y1.n;
      particles->z1.n = projectileSprite->z1.n;

      if (attacker->unitId == UNIT_ID_DIEGO_SNIPER || attacker->unitId == UNIT_ID_AMON_SNIPER ||
          attacker->unitId == UNIT_ID_DARIUS_SNIPER || attacker->unitId == UNIT_ID_LANDO ||
          attacker->unitId == UNIT_ID_SABINA) {
         particles->functionIndex = OBJF_SPARKLE_DUST;
      }
      if (attacker->unitId == UNIT_ID_DIEGO_BOWMAN || attacker->unitId == UNIT_ID_KIRA_BOWMAN ||
          attacker->unitId == UNIT_ID_AMON_BOWMAN || attacker->unitId == UNIT_ID_DARIUS_BOWMAN) {
         particles->functionIndex = OBJF_PROJECTILE_TRAIL_SPARKLES;
      }
      if (attacker->unitId == UNIT_ID_BUGABOO || attacker->unitId == UNIT_ID_EGGWORM) {
         particles->functionIndex = OBJF_PROJECTILE_TRAIL_POISON;
      }
      if (attacker->unitId == UNIT_ID_BASILISK || attacker->unitId == UNIT_ID_M_CANNON) {
         particles->functionIndex = OBJF_PROJECTILE_TRAIL_EXPLOSION;
      }
      if (attacker->unitId == UNIT_ID_KIRA_SNIPER) {
         particles->functionIndex = OBJF_PROJECTILE_TRAIL_SMOKE;
      }

      if (ArcProjectileTo(projectileSprite, 0, 0, 0, 1)) {
         gSignal3 = 1;
         gSignal4 = 1;
         obj->functionIndex = OBJF_NULL;
         projectileSprite->functionIndex = OBJF_NULL;
      }

      xmdist = OBJ.xMid - projectileSprite->x1.n;
      SET_ABS(xmdist);
      zmdist = OBJ.zMid - projectileSprite->z1.n;
      SET_ABS(zmdist);
      mdist = xmdist + zmdist;

      // Shifts compute 3/4ths
      gCameraZoom.vz = 250 + ((OBJ.xzMidDist - mdist) >> 1) + ((OBJ.xzMidDist - mdist) >> 2);

      if (obj->functionIndex == OBJF_NULL) {
         gCameraZoom.vz = 250;
         gCameraRotation.vx = 0x180;
      }
      break;
   }

   gCameraPos.vx = -(projectileSprite->x1.n >> 3);
   gCameraPos.vz = -(projectileSprite->z1.n >> 3);
   gCameraPos.vy = projectileSprite->y1.n >> 3;

   // x2,y2 (delta) set in ArcProjectileTo
   OBJ.zRot = ratan2(projectileSprite->y2.n, projectileSprite->x2.n);

   if (OBJ.yRotOfs == 0) {
      OBJ.zRot = DEG(360) - OBJ.zRot;
   }

   OBJ.zRot += OBJ.zRotOfs;
   RotateProjectile(gCameraRotation.vx, gCameraRotation.vy + OBJ.yRotOfs,
                    gCameraRotation.vz + OBJ.zRot);
   AddObjPrim6(gGraphicsPtr->ot, projectileSprite, 0);
}

#undef OBJF
#define OBJF 023
void Objf023_Camera_RangedTarget(Object *obj) {
   s16 diff;
   Object *targetSprite = OBJ.targetSprite;

   switch (obj->state) {
   case 0:
      OBJ.yRotDst = GetBestViewOfTarget(targetSprite->z1.s.hi, targetSprite->x1.s.hi, 1);

      diff = OBJ.yRotDst - gCameraRotation.vy;
      if (diff > 0) {
         if (diff > DEG(180)) {
            OBJ.yRotDst -= DEG(360);
         }
      } else if (diff < DEG(-180)) {
         OBJ.yRotDst += DEG(360);
      }

      OBJ.delay = 35;
      obj->state++;
      // fallthrough
   case 1:
      if (--OBJ.delay != 0) {
         gCameraRotation.vy += (OBJ.yRotDst - gCameraRotation.vy) >> 3;
      } else {
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

s32 CalculateProjectileHeight(s8 x1, s8 z1, s8 x2, s8 z2) {
   // TODO: Less hacky match; why does do-while continue to solve this type of issue??
   s8 ox, oz;
   s32 xstep = 0, zstep = 0;
   s32 highest;
   s32 startingElevation;

   do {
      ox = x1;
      oz = z1;
      startingElevation = gTerrainPtr[z1][x1].s.elevation;

      if (x1 > x2) {
         xstep = -1;
      }
      if (x1 < x2) {
         xstep = 1;
      }
      if (z1 > z2) {
         zstep = -1;
      }
      if (z1 < z2) {
         zstep = 1;
      }

      highest = startingElevation;
   } while (0);

   do {
      x1 += xstep;
      z1 += zstep;

      if (gTerrainPtr[z1][x1].s.terrain != TERRAIN_NO_ENTRY &&
          gTerrainPtr[z1][x1].s.terrain != TERRAIN_OBSTACLE) {
         if (gTerrainPtr[z1][x1].s.elevation > highest) {
            highest = gTerrainPtr[z1][x1].s.elevation;
         }
      } else {
         if (gMapRowPointers[z1][x1].height > highest) {
            highest = gMapRowPointers[z1][x1].height;
         }
      }
   } while (x1 != x2 || z1 != z2);

   highest = (highest - startingElevation) << 15;

   do {
      ox = ox - x2;
      SET_ABS(ox);
      oz = oz - z2;
      SET_ABS(oz);
   } while (0);

   return highest + ((s8)(ox + oz) << 14);
}

void PopulateRangedAttackGridInDir(s16 z, s16 x, s16 dz, s16 dx, s32 range, s32 which) {
   PathGridRow *pGrid;
   s8 elevation;
   s8 reach;
   s8 newElevation;
   s32 steps = 0;

   switch (which) {
   case 0:
      pGrid = &gPathGrid0[1];
      break;
   case 1:
      pGrid = &gPathGrid1[1];
      break;
   case 2:
      pGrid = &gPathGrid2[1];
      break;
   case 3:
      pGrid = &gPathGrid3[1];
      break;
   case 4:
      pGrid = &gPathGrid4[1];
      break;
   case 5:
      pGrid = &gPathGrid5[1];
      break;
   case 6:
      pGrid = &gPathGrid6[1];
      break;
   }

   range /= 2;
   elevation = gTerrainPtr[z][x].s.elevation;
   reach = elevation;

   while (1) {
      z += dz;
      x += dx;

      steps++;
      if (steps > range) {
         reach -= 2;
      } else {
         reach += 2;
      }

      newElevation = gTerrainPtr[z][x].s.elevation;
      elevation -= 2;

      if (gTerrainPtr[z][x].s.terrain == TERRAIN_NO_ENTRY && pGrid[z][x] == 0) {
         newElevation = gMapRowPointers[z][x].height;
      }
      if (gTerrainPtr[z][x].s.terrain == TERRAIN_OBSTACLE && pGrid[z][x] == 0) {
         newElevation = gMapRowPointers[z][x].height;
      }
      if (newElevation > reach || pGrid[z][x] != 0) {
         break;
      }
      if (newElevation > elevation) {
         elevation = newElevation;
      }
      if (newElevation >= elevation && gTerrainPtr[z][x].s.terrain != TERRAIN_OBSTACLE) {
         pGrid[z][x] = steps;
      }
   }
}
