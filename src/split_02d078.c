/* Unit roster bookkeeping, unit-sprite lookups, number formatting, and the portrait
 * objects (segment 0x2d078).
 *
 * Roster: ClearUnits and CreateUnit with its two entry points -- CreateUnitInNextSlot
 * takes the first free gUnits slot, CreateUnitInLastSlot forces the top slot
 * (split_0496f8.c uses it for Leena) -- initialising per-unit variance and boost fields.
 *
 * Sprite lookups, used tree-wide: GetUnitSpriteAtPosition (the gMapUnits grid ->
 * gUnits[].sprite hop nearly every fx_* unit starts from), FindUnitSpriteByNameIdx,
 * FindUnitSpriteByType (UNREFERENCED in retail), GetUnitSpriteVramRect (source rect for
 * the sprite-copy effects), and StartUnitSpritesDecoder, which re-arms the shared decoder
 * object over one strip's packed sheet.
 *
 * Four SaveRestore* static pairs (damage, hp percentages, hp triple, screen position) are
 * a one-slot mailbox: battle_013b94.c and window.c stash values that
 * Objf032_033_DisplayDamage reads back a frame later. IntToLeftPaddedGlyphs[2] (fixed 3
 * digits; _2 uses the damage gauge's alternate digit font) and IntToGlyphs render numbers
 * into glyph strips.
 *
 * Handlers: Objf032_033_DisplayDamage (floating damage number + hp bar) and four portrait
 * objects -- Objf008_BattlePortrait, Objf413_MsgBoxPortrait (mouth/eye overlays with
 * speaking/blinking sub-machines), Objf447_UnitPortrait with its Objf448 wrapper (448 is
 * dispatched by nothing in retail), and Objf575_StatusPortrait. All share one idiom:
 * search gState.portraitsToLoad for the portrait id (slot 0 fallback), take the clut from
 * gPortraitClutIds, aim GFX_PORTRAIT_A/B_FACE at the right 48x48 cell. */
#include "common.h"
#include "object.h"
#include "state.h"
#include "graphics.h"
#include "field.h"
#include "units.h"
#include "glyphs.h"

void ClearUnits(void) {
   s32 i;
   UnitStatus *pUnit;

   for (i = 0; i < UNIT_CT; i++) {
      pUnit = &gUnits[i];
      pUnit->idx = 0;
   }
}

UnitStatus *CreateUnitInNextSlot(void) { return CreateUnit(0); }

UnitStatus *CreateUnitInLastSlot(void) { return CreateUnit(1); }

UnitStatus *CreateUnit(u8 last) {
   s32 i, j;
   UnitStatus *pUnit = &gUnits[1];

   for (i = 1; i < UNIT_CT; i++, pUnit++) {
      if (pUnit->idx == 0) {
         if (last) {
            pUnit = &gUnits[UNIT_CT - 1];
            i = UNIT_CT - 1;
         }
         pUnit->idx = i;
         pUnit->level = 1;
         pUnit->team = TEAM_NULL;
         pUnit->done = 0;
         for (j = 0; j < 8; j++) {
            pUnit->experience[j] = 0;
         }
         pUnit->exp = 0;
         pUnit->hpFrac = 10000;
         pUnit->mp = 0;
         pUnit->atkVar10000 = 10000;
         pUnit->defVar10000 = 10000;
         pUnit->agiVar10000 = 10000;
         pUnit->hpVar100 = 100;
         pUnit->atkVar100 = 100;
         pUnit->defVar100 = 100;
         pUnit->agiVar100 = 100;
         pUnit->direction = 0;
         pUnit->battler = NULL;
         pUnit->sprite = NULL;
         pUnit->hideMarker = 0;
         pUnit->poisoned = 0;
         pUnit->paralyzed = 0;
         pUnit->atkBoosted = 0;
         pUnit->defBoosted = 0;
         pUnit->aglBoosted = 0;
         break;
      }
   }
   return pUnit;
}

void StartUnitSpritesDecoder(u8 stripIdx) {
   Object *obj = &gUnitSpritesDecoder;

   stripIdx -= 2;
   obj->functionIndex = OBJF_UNIT_SPRITES_DECODER;
   obj->state = 0;
   obj->d.objf050.baseSrcDataPtr = gEncodedUnitSpriteData[gUnitSetEncodedSpriteDataIdx[stripIdx]];
   obj->d.objf050.baseDstDataPtr = gScratch1_801317c0;
   obj->d.objf050.vramX = 704;
   obj->d.objf050.vramY = 0;
}

