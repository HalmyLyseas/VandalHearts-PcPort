#include "common.h"
#include "object.h"
#include "graphics.h"

// Identifying these as MapObjects since they're in the MapObject arrays.
// (See: Predefined MapObject arrays @800f6378; Objf043_SetupMapObjects @800515c8)

#undef OBJF
#define OBJF 564
void Objf564_565_566_MapObject_Water(Object *obj) {
   extern u32 s_distortedTex_80123490[32][4];
   extern u32 s_originalTex_80123690[32][4];
   RECT rect;
   s16 i;
   s16 shift;
   s16 phase;

   if (obj->functionIndex == OBJF_MAP_OBJECT_WATER_1) {
      rect.x = 328;
      rect.y = 48;
      rect.w = 8;
      rect.h = 32;

      switch (obj->state) {
      case 0:
         StoreImage(&rect, s_originalTex_80123690);
         DrawSync(0);
         obj->state++;
         break;
      case 1:
         for (i = 0; i < 128; i++) {
            ((u32 *)s_distortedTex_80123490)[i] = ((u32 *)s_originalTex_80123690)[i];
         }

         OBJ.phase += 130;
         phase = OBJ.phase;

         for (i = 0; i < 32; i++) {
            shift = ((rcos(phase & 0xfff) * 2 >> 12) + 4) * 4;

            s_distortedTex_80123490[i][0] = (s_distortedTex_80123490[i][0] >> shift) +
                                            (s_distortedTex_80123490[i][1] << (32 - shift));
            s_distortedTex_80123490[i][1] = (s_distortedTex_80123490[i][1] >> shift) +
                                            (s_distortedTex_80123490[i][2] << (32 - shift));
            s_distortedTex_80123490[i][2] = (s_distortedTex_80123490[i][2] >> shift) +
                                            (s_distortedTex_80123490[i][3] << (32 - shift));
            s_distortedTex_80123490[i][3] = (s_distortedTex_80123490[i][3] >> shift) +
                                            (s_distortedTex_80123490[i][0] << (32 - shift));
            phase += 0x100;
         }

         rect.x = 328;
         rect.y = 48;
         LoadImage(&rect, s_distortedTex_80123490);
         break;
      }

      gGfxSubTextures[GFX_MAP_TEXTURE_51][1]--;
      if (gGfxSubTextures[GFX_MAP_TEXTURE_51][1] <= 48) {
         gGfxSubTextures[GFX_MAP_TEXTURE_51][1] = 64;
      }
      gGfxSubTextures[GFX_MAP_TEXTURE_52][1] = gGfxSubTextures[GFX_MAP_TEXTURE_51][1];

   } else if (obj->functionIndex == OBJF_MAP_OBJECT_WATER_2) {
      rect.x = 332;
      rect.y = 0;
      rect.w = 8;
      rect.h = 32;

      switch (obj->state) {
      case 0:
         StoreImage(&rect, s_originalTex_80123690);
         DrawSync(0);
         obj->state++;
         break;
      case 1:
         for (i = 0; i < 128; i++) {
            ((u32 *)s_distortedTex_80123490)[i] = ((u32 *)s_originalTex_80123690)[i];
         }

         OBJ.phase += 130;
         phase = OBJ.phase;

         for (i = 0; i < 32; i++) {
            shift = ((rcos(phase & 0xfff) * 2 >> 12) + 4) * 4;

            s_distortedTex_80123490[i][0] = (s_distortedTex_80123490[i][0] >> shift) +
                                            (s_distortedTex_80123490[i][1] << (32 - shift));
            s_distortedTex_80123490[i][1] = (s_distortedTex_80123490[i][1] >> shift) +
                                            (s_distortedTex_80123490[i][2] << (32 - shift));
            s_distortedTex_80123490[i][2] = (s_distortedTex_80123490[i][2] >> shift) +
                                            (s_distortedTex_80123490[i][3] << (32 - shift));
            s_distortedTex_80123490[i][3] = (s_distortedTex_80123490[i][3] >> shift) +
                                            (s_distortedTex_80123490[i][0] << (32 - shift));
            phase += 0x100;
         }

         rect.x = 332;
         rect.y = 0;
         LoadImage(&rect, s_distortedTex_80123490);
         break;
      }
   } else {
      // OBJF_MAP_OBJECT_LAVA_1:
      rect.x = 324;
      rect.y = 0;
      rect.w = 8;
      rect.h = 32;

      switch (obj->state) {
      case 0:
         StoreImage(&rect, s_originalTex_80123690);
         DrawSync(0);
         obj->state++;
         break;
      case 1:
         for (i = 0; i < 128; i++) {
            ((u32 *)s_distortedTex_80123490)[i] = ((u32 *)s_originalTex_80123690)[i];
         }

         OBJ.phase += 130;
         phase = OBJ.phase;

         for (i = 0; i < 32; i++) {
            shift = ((rcos(phase & 0xfff) * 2 >> 12) + 4) * 4;

            s_distortedTex_80123490[i][0] = (s_distortedTex_80123490[i][0] >> shift) +
                                            (s_distortedTex_80123490[i][1] << (32 - shift));
            s_distortedTex_80123490[i][1] = (s_distortedTex_80123490[i][1] >> shift) +
                                            (s_distortedTex_80123490[i][2] << (32 - shift));
            s_distortedTex_80123490[i][2] = (s_distortedTex_80123490[i][2] >> shift) +
                                            (s_distortedTex_80123490[i][3] << (32 - shift));
            s_distortedTex_80123490[i][3] = (s_distortedTex_80123490[i][3] >> shift) +
                                            (s_distortedTex_80123490[i][0] << (32 - shift));
            phase += 0x100;
         }

         rect.x = 324;
         rect.y = 0;
         LoadImage(&rect, s_distortedTex_80123490);
         break;
      }
   }
}

