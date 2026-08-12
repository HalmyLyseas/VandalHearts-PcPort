/* Map 38's flood -- one map's set piece -- plus the generic splash sprite.
 *
 * Objf657_Map38_Floodgate (spawned by SetupMapExtras()) is the driver. On reload it applies
 * the finished state instantly (DepressButton + Map38_SetWater + Map38_RaiseFloodgate).
 * Otherwise, once Objf558_EvaluateMap38_SlaySabina sets mapState, it runs the shared
 * OBJF_BUTTON_DEPRESS cutaway, then spawns Objf658_Map38_Floodwater and
 * Objf655_Map38_RaiseFloodgate together. It sweeps the 3x11 channel issuing TA_32 prompts
 * for each non-CLASS_AIRMAN unit caught there, spawns an Objf654_Map38_WashAwayUnit per
 * victim, and follows the first one with the camera before easing back.
 *
 * Objf655 lifts the three gate tiles' faces, carrying the attached water face along until
 * level; Map38_RaiseFloodgate()/Map38_SetWater() are the instant equivalents. Objf658
 * renders the advancing water sheet and emits splashes (it carries a FIXME: fake-matched
 * do-while). Objf654 hides the real unit sprite and re-renders an accelerating clone until
 * it passes gMapMaxZ, then marks the vacated tile TA_22.
 *
 * Objf659_Splash (OBJF_SPLASH) is the generic animated splash, also used by Map 39. */
#include "common.h"
#include "object.h"
#include "graphics.h"
#include "field.h"
#include "state.h"
#include "audio.h"
#include "units.h"
#include "battle.h"

void Objf657_Map38_Floodgate(Object *);
void Map38_RaiseFloodgate(void);
void Objf655_Map38_RaiseFloodgate(Object *);
void Map38_SetWater(void);
void Objf658_Map38_Floodwater(Object *);
void Objf659_Splash(Object *);
void Objf654_Map38_WashAwayUnit(Object *);

