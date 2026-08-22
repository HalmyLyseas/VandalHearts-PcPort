#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "common.h"
#include "PsyQ/libgte.h"
#include "PsyQ/libgpu.h"

#define OT_SIZE 1024
#define GFX_CT 870
#define VRAM_WIDTH 1024
#define VRAM_HEIGHT 512

#define POLY_RGB_CODE(code, r, g, b)                                                               \
   ((((code)&0xff) << 24) + (((b)&0xff) << 16) + (((g)&0xff) << 8) + ((r)&0xff))

typedef enum GfxIdx {
   GFX_NULL = 0,
   GFX_TRANSPARENT = 1,
   GFX_ENEMY_CIRCLE = 2,
   GFX_PLAYER_CIRCLE = 3,
   GFX_GRAVE_MARKER = 4,
   GFX_TORCH_1 = 5,
   GFX_TORCH_2 = 6,
   GFX_TORCH_3 = 7,
   GFX_TORCH_4 = 8,
   GFX_YOU_1 = 9,
   GFX_YOU_2 = 10,
   GFX_WIN_1 = 11,
   GFX_WIN_2 = 12,
   GFX_LOSE_1 = 13,
   GFX_LOSE_2 = 14,
   GFX_TBD_15 = 15,
   GFX_PORTRAIT_A_FACE = 17,
   GFX_PORTRAIT_A_SPEAK_1 = 18,
   GFX_PORTRAIT_A_SPEAK_2 = 19,
   GFX_PORTRAIT_A_BLINK = 20,
   GFX_PORTRAIT_B_FACE = 21,
   GFX_PORTRAIT_B_SPEAK_1 = 22,
   GFX_PORTRAIT_B_SPEAK_2 = 23,
   GFX_PORTRAIT_B_BLINK = 24,
   GFX_MASK_EFFECT_1 = 25,
   GFX_MASK_EFFECT_2 = 26,
   GFX_MASK_EFFECT_3 = 27,
   GFX_MASK_EFFECT_4 = 28,
   GFX_DIGIT_0 = 30,
   GFX_DIGIT_1 = 31,
   GFX_DIGIT_2 = 32,
   GFX_DIGIT_3 = 33,
   GFX_DIGIT_4 = 34,
   GFX_DIGIT_5 = 35,
   GFX_DIGIT_6 = 36,
   GFX_DIGIT_7 = 37,
   GFX_DIGIT_8 = 38,
   GFX_DIGIT_9 = 39,
   GFX_PARALYZED = 40,
   GFX_MISS = 41,
   GFX_TBD_42 = 42,
   GFX_MAP_CURSOR_1 = 46,
   GFX_MAP_CURSOR_2 = 47,
   GFX_MAP_CURSOR_3 = 48,
   GFX_MAP_CURSOR_4 = 49,
   GFX_MAP_CURSOR_5 = 50,
   GFX_MAP_CURSOR_6 = 51,
   GFX_DIRECTION_ARROW_FILLED = 52,
   GFX_DIRECTION_ARROW_EMPTY = 53,
   GFX_BLUE_GEM = 54,
   GFX_RED_GEM = 55,
   GFX_YELLOW_GEM = 56,
   GFX_GRAY_GEM = 57,
   GFX_COMPASS = 58,
   GFX_E = 59,
   GFX_W = 60,
   GFX_S = 61,
   GFX_N = 62,
   GFX_BUBBLE = 63,
   GFX_RED_X = 64,
   GFX_SELECTION_ARROW = 65,
   GFX_IMPACT_1 = 67,
   GFX_IMPACT_2 = 68,
   GFX_IMPACT_3 = 69,
   GFX_IMPACT_4 = 70,
   GFX_IMPACT_5 = 71,
   GFX_IMPACT_6 = 72,
   GFX_IMPACT_7 = 73,
   GFX_COLOR_1 = 74,
   GFX_COLOR_2 = 75,
   GFX_COLOR_3 = 76,
   GFX_COLOR_4 = 77,
   GFX_COLOR_5 = 78,
   GFX_COLOR_6 = 79,
   GFX_COLOR_7 = 80,
   GFX_COLOR_8 = 81,
   GFX_COLOR_9 = 82,
   GFX_COLOR_10 = 83,
   GFX_COLOR_11 = 84,
   GFX_COLOR_12 = 85,
   GFX_COLOR_13 = 86,
   GFX_COLOR_14 = 87,
   GFX_COLOR_15 = 88,
   GFX_EXPLOSION_1 = 89,
   GFX_EXPLOSION_2 = 90,
   GFX_EXPLOSION_3 = 91,
   GFX_EXPLOSION_4 = 92,
   GFX_EXPLOSION_5 = 93,
   GFX_EXPLOSION_6 = 94,
   GFX_EXPLOSION_7 = 95,
   GFX_EXPLOSION_8 = 96,
   GFX_EXPLOSION_9 = 97,
   GFX_EXPLOSION_10 = 98,
   GFX_EXPLOSION_11 = 99,
   GFX_SPARKLE_1 = 100,
   GFX_SPARKLE_2 = 101,
   GFX_SPARKLE_3 = 102,
   GFX_SPARKLE_4 = 103,
   GFX_SPARKLE_5 = 104,
   GFX_FLAME_1 = 105,
   GFX_FLAME_2 = 106,
   GFX_FLAME_3 = 107,
   GFX_FLAME_4 = 108,
   GFX_FLAME_5 = 109,
   GFX_FLAME_6 = 110,
   GFX_FLAME_7 = 111,
   GFX_FLAME_8 = 112,
   GFX_PUFF_1 = 113,
   GFX_PUFF_2 = 114,
   GFX_PUFF_3 = 115,
   GFX_PUFF_4 = 116,
   GFX_PUFF_5 = 117,
   GFX_PUFF_6 = 118,
   GFX_PUFF_7 = 119,
   GFX_PUFF_8 = 120,
   GFX_PUFF_9 = 121,
   GFX_PUFF_10 = 122,
   GFX_DOTS = 123,
   GFX_BOULDER_1 = 124,
   GFX_BOULDER_2 = 125,
   GFX_ROCK_1 = 126,
   GFX_ROCK_2 = 127,
   GFX_ROCK_3 = 128,
   GFX_ROCK_4 = 129,
   GFX_SWIRLY_RING = 131,
   GFX_SWIRLY_RING_TOP = 132,
   GFX_SWIRLY_RING_BTM = 133,
   GFX_RING = 134,
   GFX_RING_TOP = 135,
   GFX_RING_BTM = 136,
   GFX_RUNE_1 = 137,
   GFX_RUNE_2 = 138,
   GFX_RUNE_3 = 139,
   GFX_RUNE_4 = 140,
   GFX_RUNE_5 = 141,
   GFX_RUNE_6 = 142,
   GFX_RUNE_7 = 143,
   GFX_RUNE_8 = 144,
   GFX_RUNE_9 = 145,
   GFX_RUNE_10 = 146,
   GFX_RUNE_11 = 147,
   GFX_RUNE_12 = 148,
   GFX_LIGHTNING_1 = 149,
   GFX_LIGHTNING_2 = 150,
   GFX_LIGHTNING_3 = 151,
   GFX_LIGHTNING_4 = 152,
   GFX_LIGHTNING_5 = 153,
   GFX_LIGHTNING_6 = 154,
   GFX_LIGHTNING_7 = 155,
   GFX_LIGHTNING_8 = 156,
   //? I'm thinking the extra copies are allotted for dynamic scaling/scrolling texture effects
   GFX_TILED_CLOUDS = 157,
   GFX_TILED_TBD_162 = 162,
   GFX_TILED_DYN_163 = 163,
   GFX_TILED_DYN_164 = 164,
   GFX_TILED_DYN_165 = 165,
   GFX_TILED_DYN_166 = 166,
   GFX_TILED_VEINS = 167,
   GFX_TILED_LINES = 172,
   GFX_TILED_TBD_177 = 177,
   GFX_TILED_FLAMES = 182,
   GFX_TILED_FLAMES_DYN_1 = 183,
   GFX_TILED_ICE = 187,
   GFX_TILED_ICE_DYN_1 = 188,
   GFX_TILED_STONE = 192,
   GFX_TILED_DIAMONDS = 197,
   GFX_TILED_DIAMONDS_DYN_1 = 198,
   GFX_TILED_MAGMA = 202,
   GFX_TILED_RED_SPARKLES = 207,
   GFX_TILED_RED_SPARKLES_DYN_1 = 208,
   GFX_TILED_GRAY_SPARKLES = 210,
   GFX_TILED_GRAY_SPARKLES_DYN_1 = 211,
   GFX_ARROW = 212,
   GFX_SPIKED_BALL_1 = 217,
   GFX_SPIKED_BALL_2 = 218,
   GFX_FOUNTAIN_1 = 219,
   GFX_FOUNTAIN_2 = 220,
   GFX_FLAG_1 = 221,
   GFX_FLAG_2 = 222,
   GFX_FLAG_3 = 223,
   GFX_FLAG_4 = 224,
   GFX_LAMP_POST = 225,
   GFX_TREE = 226,
   GFX_MAP_MARKER = 227,
   GFX_LIT_SPHERE_1 = 228,
   GFX_LIT_SPHERE_2 = 229,
   GFX_LIT_SPHERE_3 = 230,
   GFX_LIT_SPHERE_4 = 231,
   GFX_LIT_SPHERE_5 = 232,
   GFX_LIT_SPHERE_6 = 233,
   GFX_LIT_SPHERE_7 = 234,
   GFX_LIT_SPHERE_8 = 235,
   GFX_GLOBE_1 = 236,
   GFX_GLOBE_2 = 237,
   GFX_GLOBE_3 = 238,
   GFX_GLOBE_4 = 239,
   GFX_GLOBE_5 = 240,
   GFX_GLOBE_6 = 241,
   GFX_GLOBE_7 = 242,
   GFX_GLOBE_8 = 243,
   GFX_LARGE_RED_CIRCLE = 244,
   GFX_JP_TBD_245,
   GFX_CREST = 247,
   GFX_SKULL = 252,
   GFX_SALAMANDER_N = 253,
   GFX_SALAMANDER_NE = 254,
   GFX_SALAMANDER_E = 255,
   GFX_SALAMANDER_SE = 256,
   GFX_SALAMANDER_S = 257,
   GFX_RED_X_BTM_RIGHT = 262,
   GFX_DAGGER = 263,
   GFX_CRATE = 264,
   GFX_SWORD = 265,
   GFX_SHIELD = 266,
   GFX_BLUE_CRYSTAL = 267,
   GFX_BLUE_SHARD = 268,
   GFX_SUPPORT = 269,
   GFX_FLAME_WALL_1 = 270,
   GFX_FLAME_WALL_2 = 271,
   GFX_FLAME_WALL_3 = 272,
   GFX_FLAME_WALL_4 = 273,
   GFX_FLAME_WALL_5 = 274,
   GFX_FLAME_WALL_6 = 275,
   GFX_FLAME_WALL_7 = 276,
   GFX_FLAME_WALL_8 = 277,
   GFX_TBD_278 = 278,
   GFX_TBD_279 = 279,
   GFX_TBD_280 = 280,
   GFX_TBD_281 = 281,
   GFX_TBD_282 = 282,
   GFX_TBD_283 = 283,
   GFX_TBD_284 = 284,
   GFX_TBD_285 = 285,
   GFX_POISON_1 = 286,
   GFX_POISON_2 = 287,
   GFX_POISON_3 = 288,
   GFX_POISON_4 = 289,
   GFX_POISON_5 = 290,
   GFX_POISON_6 = 291,
   GFX_POISON_7 = 292,
   GFX_POISON_8 = 293,
   GFX_SPLASH_1 = 294,
   GFX_SPLASH_2 = 295,
   GFX_SPLASH_3 = 296,
   GFX_SPLASH_4 = 297,
   GFX_SPLASH_5 = 298,
   GFX_SPLASH_6 = 299,
   GFX_SPLASH_7 = 300,
   GFX_SPLASH_8 = 301,
   GFX_TREE_SNOWY = 302,
   GFX_TREE_EVERGREEN = 303,
   GFX_TREE_PALM = 304,
   GFX_FAERIE_1 = 305,
   GFX_FAERIE_2 = 306,
   GFX_PANORAMA_DEEP_BLUE = 311,
   GFX_TREE_BARE = 312,
   GFX_MSGBOX_TAIL_DOWN = 313,
   GFX_MSGBOX_TAIL_RIGHT = 314,
   GFX_MSGBOX_TAIL_LEFT = 315,
   GFX_MSGBOX_BUTTON_1 = 316,
   GFX_MSGBOX_BUTTON_2 = 317,
   GFX_MSGBOX_BUTTON_3 = 318,
   GFX_MSGBOX_BUTTON_4 = 319,
   GFX_CHEST_1 = 320,
   GFX_CHEST_2 = 321,
   GFX_CHEST_3 = 322,
   GFX_CHEST_4 = 323,
   GFX_PANORAMA_2 = 324,
   GFX_PANORAMA_3 = 325,
   GFX_PANORAMA_4 = 326,
   GFX_PANORAMA_RED = 327,
   GFX_PANORAMA_6 = 328,
   GFX_PANORAMA_7 = 329,
   GFX_PANORAMA_8 = 330,
   GFX_CAMPFIRE_1 = 331,
   GFX_CAMPFIRE_2 = 332,
   GFX_CAMPFIRE_3 = 333,
   GFX_CAMPFIRE_4 = 334,
   GFX_BOMB = 335,
   GFX_NOW_LOADING = 336,
   GFX_VANDAL_HEARTS = 337,
   GFX_LV = 338,
   GFX_UP = 339,
   GFX_POISONED = 340,
   GFX_AT = 341,
   GFX_DF = 342,
   GFX_MV = 343,
   GFX_SCROLL_INDICATOR = 344,
   GFX_COLORS = 346,
   GFX_STRIP_SPRITES_OFS = 347,
   GFX_MAP_TEXTURES_OFS = 400,
   GFX_MAP_TEXTURE_1 = 401,
   GFX_MAP_TEXTURE_4 = 404,
   GFX_MAP_TEXTURE_5 = 405,
   GFX_MAP_TEXTURE_6 = 406,
   GFX_MAP_TEXTURE_7 = 407,
   GFX_MAP_TEXTURE_8 = 408,
   GFX_MAP_TEXTURE_12 = 412,
   GFX_MAP_TEXTURE_14 = 414,
   GFX_MAP_TEXTURE_18 = 418,
   GFX_MAP_TEXTURE_19 = 419,
   GFX_MAP_TEXTURE_21 = 421,
   GFX_MAP_TEXTURE_23 = 423,
   GFX_MAP_TEXTURE_25 = 425,
   GFX_MAP_TEXTURE_28 = 428,
   GFX_MAP_TEXTURE_34 = 434,
   GFX_MAP_TEXTURE_51 = 451,
   GFX_MAP_TEXTURE_52 = 452,
   GFX_MAP_TEXTURE_54 = 454,
   GFX_MAP_TEXTURE_55 = 455,
   GFX_MAP_TEXTURE_56 = 456,
   GFX_MAP_TEXTURE_62 = 462,
   GFX_MAP_TEXTURE_74 = 474,
   GFX_MAP_TEXTURE_75 = 475,
   GFX_MAP_TEXTURE_76 = 476,
   GFX_MAP_TEXTURE_84 = 484,
   GFX_MAP_TEXTURE_85 = 485,
   GFX_MAP_TEXTURE_86 = 486,
   GFX_MAP_TEXTURE_87 = 487,
   GFX_MAP_TEXTURE_88 = 488,
   GFX_MAP_TEXTURE_89 = 489,
   GFX_MAP_TEXTURE_90 = 490,
   GFX_MAP_TEXTURE_128 = 528,
   GFX_MAP_TEXTURE_153 = 553,
   GFX_MAP_TEXTURE_166 = 566,
   GFX_MAP_TEXTURE_167 = 567,
   GFX_MAP_TEXTURE_184 = 584,
   GFX_MAP_TEXTURE_188 = 588,
   GFX_MAP_TEXTURE_192 = 592,
   GFX_MAP_TEXTURE_198 = 598,
   GFX_MAP_TEXTURE_206 = 606,
   GFX_MAP_TEXTURE_207 = 607,
   GFX_MAP_TEXTURE_212 = 612,
   GFX_MAP_TEXTURE_213 = 613,
   GFX_MAP_TEXTURE_229 = 629,
   GFX_MAP_TEXTURE_232 = 632,
   GFX_MAP_TEXTURE_233 = 633,
   GFX_WINDOW_TBD_657 = 657,
   GFX_WINDOW_TBD_658 = 658,
   GFX_UNIT_PREVIEW = 660,
   GFX_TBD_685 = 685,
   GFX_WINDOW_TBD_707 = 707,
   GFX_WINDOW_TBD_708 = 708,
   GFX_ITEM_ICONS_OFS = 727,
   GFX_ITEM_MEGAHERB = 729,
} GfxIdx;

