/* Battle presentation objects (segment 0x11604): the level-up jingle/stats object, the
 * impact bounce-zoom, the two battle cameras, the event zoom, the end-of-move direction
 * chooser and its direction-arrow renderer.
 *
 * Cameras: Objf017_AttackCamera is the attack close-up -- swoop onto the actor (zoom 250,
 * pitch 33.75deg), hold while the strike plays (gSignal5 handshake with the action
 * executor in battle_013b94.c), then restore the saved camera. Objf026_588_FocusCamera is
 * the general-purpose focus/follow camera used by spell FX and events: callers hand it a
 * target sprite, a vantage type (0-3 -> the GetBestViewOfTarget* variants in graphics.c)
 * and an optional zoom; the 588 table slot is the same handler with a wider default zoom
 * (350). */
#include "common.h"
#include "object.h"
#include "audio.h"
#include "battle.h"
#include "units.h"

//...
void RenderDirectionArrow(Object *, Object *, s16, u8, s16, s16, s16, s16);
//...

#undef OBJF
#define OBJF 571
void Objf571_LevelUp(Object *obj) {
   Object *sprite;
   s16 diff;

   sprite = OBJ.sprite;

   switch (obj->state) {
   case 0:
      PerformAudioCommand(0x50);
      PerformAudioCommand(AUDIO_CMD_PREPARE_XA(101));
      gPlayerControlSuppressed = 1;
      gSignal5 = 0;

      gCameraRotation.vy &= 0xfff;
      OBJ.dstRotY = GetBestViewOfTargetPlus90(sprite->z1.s.hi, sprite->x1.s.hi, 1);
      OBJ.camSavedX = gCameraPos.vx;
      OBJ.camSavedZ = gCameraPos.vz;
      OBJ.camSavedY = gCameraPos.vy;

      OBJ.camSavedRotX = gCameraRotation.vx;
      OBJ.camSavedRotZ = gCameraRotation.vz;
      OBJ.camSavedRotY = gCameraRotation.vy;

      OBJ.camSavedZoom = gCameraZoom.vz;
      OBJ.dstZoom = 250;

      diff = OBJ.dstRotY - gCameraRotation.vy;
      if (diff > 0) {
         if (diff > DEG(180)) {
            OBJ.dstRotY -= DEG(360);
         }
      } else if (diff < DEG(-180)) {
         OBJ.dstRotY += DEG(360);
      }

      OBJ.timer = 45;
      OBJ.savedGeomOfsY = gGeomOffsetY;
      OBJ.geomOfsY = gGeomOffsetY;
      OBJ.dstGeomOfsY = 190;
      obj->state++;
      // fallthrough
   case 1:
      if (OBJ.timer == 12) {
         PerformAudioCommand(AUDIO_CMD_PLAY_XA(101));
      }
      if (--OBJ.timer != 0) {
         gCameraPos.vx += (-(sprite->x1.n >> 3) - gCameraPos.vx) >> 2;
         gCameraPos.vz += (-(sprite->z1.n >> 3) - gCameraPos.vz) >> 2;
         gCameraPos.vy += ((sprite->y1.n >> 3) - gCameraPos.vy) >> 2;

         gCameraRotation.vy += (OBJ.dstRotY - gCameraRotation.vy) >> 3;
         gCameraRotation.vy += (OBJ.dstRotY - gCameraRotation.vy) >> 4;
         gCameraRotation.vx += (DEG(33.75) - gCameraRotation.vx) >> 2;
         gCameraRotation.vz += (0 - gCameraRotation.vz) >> 2;

         gCameraZoom.vz += (OBJ.dstZoom - gCameraZoom.vz) >> 2;

         OBJ.geomOfsY += (OBJ.dstGeomOfsY - OBJ.geomOfsY) >> 3;
         SetGeomOffset(160, OBJ.geomOfsY);
         gGeomOffsetY = OBJ.geomOfsY;
      } else {
         OBJ_TILE_STATE(sprite).action = TA_LEVEL_UP;
         gSignal3 = 0;
         obj->state++;
      }
      break;
   case 2:
      if (gSignal3 != 0) {
         OBJ.timer = 1;
         obj->state++;
      }
      break;
   case 3:
      if (--OBJ.timer == 0) {
         OBJ.timer = 25;
         obj->state++;
         gCameraRotation.vy &= 0xfff;

         diff = OBJ.camSavedRotY - gCameraRotation.vy;
         if (diff > 0) {
            if (diff > DEG(180)) {
               OBJ.camSavedRotY -= DEG(360);
            }
         } else if (diff < DEG(-180)) {
            OBJ.camSavedRotY += DEG(360);
         }
      }
      break;
   case 4:
      if (--OBJ.timer != 0) {
         gCameraPos.vx += (OBJ.camSavedX - gCameraPos.vx) >> 2;
         gCameraPos.vz += (OBJ.camSavedZ - gCameraPos.vz) >> 2;
         gCameraPos.vy += (OBJ.camSavedY - gCameraPos.vy) >> 2;

         gCameraRotation.vx += (OBJ.camSavedRotX - gCameraRotation.vx) >> 2;
         gCameraRotation.vz += (OBJ.camSavedRotZ - gCameraRotation.vz) >> 2;
         gCameraRotation.vy += (OBJ.camSavedRotY - gCameraRotation.vy) >> 2;

         gCameraZoom.vz += (OBJ.camSavedZoom - gCameraZoom.vz) >> 2;

         OBJ.geomOfsY += (OBJ.savedGeomOfsY - OBJ.geomOfsY) >> 2;
         SetGeomOffset(160, OBJ.geomOfsY);
         gGeomOffsetY = OBJ.geomOfsY;
      } else {
         gCameraRotation.vx = OBJ.camSavedRotX;
         gCameraRotation.vz = OBJ.camSavedRotZ;
         gCameraRotation.vy = OBJ.camSavedRotY;
         gCameraZoom.vz = OBJ.camSavedZoom;
         SetGeomOffset(160, OBJ.savedGeomOfsY);
         gGeomOffsetY = OBJ.savedGeomOfsY;
         gSignal5 = 100;
         gPlayerControlSuppressed = 0;
         obj->functionIndex = OBJF_NULL;
         PerformAudioCommand(0x51);
      }
   }
}

