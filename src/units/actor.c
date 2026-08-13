/* The battle-unit actor, plus the message-box front end, the sprite-motion helpers, and
 * the event-entity setup calls it shares with events/entities.c.
 *
 * MsgBox_ShowForSprite / MsgBox_SetPortrait / MsgBox_Close drive the upper (0x41/0x42) and
 * lower (0x43/0x44) dialogue windows, the tail anchored to a speaking sprite, and the
 * Objf413 portrait from units/roster.c.
 *
 * Objf014_BattleUnit (~1300 lines) is one object per unit on the map, paired with the
 * unit's sprite. obj->state selects the action, obj->state2 the step: 0 spawn, 1 idle +
 * player-controlled movement, 2 move, 5/9/10 struck and blocking, 6 spell casting, 8 melee
 * and 12 ranged attack, 15 high step, 16 level up, 7/13/17/18/19 the death variants
 * (TallySlainUnit, blood or rock spurt), 20 defeat speech. It sets the sprite's animIdx
 * from OBJ.animSet[animIdx + facingFront] and hands off to battle/executors.c through
 * gSignal2/3/4.
 *
 * Motion helpers: UpdateUnitSpriteOrientation (facing from camera rotation),
 * UpdateUnitSpriteMovement (walk, or jump-and-crouch on uneven terrain),
 * UpdateAirmanUnitSpriteMovement (flyers rise, cross, descend),
 * StepEntitySpriteTowardDest (the event-entity variant). Objf412_EventCamera eases the
 * camera toward gState.eventCameraRot and gState.focus / eventCameraPan.
 *
 * Event entities: SetupEventEntity[_SingleSet] binds one script from
 * gEvtEntityDataPointers plus two sprite strips to a new Objf409 (the interpreter itself
 * lives in events/entities.c); ReserveSprite MoveImages one 48x48 cell into a strip slot
 * (both called from events/scene_loaders.c). */
#include "common.h"
#include "object.h"
#include "window.h"
#include "units.h"
#include "battle.h"
#include "field.h"
#include "state.h"
#include "graphics.h"
#include "audio.h"

void MsgBox_ShowForSprite(Object *sprite, u8 lower, u8 omitTail) {
   Object *tail;

   if (!lower) {
      DrawWindow(0x41, 0, 0, 296, 64, 12, 16, WBS_ROUNDED, 0);
      DisplayCustomWindow(0x41, 0, 1, 3, 0, 25);
      DisplayCustomWindow(0x42, 0, 1, 3, 0, 25);
      if (!omitTail) {
         tail = Obj_GetUnused();
         tail->functionIndex = OBJF_UPPER_MSGBOX_TAIL;
         tail->d.objf421.sprite = sprite;
      }
   } else {
      DrawWindow(0x43, 0, 100, 296, 64, 12, 161, WBS_ROUNDED, 0);
      DisplayCustomWindow(0x43, 0, 1, 3, 0, 25);
      DisplayCustomWindow(0x44, 0, 1, 3, 0, 25);
      if (!omitTail) {
         tail = Obj_GetUnused();
         tail->functionIndex = OBJF_LOWER_MSGBOX_TAIL;
         tail->d.objf421.sprite = sprite;
      }
   }
}

void MsgBox_SetPortrait(s16 portraitId, u8 lower) {
   s32 i;
   Object *obj1;

   obj1 = gObjectArray;
   for (i = 0; i < OBJ_DATA_CT; i++) {
      if (obj1->functionIndex == OBJF_MSGBOX_PORTRAIT && obj1->d.objf413.flipped == lower) {
#ifdef PC_PORT
         /* PC_PORT (Stage 2.3): the portrait's face/speak/blink sprite pointers can be NULL
          * (a portrait with no speak/blink sub-sprite). These are NULL writes to functionIndex
          * (@0x8); the 2.2 fault handler discarded them, so guard each write -- bit-identical to
          * the validated build (the write had no observable effect on a NULL sprite anyway).
          * NULL sites: MsgBox_SetPortrait +0x4f/0x5b/0x67. See exchange/56. */
         if (obj1->d.objf413.faceSprite) obj1->d.objf413.faceSprite->functionIndex = OBJF_NULL;
         if (obj1->d.objf413.speakSprite) obj1->d.objf413.speakSprite->functionIndex = OBJF_NULL;
         if (obj1->d.objf413.blinkSprite) obj1->d.objf413.blinkSprite->functionIndex = OBJF_NULL;
#else
         obj1->d.objf413.faceSprite->functionIndex = OBJF_NULL;
         obj1->d.objf413.speakSprite->functionIndex = OBJF_NULL;
         obj1->d.objf413.blinkSprite->functionIndex = OBJF_NULL;
#endif
         break;
      }
      obj1++;
   }
   if (i == OBJ_DATA_CT) {
      obj1 = Obj_GetUnused();
   }

   obj1->functionIndex = OBJF_MSGBOX_PORTRAIT;
   obj1->state = 0;
   obj1->state2 = 0;
   obj1->state3 = 0;
   obj1->d.objf413.flipped = lower;
   obj1->d.objf413.portrait.id = portraitId;

   if (!lower) {
      obj1->x1.n = 26;
      obj1->y1.n = 25;
   } else {
      obj1->x1.n = 246;
      obj1->y1.n = 170;
   }
}

void MsgBox_Close(u8 lower) {
   s32 i;
   Object *obj1;

   if (!lower) {
      CloseWindow(0x41);
      CloseWindow(0x42);
      obj1 = gObjectArray;
      for (i = 0; i < OBJ_DATA_CT; i++) {
         if (obj1->functionIndex == OBJF_UPPER_MSGBOX_TAIL) {
            obj1->functionIndex = OBJF_NULL;
         }
         obj1++;
      }
   } else {
      CloseWindow(0x43);
      CloseWindow(0x44);
      obj1 = gObjectArray;
      for (i = 0; i < OBJ_DATA_CT; i++) {
         if (obj1->functionIndex == OBJF_LOWER_MSGBOX_TAIL) {
            obj1->functionIndex = OBJF_NULL;
         }
         obj1++;
      }
   }
   obj1 = gObjectArray;
   for (i = 0; i < OBJ_DATA_CT; i++) {
      if (obj1->functionIndex == OBJF_MSGBOX_PORTRAIT && obj1->d.objf413.flipped == lower) {
         obj1->state = 99;
         break;
      }
      obj1++;
   }
}

