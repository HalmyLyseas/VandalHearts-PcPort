#include "common.h"
#include "audio.h"
#include "state.h"
#include "object.h"
#include "units.h"
#include "window.h"
#include "graphics.h"
#include "battle.h"
#include "field.h"
#include "card.h"

#include "PsyQ/kernel.h"

void UpdateEngine(void);
void Objf047_BattleMapCursor(Object *);
void Objf049_BattleMapCursorControl(Object *);
void Objf019_Compass(Object *);
void Objf025_OverheadMapView(Object *);
void Noop_800190c0(void);
void CalculateSubstepDeltas(s32, s32, s32, s32, s32, u32, s32 *, s32 *);
s32 SmoothStepTo(Object *, s32, s32, s32);

void UpdateEngine(void) {
   s32 flag;

   UpdateAudio();
   rand();
   if (gState.vsyncMode == 2) {
      gState.frameCounter += 2;
   } else {
      gState.frameCounter += 1;
   }
   if (gState.debug) {
      // While holding 2P Circle to freeze, press 2P R1 to frame-step
      while ((PadRead(1) >> 0x10) & PAD_CIRCLE) {
         UpdateInput();
         if (gPad2StateNewPresses & PAD_R1)
            break;
      }
      // 1P Select: setup transition to state 98 (debug menu)
      if (gState.debug && gPadStateNewPresses & PAD_SELECT) {
         PerformAudioCommand(AUDIO_CMD_STOP_ALL);
         gState.primary = STATE_1;
      }
      // 2P Start: win battle
      if (gState.debug && (gPad2StateNewPresses & PAD_START)) {
         gState.battleEval = BATTLE_EVAL_VICTORY;
      }
   }
   gState.unitMarkerSpin += 0x20;
   gOscillation = (gOscillation + 0x100) & 0xfff;
   gGridColorOscillation = rcos(gOscillation) * 100 >> 12;
   gGridColorOscillation += 155U;
   gQuadIndex = 0;
   UpdateInput();
   UpdateCamera();
   UpdateLight(0x38);
   gGraphicsPtr = (gGraphicsPtr == &gGraphicBuffers[0]) ? &gGraphicBuffers[1] : &gGraphicBuffers[0];
   ClearOTag(gGraphicsPtr->ot, OT_SIZE);
   CenterCamera(0);
   RotTrans(&gCameraPos, (VECTOR *)gCameraMatrix.t, &flag);
   SetTransMatrix(&gCameraMatrix);
   if (!gState.fieldRenderingDisabled) {
      RenderField();
   }
   Obj_Execute();
   DrawSync(1);
   FntPrint("VSYNC : %d\n", GetRCnt(RCntCNT1));
   DrawSync(0);
   DecodeUnitSprites();
   if (gState.vsyncNoWait && (gPad2State & PAD_L1)) {
      VSync(1);
   } else {
      VSync(gState.vsyncMode);
   }
   ResetRCnt(RCntCNT1);
   PutDrawEnv(&gGraphicsPtr->drawEnv);
   PutDispEnv(&gGraphicsPtr->dispEnv);
   DrawOTag(gGraphicsPtr->ot);
   FntFlush(-1);
}

u16 s_mapCursorGhostY;