#undef OBJF
#define OBJF 657
void Objf657_Map38_Floodgate(Object *obj) {
   Object *obj_a1;
   s32 iz, ix;
   s32 casualties;

   switch (obj->state) {
   case 0:
      if (gState.mapState.s.field_0x0 != 0) {
         DepressButton(21, 4);
         Map38_SetWater();
         Map38_RaiseFloodgate();
         obj->functionIndex = OBJF_NULL;
         return;
      }
      obj->state++;

   // fallthrough
   case 1:
      if (gState.mapState.s.field_0x0 != 0) {
         gCameraRotation.vy &= 0xfff;
         AssignFromMainCamera(&OBJ.camera);
         obj->state++;
         obj->state3 = 0;
         gPlayerControlSuppressed = 1;
      }
      break;

   case 2:

      switch (obj->state2) {
      case 0:
         if (++obj->state3 >= 0) {
            obj->state3 = 0;
            obj->state2++;
         }
         break;

      case 1:
         obj_a1 = Obj_GetUnused();
         obj_a1->functionIndex = OBJF_BUTTON_DEPRESS;
         obj_a1->x1.n = CV(21.0);
         obj_a1->z1.n = CV(4.0);
         obj_a1->y1.n = CV(2.0);
         OBJ.child = obj_a1;
         obj->state2++;

      // fallthrough
      case 2:
         obj_a1 = OBJ.child;
         if (obj_a1->functionIndex != OBJF_BUTTON_DEPRESS) {
            PerformAudioCommand(AUDIO_CMD_PREPARE_XA(79));
            obj->state2++;
         }
         break;

      case 3:
         PanCamera(CV(26.0), CV(2.0), CV(7.0), 3);
         if (++obj->state3 >= 32) {
            obj->state3 = 0;
            obj->state2++;
         }
         break;

      case 4:
         obj_a1 = Obj_GetUnused();
         obj_a1->functionIndex = OBJF_MAP38_FLOODWATER;
         OBJ.child = obj_a1;
         obj_a1 = Obj_GetUnused();
         obj_a1->functionIndex = OBJF_MAP38_RAISE_FLOODGATE;
         PerformAudioCommand(AUDIO_CMD_PLAY_XA(79));
         OBJ.casualty = NULL;
         obj->state2++;
         break;

      case 5:
         gCameraZoom.vz += 1;
         if (obj->state3 == 30) {
            PerformAudioCommand(AUDIO_CMD_FADE_OUT_2_1);
         }
         if (++obj->state3 >= 256) {
            obj->state2++;
         }
         break;

      case 6:
         obj->x1.s.hi = 25;
         obj->z1.s.hi = 5;
         obj->x3.s.hi = 27;
         obj->z3.s.hi = 15;
         obj->state2++;

      // fallthrough
      case 7:
         if (OBJ_MAP_UNIT(obj).raw != 0 &&
             gUnits[OBJ_MAP_UNIT(obj).s.unitIdx].class != CLASS_AIRMAN) {
            gState.msgFinished = 0;
            OBJ_TILE_STATE(obj).action = TA_32;
            obj->state2++;
         } else {
            obj->state2 += 2;
         }
         break;

      case 8:
         if (gState.msgFinished) {
            obj->state2++;
         }
         break;

      case 9:
         obj->x1.s.hi += 1;
         if (obj->x1.s.hi > 27) {
            obj->state2++;
            PerformAudioCommand(AUDIO_CMD_PREPARE_XA(103));
            obj->state3 = 0;
         } else {
            obj->state2 -= 2;
         }
         break;

      case 10:
         if (++obj->state3 >= 8) {
            obj->x1.s.hi = 25;
            obj->z1.s.hi += 1;
            if (obj->z1.s.hi >= 16) {
               casualties = 0;
               for (ix = 25; ix <= 27; ix++) {
                  for (iz = 5; iz <= 15; iz++) {
                     if (gMapUnitsPtr[iz][ix].raw != 0 &&
                         gUnits[gMapUnitsPtr[iz][ix].s.unitIdx].class != CLASS_AIRMAN) {
                        obj_a1 = Obj_GetUnused();
                        obj_a1->functionIndex = OBJF_MAP38_WASH_AWAY_UNIT;
                        obj_a1->x1.n = ix * CV(1.0) + CV(0.3125);
                        obj_a1->z1.n = iz * CV(1.0) + CV(0.3125);
                        if (OBJ.casualty == NULL) {
                           OBJ.casualty = obj_a1;
                        }
                        casualties++;
                     }
                  }
               }
               if (casualties != 0) {
                  PerformAudioCommand(AUDIO_CMD_PLAY_XA(103));
               }
               obj->state3 = 0;
               obj->state2++;
            } else {
               obj->state2 -= 3;
            }
         }
         break;

      case 11:
         obj_a1 = OBJ.casualty;
         if (obj_a1 != NULL && obj_a1->functionIndex == OBJF_MAP38_WASH_AWAY_UNIT) {
            PanCamera(obj_a1->x1.n, obj_a1->y1.n, obj_a1->z1.n + obj->state3 * 8, 3);
         } else {
            PanCamera(CV(26.0), CV(2.0), CV(5.0), 3);
         }
         gCameraZoom.vz += (400 - gCameraZoom.vz) >> 3;
         if (++obj->state3 >= 128) {
            obj->state2++;
         }
         break;

      case 12:
         obj_a1 = OBJ.child;
         obj_a1->state2++;
         obj->state2++;
         break;

      case 13:
         obj_a1 = OBJ.child;
         if (obj_a1->functionIndex != OBJF_MAP38_FLOODWATER) {
            obj->state3 = 0;
            obj->state++;
         }
         break;
      }

      break;

   case 3:
      EaseOutCamera(&OBJ.camera, 3);
      if (++obj->state3 >= 32) {
         obj->state++;
      }
      break;

   case 4:
      AssignToMainCamera(&OBJ.camera);
      gPlayerControlSuppressed = 0;
      gState.signal = 0;
      obj->state++;
      break;

   case 5:
      break;
   }
}

void Map38_RaiseFloodgate(void) {
   s32 i;
   MapTileModel *tileModel;

   for (i = 0; i < 3; i++) {
      tileModel = &gMapRowPointers[4][25 + i];
      AdjustFaceElevation(tileModel, 1, -48);
      AdjustFaceElevation(tileModel, 2, -48);
      AdjustFaceElevation(tileModel, 3, -16);
      UpdateTileElevation(4, 25 + i);
   }
}