#undef OBJF
#define OBJF 014
void Objf014_BattleUnit(Object *obj) {
   s32 i_s0; // + portraitId, samex
   s32 textPtrIdx;
   u8 origAnimIdx;
   UnitStatus *unit;
   Object *sprite;
   u8 elevation;
   u8 tmpz, tmpx;
   s32 nextz, nextx;
   Object *obj1;
   Object *obj2;
   Quad *qswap;
   u8 **animSet;

   i_s0 = 0;
   unit = OBJ.unit;
   sprite = OBJ.sprite;
   origAnimIdx = OBJ.animIdx;
   textPtrIdx = 0;

   switch (obj->state) {
   case 0:

      switch (obj->state2) {
      case 0:
         unit->team = OBJ.team;
         CalculateUnitStats(unit);
         sprite = Obj_GetUnused();
         sprite->functionIndex = OBJF_NOOP;
         sprite->d.sprite.coords[0].z = rand() % 64 - 32;
         sprite->d.sprite.coords[0].x = rand();
         sprite->d.sprite.stripIdx = unit->stripIdx;
         unit->sprite = sprite;
         sprite->d.sprite.direction = unit->direction;
         OBJ.animSet = gSpriteStripAnimSets[sprite->d.sprite.stripIdx];
         sprite->x1.n = obj->x1.n;
         sprite->z1.n = obj->z1.n;
         sprite->x3.n = obj->x1.n;
         sprite->z3.n = obj->z1.n;
         OBJ.sprite = sprite;
         OBJ_MAP_UNIT(sprite).s.unitIdx = OBJ.unitIdx;
         OBJ_MAP_UNIT(sprite).s.team = OBJ.team;
         OBJ.timer = rand() % 12 + 1;
         if (OBJ_TILE_STATE(sprite).action == TA_25) {
            OBJ_TILE_STATE(sprite).action = TA_NONE;
            SetElevationFromTerrain(sprite);
            sprite->d.sprite.hidden = 1;
         }
         obj->state2++;
         break;

      case 1:
         if (--OBJ.timer == 0) {
            sprite->d.sprite.animInitialized = 0;
            obj->state++;
            obj->state2 = 0;
         }
         break;
      } // switch (obj->state2) (via state:0)

      break;

   case 1:
      OBJ.animIdx = ANIM_IDLE_B;
      if (OBJ_TILE_STATE(sprite).action == TA_NONE) {
         break;
      }
      if (OBJ_TILE_STATE(sprite).action == TA_6) {
         OBJ.pathIdx = 4;
         while (gPathBackToUnit[OBJ.pathIdx] != PATH_STEP_INVALID) {
            OBJ.pathIdx += 2;
         }
         sprite->z3.s.lo = CV(0.5);
         sprite->x3.s.lo = CV(0.5);
         obj->state2 = 0;
         obj->state++;
      }
      if (OBJ_TILE_STATE(sprite).action == TA_MELEE_ATK) {
         obj->state = 8;
         obj->state2 = 0;
      } else if (OBJ_TILE_STATE(sprite).action == TA_PUSHING_CRATE) {
         obj->state = 15;
         obj->state2 = 0;
      } else if (OBJ_TILE_STATE(sprite).action == TA_PUSHING_BOULDER) {
         obj->state = 21;
         obj->state2 = 0;
      } else if (OBJ_TILE_STATE(sprite).action == TA_RANGED_ATK) {
         obj->state = 12;
         obj->state2 = 0;
      } else if (OBJ_TILE_STATE(sprite).action == TA_BLOCK) {
         obj->state = 9;
         obj->state2 = 0;
      } else if (OBJ_TILE_STATE(sprite).action == TA_PHYS_HIT) {
         obj->state = 10;
         obj->state2 = 0;
      } else if (OBJ_TILE_STATE(sprite).action == TA_MAG_HIT) {
         obj->state = 5;
         obj->state2 = 0;
      } else if (OBJ_TILE_STATE(sprite).action == TA_BOULDER_HIT) {
         obj->state = 22;
         obj->state2 = 0;
      } else if (OBJ_TILE_STATE(sprite).action == TA_PHYS_DEFEAT) {
         obj->state = 7;
         obj->state2 = 0;
      } else if (OBJ_TILE_STATE(sprite).action == TA_PHYS_DEFEAT_MSG) {
         obj->state = 18;
         obj->state2 = 0;
      } else if (OBJ_TILE_STATE(sprite).action == TA_MAG_DEFEAT_MSG) {
         obj->state = 19;
         obj->state2 = 0;
      } else if (OBJ_TILE_STATE(sprite).action == TA_MAG_DEFEAT) {
         obj->state = 13;
         obj->state2 = 0;
      } else if (OBJ_TILE_STATE(sprite).action == TA_11) {
         obj->state = 3;
         obj->state2 = 0;
      } else if (OBJ_TILE_STATE(sprite).action == TA_CHOOSING_DIRECTION) {
         gSignal4 = sprite->d.sprite.direction;
         OBJ.direction = sprite->d.sprite.direction;
         obj->state = 4;
         obj->state2 = 0;
      } else if (OBJ_TILE_STATE(sprite).action == TA_CAST) {
         obj->state = 6;
         obj->state2 = 0;
      } else if (OBJ_TILE_STATE(sprite).action == TA_LEVEL_UP) {
         obj->state = 16;
         obj->state2 = 0;
      } else if (OBJ_TILE_STATE(sprite).action == TA_22) {
         obj->state = 17;
         obj->state2 = 0;
      } else if (OBJ_TILE_STATE(sprite).action == TA_29) {
         obj->state = 23;
         obj->state2 = 0;
      } else if (OBJ_TILE_STATE(sprite).action == TA_30) {
         obj->state = 24;
         obj->state2 = 0;
      } else if (OBJ_TILE_STATE(sprite).action == TA_31) {
         obj->state = 25;
         obj->state2 = 0;
      } else if (OBJ_TILE_STATE(sprite).action == TA_32) {
         OBJ_TILE_STATE(sprite).action = TA_NONE;
         gState.msgFinished = 0;
         if (unit->name == UNIT_KURTZ) {
            SetupBattleMsgBox(UNIT_KURTZ, PORTRAIT_KURTZ_511, 0x22);
         } else if (unit->name == UNIT_DALLAS) {
            SetupBattleMsgBox(UNIT_DALLAS, PORTRAIT_DALLAS_516, 0x22);
         } else if (unit->name == UNIT_SABINA) {
            SetupBattleMsgBox(UNIT_SABINA, PORTRAIT_SABINA_509, 0x22);
         } else if (unit->name == UNIT_KANE) {
            SetupBattleMsgBox(UNIT_KANE, PORTRAIT_KANE_ANGRY, 0x23);
            gState.mapState.s.field_0x12 = 1;
         } else if (unit->name > UNIT_END_OF_PARTY) {
            gState.msgFinished = 1;
         } else {
            // Party member
            if (gState.mapNum == 13 || gState.mapNum == 35) {
               // Injured portrait when party member caught in bridge destruction.
               SetupBattleMsgBox(unit->name, (unit->name - 1) * 7 + 2, unit->name + 21);
            } else {
               // Neutral portrait when being washed away by river etc.
               SetupBattleMsgBox(unit->name, (unit->name - 1) * 7 + 1, unit->name + 21);
            }
         }
      } else if (OBJ_TILE_STATE(sprite).action == TA_9) {
         OBJ_TILE_STATE(sprite).action = TA_NONE;
         OBJ_TARGET_MAP_UNIT(sprite).raw = 0;
         sprite->z1.s.hi = gPathBackToUnit[0];
         sprite->x1.s.hi = gPathBackToUnit[1];
         sprite->z3.n = sprite->z1.n;
         sprite->x3.n = sprite->x1.n;
         OBJ_MAP_UNIT(sprite).s.unitIdx = OBJ.unitIdx;
         OBJ_MAP_UNIT(sprite).s.team = OBJ.team;
         obj->state = 3;
         obj->state2 = 0;
      }

      break;

   case 2:

      switch (obj->state2) {
      case 0:
         sprite->x2.n = 0x20;
         sprite->z2.n = 0x20;
         if (unit->class == CLASS_AIRMAN) {
            elevation = OBJ_TERRAIN(sprite).s.elevation;
            i_s0 = OBJ.pathIdx;
            while (gPathBackToUnit[i_s0 - 1] != PATH_STEP_INVALID) {
               tmpz = gPathBackToUnit[i_s0 - 2];
               tmpx = gPathBackToUnit[i_s0 - 1];
               if (gTerrainPtr[tmpz][tmpx].s.elevation > elevation) {
                  elevation = gTerrainPtr[tmpz][tmpx].s.elevation;
               }
               i_s0 -= 2;
            }
            sprite->y3.n = elevation * CV(0.5);
         }
         OBJ_MAP_UNIT(sprite).raw = 0;

      LAB_8004cfb8: //<- from state2:1 after decrement
         OBJ_TILE_STATE(sprite).action = TA_NONE;
         tmpx = gPathBackToUnit[--OBJ.pathIdx];
         tmpz = gPathBackToUnit[--OBJ.pathIdx];
         if (tmpz != PATH_STEP_INVALID) {
            if (unit->class == CLASS_AIRMAN) {
               if (tmpx == sprite->x1.s.hi) {
                  i_s0 = 1;
               } else {
                  i_s0 = 0;
               }
               while (gPathBackToUnit[OBJ.pathIdx - 1] != PATH_STEP_INVALID) {
                  nextx = gPathBackToUnit[OBJ.pathIdx - 1];
                  nextz = gPathBackToUnit[OBJ.pathIdx - 2];
                  if (i_s0) {
                     if (tmpx != nextx) {
                        break;
                     }
                  } else {
                     if (tmpz != nextz) {
                        break;
                     }
                  }
                  OBJ.pathIdx -= 2;
                  tmpx = nextx;
                  tmpz = nextz;
               }
            }
            sprite->z3.s.hi = tmpz;
            sprite->x3.s.hi = tmpx;
            sprite->d.sprite.finishedMoving = 0;
            obj->state2++;
         } else {
            obj->state = 1;
            gSignal3 = 1;
            OBJ_TARGET_MAP_UNIT(sprite).s.unitIdx = OBJ.unitIdx;
            OBJ_TARGET_MAP_UNIT(sprite).s.team = OBJ.team;
         }

         break;

      case 1:
         if (sprite->d.sprite.finishedMoving) {
            obj->state2--;
            goto LAB_8004cfb8;
         }

      } // switch (obj->state2) (via state:2)

      break;

   case 3:
      sprite->d.sprite.direction = unit->direction;
      OBJ_TILE_STATE(sprite).action = TA_NONE;
      obj->state = 1;
      obj->state2 = 0;
      break;

   case 4:
      OBJ_TILE_STATE(sprite).action = TA_NONE;
      OBJ.animIdx = ANIM_IDLE_B;
      sprite->d.sprite.direction = gSignal4;
      if (gSignal2 == 1) {
         unit->direction = gSignal4;
         obj->state = 1;
         obj->state2 = 0;
      }
      if (gSignal2 == 2) {
         sprite->d.sprite.direction = OBJ.direction;
         obj->state = 1;
         obj->state2 = 0;
      }
      break;

   case 5:

      switch (obj->state2) {
      case 0:
         OBJ_TILE_STATE(sprite).action = TA_NONE;
         OBJ.animIdx = ANIM_STRUCK_END_B;
         gSignal3 = 0;
         obj->state2++;
         break;
      case 1:
         if (gSignal3 == 1) {
            obj->state = 1;
            obj->state2 = 0;
         }
         break;
      }

      break;

   case 6:

      switch (obj->state2) {
      case 0:
         obj1 = Obj_GetUnused();
         obj1->functionIndex = OBJF_CASTING_FX;
         obj1->x1.n = sprite->x1.n;
         obj1->z1.n = sprite->z1.n;
         OBJ_TILE_STATE(sprite).action = TA_NONE;
         OBJ.animIdx = ANIM_CASTING_B;
         obj->state2++;
         break;

      case 1:
         if (sprite->d.sprite.animFinished) {
            if (gSpellSounds[gCurrentSpell] != 0) {
               PerformAudioCommand(gSpellSounds[gCurrentSpell]);
            }
            gSignal3 = 1;
            obj->state2++;
         }
         break;

      case 2:
         gSignal3 = 2;
         obj->state = 1;
         obj->state2 = 0;
         break;
      }

      break;

   case 7:

      switch (obj->state2) {
      case 0:
         TallySlainUnit(unit);
         CommitPartyMemberStatus(unit);
         // Prevent premature looting of kurtz, sabina, etc. during village escape
         if (gState.mapNum != 29) {
            gState.droppedItem = unit->droppedItem;
         }
         OBJ.animIdx = ANIM_STRUCK_END_B;
         OBJ.timer = 35;
         obj1 = Obj_GetUnused();
         if (UnitIsRocky(unit)) {
            obj1->functionIndex = OBJF_ROCK_SPURT;
         } else {
            obj1->functionIndex = OBJF_BLOOD_SPURT;
         }
         obj1->x1.n = sprite->x1.n;
         obj1->z1.n = sprite->z1.n;
         obj->state2++;

      // fallthrough
      case 1:
         if (--OBJ.timer == 0) {
            obj2 = Obj_GetUnused();
            obj2->functionIndex = OBJF_SLAY_UNIT;
            obj2->x1.s.hi = sprite->x1.s.hi;
            obj2->z1.s.hi = sprite->z1.s.hi;
            gSignal3 = 0;
            obj->state2++;
         }
         break;

      case 2:
         if (gSignal3 != 0) {
            OBJ_TARGET_TILE_STATE(sprite).action = TA_NONE;
            OBJ_MAP_UNIT(sprite).raw = 0;
            sprite->functionIndex = OBJF_NULL;
            obj->functionIndex = OBJF_NULL;
            unit->idx = 0;
         }
         break;
      } // switch (obj->state2) (via state:7)

      break;

   case 8:

      switch (obj->state2) {
      case 0:
         if (gUnitSounds_Attacking1[unit->unitId][0] != 0) {
            obj1 = Obj_GetUnused();
            obj1->functionIndex = OBJF_AUDIO_CMD;
            obj1->d.objf581.cmd = gUnitSounds_Attacking1[unit->unitId][0];
            obj1->d.objf581.delay = gUnitSoundDelays_Attacking1[unit->unitId][0];
            if (gUnitSoundDelays_Attacking1[unit->unitId][1] != 0) {
               obj1 = Obj_GetUnused();
               obj1->functionIndex = OBJF_AUDIO_CMD;
               obj1->d.objf581.cmd = gUnitSounds_Attacking1[unit->unitId][1];
               obj1->d.objf581.delay = gUnitSoundDelays_Attacking1[unit->unitId][1] +
                                       gUnitSoundDelays_Attacking1[unit->unitId][0];
            }
         }
         if (gUnitSounds_Attacking2[unit->unitId][0] != 0) {
            obj1 = Obj_GetUnused();
            obj1->functionIndex = OBJF_AUDIO_CMD;
            obj1->d.objf581.cmd = gUnitSounds_Attacking2[unit->unitId][0];
            obj1->d.objf581.delay = gUnitSoundDelays_Attacking2[unit->unitId][0];
            if (gUnitSoundDelays_Attacking2[unit->unitId][1] != 0) {
               obj1 = Obj_GetUnused();
               obj1->functionIndex = OBJF_AUDIO_CMD;
               obj1->d.objf581.cmd = gUnitSounds_Attacking2[unit->unitId][1];
               obj1->d.objf581.delay = gUnitSoundDelays_Attacking2[unit->unitId][1] +
                                       gUnitSoundDelays_Attacking2[unit->unitId][0];
            }
         }
         OBJ_TILE_STATE(sprite).action = TA_NONE;
         OBJ.animIdx = ANIM_ATTACKING_B;
         if (unit->unitId == UNIT_ID_BAHAMUT || unit->unitId == UNIT_ID_SALAMANDER) {
            obj2 = Obj_GetUnused();
            obj2->functionIndex = OBJF_FLAME_BREATH;
            obj2->x1.n = sprite->x1.n;
            obj2->z1.n = sprite->z1.n;
         }
         if (unit->unitId == UNIT_ID_ASH_VANDALIER || unit->unitId == UNIT_ID_DOLF_DARK_ANGEL) {
            obj2 = Obj_GetUnused();
            obj2->functionIndex = OBJF_ELITE_MELEE_SPARKLES;
            obj2->x1.n = sprite->x1.n;
            obj2->z1.n = sprite->z1.n;
         }
         obj->state2++;
         break;

      case 1:
         if (sprite->d.sprite.animFinished) {
            OBJ.animIdx = ANIM_ATTACKING_END_B;
            gSignal3 = 1;
            obj->state2++;
         }
         break;

      case 2:
         if (sprite->d.sprite.animFinished) {
            gSignal4 = 1;
            obj->state = 1;
            obj->state2 = 0;
         }
         break;
      } // switch (obj->state2) (via state:8)

      break;

   case 9:

      switch (obj->state2) {
      case 0:
         PerformAudioCommand(gUnitSounds_Blocking[unit->unitId]);
         OBJ.animIdx = ANIM_BLOCKING_B;
         OBJ_TILE_STATE(sprite).action = TA_NONE;
         obj1 = Obj_GetUnused();
         obj1->functionIndex = OBJF_UNIT_BLOCKING;
         obj1->x1.n = sprite->x1.n;
         obj1->z1.n = sprite->z1.n;
         obj->state2++;
         break;

      case 1:
         if (sprite->d.sprite.animFinished) {
            gSignal3 = 1;
            obj->state = 1;
            obj->state2 = 0;
         }
         break;
      }

      break;

   case 10:

      switch (obj->state2) {
      case 0:
         PerformAudioCommand(gUnitSounds_Struck[unit->unitId]);
         OBJ_TILE_STATE(sprite).action = TA_NONE;
         OBJ.animIdx = ANIM_STRUCK_B;
         obj1 = Obj_GetUnused();
         obj1->functionIndex = OBJF_UNIT_STRUCK;
         obj1->x1.n = sprite->x1.n;
         obj1->z1.n = sprite->z1.n;
         obj->state2++;
         break;

      case 1:
         if (sprite->d.sprite.animFinished) {
            gSignal3 = 1;
            obj->state = 1;
            obj->state2 = 0;
         }
         break;
      }

      break;

   case 11:
      if (OBJ_TILE_STATE(sprite).action != TA_4) {
         obj->state = 3;
         obj->state2 = 0;
      } else {
         sprite->d.sprite.direction = OBJ_TILE_STATE(sprite).cachedShort;
      }
      break;

   case 12:

      switch (obj->state2) {
      case 0:
         if (gUnitSounds_Attacking1[unit->unitId][0] != 0) {
            obj1 = Obj_GetUnused();
            obj1->functionIndex = OBJF_AUDIO_CMD;
            obj1->d.objf581.cmd = gUnitSounds_Attacking1[unit->unitId][0];
            obj1->d.objf581.delay = gUnitSoundDelays_Attacking1[unit->unitId][0];
            if (gUnitSoundDelays_Attacking1[unit->unitId][1] != 0) {
               obj1 = Obj_GetUnused();
               obj1->functionIndex = OBJF_AUDIO_CMD;
               obj1->d.objf581.cmd = gUnitSounds_Attacking1[unit->unitId][1];
               obj1->d.objf581.delay = gUnitSoundDelays_Attacking1[unit->unitId][1] +
                                       gUnitSoundDelays_Attacking1[unit->unitId][0];
            }
         }
         OBJ_TILE_STATE(sprite).action = TA_NONE;
         if (CalculateProjectileHeight(sprite->x1.s.hi, sprite->z1.s.hi, gTargetX, gTargetZ) >
             0x20000) {
            OBJ.animIdx = ANIM_ATTACKING_FAR_B;
         } else {
            OBJ.animIdx = ANIM_ATTACKING_B;
         }
         if (gState.mapNum == 13 && unit->name == UNIT_KIRA && gState.mapState.s.field_0x13 == 0) {
            obj->state2++;
            gState.mapState.s.field_0x13 = 1;
         } else {
            obj->state2 = 4;
         }
         break;

      case 1:
         MsgBox_ShowForSprite(sprite, 0, 0);
         MsgBox_SetPortrait(PORTRAIT_KIRA_ANGRY, 0);
         MsgBox_SetText(1, 0x14, 0x100);
         obj->state2++;
         break;

      case 2:
         if (gState.msgBoxFinished) {
            MsgBox_ShowForSprite(sprite, 1, 1);
            MsgBox_SetPortrait(PORTRAIT_ASH, 1);
            MsgBox_SetText(2, 0x15, 0x100);
            obj->state2++;
         }
         break;

      case 3:
         if (gState.msgBoxFinished) {
            MsgBox_Close(0);
            MsgBox_Close(1);
            obj->state2++;
         }
         break;

      case 4:
         if (sprite->d.sprite.animFinished) {
            OBJ.animIdx += 2;
            if (gUnitSounds_Attacking2[unit->unitId][0] != 0) {
               obj1 = Obj_GetUnused();
               obj1->functionIndex = OBJF_AUDIO_CMD;
               obj1->d.objf581.cmd = gUnitSounds_Attacking2[unit->unitId][0];
               obj1->d.objf581.delay = 0;
            }
            obj1 = Obj_GetUnused();
            obj1->functionIndex = OBJF_PROJECTILE;
            if (gMapUnitsPtr[gTargetZ][gTargetX].s.team == TEAM_CHEST) {
               obj1->functionIndex = OBJF_PROJECTILE_INDIRECT;
            }
            obj1->x1.n = sprite->x1.n;
            obj1->z1.n = sprite->z1.n;
            obj1->y1.n = sprite->y1.n + CV(0.5);

            obj1 = Obj_GetUnused();
            obj1->functionIndex = OBJF_RANGED_TARGET_CAMERA;
            obj1->d.objf023.targetSprite = GetUnitSpriteAtPosition(gTargetZ, gTargetX);
            obj->state2++;
         }
         break;

      case 5:
         if (sprite->d.sprite.animFinished) {
            obj->state = 1;
            obj->state2 = 0;
         }
         break;
      } // switch (obj->state2) (via state:12)

      break;

   case 13:

      switch (obj->state2) {
      case 0:
         TallySlainUnit(unit);
         CommitPartyMemberStatus(unit);
         if (gState.mapNum != 29) {
            gState.droppedItem = unit->droppedItem;
         }
         gSignal3 = 0;
         obj->state2++;

      // fallthrough
      case 1:
         if (gSignal3 == 1) {
            OBJ.timer = 5;
            obj->state2++;
         } else {
            OBJ.animIdx = ANIM_STRUCK_END_B;
         }
         break;

      case 2:
         if (--OBJ.timer == 0) {
            OBJ_TILE_STATE(sprite).action = TA_NONE;
            OBJ_MAP_UNIT(sprite).raw = 0;
            sprite->functionIndex = OBJF_NULL;
            obj->functionIndex = OBJF_NULL;
            unit->idx = 0;
         }
         break;
      }

      break;

   case 15:

      switch (obj->state2) {
      case 0:
         OBJ_TILE_STATE(sprite).action = TA_NONE;
         OBJ.animIdx = ANIM_HIGH_STEP_B;
         if (unit->class == CLASS_AIRMAN) {
            OBJ.timer = 11;
         } else {
            OBJ.timer = 4;
         }
         obj->state2++;
         break;

      case 1:
         if (--OBJ.timer == 0) {
            PerformAudioCommand(AUDIO_CMD_PLAY_SFX(233));
            gSignal3 = 1;
            obj->state2++;
         }
         break;

      case 2:
         if (sprite->d.sprite.animFinished) {
            gSignal4 = 1;
            obj->state = 1;
            obj->state2 = 0;
         }
         break;
      }

      break;

   case 16:

      switch (obj->state2) {
      case 0:
         OBJ_TILE_STATE(sprite).action = TA_NONE;
         obj1 = Obj_GetUnused();
         obj1->functionIndex = OBJF_LEVEL_UP_FX;
         obj1->x1.n = sprite->x1.n;
         obj1->z1.n = sprite->z1.n;
         OBJ.animIdx = ANIM_HOP_B;
         if (unit->class == CLASS_AIRMAN) {
            OBJ.animIdx = ANIM_IDLE_B;
            obj->state2 = 5;
         } else {
            obj->state2++;
         }
         break;

      case 1:
      case 2:
      case 3:
         if (sprite->d.sprite.animFinished) {
            sprite->d.sprite.animInitialized = 0;
            obj->state2++;
         }
         break;

      case 4:
         if (sprite->d.sprite.animFinished) {
            OBJ.animIdx = ANIM_IDLE_B;
            obj->state2++;
         }

      // fallthrough
      case 5:
         if (gSignal3 != 0) {
            obj->state = 1;
            obj->state2 = 0;
         }
         break;
      } // switch (obj->state2) (via state:16)

      break;

   case 17:
      TallySlainUnit(unit);
      CommitPartyMemberStatus(unit);
      if (gState.mapNum != 29) {
         gState.droppedItem = unit->droppedItem;
      }
      OBJ_TILE_STATE(sprite).action = TA_NONE;
      OBJ_MAP_UNIT(sprite).raw = 0;
      sprite->functionIndex = OBJF_NULL;
      obj->functionIndex = OBJF_NULL;
      unit->idx = 0;
      return;

   case 18:

      switch (obj->state2) {
      case 0:
         TallySlainUnit(unit);
         CommitPartyMemberStatus(unit);
         if (gState.mapNum != 29) {
            gState.droppedItem = unit->droppedItem;
         }
         OBJ_TILE_STATE(sprite).action = TA_NONE;
         OBJ.animIdx = ANIM_STRUCK_B;
         obj1 = Obj_GetUnused();
         obj1->functionIndex = OBJF_UNIT_STRUCK;
         obj1->x1.n = sprite->x1.n;
         obj1->z1.n = sprite->z1.n;
         obj1 = Obj_GetUnused();
         if (UnitIsRocky(unit)) {
            obj1->functionIndex = OBJF_ROCK_SPURT;
         } else {
            obj1->functionIndex = OBJF_BLOOD_SPURT;
         }
         obj1->x1.n = sprite->x1.n;
         obj1->z1.n = sprite->z1.n;
         obj->state2++;
         break;

      case 1:
         if (sprite->d.sprite.animFinished) {
            obj->state = 20;
            obj->state2 = 0;
         }
         break;
      }

      break;

   case 19:

      switch (obj->state2) {
      case 0:
         TallySlainUnit(unit);
         CommitPartyMemberStatus(unit);
         if (gState.mapNum != 29) {
            gState.droppedItem = unit->droppedItem;
         }
         OBJ_TILE_STATE(sprite).action = TA_NONE;
         OBJ.animIdx = ANIM_STRUCK_END_B;
         gSignal3 = 0;
         obj->state2++;
         break;

      case 1:
         if (gSignal3 == 1) {
            obj->state = 20;
            obj->state2 = 0;
         }
         break;
      }

      break;

   case 20:

      switch (obj->state2) {
      case 0:
         OBJ.animIdx = ANIM_STRUCK_END_B;
         OBJ.timer = 10;
         obj->state2++;
         break;

      case 1:
         obj->state2 = 99;
         if (unit->unitType == UNIT_TYPE_MAGE_TOWER) {
            MsgBox_ShowForSprite(sprite, 1, 1);
            if (gState.mapState.s.field_0x13 < 2) {
               MsgBox_SetPortrait(PORTRAIT_ASH_UPSET, 1);
            } else {
               MsgBox_SetPortrait(PORTRAIT_ASH_ANGRY, 1);
            }
            MsgBox_SetText(2, gState.mapState.s.field_0x13 + 15, 0x100);
            gState.mapState.s.field_0x13++;
            obj->state2 = 98;
         } else {
            MsgBox_ShowForSprite(sprite, 0, 0);
            if (unit->name <= UNIT_END_OF_PARTY) {
               i_s0 = (unit->name - 1) * 7 + 2;
               textPtrIdx = unit->name;
               obj->state2 = 98;
            }
            if (unit->name == UNIT_ZOOT) {
               i_s0 = PORTRAIT_ZOOT_559;
               obj->state2 = 98;
               if (gState.mapNum == 10) {
                  i_s0 = PORTRAIT_ZOOT_449;
                  obj->state2 = 96;
               }
               textPtrIdx = 13;
            }
            if (unit->name == UNIT_HASSAN) {
               i_s0 = PORTRAIT_HASSAN_INJURED;
               textPtrIdx = 13;
               obj->state2 = 96;
            }
            if (unit->unitType == UNIT_TYPE_VILLAGER) {
               i_s0 = gUnitPortraitIds[unit->unitId] + 1;
               obj->state2 = 98;
               gState.mapState.s.field_0x13++;
               textPtrIdx = 13;
               if (gState.mapState.s.field_0x13 == 1) {
                  obj->state2 = 2;
               }
               if (gState.mapState.s.field_0x13 == 9) {
                  obj->state2 = 3;
               }
            }
            if (unit->name == UNIT_MAGNUS) {
               i_s0 = PORTRAIT_MAGNUS_DARK_LORD_492;
               textPtrIdx = 13;
               obj->state2 = 96;
            }
            if (unit->name == UNIT_CLIVE) {
               i_s0 = PORTRAIT_CLIVE_INJURED;
               textPtrIdx = 14;
               obj->state2 = 98;
            }
            if (unit->name == UNIT_LANDO) {
               i_s0 = PORTRAIT_LANDO_INJURED;
               textPtrIdx = 20;
               obj->state2 = 98;
            }
            if (unit->name == UNIT_DUMAS) {
               i_s0 = PORTRAIT_DUMAS_562;
               textPtrIdx = 21;
               obj->state2 = 98;
            }
            if (unit->name == UNIT_DALLAS) {
               i_s0 = PORTRAIT_DALLAS_518;
               textPtrIdx = 20;
               obj->state2 = 98;
            }
            if (unit->name == UNIT_KURTZ) {
               i_s0 = PORTRAIT_KURTZ_511;
               textPtrIdx = 13;
               obj->state2 = 98;
            }
            if (unit->unitType == UNIT_TYPE_SALAMANDER) {
               i_s0 = PORTRAIT_SALAMANDER;
               textPtrIdx = 13;
               obj->state2 = 98;
            }
            if (unit->name == UNIT_KANE && gState.mapNum == 40) {
               if (gState.mapNum == 40) {
                  i_s0 = PORTRAIT_KANE_BLK_KNIGHT_INJURED;
                  textPtrIdx = 27;
                  obj->state2 = 98;
               }
            }
            if (unit->name == UNIT_XENO) {
               i_s0 = PORTRAIT_XENO_INJURED;
               textPtrIdx = 23;
               obj->state2 = 98;
            }
            if (unit->unitType == UNIT_TYPE_DOOM_LORD) {
               PerformAudioCommand(AUDIO_CMD_PREPARE_XA(134));
               i_s0 = PORTRAIT_DOLF_495;
               textPtrIdx = 13;
               obj->state2 = 7;
            }
            if (unit->unitType == UNIT_TYPE_DARK_ANGEL) {
               i_s0 = PORTRAIT_DOLF_581;
               textPtrIdx = 14;
               obj->state2 = 96;
            }
            if (gState.mapNum == 38) {
               if (unit->name == UNIT_KANE) {
                  i_s0 = PORTRAIT_KANE_INJURED;
                  textPtrIdx = 16;
                  gState.mapState.s.field_0x12 = 1;
                  obj->state2 = 4;
               }
               if (unit->name == UNIT_SABINA) {
                  i_s0 = PORTRAIT_SABINA_509;
                  obj->state2 = 98;
                  textPtrIdx = 18;
                  if (gState.mapState.s.field_0x13 != 0 && gState.mapState.s.field_0x12 == 0) {
                     obj->state2 = 5;
                  }
               }
            }
            if (gState.mapNum == 29) {
               if (unit->name == UNIT_KANE) {
                  i_s0 = PORTRAIT_KANE_INJURED;
                  textPtrIdx = 13;
                  obj->state2 = 98;
               }
               if (unit->name == UNIT_SABINA) {
                  i_s0 = PORTRAIT_SABINA_508;
                  textPtrIdx = 14;
                  obj->state2 = 98;
               }
               if (unit->name == UNIT_DALLAS) {
                  i_s0 = PORTRAIT_DALLAS_517;
                  textPtrIdx = 15;
                  obj->state2 = 98;
               }
               if (unit->name == UNIT_KURTZ) {
                  i_s0 = PORTRAIT_KURTZ_511;
                  textPtrIdx = 16;
                  obj->state2 = 98;
               }
            }
            if (unit->name == UNIT_LEENA) {
               i_s0 = PORTRAIT_LEENA_573;
               textPtrIdx = 13;
               obj->state2 = 6;
            }
            MsgBox_SetPortrait(i_s0, 0);
            MsgBox_SetText(1, textPtrIdx, 0x100);
            gState.msgBoxFinished = 0;
         }
         break;

      case 2:
         if (gState.msgBoxFinished) {
            MsgBox_ShowForSprite(sprite, 1, 1);
            CloseWindow(0x38);
            MsgBox_SetPortrait(PORTRAIT_ASH_UPSET, 1);
            textPtrIdx = 14;
            MsgBox_SetText(2, textPtrIdx, 0x100);
            obj->state2 = 98;
         }
         break;

      case 3:
         if (gState.msgBoxFinished) {
            MsgBox_ShowForSprite(sprite, 1, 1);
            MsgBox_SetPortrait(PORTRAIT_ASH_UPSET, 1);
            CloseWindow(0x38);
            textPtrIdx = 15;
            MsgBox_SetText(2, textPtrIdx, 0x100);
            obj->state2 = 98;
         }
         break;

      case 4:
         if (gState.msgBoxFinished) {
            CloseWindow(0x38);
            MsgBox_ShowForSprite(sprite, 1, 1);
            MsgBox_SetPortrait(PORTRAIT_SABINA, 1);
            textPtrIdx = 17;
            MsgBox_SetText(2, textPtrIdx, 0x100);
            obj->state2 = 98;
         }
         break;

      case 5:
         if (gState.msgBoxFinished) {
            CloseWindow(0x38);
            MsgBox_ShowForSprite(sprite, 1, 1);
            MsgBox_SetPortrait(PORTRAIT_KANE_ANGRY, 1);
            textPtrIdx = 19;
            MsgBox_SetText(2, textPtrIdx, 0x100);
            obj->state2 = 98;
         }
         break;

      case 6:
         if (gState.msgBoxFinished) {
            MsgBox_ShowForSprite(sprite, 1, 1);
            MsgBox_SetPortrait(PORTRAIT_ASH_UPSET, 1);
            textPtrIdx = 14;
            MsgBox_SetText(2, textPtrIdx, 0x100);
            obj->state2 = 98;
         }
         break;

      case 7:
         if (gState.msgBoxFinished) {
            PerformAudioCommand(AUDIO_CMD_PLAY_XA(134));
            MsgBox_Close(0);
            OBJ.timer = 90;
            obj1 = Obj_GetUnused();
            obj1->functionIndex = OBJF_MID_BATTLE_TRANSFORMATION;
            obj1->x1.s.hi = sprite->x1.s.hi;
            obj1->z1.s.hi = sprite->z1.s.hi;
            gState.transformFxDone = 0;
            obj->state2++;
         }
         break;

      case 8:
         if (--OBJ.timer == 0) {
            SetupBattleUnit(13, sprite->z1.s.hi, sprite->x1.s.hi, 32, TEAM_ENEMY, DIR_WEST, 100, 60,
                            99);
            OBJ_MAP_UNIT(sprite).raw = 0;
            sprite->d.sprite.hidden = 1;
            OBJ.timer = 90;
            obj->state2++;
         }
         break;

      case 9:
         if (--OBJ.timer == 0) {
            PerformAudioCommand(AUDIO_CMD_FADE_OUT_8_1);
            gState.transformFxDone = 1;
            /*gSignal3 = 1;
            gSignal4 = 1;
            sprite->functionIndex = OBJF_NULL;
            obj->functionIndex = OBJF_NULL;
            unit->idx = 0;*/
            //?
            goto LAB_8004ec50;
         }
         break;

      case 96:
         if (gState.msgBoxFinished) {
            MsgBox_Close(0);
            OBJ_TARGET_TILE_STATE(sprite).action = TA_NONE;
            unit->name = UNIT_NULL;
            gSignal3 = 1;
            gSignal4 = 1;
            obj->state++;
         }
         break;

      case 98:
         if (gState.msgBoxFinished) {
            PerformAudioCommand(AUDIO_CMD_PLAY_SFX(232));
            MsgBox_Close(0);
            MsgBox_Close(1);
            obj1 = Obj_GetUnused();
            obj1->functionIndex = OBJF_STRETCH_WARP_SPRITE;
            obj1->x1.n = sprite->x1.n;
            obj1->z1.n = sprite->z1.n;
            OBJ.timer = 20;
            obj->state2 = 99;
         }
         break;

      case 99:
         if (--OBJ.timer == 0) {
            OBJ_TARGET_TILE_STATE(sprite).action = TA_NONE;
            OBJ_MAP_UNIT(sprite).raw = 0;
         //?
         LAB_8004ec50:
            gSignal3 = 1;
            gSignal4 = 1;
            sprite->functionIndex = OBJF_NULL;
            obj->functionIndex = OBJF_NULL;
            unit->idx = 0;
         }
         break;
      } // switch (obj->state2) (via state:20)

      break;

   case 21:

      switch (obj->state2) {
      case 0:
         OBJ_TILE_STATE(sprite).action = TA_NONE;
         obj1 = Obj_GetUnused();
         obj1->functionIndex = OBJF_PUSHED_BOULDER;
         obj1->d.objf020.unitSprite = sprite;
         obj->state2++;
         break;

      case 1:
         if (gSignal5 == 1) {
            OBJ.animIdx = ANIM_HIGH_STEP_B;
            if (unit->class == CLASS_AIRMAN) {
               OBJ.timer = 11;
            } else {
               OBJ.timer = 4;
            }
            obj->state2++;
         }
         break;

      case 2:
         if (--OBJ.timer == 0) {
            PerformAudioCommand(AUDIO_CMD_PLAY_SFX(233));
            gSignal3 = 1;
            gSignal5 = 2;
            obj->state2++;
         }
         break;

      case 3:
         if (sprite->d.sprite.animFinished) {
            gSignal4 = 1;
            obj->state = 1;
            obj->state2 = 0;
         }
         break;
      } // switch (obj->state2) (via state:21)

      break;

   case 22:

      switch (obj->state2) {
      case 0:
         PerformAudioCommand(AUDIO_CMD_PLAY_SFX(228));
         OBJ_TILE_STATE(sprite).action = TA_NONE;
         OBJ.animIdx = ANIM_STRUCK_B;
         obj1 = Obj_GetUnused();
         obj1->functionIndex = OBJF_UNIT_STRUCK;
         obj1->x1.n = sprite->x1.n;
         obj1->z1.n = sprite->z1.n;
         unit->hpFrac /= 2;
         CalculateUnitStats(unit);
         obj->state2++;
         break;

      case 1:
         if (sprite->d.sprite.animFinished) {
            obj->state = 1;
            obj->state2 = 0;
         }
         break;
      }

      break;

   case 23:
   case 24:

      switch (obj->state2) {
      case 0:
         OBJ_TILE_STATE(sprite).action = TA_NONE;
         OBJ.animIdx = OBJ_TILE_STATE(sprite).cachedByte;
         obj->state2++;
         break;

      case 1:
         if (sprite->d.sprite.animFinished) {
            gSignal3 = 1;
            obj->state2++;
         }
         break;

      case 2:
         if (OBJ_TILE_STATE(sprite).cachedByte == 0 || obj->state == 24) {
            obj->state = 1;
            obj->state2 = 0;
         }
         break;
      }

      break;

   case 25:

      switch (obj->state2) {
      case 0:
         OBJ_TILE_STATE(sprite).action = TA_NONE;
         OBJ.animIdx = ANIM_BLOCKING_END_B;
         obj->state2++;
      // fallthrough
      case 1:
         if (!gState.shielding) {
            obj->state = 1;
            obj->state2 = 0;
         }
         break;
      }

      break;

   } // switch (obj->state)

   if (unit->class == CLASS_AIRMAN) {
      sprite->y1.n -= (CV(0.46875) + (rcos(sprite->d.sprite.coords[0].x & 0xfff) * 50 >> 12));
      UpdateAirmanUnitSpriteMovement(sprite, obj);
   } else {
      UpdateUnitSpriteMovement(sprite, obj);
   }
   UpdateUnitSpriteOrientation(sprite);
   if (origAnimIdx != OBJ.animIdx) {
      sprite->d.sprite.animInitialized = 0;
   }
   animSet = OBJ.animSet;
   sprite->d.sprite.animData = animSet[OBJ.animIdx + sprite->d.sprite.facingFront];
   UpdateUnitSpriteAnimation(sprite);

   if (unit->class != CLASS_AIRMAN) {
      if (OBJ.animIdx != ANIM_IDLE_B) {
         if (gOverheadMapState != 0) {
            RenderOverheadMapUnitMarker(gGraphicsPtr->ot, sprite, 0, unit->team);
         } else {
            RenderUnitSprite(gGraphicsPtr->ot, sprite, 0);
            RenderUnitHelpers(gGraphicsPtr->ot, sprite, unit->team, unit->done, unit->poisoned,
                              unit->paralyzed, unit->hideMarker);
         }
      } else {
         if (gOverheadMapState != 0) {
            RenderOverheadMapUnitMarker(gGraphicsPtr->ot, sprite, 1, unit->team);
         } else {
            RenderUnitSprite(gGraphicsPtr->ot, sprite, 1);
            RenderUnitHelpers(gGraphicsPtr->ot, sprite, unit->team, unit->done, unit->poisoned,
                              unit->paralyzed, unit->hideMarker);
         }
      }
   } else {
      qswap = gSpriteBoxQuads[0];
      gSpriteBoxQuads[0] = gSpriteBoxQuads[8];
      gSpriteBoxQuads[8] = qswap;

      qswap = gSpriteBoxQuads[1];
      gSpriteBoxQuads[1] = gSpriteBoxQuads[9];
      gSpriteBoxQuads[9] = qswap;

      qswap = gSpriteBoxQuads[2];
      gSpriteBoxQuads[2] = gSpriteBoxQuads[11];
      gSpriteBoxQuads[11] = qswap;

      gQuad_800fe65c[0].vx =
          -((rcos((sprite->d.sprite.coords[0].x + DEG(180)) & 0xfff) * 5 >> 12) + 15);
      gQuad_800fe65c[1].vx = -gQuad_800fe65c[0].vx;
      gQuad_800fe65c[2].vx = gQuad_800fe65c[0].vx;
      gQuad_800fe65c[3].vx = -gQuad_800fe65c[0].vx;

      gQuad_800fe65c[0].vy = -((rcos((sprite->d.sprite.coords[0].x + DEG(180)) & 0xfff) >> 12) + 4);
      gQuad_800fe65c[1].vy = gQuad_800fe65c[0].vy;
      gQuad_800fe65c[2].vy = -gQuad_800fe65c[0].vy;
      gQuad_800fe65c[3].vy = -gQuad_800fe65c[0].vy;

      // Shadow (for flying unit)
      obj1 = gTempGfxObj;
      obj1->d.sprite.hidden = 0;
      obj1->d.sprite.semiTrans = 0;
      obj1->x1.n = sprite->x1.n;
      obj1->z1.n = sprite->z1.n;
      obj1->d.sprite.gfxIdx = sprite->d.sprite.gfxIdx;
      obj1->d.sprite.stripIdx = sprite->d.sprite.stripIdx;
      obj1->d.sprite.facingLeft = sprite->d.sprite.facingLeft;
      obj1->d.sprite.clut = CLUT_BLACK;

      if (gOverheadMapState == 0) {
         RenderUnitSprite(gGraphicsPtr->ot, obj1, 1);
      }

      qswap = gSpriteBoxQuads[0];
      gSpriteBoxQuads[0] = gSpriteBoxQuads[8];
      gSpriteBoxQuads[8] = qswap;

      qswap = gSpriteBoxQuads[1];
      gSpriteBoxQuads[1] = gSpriteBoxQuads[9];
      gSpriteBoxQuads[9] = qswap;

      qswap = gSpriteBoxQuads[2];
      gSpriteBoxQuads[2] = gSpriteBoxQuads[11];
      gSpriteBoxQuads[11] = qswap;

      if (OBJ.animIdx == ANIM_IDLE_B) {
         SetElevationFromTerrain(sprite);
      }
      sprite->d.sprite.coords[0].x += 150;
      sprite->y1.n += (120 + (rcos(sprite->d.sprite.coords[0].x & 0xfff) * 50 >> 12));
      if (gOverheadMapState != 0) {
         RenderOverheadMapUnitMarker(gGraphicsPtr->ot, sprite, 0, unit->team);
      } else {
         RenderUnitSprite(gGraphicsPtr->ot, sprite, 0);
         RenderUnitHelpers(gGraphicsPtr->ot, sprite, unit->team, unit->done, unit->poisoned,
                           unit->paralyzed, unit->hideMarker);
      }
   }
}