void Objf047_BattleMapCursor(Object *obj) {
   // static u16 mapCursorGhostY = 0;
   // extern s16 gMapCursorAnimData[];
   static s16 mapCursorAnimData[] = {0, GFX_MAP_CURSOR_1,
                                     1, GFX_MAP_CURSOR_2,
                                     1, GFX_MAP_CURSOR_3,
                                     1, GFX_MAP_CURSOR_4,
                                     1, GFX_MAP_CURSOR_5,
                                     1, GFX_MAP_CURSOR_6,
                                     1, 0,
                                     1, 0};
   s16 i;
   s16 x, z;

   if (!gIsEnemyTurn && !gPlayerControlSuppressed && !gClearSavedPadState &&
       !gMapCursorSuppressed) {

      if (gState.mapCursorOutOfRange) {
         obj->d.sprite.clut = CLUT_REDS;
      } else {
         obj->d.sprite.clut = CLUT_NULL;
      }

      obj->d.sprite.animData = mapCursorAnimData;

      z = gMapCursorZ;
      x = gMapCursorX;

      obj->d.sprite.coords[0].x = x * CV(1.0);
      obj->d.sprite.coords[2].x = x * CV(1.0);
      obj->d.sprite.coords[1].x = x * CV(1.0) + (CV(1.0) - 1);
      obj->d.sprite.coords[3].x = x * CV(1.0) + (CV(1.0) - 1);

      obj->d.sprite.coords[0].z = z * CV(1.0);
      obj->d.sprite.coords[2].z = z * CV(1.0) + (CV(1.0) - 1);
      obj->d.sprite.coords[1].z = z * CV(1.0);
      obj->d.sprite.coords[3].z = z * CV(1.0) + (CV(1.0) - 1);

      obj->x1.s.hi = x;
      obj->z1.s.hi = z;

      if (gTerrainPtr[z][x].s.terrain != TERRAIN_NO_ENTRY) {
         SetElevationFromTerrain(obj);
      } else {
         obj->y1.n = gMapRowPointers[z][x].height * CV(0.5);
      }

      obj->d.sprite.coords[0].y = obj->y1.n;
      obj->d.sprite.coords[1].y = obj->y1.n;
      obj->d.sprite.coords[2].y = obj->y1.n;
      obj->d.sprite.coords[3].y = obj->y1.n;

      obj->x1.s.lo = CV(0.5);
      obj->z1.s.lo = CV(0.5);

      obj->d.sprite.otOfs = -2;
      obj->d.sprite.semiTrans = 0;
      UpdateObjAnimation(obj);
      AddObjPrim5(gGraphicsPtr->ot, obj);

      if (gCameraRotation.vx <= DEG(67.5)) {
         obj->d.sprite.semiTrans = 4;

         s_mapCursorGhostY += 18;
         s_mapCursorGhostY &= 0xff;

         obj->d.sprite.coords[0].y += s_mapCursorGhostY;
         obj->d.sprite.coords[1].y += s_mapCursorGhostY;
         obj->d.sprite.coords[2].y += s_mapCursorGhostY;
         obj->d.sprite.coords[3].y += s_mapCursorGhostY;

         for (i = 0; i < 13; i++) {
            obj->y1.n = obj->d.sprite.coords[0].y;
            AddObjPrim5(gGraphicsPtr->ot, obj);
            obj->d.sprite.coords[0].y += CV(1.0);
            obj->d.sprite.coords[1].y += CV(1.0);
            obj->d.sprite.coords[2].y += CV(1.0);
            obj->d.sprite.coords[3].y += CV(1.0);
         }
      }
   }
}