typedef enum ClutIdx {
   CLUT_NULL = 0,
   CLUT_2 = 2,
   CLUT_REDS = 3,
   CLUT_BLUES = 4,
   CLUT_GRAYS = 5,
   CLUT_6 = 6,
   CLUT_BLACK = 7,
   CLUT_PURPLES = 8,
   CLUT_GREENS = 9,
   CLUT_MASK = 10,
   CLUT_12 = 12,
   CLUT_14 = 14,
   CLUT_WINDOW = 16,
   CLUT_INACTIVE_WINDOW = 19,
   CLUT_25 = 25,
   CLUT_58 = 58,
} ClutIdx;

typedef enum AnimIdx {
   ANIM_IDLE_B = 0,
   ANIM_IDLE_F = 1,
   ANIM_STRUCK_B = 2,
   ANIM_STRUCK_F = 3,
   ANIM_BLOCKING_B = 4,
   ANIM_BLOCKING_F = 5,
   ANIM_WALKING_B = 6,
   ANIM_WALKING_F = 7,
   ANIM_JUMPING_B = 8,
   ANIM_JUMPING_F = 9,
   ANIM_CROUCHING_B = 10,
   ANIM_CROUCHING_F = 11,
   ANIM_ATTACKING_B = 12,
   ANIM_ATTACKING_F = 13,
   ANIM_ATTACKING_END_B = 14,
   ANIM_ATTACKING_END_F = 15,
   ANIM_CASTING_B = 16,
   ANIM_CASTING_F = 17,
   ANIM_STRUCK_END_B = 18,
   ANIM_STRUCK_END_F = 19,
   ANIM_BLOCKING_END_B = 20,
   ANIM_BLOCKING_END_F = 21,
   ANIM_HOP_B = 22,
   ANIM_HOP_F = 23,
   ANIM_HIGH_STEP_B = 24,
   ANIM_HIGH_STEP_F = 25,
   ANIM_ATTACKING_FAR_B = 26,
   ANIM_ATTACKING_FAR_F = 27,
   ANIM_ATTACKING_FAR_END_B = 28,
   ANIM_ATTACKING_FAR_END_F = 29
} AnimIdx;