Object *GetUnitSpriteAtPosition(u8 z, u8 x) {
   Object *unitSprite;
   u8 unitIdx = gMapUnitsPtr[z][x].s.unitIdx;

   if (unitIdx == 0
#ifdef PC_PORT
       || unitIdx >= UNIT_CT   /* latent retail OOB: a stale / off-map grid cell can hold an idx past
                                * gUnits[UNIT_CT] (PSX reads in-arena garbage; PC faults). Treat as empty. */
#endif
      ) {
      unitSprite = NULL;
   } else {
      unitSprite = gUnits[unitIdx].sprite;
   }

   return unitSprite;
}

Object *FindUnitSpriteByNameIdx(u8 nameIdx) {
   s32 i;
   UnitStatus *pUnit = &gUnits[1];

   for (i = 1; i < UNIT_CT; i++) {
      if (pUnit->idx != 0 && pUnit->name == nameIdx) {
         return pUnit->sprite;
      }
      pUnit++;
   }

   return NULL;
}

/* UNREFERENCED in retail -- kept byte-exact. */
Object *FindUnitSpriteByType(u8 unitType) {
   s32 i;
   UnitStatus *pUnit = &gUnits[1];

   for (i = 1; i < UNIT_CT; i++) {
      if (pUnit->idx != 0 && pUnit->unitType == unitType) {
         return pUnit->sprite;
      }
      pUnit++;
   }

   return NULL;
}

void GetUnitSpriteVramRect(Object *unitSprite, s32 *px, s32 *py, s32 *pw, s32 *ph) {
   s32 tpage;
   s32 gfx = unitSprite->d.sprite.gfxIdx;
   s32 stripIdx = unitSprite->d.sprite.stripIdx;

   if (gfx < 35) {
      if (gfx < 26) {
         *px = 0xb00;
         *py = 0;
      } else {
         *px = 0xb00;
         *py = 0x100;
      }
      *px += gTexwSpriteSetFrames[gfx].x;
      *py += gTexwSpriteSetFrames[gfx].y;
      *pw = gTexwSpriteSetFrames[gfx].w;
      *ph = gTexwSpriteSetFrames[gfx].h;
   } else {
      if (gfx < 40) {
         tpage = gSpriteStripTPageIds[stripIdx * 2];
         gfx += (stripIdx % 5) * 5;
         gfx -= 35;
      } else {
         tpage = gSpriteStripTPageIds[stripIdx * 2 + 1];
         gfx += (stripIdx % 5) * 5;
         gfx -= 40;
      }
      if (tpage >= 16) {
         tpage -= 16;
         *px = tpage * 0x100;
         *py = 0x100;
      } else {
         *px = tpage * 0x100;
         *py = 0;
      }
      *px += gTexwSpriteStripFrames[gfx].x;
      *py += gTexwSpriteStripFrames[gfx].y;
      *pw = gTexwSpriteStripFrames[gfx].w;
      *ph = gTexwSpriteStripFrames[gfx].h;
   }
}

s16 s_savedDamage_80123230;

void SaveRestoreDamage(s16 *damage, s32 restore) {
   if (!restore) {
      s_savedDamage_80123230 = *damage;
   } else {
      *damage = s_savedDamage_80123230;
   }
}

s16 s_savedBeforeHpPercent_80123234;
s16 s_savedAfterHpPercent_80123238;

void SaveRestoreHpPercents(s16 *before, s16 *after, s32 restore) {
   if (!restore) {
      s_savedBeforeHpPercent_80123234 = *before;
      s_savedAfterHpPercent_80123238 = *after;
   } else {
      *before = s_savedBeforeHpPercent_80123234;
      *after = s_savedAfterHpPercent_80123238;
   }
}

s16 s_savedMaxHp_8012323c;
s16 s_savedPrevHp_80123240;
s16 s_savedCurHp_80123244;

void SaveRestoreHp(s16 *maxHp, s16 *prevHp, s16 *curHp, s32 restore) {
   if (!restore) {
      s_savedMaxHp_8012323c = *maxHp;
      s_savedPrevHp_80123240 = *prevHp;
      s_savedCurHp_80123244 = *curHp;
   } else {
      *maxHp = s_savedMaxHp_8012323c;
      *prevHp = s_savedPrevHp_80123240;
      *curHp = s_savedCurHp_80123244;
   }
}