void Objf049_BattleMapCursorControl(Object *obj) {
   static s32 mapCursorSavedPadState = 0;
   static s32 mapCursorPadHoldCounter = 0;
   u32 pad;

   if (gState.signal != 0) {
      return;
   }
   if (gSavedPadState & PAD_X) {
      // holding X jump-starts the counter, accelerating cursor movement
      mapCursorPadHoldCounter = 30;
   }

   pad = gSavedPadStateNewPresses;

   if (mapCursorSavedPadState == gSavedPadState) {
      mapCursorPadHoldCounter++;
   } else {
      // input changed: save new value and reset counter
      mapCursorSavedPadState = gSavedPadState;
      mapCursorPadHoldCounter = 0;
   }
   if (mapCursorPadHoldCounter > 30) {
      // repeat saved input; UpdateInput() repeats d-pad input as well, so this just gives an
      // intermittent "bump" (besides enabling the x-button boost)
      pad = gSavedPadState;
      mapCursorPadHoldCounter = 18;
   }

   SetElevationFromTerrain(obj);

   switch (obj->state) {
   case 0:
      obj->x1.s.hi = gMapCursorStartingPos[gState.mapNum].x;
      obj->z1.s.hi = gMapCursorStartingPos[gState.mapNum].z;

      if (gState.primary == STATE_LOAD_IN_BATTLE_SAVE) {
         obj->x1.s.hi = gDeferredInBattleSaveData.mapCursorX;
         obj->z1.s.hi = gDeferredInBattleSaveData.mapCursorZ;
      }
      // snap to tile center
      obj->x1.s.lo = CV(0.5);
      obj->z1.s.lo = CV(0.5);

      gMapCursorX = 0;
      gMapCursorZ = 0;

      for (gMapCursorX = 0; gMapCursorX <= obj->x1.s.hi; gMapCursorX++) {
         CenterCamera(1);
      }
      for (gMapCursorZ = 0; gMapCursorZ <= obj->z1.s.hi; gMapCursorZ++) {
         CenterCamera(1);
      }

      obj->state++;
      break;
   case 1:
      switch ((gCameraRotation.vy & 0xfff) >> 10) {
      case CAM_DIR_SOUTH:
         if (pad & PAD_UP) {
            obj->z1.n += CV(1.0);
         }
         if (pad & PAD_DOWN) {
            obj->z1.n -= CV(1.0);
         }
         if (pad & PAD_LEFT) {
            obj->x1.n -= CV(1.0);
         }
         if (pad & PAD_RIGHT) {
            obj->x1.n += CV(1.0);
         }
         break;
      case CAM_DIR_EAST:
         if (pad & PAD_UP) {
            obj->x1.n -= CV(1.0);
         }
         if (pad & PAD_DOWN) {
            obj->x1.n += CV(1.0);
         }
         if (pad & PAD_LEFT) {
            obj->z1.n -= CV(1.0);
         }
         if (pad & PAD_RIGHT) {
            obj->z1.n += CV(1.0);
         }
         break;
      case CAM_DIR_NORTH:
         if (pad & PAD_UP) {
            obj->z1.n -= CV(1.0);
         }
         if (pad & PAD_DOWN) {
            obj->z1.n += CV(1.0);
         }
         if (pad & PAD_LEFT) {
            obj->x1.n += CV(1.0);
         }
         if (pad & PAD_RIGHT) {
            obj->x1.n -= CV(1.0);
         }
         break;
      case CAM_DIR_WEST:
         if (pad & PAD_UP) {
            obj->x1.n += CV(1.0);
         }
         if (pad & PAD_DOWN) {
            obj->x1.n -= CV(1.0);
         }
         if (pad & PAD_LEFT) {
            obj->z1.n += CV(1.0);
         }
         if (pad & PAD_RIGHT) {
            obj->z1.n -= CV(1.0);
         }
         break;
      }
   }

   if (obj->x1.s.hi < gMapMinX) {
      obj->x1.s.hi = gMapMinX;
   }
   if (obj->x1.s.hi > gMapMaxX) {
      obj->x1.s.hi = gMapMaxX;
   }
   if (obj->z1.s.hi < gMapMinZ) {
      obj->z1.s.hi = gMapMinZ;
   }
   if (obj->z1.s.hi > gMapMaxZ) {
      obj->z1.s.hi = gMapMaxZ;
   }

   gMapCursorX = obj->x1.s.hi;
   gMapCursorZ = obj->z1.s.hi;

   if (!gPlayerControlSuppressed && !gIsEnemyTurn) {
      gCameraPos.vy += ((obj->y1.n >> 3) - gCameraPos.vy) >> 2;
   }
}