/*typedef enum SpriteBoxIdx {
   SPRITE_BOX_0 = 0,
   SPRITE_BOX_1 = 1,
   SPRITE_BOX_2 = 2,
   SPRITE_BOX_3 = 3,
   SPRITE_BOX_4 = 4,
   SPRITE_BOX_5 = 5,
   SPRITE_BOX_6 = 6,
   SPRITE_BOX_7 = 7,
   SPRITE_BOX_8 = 8,
   SPRITE_BOX_9 = 9,
   SPRITE_BOX_10 = 10,
   SPRITE_BOX_11 = 11,
   SPRITE_BOX_12 = 12,
   SPRITE_BOX_13 = 13,
   SPRITE_BOX_14 = 14,
   SPRITE_BOX_15 = 15,
   SPRITE_BOX_16 = 16,
   SPRITE_BOX_17 = 17,
   SPRITE_BOX_18 = 18
} SpriteBoxIdx;*/

typedef enum GpuCode {
   GPU_CODE_SEMI_TRANS = 0x02,
   GPU_CODE_POLY_FT4 = 0x2c,
} GpuCode;

typedef struct Graphics {
   DRAWENV drawEnv;
   DISPENV dispEnv;
   POLY_FT4 quads[1300];
   u32 ot[OT_SIZE];
} Graphics;