void UpdateUnitSpriteOrientation(Object *sprite) {
   s32 dir;

   do {
      dir = ((gCameraRotation.vy + sprite->d.sprite.direction + sprite->d.sprite.coords[0].z) &
             0xfff) >>
            10;
   } while (0);

   sprite->d.sprite.facingLeft = !(dir == 0 || dir == 1);
   sprite->d.sprite.facingFront = (dir == 1 || dir == 2);
}

void UpdateUnitSpriteMovement(Object *sprite, Object *battler) {
   switch (sprite->state) {
   case 0:
      sprite->d.sprite.finishedMoving = 0;
      // Do nothing if already at destination
      if (sprite->x1.n == sprite->x3.n && sprite->z1.n == sprite->z3.n) {
         return;
      }
      // Face destination
      if (sprite->x1.n < sprite->x3.n) {
         sprite->d.sprite.direction = ANGLE_WEST;
      }
      if (sprite->x1.n > sprite->x3.n) {
         sprite->d.sprite.direction = ANGLE_EAST;
      }
      if (sprite->z1.n < sprite->z3.n) {
         sprite->d.sprite.direction = ANGLE_SOUTH;
      }
      if (sprite->z1.n > sprite->z3.n) {
         sprite->d.sprite.direction = ANGLE_NORTH;
      }
      sprite->state++;
      break;

   case 1:
      if (SmoothStepTo(sprite, sprite->z3.n, sprite->x3.n, 0) == 0) {
         // Terrain is not level, so initiate a jump
         battler->d.objf014.animIdx = ANIM_JUMPING_B;
         sprite->state += 2;
      } else {
         battler->d.objf014.animIdx = ANIM_WALKING_B;
         sprite->state++;
      }
      break;

   case 2:
      if (SmoothStepTo(sprite, sprite->z3.n, sprite->x3.n, 1) != 0) {
         sprite->d.sprite.finishedMoving = 1;
         sprite->state = 0;
      }
      break;

   case 3:
      if (!sprite->d.sprite.animFinished) {
         break;
      }
      sprite->state++;

   // fallthrough
   case 4:
      if (SmoothStepTo(sprite, sprite->z3.n, sprite->x3.n, 1) != 0) {
         // Finish landing jump with a crouch
         battler->d.objf014.animIdx = ANIM_CROUCHING_B;
         sprite->state++;
      }
      break;

   case 5:
      if (sprite->d.sprite.animFinished) {
         sprite->d.sprite.finishedMoving = 1;
         sprite->state = 0;
      }
      break;
   }
}