void Objf019_Compass(Object *obj) {
   static VECTOR compassOffset = {0, 0, 0x300, 0};
   static Quad compassQuad = {{48, 0, -48, 0}, {-48, 0, -48, 0}, {48, 0, 48, 0}, {-48, 0, 48, 0}};
   static Quad compassLetterQuads[4] = {
       {{-48, 0, -12, 0}, {-72, 0, -12, 0}, {-48, 0, 12, 0}, {-72, 0, 12, 0}},
       {{72, 0, -12, 0}, {48, 0, -12, 0}, {72, 0, 12, 0}, {48, 0, 12, 0}},
       {{12, 0, 48, 0}, {-12, 0, 48, 0}, {12, 0, 75, 0}, {-12, 0, 72, 0}},
       {{12, 0, -60, 0}, {-12, 0, -60, 0}, {12, 0, -36, 0}, {-12, 0, -36, 0}}};

   POLY_FT4 *shadowPoly;
   POLY_FT4 *compassPoly;
   // POLY_FT4 *letterPoly;
   u8 x, y, w, h;
   u16 tpage, clut;
   s32 p, flag, i;
   s32 gfx;
   Quad *quadp;

   if (!gState.inEvent) {
      PushMatrix();
      SetGeomOffset(270, 50);
      TransMatrix(&gCameraMatrix, &compassOffset);
      SetTransMatrix(&gCameraMatrix);

      x = gGfxSubTextures[GFX_COMPASS][0];
      y = gGfxSubTextures[GFX_COMPASS][1];
      w = gGfxSubTextures[GFX_COMPASS][2];
      h = gGfxSubTextures[GFX_COMPASS][3];

      compassPoly = &gGraphicsPtr->quads[gQuadIndex++];
      shadowPoly = &gGraphicsPtr->quads[gQuadIndex++];

      compassPoly->clut = shadowPoly->clut = gGfxClutIds[GFX_COMPASS];
      compassPoly->tpage = shadowPoly->tpage = gGfxTPageIds[GFX_COMPASS];

      setUVWH(compassPoly, x, y, w, h);
      setUVWH(shadowPoly, x, y, w, h);

      RotTransPers4(&compassQuad[0], &compassQuad[1], &compassQuad[2], &compassQuad[3],
                    (s32 *)&compassPoly->x0, (s32 *)&compassPoly->x1, (s32 *)&compassPoly->x2,
                    (s32 *)&compassPoly->x3, &p, &flag);
      // setcode(compassPoly, 0x2c); setRGB0(compassPoly, 0x80, 0x80, 0x80);
      *(u32 *)&compassPoly->r0 = POLY_RGB_CODE(GPU_CODE_POLY_FT4, 0x80, 0x80, 0x80);
      addPrim(&gGraphicsPtr->ot[OT_SIZE - 5], compassPoly);

      // Shadow
      SetGeomOffset(270, 58);
      RotTransPers4(&compassQuad[0], &compassQuad[1], &compassQuad[2], &compassQuad[3],
                    (s32 *)&shadowPoly->x0, (s32 *)&shadowPoly->x1, (s32 *)&shadowPoly->x2,
                    (s32 *)&shadowPoly->x3, &p, &flag);
      *(u32 *)&shadowPoly->r0 = POLY_RGB_CODE(GPU_CODE_POLY_FT4, 0, 0, 0);
      addPrim(&gGraphicsPtr->ot[OT_SIZE - 6], shadowPoly);

      // E/W/S/N
      SetGeomOffset(270, 50);
      compassPoly = &gGraphicsPtr->quads[gQuadIndex];
      clut = gGfxClutIds[GFX_E];
      tpage = gGfxTPageIds[GFX_E];
      gfx = GFX_E;
      quadp = &compassLetterQuads[0];
      for (i = 0; i < 4; i++, gfx++, quadp++) {
         compassPoly->clut = clut;
         compassPoly->tpage = tpage;

         x = gGfxSubTextures[gfx][0];
         y = gGfxSubTextures[gfx][1];
         w = gGfxSubTextures[gfx][2];
         h = gGfxSubTextures[gfx][3];
         setUVWH(compassPoly, x, y, w, h);

         RotTransPers4(&(*quadp)[0], &(*quadp)[1], &(*quadp)[2], &(*quadp)[3],
                       (s32 *)&compassPoly->x0, (s32 *)&compassPoly->x1, (s32 *)&compassPoly->x2,
                       (s32 *)&compassPoly->x3, &p, &flag);
         *(u32 *)&compassPoly->r0 = POLY_RGB_CODE(GPU_CODE_POLY_FT4, 0x80, 0x80, 0x80);
         addPrim(&gGraphicsPtr->ot[OT_SIZE - 5], compassPoly);

         compassPoly++;
         gQuadIndex++;
      }
      SetGeomOffset(gGeomOffsetX, gGeomOffsetY);
      PopMatrix();
   }
}