typedef SVECTOR Quad[4];

// Usages of this type might need to be replaced w/ u8[4] to match. Union didn't seem to help.
typedef struct TextureWindow {
   u8 x, y, w, h;
} TextureWindow;

/* Partial; defining as needed */
// neutral,injured,happy,angry,upset
typedef enum PortraitId {
   PORTRAIT_NULL = 0,
   PORTRAIT_ASH = 1,
   PORTRAIT_ASH_ANGRY = 4,
   PORTRAIT_ASH_UPSET = 5,
   PORTRAIT_ASH_6 = 6,
   PORTRAIT_CLINT = 8,
   PORTRAIT_CLINT_UPSET = 12,
   PORTRAIT_DIEGO = 15,
   PORTRAIT_DIEGO_HAPPY = 17,
   PORTRAIT_DIEGO_ANGRY = 18,
   PORTRAIT_DIEGO_UPSET = 19,
   PORTRAIT_ELENI = 22,
   PORTRAIT_ELENI_HAPPY = 24,
   PORTRAIT_ELENI_25 = 25,
   PORTRAIT_ELENI_28 = 28,
   PORTRAIT_HUXLEY = 29,
   PORTRAIT_KIRA = 36,
   PORTRAIT_KIRA_ANGRY = 39,
   PORTRAIT_GROG = 43,
   PORTRAIT_DOLAN = 50,
   PORTRAIT_AMON = 57,
   PORTRAIT_AMON_HAPPY = 59,
   PORTRAIT_AMON_ANGRY = 60,
   PORTRAIT_AMON_62 = 62,
   PORTRAIT_SARA = 64,
   PORTRAIT_SARA_HAPPY = 66,
   PORTRAIT_SARA_ANGRY = 67,
   PORTRAIT_SARA_69 = 69,
   PORTRAIT_ZOHAR = 71,
   PORTRAIT_ZOHAR_HAPPY = 73,
   PORTRAIT_ZOHAR_ANGRY = 74,
   PORTRAIT_DARIUS = 78,
   PORTRAIT_DARIUS_ANGRY = 81,
   PORTRAIT_ASH_CHAMPION = 85,
   PORTRAIT_CLINT_SWORDSMAN = 92,
   PORTRAIT_DIEGO_BOWMAN = 99,
   PORTRAIT_ELENI_SORCERER = 106,
   PORTRAIT_HUXLEY_BISHOP = 113,
   PORTRAIT_KIRA_BOWMAN = 120,
   PORTRAIT_GROG_SWORDSMAN = 127,
   PORTRAIT_DOLAN_SWORDSMAN = 134,
   PORTRAIT_AMON_BOWMAN = 141,
   PORTRAIT_SARA_BISHOP = 148,
   PORTRAIT_ZOHAR_SORCERER = 155,
   PORTRAIT_DARIUS_BOWMAN = 162,
   PORTRAIT_ASH_169 = 169,
   PORTRAIT_CLINT_GUARDSMAN = 176,
   PORTRAIT_DIEGO_HAWKNIGHT = 183,
   PORTRAIT_ELENI_MONK = 190,
   PORTRAIT_HUXLEY_MONK = 197,
   PORTRAIT_KIRA_HAWKNIGHT = 204,
   PORTRAIT_GROG_GUARDSMAN = 211,
   PORTRAIT_DOLAN_GUARDSMAN = 218,
   PORTRAIT_AMON_HAWKNIGHT = 225,
   PORTRAIT_SARA_MONK = 232,
   PORTRAIT_ZOHAR_MONK = 239,
   PORTRAIT_DARIUS_HAWKNIGHT = 246,
   PORTRAIT_ASH_PARAGON = 253,
   PORTRAIT_CLINT_DUELIST = 260,
   PORTRAIT_DIEGO_SNIPER = 267,
   PORTRAIT_ELENI_ENCHANTER = 274,
   PORTRAIT_HUXLEY_ARCHBISHOP = 281,
   PORTRAIT_KIRA_SNIPER = 288,
   PORTRAIT_GROG_DUELIST = 295,
   PORTRAIT_DOLAN_DUELIST = 302,
   PORTRAIT_AMON_SNIPER = 309,
   PORTRAIT_SARA_ARCHBISHOP = 316,
   PORTRAIT_ZOHAR_ENCHANTER = 323,
   PORTRAIT_DARIUS_SNIPER = 330,
   PORTRAIT_ASH_VANDALIER = 337,
   PORTRAIT_CLINT_DRAGOON = 344,
   PORTRAIT_DIEGO_SKY_LORD = 351,
   PORTRAIT_ELENI_NINJA = 358,
   PORTRAIT_HUXLEY_NINJA = 365,
   PORTRAIT_KIRA_SKY_LORD = 372,
   PORTRAIT_GROG_DRAGOON = 379,
   PORTRAIT_DOLAN_DRAGOON = 386,
   PORTRAIT_AMON_SKY_LORD = 393,
   PORTRAIT_SARA_NINJA = 400,
   PORTRAIT_ZOHAR_NINJA = 407,
   PORTRAIT_DARIUS_SKY_LORD = 414,
   PORTRAIT_END_OF_PARTY = 431,
   PORTRAIT_ZOOT_449 = 449,
   PORTRAIT_C_KNIGHT = 463,
   PORTRAIT_HASSAN_INJURED = 478,
   PORTRAIT_KANE = 480,
   PORTRAIT_KANE_INJURED = 481,
   PORTRAIT_KANE_HAPPY = 482,
   PORTRAIT_KANE_ANGRY = 483,
   PORTRAIT_KANE_BLK_KNIGHT = 485,
   PORTRAIT_KANE_BLK_KNIGHT_INJURED = 486,
   PORTRAIT_MAGNUS_DARK_LORD_492 = 492,
   PORTRAIT_DOLF_495 = 495,
   PORTRAIT_SABINA = 506,
   PORTRAIT_SABINA_508 = 508,
   PORTRAIT_SABINA_509 = 509,
   PORTRAIT_KURTZ_511 = 511,
   PORTRAIT_LANDO_514 = 514,
   PORTRAIT_LANDO_INJURED = 515,
   PORTRAIT_DALLAS_516 = 516,
   PORTRAIT_DALLAS_517 = 517,
   PORTRAIT_DALLAS_518 = 518,
   PORTRAIT_ZOOT_559 = 559,
   PORTRAIT_JUGGERNAUT = 560,
   PORTRAIT_DUMAS = 561,
   PORTRAIT_DUMAS_562 = 562,
   PORTRAIT_OROSIUS = 565,
   PORTRAIT_LEENA = 571,
   PORTRAIT_LEENA_573 = 573,
   PORTRAIT_DOLF_581 = 581,
   PORTRAIT_XENO = 585,
   PORTRAIT_XENO_INJURED = 586,
   PORTRAIT_DOJO_MASTER = 589,
   PORTRAIT_590 = 590,
   PORTRAIT_591 = 591,
   PORTRAIT_592 = 592,
   PORTRAIT_593 = 593,
   PORTRAIT_594 = 594,
   PORTRAIT_595 = 595,
   PORTRAIT_596 = 596,
   PORTRAIT_597 = 597,
   PORTRAIT_598 = 598,
   PORTRAIT_599 = 599,
   PORTRAIT_600 = 600,
   PORTRAIT_601 = 601,
   PORTRAIT_602 = 602,
   PORTRAIT_603 = 603,
   PORTRAIT_604 = 604,
   PORTRAIT_605 = 605,
   PORTRAIT_606 = 606,
   PORTRAIT_607 = 607,
   PORTRAIT_608 = 608,
   PORTRAIT_609 = 609,
   PORTRAIT_610 = 610,
   PORTRAIT_611 = 611,
   PORTRAIT_612 = 612,
   PORTRAIT_613 = 613,
   PORTRAIT_614 = 614,
   PORTRAIT_615 = 615,
   PORTRAIT_616 = 616,
   PORTRAIT_617 = 617,
   PORTRAIT_618 = 618,
   PORTRAIT_619 = 619,
   PORTRAIT_620 = 620,
   PORTRAIT_621 = 621,
   PORTRAIT_622 = 622,
   PORTRAIT_623 = 623,
   PORTRAIT_624 = 624,
   PORTRAIT_625 = 625,
   PORTRAIT_626 = 626,
   PORTRAIT_627 = 627,
   PORTRAIT_628 = 628,
   PORTRAIT_629 = 629,
   PORTRAIT_630 = 630,
   PORTRAIT_631 = 631,
   PORTRAIT_632 = 632,
   PORTRAIT_633 = 633,
   PORTRAIT_634 = 634,
   PORTRAIT_635 = 635,
   PORTRAIT_636 = 636,
   PORTRAIT_637 = 637,
   PORTRAIT_638 = 638,
   PORTRAIT_639 = 639,
   PORTRAIT_SALAMANDER = 655,
   PORTRAIT_APPARITION_675 = 675,
   PORTRAIT_CLIVE_INJURED = 677,
   PORTRAIT_KANE_APPARITION = 678,
   PORTRAIT_APPARITION_681 = 681,
   PORTRAIT_682 = 682,
   PORTRAIT_KANE_CURSED = 684,
   PORTRAIT_JUGGERNAUT_INJURED = 686,
   PORTRAIT_687 = 687,
   PORTRAIT_688 = 688,
   PORTRAIT_689 = 689,
   PORTRAIT_690 = 690,
   PORTRAIT_691 = 691,
} PortraitId;

