/* Map 35's collapsing bridge, plus the shared flaming-rock projectile.
 *
 * Objf652_Map35_Button (spawned by SetupMapExtras()) is the entire Map 35 sequence. When
 * Objf555_EvaluateMap35_SlayKurtz sets mapState it runs the shared OBJF_BUTTON_DEPRESS cutaway,
 * focuses on the span, and branches on whether any player unit stands on the 5x2 deck:
 * with no player casualties it simply drops the span, otherwise it walks the deck tile by
 * tile issuing TA_32 dialogue prompts first. Either path throws OBJF_EXPLODING_TILE debris
 * built from the embedded tile model, vents OBJF_SMOKE, marks the doomed units TA_22 and
 * calls Map35_RemoveBridge(), which turns the deck into water tiles. State 0 applies the
 * finished state directly on reload.
 *
 * Objf702_FlamingRock (OBJF_FLAMING_ROCK) is not a Map 35 object and sits here only by
 * address adjacency: a ballistic burning rock trailing explosion puffs, dying on terrain
 * contact; spawned by Objf336_Salamander_Segment (spells/salamander.c) and by the cut
 * Objf734_MeteorImpact_Unused (events/fx_scenes.c). */
#include "common.h"
#include "object.h"
#include "graphics.h"
#include "field.h"
#include "state.h"
#include "audio.h"
#include "units.h"
#include "battle.h"

void Map35_RemoveBridgeTile(s32, s32);
void Map35_RemoveBridge(void);
void Objf652_Map35_Button(Object *);
void Objf702_FlamingRock(Object *);

static MapTileModel sMapTileModel_80100e9c = {
#include "assets/80100e9c.inc"
};

void Map35_RemoveBridgeTile(s32 z, s32 x) {
   MapTileModel *tileModel;

   tileModel = &gMapRowPointers[z][x];
   AdjustFaceElevation(tileModel, 0, 64);
   AdjustFaceElevation(tileModel, 1, 48);
   tileModel->gfx[0] = GFX_MAP_TEXTURE_5;
   gTerrainPtr[z][x].s.elevation = -(tileModel->vertices[0].vy >> 4);
   gTerrainPtr[z][x].s.terrain = TERRAIN_WATER;
   UpdateTileElevation(z, x);
}

void Map35_RemoveBridge(void) {
   s32 iz, ix;

   for (ix = 9; ix <= 13; ix++) {
      for (iz = 7; iz <= 8; iz++) {
         Map35_RemoveBridgeTile(iz, ix);
      }
   }
}