void UpdateAirmanUnitSpriteMovement(Object *sprite, Object *battler) {
   switch (sprite->state) {
   case 0:
      sprite->d.sprite.finishedMoving = 0;
      sprite->state++;

   // fallthrough
   case 1:
      if (sprite->x1.n == sprite->x3.n && sprite->z1.n == sprite->z3.n) {
         return;
      }
      battler->d.objf014.animIdx = ANIM_WALKING_B;
      sprite->y2.n = 32;
      if (sprite->x1.n < sprite->x3.n) {
         sprite->d.sprite.direction = ANGLE_WEST;
      }
      if (sprite->x1.n > sprite->x3.n) {
         sprite->d.sprite.direction = ANGLE_EAST;
      }
      if (sprite->z1.n < sprite->z3.n) {
         sprite->d.sprite.direction = ANGLE_SOUTH;
      }
      if (sprite->z1.n > sprite->z3.n) {
         sprite->d.sprite.direction = ANGLE_NORTH;
      }
      if (sprite->y1.n < sprite->y3.n) {
         sprite->state++;
      } else {
         if ((OBJ_TARGET_TERRAIN(sprite).s.elevation * CV(0.5) < sprite->y1.n) &&
             (gPathBackToUnit[battler->d.objf014.pathIdx - 1] == PATH_STEP_INVALID)) {
            sprite->state += (sprite->d.sprite.direction >> 9) + 3;
         } else {
            sprite->state += (sprite->d.sprite.direction >> 9) + 2;
         }
      }
      break;

   case 2:
      sprite->y1.n += sprite->y2.n;
      sprite->y2.n += 8;
      if (sprite->y1.n >= sprite->y3.n) {
         sprite->y1.n = sprite->y3.n;
         sprite->state += (sprite->d.sprite.direction >> 9) + 1;
         if (gPathBackToUnit[battler->d.objf014.pathIdx - 1] == PATH_STEP_INVALID) {
            sprite->state++;
         }
      }
      break;

   case 3:
   case 4:
      sprite->z2.n += 4;
      sprite->z1.n += sprite->z2.n;
      if (sprite->z1.n >= sprite->z3.n) {
         sprite->z1.n = sprite->z3.n;
         if (sprite->state == 3) {
            sprite->d.sprite.finishedMoving = 1;
            sprite->state = 1;
         } else {
            sprite->state = 11;
            goto HandleState11;
         }
      }
      break;

   case 5:
   case 6:
      sprite->x2.n += 4;
      sprite->x1.n += sprite->x2.n;
      if (sprite->x1.n >= sprite->x3.n) {
         sprite->x1.n = sprite->x3.n;
         if (sprite->state == 5) {
            sprite->d.sprite.finishedMoving = 1;
            sprite->state = 1;
         } else {
            sprite->state = 11;
            goto HandleState11;
         }
      }
      break;

   case 7:
   case 8:
      sprite->z2.n += 4;
      sprite->z1.n -= sprite->z2.n;
      if (sprite->z1.n <= sprite->z3.n) {
         sprite->z1.n = sprite->z3.n;
         if (sprite->state == 7) {
            sprite->d.sprite.finishedMoving = 1;
            sprite->state = 1;
         } else {
            sprite->state = 11;
            goto HandleState11;
         }
      }
      break;

   case 9:
   case 10:
      sprite->x2.n += 4;
      sprite->x1.n -= sprite->x2.n;
      if (sprite->x1.n <= sprite->x3.n) {
         sprite->x1.n = sprite->x3.n;
         if (sprite->state == 9) {
            sprite->d.sprite.finishedMoving = 1;
            sprite->state = 1;
         } else {
            sprite->state = 11;
            goto HandleState11;
         }
      }
      break;

   case 11:
   HandleState11:
      sprite->y3.n = OBJ_TARGET_TERRAIN(sprite).s.elevation * CV(0.5);
      sprite->state++;
      break;

   case 12:
      sprite->y2.n += 8;
      sprite->y1.n -= sprite->y2.n;
      if (sprite->y1.n <= sprite->y3.n) {
         sprite->y1.n = sprite->y3.n;
         sprite->d.sprite.finishedMoving = 1;
         sprite->state = 1;
      }
      break;
   }
}