typedef union PortraitOverlayOffsets {
   struct {
      u8 blinkX, blinkY, speakX, speakY;
   } s;
   s8 bytes[4];
} PortraitOverlayOffsets;

typedef struct PortraitsDb {
   u8 cluts[700];
   s16 sceneSets[103][26];
} PortraitsDb;

typedef struct MaskEffectPreset {
   s16 srcGfxIdx;
   s16 dstGfxIdx;
   u8 width;
   u8 height;
   s16 semiTrans;
   s16 clut;
   CVECTOR color;
} MaskEffectPreset;

typedef struct Cylinder {
   SVECTOR bottom;
   SVECTOR top;
   CVECTOR color;
   s16 sideCt;
   s16 semiTrans;
   s16 gfxIdx;
   s16 bottomRadius;
   s16 topRadius;
   s16 useColor;
   s16 clut;
   s16 theta;
} Cylinder;

typedef struct Camera {
   s16 rotX;
   s16 rotY;
   s16 rotZ;
   s16 zoom;
   s16 posX;
   s16 posY;
   s16 posZ;
} Camera;

extern Graphics gGraphicBuffers[2];
extern Graphics *gGraphicsPtr;
extern SVECTOR gLightRotation;
extern CVECTOR gLightColor;
extern CVECTOR gSideLightColor[26];
extern SVECTOR gSideLightNormals[26];
extern MATRIX gCameraMatrix;
extern SVECTOR gCameraPos;
extern SVECTOR gCameraRotation;
extern VECTOR gCameraZoom;
extern s32 gGeomOffsetX, gGeomOffsetY;
extern s32 gQuadIndex;
extern s16 gOscillation;
extern u8 gGridColorOscillation;
extern s32 gDecodingSprites;
extern RECT gTempRect;