#undef OBJF
#define OBJF 024
void Objf024_BounceZoom(Object *obj) {
   s16 tmp;

   switch (obj->state) {
   case 0:
      OBJ.savedZoom = gCameraZoom.vz;
      // ?: no state++
   // fallthrough
   case 1:
      OBJ.todo_x26 += DEG(8.4375);
      if (OBJ.soft) {
         OBJ.todo_x28 = (rcos(OBJ.todo_x26 & 0xfff) * 5) * 2 >> 12;
      } else {
         OBJ.todo_x28 = (rcos(OBJ.todo_x26 & 0xfff) * 35) * 2 >> 12;
      }
      OBJ.todo_x2a += 1000;
      if (OBJ.soft) {
         tmp = (rcos(OBJ.todo_x2a & 0xfff) * OBJ.todo_x28) >> 12;
         gCameraZoom.vz = tmp + 320;
      } else {
         tmp = (rcos(OBJ.todo_x2a & 0xfff) * OBJ.todo_x28) >> 12;
         gCameraZoom.vz = tmp + 350;
      }
      if (OBJ.todo_x26 >= DEG(90)) {
         gCameraZoom.vz = OBJ.savedZoom;
         obj->functionIndex = OBJF_NULL;
      }
   }
}

#undef OBJF
#define OBJF 017
void Objf017_AttackCamera(Object *obj) {
   Object *sprite;
   s16 diff;

   sprite = OBJ.sprite;

   switch (obj->state) {
   case 0:
      gCameraRotation.vy &= 0xfff;
      gPlayerControlSuppressed = 1;
      gSignal5 = 0;
      // ?: LO byte of camSavedX doubles as a caller-set arg for specifying melee/ranged
      if (LO(OBJ.camSavedX) != 0) {
         // ?: melee
         OBJ.dstCamRotY = GetBestViewOfTargetPlus90(sprite->z1.s.hi, sprite->x1.s.hi, 0);
      } else {
         // ?: ranged
         OBJ.dstCamRotY = GetBestViewOfTargetMinus90(sprite->z1.s.hi, sprite->x1.s.hi, 1);
      }
      OBJ.camSavedX = gCameraPos.vx;
      OBJ.camSavedZ = gCameraPos.vz;
      OBJ.camSavedY = gCameraPos.vy;
      OBJ.camSavedRotX = gCameraRotation.vx;
      OBJ.camSavedRotZ = gCameraRotation.vz;
      OBJ.camSavedRotY = gCameraRotation.vy;
      OBJ.camSavedZoom = gCameraZoom.vz;
      OBJ.dstZoom = 250;

      diff = OBJ.dstCamRotY - gCameraRotation.vy;
      if (diff > 0) {
         if (diff > DEG(180)) {
            OBJ.dstCamRotY -= DEG(360);
         }
      } else if (diff < DEG(-180)) {
         OBJ.dstCamRotY += DEG(360);
      }

      OBJ.timer = 35;
      OBJ.savedGeomOfsY = gGeomOffsetY;
      OBJ.geomOfsY = gGeomOffsetY;
      OBJ.dstGeomOfsY = 150;
      obj->state++;
   // fallthrough
   case 1:
      if (--OBJ.timer != 0) {
         gCameraPos.vx += (-(sprite->x1.n >> 3) - gCameraPos.vx) >> 2;
         gCameraPos.vz += (-(sprite->z1.n >> 3) - gCameraPos.vz) >> 2;
         gCameraPos.vy += ((sprite->y1.n >> 3) - gCameraPos.vy) >> 2;
         gCameraRotation.vy += (OBJ.dstCamRotY - gCameraRotation.vy) >> 3;
         gCameraRotation.vy += (OBJ.dstCamRotY - gCameraRotation.vy) >> 4;
         gCameraRotation.vx += (DEG(33.75) - gCameraRotation.vx) >> 2;
         gCameraRotation.vz += (0 - gCameraRotation.vz) >> 2;
         gCameraZoom.vz += (OBJ.dstZoom - gCameraZoom.vz) >> 2;
         OBJ.geomOfsY += (OBJ.dstGeomOfsY - OBJ.geomOfsY) >> 2;
         SetGeomOffset(160, OBJ.geomOfsY);
         gGeomOffsetY = OBJ.geomOfsY;
      } else {
         gSignal5 = 1;
         obj->state++;
      }
      break;
   case 2:
      if (gSignal5 == 99) {
         obj->state++;
         OBJ.timer = 20;
         gCameraRotation.vy &= 0xfff;

         diff = OBJ.camSavedRotY - gCameraRotation.vy;
         if (diff > 0) {
            if (diff > DEG(180)) {
               OBJ.camSavedRotY -= DEG(360);
            }
         } else if (diff < DEG(-180)) {
            OBJ.camSavedRotY += DEG(360);
         }
      }
      break;
   case 3:
      if (--OBJ.timer != 0) {
         gCameraPos.vx += (OBJ.camSavedX - gCameraPos.vx) >> 2;
         gCameraPos.vz += (OBJ.camSavedZ - gCameraPos.vz) >> 2;
         gCameraPos.vy += (OBJ.camSavedY - gCameraPos.vy) >> 2;
         gCameraRotation.vx += (OBJ.camSavedRotX - gCameraRotation.vx) >> 2;
         gCameraRotation.vz += (OBJ.camSavedRotZ - gCameraRotation.vz) >> 2;
         gCameraRotation.vy += (OBJ.camSavedRotY - gCameraRotation.vy) >> 2;
         gCameraZoom.vz += (OBJ.camSavedZoom - gCameraZoom.vz) >> 2;
         OBJ.geomOfsY += (OBJ.savedGeomOfsY - OBJ.geomOfsY) >> 2;
         SetGeomOffset(160, OBJ.geomOfsY);
         gGeomOffsetY = OBJ.geomOfsY;
      } else {
         gCameraRotation.vx = OBJ.camSavedRotX;
         gCameraRotation.vz = OBJ.camSavedRotZ;
         gCameraRotation.vy = OBJ.camSavedRotY;
         gCameraZoom.vz = OBJ.camSavedZoom;
         SetGeomOffset(160, OBJ.savedGeomOfsY);
         gGeomOffsetY = OBJ.savedGeomOfsY;
         gSignal5 = 100;
         gPlayerControlSuppressed = 0;
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 026
void Objf026_588_FocusCamera(Object *obj) {
   Object *target;
   s16 diff;

   target = OBJ.target;

#ifdef PC_PORT
   /* PC_PORT (Stage 2.3): OBJ.target (the followed unit's sprite) can be NULL when the
    * camera targets an empty/vacated tile. PSX / the 2.2 fault handler read 0 through NULL;
    * target is read-only here (only target->{x1,z1,y1} are read), so redirect NULL to a
    * zero Object -- every field then reads 0, bit-identical to the handler's per-read
    * zeroing (read-0, NOT skip: the camera still eases toward origin). NULL sites:
    * battle_011604.c:296/328/329/333. See exchange/56. */
   {
      static const Object s_nullObj = {0};
      if (target == NULL) target = (Object *)&s_nullObj;
   }
#endif

   switch (obj->state) {
   case 0:
      OBJ.todo_x44 = !OBJ.todo_x44;
      gCameraRotation.vy &= 0xfff;

      if (OBJ.zoom == 0) {
         OBJ.dstZoom = 250;
         if (obj->functionIndex == OBJF_FOCUS_CAMERA_WIDE) {
            OBJ.dstZoom = 350;
         }
      } else {
         OBJ.dstZoom = OBJ.zoom;
      }

      switch (OBJ.type) {
      case 0:
         OBJ.dstCamRotY = GetBestViewOfTarget(target->z1.s.hi, target->x1.s.hi, 1);
         break;
      case 1:
         OBJ.dstCamRotY = GetBestViewOfTargetPlus90(target->z1.s.hi, target->x1.s.hi, OBJ.todo_x44);
         break;
      case 2:
         OBJ.dstCamRotY = GetBestViewOfTargetMinus90(target->z1.s.hi, target->x1.s.hi, OBJ.todo_x44);
         break;
      case 3:
         OBJ.dstCamRotY = GetBestViewOfTargetPlus180(target->z1.s.hi, target->x1.s.hi, OBJ.todo_x44);
         break;
      }

      diff = OBJ.dstCamRotY - gCameraRotation.vy;
      if (diff > 0) {
         if (diff > DEG(180)) {
            OBJ.dstCamRotY -= DEG(360);
         }
      } else if (diff < DEG(-180)) {
         OBJ.dstCamRotY += DEG(360);
      }

      OBJ.timer = 35;
      OBJ.savedGeomOfsY = gGeomOffsetY;
      OBJ.geomOfsY = gGeomOffsetY;
      if (obj->functionIndex == OBJF_FOCUS_CAMERA_WIDE) {
         OBJ.dstGeomOfsY = gGeomOffsetY;
      } else {
         OBJ.dstGeomOfsY = 150;
      }
      obj->state++;

   // fallthrough
   case 1:
      if (--OBJ.timer != 0) {
         gCameraPos.vx += (-(target->x1.n >> 3) - gCameraPos.vx) >> 2;
         gCameraPos.vz += (-(target->z1.n >> 3) - gCameraPos.vz) >> 2;
         if (obj->functionIndex == OBJF_FOCUS_CAMERA_WIDE) {
            gCameraPos.vy += (((target->y1.n + CV(0.5)) >> 3) - gCameraPos.vy) >> 2;
         } else {
            gCameraPos.vy += ((target->y1.n >> 3) - gCameraPos.vy) >> 2;
         }
         gCameraRotation.vy += (OBJ.dstCamRotY - gCameraRotation.vy) >> 3;
         gCameraRotation.vy += (OBJ.dstCamRotY - gCameraRotation.vy) >> 4;
         gCameraRotation.vx += (DEG(33.75) - gCameraRotation.vx) >> 2;
         gCameraRotation.vz += (0 - gCameraRotation.vz) >> 2;
         gCameraZoom.vz += (OBJ.dstZoom - gCameraZoom.vz) >> 2;
         OBJ.geomOfsY += (OBJ.dstGeomOfsY - OBJ.geomOfsY) >> 2;
         SetGeomOffset(160, OBJ.geomOfsY);
         gGeomOffsetY = OBJ.geomOfsY;
      } else {
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 410
void Objf410_EventZoom(Object *obj) {
   switch (obj->state) {
   case 0:
      OBJ.timer = 35;
      obj->state++;
   // fallthrough
   case 1:
      if (--OBJ.timer != 0) {
         gCameraZoom.vz += (OBJ.zoom - gCameraZoom.vz) >> 2;
      } else {
         obj->functionIndex = OBJF_NULL;
      }
   }
}

#undef OBJF
#define OBJF 016
void Objf016_ChooseDoneDirection(Object *obj) {
   s16 dir;
   s16 fInner, fOuter, nfInner, nfOuter;
   Object *otherObj;

   dir = 0;

   switch (obj->state) {
   case 0:
      otherObj = OBJ.unit->battler;
      otherObj = otherObj->d.objf014.sprite;
      gSignal4 = otherObj->d.sprite.direction;
      obj->x1.n = otherObj->x1.n;
      obj->z1.n = otherObj->z1.n;
      obj->y1.n = otherObj->y1.n + CV(0.5);
      obj->state++;
   // fallthrough
   case 1:
      switch ((gCameraRotation.vy & 0xfff) >> 10) {
      case CAM_DIR_SOUTH:
         dir = DEG(0);
         break;
      case CAM_DIR_EAST:
         dir = DEG(270);
         break;
      case CAM_DIR_NORTH:
         dir = DEG(180);
         break;
      case CAM_DIR_WEST:
         dir = DEG(90);
         break;
      }

      if (gPadStateNewPresses & PAD_UP) {
         gSignal4 = dir + DEG(0);
      }
      if (gPadStateNewPresses & PAD_RIGHT) {
         gSignal4 = dir + DEG(90);
      }
      if (gPadStateNewPresses & PAD_DOWN) {
         gSignal4 = dir + DEG(180);
      }
      if (gPadStateNewPresses & PAD_LEFT) {
         gSignal4 = dir + DEG(270);
      }
      gSignal4 &= 0xfff;

      if (gPadStateNewPresses & PAD_X) {
         gSignal2 = 2;
         obj->functionIndex = OBJF_NULL;
         return;
      }

      OBJ.angle += DEG(45);

      fInner = (rcos(OBJ.angle & 0xfff) * 50 >> 12) + CV(0.75);
      fOuter = (rcos(OBJ.angle & 0xfff) * 130 >> 12) + CV(2.0);
      nfInner = (rcos(OBJ.angle & 0xfff) * 25 >> 12) + CV(0.75);
      nfOuter = (rcos(OBJ.angle & 0xfff) * 50 >> 12) + CV(1.5);
      otherObj = Obj_GetUnused();

      RenderDirectionArrow(obj, otherObj, DEG(0), gSignal4 == DEG(0), fInner, fOuter, nfInner,
                           nfOuter);
      RenderDirectionArrow(obj, otherObj, DEG(90), gSignal4 == DEG(90), fInner, fOuter, nfInner,
                           nfOuter);
      RenderDirectionArrow(obj, otherObj, DEG(180), gSignal4 == DEG(180), fInner, fOuter, nfInner,
                           nfOuter);
      RenderDirectionArrow(obj, otherObj, DEG(270), gSignal4 == DEG(270), fInner, fOuter, nfInner,
                           nfOuter);
   }

   if (gPadStateNewPresses & PAD_CIRCLE) {
      gSignal2 = 1;
      obj->functionIndex = OBJF_NULL;
      return;
   }
}

void RenderDirectionArrow(Object *owner, Object *sprite, s16 angle, u8 filled, s16 fInner,
                          s16 fOuter, s16 nfInner, s16 nfOuter) {
   s16 angle2, angle3, angle0, angle1;
   s16 inner, outer;

   if (filled) {
      angle2 = angle - DEG(33.75);
      angle3 = angle + DEG(33.75);
      angle0 = angle - DEG(11.25);
      angle1 = angle + DEG(11.25);
      inner = fInner;
      outer = fOuter;
      sprite->d.sprite.gfxIdx = GFX_DIRECTION_ARROW_FILLED;
   } else {
      angle2 = angle - DEG(16.875);
      angle3 = angle + DEG(16.875);
      angle0 = angle - DEG(8.4375);
      angle1 = angle + DEG(8.4375);
      inner = nfInner;
      outer = nfOuter;
      sprite->d.sprite.gfxIdx = GFX_DIRECTION_ARROW_EMPTY;
   }

   sprite->d.sprite.coords[0].x = owner->x1.n + (rcos((angle0 - DEG(90)) & 0xfff) * outer >> 12);
   sprite->d.sprite.coords[0].z = owner->z1.n + (rcos(angle0 & 0xfff) * outer >> 12);

   sprite->d.sprite.coords[2].x = owner->x1.n + (rcos((angle2 - DEG(90)) & 0xfff) * inner >> 12);
   sprite->d.sprite.coords[2].z = owner->z1.n + (rcos(angle2 & 0xfff) * inner >> 12);

   sprite->d.sprite.coords[1].x = owner->x1.n + (rcos((angle1 - DEG(90)) & 0xfff) * outer >> 12);
   sprite->d.sprite.coords[1].z = owner->z1.n + (rcos(angle1 & 0xfff) * outer >> 12);

   sprite->d.sprite.coords[3].x = owner->x1.n + (rcos((angle3 - DEG(90)) & 0xfff) * inner >> 12);
   sprite->d.sprite.coords[3].z = owner->z1.n + (rcos(angle3 & 0xfff) * inner >> 12);

   sprite->d.sprite.coords[0].y = owner->y1.n;
   sprite->d.sprite.coords[1].y = owner->y1.n;
   sprite->d.sprite.coords[2].y = owner->y1.n;
   sprite->d.sprite.coords[3].y = owner->y1.n;

   sprite->x1.n = sprite->d.sprite.coords[2].x;
   sprite->z1.n = sprite->d.sprite.coords[2].z;
   sprite->y1.n = sprite->d.sprite.coords[2].y;
   sprite->d.sprite.otOfs = 10;
   AddObjPrim5(gGraphicsPtr->ot, sprite);
}