s16 s_savedPosX_80123248;
s16 s_savedPosY_8012324c;

void SaveRestorePos(s16 *x, s16 *y, s32 restore) {
   if (!restore) {
      s_savedPosX_80123248 = *x;
      s_savedPosY_8012324c = *y;
   } else {
      *x = s_savedPosX_80123248;
      *y = s_savedPosY_8012324c;
   }
}

void IntToLeftPaddedGlyphs2(s16 n, u8 *dst) {
   // Uses alternative digit font (e.g. for damage hp gauge)
   s16 units;

   units = n / 100;
   n -= units * 100;
   *dst++ = GLYPH_CHAR_ALT_0 + units;

   units = n / 10;
   n -= units * 10;
   *dst++ = GLYPH_CHAR_ALT_0 + units;

   *dst = GLYPH_CHAR_ALT_0 + n;

   // Replace leading zeroes with blanks
   if (dst[-2] == GLYPH_CHAR_ALT_0) {
      dst[-2] = GLYPH_BG;
      if (dst[-1] == GLYPH_CHAR_ALT_0) {
         dst[-1] = GLYPH_BG;
      }
   }
}

void IntToLeftPaddedGlyphs(s16 n, u8 *dst) {
   s16 units;

   units = n / 100;
   n -= units * 100;
   *dst++ = GLYPH_CHAR_0 + units;

   units = n / 10;
   n -= units * 10;
   *dst++ = GLYPH_CHAR_0 + units;

   *dst = GLYPH_CHAR_0 + n;

   // Replace leading zeroes with blanks
   if (dst[-2] == GLYPH_CHAR_0) {
      dst[-2] = GLYPH_BG;
      if (dst[-1] == GLYPH_CHAR_0) {
         dst[-1] = GLYPH_BG;
      }
   }
}

s32 IntToGlyphs(s16 n, u8 *dst) {
   s16 units;
   s32 len = 1;
   s32 started = 0;

   units = n / 1000;
   n -= units * 1000;

   if (units != 0 || started) {
      *dst++ = GLYPH_CHAR_0 + units;
      started = 1;
      len++;
   }

   units = n / 100;
   n -= units * 100;

   if (units != 0 || started) {
      *dst++ = GLYPH_CHAR_0 + units;
      started = 1;
      len++;
   }

   units = n / 10;
   n -= units * 10;

   if (units != 0 || started) {
      *dst++ = GLYPH_CHAR_0 + units;
      started = 1;
      len++;
   }

   *dst = GLYPH_CHAR_0 + n;
   return len;
}