// For each idx of gCurrentUnitSet, stores the corresponding idx of gEncodedUnitSpriteData
extern s16 gUnitSetEncodedSpriteDataIdx[20];
extern void *gEncodedUnitSpriteData[20];
extern s16 gSpriteStripClutIds[25];
extern s16 gSpriteStripTPageCells[50];
extern s16 gSpriteStripTPageIds[50];
extern s16 gSpriteStripUnitIds[23];
extern s16 gSceneSpriteStripUnitIds[105][20];
extern u8 **gSpriteStripAnimSets[25];
/* [UNIT_DB_CT] (144) is the real hardware size, but SetupSprites (src/states/game_setup.c:1073) indexes
 * this with gSpriteStripUnitIds[i], and the decomp already carried a `//?: Won't this read
 * out-of-bounds of gUnitAnimSets?` comment at that line. The index ceiling is set by DATA:
 * gSceneSpriteStripUnitIds' shipped table (byte-identical in both regions) carries ids up to 488,
 * so the stray reads land well past the table on hardware too (benign there -- adjacent statics
 * read as garbage pointers the renderer never uses). The PC port's safety widening ([489], sized
 * from an exhaustive scan of that table) lives in the US tree's include/graphics.h, which is the
 * header the PC build stages for BOTH regions; the declaration below keeps the true hardware size
 * for the matching build. */