void Objf412_EventCamera(Object *obj) {
   s32 diff;
   s32 oneEighth;
   Object *focus;

   if (!gState.inEvent) {
      return;
   }

   if (gState.debug && (gPadStateNewPresses & PAD_START)) {
      gState.primary = STATE_SET_SCENE_STATE;
      gState.secondary = 0;
      gState.state3 = 0;
      gState.state4 = 0;
   }

   if (obj->state == 0) {
      gState.eventCameraRot = gCameraRotation.vy;
      obj->state++;
   }

   diff = gState.eventCameraRot - gCameraRotation.vy;
   oneEighth = diff >> 3; // a0
   if (abs(oneEighth) > 2) {
      gCameraRotation.vy += oneEighth;
   } else {
      if (abs(diff) > 2) {
         gCameraRotation.vy += (oneEighth < 0) ? -2 : +2;
      } else {
         gCameraRotation.vy += diff;
      }
   }

   focus = gState.focus;
   if (focus != NULL) {
      gCameraPos.vx += (-(focus->x1.n >> 3) - gCameraPos.vx) >> 3;
      gCameraPos.vz += (-(focus->z1.n >> 3) - gCameraPos.vz) >> 3;
      gCameraPos.vy +=
          (((focus->y1.n + CV(1.0) + gState.eventCameraHeight) >> 3) - gCameraPos.vy) >> 3;
   } else {
      // x
      diff = -(gState.eventCameraPan.x >> 3) - gCameraPos.vx;
      oneEighth = diff >> 3;
      if (abs(oneEighth) > 2) {
         gCameraPos.vx += oneEighth;
      } else {
         if (abs(diff) > 2) {
            gCameraPos.vx += (oneEighth < 0) ? -2 : +2;
         } else {
            gCameraPos.vx += diff;
         }
      }
      // z
      diff = -(gState.eventCameraPan.z >> 3) - gCameraPos.vz;
      oneEighth = diff >> 3;
      if (abs(oneEighth) > 2) {
         gCameraPos.vz += oneEighth;
      } else {
         if (abs(diff) > 2) {
            gCameraPos.vz += (oneEighth < 0) ? -2 : +2;
         } else {
            gCameraPos.vz += diff;
         }
      }
      // y
      diff = ((gState.eventCameraPan.y + 0x100 + gState.eventCameraHeight) >> 3) - gCameraPos.vy;
      oneEighth = diff >> 3;
      if (abs(oneEighth) > 2) {
         gCameraPos.vy += oneEighth;
      } else {
         if (abs(diff) > 2) {
            gCameraPos.vy += (oneEighth < 0) ? -2 : +2;
         } else {
            gCameraPos.vy += diff;
         }
      }
   }
}