#undef OBJF
#define OBJF 032
void Objf032_033_DisplayDamage(Object *obj) {
   // FIXME: maxHp, prevHp, curHp?
   Object *obj_s2, *obj_s3, *unitSprite;
   SVECTOR screenXY;
   s32 p;
   s32 flag;
   s16 savedX;
   s16 savedY;
   s16 savedDamage;
   s16 savedHpBefore;
   s16 savedHpAfter;

   obj_s2 = OBJ.barFg;
   obj_s3 = OBJ.barBg;

   switch (obj->state) {
   case 0:
      unitSprite = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
      RotTransPers(&unitSprite->vec, &screenXY, &p, &flag);
      screenXY.vy += 28;
      if (screenXY.vy > 0xc0) {
         screenXY.vy = 0xc0;
      }

      SaveRestorePos(&screenXY.vx, &screenXY.vy, 0);
      SaveRestorePos(&savedX, &savedY, 1);
      SaveRestoreDamage(&savedDamage, 1);

      if (savedDamage != 0) {
         obj_s2 = Obj_GetUnused();
         obj_s2->functionIndex = OBJF_FLOATING_DAMAGE_TEXT;
         obj_s2->x1.s.hi = obj->x1.s.hi;
         obj_s2->z1.s.hi = obj->z1.s.hi;
         obj_s2->d.objf051.damage = savedDamage;
         obj_s2->d.objf051.clut = CLUT_BLUES;
      }

      if (OBJ_MAP_UNIT(obj).s.team == TEAM_PLAYER) {
         DisplayBasicWindow(0x1b);
         DisplayBasicWindow(0x1c);
      } else {
         DisplayCustomWindow(0x1b, 0, 1, 0, 0, 23);
         DisplayCustomWindow(0x1c, 0, 1, 0, 0, 23);
      }

      SaveRestoreHpPercents(&savedHpBefore, &savedHpAfter, 1);

      obj_s2 = Obj_GetUnused();
      OBJ.barFg = obj_s2;
      obj_s2->functionIndex = OBJF_NOOP;
      obj_s2->x1.n = savedX - 56;
      obj_s2->x3.n = savedX - 56 + (savedHpBefore * 80 / 100);
      obj_s2->y1.n = savedY - 4;
      obj_s2->y3.n = savedY + 4;
      obj_s2->d.sprite.gfxIdx = GFX_COLOR_5;
      obj_s2->d.sprite.clut = CLUT_REDS;
      obj_s2->d.sprite.otOfs = 3;

      obj_s3 = Obj_GetUnused();
      OBJ.barBg = obj_s3;
      obj_s3->functionIndex = OBJF_NOOP;
      obj_s3->x1.n = savedX - 56;
      obj_s3->x3.n = savedX + 23;
      obj_s3->y1.n = savedY - 4;
      obj_s3->y3.n = savedY + 4;
      obj_s3->d.sprite.gfxIdx = GFX_COLOR_1;
      obj_s3->d.sprite.otOfs = 3;

      if (savedHpAfter < savedHpBefore) {
         //?
         s32 tmp = savedHpAfter;
         if ((tmp < 3) && (tmp % 2)) {
            savedHpAfter = 3;
         }
         OBJ.barDstX3 = (savedX - 56) + (savedHpAfter * 80 / 100);
      } else {
         OBJ.barDstX3 = (savedX - 54) + (savedHpAfter * 80 / 100);
      }

      //?
      SaveRestoreHp((s16 *)&OBJ.maxHp, (s16 *)&OBJ.prevHp, (s16 *)&OBJ.curHp, 1);
      OBJ.maxHp = (s16)OBJ.maxHp;
      OBJ.prevHp = (s16)OBJ.prevHp << 0x10;
      OBJ.curHp = (s16)OBJ.curHp << 0x10;

      OBJ.timer = 40;
      obj->state++;

   // fallthrough
   case 1:
      OBJ.prevHp += (OBJ.curHp - OBJ.prevHp) >> 2;
      if (OBJ.timer == 30) {
         OBJ.prevHp = OBJ.curHp;
      }

      IntToLeftPaddedGlyphs2(OBJ.prevHp >> 0x10, &gGlyphStrip_50[1]);
      IntToLeftPaddedGlyphs2(OBJ.maxHp, &gGlyphStrip_50[5]);
      DrawGlyphStripGroup(gGlyphStripGroups[28], GFX_TBD_685);

      obj_s2->x3.n += (OBJ.barDstX3 - obj_s2->x3.n) >> 2;

      if (--OBJ.timer == 0) {
         CloseWindow(0x1b);
         CloseWindow(0x1c);
         obj->functionIndex = OBJF_NULL;
         obj_s2->functionIndex = OBJF_NULL;
         obj_s3->functionIndex = OBJF_NULL;
         return;
      }
   }

   if (obj_s2->x3.n > obj_s3->x1.n) {
      // Bar fill (if visible)
      AddObjPrim_Gui(gGraphicsPtr->ot, obj_s2);
   }
   // Bar background
   AddObjPrim_Gui(gGraphicsPtr->ot, obj_s3);
}