extern u8 **gUnitAnimSets[UNIT_DB_CT];
extern u8 gUnitClutIds[492];

// Mix of 48x48 and 64x48 px frames (tpages 11 & 27); wider frames allow for e.g. slash animations
extern TextureWindow gTexwSpriteSetFrames[35];
// 48x48 px frames
extern TextureWindow gTexwSpriteStripFrames[25];
// 96x48 px frames (tpages 11 & 27)
extern TextureWindow gTexwWideSpriteSetFrames[10];

// Hard-coded values start as indices into gClutIds & are replaced w/ actual CLUT IDs in SetupGfx()
extern s16 gGfxClutIds[GFX_CT];
extern s16 gGfxTPageIds[GFX_CT];
extern s16 gGfxTPageCells[GFX_CT];
// extern TextureWindow gGfxSubTextures[GFX_CT];
extern u8 gGfxSubTextures[GFX_CT][4]; // (x, y, w, h)
// extern u16 gTPageIds[4][32];
extern s16 gTPageIds[128];

// ?: Going off of the loop at {@addr 0x8005cd94}, this should
// have 128 elements, but the last few bytes ({@addr 0x8014014c})
// are used in a completely unrelated manner: {@addr 800c471c}
//
// CONFIRMED 2026-07-21, and the "?" can be resolved: the original really does write 128 entries
// and really does overrun. SetupGfx (src/states/game_setup.c:1712) runs `for i<8 { for k<16 }` = 128
// s16 writes from 0x80140054, i.e. through 0x80140153 -- straight over `s_cdSyncStatus`
// (0x8014014c, an s32) and the 4-byte gap before gPortraitOverlayOffsets (0x80140154). It is a
// genuine out-of-bounds write in the retail game, benign only by luck: src/core/audio.c always does
// `s_cdSyncStatus = CdSync(1, buf);` immediately before every read of it (core/audio.c:473 vs 475/479),
// so the clobbered value is never observed. Found by an AddressSanitizer sweep,
// which flagged it on every SetupGfx call.
extern s16 gClutIds[124];