void StepEntitySpriteTowardDest(Object *sprite, Object *entity) {
   // TBD: Is this y coord used as state anywhere else?
   switch (HI(sprite->d.sprite.coords[0].y)) {
   case 0:
      if (sprite->x1.n == sprite->x3.n && sprite->z1.n == sprite->z3.n) {
         return;
      }
      sprite->d.sprite.finishedMoving = 0;
      if (sprite->x1.n < sprite->x3.n) {
         HI(sprite->d.sprite.coords[0].y) += 1;
      }
      if (sprite->x1.n > sprite->x3.n) {
         HI(sprite->d.sprite.coords[0].y) += 2;
      }
      if (sprite->z1.n < sprite->z3.n) {
         HI(sprite->d.sprite.coords[0].y) += 3;
      }
      if (sprite->z1.n > sprite->z3.n) {
         HI(sprite->d.sprite.coords[0].y) += 4;
      }
      if (entity->d.objf409.maintainDirection == 0) {
         if (sprite->x1.n < sprite->x3.n) {
            sprite->d.sprite.direction = ANGLE_WEST;
         }
         if (sprite->x1.n > sprite->x3.n) {
            sprite->d.sprite.direction = ANGLE_EAST;
         }
         if (sprite->z1.n < sprite->z3.n) {
            sprite->d.sprite.direction = ANGLE_SOUTH;
         }
         if (sprite->z1.n > sprite->z3.n) {
            sprite->d.sprite.direction = ANGLE_NORTH;
         }
      }
      break;

   case 1:
      sprite->z1.n = sprite->z3.n;
      sprite->x1.n += sprite->x2.n;
      if (sprite->x1.n >= sprite->x3.n) {
         sprite->x1.n = sprite->x3.n;
         sprite->d.sprite.finishedMoving = 1;
         HI(sprite->d.sprite.coords[0].y) = 0;
      }
      break;

   case 2:
      sprite->z1.n = sprite->z3.n;
      sprite->x1.n -= sprite->x2.n;
      if (sprite->x1.n <= sprite->x3.n) {
         sprite->x1.n = sprite->x3.n;
         sprite->d.sprite.finishedMoving = 1;
         HI(sprite->d.sprite.coords[0].y) = 0;
      }
      break;

   case 3:
      sprite->x1.n = sprite->x3.n;
      sprite->z1.n += sprite->z2.n;
      if (sprite->z1.n >= sprite->z3.n) {
         sprite->z1.n = sprite->z3.n;
         sprite->d.sprite.finishedMoving = 1;
         HI(sprite->d.sprite.coords[0].y) = 0;
      }
      break;

   case 4:
      sprite->x1.n = sprite->x3.n;
      sprite->z1.n -= sprite->z2.n;
      if (sprite->z1.n <= sprite->z3.n) {
         sprite->z1.n = sprite->z3.n;
         sprite->d.sprite.finishedMoving = 1;
         HI(sprite->d.sprite.coords[0].y) = 0;
      }
      break;
   }
}