#undef OBJF
#define OBJF 008
void Objf008_BattlePortrait(Object *obj) {
   Object *window, *sprite;
   u8 gfx;
   s32 i;
   s16 portraitNum;

   sprite = OBJ.sprite;
   window = OBJ.window;

   gfx = OBJ.flipped ? GFX_PORTRAIT_B_FACE : GFX_PORTRAIT_A_FACE;

   switch (obj->state) {
   case 0:
      window = &gObjectArray[0];
      for (i = 0; i < OBJ_DATA_CT; i++) {
         if ((window->functionIndex == OBJF_WINDOW_TBD_004 ||
              window->functionIndex == OBJF_WINDOW_TBD_005) &&
             window->d.objf004.windowId == OBJ.windowId) {
            OBJ.window = window;
            break;
         }
         window++;
      }
      sprite = Obj_GetUnused();
      sprite->functionIndex = OBJF_NOOP;
      sprite->d.sprite.gfxIdx = gfx;
      sprite->d.sprite.otOfs = 2;
      OBJ.sprite = sprite;
      obj->state++;
      break;

   case 1:
      if (OBJ.flipped) {
         sprite->x1.n = window->x1.n + 24;
         sprite->x3.n = window->x1.n - 23;
      } else {
         sprite->x1.n = window->x1.n - 23;
         sprite->x3.n = window->x1.n + 24;
      }
      sprite->y1.n = window->y1.n - 23;
      sprite->y3.n = window->y1.n + 24;

      for (portraitNum = 0; portraitNum <= 60; portraitNum++) {
         if (gState.portraitsToLoad[portraitNum] == OBJ.portraitId) {
            break;
         }
      }
      if (portraitNum == 61) {
         portraitNum = 0;
      }

      sprite->d.sprite.clut = gPortraitClutIds[portraitNum];

      if (portraitNum < 25) {
         gGfxTPageIds[gfx] = gTPageIds[10];
      } else {
         if (portraitNum < 50) {
            gGfxTPageIds[gfx] = gTPageIds[7];
         } else {
            gGfxTPageIds[gfx] = gTPageIds[5];
         }
      }

      if (portraitNum >= 50) {
         portraitNum -= 50;
      }
      if (portraitNum >= 25) {
         portraitNum -= 25;
      }

      gGfxSubTextures[gfx][0] = (portraitNum % 5) * 48;
      gGfxSubTextures[gfx][1] = (portraitNum / 5) * 48;
      break;
   }

   AddObjPrim_Gui(gGraphicsPtr->ot, sprite);
}