#undef OBJF
#define OBJF 652
void Objf652_Map35_Button(Object *obj) {
   extern MapTileModel s_tileModel_801246a4;

   Object *obj_s0;
   s32 i, j, k;
   s32 avail;
   s32 alt;
   MapTileModel *pTileModel;
   s8 unused[16];
   s32 casualties;

   switch (obj->state) {
   case 0:
      if (gState.mapState.s.field_0x0 != 0) {
         DepressButton(18, 8);
         Map35_RemoveBridge();
         obj->functionIndex = OBJF_NULL;
         return;
      }
      obj->state++;

   // fallthrough
   case 1:
      if (gState.mapState.s.field_0x0 != 0) {
         gPlayerControlSuppressed = 1;
         obj->state++;
      }
      break;

   case 2:

      switch (obj->state2) {
      case 0:
         gCameraRotation.vy &= 0xfff;
         AssignFromMainCamera(&OBJ.camera);
         obj_s0 = Obj_GetUnused();
         obj_s0->functionIndex = OBJF_BUTTON_DEPRESS;
         obj_s0->z1.s.hi = 8;
         obj_s0->x1.s.hi = 18;
         OBJ.buttonDepress = obj_s0;
         casualties = 0;
         OBJ.hitPlayerUnit = 0;
         for (j = 9; j <= 13; j++) {
            for (i = 7; i <= 8; i++) {
               if (gMapUnitsPtr[i][j].s.team == TEAM_PLAYER) {
                  casualties++;
               }
            }
         }
         if (casualties != 0) {
            OBJ.hitPlayerUnit = 1;
         }
         obj->state2++;
         break;

      case 1:
         obj_s0 = OBJ.buttonDepress;
         if (obj_s0->state == 99) {
            obj->state2++;
         }
         break;

      case 2:
         obj->state++;
         obj->state2 = 0;
         if (OBJ.hitPlayerUnit) {
            PerformAudioCommand(AUDIO_CMD_PREPARE_XA(19));
         } else {
            PerformAudioCommand(AUDIO_CMD_PREPARE_XA(37));
         }
         break;
      }

      break;

   case 3:

      switch (obj->state2) {
      case 0:
         obj->x1.n = CV(11.0);
         obj->z1.n = CV(8.0);
         obj->y1.n = CV(3.5);
         obj_s0 = Obj_GetUnused();
         obj_s0->functionIndex = OBJF_FOCUS_CAMERA;
         obj_s0->d.objf026.target = obj;
         obj_s0->d.objf026.zoom = 512;
         obj->state3 = 0;
         obj->state2++;

      // fallthrough
      case 1:
         if (++obj->state3 >= 32) {
            obj->state2 = 0;
            obj->state3 = 0;
            obj->state++;
         }
         break;

      case 2:
         break;
      }

      break;

   case 4:
      if (!OBJ.hitPlayerUnit) {
         PerformAudioCommand(AUDIO_CMD_PLAY_XA(37));
      }

      s_tileModel_801246a4 = sMapTileModel_80100e9c;
      pTileModel = &gMapRowPointers[15][32]; //?
      for (j = 0; j < 10; j++) {
         pTileModel[j] = sMapTileModel_80100e9c;
      }

      alt = 0;
      if (!OBJ.hitPlayerUnit) {
         for (j = 9; j <= 13; j++) {
            for (i = 7; i <= 8; i++) {
               if (gMapUnitsPtr[i][j].raw != 0) {
                  gTileStateGridPtr[i][j].action = TA_22;
               }
               if (alt != 0) {
                  obj_s0 = Obj_GetUnused();
                  obj_s0->functionIndex = OBJF_EXPLODING_TILE;
                  obj_s0->d.objf653.translation.vx = j * CV(1.0);
                  obj_s0->d.objf653.translation.vy = CV(2.5);
                  obj_s0->d.objf653.translation.vz = i * CV(1.0);
                  obj_s0->d.objf653.tileModel = &sMapTileModel_80100e9c;
                  obj_s0->d.objf653.riverVelocity = CV(0.0625);
                  obj_s0->state3 = (rand() >> 2) % 48;
                  alt = 0;
               } else {
                  alt++;
               }
            }
         }
      }

      obj->state++;
      break;

   case 5:
      if (OBJ.hitPlayerUnit) {
         obj->state++;
         OBJ.smokeElapsed = 0;
         PerformAudioCommand(AUDIO_CMD_PLAY_XA(115));
      } else {
         gCameraRotation.vy -= DEG(1.40625);
         if (++obj->state3 >= 64) {
            Map35_RemoveBridge();
            obj->state3 = 0;
            gCameraRotation.vy &= 0xfff;
            obj->state2 = 0;
            if (OBJ.hitPlayerUnit) {
               obj->state++;
               OBJ.smokeElapsed = 0;
            } else {
               obj->state += 2;
            }
         }
      }

      break;

   case 6:
      OBJ.smokeElapsed++;
      if (--OBJ.smokeTimer <= 0 && obj->state2 < 7) {
         for (j = 9; j <= 13; j++) {
            for (i = 7; i <= 8; i++) {
               obj_s0 = Obj_GetUnused();
               obj_s0->functionIndex = OBJF_SMOKE;
               obj_s0->x1.n = j * CV(1.0);
               obj_s0->z1.n = i * CV(1.0) + CV(0.5);
               obj_s0->y1.n = GetTerrainElevation(obj_s0->z1.s.hi, obj_s0->x1.s.hi);
               obj_s0->d.sprite.semiTrans = 2;
               obj_s0->x2.n = -rand() % CV(0.25);
               obj_s0->z2.n = rand() % CV(0.25) - CV(0.125);
               obj_s0->y2.n = rand() % CV(0.25);
            }
         }
         OBJ.smokeTimer = 16 - (OBJ.smokeElapsed >> 4);
         if (OBJ.smokeTimer < 4) {
            OBJ.smokeTimer = 4;
         }
      }

      switch (obj->state2) {
      case 0:
         OBJ.tileX = 9;
         obj->state2++;

      // fallthrough
      case 1:
         OBJ.tileZ = 7;
         obj->state2++;

      // fallthrough
      case 2:
         j = OBJ.tileX;
         i = OBJ.tileZ;
         if (gMapUnitsPtr[i][j].raw != 0) {
            gState.msgFinished = 0;
            gTileStateGridPtr[i][j].action = TA_32;
            obj->state2++;
         } else {
            obj->state2 += 2;
         }
         break;

      case 3:
         if (gState.msgFinished) {
            obj->state2++;
         }
         break;

      case 4:
         OBJ.tileZ++;
         if (OBJ.tileZ >= 9) {
            obj->state2++;
         } else {
            obj->state2 = 2;
         }
         break;

      case 5:
         OBJ.tileX++;
         if (OBJ.tileX >= 14) {
            obj->state2++;
            PerformAudioCommand(AUDIO_CMD_PREPARE_XA(37));
            PerformAudioCommand(AUDIO_CMD_FADE_OUT_8_8);
            obj->state3 = 0;
         } else {
            obj->state2 = 1;
         }
         break;

      case 6:
         if (++obj->state3 >= 64) {
            k = 2;
            PerformAudioCommand(AUDIO_CMD_PLAY_XA(37));
            for (j = 9; j <= 13; j++) {
               for (i = 7; i <= 8; i++) {
                  if (gMapUnitsPtr[i][j].raw != 0) {
                     gTileStateGridPtr[i][j].action = TA_22;
                  }
                  avail = Obj_CountUnused();
                  if (--k <= 0 && avail >= 50) {
                     obj_s0 = Obj_GetUnused();
                     obj_s0->functionIndex = OBJF_EXPLODING_TILE;
                     obj_s0->d.objf653.translation.vx = j * CV(1.0);
                     obj_s0->d.objf653.translation.vy = CV(2.5);
                     obj_s0->d.objf653.translation.vz = i * CV(1.0);
                     obj_s0->d.objf653.tileModel = &sMapTileModel_80100e9c;
                     obj_s0->d.objf653.riverVelocity = CV(0.0625);
                     obj_s0->state3 = (rand() >> 2) % 48;
                     k = 1;
                  }
               }
            }
            Map35_RemoveBridge();
            obj->state3 = 0;
            obj->state2++;
            PerformAudioCommand(AUDIO_CMD_FADE_IN_8_8);
         }
         break;

      case 7:
         gCameraZoom.vz += 10;
         gCameraRotation.vx += 1;
         if (++obj->state3 >= 96) {
            obj->state++;
            obj->state3 = 0;
         }
         break;
      }

      break;

   case 7:
      EaseOutCamera(&OBJ.camera, 3);
      if (++obj->state3 >= 32) {
         AssignToMainCamera(&OBJ.camera);
         obj->state++;
      }
      break;

   case 8:
      gPlayerControlSuppressed = 0;
      gState.signal = 0;
      obj->state++;
      break;

   case 9:
      break;
   }
}