void SetupEventEntity_SingleSet(u8 **baseAnimSet, s16 *p, u8 stripIdxA, u8 stripIdxB) {
   Object *entity;

   entity = Obj_GetUnused();
   entity->functionIndex = OBJF_EVENT_ENTITY;
   entity->d.objf409.baseAnimSet = baseAnimSet;
   entity->d.objf409.pNextCommand = p;
   // Adding 2 since the area where strips 0 and 1 would be is used for other purposes (font, etc)
   entity->d.objf409.stripIdxA = stripIdxA + 2;
   entity->d.objf409.stripIdxB = stripIdxB + 2;
}

void SetupEventEntity(u8 **baseAnimSet, u8 **altAnimSet, s16 *p, u8 stripIdxA, u8 stripIdxB) {
   Object *entity;

   entity = Obj_GetUnused();
   entity->functionIndex = OBJF_EVENT_ENTITY;
   entity->d.objf409.baseAnimSet = baseAnimSet;
   entity->d.objf409.altAnimSet = altAnimSet;
   entity->d.objf409.pNextCommand = p;
   entity->d.objf409.stripIdxA = stripIdxA + 2;
   entity->d.objf409.stripIdxB = stripIdxB + 2;
}

void ReserveSprite(u8 srcIdxWithinSheet, u8 dstStripIdx, u8 dstSubIdx) {
   DVECTOR spriteOffsets[24] = {{0, 0},    {12, 0},   {24, 0},   {36, 0},   {0, 48},   {12, 48},
                                {24, 48},  {36, 48},  {0, 96},   {12, 96},  {24, 96},  {36, 96},
                                {0, 144},  {12, 144}, {24, 144}, {36, 144}, {0, 192},  {12, 192},
                                {24, 192}, {36, 192}, {0, 256},  {12, 256}, {24, 256}, {36, 256}};
   s32 spriteX;
   s32 stripX;
   RECT srcRect;
   s32 y;

   spriteX = dstSubIdx * (48 >> 2);
   if (dstSubIdx > 4) {
      // Jump the gap (icons)
      spriteX += 4;
   }

   srcIdxWithinSheet -= 1;
#ifdef PC_PORT
   /* spriteOffsets has 24 entries; retail indexes it with the raw event argument. An out-of-range
    * sprite id (0 -> this u8 underflows to 255, or >24) reads past the array, producing a garbage
    * srcRect that makes MoveImage walk off VRAM. Harmless stack garbage on PSX (coords stay inside the
    * 1MB framebuffer); a wild s_vram access on PC. Skip the reserve for an out-of-range id. */
   if (srcIdxWithinSheet >= 24)
      return;
#endif
   srcRect.w = 48 >> 2;
   srcRect.h = 48;
   srcRect.x = spriteOffsets[srcIdxWithinSheet].vx + 704;
   srcRect.y = spriteOffsets[srcIdxWithinSheet].vy;

   if (dstStripIdx > 4) {
      stripX = 768;
   } else {
      stripX = 640;
   }
   if (dstStripIdx > 14) {
      stripX += 128;
   }

   y = dstStripIdx % 5 * 48;
   if (dstStripIdx % 10 < 5) {
      y += 256;
   }

   MoveImage(&srcRect, stripX + spriteX, y);
   DrawSync(0);
}