#undef OBJF
#define OBJF 413
void Objf413_MsgBoxPortrait(Object *obj) {
   // obj->state3: blinkState
   Object *speakSprite, *blinkSprite, *faceSprite, *anchor;
   // This could use some fiddling
   u8 portrait_u8;
   s16 portrait_s16;
   s32 gfx;

   faceSprite = OBJ.faceSprite;
   speakSprite = OBJ.speakSprite;
   blinkSprite = OBJ.blinkSprite;

   portrait_u8 = OBJ.portrait.idx;

   switch (obj->state) {
   case 0:
      for (portrait_s16 = 0; portrait_s16 < 75; portrait_s16++) {
         if (gState.portraitsToLoad[portrait_s16] == OBJ.portrait.id) {
            break;
         }
      }
      if (portrait_s16 == 75) {
         portrait_s16 = 0;
      }

      // Starts as s16 portrait ID to search for, and is replaced by the resulting one-byte index
      OBJ.portrait.idx = portrait_s16;

      gfx = (!OBJ.flipped) ? GFX_PORTRAIT_A_FACE : GFX_PORTRAIT_B_FACE;

      portrait_s16 = OBJ.portrait.idx;
      portrait_u8 = OBJ.portrait.idx;

      if (portrait_s16 < 25) {
         gGfxTPageIds[gfx++] = gTPageIds[10];
      } else {
         gGfxTPageIds[gfx++] = gTPageIds[7];
      }

      switch (portrait_s16 / 5) {
      case 0:
         gGfxTPageIds[gfx++] = gTPageIds[0];
         gGfxTPageIds[gfx++] = gTPageIds[0];
         gGfxTPageIds[gfx] = gTPageIds[0];
         break;
      case 1:
         gGfxTPageIds[gfx++] = gTPageIds[1];
         gGfxTPageIds[gfx++] = gTPageIds[1];
         gGfxTPageIds[gfx] = gTPageIds[1];
         break;
      case 2:
         gGfxTPageIds[gfx++] = gTPageIds[2];
         gGfxTPageIds[gfx++] = gTPageIds[2];
         gGfxTPageIds[gfx] = gTPageIds[2];
         break;
      case 3:
         gGfxTPageIds[gfx++] = gTPageIds[3];
         gGfxTPageIds[gfx++] = gTPageIds[3];
         gGfxTPageIds[gfx] = gTPageIds[3];
         break;
      case 4:
         gGfxTPageIds[gfx++] = gTPageIds[4];
         gGfxTPageIds[gfx++] = gTPageIds[4];
         gGfxTPageIds[gfx] = gTPageIds[4];
         break;
      case 5:
         gGfxTPageIds[gfx++] = gTPageIds[16];
         gGfxTPageIds[gfx++] = gTPageIds[16];
         gGfxTPageIds[gfx] = gTPageIds[16];
         break;
      case 6:
         gGfxTPageIds[gfx++] = gTPageIds[17];
         gGfxTPageIds[gfx++] = gTPageIds[17];
         gGfxTPageIds[gfx] = gTPageIds[17];
         break;
      case 7:
         gGfxTPageIds[gfx++] = gTPageIds[18];
         gGfxTPageIds[gfx++] = gTPageIds[18];
         gGfxTPageIds[gfx] = gTPageIds[18];
         break;
      case 8:
         gGfxTPageIds[gfx++] = gTPageIds[19];
         gGfxTPageIds[gfx++] = gTPageIds[19];
         gGfxTPageIds[gfx] = gTPageIds[19];
         break;
      case 9:
         gGfxTPageIds[gfx++] = gTPageIds[20];
         gGfxTPageIds[gfx++] = gTPageIds[20];
         gGfxTPageIds[gfx] = gTPageIds[20];
         break;
      }
      gfx -= 3;

      // Mouth overlays
      gGfxSubTextures[gfx + 1][0] = (portrait_s16 % 5) * 48;
      gGfxSubTextures[gfx + 2][0] = (portrait_s16 % 5) * 48 + 16;

      // Eye overlay
      gGfxSubTextures[gfx + 3][0] = (portrait_s16 % 5) * 48 + 32;

      if (portrait_s16 >= 25) {
         portrait_s16 -= 25;
      }

      gGfxSubTextures[gfx][0] = (portrait_s16 % 5) * 48;
      gGfxSubTextures[gfx][1] = (portrait_s16 / 5) * 48;

      faceSprite = Obj_GetUnused();
      faceSprite->functionIndex = OBJF_NOOP;
      faceSprite->d.sprite.gfxIdx = gfx;
      faceSprite->d.sprite.otOfs = 3;
      faceSprite->d.sprite.clut = gPortraitClutIds[portrait_u8];
      OBJ.faceSprite = faceSprite;

      speakSprite = Obj_GetUnused();
      speakSprite->functionIndex = OBJF_NOOP;
      speakSprite->d.sprite.gfxIdx = gfx + 1;
      speakSprite->d.sprite.clut = gPortraitClutIds[portrait_u8];
      speakSprite->d.sprite.otOfs = 2;
      OBJ.speakSprite = speakSprite;

      blinkSprite = Obj_GetUnused();
      blinkSprite->functionIndex = OBJF_NOOP;
      blinkSprite->d.sprite.gfxIdx = gfx + 3;
      blinkSprite->d.sprite.clut = gPortraitClutIds[portrait_u8];
      blinkSprite->d.sprite.otOfs = 2;
      OBJ.blinkSprite = blinkSprite;

      obj->state2 = 0; // (speaking state)
      obj->state3 = 0; // (blinking state)
      obj->state++;
      break;

   case 1:
      break;

   case 99:
      obj->functionIndex = OBJF_NULL;
      faceSprite->functionIndex = OBJF_NULL;
      speakSprite->functionIndex = OBJF_NULL;
      blinkSprite->functionIndex = OBJF_NULL;
      return;
   }

   anchor = OBJ.anchor;
   if (anchor != NULL) {
      obj->x1.n = anchor->x1.n + OBJ.anchorOfsX;
      obj->y1.n = anchor->y1.n + OBJ.anchorOfsY;
   }

   if (!OBJ.flipped) {
      faceSprite->x1.n = obj->x1.n;
      faceSprite->x3.n = obj->x1.n + 47;
   } else {
      faceSprite->x1.n = obj->x1.n + 47;
      faceSprite->x3.n = obj->x1.n;
   }
   faceSprite->y1.n = obj->y1.n;
   faceSprite->y3.n = obj->y1.n + 47;

   if (!OBJ.flipped) {
      speakSprite->x1.n = obj->x1.n + gPortraitOverlayOffsets[portrait_u8].s.speakX;
      speakSprite->x3.n = speakSprite->x1.n + 15;
   } else {
      speakSprite->x1.n = obj->x1.n + 46 - gPortraitOverlayOffsets[portrait_u8].s.speakX + 1;
      speakSprite->x3.n = speakSprite->x1.n - 15;
   }
   speakSprite->y1.n = obj->y1.n + gPortraitOverlayOffsets[portrait_u8].s.speakY;
   speakSprite->y3.n = speakSprite->y1.n + 16;

   if (!OBJ.flipped) {
      blinkSprite->x1.n = obj->x1.n + gPortraitOverlayOffsets[portrait_u8].s.blinkX;
      blinkSprite->x3.n = blinkSprite->x1.n + 15;
   } else {
      blinkSprite->x1.n = obj->x1.n + 46 - gPortraitOverlayOffsets[portrait_u8].s.blinkX + 1;
      blinkSprite->x3.n = blinkSprite->x1.n - 15;
   }
   blinkSprite->y1.n = obj->y1.n + gPortraitOverlayOffsets[portrait_u8].s.blinkY;
   blinkSprite->y3.n = blinkSprite->y1.n + 16;

   if (gState.msgTextWaitTimer[OBJ.flipped + 1] != 0) {
      gState.msgTextWaitTimer[OBJ.flipped + 1]--;

      // Speaking animation
      switch (obj->state2) {
      case 0:
         gfx = (gState.vsyncMode != 2) ? 2 : 1;
         OBJ.speakTimer = gfx;
         obj->state2++;

      // fallthrough
      case 1:
         if (--OBJ.speakTimer == 0) {
            gfx = (gState.vsyncMode != 2) ? 6 : 3;
            OBJ.speakTimer = gfx;
            obj->state2++;
         }
         break;

      case 2:
         speakSprite->d.sprite.hidden = 0;

         if (--OBJ.speakTimer == 0) {
            speakSprite->d.sprite.gfxIdx++;
            gfx = (gState.vsyncMode != 2) ? 4 : 2;
            OBJ.speakTimer = gfx;
            obj->state2++;
         }
         break;

      case 3:
         speakSprite->d.sprite.hidden = 0;

         if (--OBJ.speakTimer == 0) {
            speakSprite->d.sprite.gfxIdx--;
            gfx = (gState.vsyncMode != 2) ? 4 : 2;
            OBJ.speakTimer = gfx;
            obj->state2++;
         }
         break;

      case 4:
         speakSprite->d.sprite.hidden = 0;

         if (--OBJ.speakTimer == 0) {
            gfx = (gState.vsyncMode != 2) ? 2 : 1;
            OBJ.speakTimer = gfx;
            speakSprite->d.sprite.hidden = 1;
            obj->state2 -= 3;
         }
         break;
      }

   } else {
      speakSprite->d.sprite.hidden = 1;
   }

   // Blinking animation
   switch (obj->state3) {
   case 0:
      blinkSprite->d.sprite.hidden = 1;

      if (gState.vsyncMode == 2) {
         OBJ.blinkTimer = rand() % 50 + 20;
         if (rand() == 0) {
            OBJ.blinkTimer = 2;
         }
      } else {
         OBJ.blinkTimer = rand() % 100 + 40;
         if (rand() == 0) {
            OBJ.blinkTimer = 4;
         }
      }
      obj->state3++;
      break;

   case 1:
      if (--OBJ.blinkTimer == 0) {
         blinkSprite->d.sprite.hidden = 0;

         if (gState.vsyncMode == 2) {
            OBJ.blinkTimer = 5;
            if (rand() == 0) {
               //?: TINY chance to blink 4x as long?
               OBJ.blinkTimer = 20;
            }
         } else {
            OBJ.blinkTimer = 10;
            if (rand() == 0) {
               OBJ.blinkTimer = 40;
            }
         }
         obj->state3++;
      }
      break;

   case 2:
      if (--OBJ.blinkTimer == 0) {
         obj->state3 -= 2;
      }
      break;
   }

   AddObjPrim_Gui(gGraphicsPtr->ot, faceSprite);
   AddObjPrim_Gui(gGraphicsPtr->ot, speakSprite);
   AddObjPrim_Gui(gGraphicsPtr->ot, blinkSprite);
}