void Objf449_MapObject_FlowingWater(Object *obj) {
   gGfxSubTextures[GFX_MAP_TEXTURE_5][1]++;
   if (gGfxSubTextures[GFX_MAP_TEXTURE_5][1] >= 16) {
      gGfxSubTextures[GFX_MAP_TEXTURE_5][1] = 0;
   }
}

void Objf568_MapObject_Rail(Object *obj) {
   // Simulated train movement
   s32 i;

   gLightRotation.vy = GetLightRotY() + 0xc0;

   gGfxSubTextures[GFX_MAP_TEXTURE_7][1] += 8;
   if (gGfxSubTextures[GFX_MAP_TEXTURE_7][1] >= 32) {
      gGfxSubTextures[GFX_MAP_TEXTURE_7][1] = 0;
   }

   gGfxSubTextures[GFX_MAP_TEXTURE_23][1] += 8;
   if (gGfxSubTextures[GFX_MAP_TEXTURE_23][1] >= 48) {
      gGfxSubTextures[GFX_MAP_TEXTURE_23][1] = 16;
   }

   for (i = 8; i < 19; i++) {
      gGfxSubTextures[GFX_MAP_TEXTURES_OFS + i][1] = gGfxSubTextures[GFX_MAP_TEXTURE_7][1];
   }
   for (i = 24; i < 35; i++) {
      gGfxSubTextures[GFX_MAP_TEXTURES_OFS + i][1] = gGfxSubTextures[GFX_MAP_TEXTURE_23][1];
   }

   switch (obj->mem) {
   case 0:
      gGfxSubTextures[GFX_MAP_TEXTURE_90][0] = 224;
      obj->mem++;
      break;
   case 1:
      gGfxSubTextures[GFX_MAP_TEXTURE_90][0] = 240;
      obj->mem = 0;
      break;
   }
}

void Objf411_MapObject_VileBog(Object *obj) {
   switch (obj->state) {
   case 0:
   case 4:
   case 8:
   case 12:
      gGfxSubTextures[GFX_MAP_TEXTURE_21][0] = obj->state2 * 16 + 80;
      obj->state2++;
      break;
   case 15:
      obj->state = 0;
      obj->state2 = 0;
      return;
   }
   obj->state++;
}

void Objf569_572_MapObject_Lava(Object *obj) {
   switch (obj->state) {
   case 0:
   case 3:
   case 6:
   case 9:
   case 12:
   case 15:
   case 18:
   case 21:
      if (obj->functionIndex == OBJF_MAP_OBJECT_LAVA_2) {
         gGfxSubTextures[GFX_MAP_TEXTURE_198][0] = obj->state2 * 16 + 48;
      } else {
         gGfxSubTextures[GFX_MAP_TEXTURE_4][0] = obj->state2 * 16 + 16;
         // shadowed area
         gGfxSubTextures[GFX_MAP_TEXTURE_128][0] = obj->state2 * 16 + 112;
      }
      obj->state2++;
      break;
   case 23:
      obj->state = 0;
      obj->state2 = 0;
      return;
   }
   obj->state++;
}

void Objf042_MapObject_FlowingSand(Object *obj) {
   gGfxSubTextures[GFX_MAP_TEXTURE_1][1]--;
   if (gGfxSubTextures[GFX_MAP_TEXTURE_1][1] == 0) {
      gGfxSubTextures[GFX_MAP_TEXTURE_1][1] = 16;
   }
}