#undef OBJF
#define OBJF 702
void Objf702_FlamingRock(Object *obj) {
   extern s16 gExplosionAnimData_800ffa80[26];

   static s16 rockAnimData[12] = {4, GFX_ROCK_1, 2, GFX_ROCK_2, 2, GFX_ROCK_3,
                                  2, GFX_ROCK_4, 2, GFX_NULL,   1, GFX_NULL};

   s16 x, y, z;
   Object *particle;
   s32 i, skip;
   POLY_FT4 *poly;

   switch (obj->state) {
   case 0:

      switch (obj->state2) {
      case 0:
         obj->state3 = rand() & 7;
         obj->state2++;
      // fallthrough
      case 1:
         if (--obj->state3 <= 0) {
            obj->state3 = 0;
            obj->state2 = 0;
            obj->state++;
            obj->d.sprite.animData = rockAnimData;
         }
         break;
      }

      break;

   case 1:
      x = obj->x1.n;
      y = obj->y1.n;
      z = obj->z1.n;

      obj->y1.n += obj->y2.n;
      obj->x1.n += obj->x2.n;
      obj->z1.n += obj->z2.n;
      obj->y2.n += obj->y3.n;

      particle = CreatePositionedObj(obj, OBJF_PARTICLE_710);
      particle->x1.n = obj->x1.n;
      particle->z1.n = obj->z1.n;
      particle->y1.n = obj->y1.n;
      particle->state2 = rand() % 8 + 10;
      particle->d.sprite.animData = gExplosionAnimData_800ffa80;
      particle->mem = 16;
      skip = rand() & 7;
      for (i = 0; i < skip; i++) {
         UpdateObjAnimation(particle);
      }

      particle->x2.n = -((x - obj->x1.n) >> 2) + ((rand() >> 2) % 10 - 5);
      particle->z2.n = -((z - obj->z1.n) >> 2) + ((rand() >> 2) % 10 - 5);
      particle->y2.n = -((y - obj->y1.n) >> 2);

      UpdateObjAnimation(obj);
      AddObjPrim6(gGraphicsPtr->ot, obj, 0);
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      setRGB0(poly, 240, 128, 128);

      if (obj->y1.n < GetTerrainElevation(obj->z1.s.hi, obj->x1.s.hi)) {
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}