/* Dispatched by nothing in retail: no enum, no spell/event/scene-loader/map-table entry
 * (verified against all dispatcher dumps) -- an unused wrapper over Objf447. */
void Objf448_UnitPortraitWrapper(Object *obj) { Objf447_UnitPortrait(obj); }

#undef OBJF
#define OBJF 447
void Objf447_UnitPortrait(Object *obj) {
   Object *sprite, *window;
   s32 i;
   s16 portraitNum;

   if (gState.unitListPortraitId == 0) {
      return;
   }

   sprite = OBJ.sprite;
   window = OBJ.window;

   switch (obj->state) {
   case 0:
      window = &gObjectArray[0];
      for (i = 0; i < OBJ_DATA_CT; i++) {
         if ((window->functionIndex == OBJF_WINDOW_TBD_004 ||
              window->functionIndex == OBJF_WINDOW_TBD_005) &&
             window->d.objf004.windowId == OBJ.windowId) {
            OBJ.window = window;
            break;
         }
         window++;
      }
      sprite = Obj_GetUnused();
      sprite->functionIndex = OBJF_NOOP;
      sprite->d.sprite.gfxIdx = GFX_PORTRAIT_B_FACE;
      sprite->d.sprite.otOfs = 10;
      OBJ.sprite = sprite;
      obj->state++;

   // fallthrough
   case 1:
      sprite->x1.n = window->x1.n + 24;
      sprite->x3.n = window->x1.n - 23;
      sprite->y1.n = window->y1.n - 23;
      sprite->y3.n = window->y1.n + 24;

      for (portraitNum = 0; portraitNum <= 60; portraitNum++) {
         if (gState.portraitsToLoad[portraitNum] == gState.unitListPortraitId) {
            break;
         }
      }
      if (portraitNum == 61) {
         portraitNum = 0;
      }

      sprite->d.sprite.clut = gPortraitClutIds[portraitNum];

      if (portraitNum < 25) {
         gGfxTPageIds[GFX_PORTRAIT_B_FACE] = gTPageIds[10];
      } else {
         if (portraitNum < 50) {
            gGfxTPageIds[GFX_PORTRAIT_B_FACE] = gTPageIds[7];
         } else {
            gGfxTPageIds[GFX_PORTRAIT_B_FACE] = gTPageIds[5];
         }
      }

      if (portraitNum >= 50) {
         portraitNum -= 50;
      }
      if (portraitNum >= 25) {
         portraitNum -= 25;
      }

      gGfxSubTextures[GFX_PORTRAIT_B_FACE][0] = (portraitNum % 5) * 48;
      gGfxSubTextures[GFX_PORTRAIT_B_FACE][1] = (portraitNum / 5) * 48;
      break;

   case 99:
      obj->functionIndex = OBJF_NULL;
      sprite->functionIndex = OBJF_NULL;
      return;
   }

   AddObjPrim_Gui(gGraphicsPtr->ot, sprite);
}