#undef OBJF
#define OBJF 025
void Objf025_OverheadMapView(Object *obj) {
   static s16 overheadMapZoomLevels[BATTLE_CT] = {
       1300, 1500, 1700, 1350, 1550, 3550, 1500, 1500, 1900, 1500, 1400, 1500, 1500, 1450, 1500,
       1650, 1450, 1600, 2200, 1900, 2400, 1700, 1400, 1400, 1600, 1650, 2750, 1500, 1850, 2350,
       1950, 2000, 3550, 1850, 2000, 1900, 2000, 1750, 1950, 1650, 2550, 1450, 1550, 1650};

   switch (obj->state) {
   case 0:
      CloseWindow(0x1e);
      gOverheadMapState = 1;
      gCameraRotation.vy &= 0xfff;
      OBJ.camSavedRotX = gCameraRotation.vx;
      OBJ.camSavedRotY = gCameraRotation.vy;
      OBJ.camSavedZoom = gCameraZoom.vz;
      OBJ.camSavedX = gCameraPos.vx;
      OBJ.camSavedY = gCameraPos.vy;
      OBJ.camSavedZ = gCameraPos.vz;
      obj->state++;
      // fallthrough
   case 1:
      gPlayerControlSuppressed = 1;
      obj->state++;
      break;
   case 2:
      gCameraZoom.vz += (overheadMapZoomLevels[gState.mapNum] - gCameraZoom.vz) >> 2;
      gCameraRotation.vx += (DEG(90) - gCameraRotation.vx) >> 2;
      gCameraRotation.vy += (DEG(180) - gCameraRotation.vy) >> 2;
      gCameraPos.vx += (((-(gMapMaxX + 1) / 2) * 32) - gCameraPos.vx) >> 2;
      gCameraPos.vz += (((-(gMapMaxZ + 1) / 2) * 32) - gCameraPos.vz) >> 2;
      gCameraPos.vy += -gCameraPos.vy >> 2;

      if ((gPadStateNewPresses & PAD_TRIANGLE) || (gPadStateNewPresses & PAD_X)) {
         gOverheadMapState = 0;
         OBJ.delay = 25;
         obj->state++;
      }
      break;
   case 3:
      gCameraRotation.vx += (OBJ.camSavedRotX - gCameraRotation.vx) >> 2;
      gCameraRotation.vy += (OBJ.camSavedRotY - gCameraRotation.vy) >> 2;
      gCameraZoom.vz += (OBJ.camSavedZoom - gCameraZoom.vz) >> 2;
      gCameraPos.vx += (OBJ.camSavedX - gCameraPos.vx) >> 2;
      gCameraPos.vz += (OBJ.camSavedZ - gCameraPos.vz) >> 2;
      gCameraPos.vy += (OBJ.camSavedY - gCameraPos.vy) >> 2;
      if (--OBJ.delay == 0) {
         gCameraRotation.vx = OBJ.camSavedRotX;
         gCameraRotation.vy = OBJ.camSavedRotY;
         gCameraZoom.vz = OBJ.camSavedZoom;
         gCameraPos.vx = OBJ.camSavedX;
         gCameraPos.vy = OBJ.camSavedY;
         gCameraPos.vz = OBJ.camSavedZ;
         gSignal1 = 99;
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

void Noop_800190c0(void) { Noop_800c72c0(); }

void CalculateSubstepDeltas(s32 currentXorZ, s32 currentY, s32 destXorZ, s32 destY, s32 rise,
                            u32 substeps, s32 *deltaXorZ, s32 *deltaY) {
   // rise is usually ~7000
   *deltaXorZ = (destXorZ - currentXorZ) / (s32)substeps;
   *deltaY = ((destY - currentY) + (((s32)substeps + 1) * (s32)substeps) / 2 * rise) / substeps;
}

typedef struct SubstepProperties {
   u8 substeps;
   s16 rise;
} SubstepProperties;

s32 s_rise_80123144;
s32 s_currentX_80123148;
s32 s_destX_8012314c;
s32 s_currentZ_80123150;
s32 s_destZ_80123154;
s32 s_currentY_80123158;
s32 s_destY_8012315c;
s32 s_deltaX_80123160;
s32 s_deltaY_80123164;
s32 s_deltaZ_80123168;
s16 s_isubstep_8012316c;
u8 s_substeps_80123170;

s32 SmoothStepTo(Object *sprite, s32 destZ, s32 destX, s32 cont) {
   // for each [elevation diff]
   static SubstepProperties ascendingSubsteps[5] = {
       {3, 7200}, {4, 7000}, {7, 7000}, {8, 7000}, {9, 7000}};
   static SubstepProperties levelOrDescendingSubsteps[5] = {
       {3, 7000}, {4, 7000}, {7, 7000}, {8, 7000}, {9, 7000}};
   s32 unused[3];
   s8 elevationDiff;
   u8 destElevation, currentElevation;
   s32 diff;

   if (cont == 0) {
      s_currentX_80123148 = sprite->x1.n << 8;
      s_destX_8012314c = destX << 8;

      s_currentY_80123158 = sprite->y1.n << 8;
      s_destY_8012315c = gTerrainPtr[destZ >> 8][destX >> 8].s.elevation << 8 << 7;

      s_currentZ_80123150 = sprite->z1.n << 8;
      s_destZ_80123154 = destZ << 8;

      destElevation = gTerrainPtr[destZ >> 8][destX >> 8].s.elevation;
      currentElevation = OBJ_TERRAIN(sprite).s.elevation;
      diff = destElevation - currentElevation;
      elevationDiff = diff;
      if (elevationDiff > 0) {
         // Climbing
         s_substeps_80123170 = ascendingSubsteps[elevationDiff].substeps;
         s_rise_80123144 = ascendingSubsteps[elevationDiff].rise;
      } else {
         // Descending or level
         elevationDiff = -elevationDiff;
         s_substeps_80123170 = levelOrDescendingSubsteps[elevationDiff].substeps;
         s_rise_80123144 = levelOrDescendingSubsteps[elevationDiff].rise;
      }

      if (s_currentX_80123148 != s_destX_8012314c) {
         // Moving along X axis
         CalculateSubstepDeltas(s_currentX_80123148, s_currentY_80123158, s_destX_8012314c,
                                s_destY_8012315c, s_rise_80123144, s_substeps_80123170,
                                &s_deltaX_80123160, &s_deltaY_80123164);
         s_deltaZ_80123168 = 0;
      } else {
         // Moving along Z axis
         CalculateSubstepDeltas(s_currentZ_80123150, s_currentY_80123158, s_destZ_80123154,
                                s_destY_8012315c, s_rise_80123144, s_substeps_80123170,
                                &s_deltaZ_80123168, &s_deltaY_80123164);
         s_deltaX_80123160 = 0;
      }
      s_isubstep_8012316c = 0;

      // For cont:0, return 1 where elevation is level
      return elevationDiff == 0;
   } else {
      s_currentX_80123148 += s_deltaX_80123160;
      s_currentZ_80123150 += s_deltaZ_80123168;

      s_deltaY_80123164 -= s_rise_80123144;
      s_currentY_80123158 += s_deltaY_80123164;

      sprite->x1.n = s_currentX_80123148 >> 8;
      sprite->z1.n = s_currentZ_80123150 >> 8;
      sprite->y1.n = s_currentY_80123158 >> 8;
      s_isubstep_8012316c++;

      if (s_isubstep_8012316c == s_substeps_80123170) {
         sprite->x1.n = s_destX_8012314c >> 8;
         sprite->z1.n = s_destZ_80123154 >> 8;
         sprite->y1.n = s_destY_8012315c >> 8;

         // For cont:1, return 1 when movement completes
         return 1;
      }
   }
   return 0;
}