extern PortraitOverlayOffsets gPortraitOverlayOffsetsDb[692];
extern PortraitsDb gPortraitsDb;
extern PortraitOverlayOffsets gPortraitOverlayOffsets[50];
extern u8 gPortraitClutIds[75];

extern Quad gQuad_800fe53c;
extern Quad gQuad_800fe63c;
extern Quad gQuad_800fe65c;
extern Quad *gSpriteBoxQuads[19];

void DecodeUnitSprites(void);
void StartUnitSpritesDecoder(u8);
struct Object *GetUnitSpriteAtPosition(u8, u8);
// FIXME: Discrepancy in Objf327_HealingCircle_FX2
//  void ApplyMaskEffect(s16, s16, s16, s16, s16, s16, s16, s16, s16, s16);
void RenderMaskEffect(struct Object *, MaskEffectPreset *);

void AddObjPrim_Gui(u32 *ot, struct Object *obj);
// Discrepancy in Objf680_LitDummySprite (missing last arg)
// void RenderUnitSprite(u32 *ot, struct Object *sprite, s32 useMapElevation);
void UpdateObjAnimation(struct Object *);

static inline u16 GetLightRotY() { return gLightRotation.vy; }
static inline s32 GetCamZoom() { return gCameraZoom.vz; }

static inline s16 GetCamPosX() { return gCameraPos.vx; }
static inline s16 GetCamPosY() { return gCameraPos.vy; }
static inline s16 GetCamPosZ() { return gCameraPos.vz; }

static inline s16 GetCamRotX() { return gCameraRotation.vx; }
static inline s16 GetCamRotY() { return gCameraRotation.vy; }
static inline s16 GetCamDir() { return gCameraRotation.vy >> 10; }
static inline u16 GetCamRotY_U() { return gCameraRotation.vy; }

/*static inline void SetCamPosX(s32 value) { gCameraPos.vx = value; }
static inline void SetCamPosY(s32 value) { gCameraPos.vy = value; }
static inline void SetCamPosZ(s32 value) { gCameraPos.vz = value; }
static inline void SetCamRotY(s32 value) { gCameraRotation.vy = value; }*/

static inline void OffsetCamPosX(s32 value) { gCameraPos.vx = GetCamPosX() + value; }
static inline void OffsetCamPosY(s32 value) { gCameraPos.vy = GetCamPosY() + value; }
static inline void OffsetCamPosZ(s32 value) { gCameraPos.vz = GetCamPosZ() + value; }
static inline void OffsetCamRotY(s32 value) { gCameraRotation.vy = GetCamRotY() + value; }

void AssignFromMainCamera(Camera *);
void AssignToMainCamera(Camera *);
void EaseOutCamera(Camera *, s16);

#endif