#undef OBJF
#define OBJF 575
void Objf575_StatusPortrait(Object *obj) {
   s16 portraitNum;

   switch (obj->state) {
   case 0:
      OBJ.gfxIdx = GFX_PORTRAIT_B_FACE;
      OBJ.otOfs = 2;
      obj->state++;

   // fallthrough
   case 1:
      obj->x1.n = 29;
      obj->x3.n = 76;
      obj->y1.n = 65;
      obj->y3.n = 112;

      for (portraitNum = 0; portraitNum < 75; portraitNum++) {
         if (gState.portraitsToLoad[portraitNum] == OBJ.portraitId) {
            break;
         }
      }
      if (portraitNum == 75) {
         portraitNum = 0;
      }

      OBJ.clut = gPortraitClutIds[portraitNum];

      if (portraitNum < 25) {
         gGfxTPageIds[GFX_PORTRAIT_B_FACE] = gTPageIds[10];
      } else {
         gGfxTPageIds[GFX_PORTRAIT_B_FACE] = gTPageIds[7];
      }

      if (portraitNum >= 25) {
         portraitNum -= 25;
      }

      gGfxSubTextures[GFX_PORTRAIT_B_FACE][0] = (portraitNum % 5) * 48;
      gGfxSubTextures[GFX_PORTRAIT_B_FACE][1] = (portraitNum / 5) * 48;

      obj->state++;
      break;

   case 2:
      break;
   }

   AddObjPrim_Gui(gGraphicsPtr->ot, obj);
}