#undef OBJF
#define OBJF 655
void Objf655_Map38_RaiseFloodgate(Object *obj) {
   s32 i;
   MapTileModel *tileModel;

   switch (obj->state) {
   case 0:
      tileModel = &gMapRowPointers[4][25];
      obj->y1.n = -tileModel->vertices[tileModel->faces[1][0]].vy << 4; // top
      obj->y2.n = -tileModel->vertices[tileModel->faces[2][0]].vy << 4; // bottom
      OBJ.waterFaceY = -tileModel->vertices[tileModel->faces[3][0]].vy << 4;
      obj->mem = 0;
      obj->y3.n = 2;
      obj->state++;

   // fallthrough
   case 1:
      for (i = 0; i < 3; i++) {
         tileModel = &gMapRowPointers[4][25 + i];
         SetFaceElevation(tileModel, 1, -(obj->y1.n + obj->mem) >> 4);
         SetFaceElevation(tileModel, 2, -(obj->y2.n + obj->mem) >> 4);
         if (obj->mem <= 256) {
            // Continue raising attached water face until level
            SetFaceElevation(tileModel, 3, -(OBJ.waterFaceY + obj->mem) >> 4);
         }
      }
      obj->mem += obj->y3.n;
      if (obj->mem >= 768) {
         for (i = 0; i < 3; i++) {
            UpdateTileElevation(4, 25 + i);
         }
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

void Map38_SetWater(void) {
   s32 iz, ix;
   MapTileModel *tileModel;

   for (ix = 25; ix <= 27; ix++) {
      for (iz = 5; iz <= 15; iz++) {
         tileModel = &gMapRowPointers[iz][ix];
         AdjustFaceElevation(tileModel, 0, -16);
         tileModel->gfx[0] = GFX_MAP_TEXTURE_52;
         gTerrainPtr[iz][ix].s.terrain = TERRAIN_WATER;
         gTerrainPtr[iz][ix].s.elevation = -(tileModel->vertices[0].vy >> 4);
      }
   }
}

#undef OBJF
#define OBJF 658
void Objf658_Map38_Floodwater(Object *obj) {
   // FIXME: Fake match (do-while)

   Quad quad = {
       {-128, 0, -128, 0},
       {128, 0, -128, 0},
       {-128, 0, 128, 0},
       {128, 0, 128, 0},
   };

   s32 i, j, k;
   s32 iVar4;
   u8 fade;
   s16 r;
   s16 elevation;
   POLY_FT4 *poly;
   Object *obj_s0;
   Object *obj_s2;
   SVectorXZY *pCoord;
   s32 iVar7;
   s16 theta;
   s8 unused[32];

   switch (obj->state) {
   case 0:
      obj->state++;

   // fallthrough
   case 1:
      obj->state++;
      obj->state2 = 0;
      obj->state3 = 16;
      obj->mem = 4;
      obj->z1.n = 0;
      obj->z2.n = CV(0.25);
      OBJ.theta = 0;
      OBJ.timer = 0;
      OBJ.todo_x30 = 0;
      break;

   case 2:
      obj->state++;

   // fallthrough
   case 3:
      if (obj->state2 < 2 && --OBJ.timer <= 0) {
         for (iVar4 = 0; iVar4 < 8; iVar4++) {
            obj_s0 = Obj_GetUnused();
            obj_s0->functionIndex = OBJF_SPLASH;
            obj_s0->x1.n = CV(25.125) + iVar4 * CV(0.375);
            obj_s0->z1.n = CV(4.5);
            obj_s0->y1.n = GetTerrainElevation(obj_s0->z1.s.hi, obj_s0->x1.s.hi) + CV(0.5);
            obj_s0->z2.n = CV(0.125);
            obj_s0->d.sprite.semiTrans = 2;
         }
         OBJ.timer = rand() % 3 + 6;
      }
      obj->mem = (obj->state3 >> 5) + 1;
      if (obj->mem > 4) {
         obj->mem = 4;
      }
      obj->z1.n += obj->z2.n;
      if (obj->z1.n >= CV(11.0)) {
         obj->z1.n = CV(11.0);
      }

      switch (obj->state2) {
      case 0:
         OBJ.theta += DEG(22.5);
         OBJ.todo_x30 += 2;
         r = OBJ.todo_x30 >> 3;
         if (r >= CV(0.1875)) {
            OBJ.todo_x30 = 0;
            r = CV(0.1875);
            obj->state2++;
         }
         break;

      case 1:
         OBJ.theta += DEG(22.5);
         r = CV(0.1875);
         break;

      case 2:
         OBJ.theta += DEG(22.5);
         OBJ.todo_x30 -= 2;
         r = OBJ.todo_x30 >> 3;
         if (r <= 0) {
            OBJ.todo_x30 = 0;
            r = 0;
            obj->state2++;
         }
         break;

      default:
         OBJ.todo_x30 += 1;
         r = 0;
         if (OBJ.todo_x30 >= 32) {
            obj->state++;
         }
         break;
      }

      if (obj->state3 <= 128) {
         obj_s2 = Obj_GetUnused();
         obj_s2->functionIndex = OBJF_NOOP;
         obj_s2->d.sprite.gfxIdx = GFX_MAP_TEXTURE_52;
         elevation = GetTerrainElevation(5, 26);
         theta = OBJ.theta;
         quad[3].vy = quad[2].vy = abs(r * rsin(theta) >> 12);

         for (i = 5; i < obj->z1.s.hi + 5; i++) {
            if (i == 5) {
               quad[0].vy = 0;
               quad[1].vy = 0;
            } else {
               quad[0].vy = quad[2].vy;
               quad[1].vy = quad[3].vy;
               theta += DEG(33.75);
               iVar7 = r * rsin(theta) >> 12;
               quad[2].vy = iVar7;
               quad[3].vy = iVar7;
            }
            for (j = 25; j <= 27; j++) {
               pCoord = &obj_s2->d.sprite.coords[0];
               for (iVar4 = 0; iVar4 < 4; iVar4++) {
                  pCoord->x = j * CV(1.0) + quad[iVar4].vx + CV(0.5);
                  pCoord->z = i * CV(1.0) + quad[iVar4].vz + CV(0.5);
                  pCoord->y = elevation + obj->state3 + quad[iVar4].vy;
                  pCoord++;
               }
               pCoord = &obj_s2->d.sprite.coords[0];
               obj_s2->x1.n = (pCoord[0].x + pCoord[1].x + pCoord[2].x + pCoord[3].x) / 4;
               obj_s2->z1.n = (pCoord[0].z + pCoord[1].z + pCoord[2].z + pCoord[3].z) / 4;
               obj_s2->y1.n = (pCoord[0].y + pCoord[1].y + pCoord[2].y + pCoord[3].y) / 4;
               obj_s2->d.sprite.semiTrans = 1;
               obj_s2->d.sprite.otOfs = (4 - obj->mem) * 3 - 5;

               for (k = 0; k < obj->mem; k++) {
                  obj_s2->d.sprite.gfxIdx = GFX_MAP_TEXTURE_52;
                  AddObjPrim5(gGraphicsPtr->ot, obj_s2);
                  poly = &gGraphicsPtr->quads[gQuadIndex - 1];
                  fade = 256 - obj->state3;
                  setRGB0(poly, fade, fade, fade);
               }
               for (k = obj->mem; k < 2; k++) {
                  obj_s2->d.sprite.semiTrans = 2;
                  obj_s2->d.sprite.gfxIdx = GFX_MAP_TEXTURE_52;
                  AddObjPrim5(gGraphicsPtr->ot, obj_s2);
                  poly = &gGraphicsPtr->quads[gQuadIndex - 1];
                  fade = obj->state3;
                  setRGB0(poly, fade, fade, fade);
               }
            }
         }

         i = obj->z1.s.hi + 5;
         for (j = 25; j <= 27; j++) {
            obj_s2->d.sprite.coords[0].y = obj_s2->d.sprite.coords[1].y =
                obj_s2->d.sprite.coords[2].y = obj_s2->d.sprite.coords[3].y =
                    obj->state3 + elevation;

            obj_s2->d.sprite.coords[0].x = j * CV(1.0) + CV(1.0);
            obj_s2->d.sprite.coords[1].x = j * CV(1.0);
            obj_s2->d.sprite.coords[2].x = j * CV(1.0) + CV(1.0);
            obj_s2->d.sprite.coords[3].x = j * CV(1.0);

            obj_s2->d.sprite.coords[0].z = i * CV(1.0);
            obj_s2->d.sprite.coords[1].z = i * CV(1.0);
            obj_s2->d.sprite.coords[2].z = i * CV(1.0) + (u8)obj->z1.s.lo;
            obj_s2->d.sprite.coords[3].z = i * CV(1.0) + (u8)obj->z1.s.lo;
            obj_s2->d.sprite.semiTrans = 1;

            do {
               iVar7 = obj->mem * 3;
               iVar7 -= 5;
               obj_s2->d.sprite.otOfs = iVar7;
            } while (0);

            for (k = 0; k < obj->mem; k++) {
               obj_s2->d.sprite.gfxIdx = GFX_MAP_TEXTURE_52;
               AddObjPrim4(gGraphicsPtr->ot, obj_s2);
               poly = &gGraphicsPtr->quads[gQuadIndex - 1];
               fade = 256 - obj->state3;
               setRGB0(poly, fade, fade, fade);
            }
            for (k = obj->mem; k < 2; k++) {
               obj_s2->d.sprite.semiTrans = 2;
               obj_s2->d.sprite.gfxIdx = GFX_MAP_TEXTURE_52;
               AddObjPrim5(gGraphicsPtr->ot, obj_s2);
               poly = &gGraphicsPtr->quads[gQuadIndex - 1];
               fade = obj->state3;
               setRGB0(poly, fade, fade, fade);
            }
         }
         obj_s2->functionIndex = OBJF_NULL;
      }
      if (++obj->state3 >= 128) {
         obj->state3 = 128;
      }
      if (obj->state == 4) {
         Map38_SetWater();
      }
      break;

   case 4:
      obj->state++;
      break;

   case 5:
      gState.mapState.s.field_0x0 = 3;
      obj->functionIndex = OBJF_NULL;
      break;
   }
}

#undef OBJF
#define OBJF 659
void Objf659_Splash(Object *obj) {
   static s16 splashAnimData[20] = {
       7, GFX_SPLASH_1, 2, GFX_SPLASH_2, 2, GFX_SPLASH_3, 2, GFX_SPLASH_4, 2, GFX_SPLASH_5,
       2, GFX_SPLASH_6, 2, GFX_SPLASH_7, 2, GFX_SPLASH_8, 2, GFX_NULL,     0, GFX_NULL};
   static Quad quad = {{-16, -16, 0, 0}, {16, -16, 0, 0}, {-16, 16, 0, 0}, {16, 16, 0, 0}};

   s32 i;

   switch (obj->state) {
   case 0:
      obj->d.sprite.animData = splashAnimData;
      obj->d.sprite.semiTrans = 4;
      obj->state++;
      // TODO: Object?
      if (obj->d.sprite.coords[0].x == 0) {
         obj->d.sprite.coords[0].x = rand() % 64;
      }
      obj->d.sprite.coords[0].z = 128;
      obj->mem = 0;

   // fallthrough
   case 1:
      obj->x1.n += obj->x2.n;
      obj->z1.n += obj->z2.n;
      for (i = 0; i < 4; i++) {
         gQuad_800fe63c[i] = quad[i];
      }
      gQuad_800fe63c[0].vy -= (obj->d.sprite.coords[0].x * rsin(obj->mem) >> 12);
      gQuad_800fe63c[1].vy -= (obj->d.sprite.coords[0].x * rsin(obj->mem) >> 12);
      obj->mem += obj->d.sprite.coords[0].z;
      UpdateObjAnimation(obj);
      AddObjPrim6(gGraphicsPtr->ot, obj, 0);
      if (obj->d.sprite.animFinished) {
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 654
void Objf654_Map38_WashAwayUnit(Object *obj) {
   Object *unitSprite;
   Object *clonedSprite;

   switch (obj->state) {
   case 0:
      OBJ.tileX = obj->x1.s.hi;
      OBJ.tileZ = obj->z1.s.hi;
      obj->state++;

   // fallthrough
   case 1:
      obj->state++;
      break;

   case 2:
      unitSprite = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
      obj->x1.n = unitSprite->x1.n;
      obj->z1.n = unitSprite->z1.n;
      obj->y1.n = unitSprite->y1.n;
      OBJ.unitSprite = unitSprite;
      unitSprite->d.sprite.hidden = 1;

      clonedSprite = Obj_GetUnused();
      CopyObject(unitSprite, clonedSprite);
      clonedSprite->functionIndex = OBJF_NOOP;
      clonedSprite->d.sprite.hidden = 0;
      RenderUnitSprite(gGraphicsPtr->ot, clonedSprite, 0);
      clonedSprite->functionIndex = OBJF_NULL;

      obj->state3 = 0;
      obj->state++;
      break;

   case 3:
      unitSprite = OBJ.unitSprite;

      switch (obj->state2) {
      case 0:
         obj->x2.n = 0;
         obj->z2.n = CV(0.03125);
         obj->z3.n = CV(0.00390625);
         if (++obj->state3 >= 32) {
            obj->state2++;
         }
         break;

      case 1:
         obj->x2.n = 0;
         obj->z3.n = CV(0.03125);
         if (obj->z1.s.hi >= gMapMaxZ) {
            gTileStateGridPtr[OBJ.tileZ][OBJ.tileX].action = TA_22;
            obj->functionIndex = OBJF_NULL;
         }
         break;
      }

      obj->x1.n += obj->x2.n;
      obj->z1.n += obj->z2.n;
      obj->z2.n += obj->z3.n;

      clonedSprite = Obj_GetUnused();
      CopyObject(unitSprite, clonedSprite);
      clonedSprite->d.sprite.hidden = 0;
      clonedSprite->x1.n = obj->x1.n;
      clonedSprite->z1.n = obj->z1.n;
      RenderUnitSprite(gGraphicsPtr->ot, clonedSprite, 0);
      clonedSprite->functionIndex = OBJF_NULL;

      if (--obj->mem <= 0) {
         CreatePositionedObj(obj, OBJF_SPLASH);
         obj->mem = rand() % 5;
      }
      break;
   }
}
