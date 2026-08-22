#include "common.h"
#include "object.h"
#include "state.h"
#include "graphics.h"

void SetScreenEffectOrdering(s16 otOfs) { gState.screenEffect->d.objf369.otOfs = otOfs; }

void Noop_800a8b0c(void) {}

#undef OBJF
#define OBJF 369
void Objf369_ScreenEffect(Object *obj) {
   extern DR_MODE s_drawModes1_80124804[2];
   extern DR_MODE s_drawModes2_8012481c[2];
   extern POLY_F4 s_polys_801247d4[2];

   s16 otOfs;
   s32 i;
   POLY_F4 *pPoly;
   s16 r, g, b;
   s16 r2, g2, b2;
   u8 intensity;
   DR_MODE *pDrawMode;
   // state2: abr

   obj->mem = (obj->mem + 1) & 1;
   otOfs = OBJ.otOfs + (OT_SIZE - 3);
   if (otOfs > (OT_SIZE - 2)) {
      otOfs = (OT_SIZE - 2);
   }

   switch (obj->state) {
   case 0:
      OBJ.semiTrans = 1;
      pPoly = &s_polys_801247d4[0];

      for (i = 0; i < 2; i++) {
         SetPolyF4(pPoly);
         if (OBJ.semiTrans != 0) {
            setSemiTrans(pPoly, 1);
         } else {
            setSemiTrans(pPoly, 0);
         }
         setXYWH(pPoly, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
         pPoly++;
         SetDrawMode(&s_drawModes1_80124804[i], 0, 0, GetTPage(0, 1, 640, 0), NULL);
         SetDrawMode(&s_drawModes2_8012481c[i], 0, 0, GetTPage(0, 0, 640, 0), NULL);
      }

      OBJ.intensity = 0;
      LO(OBJ.speed) = 2; //?
      break;

   case 1:
      if (obj->state2 == 0) {
         break;
      }
      for (i = 0; i < 2; i++) {
         SetDrawMode(&s_drawModes1_80124804[i], 0, 0, GetTPage(0, obj->state2, 640, 0), NULL);
      }

      OBJ.speed = obj->state3;
      OBJ.intensity = 0;
      obj->state++;

   // fallthrough
   case 2:
      OBJ.intensity += OBJ.speed;
      if (OBJ.intensity >= 255) {
         OBJ.intensity = 255;
         obj->state2 = 0;
         obj->state++;
      }
      break;

   case 3:
      if (obj->state2 == 0) {
         break;
      }
      OBJ.speed = obj->state3;
      OBJ.intensity = 255;
      obj->state++;

   // fallthrough
   case 4:
      OBJ.intensity -= OBJ.speed;
      if (OBJ.intensity <= 0) {
         OBJ.intensity = 0;
         obj->state2 = 0;
         obj->state = 1;
         OBJ.unused_0x30 = 0;
      }
      break;

   case 5:
   case 6:
   case 7:
      r = OBJ.color.r;
      g = OBJ.color.g;
      b = OBJ.color.b;
      r += OBJ.rd;
      g += OBJ.gd;
      b += OBJ.bd;
      if (OBJ.rd > 0 && r > OBJ.rmax) {
         r = OBJ.rmax;
      }
      if (OBJ.gd > 0 && g > OBJ.gmax) {
         g = OBJ.gmax;
      }
      if (OBJ.bd > 0 && b > OBJ.bmax) {
         b = OBJ.bmax;
      }
      if (OBJ.rd < 0 && r < OBJ.rmax) {
         r = OBJ.rmax;
      }
      if (OBJ.gd < 0 && g < OBJ.gmax) {
         g = OBJ.gmax;
      }
      if (OBJ.bd < 0 && b < OBJ.bmax) {
         b = OBJ.bmax;
      }
      OBJ.color.r = r;
      OBJ.color.g = g;
      OBJ.color.b = b;

      if (obj->state == 7) {
         if (r == OBJ.rmax && g == OBJ.gmax && b == OBJ.bmax) {
            obj->state++;
         }
      }
      break;

   case 8:
      OBJ.rd = 0;
      OBJ.gd = 0;
      OBJ.bd = 0;
      obj->state = 0;
      break;
   }

   switch (obj->state) {
   case 0:
   case 1:
      break;

   case 2:
   case 3:
   case 4:
      pDrawMode = &s_drawModes2_8012481c[obj->mem];
      addPrim(&gGraphicsPtr->ot[OT_SIZE - 2], pDrawMode);

      pPoly = &s_polys_801247d4[obj->mem];
      intensity = OBJ.intensity;
      setRGB0(pPoly, intensity, intensity, intensity);
      addPrim(&gGraphicsPtr->ot[otOfs], pPoly);

      pDrawMode = &s_drawModes1_80124804[obj->mem];
      addPrim(&gGraphicsPtr->ot[otOfs], pDrawMode);

      break;

   case 5:
   case 6:
   case 7:
      pPoly = s_polys_801247d4;
      if (OBJ.semiTrans != 0) {
         setSemiTrans(&s_polys_801247d4[obj->mem], 1);
      } else {
         setSemiTrans(&s_polys_801247d4[obj->mem], 0);
      }

      SetDrawMode(&s_drawModes1_80124804[obj->mem], 0, 0, GetTPage(0, obj->state2, 640, 0), NULL);

      pDrawMode = &s_drawModes2_8012481c[obj->mem];
      addPrim(&gGraphicsPtr->ot[OT_SIZE - 2], pDrawMode);

      r2 = OBJ.color.r;
      g2 = OBJ.color.g;
      b2 = OBJ.color.b;

      pPoly = &s_polys_801247d4[obj->mem];
      setRGB0(pPoly, r2, g2, b2);
      addPrim(&gGraphicsPtr->ot[otOfs], pPoly);

      pDrawMode = &s_drawModes1_80124804[obj->mem];
      addPrim(&gGraphicsPtr->ot[otOfs], pDrawMode);

      break;

   case 8:
      break;
   }
}

void SetScreenEffectIntensity(s32 intensity) {
   gState.screenEffect->d.objf369.color.r = intensity;
   gState.screenEffect->d.objf369.color.g = intensity;
   gState.screenEffect->d.objf369.color.b = intensity;
}

void Objf316_Noop(Object *obj) {}

void FadeOutScreen(s32 abr, s32 speed) {
   Object *screenEffect;

   screenEffect = gState.screenEffect;
   screenEffect->state2 = abr;
   screenEffect->d.objf369.rd = screenEffect->d.objf369.gd = screenEffect->d.objf369.bd = speed;
   screenEffect->d.objf369.rmax = screenEffect->d.objf369.gmax = screenEffect->d.objf369.bmax = 255;
   screenEffect->state = 6;
}

void FadeInScreen(s32 abr, s32 speed) {
   Object *screenEffect;

   screenEffect = gState.screenEffect;
   screenEffect->state2 = abr;
   screenEffect->d.objf369.rd = screenEffect->d.objf369.gd = screenEffect->d.objf369.bd = -speed;
   screenEffect->d.objf369.rmax = screenEffect->d.objf369.gmax = screenEffect->d.objf369.bmax = 0;
   screenEffect->state = 7;
}

Object *gScreenFade;

void Event_FadeOutScreen(s32 abr, s32 speed) {
   gScreenFade->state = 1;
   gScreenFade->state3 = abr;
   gScreenFade->d.objf795.delta = speed;
   gScreenFade->d.objf795.max = 255;
}

void Event_FadeInScreen(s32 abr, s32 speed) {
   gScreenFade->state = 2;
   gScreenFade->state3 = abr;
   gScreenFade->d.objf795.delta = -speed;
   gScreenFade->d.objf795.max = 0;
}

#undef OBJF
#define OBJF 795
void Objf795_EventFade(Object *obj) {
   //? Why so many elements?
   extern POLY_F4 s_polys_80124834[2][16];
   extern DR_MODE s_drawModes_80124b34[2][32];

   POLY_F4 *pPoly;
   DR_MODE *pDrawMode;
   s16 fade;
   s16 finished;
   s16 otOfs;
   u8 b;
   // state3: abr

   obj->mem = (obj->mem + 1) & 1;
   otOfs = OT_SIZE - 2;

   switch (obj->state) {
   case 0:
      break;

   case 1:
   case 2:
      fade = OBJ.fade;
      fade += OBJ.delta;
      finished = 0;
      if (OBJ.delta > 0 && fade > OBJ.max) {
         fade = OBJ.max;
         finished = 1;
      }
      if (OBJ.delta < 0 && fade < OBJ.max) {
         fade = OBJ.max;
         finished = 1;
      }
      OBJ.fade = fade;
      if (obj->state == 2 && finished) {
         obj->state++;
      }
      break;

   // fallthrough
   case 3:
      obj->state = 0;
      break;
   }

   switch (obj->state) {
   case 1:
   case 2:
      pDrawMode = &s_drawModes_80124b34[obj->mem][0];
      SetDrawMode(pDrawMode, 0, 0, GetTPage(0, 1, 640, 0), NULL);
      AddPrim(&gGraphicsPtr->ot[otOfs], pDrawMode);

      pPoly = &s_polys_80124834[obj->mem][0];
      setPolyF4(pPoly);
      setXYWH(pPoly, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
      setSemiTrans(pPoly, 1);
      b = OBJ.fade;
      setRGB0(pPoly, b, b, b);
      AddPrim(&gGraphicsPtr->ot[otOfs], pPoly);

      pDrawMode = &s_drawModes_80124b34[obj->mem][1];
      SetDrawMode(pDrawMode, 0, 0, GetTPage(0, obj->state3, 0, 0), NULL);
      AddPrim(&gGraphicsPtr->ot[otOfs], pDrawMode);
      break;
   }
}

/* Returns theta2 wrapped +/-360deg so easing from theta1 turns the SHORT way round --
 * used by every map camera that eases its yaw toward a target angle. */
s32 ShortestTurnAngle(s32 theta1, s16 theta2) {
   s16 diff;
   s16 ret;

   diff = theta2 - (theta1 & 0xfff);
   ret = theta2;
   if (diff > DEG(0)) {
      if (diff > DEG(180)) {
         ret = theta2 - DEG(360);
      } else if (diff < DEG(-180)) {
         ret = theta2 + DEG(360);
      }
   }
   return ret;
}

#undef OBJF
#define OBJF 387
void Objf387_FullscreenImage(Object *obj) {
   extern DR_MODE s_drawModes1_80124e34[2];
   extern DR_MODE s_drawModes2_80124e4c[2];
   extern SPRT s_sprites_80124e64[2][2];

   s32 i;
   Object *p;
   SPRT *pSprt;
   DR_MODE *pDrawMode;

   switch (obj->state) {
   case 0:
      for (i = 0; i < OBJ_DATA_CT; i++) {
         p = &gObjectArray[i];
         if (p->functionIndex == OBJF_FULLSCREEN_IMAGE && p != obj) {
            obj->functionIndex = OBJF_NULL;
            return;
         }
      }
      for (i = 0; i < 2; i++) {
         pDrawMode = &s_drawModes1_80124e34[i];
         SetDrawMode(pDrawMode, 0, 0, GetTPage(1, 0, 320, 256), NULL);
         pDrawMode = &s_drawModes2_80124e4c[i];
         SetDrawMode(pDrawMode, 0, 0, GetTPage(1, 0, 448, 256), NULL);

         pSprt = &s_sprites_80124e64[i][0];
         SetSprt(&pSprt[0]);
         setRGB0(&pSprt[0], 128, 128, 128);
         setXY0(&pSprt[0], 0, 0);
         setUV0(&pSprt[0], 0, 0);
         setClut(&pSprt[0], 768, 248);
         setWH(&pSprt[0], 256, 240);

         SetSprt(&pSprt[1]);
         setRGB0(&pSprt[1], 128, 128, 128);
         setXY0(&pSprt[1], 256, 0);
         setUV0(&pSprt[1], 0, 0);
         setClut(&pSprt[1], 768, 248);
         setWH(&pSprt[1], 64, 240);
      }
      obj->state++;

   // fallthrough
   case 1:
      obj->mem++;
      obj->mem %= 2;
      pSprt = &s_sprites_80124e64[obj->mem][0];
      AddPrim(&gGraphicsPtr->ot[2], &pSprt[1]);
      AddPrim(&gGraphicsPtr->ot[2], &s_drawModes2_80124e4c[obj->mem]);
      AddPrim(&gGraphicsPtr->ot[2], &pSprt[0]);
      AddPrim(&gGraphicsPtr->ot[2], &s_drawModes1_80124e34[obj->mem]);
      break;
   }
}

void LoadFullscreenImage(s32 cdf) {
   RECT timRect;
   RECT clutRect;
   TIM_IMAGE tim;

   LoadCdFile(cdf, 0);
   OpenTIM(&gScratch3_80180210[0]);

   if (ReadTIM(&tim) != 0) {
      setRECT(&timRect, 320, 256, 160, 240);
      setRECT(&clutRect, 768, 248, 256, 1);
      LoadImage(&timRect, tim.paddr);
      LoadImage(&clutRect, tim.caddr);
   }
}
