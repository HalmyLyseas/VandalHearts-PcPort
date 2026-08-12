#ifndef OBJ_H
#define OBJ_H

#include "common.h"
#include "graphics.h"

#define OBJ_DATA_CT 350
#define OBJ_DATA_LAST_IDX 349

#define OBJ_HELPER_2(x) obj->d.objf##x
#define OBJ_HELPER_1(x) OBJ_HELPER_2(x)
#define OBJ OBJ_HELPER_1(OBJF)
#define OBJF 000

typedef enum ObjFunctionIdx {
   OBJF_NULL = 0,
   OBJF_NOOP = 1, // Used for sprites, data stores, ...
   OBJF_MENU_CHOICE = 2,
   OBJF_BATTLE_ACTIONS = 3,
   OBJF_WINDOW_TBD_004 = 4,
   OBJF_WINDOW_TBD_005 = 5,
   OBJF_LOGO = 6,
   OBJF_APPLY_POISON = 7,
   OBJF_BATTLE_PORTRAIT = 8,
   OBJF_ITEM_ICON_MGR = 9,
   OBJF_BATTLE_MGR = 13,
   OBJF_BATTLE_UNIT = 14,
   OBJF_TARGETING_ATTACK = 15,
   OBJF_CHOOSE_DONE_DIRECTION = 16,
   OBJF_ATTACK_CAMERA = 17,
   OBJF_COMPASS = 19,
   OBJF_PUSHED_BOULDER = 20,
   OBJF_UNIT_ATTACKING = 21,
   OBJF_PROJECTILE = 22,
   OBJF_RANGED_TARGET_CAMERA = 23,
   OBJF_BOUNCE_ZOOM = 24,
   OBJF_OVERHEAD_MAP_VIEW = 25,
   OBJF_FOCUS_CAMERA = 26,
   OBJF_TARGETING_SPELL = 27,
   OBJF_UNIT_CASTING = 28,
   OBJF_PROJECTILE_INDIRECT = 29, // Unused? maybe to open chest w/ ranged attacker?
   OBJF_FIELD_INFO = 30,
   OBJF_BATTLE_SPELLS_LIST = 31,
   OBJF_DISPLAY_DAMAGE = 32,
   OBJF_DISPLAY_DAMAGE_2 = 33,
   OBJF_MAP_OBJECT_TREE = 35,
   OBJF_MAP_OBJECT_GRAVE_MARKER = 36,
   OBJF_MAP_OBJECT_FOUNTAIN = 37,
   OBJF_MAP_OBJECT_LAMP_POST = 38,
   OBJF_MAP_OBJECT_FLAG = 39,
   OBJF_MAP_OBJECT_CHEST = 40,
   OBJF_MAP_OBJECT_FLOWING_SAND = 42,
   OBJF_SETUP_MAP_OBJECTS = 43,
   OBJF_BLOOD_SPURT_PARTICLE_OFFSET = 45,
   OBJF_MAP_OBJECT_CRATE = 46,
   OBJF_BATTLE_MAP_CURSOR = 47,
   OBJF_PUSH = 48,
   OBJF_BATTLE_MAP_CURSOR_CONTROL = 49,
   OBJF_UNIT_SPRITES_DECODER = 50,
   OBJF_FLOATING_DAMAGE_TEXT = 51,
   OBJF_ATTACK_INFO_MARKER = 52,
   OBJF_STRETCH_WARP_SPRITE = 62,
   OBJF_EXPANDING_RING = 77,
   OBJF_DAMAGE_FX2 = 78,
   OBJF_SLAY_FX3 = 79,
   OBJF_ROMAN_FIRE_FLAME = 81,
   OBJF_AVALANCHE_DUST_CLOUD = 84,
   OBJF_MAP13_EXPLOSION_PILLAR = 85,
   OBJF_MAP15_HULL_SPLASH = 86,
   OBJF_SPARK_PARTICLE = 88,
   OBJF_DAGGER_STORM_FX2 = 90,
   OBJF_DAGGER_STORM_DAGGER = 91,
   OBJF_MOOD_RING_RING = 95,
   OBJF_MOOD_RING_FX2 = 96,
   OBJF_MAP20_SCN30_ARROW = 98,
   OBJF_STREAK_PARTICLE = 99,
   OBJF_HEALING_FX2 = 100,
   OBJF_HEALING_SPARKLE = 101,
   OBJF_POISON_FX2 = 102,
   OBJF_POISON_BUBBLES = 103,
   OBJF_MAGIC_CHARGE_GLYPH_RING = 107,
   OBJF_HARMFUL_WAVE_RING = 109,
   OBJF_CASTING_STAT_BUFF = 110,
   OBJF_FAERIE_SPARKLE = 116,
   OBJF_FAERIE_SPARKLE_TRAIL = 117,
   OBJF_FAERIE_TARGET = 118,
   OBJF_RADIAL_FX_SPRITE = 119,
   OBJF_SLAY_UNIT = 131,
   OBJF_ENGULF_FLAME_DAMAGE = 132,
   OBJF_FLAME_RING_EMITTER = 133,
   OBJF_ENGULF_FLAME_SLAY = 134,
   OBJF_ENGULF_LIGHTNING_DAMAGE = 136,
   OBJF_LIGHTNING_RING_EMITTER = 137,
   OBJF_ENGULF_LIGHTNING_SLAY = 138,
   OBJF_ENGULF_UNUSED_139 = 139,
   OBJF_ENGULF_EXPLOSION_DAMAGE = 140,
   OBJF_EXPLOSION_RING_EMITTER = 141,
   OBJF_ENGULF_EXPLOSION_SLAY = 142,
   OBJF_HOLY_LIGHTNING_FX2 = 144,
   OBJF_HOLY_LIGHTNING_ELECTRIC_ORB = 145,
   OBJF_LIGHTNING_BOLT = 147,
   OBJF_THUNDER_STRIKE = 148,
   OBJF_FLASHING_UNIT_SPRITE = 149,
   OBJF_SPELLBIND_GLYPH = 155,
   OBJF_DELTA_MIRAGE_RAY = 157,
   OBJF_EXPLOSION_RAYS = 159,
   OBJF_ICE_STORM_TARGET = 160,
   OBJF_ICE_STORM_SPLASH = 162,
   OBJF_STONE_SHOWER_ROCK = 164,
   OBJF_STONE_SHOWER_FX2 = 165,
   OBJF_RAINBOW_SWIRL = 167,
   OBJF_RAINBOW_STROKE_RAINBOW_SWIRL = 168,
   OBJF_HOMING_RAY = 171,
   OBJF_HOLY_PRESSURE_CUBE = 172,
   OBJF_FIRE_GEM_BEAM = 173,
   OBJF_THUNDER_FLASH_RAY = 179,
   OBJF_SPIKED_BALL_UNUSED = 183,
   OBJF_AVALANCHE_ROCK = 185,
   OBJF_EXPLOSION_STRIKE_UNUSED = 186,
   OBJF_DARK_FIRE_RAY = 188,
   OBJF_MAGIC_ARROW_ARROW = 191,
   OBJF_DARK_STAR_FX2 = 193,
   OBJF_ROLLING_THUNDER_FX2 = 195,
   OBJF_ROLLING_THUNDER_ORB_PAIR = 196,
   OBJF_ROLLING_THUNDER_FX1 = 197,
   OBJF_ROLLING_THUNDER_CASTING_BOLT = 198,
   OBJF_UNIT_STRUCK = 201,
   OBJF_UNIT_BLOCKING = 202,
   OBJF_BLOCKING_IMPACT_PARTICLE = 203,
   OBJF_SUMMON_CREST = 204,
   OBJF_BLOOD_SPURT = 205,
   OBJF_BLOOD_SPURT_PARTICLE = 206,
   OBJF_AVALANCHE_BOULDER = 211,
   OBJF_HOLY_LIGHTNING_CASTING_BOLT = 212,
   OBJF_DUST_CLOUD_SPAWNER = 213,
   OBJF_DUST_CLOUD = 214,
   OBJF_CLOUD = 215,
   OBJF_EXPLOSION_FX2 = 220,
   OBJF_ENERGY_BALL_UNUSED = 223,
   OBJF_THUNDER_BALL_INITIAL_ORB = 225,
   OBJF_THUNDER_BALL_CHILD_ORB = 226,
   OBJF_MAP36_LEENA = 256,
   OBJF_MAP36_VILLAGER = 258,
   OBJF_MAP23_SCN39_SHRINK_WARP_SPRITE = 265,
   OBJF_MAP61_SCN83_SHRINK_WARP_SPRITE = 266,
   OBJF_LIGHTNING_TENDRIL_UNUSED = 269,
   OBJF_STAT_RAISED = 272,
   OBJF_OUTWARD_RAY = 273,
   OBJF_CAMERA_TBD_277 = 277,
   OBJF_FAINT_SPARKLES = 278,
   OBJF_ICE_STORM_CAMERA = 279,
   OBJF_DARK_HURRICANE_TARGET = 280,
   OBJF_DARK_HURRICANE_FX2 = 281,
   OBJF_DARK_HURRICANE_FX3 = 282,
   OBJF_DARK_HURRICANE_CLOUD = 283,
   OBJF_CASTING_FX = 285,
   OBJF_MAP13_BRIDGE_EXPLOSION_IN_BATTLE = 288,
   OBJF_CHIMNEY_SMOKE_RING = 289,
   OBJF_REVEAL_CHEST_ITEM = 290,
   OBJF_CHEST_IMPACT = 291,
   OBJF_BLUE_ITEM_SPARKLE = 292,
   OBJF_REVEAL_HIDDEN_ITEM = 294,
   OBJF_SMOKE = 295, //?
   OBJF_MAP17_SPRAY_PARTICLE = 296,
   OBJF_SPLASH_DROPLETS = 297,
   OBJF_SPLASH_WITH_DROPLETS = 298,
   OBJF_MAP15_OCEAN = 299,
   OBJF_MAP32_SMOKESTACK = 300,
   OBJF_MAP32_SMOKESTACK_PARTICLE = 301,
   OBJF_CHIMNEY_SMOKE = 302,
   OBJF_MAP31_SCN61_XENO_FLAME = 304,
   OBJF_EXTRA_HEALING_FX2 = 306, // + Hyper Healing
   OBJF_EVIL_STREAM_FX3 = 307,
   OBJF_EXPLOSION = 309,
   OBJF_LIFE_ORB_BEAM = 310,
   OBJF_BUBBLE_SWIRL_BUBBLE = 312,
   OBJF_INWARD_RAY = 314,
   OBJF_VERTICAL_RAY = 315,
   OBJF_AVALANCHE_FX3 = 317,
   OBJF_MAP67_SCN34_RIFT_ARCS = 319,
   OBJF_MAP67_SCN34_BOLT_ENDPOINT = 320,
   OBJF_EXPANDING_EXPLOSION_RING_UNUSED = 321,
   OBJF_SUMMON_RUNE_COLUMN_323 = 323,
   OBJF_EVIL_STREAM_FX2 = 324,
   OBJF_MAP43_SCN93_MAGIC_STONE_FX = 328,
   OBJF_SALAMANDER_HEAD = 335,
   OBJF_SALAMANDER_SEGMENT = 336,
   OBJF_DAGGER_STORM_BLOOD_SPLATTER = 337,
   OBJF_AVALANCHE_RUBBLE = 339,
   OBJF_FILE_SAVE_MENU = 341,
   OBJF_FILE_SAVE_MENU_IBS = 342,
   OBJF_FILE_LOAD_MENU_343 = 343,
   OBJF_ROMAN_FIRE_FX2 = 344,
   OBJF_ROMAN_FIRE_FX3 = 345,
   OBJF_BUTTON_DEPRESS = 346,
   OBJF_MAP26 = 347,
   OBJF_RUBBLE = 349,
   OBJF_MSGBOX_TEXT = 351,
   OBJF_MAP29 = 352,
   OBJF_FILE_SAVE_MENU_UNK = 353,
   OBJF_MAP19_ELEVATOR_355 = 355,
   OBJF_MAP19_ELEVATOR_356 = 356,
   OBJF_MAP19 = 357,
   OBJF_MAP19_ELEVATOR = 358,
   OBJF_PHASE_SHIFT_MAP_SCALER = 359,
   OBJF_FILE_LOAD_MENU_DEBUG = 360,
   OBJF_MAP13_BRIDGE_EXPLOSION_IN_SCENE = 361,
   OBJF_DRAWBRIDGE_BUTTON = 362,
   OBJF_MAP15_PLANK = 364,
   OBJF_MAP17_FLOODGATE = 365,
   OBJF_MAP17_BUTTON = 366,
   OBJF_FILE_LOAD_MENU_367 = 367,
   OBJF_MAP17_DRAINING_WATER = 368,
   OBJF_SCREEN_EFFECT = 369,
   OBJF_MAGE_OIL_FX2 = 370,
   OBJF_RESTORE_8_MP_UNK_FX2 = 371,
   OBJF_MAGIC_CHARGE_FX2 = 372, // + Mage Gem
   OBJF_FILE_LOAD_MENU = 373,
   OBJF_FILE_LOAD_MENU_IBS = 374,
   OBJF_FLAME_BREATH_PARTICLE = 375,
   OBJF_FILE_LOAD_MENU_DEFEAT = 376,
   OBJF_EVIL_STREAM_ROCK = 379,
   OBJF_LEVEL_UP_FX = 380,
   OBJF_FLAME_BREATH = 382,
   OBJF_SPARKLE = 383,
   OBJF_REVEAL_MIMIC = 385,
   OBJF_HEALING_SPARKLE_386 = 386,
   OBJF_FULLSCREEN_IMAGE = 387,
   OBJF_DARK_HURRICANE_VORTEX = 389,
   OBJF_DARK_HURRICANE_VORTEX_LAYER = 390,
   OBJF_DYNAMO_HUM_ELECTRIC_ORB = 395,
   OBJF_DYNAMO_HUM_ORB_ELECTRICITY = 396,
   OBJF_EXPLOSION_BURST_PARTICLE_UNUSED = 398,
   OBJF_MAP11 = 399,
   OBJF_AI_BUILD_SPELL_VALUE_GRID = 400,
   OBJF_AI_BUILD_ENEMY_PROXIMITY_GRID = 401,
   OBJF_AI_PLAN_SPELL_CAST = 402,
   OBJF_AI_PLAN_ATTACK = 403,
   OBJF_AI_PLAN_RETREAT = 404,
   OBJF_PANORAMA = 405,
   OBJF_SHOP_OR_DEPOT = 406,
   OBJF_NOOP_407 = 407,
   OBJF_CLOSED_WINDOW = 408,
   OBJF_EVENT_ENTITY = 409,
   OBJF_EVENT_ZOOM = 410,
   OBJF_MAP_OBJECT_VILE_BOG = 411,
   OBJF_EVENT_CAMERA = 412,
   OBJF_MSGBOX_PORTRAIT = 413,
   OBJF_DEBUG_MENU = 414,
   OBJF_MAP_OBJECT_TORCH = 415,
   OBJF_BATTLE_VICTORY = 420,
   OBJF_UPPER_MSGBOX_TAIL = 421,
   OBJF_LOWER_MSGBOX_TAIL = 422,
   OBJF_BATTLE_DEFEAT = 423,
   OBJF_BATTLE_ENDER = 424,
   OBJF_BATTLE_OPTIONS = 425,
   OBJF_EVALUATE_BATTLE_10 = 426,
   OBJF_EVALUATE_BATTLE_11 = 427,
   OBJF_EVALUATE_BATTLE_12 = 428,
   OBJF_EVALUATE_BATTLE_13 = 429,
   OBJF_EVALUATE_BATTLE_14 = 430,
   OBJF_EVALUATE_BATTLE_15 = 431,
   OBJF_EVALUATE_BATTLE_16 = 432,
   OBJF_EVALUATE_BATTLE_17 = 433,
   OBJF_EVALUATE_BATTLE_STD = 434,
   OBJF_EVALUATE_BATTLE_19 = 435,
   OBJF_EVALUATE_BATTLE_20 = 436,
   OBJF_EVALUATE_BATTLE_21 = 437,
   OBJF_EVALUATE_BATTLE_08 = 438,
   OBJF_EVALUATE_BATTLE_23 = 439,
   OBJF_EVALUATE_BATTLE_26 = 442,
   OBJF_EVALUATE_BATTLE_27 = 443,
   OBJF_EVALUATE_BATTLE_28 = 444,
   OBJF_EVALUATE_BATTLE_29 = 445,
   OBJF_BATTLE_VICTORY_PARTICLE = 446,
   OBJF_UNIT_PORTRAIT = 447,
   OBJF_MAP_OBJECT_FLOWING_WATER = 449,
   OBJF_FADE_LIGHT_BRIGHTEN = 535,
   OBJF_FADE_LIGHT_DARKEN = 536,
   OBJF_EVALUATE_BATTLE_32 = 552,
   OBJF_EVALUATE_BATTLE_33 = 553,
   OBJF_EVALUATE_BATTLE_35 = 555,
   OBJF_EVALUATE_BATTLE_37 = 557,
   OBJF_EVALUATE_BATTLE_38 = 558,
   OBJF_EVALUATE_BATTLE_39 = 559,
   OBJF_EVALUATE_BATTLE_40 = 560,
   OBJF_EVALUATE_BATTLE_42 = 562,
   OBJF_EVALUATE_BATTLE_43 = 563,
   OBJF_MAP_OBJECT_WATER_1 = 564,
   OBJF_MAP_OBJECT_WATER_2 = 565,
   OBJF_MAP_OBJECT_LAVA_1 = 566,
   OBJF_OPENING_CHEST = 567,
   OBJF_MAP_OBJECT_RAIL = 568,
   OBJF_MAP_OBJECT_LAVA_2 = 569,
   OBJF_AI_CHOOSE_ACTION = 570,
   OBJF_LEVEL_UP = 571,
   OBJF_MAP_OBJECT_LAVA_3 = 572,
   OBJF_BATTLE_ITEMS_LIST = 573,
   OBJF_DISPLAY_ICON = 574,
   OBJF_STATUS_PORTRAIT = 575,
   OBJF_TAVERN = 576,
   OBJF_DYNAMIC_ICON = 577,
   OBJF_DOJO = 578,
   OBJF_WORLD_MAP = 579,
   OBJF_TOWN = 580,
   OBJF_AUDIO_CMD = 581,
   OBJF_MAIN_MENU_JPN = 582,
   OBJF_BATTLE_PLAYER_EVENT = 585,
   OBJF_BATTLE_MSGBOX = 586,
   OBJF_BATTLE_ENEMY_EVENT = 587,
   OBJF_FOCUS_CAMERA_WIDE = 588,
   OBJF_AI_MOVE_TO_ESCAPE_POINT = 589,
   OBJF_BATTLE_TURN_TICKER = 590,
   OBJF_MAP_OBJECT_BOULDER = 591,
   OBJF_BATTLE_TURN_START = 592,
   OBJF_BATTLE_RESULTS_UNIT = 593,
   OBJF_BATTLE_RESULTS = 594,
   OBJF_STATUS_WINDOW = 595,
   OBJF_STATUS_WINDOW_MGR = 596,
   OBJF_BATTLE_INTRO = 597,
   OBJF_WORLD_ACTIONS = 598,
   OBJF_MAP32_CAR_RELEASE = 650,
   OBJF_MAP33_LAVA_PIT_PLATFORM = 651,
   OBJF_MAP35_BUTTON = 652,
   OBJF_EXPLODING_TILE = 653,
   OBJF_MAP38_WASH_AWAY_UNIT = 654,
   OBJF_MAP38_RAISE_FLOODGATE = 655,
   OBJF_MAP39 = 656,
   OBJF_MAP38_FLOODGATE = 657,
   OBJF_MAP38_FLOODWATER = 658,
   OBJF_SPLASH = 659,
   OBJF_FLAME = 661,
   OBJF_MAP28_OPEN_DOOR = 662,
   OBJF_MAP28_BUTTON = 663,
   OBJF_MAP27_OPEN_CELL_DOOR = 664,
   OBJF_MAP27_BUTTONS = 665,
   OBJF_MAP14_LOWER_SAND_MOUND = 666,
   OBJF_MAP14_LOWER_SAND_TILE = 667,
   OBJF_MAP14_RAISE_SAND_MOUND = 668,
   OBJF_MAP14_RAISE_SAND_TILE = 669,
   OBJF_MAP14_SAND = 670,
   OBJF_MAP39_SPLASHING_TILE = 672,
   OBJF_LEENA_FORCEFIELD = 675,
   OBJF_HEAVY_RAINFALL = 676,
   OBJF_RAINFALL_DROP = 677,
   OBJF_RIPPLE = 678,
   OBJF_STAT_BUFF_FX = 681,
   OBJF_ADJUST_FACE_ELEVATION = 683,
   OBJF_SLIDING_FACE = 684,
   OBJF_ROCK_SPURT = 685,
   OBJF_ROCK_SPURT_PARTICLE_2 = 686,
   OBJF_LIGHT_RAINFALL = 687,
   OBJF_MAGIC_STONE_EXPLOSION = 690,
   OBJF_ENTITY_BLEND_FADE_UNUSED = 695,
   OBJF_MAP61_SCN83_ELENI_SPARKLE_RINGS = 698,
   OBJF_DYNAMO_HUM_COLORED_BOLT = 700,
   OBJF_FLAMING_ROCK = 702,
   OBJF_MAP40_BARRICADE = 703,
   OBJF_RISING_GLYPH = 707,
   OBJF_MAP14_UNUSED_708 = 708,
   OBJF_MAP14_UNUSED_709 = 709,
   OBJF_PARTICLE_710 = 710, // Generic animated particle?
   OBJF_SUMMON_RUNE_COLUMN_713 = 713,
   OBJF_SPELLBIND_FX2 = 715,
   OBJF_SPELLBIND_FX3 = 716,
   OBJF_TBD_717 = 717,
   OBJF_TBD_718 = 718,
   OBJF_DIMENSIONAL_RIFT = 719,
   OBJF_MAP61_SCN83_XENO_CASTING_CYLINDER_CREST = 721,
   OBJF_DIMENSIONAL_RIFT_SPARKLES = 722,
   OBJF_HOMING_PARTICLE = 723,
   OBJF_FLICKERING_EXPAND_RING = 728,
   OBJF_MAP67_SCN34_SHRINK_WARP_SPRITE = 729,
   OBJF_MID_BATTLE_TRANSFORMATION = 732,
   OBJF_STAT_BUFF_ICON = 733,
   OBJF_SPARKLE_DUST = 735,
   OBJF_REMOVE_PARALYSIS_BUBBLE = 736,
   OBJF_REMOVE_PARALYSIS = 737,
   OBJF_MAP40_LOWER_BARRICADE = 738,
   OBJF_PARTICLE_739 = 739,
   OBJF_REMOVE_PARALYSIS_SPARKLES = 740,
   OBJF_DIMENSIONAL_RIFT_OPEN_741 = 741,
   OBJF_ENTITY_BLOCKING = 746,
   OBJF_WYRMFANG_FLAMES_CW = 747,
   OBJF_WYRMFANG_FLAMES_CCW = 748,
   OBJF_WYRMFANG_FLAME = 749,
   OBJF_MAP33_SCN65_LOWER_PLATFORM = 750,
   OBJF_MAP33_LOWER_PLATFORM = 751,
   OBJF_MAP15_PIRATE_STAND_IN = 755,
   OBJF_PUSHED_OBJECT_SPLASH = 757,
   OBJF_ROCK_SPURT_PARTICLE = 759,
   OBJF_ELITE_MELEE_SPARKLES = 760,
   OBJF_REVEAL_USED_ITEM = 761,
   OBJF_BOULDER_RUBBLE = 763,
   OBJF_PROJECTILE_TRAIL_POISON = 764,
   OBJF_PROJECTILE_TRAIL_EXPLOSION = 765,
   OBJF_PROJECTILE_TRAIL_SMOKE = 766,
   OBJF_PROJECTILE_TRAIL_SPARKLES = 767,
   OBJF_ITEM_SPELL = 770,
   OBJF_HEALING_PLUS_FX2 = 791,
   OBJF_ULTRA_HEALING_FX2 = 792,
   OBJF_SUPREME_HEALING_FX2 = 793, // + Holy H2O
   OBJF_FADE = 795,
   OBJF_MAIN_MENU = 796,
   OBJF_ENGULF_LIGHTNING_DAMAGE_B = 799,
   OBJF_ENGULF_LIGHTNING_SLAY_B = 800,
   OBJF_FLAME_RING_SPRITE = 801,
   OBJF_EXPLOSION_RING_SPRITE = 802,
   OBJF_LIGHTNING_RING_SPRITE = 803,
} ObjFunctionIdx;

/* ---- PC_PORT (Stage 2.3): 64-bit coords-alias padding ---------------------------------------
 * Several union members in Object_DataStore DELIBERATELY alias Object_Sprite's `coords[4]` at
 * 0x3C: an effect writes its vertices through its own struct, and the shared renderer
 * (AddObjPrim2/4/5 in object.c) reads them back as `obj->d.sprite.coords`. That alias holds only
 * while every one of those structs puts coords at the same offset.
 *
 * Object_Sprite has `void *animData` at 0x38, immediately before coords. On a 64-bit host that
 * pointer grows 4 -> 8 bytes and pushes Object_Sprite.coords from +24 to +32, while the effect
 * structs -- which have plain s16 fields there -- keep coords at +24. The renderer then reads 8
 * bytes past the vertices the effect actually wrote. Symptom: the casting "inward ray" effect
 * (Objf314_InwardRay) drew white blobs instead of thin rays, because its quad's tail vertices
 * came back as unrelated struct fields.
 *
 * This pad restores the alias on 64-bit hosts. It is a no-op for the matching build (which never
 * defines PC_PORT) and for any 32-bit build. Object_396 needs no pad: it already carries the same
 * `void *animData` at 0x38 and therefore shifts identically to Object_Sprite. */
#if defined(PC_PORT) && (defined(__LP64__) || defined(_WIN64) || defined(__x86_64__) || defined(__aarch64__))
#define PC_PORT_LP64 1   /* PC build on a 64-bit host; matching/32-bit builds leave it undefined */
/* Two sizes: Object_Sprite's `void *animData` sits at a 4-byte-aligned offset, so on LP64 it
 * costs +8 (4 bytes of alignment padding + 4 bytes of growth) and pushes coords 24 -> 32. Most
 * aliasing structs naturally sit at 24 and need the full +8; three (Object_323_713, _675, _719)
 * already carry their own pointer and land at 28, needing only +4. Verified by measuring
 * offsetof(...,coords) for all 14 at both widths -- they must ALL read 32 at -m64.
 *
 * ⚠️ The PAD only fixes `coords`. A struct whose OWN pointer is a LEADING field at 0x24 (aliasing
 * Object_Sprite's 4 hidden/facing bytes) ALSO shifts gfxIdx/clut/boxIdx off Object_Sprite's alias on
 * LP64 -- the PAD does not help those. Object_675 and Object_719 have that shape and are fixed
 * individually below (leading pointer -> a 4-byte gap + the pointer relocated to the 0x38 animData
 * slot, like the correct Object_396). See docs/width-bugs.md. */
#define PC_PORT_COORDS_ALIAS_PAD8 u8 pc_coordsAliasPad[8];
#define PC_PORT_COORDS_ALIAS_PAD4 u8 pc_coordsAliasPad[4];
#else
#define PC_PORT_COORDS_ALIAS_PAD8
#define PC_PORT_COORDS_ALIAS_PAD4
#endif

typedef struct Object_Sprite {
   /* :0x24 */ s8 hidden;
   /* :0x25 */ s8 facingLeft;
   /* :0x26 */ s8 animInitialized;
   /* :0x27 */ s8 finishedMoving;
   /* :0x28 */ s16 gfxIdx;
   /* :0x2A */ s16 direction; /* s->w->n->e, 0x400 increments */
   /* :0x2C */ s16 clut;
   /* :0x2E */ s8 currentFrameDelay;
   /* :0x2F */ s8 animDataIdx;
   /* :0x30 */ s8 animFinished;
   /* :0x31 */ s8 semiTrans;
   /* :0x32 */ s16 otOfs;
   /* :0x34 */ s16 boxIdx;
   /* :0x36 */ s16 stripIdx;
   /* :0x38 */ void *animData;
   /* :0x3C */ SVectorXZY coords[4];
   /* :0x54 */ s8 facingFront;
   /* :0x55 */ u8 unk_0x55[3];
   /* :0x58 */ s16 animRelativeOfs;
   /* :0x5A */ s16 animYOfs;
   /* :0x5C */ s8 animSingleAxis;
   /* :0x5D */ u8 unk_0x5D[3];
} Object_Sprite;

/* TBD - Sprites passed to AddObjPrim2 use a different coord layout (xy), and sometimes use other
 * fields for their own data. */
typedef struct Object_Sprite2 {
   /* :0x24 */ s8 hidden;
   /* :0x25 */ u8 unk_0x25[3];
   /* :0x28 */ s16 gfxIdx;
   /* :0x2A */ u8 unk_0x2A[2];
   /* :0x2C */ s16 clut;
   /* :0x2E */ u8 unk_0x2E[3];
   /* :0x31 */ s8 semiTrans;
   /* :0x32 */ s16 otOfs;
   /* :0x34 */ u8 unk_0x34[8];
   PC_PORT_COORDS_ALIAS_PAD8
   /* :0x3C */ SVectorXY coords[4];
   /* :0x4C */ u8 unk_0x4C[20];
} Object_Sprite2;

/* Data Store (Union) */
typedef union Object_DataStore {
   s8 bytes[60];
   s16 shorts[30];
   s32 ints[15];
   struct Object *objs[15];
   void *pointers[15];
   Cylinder cylinder;
   SVECTOR svectors[7];
} Object_DataStore;

/* Battle - Actions */
typedef struct Object_003 {
   /* :0x24 */ s8 range;
   /* :0x25 */ s8 performedSubaction; // Whether unit has committed to a turn by using examine/push
   /* :0x26 */ u8 unk_0x26;
   /* :0x27 */ s8 remainingRange;
   /* :0x28 */ struct UnitStatus *unit;
   /* :0x2C */ u8 unk_0x2C;
   /* :0x2D */ s8 cursorUnitIdx;
   /* :0x2E */ u8 unk_0x2E[30];
   /* :0x4C */ s8 timer;
   /* :0x4D */ u8 unk_0x4D[19];
} Object_003;

/* Window (incomplete) */
typedef struct Object_004_005_408 {
   /* :0x24 */ s32 effectPhase;
   /* :0x28 */ s16 clut;
   /* :0x2A */ s16 choicesTopMargin;
   /* :0x2C */ /*s16 highlightY;*/ s32 highlightY;
   ///* :0x2E */ s16 highlightYOffset;
   /* :0x30 */ s32 dstHighlightY;
   /* :0x34 */ s8 highlightHeight;
   /* :0x35 */ s8 choiceCt;
   /* :0x36 */ s8 choiceHeight;
   /* :0x37 */ s8 windowId;
   /* :0x38 */ s16 halfHeight;
   /* :0x3A */ s16 effectHalfHeight;
   /* :0x3C */ s16 halfWidth;
   /* :0x3E */ u8 unk_0x3E[2];
   /* :0x40 */ s16 relQuadX0;
   /* :0x42 */ s16 relQuadY0;
   /* :0x44 */ s16 relQuadX1;
   /* :0x46 */ s16 relQuadY1;
   /* :0x48 */ s16 relQuadX2;
   /* :0x4A */ s16 relQuadY2;
   /* :0x4C */ s16 relQuadX3;
   /* :0x4E */ s16 relQuadY3;
   /* :0x50 */ struct Object *highlight;
   /* :0x54 */ struct Object *window;
   /* :0x58 */ s16 effectX;
   /* :0x5A */ s16 effectY;
   /* :0x5C */ s8 effect;
   /* :0x5D */ s8 translucentHighlight;
   /* :0x5E */ s8 otOfs;
   /* :0x5F */ s8 disableWraparound;
} Object_004_005_408;

/* Apply Poison */
typedef struct Object_007 {
   /* :0x24 */ struct UnitStatus *unit;
   /* :0x28 */ s8 timer;
   /* :0x29 */ u8 unk_0x29[55];
} Object_007;

/* Battle Portrait */
typedef struct Object_008 {
   /* :0x24 */ s8 windowId;
   /* :0x25 */ s8 flipped;
   /* :0x26 */ u8 unk_0x26[2];
   /* :0x28 */ struct Object *window;
   /* :0x2C */ struct Object *sprite;
   /* :0x30 */ s16 portraitId;
   /* :0x32 */ u8 unk_0x32[46];
} Object_008;

/* Item Icon Manager */
typedef struct Object_009 {
   /* :0x24 */ u8 unk_0x24[56];
   /* :0x5C */ struct Object *shopOrDepot; // objf406
} Object_009;

/* Battle Manager */
typedef struct Object_013 {
   /* :0x24 */ struct UnitStatus *unit;
   /* :0x28 */ struct Object *unitSprite;
   /* :0x2C */ s8 timer;
   /* :0x2D */ s8 followUnit;
   /* :0x2E */ u8 unk_0x2E[50];
} Object_013;

/* Battle Unit */
typedef struct Object_014 {
   /* :0x24 */ u8 team;
   /* :0x25 */ u8 unitIdx;
   /* :0x26 */ s8 pathIdx;
   /* :0x27 */ s8 animIdx;
   /* :0x28 */ struct UnitStatus *unit;
   /* :0x2C */ u8 **animSet;
   /* :0x30 */ struct Object *sprite;
   /* :0x34 */ s8 timer;
   /* :0x35 */ u8 unk_0x35;
   /* :0x36 */ s16 direction;
   /* :0x38 */ u8 unk_0x38[40];
} Object_014;

/* Choose Done Direction */
typedef struct Object_016 {
   /* :0x24 */ struct UnitStatus *unit;
   /* :0x28 */ s16 angle;
   /* :0x2A */ u8 unk_0x2A[54];
} Object_016;

/* Camera - TBD */
typedef struct Object_017 {
   /* :0x24 */ struct Object *sprite;
   /* :0x28 */ s16 camSavedX;
   /* :0x2A */ s16 camSavedZ;
   /* :0x2C */ s16 camSavedY;
   /* :0x2E */ s16 camSavedRotX;
   /* :0x30 */ s16 camSavedRotZ;
   /* :0x32 */ s16 camSavedRotY;
   /* :0x34 */ s16 camSavedZoom;
   /* :0x36 */ s16 dstZoom;
   /* :0x38 */ s16 dstCamRotY;
   /* :0x3A */ s16 timer;
   /* :0x3C */ s16 savedGeomOfsY;
   /* :0x3E */ s16 geomOfsY;
   /* :0x40 */ s16 dstGeomOfsY;
   /* :0x42 */ u8 unk_0x42[30];
} Object_017;

/* Pushed Boulder */
typedef struct Object_020 {
   /* :0x24 */ struct Object *unitSprite;
   /* :0x28 */ s16 savedCamX;
   /* :0x2A */ s16 savedCamZ;
   /* :0x2C */ s16 savedCamY;
   /* :0x2E */ s16 savedCamRotX;
   /* :0x30 */ u8 unk_0x30[4];
   /* :0x34 */ s16 savedZoom;
   /* :0x36 */ u8 unk_0x36[4];
   /* :0x3A */ s16 timer;
   /* :0x3C */ u8 unk_0x3C[36];
} Object_020;

/* Unit Attacking */
typedef struct Object_021 {
   /* :0x24 */ struct UnitStatus *attacker;
   /* :0x28 */ struct UnitStatus *defender;
   /* :0x2C */ struct Object *attackerSprite;
   /* :0x30 */ struct Object *defenderSprite;
   /* :0x34 */ s8 countering;
   /* :0x35 */ u8 unk_0x35[3];
   /* :0x38 */ s8 timer;
   /* :0x39 */ s8 mapSizeX;
   /* :0x3A */ s8 mapSizeZ;
   /* :0x3B */ u8 unk_0x3B[37];
} Object_021;

/* Projectile */
typedef struct Object_022_029 {
   /* :0x24 */ struct Object *sprite;
   /* :0x28 */ s16 xMid;
   /* :0x2A */ s16 zMid;
   /* :0x2C */ s16 xzMidDist;
   /* :0x2E */ s16 zMidDist;
   /* :0x30 */ s16 yRotOfs;
   /* :0x32 */ s16 zRotOfs;
   /* :0x34 */ u8 unk_0x34[4];
   /* :0x38 */ s16 zRot;
   /* :0x3A */ u8 unk_0x3A[38];
} Object_022_029;

/* Camera - Ranged Target */
typedef struct Object_023 {
   /* :0x24 */ struct Object *targetSprite;
   /* :0x28 */ s16 yRotDst;
   /* :0x2A */ s16 delay;
   /* :0x2C */ u8 unk_0x2C[52];
} Object_023;

/* Camera - Bounce Zoom */
typedef struct Object_024 {
   /* :0x24 */ s16 soft;
   /* :0x26 */ s16 decayAngle;
   /* :0x28 */ s16 amplitude;
   /* :0x2A */ s16 phase;
   /* :0x2C */ s16 savedZoom;
   /* :0x2E */ u8 unk_0x2E[50];
} Object_024;

/* Overhead Map View */
typedef struct Object_025 {
   /* :0x24 */ s16 camSavedRotX;
   /* :0x26 */ s16 camSavedRotY;
   /* :0x28 */ s16 camSavedZoom;
   /* :0x2A */ s16 camSavedX;
   /* :0x2C */ s16 camSavedY;
   /* :0x2E */ s16 camSavedZ;
   /* :0x30 */ s8 delay;
   /* :0x31 */ u8 unk_0x31[47];
} Object_025;

/* Camera - TBD */
typedef struct Object_026_588 {
   /* :0x24 */ struct Object *target;
   /* :0x28 */ s8 type;
   /* :0x29 */ u8 unk_0x29;
   /* :0x2A */ s16 zoom;
   /* :0x2C */ s16 todo_x2c;
   /* :0x2E */ u8 unk_0x2E[8];
   /* :0x36 */ s16 dstZoom;
   /* :0x38 */ s16 dstCamRotY;
   /* :0x3A */ s16 timer;
   /* :0x3C */ s16 savedGeomOfsY;
   /* :0x3E */ s16 geomOfsY;
   /* :0x40 */ s16 dstGeomOfsY;
   /* :0x42 */ u8 unk_0x42[2];
   /* :0x44 */ s8 wideOcclusionTest;
   /* :0x45 */ u8 unk_0x45[27];
} Object_026_588;

/* Targeting Spell */
typedef struct Object_027 {
   /* :0x24 */ u8 unk_0x24[4];
   /* :0x28 */ struct UnitStatus *caster;
   /* :0x2C */ u8 unk_0x2C[52];
} Object_027;

/* Unit Casting Spell */
typedef struct Object_028 {
   /* :0x24 */ s8 timer;
   /* :0x25 */ s8 mapSizeX;
   /* :0x26 */ s8 mapSizeZ;
   /* :0x27 */ u8 unk_0x27[57];
} Object_028;

/* Unit/Field Info */
typedef struct Object_030 {
   /* :0x24 */ s8 previousTerrain;
   /* :0x25 */ u8 unk_0x25[3];
   /* :0x28 */ s8 unitIdx;
   /* :0x29 */ u8 unk_0x29[55];
} Object_030;

/* Battle Spells List */
typedef struct Object_031 {
   /* :0x24 */ struct UnitStatus *unit;
   /* :0x28 */ s8 spell;
   /* :0x29 */ u8 unk_0x29[55];
} Object_031;

/* Display Damage */
typedef struct Object_032_033 {
   /* :0x24 */ struct Object *barFg;
   /* :0x28 */ struct Object *barBg;
   /* :0x2C */ s16 barDstX3;
   /* :0x2E */ s16 timer;
   /* :0x30 */ s32 maxHp;
   /* :0x34 */ s32 prevHp;
   /* :0x38 */ s32 curHp;
   /* :0x3C */ u8 unk_0x3C[36];
} Object_032_033;

/* Map Object - Chest */
typedef struct Object_040 {
   /* :0x24 */ s8 item;
   /* :0x25 */ u8 unk_0x25[3];
   /* :0x28 */ s16 lidAngle;
   /* :0x2A */ s16 lidAngleVel;
   /* :0x2C */ s8 facing;
   /* :0x2D */ s8 terrain;
   /* :0x2E */ u8 unk_0x2E[34];
   /* :0x50 */ s16 lidCrease1Z;
   /* :0x52 */ s16 lidCrease1Y;
   /* :0x54 */ s16 lidCrease2Z;
   /* :0x56 */ s16 lidCrease2Y;
   /* :0x58 */ s16 lidBackZ;
   /* :0x5A */ s16 lidBackY;
   /* :0x5C */ s16 lidFrontZ;
   /* :0x5E */ s16 lidFrontY;
} Object_040;

/* Map Object - Crate */
typedef struct Object_046 {
   /* :0x24 */ u8 unk_0x24[4];
   /* :0x28 */ s8 stack;
   /* :0x29 */ u8 unk_0x29;
   /* :0x2A */ s16 elevation;
   /* :0x2C */ u8 unk_0x2C[48];
   /* :0x5C */ s8 terrain;
   /* :0x5D */ u8 unk_0x5D[3];
} Object_046;

/* Push */
typedef struct Object_048 {
   /* :0x24 */ struct UnitStatus *unit;
   /* :0x28 */ s8 stack;
   /* :0x29 */ u8 unk_0x29;
   /* :0x2A */ s16 direction;
   /* :0x2C */ s8 targetX;
   /* :0x2D */ s8 targetZ;
   /* :0x2E */ u8 unk_0x2E[2];
   /* :0x30 */ s16 osc;
   /* :0x32 */ u8 unk_0x32[42];
   /* :0x5C */ s8 timer;
   /* :0x5D */ u8 unk_0x5D[3];
} Object_048;

/* Unit Sprites Decoder */
typedef struct Object_050 {
   /* :0x24 */ u8 unk_0x24[24];
   /* :0x3C */ s32 remainingBytes;
   /* :0x40 */ u16 encodingBits;
   /* :0x42 */ u16 cacheOfs;
   /* :0x44 */ void *src;
   /* :0x48 */ void *dst;
   /* :0x4C */ u8 unk_0x4C[4];
   /* :0x50 */ void *baseSrcDataPtr;
   /* :0x54 */ void *baseDstDataPtr;
   /* :0x58 */ s16 vramX;
   /* :0x5A */ s16 vramY;
   /* :0x5C */ u8 unk_0x5C[4];
} Object_050;

/* Floating Damage Text */
/* TODO: Rename stuff to also account for restoration. */
typedef struct Object_051 {
   /* :0x24 */ s16 damage;
   /* :0x26 */ s16 clut;
   /* :0x28 */ s16 phase;
   /* :0x2A */ s16 timer;
   /* :0x2C */ u8 unk_0x2C[32];
   /* :0x4C */ s16 hundreds;
   /* :0x4E */ s16 tens;
   /* :0x50 */ s16 ones;
   /* :0x52 */ s16 numDigits;
   /* :0x54 */ u8 unk_0x54[4];
   /* :0x58 */ struct Object *unitSprite;
   /* :0x5C */ struct Object *digitSprite;
} Object_051;

/* Attack Info Marker */
typedef struct Object_052 {
   /* :0x24 */ s16 type;
   /* :0x26 */ s16 clut;
   /* :0x28 */ s16 angle;
   /* :0x2A */ s16 timer;
   /* :0x2C */ u8 unk_0x2C[44];
   /* :0x58 */ struct Object *unitSprite;
   /* :0x5C */ struct Object *sprite;
} Object_052;

/* Debug - VRAM */
typedef struct Object_059 {
   /* :0x24 */ s16 widthIdx;
   /* :0x26 */ s16 heightIdx;
   /* :0x28 */ s16 width;
   /* :0x2A */ s16 height;
   /* :0x2C */ u8 unk_0x2C[52];
} Object_059;

/* Stretch Warp Sprite */
typedef struct Object_062 {
   /* :0x24 */ s16 speed;
   /* :0x26 */ s16 phase;
   /* :0x28 */ u8 unk_0x28[40];
   /* :0x50 */ struct Object *sprite;
   /* :0x54 */ u8 unk_0x54[12];
} Object_062;

/* Fade From Black */
typedef struct Object_070 {
   /* :0x24 */ s16 speed;
   /* :0x26 */ u8 unk_0x26[38];
   /* :0x4C */ s16 fade;
   /* :0x4E */ u8 unk_0x4E[18];
} Object_070;

/* Fade To Black */
typedef struct Object_071 {
   /* :0x24 */ s16 speed;
   /* :0x26 */ u8 unk_0x26[38];
   /* :0x4C */ s16 fade;
   /* :0x4E */ u8 unk_0x4E[18];
} Object_071;

/* Fade From White */
typedef struct Object_072 {
   /* :0x24 */ s16 speed;
   /* :0x26 */ u8 unk_0x26[38];
   /* :0x4C */ s16 fade;
   /* :0x4E */ u8 unk_0x4E[18];
} Object_072;

/* Fade To White */
typedef struct Object_073 {
   /* :0x24 */ s16 speed;
   /* :0x26 */ u8 unk_0x26[38];
   /* :0x4C */ s16 fade;
   /* :0x4E */ u8 unk_0x4E[18];
} Object_073;

/* Fade In Sprite */
typedef struct Object_074 {
   /* :0x24 */ s16 speed;
   /* :0x26 */ u8 unk_0x26[2];
   /* :0x28 */ s16 fade;
   /* :0x2A */ u8 unk_0x2A[50];
   /* :0x5C */ struct Object *sprite;
} Object_074;

/* Fade Out Sprite */
typedef struct Object_075 {
   /* :0x24 */ s16 speed;
   /* :0x26 */ u8 unk_0x26[2];
   /* :0x28 */ s16 fade;
   /* :0x2A */ u8 unk_0x2A[50];
   /* :0x5C */ struct Object *sprite;
} Object_075;

/* FX - Circles (TBD) */
typedef struct Object_076 {
   /* :0x24 */ s16 timer;
   /* :0x26 */ u8 unk_0x26[40];
   /* :0x4E */ s16 axis;
   /* :0x50 */ s16 dirSign;
   /* :0x52 */ u8 unk_0x52[14];
} Object_076;

/* FX - Circle (TBD) */
typedef struct Object_077 {
   /* :0x24 */ s16 fade;
   /* :0x26 */ u8 unk_0x26[38];
   /* :0x4C */ s16 angle;
   /* :0x4E */ s16 axis;
   /* :0x50 */ s16 dirSign;
   /* :0x52 */ u8 unk_0x52[14];
} Object_077;

/* Damage - FX2 */
typedef struct Object_078 {
   /* :0x24 */ s16 timer;
   /* :0x26 */ u8 unk_0x26[58];
} Object_078;

/* Slay - FX3 */
typedef struct Object_079 {
   /* :0x24 */ s16 timer;
   /* :0x26 */ u8 unk_0x26[58];
} Object_079;

/* Roman Fire - FX1 */
typedef struct Object_080 {
   /* :0x24 */ u8 unk_0x24[48];
   /* :0x54 */ struct Object *target;
   /* :0x58 */ struct Object *cam;
   /* :0x5C */ struct Object *sprite;
} Object_080;

/* FX - TBD */
typedef struct Object_082 {
   /* :0x24 */ s16 radius;
   /* :0x26 */ u8 unk_0x26[2];
   /* :0x28 */ s16 theta1;
   /* :0x2A */ s16 theta2;
   /* :0x2C */ s16 theta3;
   /* :0x2E */ s16 theta4;
   /* :0x30 */ u8 unk_0x30[40];
   /* :0x58 */ struct Object *anchor;
   /* :0x5C */ struct Object *sprite;
} Object_082;

/* Map 20 - Scene 30 - Arrow Spawner */
typedef struct Object_087 {
   /* :0x24 */ union {
      struct Object *entitySpriteParam;
      struct {
         s16 z, y;
      } velocity;
   } variant_0x24;
   /* :0x28 */ s16 yAccel;
   /* :0x2A */ u8 unk_0x2A[50];
   /* :0x5C */ struct Object *entitySprite;
} Object_087;

/* FX - TBD */
typedef struct Object_088 {
   /* :0x24 */ u8 unk_0x24[4];
   /* :0x28 */ s16 gfxIdx;
   /* :0x2A */ u8 unk_0x2A[2];
   /* :0x2C */ s16 clut;
   /* :0x2E */ u8 unk_0x2E[14];
   PC_PORT_COORDS_ALIAS_PAD8
   /* :0x3C */ SVectorXZY coords[4];
   /* :0x54 */ u8 unk_0x54[12];
} Object_088;

/* Map 15 - Scene 17 - Cinematic */
typedef struct Object_089 {
   /* :0x24 */ s16 approachOfsX;
   /* :0x26 */ u8 unk_0x26[2];
   /* :0x28 */ Camera camera;
   /* :0x36 */ u8 unk_0x36[26];
   /* :0x50 */ struct Object *focus;
   /* :0x54 */ struct Object *ocean;
   /* :0x58 */ s16 dstRotY;
   /* :0x5A */ u8 unk_0x5A[2];
   /* :0x5C */ struct Object *plank;
} Object_089;

/* Dagger Storm - FX2 */
typedef struct Object_090 {
   /* :0x24 */ s16 theta1;
   /* :0x26 */ s16 theta2;
   /* :0x28 */ s16 radius;
   /* :0x2A */ u8 unk_0x2A[50];
   /* :0x5C */ s16 fatal;
   /* :0x5E */ u8 unk_0x5E[2];
} Object_090;

/* Dagger Storm - Dagger */
typedef struct Object_091 {
   /* :0x24 */ s16 theta1;
   /* :0x26 */ s16 theta2;
   /* :0x28 */ s16 radius;
   /* :0x2A */ u8 unk_0x2A[18];
   /* :0x3C */ SVectorXZY positions[4];
   /* :0x54 */ u8 unk_0x54[4];
   /* :0x58 */ struct Object *targetSprite;
   /* :0x5C */ struct Object *sprite;
} Object_091;

/* Mood Ring - FX1 */
typedef struct Object_094 {
   /* :0x24 */ u8 unk_0x24[52];
   /* :0x58 */ s8 launchingOutward;
   /* :0x59 */ u8 unk_0x59[7];
} Object_094;

/* Mood Ring - Ring */
typedef struct Object_095 {
   /* :0x24 */ u8 unk_0x24[4];
   /* :0x28 */ s16 gfxIdx_unused;
   /* :0x2A */ u8 unk_0x2A[10];
   /* :0x34 */ s16 boxIdx_unused;
   /* :0x36 */ u8 unk_0x36[14];
   /* :0x44 */ s32 unused_0x44;
   /* :0x48 */ u8 unk_0x48[12];
   /* :0x54 */ s16 theta;
   /* :0x56 */ s16 radius;
   /* :0x58 */ u8 unk_0x58[4];
   /* :0x5C */ struct Object *parent; // objf094
} Object_095;

/* Mood Ring - FX2 */
typedef struct Object_096 {
   /* :0x24 */ s16 fatal;
   /* :0x26 */ u8 unk_0x26[54];
   /* :0x5C */ struct Object *targetSprite;
} Object_096;

/* Mood Ring - FX3 */
typedef struct Object_097 {
   /* :0x24 */ s16 fatal;
   /* :0x26 */ u8 unk_0x26[54];
   /* :0x5C */ struct Object *targetSprite;
} Object_097;

/* Map 20 - Scene 30 - Arrow */
typedef struct Object_098 {
   /* :0x24 */ s8 hidden;
   /* :0x25 */ u8 unk_0x25[3];
   /* :0x28 */ s16 gfxIdx;
   /* :0x2A */ u8 unk_0x2A[6];
   /* :0x30 */ s8 animFinished;
   /* :0x31 */ u8 unk_0x31[11];
   PC_PORT_COORDS_ALIAS_PAD8
   /* :0x3C */ SVectorXZY coords[4];
   /* :0x54 */ SVectorXZY pos;
   /* :0x5A */ u8 unk_0x5A[6];
} Object_098;

/* FX - TBD */
typedef struct Object_099 {
   /* :0x24 */ s16 theta1;
   /* :0x26 */ s16 theta2;
   /* :0x28 */ s16 speed;
   /* :0x2A */ u8 unk_0x2A[2];
   /* :0x2C */ s16 clut;
   /* :0x2E */ u8 unk_0x2E[2];
   /* :0x30 */ s16 lifetime;
   /* :0x32 */ u8 unk_0x32[42];
   /* :0x5C */ struct Object *sprite;
} Object_099;

/* Healing - FX2 */
// Used for: Healing, Self Healing, Herb, Megaherb, Moon Pie
typedef struct Object_100 {
   /* :0x24 */ s16 timer;
   /* :0x26 */ u8 unk_0x26[18];
   /* :0x38 */ struct Object *unitSprite;
   /* :0x3C */ u8 unk_0x3C[32];
   /* :0x5C */ struct Object *dataStore;
} Object_100;

/* Healing Sparkle */
typedef struct Object_101 {
   /* :0x24 */ s16 riseY;
   /* :0x26 */ s16 orbitAngle;
   /* :0x28 */ s16 riseSpeed;
   /* :0x2A */ s16 orbitSpeed;
   /* :0x2C */ u8 unk_0x2C[44];
   /* :0x58 */ struct Object *sprite;
   /* :0x5C */ struct Object *parent;
} Object_101;

/* Poison - FX2 */
typedef struct Object_102_227 {
   /* :0x24 */ s16 timer;
   /* :0x26 */ u8 unk_0x26[18];
   /* :0x38 */ struct Object *unitSprite;
   /* :0x3C */ u8 unk_0x3C[32];
   /* :0x5C */ struct Object *dataStore;
} Object_102_227;

/* Poison Bubbles */
typedef struct Object_103 {
   /* :0x24 */ s16 riseY;
   /* :0x26 */ s16 baseAngle;
   /* :0x28 */ s16 riseSpeed;
   /* :0x2A */ s16 swayRadius;
   /* :0x2C */ u8 unk_0x2C[44];
   /* :0x58 */ struct Object *sprite;
   /* :0x5C */ struct Object *parent;
} Object_103;

/* Cure - FX2 */
typedef struct Object_104 {
   /* :0x24 */ s16 animAngle;
   /* :0x26 */ s16 collapseAngle;
   /* :0x28 */ u8 unk_0x28[24];
   /* :0x40 */ s16 ringSpin;
   /* :0x42 */ u8 unk_0x42[2];
   /* :0x44 */ struct Object *ringSprite;
   /* :0x48 */ u8 unk_0x48[20];
   /* :0x5C */ struct Object *dataStore;
} Object_104;

/* Magic Charge - FX3 */
typedef struct Object_106 {
   /* :0x24 */ s16 animAngle;
   /* :0x26 */ s16 bobAngle;
   /* :0x28 */ u8 unk_0x28[52];
   /* :0x5C */ struct Object *glyphRing;
} Object_106;

/* Magic Charge - Glyph Ring */
typedef struct Object_107 {
   /* :0x24 */ u8 unk_0x24[12];
   /* :0x30 */ s16 fade;
   /* :0x32 */ s16 fadeTicker;
   /* :0x34 */ u8 unk_0x34[24];
   /* :0x4C */ s16 killOnFadeOut;
   /* :0x4E */ u8 unk_0x4E[10];
   /* :0x58 */ s16 ringAngle;
   /* :0x5A */ u8 unk_0x5A[6];
} Object_107;

/* Harmful Wave - FX2 */
typedef struct Object_108 {
   /* :0x24 */ s16 timer;
   /* :0x26 */ u8 unk_0x26[18];
   /* :0x38 */ struct Object *unitSprite;
   /* :0x3C */ u8 unk_0x3C[36];
} Object_108;

/* Harmful Wave - Ring */
typedef struct Object_109 {
   /* :0x24 */ s16 timer;
   /* :0x26 */ u8 unk_0x26[38];
   /* :0x4C */ s16 height;
   /* :0x4E */ u8 unk_0x4E[14];
   /* :0x5C */ struct Object *unitSprite;
} Object_109;

/* Casting Stat Buff */
typedef struct Object_110 {
   /* :0x24 */ s16 theta;
   /* :0x26 */ u8 unk_0x26[10];
   /* :0x30 */ s16 glyphsFade;
   /* :0x32 */ s16 crestFade;
   /* :0x34 */ u8 unk_0x34[24];
   /* :0x4C */ s16 clut;
   /* :0x4E */ u8 unk_0x4E[10];
   /* :0x58 */ struct Object *buffFx;
   /* :0x5C */ u8 unk_0x5C[4];
} Object_110;

/* Faerie - FX2 */
typedef struct Object_115 {
   /* :0x24 */ s16 theta;
   /* :0x26 */ s16 dist;
   /* :0x28 */ u8 unk_0x28[4];
   /* :0x2C */ s16 timer;
   /* :0x2E */ u8 unk_0x2E[42];
   /* :0x58 */ struct Object *targetSprite;
   /* :0x5C */ struct Object *faerieSprite;
} Object_115;

/* Faerie - Sparkle */
typedef struct Object_116 {
   /* :0x24 */ s16 xOfs;
   /* :0x26 */ s16 zOfs;
   /* :0x28 */ s16 yOfs;
   /* :0x2A */ s16 fade;
   /* :0x2C */ u8 unk_0x2C[48];
   /* :0x5C */ struct Object *sprite;
} Object_116;

/* Faerie - Sparkle Trail */
typedef struct Object_117 {
   /* :0x24 */ u8 unk_0x24[6];
   /* :0x2A */ s16 fade;
   /* :0x2C */ u8 unk_0x2C[48];
   /* :0x5C */ struct Object *sprite;
} Object_117;

/* Faerie - Target */
typedef struct Object_118 {
   /* :0x24 */ struct Object *targetSprite;
   /* :0x28 */ s16 timer;
   /* :0x2A */ u8 unk_0x2A[2];
   /* :0x2C */ s16 unused_0x2C;
   /* :0x2E */ u8 unk_0x2E[30];
   /* :0x4C */ s16 clut;
   /* :0x4E */ s16 fade;
   /* :0x50 */ u8 unk_0x50[16];
} Object_118;

/* FX - TBD */
typedef struct Object_119 {
   /* :0x24 */ s16 type;
   /* :0x26 */ s16 boxIdx;
   /* :0x28 */ s16 clut;
   /* :0x2A */ s16 semiTrans;
   /* :0x2C */ s16 once;
   /* :0x2E */ s16 radius;
   /* :0x30 */ s16 theta;
   /* :0x32 */ s16 h;
   /* :0x34 */ u8 unk_0x34[24];
   /* :0x4C */ s16 timer;
   /* :0x4E */ u8 unk_0x4E[14];
   /* :0x5C */ struct Object *sprite;
} Object_119;

/* Dark Fire - FX1 */
typedef struct Object_122 {
   /* :0x24 */ s16 timer;
   /* :0x26 */ s16 theta;
   /* :0x28 */ u8 unk_0x28[16];
   /* :0x38 */ s16 savedLightRotZ;
   /* :0x3A */ s16 savedLightRotY;
   /* :0x3C */ u8 unk_0x3C[28];
   /* :0x58 */ struct Object *sprite;
   /* :0x5C */ struct Object *target;
} Object_122;

/* FX - TBD */
typedef struct Object_130 {
   /* :0x24 */ s16 bandStart;
   /* :0x26 */ s16 bandEnd;
   /* :0x28 */ u8 unk_0x28[38];
   /* :0x4E */ s16 dirZ;
   /* :0x50 */ s16 dirX;
   /* :0x52 */ u8 unk_0x52[14];
} Object_130;

/* Slay Unit */
typedef struct Object_131 {
   /* :0x24 */ s16 speed;
   /* :0x26 */ u8 unk_0x26[54];
   /* :0x5C */ struct Object *unitSprite;
} Object_131;

/* FX - TBD */
/* 132, 134, 136, 138, 140, 142, 799, 800 */
typedef struct Object_132_Etc {
   /* :0x24 */ s16 phase; // also used as a timer
   /* :0x26 */ u8 unk_0x26[6];
   /* :0x2C */ s16 fade;
   /* :0x2E */ s16 increasing;
   /* :0x30 */ u8 unk_0x30[28];
   /* :0x4C */ s16 clut;
   /* :0x4E */ u8 unk_0x4E[2];
   /* :0x50 */ struct Object *unitSprite;
   /* :0x54 */ u8 unk_0x54[12];
} Object_132_Etc;

/* FX - TBD */
typedef struct Object_133_Etc {
   /* :0x24 */ s16 clut;
   /* :0x26 */ s16 semiTrans;
   /* :0x28 */ s16 maxHeight;
   /* :0x2A */ s16 pulseSpeed; // increments x34
   /* :0x2C */ s16 unused_0x2C;
   /* :0x2E */ s16 radius;
   /* :0x30 */ s16 theta;
   /* :0x32 */ s16 h;
   /* :0x34 */ s16 pulsePhase;
   /* :0x36 */ u8 unk_0x36[22];
   /* :0x4C */ s16 timer;
   /* :0x4E */ u8 unk_0x4E[14];
   /* :0x5C */ struct Object *sprite;
} Object_133_Etc;

/* Holy Lightning - FX2 */
typedef struct Object_144 {
   /* :0x24 */ s16 timer;
   /* :0x26 */ s16 endingFxType;
   /* :0x28 */ u8 unk_0x28[48];
   /* :0x58 */ struct Object *targetFlashingFx;
   /* :0x5C */ struct Object *unitSprite;
} Object_144;

/* Holy Lightning - Electric Orb */
typedef struct Object_145 {
   /* :0x24 */ s16 fade;
   /* :0x26 */ u8 unk_0x26[14];
   /* :0x34 */ s16 x_1; // TBD
   /* :0x36 */ s16 y_1;
   /* :0x38 */ s16 z_1;
   /* :0x3A */ s16 x_2;
   /* :0x3C */ s16 y_2;
   /* :0x3E */ s16 z_2;
   /* :0x40 */ s16 unusedJitterX; //
   /* :0x42 */ s16 unusedJitterY; //? Randomized but never used?
   /* :0x44 */ s16 unusedJitterZ; //
   /* :0x46 */ u8 unk_0x46[6];
   /* :0x4C */ s16 x_3;
   /* :0x4E */ s16 y_3;
   /* :0x50 */ s16 z_3;
   /* :0x52 */ u8 unk_0x52[2];
   /* :0x54 */ s16 x_4;
   /* :0x56 */ s16 z_4;
   /* :0x58 */ struct Object *orbSprite;
   /* :0x5C */ struct Object *lightningSprite;
} Object_145;

/* FX - TBD */
typedef struct Object_147 {
   /* :0x24 */ s16 clut;
   /* :0x26 */ u8 unk_0x26[14];
   /* :0x34 */ s16 joint1X;
   /* :0x36 */ s16 joint1Y;
   /* :0x38 */ s16 joint1Z;
   /* :0x3A */ s16 joint2X;
   /* :0x3C */ s16 joint2Y;
   /* :0x3E */ s16 joint2Z;
   /* :0x40 */ s16 joint3X;
   /* :0x42 */ s16 joint3Y;
   /* :0x44 */ s16 joint3Z;
   /* :0x46 */ u8 unk_0x46[6];
   /* :0x4C */ s16 tipX;
   /* :0x4E */ s16 tipY;
   /* :0x50 */ s16 tipZ;
   /* :0x52 */ u8 unk_0x52[10];
   /* :0x5C */ struct Object *sprite;
} Object_147;

/* FX - TBD */
typedef struct Object_148 {
   /* :0x24 */ s16 timer;
   /* :0x26 */ s16 endingFxType;
   /* :0x28 */ u8 unk_0x28[36];
   /* :0x4C */ s16 clut;
   /* :0x4E */ u8 unk_0x4E[18];
} Object_148;

/* Flashing Unit Sprite */
typedef struct Object_149 {
   /* :0x24 */ s16 clut;
   /* :0x26 */ s16 fadeSpeed;
   /* :0x28 */ u8 unk_0x28[4];
   /* :0x2C */ s16 fade;
   /* :0x2E */ s16 increasing;
   /* :0x30 */ u8 unk_0x30[44];
   /* :0x5C */ struct Object *unitSprite;
} Object_149;

/* Spellbind - Glyph */
typedef struct Object_155 {
   /* :0x24 */ s16 initialIdx;
   /* :0x26 */ u8 unk_0x26[2];
   /* :0x28 */ s16 gfxIdx;
   /* :0x2A */ u8 unk_0x2A[7];
   /* :0x31 */ u8 semiTrans;
   /* :0x32 */ u8 unk_0x32[10];
   PC_PORT_COORDS_ALIAS_PAD8
   /* :0x3C */ SVectorXZY coords[4];
   /* :0x54 */ s16 idx;
   /* :0x56 */ s16 theta;
   /* :0x58 */ s16 layerY;
   /* :0x5A */ s16 yOfs;
   /* :0x5C */ s16 ySpeed;
   /* :0x5E */ s16 wobblePhase;
} Object_155;

/* Delta Mirage - FX1 */
typedef struct Object_156 {
   /* :0x24 */ s16 theta;
   /* :0x26 */ s16 edgeAngle0;
   /* :0x28 */ s16 edgeAngle1;
   /* :0x2A */ s16 edgeAngle2;
   /* :0x2C */ u8 unk_0x2C[4];
   /* :0x30 */ s16 timer;
   /* :0x32 */ u8 unk_0x32[4];
   /* :0x36 */ s16 collapsePhase;
   /* :0x38 */ u8 unk_0x38[40];
} Object_156;

/* Delta Mirage - Ray */
typedef struct Object_157 {
   /* :0x24 */ s16 maxLength;
   /* :0x26 */ s16 thetaX;
   /* :0x28 */ s16 thetaZ;
   /* :0x2A */ s16 thetaY;
   /* :0x2C */ s16 dThetaX;
   /* :0x2E */ s16 dThetaZ;
   /* :0x30 */ s16 dThetaY;
   /* :0x32 */ s16 timer;
   /* :0x34 */ s16 length;
   /* :0x36 */ s16 lengthScale;
   /* :0x38 */ u8 unk_0x38[36];
   /* :0x5C */ struct Object *parent;
} Object_157;

/* Explosion - FX1 */
typedef struct Object_158 {
   /* :0x24 */ s16 timer;
   /* :0x26 */ u8 unk_0x26[54];
   /* :0x5C */ struct Object *unitSprite;
} Object_158;

/* Explosion - Rays */
typedef struct Object_159 {
   /* :0x24 */ s16 theta1;
   /* :0x26 */ s16 span1;
   /* :0x28 */ s16 speed1;
   /* :0x2A */ s16 radius1;
   /* :0x2C */ s16 theta2;
   /* :0x2E */ s16 span2;
   /* :0x30 */ s16 speed2;
   /* :0x32 */ s16 radius2;
   /* :0x34 */ s16 theta3;
   /* :0x36 */ s16 span3;
   /* :0x38 */ s16 speed3;
   /* :0x3A */ s16 radius3;
   /* :0x3C */ s16 theta4;
   /* :0x3E */ s16 span4;
   /* :0x40 */ s16 speed4;
   /* :0x42 */ s16 radius4;
   /* :0x44 */ s16 theta5;
   /* :0x46 */ s16 span5;
   /* :0x48 */ s16 speed5;
   /* :0x4A */ s16 radius5;
   /* :0x4C */ s16 theta6;
   /* :0x4E */ s16 span6;
   /* :0x50 */ s16 speed6;
   /* :0x52 */ s16 radius6;
   /* :0x54 */ u8 unk_0x54[4];
   /* :0x58 */ s16 spin;
   /* :0x5A */ s16 tilt;
   /* :0x5C */ struct Object *sprite;
} Object_159;

/* Ice Storm - Target */
typedef struct Object_160 {
   /* :0x24 */ struct Object *targetSprite;
   /* :0x28 */ s16 timer;
   /* :0x2A */ u8 unk_0x2A[2];
   /* :0x2C */ s16 unused_0x2C;
   /* :0x2E */ u8 unk_0x2E[30];
   /* :0x4C */ s16 clut;
   /* :0x4E */ s16 fade;
   /* :0x50 */ u8 unk_0x50[16];
} Object_160;

/* Piercing Light - FX1 */
typedef struct Object_161 {
   /* :0x24 */ u8 unk_0x24[20];
   /* :0x38 */ s16 clut;
   /* :0x3A */ u8 unk_0x3A[34];
   /* :0x5C */ struct Object *unitSprite;
} Object_161;

/* Ice Storm - Splash */
typedef struct Object_162 {
   /* :0x24 */ s16 xOfs;
   /* :0x26 */ s16 zOfs;
   /* :0x28 */ u8 unk_0x28[2];
   /* :0x2A */ s16 fade;
   /* :0x2C */ u8 unk_0x2C[48];
   /* :0x5C */ struct Object *sprite;
} Object_162;

/* Stone Shower - FX1 */
typedef struct Object_163 {
   /* :0x24 */ s16 radius;
   /* :0x26 */ s16 timer;
   /* :0x28 */ u8 unk_0x28[52];
   /* :0x5C */ struct Object *dataStore;
} Object_163;

/* Stone Shower - Rock */
typedef struct Object_164 {
   /* :0x24 */ s16 yOfs;
   /* :0x26 */ s16 theta;
   /* :0x28 */ s16 ySpeed;
   /* :0x2A */ u8 unk_0x2A[6];
   /* :0x30 */ s16 radius;
   /* :0x32 */ s16 delay;
   /* :0x34 */ u8 unk_0x34[24];
   /* :0x4C */ s16 startY;
   /* :0x4E */ s16 endY;
   /* :0x50 */ u8 unk_0x50[4];
   /* :0x54 */ s16 downward;
   /* :0x56 */ u8 unk_0x56[2];
   /* :0x58 */ struct Object *sprite;
   /* :0x5C */ struct Object *parent;
} Object_164;

/* Stone Shower - FX2 */
typedef struct Object_165 {
   /* :0x24 */ s16 endingFxType;
   /* :0x26 */ s16 timer;
   /* :0x28 */ u8 unk_0x28[52];
   /* :0x5C */ struct Object *dataStore;
} Object_165;

/* Rainbow Swirl */
typedef struct Object_167 {
   /* :0x24 */ s16 theta1;
   /* :0x26 */ s16 span1;
   /* :0x28 */ s16 speed1;
   /* :0x2A */ s16 y1;
   /* :0x2C */ s16 theta2;
   /* :0x2E */ s16 span2;
   /* :0x30 */ s16 speed2;
   /* :0x32 */ s16 y2;
   /* :0x34 */ s16 theta3;
   /* :0x36 */ s16 span3;
   /* :0x38 */ s16 speed3;
   /* :0x3A */ s16 y3;
   /* :0x3C */ s16 theta4;
   /* :0x3E */ s16 span4;
   /* :0x40 */ s16 speed4;
   /* :0x42 */ s16 y4;
   /* :0x44 */ s16 theta5;
   /* :0x46 */ s16 span5;
   /* :0x48 */ s16 speed5;
   /* :0x4A */ s16 y5;
   /* :0x4C */ s16 theta6;
   /* :0x4E */ s16 span6;
   /* :0x50 */ s16 speed6;
   /* :0x52 */ s16 y6;
   /* :0x54 */ s16 arcWidth;
   /* :0x56 */ s16 radius;
   /* :0x58 */ s16 spin;
   /* :0x5A */ s16 semiTrans;
   /* :0x5C */ struct Object *sprite;
} Object_167;

/* Rainbow Stroke - Rainbow Swirl */
typedef struct Object_168 {
   /* :0x24 */ s16 theta1;
   /* :0x26 */ s16 span1;
   /* :0x28 */ s16 speed1;
   /* :0x2A */ s16 y1;
   /* :0x2C */ s16 theta2;
   /* :0x2E */ s16 span2;
   /* :0x30 */ s16 speed2;
   /* :0x32 */ s16 y2;
   /* :0x34 */ s16 theta3;
   /* :0x36 */ s16 span3;
   /* :0x38 */ s16 speed3;
   /* :0x3A */ s16 y3;
   /* :0x3C */ s16 theta4;
   /* :0x3E */ s16 span4;
   /* :0x40 */ s16 speed4;
   /* :0x42 */ s16 y4;
   /* :0x44 */ s16 theta5;
   /* :0x46 */ s16 span5;
   /* :0x48 */ s16 speed5;
   /* :0x4A */ s16 y5;
   /* :0x4C */ s16 theta6;
   /* :0x4E */ s16 span6;
   /* :0x50 */ s16 speed6;
   /* :0x52 */ s16 y6;
   /* :0x54 */ s16 arcWidth;
   /* :0x56 */ s16 radius;
   /* :0x58 */ s16 spin;
   /* :0x5A */ s16 semiTrans;
   /* :0x5C */ struct Object *sprite;
} Object_168;

/* Evil Stream - FX1 */
typedef struct Object_169 {
   /* :0x24 */ s16 clut;
   /* :0x26 */ s16 growPhase;
   /* :0x28 */ u8 unk_0x28[2];
   /* :0x2A */ s16 fade;
   /* :0x2C */ u8 unk_0x2C[52];
} Object_169;

/* Piercing Ray, Plasma Wave - FX1 */
typedef struct Object_170 {
   /* :0x24 */ u8 unk_0x24[56];
   /* :0x5C */ struct Object *unitSprite;
} Object_170;

/* Homing Ray */
typedef struct Object_171 {
   /* :0x24 */ s16 clut;
   /* :0x26 */ s16 amplitudeX;
   /* :0x28 */ s16 amplitudeY;
   /* :0x2A */ s16 amplitudeZ;
   /* :0x2C */ s16 dx;
   /* :0x2E */ s16 dz;
   /* :0x30 */ s16 dy;
   /* :0x32 */ u8 unk_0x32[2];
   /* :0x34 */ s16 headIdx;
   /* :0x36 */ s16 tailIdx;
   /* :0x38 */ u8 unk_0x38[40];
} Object_171;

/* Holy Pressure - Cube */
typedef struct Object_172 {
   /* :0x24 */ u8 unk_0x24[4];
   /* :0x28 */ s16 spin;
   /* :0x2A */ s16 timer;
   /* :0x2C */ u8 unk_0x2C[52];
} Object_172;

/* Fire Gem - Beam */
typedef struct Object_173 {
   /* :0x24 */ s16 spinPhase;
   /* :0x26 */ s16 colorIdx;
   /* :0x28 */ s16 beamHeight;
   /* :0x2A */ s16 ringY;
   /* :0x2C */ s16 colorDescending;
   /* :0x2E */ s16 colorTimer;
   /* :0x30 */ s16 phase;
   /* :0x32 */ u8 unk_0x32[42];
   /* :0x5C */ struct Object *fx;
} Object_173;

/* Rainbow Storm - FX1 */
typedef struct Object_175 {
   /* :0x24 */ s16 theta;
   /* :0x26 */ u8 unk_0x26[50];
   /* :0x58 */ s16 iterator;
   /* :0x5A */ u8 unk_0x5A[2];
   /* :0x5C */ struct Object *dataStore;
} Object_175;

/* Rainbow Stroke - FX1 */
typedef struct Object_176 {
   /* :0x24 */ s16 theta;
   /* :0x26 */ u8 unk_0x26[50];
   /* :0x58 */ struct Object *cam;
   /* :0x5C */ struct Object *fx;
} Object_176;

/* Holy Pressure - FX1 */
typedef struct Object_177 {
   /* :0x24 */ s16 timer;
   /* :0x26 */ u8 unk_0x26[50];
   /* :0x58 */ s16 iterator;
   /* :0x5A */ u8 unk_0x5A[2];
   /* :0x5C */ struct Object *dataStore;
} Object_177;

/* Thunder Flash - FX1 */
typedef struct Object_178 {
   /* :0x24 */ s16 maxRadius;
   /* :0x26 */ u8 unk_0x26[2];
   /* :0x28 */ s16 theta;
   /* :0x2A */ u8 unk_0x2A[2];
   /* :0x2C */ s16 fade;
   /* :0x2E */ u8 unk_0x2E[8];
   /* :0x36 */ s16 timer;
   /* :0x38 */ s16 unused_0x38;
   /* :0x3A */ u8 unk_0x3A[36];
   /* :0x5E */ s16 unused_0x5E;
} Object_178;

/* Thunder Flash - Ray */
typedef struct Object_179 {
   /* :0x24 */ s16 maxRadius;
   /* :0x26 */ s16 thetaX;
   /* :0x28 */ s16 thetaZ;
   /* :0x2A */ s16 thetaY;
   /* :0x2C */ s16 thetaXSpeed;
   /* :0x2E */ s16 thetaZSpeed;
   /* :0x30 */ s16 thetaYSpeed;
   /* :0x32 */ s16 timer;
   /* :0x34 */ u8 unk_0x34[2];
   /* :0x36 */ s16 growth;
   /* :0x38 */ u8 unk_0x38[36];
   /* :0x5C */ struct Object *parent;
} Object_179;

/* Spread Force - FX1 */
typedef struct Object_180 {
   /* :0x24 */ s16 maxRadius;
   /* :0x26 */ u8 unk_0x26[2];
   /* :0x28 */ s16 growth;
   /* :0x2A */ s16 arcStep;
   /* :0x2C */ s16 fade;
   /* :0x2E */ u8 unk_0x2E[8];
   /* :0x36 */ s16 timer;
   /* :0x38 */ u8 unk_0x38[32];
   /* :0x58 */ s16 iterator;
   /* :0x5A */ u8 unk_0x5A[2];
   /* :0x5C */ struct Object *dataStore;
} Object_180;

/* FX - TBD */
typedef struct Object_182 {
   /* :0x24 */ s16 savedZoom;
   /* :0x26 */ s16 theta;
   /* :0x28 */ s16 unused_0x28;
   /* :0x2A */ u8 unk_0x2A[50];
   /* :0x5C */ struct Object *cam; // objf026
} Object_182;

/* FX - TBD */
typedef struct Object_183 {
   /* :0x24 */ u8 unk_0x24[2];
   /* :0x26 */ s16 wobbleX;
   /* :0x28 */ s16 wobbleY;
   /* :0x2A */ s16 wobbleZ;
   /* :0x2C */ s16 dx;
   /* :0x2E */ s16 dz;
   /* :0x30 */ s16 dy;
   /* :0x32 */ u8 unk_0x32[4];
   /* :0x36 */ s16 timer;
   /* :0x38 */ u8 unk_0x38[36];
   /* :0x5C */ struct Object *sprite;
} Object_183;

/* Avalanche - FX1 */
typedef struct Object_184 {
   /* :0x24 */ s16 timer;
   /* :0x26 */ s16 rise;
   /* :0x28 */ u8 unk_0x28[2];
   /* :0x2A */ s16 dropHeight;
   /* :0x2C */ s16 fallTimer;
   /* :0x2E */ u8 unk_0x2E[2];
   /* :0x30 */ s16 dx;
   /* :0x32 */ s16 dz;
   /* :0x34 */ s16 xOfs_unused;
   /* :0x36 */ s16 zOfs_unused;
   /* :0x38 */ s16 dropSpan;
   /* :0x3A */ s16 fallOfs;
   /* :0x3C */ u8 unk_0x3C[4];
   /* :0x40 */ struct Object *cam;      // objf026
   /* :0x44 */ struct Object *boulder;  // objf211
   /* :0x48 */ struct Object *rocks[6]; // objf185
} Object_184;

/* Avalanche - Rock */
typedef struct Object_185 {
   /* :0x24 */ s16 height;
   /* :0x26 */ s16 theta;
   /* :0x28 */ s16 velocity;
   /* :0x2A */ u8 unk_0x2A[46];
   /* :0x58 */ struct Object *sprite;
   /* :0x5C */ struct Object *parent;
} Object_185;

/* FX - TBD */
typedef struct Object_186 {
   /* :0x24 */ u8 unk_0x24[2];
   /* :0x26 */ s16 fxType;
   /* :0x28 */ u8 unk_0x28[56];
} Object_186;

/* Dark Fire - Ray */
typedef struct Object_188 {
   /* :0x24 */ s16 maxRadius;
   /* :0x26 */ s16 thetaX;
   /* :0x28 */ s16 thetaZ;
   /* :0x2A */ s16 thetaY;
   /* :0x2C */ s16 thetaXSpeed;
   /* :0x2E */ s16 thetaZSpeed;
   /* :0x30 */ s16 thetaYSpeed;
   /* :0x32 */ s16 timer;
   /* :0x34 */ s16 radius;
   /* :0x36 */ s16 growth;
   /* :0x38 */ u8 unk_0x38[36];
   /* :0x5C */ struct Object *target;
} Object_188;

/* Ice Storm - FX1 */
typedef struct Object_189 {
   /* :0x24 */ s16 unused_0x24;
   /* :0x26 */ s16 theta1;
   /* :0x28 */ s16 theta2;
   /* :0x2A */ u8 unk_0x2A[2];
   /* :0x2C */ s16 spin;
   /* :0x2E */ u8 unk_0x2E[46];
   /* :0x5C */ s16 uvScroll;
   /* :0x5E */ u8 unk_0x5E[2];
} Object_189;

/* Magic Arrow - Arrow */
typedef struct Object_191 {
   /* :0x24 */ s16 clut_unused;
   /* :0x26 */ s16 wobbleX;
   /* :0x28 */ s16 wobbleY;
   /* :0x2A */ s16 wobbleZ;
   /* :0x2C */ s16 dx;
   /* :0x2E */ s16 dz;
   /* :0x30 */ s16 dy;
   /* :0x32 */ u8 unk_0x32[2];
   /* :0x34 */ s16 timer;
   /* :0x36 */ u8 unk_0x36[42];
} Object_191;

/* Perfect Guard - FX2 */
typedef struct Object_192 {
   /* :0x24 */ s16 theta;
   /* :0x26 */ u8 unk_0x26[50];
   /* :0x58 */ s16 openFinished;
   /* :0x5A */ s16 closeFinished;
   /* :0x5C */ s16 uvScroll;
   /* :0x5E */ s16 spinTimer;
} Object_192;

/* Dark Star - FX3 */
typedef struct Object_193 {
   /* :0x24 */ s16 timer;
   /* :0x26 */ u8 unk_0x26[40];
   /* :0x4E */ s16 triRadius;
   /* :0x50 */ s16 orbitRadius;
   /* :0x52 */ u8 unk_0x52[8];
   /* :0x5A */ s16 endingFxType;
   /* :0x5C */ struct Object *targetSprite;
} Object_193;

/* Rolling Thunder - FX2 */
typedef struct Object_195 {
   /* :0x24 */ s16 timer;
   /* :0x26 */ s16 endingFxType;
   /* :0x28 */ u8 unk_0x28[56];
} Object_195;

/* Rolling Thunder - Orb Pair */
typedef struct Object_196 {
   /* :0x24 */ s16 timer;
   /* :0x26 */ u8 unk_0x26[2];
   /* :0x28 */ s16 phase;
   /* :0x2A */ s16 theta;
   /* :0x2C */ s16 wobble;
   /* :0x2E */ u8 unk_0x2E[6];
   /* :0x34 */ s16 radius;
   /* :0x36 */ u8 unk_0x36[22];
   /* :0x4C */ s16 clut;
   /* :0x4E */ u8 unk_0x4E[6];
   /* :0x54 */ struct Object *lightningSprite;
   /* :0x58 */ struct Object *orbSprite;
   /* :0x5C */ struct Object *targetSprite;
} Object_196;

/* Rolling Thunder - FX1 */
typedef struct Object_197 {
   /* :0x24 */ union {
      struct Object *unitSpriteParam;
      s16 timer;
   } variant_0x24;
   /* :0x28 */ u8 unk_0x28[16];
   /* :0x38 */ struct Object *unitSprite;
   /* :0x3C */ struct Object *bolts[9];
} Object_197;

/* Rolling Thunder - Casting Bolt */
typedef struct Object_198 {
   /* :0x24 */ s16 pulseTheta;
   /* :0x26 */ s16 theta;
   /* :0x28 */ s16 pulseSpeed;
   /* :0x2A */ s16 baseHeight;
   /* :0x2C */ s16 radiusTheta;
   /* :0x2E */ s16 rotationSpeed;
   /* :0x30 */ u8 unk_0x30[40];
   /* :0x58 */ struct Object *sprite;
   /* :0x5C */ struct Object *parent;
} Object_198;

/* Magic Arrow - FX1 */
typedef struct Object_199 {
   /* :0x24 */ u8 unk_0x24[56];
   /* :0x5C */ struct Object *unitSprite;
} Object_199;

/* Unit Struck */
typedef struct Object_201 {
   /* :0x24 */ u8 unk_0x24[8];
   /* :0x2C */ s16 fade;
   /* :0x2E */ s16 flashState;
   /* :0x30 */ u8 unk_0x30[40];
   /* :0x58 */ struct Object *impactSprite;
   /* :0x5C */ struct Object *unitSprite;
} Object_201;

/* Unit Blocking */
typedef struct Object_202_746 {
   /* :0x24 */ union {
      struct Object *entitySpriteParam;
      s16 timer;
   } variant_0x24;
   /* :0x28 */ u8 unk_0x28[4];
   /* :0x2C */ s16 fade;
   /* :0x2E */ s16 flashState;
   /* :0x30 */ u8 unk_0x30[40];
   /* :0x58 */ struct Object *unusedSprite;
   /* :0x5C */ struct Object *unitSprite;
} Object_202_746;

/* Blocking Impact */
typedef struct Object_203 {
   /* :0x24 */ u8 unk_0x24[4];
   /* :0x28 */ s16 theta;
   /* :0x2A */ u8 unk_0x2A[34];
   /* :0x4C */ s16 xOfs;
   /* :0x4E */ s16 zOfs;
   /* :0x50 */ s16 yOfs;
   /* :0x52 */ u8 unk_0x52[6];
   /* :0x58 */ s16 direction;
   /* :0x5A */ u8 unk_0x5A[2];
   /* :0x5C */ struct Object *sprite;
} Object_203;

/* Summon Crest */
typedef struct Object_204 {
   /* :0x24 */ s16 timer;
   /* :0x26 */ u8 unk_0x26[38];
   /* :0x4C */ s16 maskClut;
   /* :0x4E */ s16 clut;
   /* :0x50 */ s16 vramSrcX;
   /* :0x52 */ s16 vramSrcY;
   /* :0x54 */ u8 unk_0x54[8];
   /* :0x5C */ struct Object *unitSprite;
} Object_204;

/* Blood Spurt */
typedef struct Object_205 {
   /* :0x24 */ s16 timer;
   /* :0x26 */ u8 unk_0x26[6];
   /* :0x2C */ s16 fade;
   /* :0x2E */ s16 flashState;
   /* :0x30 */ u8 unk_0x30[44];
   /* :0x5C */ struct Object *unitSprite;
} Object_205;

/* Blood Spurt Particle */
typedef struct Object_206 {
   /* :0x24 */ SVectorXZY position1;
   /* :0x2A */ s16 position7_x;
   /* :0x2C */ SVectorXZY position2;
   /* :0x32 */ s16 position7_z;
   /* :0x34 */ SVectorXZY position3;
   /* :0x3A */ s16 position7_y;
   /* :0x3C */ SVectorXZY position4;
   /* :0x42 */ s16 position8_x;
   /* :0x44 */ SVectorXZY position5;
   /* :0x4A */ s16 position8_z;
   /* :0x4C */ SVectorXZY position6;
   /* :0x52 */ s16 position8_y;
   /* :0x54 */ u8 unk_0x54[4];
   /* :0x58 */ s16 theta;
   /* :0x5A */ s16 shade;
   /* :0x5C */ struct Object *sprite;
} Object_206;

/* Holy Lightning - FX1 */
typedef struct Object_208 {
   /* :0x24 */ s16 timer;
   /* :0x26 */ u8 unk_0x26[18];
   /* :0x38 */ struct Object *unitSprite;
   /* :0x3C */ struct Object *bolts[9];
} Object_208;

/* Avalanche - Boulder */
typedef struct Object_211 {
   /* :0x24 */ s16 todo_x24;
   /* :0x26 */ u8 unk_0x26[38];
   /* :0x4C */ s16 scale;
   /* :0x4E */ s16 rotY;
   /* :0x50 */ s16 rotZ;
   /* :0x52 */ s16 rotX;
   /* :0x54 */ u8 unk_0x54[12];
} Object_211;

/* Holy Lightning - Casting Bolt */
typedef struct Object_212 {
   /* :0x24 */ s16 pulseTheta;
   /* :0x26 */ s16 theta;
   /* :0x28 */ s16 pulseSpeed;
   /* :0x2A */ s16 baseHeight;
   /* :0x2C */ s16 riseOfs;
   /* :0x2E */ s16 riseSpeed;
   /* :0x30 */ u8 unk_0x30[28];
   /* :0x4C */ s16 riseSpeedBias;
   /* :0x4E */ u8 unk_0x4E[10];
   /* :0x58 */ struct Object *sprite;
   /* :0x5C */ struct Object *parent;
} Object_212;

/* Dust Cloud Spawner */
typedef struct Object_213 {
   /* :0x24 */ s16 timer;
   /* :0x26 */ u8 unk_0x26[58];
} Object_213;

/* Dust Cloud */
typedef struct Object_214 {
   /* :0x24 */ s16 theta;
   /* :0x26 */ u8 unk_0x26[2];
   /* :0x28 */ s16 halfSize;
   /* :0x2A */ u8 unk_0x2A[2];
   /* :0x2C */ struct Object *sprite;
   /* :0x30 */ SVectorXZY positions[8];
} Object_214;

/* Cloud (Sand, Dust, etc.) */
typedef struct Object_215 {
   /* :0x24 */ s16 theta;
   /* :0x26 */ u8 unk_0x26[2];
   /* :0x28 */ SVectorXZY position1;
   /* :0x2E */ s16 position7_x;
   /* :0x30 */ SVectorXZY position2;
   /* :0x36 */ s16 position7_z;
   /* :0x38 */ SVectorXZY position3;
   /* :0x3E */ s16 position7_y;
   /* :0x40 */ SVectorXZY position4;
   /* :0x46 */ s16 position8_x;
   /* :0x48 */ SVectorXZY position5;
   /* :0x4E */ s16 position8_z;
   /* :0x50 */ SVectorXZY position6;
   /* :0x56 */ s16 position8_y;
   /* :0x58 */ s16 halfSize;
   /* :0x5A */ u8 unk_0x5A[2];
   /* :0x5C */ struct Object *sprite;
} Object_215;

/* Explosion - FX2 */
typedef struct Object_220 {
   /* :0x24 */ s16 theta;
   /* :0x26 */ s16 spin;
   /* :0x28 */ u8 unk_0x28[50];
   /* :0x5A */ s16 endingFxType;
   /* :0x5C */ struct Object *sprite;
} Object_220;

/* FX - TBD */
typedef struct Object_222 {
   /* :0x24 */ s16 savedZoom;
   /* :0x26 */ u8 unk_0x26[6];
   /* :0x2C */ s16 fade;
   /* :0x2E */ u8 unk_0x2E[42];
   /* :0x58 */ struct Object *fx;  // objf223
   /* :0x5C */ struct Object *cam; // objf026
} Object_222;

/* FX - TBD */
typedef struct Object_223 {
   /* :0x24 */ u8 unk_0x24[8];
   /* :0x2C */ s16 dx;
   /* :0x2E */ s16 dz;
   /* :0x30 */ s16 dy;
   /* :0x32 */ u8 unk_0x32[4];
   /* :0x36 */ s16 timer;
   /* :0x38 */ u8 unk_0x38[32];
   /* :0x58 */ struct Object *lightningSprite;
   /* :0x5C */ struct Object *orbSprite;
} Object_223;

/* Thunder Ball - FX1 */
typedef struct Object_224 {
   /* :0x24 */ s16 savedZoom;
   /* :0x26 */ u8 unk_0x26[58];
} Object_224;

/* Thunder Ball - Initial Orb */
typedef struct Object_225 {
   /* :0x24 */ u8 unk_0x24[8];
   /* :0x2C */ s16 dx;
   /* :0x2E */ s16 dz;
   /* :0x30 */ s16 dy;
   /* :0x32 */ u8 unk_0x32[4];
   /* :0x36 */ s16 timer;
   /* :0x38 */ u8 unk_0x38[32];
   /* :0x58 */ struct Object *lightningSprite;
   /* :0x5C */ struct Object *orbSprite;
} Object_225;

/* Thunder Ball - Child Orb */
typedef struct Object_226 {
   /* :0x24 */ u8 unk_0x24[2];
   /* :0x26 */ s16 wobbleX;
   /* :0x28 */ u8 unk_0x28[2];
   /* :0x2A */ s16 wobbleZ;
   /* :0x2C */ s16 dx;
   /* :0x2E */ s16 dz;
   /* :0x30 */ s16 dy;
   /* :0x32 */ u8 unk_0x32[4];
   /* :0x36 */ s16 timer;
   /* :0x38 */ u8 unk_0x38[32];
   /* :0x58 */ struct Object *lightningSprite;
   /* :0x5C */ struct Object *orbSprite;
} Object_226;

/* Map 36 - Leena / Villager */
typedef struct Object_256_258 {
   /* :0x24 */ u8 unk_0x24;
   /* :0x25 */ u8 facingLeft;
   /* :0x26 */ u8 unk_0x26[2];
   /* :0x28 */ s16 gfxIdx;
   /* :0x2A */ u8 unk_0x2A[2];
   /* :0x2C */ s16 clut;
   /* :0x2E */ u8 unk_0x2E[6];
   /* :0x34 */ s16 boxIdx;
   /* :0x36 */ u8 unk_0x36[6];
   /* :0x3C */ u16 tpage;
   /* :0x3E */ s16 clut_unused;
   /* :0x40 */ u8 unk_0x40[4];
   /* :0x44 */ s16 texwIdx;
   /* :0x46 */ u8 unk_0x46[26];
} Object_256_258;

/* Shrink & Warp Sprite */
typedef struct Object_265_266_729 {
   /* :0x24 */ struct Object *entitySprite;
   /* :0x28 */ u8 unk_0x28[52];
   /* :0x5C */ struct Object *sprite;
} Object_265_266_729;

/* FX - TBD */
typedef struct Object_269 {
   /* :0x24 */ s16 theta1;
   /* :0x26 */ s16 theta2;
   /* :0x28 */ u8 unk_0x28[44];
   /* :0x54 */ struct Object *sprite;
   /* :0x58 */ struct Object *parent; // objf270
   /* :0x5C */ struct Object *link;   // objf269/objf270
} Object_269;

/* FX - TBD */
typedef struct Object_270 {
   /* :0x24 */ s16 unused_0x24;
   /* :0x26 */ s16 unused_0x26;
   /* :0x28 */ u8 unk_0x28[48];
   /* :0x58 */ struct Object *sprite;
   /* :0x5C */ u8 unk_0x5C[4];
} Object_270;

/* Map 36 - Scene 74 - Leena Casting Shield */
typedef struct Object_271 {
   /* :0x24 */ struct Object *entitySprite;
   /* :0x28 */ u8 unk_0x28[52];
   /* :0x5C */ struct Object *dataStore; // cylinder
} Object_271;

/* Stat Raised */
typedef struct Object_272 {
   /* :0x24 */ u8 unk_0x24[4];
   /* :0x28 */ s16 gfxIdx;
   /* :0x2A */ u8 unk_0x2A[10];
   /* :0x34 */ s16 boxIdx;
   /* :0x36 */ u8 unk_0x36[22];
   /* :0x4C */ s16 type;
   /* :0x4E */ u8 unk_0x4E[6];
   /* :0x54 */ s16 statGfxIdx;
   /* :0x56 */ s16 upGfxIdx;
   /* :0x58 */ s16 animRelativeOfs;
   /* :0x5A */ s16 animYOfs;
   /* :0x5C */ struct Object *unitSprite;
} Object_272;

/* Outward Ray */
typedef struct Object_273 {
   /* :0x24 */ u8 unk_0x24[4];
   /* :0x28 */ s16 gfxIdx;
   /* :0x2A */ u8 unk_0x2A[2];
   /* :0x2C */ s16 clut;
   /* :0x2E */ u8 unk_0x2E[3];
   /* :0x31 */ s8 semiTrans;
   /* :0x32 */ u8 unk_0x32[2];
   /* :0x34 */ s16 theta1;
   /* :0x36 */ s16 theta2;
   /* :0x38 */ s16 dist;
   /* :0x3A */ s16 unused_0x3A;
   PC_PORT_COORDS_ALIAS_PAD8
   /* :0x3C */ SVectorXZY coords[4];
   /* :0x54 */ s16 rotationSpeed_unused;
   /* :0x56 */ u8 unk_0x56[10];
} Object_273;

/* FX - TBD */
typedef struct Object_275 {
   /* :0x24 */ u8 unk_0x24[4];
   /* :0x28 */ s16 gfxIdx;
   /* :0x2A */ u8 unk_0x2A[6];
   /* :0x30 */ s16 animFinished;
   /* :0x32 */ u8 unk_0x32[6];
   /* :0x38 */ void *animData;
   /* :0x3C */ u8 unk_0x3C[36];
} Object_275;

/* Zoom - TBD */
typedef struct Object_277 {
   /* :0x24 */ s16 panX_unused;
   /* :0x26 */ s16 panZ_unused;
   /* :0x28 */ s16 dstZoom;
   /* :0x2A */ u8 unk_0x2A[14];
   /* :0x38 */ s16 smoothness;
   /* :0x3A */ u8 unk_0x3A[38];
} Object_277;

/* Faint Sparkles */
typedef struct Object_278 {
   /* :0x24 */ struct Object *targetSprite;
   /* :0x28 */ u8 unk_0x28[4];
   /* :0x2C */ s16 clut;
   /* :0x2E */ u8 unk_0x2E[46];
   /* :0x5C */ struct Object *sprite;
} Object_278;

/* Ice Storm - Camera */
typedef struct Object_279 {
   /* :0x24 */ struct Object *targetSprite;
   /* :0x28 */ u8 unk_0x28[2];
   /* :0x2A */ s16 dstZoom;
   /* :0x2C */ s16 dstCamRotX;
   /* :0x2E */ s16 delayType;
   /* :0x30 */ u8 unk_0x30[8];
   /* :0x38 */ s16 dstCamRotY;
   /* :0x3A */ s16 timer;
   /* :0x3C */ u8 unk_0x3C[36];
} Object_279;

/* Dark Hurricane - Target */
typedef struct Object_280 {
   /* :0x24 */ s16 theta;
   /* :0x26 */ u8 unk_0x26[46];
   /* :0x54 */ struct Object *parent; // objf389
   /* :0x58 */ struct Object *unitSprite;
   /* :0x5C */ struct Object *sprite;
} Object_280;

/* Dark Hurricane - FX2 / FX3 */
typedef struct Object_281_282 {
   /* :0x24 */ s16 theta;
   /* :0x26 */ u8 unk_0x26[50];
   /* :0x58 */ struct Object *targetSprite;
   /* :0x5C */ struct Object *sprite;
} Object_281_282;

/* Dark Hurricane - Cloud */
typedef struct Object_283 {
   /* :0x24 */ u8 unk_0x24[4];
   /* :0x28 */ s16 gfxIdx;
   /* :0x2A */ u8 unk_0x2A[10];
   /* :0x34 */ s16 boxIdx;
   /* :0x36 */ u8 unk_0x36[2];
   /* :0x38 */ void *animData;
   /* :0x3C */ u8 unk_0x3C[32];
   /* :0x5C */ struct Object *parent; // objf389
} Object_283;

/* FX - TBD */
typedef struct Object_284 {
   /* :0x24 */ u8 unk_0x24[8];
   /* :0x2C */ s16 clut;
   /* :0x2E */ u8 unk_0x2E[6];
   /* :0x34 */ s16 boxIdx;
   /* :0x36 */ u8 unk_0x36[2];
   /* :0x38 */ void *animData;
   /* :0x3C */ Quad quad;
   /* :0x5C */ u8 unk_0x5C[4];
} Object_284;

/* FX - TBD */
typedef struct Object_287 {
   /* :0x24 */ u8 unk_0x24[56];
   /* :0x5C */ struct Object *unitSprite;
} Object_287;

/* Map 13 - Bridge Explosion (Battle) */
typedef struct Object_288 {
   /* :0x24 */ s16 currentX;
   /* :0x26 */ u8 unk_0x26[6];
   /* :0x2C */ s16 dstRotY_unused;
   /* :0x2E */ u8 unk_0x2E[2];
   /* :0x30 */ Camera camera;
   /* :0x3E */ u8 unk_0x3E[22];
   /* :0x54 */ s16 hitPlayerUnit;
   /* :0x56 */ u8 unk_0x56[6];
   /* :0x5C */ s16 iterator;
   /* :0x5E */ u8 unk_0x5E[2];
} Object_288;

/* Reveal Item */
typedef struct Object_290_294_761 {
   /* :0x24 */ s16 timer;
   /* :0x26 */ u8 unk_0x26[2];
   /* :0x28 */ s16 theta;
   /* :0x2A */ u8 unk_0x2A[2];
   /* :0x2C */ s16 gfxIdx;
   /* :0x2E */ u8 unk_0x2E[50];
} Object_290_294_761;

/* Blue Item Sparkles */
typedef struct Object_292 {
   /* :0x24 */ u8 unk_0x24[56];
   /* :0x5C */ struct Object *sprite;
} Object_292;

/* Map 31 - Scene 61 - Xeno's Flames */
typedef struct Object_303 {
   /* :0x24 */ struct Object *entitySprite;
   /* :0x28 */ s16 theta;
   /* :0x2A */ u8 unk_0x2A[54];
} Object_303;

/* Flame */
typedef struct Object_304_661 {
   /* :0x24 */ u8 unk_0x24[13];
   /* :0x31 */ s8 semiTrans;
   /* :0x32 */ u8 unk_0x32[6];
   /* :0x38 */ void *animData;
   /* :0x3C */ u8 unk_0x3C[28];
   /* :0x58 */ s16 radius;
   /* :0x5A */ s16 theta;
   /* :0x5C */ Quad *quadp;
} Object_304_661;

/* Magic Stone - FX (Map 43 - Scene 93 / Map 44 - Scene 00) */
typedef struct Object_305_328 {
   /* :0x24 */ union {
      struct Object *entitySpriteParam;
      struct {
         s16 ray;
         s16 explosion;
      } timers;
   } variant_0x24;
   /* :0x28 */ u8 unk_0x28[48];
   /* :0x58 */ struct Object *sprite;
   /* :0x5C */ struct Object *unused_0x5C;
} Object_305_328;

/* Healing - FX2 */
// TODO: Disambiguate from Object_100?
// 306: Extra Healing, Hyper Healing
// 791: Healing Plus
// 792: Ultra Healing
// 793: Supreme Healing, Holy H2O
typedef struct Object_306_Etc {
   /* :0x24 */ s16 timer;
   /* :0x26 */ u8 unk_0x26[18];
   /* :0x38 */ struct Object *unitSprite;
   /* :0x3C */ u8 unk_0x3C[36];
} Object_306_Etc;

/* Evil Stream - FX2 / FX3 */
typedef struct Object_307_324 {
   /* :0x24 */ Cylinder cylinder;
   /* :0x48 */ u8 unk_0x48[16];
   /* :0x58 */ struct Object *targetSprite;
   /* :0x5C */ struct Object *sprite;
} Object_307_324;

/* Life Orb - Beam */
typedef struct Object_310 {
   /* :0x24 */ s16 height;
   /* :0x26 */ s16 unused_0x26;
   /* :0x28 */ s16 radius;
   /* :0x2A */ u8 unk_0x2A[50];
   /* :0x5C */ struct Object *sprite;
} Object_310;

/* Bubble Swirl - Bubble */
typedef struct Object_312 {
   /* :0x24 */ u8 unk_0x24[4];
   /* :0x28 */ s16 gfxIdx;
   /* :0x2A */ u8 unk_0x2A[2];
   /* :0x2C */ s16 clut;
   /* :0x2E */ u8 unk_0x2E[6];
   /* :0x34 */ s16 boxIdx;
   /* :0x36 */ u8 unk_0x36[6];
   /* :0x3C */ s16 theta;
   /* :0x3E */ s16 yVelocity;
   /* :0x40 */ s16 rVelocity;
   /* :0x42 */ s16 radius;
   /* :0x44 */ u8 unk_0x44[28];
} Object_312;

/* Life Orb - FX1 */
typedef struct Object_313 {
   /* :0x24 */ u8 unk_0x24[56];
   /* :0x5C */ struct Object *beam; // objf310
} Object_313;

/* Inward Ray */
typedef struct Object_314 {
   /* :0x24 */ u8 unk_0x24[4];
   /* :0x28 */ s16 gfxIdx;
   /* :0x2A */ u8 unk_0x2A[2];
   /* :0x2C */ s16 clut;
   /* :0x2E */ u8 unk_0x2E[4];
   /* :0x32 */ s16 otOfs;
   /* :0x34 */ s16 theta1;
   /* :0x36 */ s16 theta2;
   /* :0x38 */ s16 dist;
   /* :0x3A */ s16 unused_0x3A;
   PC_PORT_COORDS_ALIAS_PAD8
   /* :0x3C */ SVectorXZY coords[4];
   /* :0x54 */ s16 rotationSpeed_unused;
   /* :0x56 */ u8 unk_0x56[10];
} Object_314;

/* Vertical Ray */
typedef struct Object_315 {
   /* :0x24 */ u8 unk_0x24[4];
   /* :0x28 */ s16 gfxIdx;
   /* :0x2A */ u8 unk_0x2A[2];
   /* :0x2C */ s16 clut;
   /* :0x2E */ u8 unk_0x2E[4];
   /* :0x32 */ s16 otOfs;
   /* :0x34 */ s16 theta;
   /* :0x36 */ s16 radius;
   /* :0x38 */ s16 top;
   /* :0x3A */ s16 height;
   PC_PORT_COORDS_ALIAS_PAD8
   /* :0x3C */ SVectorXZY coords[4];
   /* :0x54 */ s16 rotationSpeed;
   /* :0x56 */ s16 speed;
   /* :0x58 */ u8 unk_0x58[8];
} Object_315;

/* Avalanche - FX2 / FX3 */
typedef struct Object_317_338 {
   /* :0x24 */ s32 availableSlots;
   /* :0x28 */ s32 rubbleAmount;
   /* :0x2C */ u8 unk_0x2C[52];
} Object_317_338;

/* Map 67 - Scene 34 - TBD */
typedef struct Object_319 {
   /* :0x24 */ struct Object *entitySprite;
   /* :0x28 */ u8 unk_0x28[9];
   /* :0x31 */ s8 semiTrans;
   /* :0x32 */ u8 unk_0x32[30];
   /* :0x50 */ struct Object *arc2Endpoint;
   /* :0x54 */ u8 unk_0x54[4];
   /* :0x58 */ struct Object *arc2;
   /* :0x5C */ struct Object *arc1Endpoint;
} Object_319;

/* FX - TBD */
typedef struct Object_321 {
   /* :0x24 */ s16 thetas[8];
   /* :0x34 */ s16 radius;
   /* :0x36 */ u8 unk_0x36[38];
   /* :0x5C */ struct Object *sprite;
} Object_321;

/* Magic Restoration (Magic Charge, etc.) - FX2 */
typedef struct Object_322_Etc {
   /* :0x24 */ s16 amount;
   /* :0x26 */ u8 unk_0x26[58];
} Object_322_Etc;

/* FX - TBD */
typedef struct Object_323_713 {
   /* :0x24 */ struct Object *entitySprite;
   /* :0x28 */ s16 gfxIdx;
   /* :0x2A */ u8 unk_0x2A[7];
   /* :0x31 */ s8 semiTrans;
   /* :0x32 */ u8 unk_0x32[10];
   PC_PORT_COORDS_ALIAS_PAD4
   /* :0x3C */ SVectorXZY coords[4];
   /* :0x54 */ u8 unk_0x54[8];
   /* :0x5C */ struct Object *dataStore; // cylinder
} Object_323_713;

/* FX - TBD */
typedef struct Object_326 {
   /* :0x24 */ s16 height;
   /* :0x26 */ s16 speed;
   /* :0x28 */ s16 radius;
   /* :0x2A */ u8 unk_0x2A[46];
   /* :0x58 */ struct Object *dataStore;
   /* :0x5C */ struct Object *sprite;
} Object_326;

/* Healing Circle - FX2 */
typedef struct Object_327 {
   /* :0x24 */ u8 unk_0x24[56];
   /* :0x5C */ struct Object *targetSprite;
} Object_327;

/* Magic Restoration (Magic Charge, etc.) - FX1 */
typedef struct Object_330 {
   /* :0x24 */ s16 theta1;
   /* :0x26 */ s16 theta2;
   /* :0x28 */ s16 startingTheta1;
   /* :0x2A */ s16 startingTheta2;
   /* :0x2C */ u8 unk_0x2C[4];
   /* :0x30 */ s16 ringSize;
   /* :0x32 */ u8 unk_0x32[34];
   /* :0x54 */ struct Object *dataStore;
   /* :0x58 */ struct Object *ringBottom;
   /* :0x5C */ struct Object *ringTop;
} Object_330;

/* FX - TBD */
typedef struct Object_331 {
   /* :0x24 */ u8 unk_0x24[4];
   /* :0x28 */ s16 gfxIdx;
   /* :0x2A */ u8 unk_0x2A[10];
   /* :0x34 */ s16 boxIdx;
   /* :0x36 */ u8 unk_0x36[38];
   /* :0x5C */ struct Object *parent; // (Maybe)
} Object_331;

/* Rolling Fire - FX1 */
typedef struct Object_332 {
   /* :0x24 */ u8 unk_0x24[4];
   /* :0x28 */ s16 gfxIdx;
   /* :0x2A */ u8 unk_0x2A[14];
   /* :0x38 */ void *animData;
   /* :0x3C */ u8 unk_0x3C[32];
   /* :0x5C */ struct Object *dataStore; // cylinder
} Object_332;

/* Salamander - FX1 */
typedef struct Object_334 {
   /* :0x24 */ s32 rockSpawnCounter;
   /* :0x28 */ u8 unk_0x28[56];
} Object_334;

/* Salamander - Head / Segment */
typedef struct Object_335_336 {
   /* :0x24 */ s16 theta1;
   /* :0x26 */ s16 theta2;
   /* :0x28 */ s16 theta3;
   /* :0x2A */ s16 theta4;
   /* :0x2C */ s16 radius;
   /* :0x2E */ s16 theta5;
   /* :0x30 */ SVectorXYZ position1;
   /* :0x36 */ u8 unk_0x36[2];
   /* :0x38 */ SVectorXYZ position2;
   /* :0x3E */ u8 unk_0x3E[2];
   /* :0x40 */ SVectorXYZ position3;
   /* :0x46 */ u8 unk_0x46[2];
   /* :0x48 */ SVectorXYZ position4;
   /* :0x4E */ s16 unused_0x4E;
   /* :0x50 */ struct Object *parent; // objf334 (fx1)
   /* :0x54 */ struct Object *sprite;
   /* :0x58 */ u8 unk_0x58[4];
   /* :0x5C */ struct Object *link; // objf334/objf335/objf336
} Object_335_336;

/* File Save Menu */
typedef struct Object_341_342_353 {
   /* :0x24 */ s32 error;
   /* :0x28 */ s32 savedWindowId;
   /* :0x2C */ s16 choice;
   /* :0x2E */ u8 unk_0x2E[50];
} Object_341_342_353;

/* File Load Menu */
typedef struct Object_343_Etc {
   /* :0x24 */ s32 error;
   /* :0x28 */ s32 savedWindowId;
   /* :0x2C */ s16 choice;
   /* :0x2E */ u8 unk_0x2E[2];
   /* :0x30 */ s16 numChoices;
   /* :0x32 */ u8 unk_0x32[2];
   /* :0x34 */ s16 slotOccupied[4];
   /* :0x3C */ u8 unk_0x3C[16];
   /* :0x4C */ s16 savedSeqId;
   /* :0x4E */ u8 unk_0x4E[18];
} Object_343_Etc;

/* Button Depress */
typedef struct Object_346 {
   /* :0x24 */ s16 dstCamRotY;
   /* :0x26 */ u8 unk_0x26[58];
} Object_346;

/* Map 26 */
typedef struct Object_347 {
   /* :0x24 */ u8 unk_0x24[8];
   /* :0x2C */ Camera camera;
   /* :0x3A */ u8 unk_0x3A[34];
   /* :0x5C */ struct Object *buttonDepress; // objf346
} Object_347;

/* FX - TBD */
typedef struct Object_348 {
   /* :0x24 */ s16 targetRadius[8];
   /* :0x34 */ s16 radius[8];
   /* :0x44 */ s16 unused_0x44[8];
   /* :0x54 */ u8 unk_0x54[8];
   /* :0x5C */ struct Object *dataStore; // cylinder
} Object_348;

/* MsgBox Text */
typedef struct Object_351 {
   /* :0x24 */ s16 maxCharsPerLine;
   /* :0x26 */ s16 lineSpacing;
   /* :0x28 */ s16 maxRows;
   /* :0x2A */ u8 unk_0x2A[2];
   /* :0x2C */ s16 textSpeedAccum;
   /* :0x2E */ s16 textSpeed;
   /* :0x30 */ s16 type;
   /* :0x32 */ s16 textPtrIdx;
   /* :0x34 */ s16 pregapChars;
   /* :0x36 */ u8 unk_0x36[2];
   /* :0x38 */ s16 readingFromStringTable;
   /* :0x3A */ s16 indentChars;
   /* :0x3C */ RECT rect;
   /* :0x44 */ s8 fastForward;
   /* :0x45 */ s8 todo_x45;
   /* :0x46 */ u8 unk_0x46[2];
   /* :0x48 */ s16 speakAnimSuppressed;
   /* :0x4A */ u8 unk_0x4A[10];
   /* :0x54 */ u8 *textResumePtr;
   /* :0x58 */ struct Object *buttonIcon;
   /* :0x5C */ u8 *textPtr;
} Object_351;

/* Map 29 */
typedef struct Object_352 {
   /* :0x24 */ u8 unk_0x24[8];
   /* :0x2C */ Camera camera;
   /* :0x3A */ u8 unk_0x3A[34];
   /* :0x5C */ struct Object *buttonDepress; // objf346
} Object_352;

/* Map 19 - Elevator (TBD) */
typedef struct Object_355_356 {
   /* :0x24 */ union {
      struct Object *elevator1Param;
      struct {
         s16 a;
         s16 b;
      } dstCamRotY;
   } variant_0x24;
   /* :0x28 */ struct Object *elevator2Param;
   /* :0x2C */ Camera camera;
   /* :0x3A */ u8 unk_0x3A[30];
   /* :0x58 */ struct Object *elevator2;
   /* :0x5C */ struct Object *elevator1;
} Object_355_356;

/* Map 19 */
typedef struct Object_357 {
   /* :0x24 */ u8 unk_0x24[52];
   /* :0x58 */ struct Object *elevator2;
   /* :0x5C */ struct Object *elevator1;
} Object_357;

/* Map 19 - Elevator (TBD) */
typedef struct Object_358 {
   /* :0x24 */ s16 riseAccum0;
   /* :0x26 */ s16 riseAccum1;
   /* :0x28 */ s16 riseAccum2;
   /* :0x2A */ u8 unk_0x2A[2];
   /* :0x2C */ s16 riseSpeed0;
   /* :0x2E */ s16 riseSpeed1;
   /* :0x30 */ s16 riseSpeed2;
   /* :0x32 */ u8 unk_0x32[2];
   /* :0x34 */ s16 riseApplied0;
   /* :0x36 */ s16 riseApplied1;
   /* :0x38 */ s16 riseApplied2;
   /* :0x3A */ u8 unk_0x3A[38];
} Object_358;

/* Map 13 - Bridge Explosion (Scene) */
typedef struct Object_361 {
   /* :0x24 */ s16 currentX;
   /* :0x26 */ u8 unk_0x26[58];
} Object_361;

/* Drawbridge Button */
typedef struct Object_362 {
   /* :0x24 */ s16 timer;
   /* :0x26 */ u8 unk_0x26[2];
   /* :0x28 */ s16 gfxSetIdx;
   /* :0x2A */ u8 unk_0x2A[2];
   /* :0x2C */ s16 battleNum; // i.e. mapNum - 9 (same as displayed in debug menu)
   /* :0x2E */ u8 unk_0x2E[2];
   /* :0x30 */ struct MapTileModel *tileModel;
   /* :0x34 */ s16 dstCamPosY;
   /* :0x36 */ u8 unk_0x36[2];
   /* :0x38 */ s16 dstCamRotY;
   /* :0x3A */ u8 unk_0x3A[38];
} Object_362;

/* Wyrmfang - FX1 */
typedef struct Object_363 {
   /* :0x24 */ u8 unk_0x24[52];
   /* :0x58 */ struct Object *ccwFlames;
   /* :0x5C */ struct Object *cwFlames;
} Object_363;

/* Map 17 - Floodgate */
typedef struct Object_365 {
   /* :0x24 */ s16 dstCamRotY;
   /* :0x26 */ u8 unk_0x26[6];
   /* :0x2C */ s16 face1Init;
   /* :0x2E */ s16 face2Init;
   /* :0x30 */ s16 face3Init;
   /* :0x32 */ u8 unk_0x32[2];
   /* :0x34 */ s16 face1Curr;
   /* :0x36 */ s16 face2Curr;
   /* :0x38 */ s16 face3Curr;
   /* :0x3A */ u8 unk_0x3A[38];
} Object_365;

/* Map 17 - Button */
typedef struct Object_366 {
   /* :0x24 */ s16 unused_0x24;
   /* :0x26 */ u8 unk_0x26[6];
   /* :0x2C */ Camera camera;
   /* :0x3A */ u8 unk_0x3A[34];
   /* :0x5C */ struct Object *buttonDepress;
} Object_366;

/* Screen Effect (Incomplete) */
typedef struct Object_369 {
   /* :0x24 */ u8 unk_0x24[4];
   /* :0x28 */ s16 speed;
   /* :0x2A */ u8 unk_0x2A[2];
   /* :0x2C */ s16 intensity;
   /* :0x2E */ u8 unk_0x2E[2];
   /* :0x30 */ s16 unused_0x30;
   /* :0x32 */ u8 unk_0x32[2];
   /* :0x34 */ s16 otOfs;
   /* :0x36 */ u8 unk_0x36[2];
   /* :0x38 */ CVECTOR color;
   /* :0x3C */ s16 rmax;
   /* :0x3E */ s16 gmax;
   /* :0x40 */ s16 bmax;
   /* :0x42 */ u8 unk_0x42[2];
   /* :0x44 */ s16 rd;
   /* :0x46 */ s16 gd;
   /* :0x48 */ s16 bd;
   /* :0x4A */ u8 unk_0x4A[2];
   /* :0x4C */ s16 semiTrans;
   /* :0x4E */ u8 unk_0x4E[18];
} Object_369;

/* FX - TBD */
typedef struct Object_276 {
   /* :0x24 */ union {
      struct Object *targetParam;
      s16 unk;
   } variant_0x24;
   /* :0x28 */ u8 unk_0x28[16];
   /* :0x38 */ void *animData;
   /* :0x3C */ u8 unk_0x3C[28];
   /* :0x58 */ struct Object *unused_0x58;
   /* :0x5C */ struct Object *dataStore; // cylinder
} Object_276;

/* FX - TBD */
typedef struct Object_377 {
   /* :0x24 */ s16 theta1;
   /* :0x26 */ s16 theta2;
   /* :0x28 */ s16 theta3;
   /* :0x2A */ s16 theta4;
   /* :0x2C */ s16 radius;
   /* :0x2E */ s16 theta5;
   /* :0x30 */ u8 unk_0x30[36];
   /* :0x54 */ struct Object *sprite;
   /* :0x58 */ u8 unk_0x58[4];
   /* :0x5C */ struct Object *link;
} Object_377;

/* Phase Shift - FX1 */
typedef struct Object_378 {
   /* :0x24 */ SVectorXY positions[8];
   /* :0x44 */ u8 unk_0x44[20];
   /* :0x58 */ struct Object *runeSprite;
   /* :0x5C */ struct Object *crestSprite;
} Object_378;

/* Evil Stream - Rock */
typedef struct Object_379 {
   /* :0x24 */ u8 unk_0x24[16];
   /* :0x34 */ s16 boxIdx;
   /* :0x36 */ u8 unk_0x36[2];
   /* :0x38 */ void *animData;
   /* :0x3C */ s16 maxHeight;
   /* :0x3E */ u8 unk_0x3E[34];
} Object_379;

/* Level Up - FX */
typedef struct Object_380 {
   /* :0x24 */ s16 timer;
   /* :0x26 */ u8 unk_0x26[50];
   /* :0x58 */ struct Object *unitSprite;
   /* :0x5C */ struct Object *sprite;
} Object_380;

/* Flame Breath */
typedef struct Object_382 {
   /* :0x24 */ u8 unk_0x24[4];
   /* :0x28 */ s16 gfxIdx;
   /* :0x2A */ s16 direction;
   /* :0x2C */ s16 clut;
   /* :0x2E */ u8 unk_0x2E[6];
   /* :0x34 */ s16 boxIdx;
   /* :0x36 */ u8 unk_0x36[6];
   /* :0x3C */ s16 theta;
   /* :0x3E */ s16 yTheta;
   /* :0x40 */ s16 unused_0x40;
   /* :0x42 */ u8 unk_0x42[30];
} Object_382;

/* Healing Sparkle */
// TODO: Disambiguate from Object_101?
typedef struct Object_386 {
   /* :0x24 */ u8 unk_0x24[8];
   /* :0x2C */ s16 clut;
   /* :0x2E */ u8 unk_0x2E[10];
   /* :0x38 */ void *animData;
   /* :0x3C */ s16 timer;
   /* :0x3E */ u8 unk_0x3E[34];
} Object_386;

/* Dark Hurricane - FX1 */
typedef struct Object_388 {
   /* :0x24 */ u8 unk_0x24[44];
   /* :0x50 */ struct Object *vortex; // objf389
   /* :0x54 */ u8 unk_0x54[12];
} Object_388;

/* Dark Hurricane - Vortex */
typedef struct Object_389 {
   /* :0x24 */ u8 unk_0x24[8];
   /* :0x2C */ s16 radius;
   /* :0x2E */ u8 unk_0x2E[2];
   /* :0x30 */ s16 theta;
   /* :0x32 */ u8 unk_0x32[34];
   /* :0x54 */ struct Object *sprite;
   /* :0x58 */ struct Object *parent; // objf388
   /* :0x5C */ u8 unk_0x5C[4];
} Object_389;

/* Dark Hurricane - Vortex Layer */
typedef struct Object_390 {
   /* :0x24 */ s16 radius2;
   /* :0x26 */ s16 theta2;
   /* :0x28 */ s16 height;
   /* :0x2A */ u8 unk_0x2A[2];
   /* :0x2C */ s16 radius;
   /* :0x2E */ u8 unk_0x2E[2];
   /* :0x30 */ s16 theta;
   /* :0x32 */ u8 unk_0x32[2];
   /* :0x34 */ s16 maxRadius;
   /* :0x36 */ u8 unk_0x36[2];
   /* :0x38 */ s16 unused_0x38;
   /* :0x3A */ u8 unk_0x3A[26];
   /* :0x54 */ struct Object *sprite;
   /* :0x58 */ struct Object *parent; // objf389/objf390
   /* :0x5C */ u8 unk_0x5C[4];
} Object_390;

/* FX - TBD */
typedef struct Object_392 {
   /* :0x24 */ struct Object *neighborY0;
   /* :0x28 */ struct Object *neighborY1;
   /* :0x2C */ struct Object *neighborX0;
   /* :0x30 */ struct Object *neighborX1;
   /* :0x34 */ u8 unk_0x34[40];
   /* :0x5C */ struct Object *controller;
} Object_392;

/* Map 44 - Scene 00 - Explosion Rays */
typedef struct Object_393 {
   /* :0x24 */ struct Object *entitySprite;
   /* :0x28 */ u8 unk_0x28[52];
   /* :0x5C */ struct Object *sprite;
} Object_393;

/* Dynamo Hum - FX1 */
typedef struct Object_394 {
   /* :0x24 */ u8 unk_0x24[56];
   /* :0x5C */ struct Object *orb; // objf395
} Object_394;

/* Dynamo Hum - Electric Orb */
typedef struct Object_395 {
   /* :0x24 */ u8 unk_0x24[4];
   /* :0x28 */ s16 gfxIdx;
   /* :0x2A */ u8 unk_0x2A[2];
   /* :0x2C */ s16 clut;
   /* :0x2E */ u8 unk_0x2E[2];
   /* :0x30 */ s8 animFinished;
   /* :0x31 */ u8 unk_0x31[3];
   /* :0x34 */ s16 boxIdx;
   /* :0x36 */ u8 unk_0x36[2];
   /* :0x38 */ void *animData;
   /* :0x3C */ s16 theta;
   /* :0x3E */ s16 size;
   /* :0x40 */ u8 unk_0x40[28];
   /* :0x5C */ struct Object *parent; // objf394
} Object_395;

/* Dynamo Hum - Orb Electricity */
typedef struct Object_396 {
   /* :0x24 */ u8 unk_0x24[4];
   /* :0x28 */ s16 gfxIdx;
   /* :0x2A */ s16 bowDir;
   /* :0x2C */ s16 clut;
   /* :0x2E */ u8 unk_0x2E[10];
   /* :0x38 */ void *animData;
   /* :0x3C */ SVectorXZY coords[4];
   /* :0x54 */ s32 length;
   /* :0x58 */ struct Object *links[2];
} Object_396;

/* Map 11 */
typedef struct Object_399 {
   /* :0x24 */ s16 dstRotY;
   /* :0x26 */ u8 unk_0x26[6];
   /* :0x2C */ Camera camera;
   /* :0x3A */ u8 unk_0x3A[38];
} Object_399;

/* AI - TBD */
typedef struct Object_400 {
   /* :0x24 */ struct UnitStatus *unit;
   /* :0x28 */ s16 resumeZ;
   /* :0x2A */ s16 resumeX;
   /* :0x2C */ u8 unk_0x2C[52];
} Object_400;

/* AI - TBD */
typedef struct Object_401 {
   /* :0x24 */ s8 team;
   /* :0x25 */ s8 unitIdx;
   /* :0x26 */ s16 unitIter;
   /* :0x28 */ u8 unk_0x28[56];
} Object_401;

/* AI - TBD */
typedef struct Object_402_Etc {
   /* :0x24 */ s16 resumeZ;
   /* :0x26 */ s16 resumeX;
   /* :0x28 */ u8 unk_0x28[56];
} Object_402_Etc;

/* Panorama */
typedef struct Object_405 {
   /* :0x24 */ s16 yRot;
   /* :0x26 */ s16 xRot;
   /* :0x28 */ s16 xOffset;
   /* :0x2A */ s16 yOffset;
   /* :0x2C */ u8 unk_0x2C[52];
} Object_405;

/* Shop or Depot */
typedef struct Object_406 {
   /* :0x24 */ s8 shopId;
   /* :0x25 */ s8 category;
   /* :0x26 */ s8 shopRows;
   /* :0x27 */ s8 shopTop;
   /* :0x28 */ s8 shopNeedsRedraw;
   /* :0x29 */ s8 goldTimer;
   /* :0x2A */ s8 itemToPurchase;
   /* :0x2B */ s8 partyIdx1;
   /* :0x2C */ s8 partyNeedsRedraw;
   /* :0x2D */ s8 formerEquipment;
   /* :0x2E */ s8 partyRows;
   /* :0x2F */ s8 partyTop;
   /* :0x30 */ s8 selectionModified;
   /* :0x31 */ s8 item;
   /* :0x32 */ s8 partyIdx2;
   /* :0x33 */ s8 itemIdx; // idx of PartyMember.items to sell
   /* :0x34 */ s8 depotNeedsRedraw;
   /* :0x35 */ s8 depotCategory;
   /* :0x36 */ s16 depotTop;
   /* :0x38 */ s8 partyChoice;
   /* :0x39 */ u8 unk_0x39[31];
   /* :0x58 */ s8 exitTimer;
   /* :0x59 */ u8 unk_0x59[3];
   /* :0x5C */ s8 goldState;
   /* :0x5D */ u8 unk_0x5D[3];
} Object_406;

/* Event Entity */
typedef struct Object_409 {
   /* :0x24 */ u8 unk_0x24[3];
   /* :0x27 */ s8 animIdx;
   /* :0x28 */ u8 unk_0x28[8];
   /* :0x30 */ struct Object *sprite;
   /* :0x34 */ u8 unk_0x34[8];
   /* :0x3C */ u8 **baseAnimSet;
   /* :0x40 */ s8 commandState;
   /* :0x41 */ u8 unk_0x41;
   /* :0x42 */ s16 timer;
   /* :0x44 */ s16 *pNextCommand;
   /* :0x48 */ s16 stripIdxA;
   /* :0x4A */ s16 stripIdxB;
   /* :0x4C */ s8 maintainDirection;
   /* :0x4D */ s8 usingAltAnimSet;
   /* :0x4E */ s8 elevationType; // TBD
   /* :0x4F */ u8 unk_0x4F;
   /* :0x50 */ u8 **altAnimSet;
   /* :0x54 */ u8 unk_0x54[12];
} Object_409;

/* Camera - Event Zoom */
typedef struct Object_410 {
   /* :0x24 */ s16 zoom;
   /* :0x26 */ s16 timer;
   /* :0x28 */ u8 unk_0x28[56];
} Object_410;

/* MsgBox Portrait */
typedef struct Object_413 {
   /* :0x24 */ union {
      s16 id;
      s8 idx;
   } portrait;
   /* :0x26 */ s8 flipped;
   /* :0x27 */ u8 unk_0x27;
   /* :0x28 */ s8 speakTimer;
   /* :0x29 */ s8 blinkTimer;
   /* :0x2A */ u8 unk_0x2A[2];
   /* :0x2C */ struct Object *faceSprite;
   /* :0x30 */ struct Object *speakSprite;
   /* :0x34 */ struct Object *blinkSprite;
   /* :0x38 */ struct Object *anchor;
   /* :0x3C */ s16 anchorOfsX;
   /* :0x3E */ s16 anchorOfsY;
   /* :0x40 */ u8 unk_0x40[32];
} Object_413;

/* Battle Victory/Defeat */
typedef struct Object_420_423 {
   /* :0x24 */ s8 timer;
   /* :0x25 */ u8 unk_0x25[59];
} Object_420_423;

/* MsgBox Tail */
typedef struct Object_421_422 {
   /* :0x24 */ struct Object *sprite;
   /* :0x28 */ s16 left;
   /* :0x2A */ s16 top;
   /* :0x2C */ s16 right;
   /* :0x2E */ s16 bottom;
   /* :0x30 */ u8 unk_0x30[48];
} Object_421_422;

/* Battle Ender */
typedef struct Object_424 {
   /* :0x24 */ s8 timer;
   /* :0x25 */ u8 unk_0x25[59];
} Object_424;

/* Battle - Options */
typedef struct Object_425 {
   /* :0x24 */ s8 timer;
   /* :0x25 */ u8 unk_0x25[59];
} Object_425;

/* Evaluate Battle 08 */
typedef struct Object_438 {
   /* :0x24 */ s8 delay;
   /* :0x25 */ u8 unk_0x25[59];
} Object_438;

/* Battle - Victory/Defeat Particle */
typedef struct Object_446 {
   /* :0x24 */ s8 hidden;
   /* :0x25 */ u8 unk_0x25[3];
   /* :0x28 */ s16 gfxIdx;
   /* :0x2A */ u8 unk_0x2A[2];
   /* :0x2C */ s16 clut;
   /* :0x2E */ u8 unk_0x2E[3];
   /* :0x31 */ s8 semiTrans;
   /* :0x32 */ s16 otOfs;
   /* :0x34 */ u8 unk_0x34[8];
   PC_PORT_COORDS_ALIAS_PAD8
   /* :0x3C */ SVectorXY coords[4];
   /* :0x4C */ s16 angle0;
   /* :0x4E */ s16 angle1;
   /* :0x50 */ s16 angleStep0;
   /* :0x52 */ s16 angleStep1;
   /* :0x54 */ s16 radius0;
   /* :0x56 */ s16 radius1;
   /* :0x58 */ s16 radiusStep0;
   /* :0x5A */ s16 radiusStep1;
   /* :0x5C */ s16 life;
   /* :0x5E */ s16 delay;
} Object_446;

/* Unit Portrait (in depot, dojo, etc.) */
typedef struct Object_447 {
   /* :0x24 */ s8 windowId;
   /* :0x25 */ u8 unk_0x25[3];
   /* :0x28 */ struct Object *window;
   /* :0x2C */ struct Object *sprite;
   /* :0x30 */ u8 unk_0x30[48];
} Object_447;

/* Map 61 - Scene 83 - Vandal Heart Forcefield */
typedef struct Object_530 {
   /* :0x24 */ struct Object *entitySprite;
   /* :0x28 */ s16 theta;
   /* :0x2A */ u8 unk_0x2A[54];
} Object_530;

/* Map Object - Rippling Water/Lava (animated texture) */
typedef struct Object_564_565_566 {
   /* :0x24 */ u16 phase;
   /* :0x26 */ u8 unk_0x26[58];
} Object_564_565_566;

/* Opening Chest */
typedef struct Object_567 {
   /* :0x24 */ struct UnitStatus *opener;
   /* :0x28 */ u8 sendToDepot;
   /* :0x29 */ u8 unk_0x29[3];
   /* :0x2C */ struct Object *openerSprite;
   /* :0x30 */ u8 unk_0x30[48];
} Object_567;

/* Level Up - Camera Control, Sound */
typedef struct Object_571 {
   /* :0x24 */ struct Object *sprite;
   /* :0x28 */ s16 camSavedX;
   /* :0x2A */ s16 camSavedZ;
   /* :0x2C */ s16 camSavedY;
   /* :0x2E */ s16 camSavedRotX;
   /* :0x30 */ s16 camSavedRotZ;
   /* :0x32 */ s16 camSavedRotY;
   /* :0x34 */ s16 camSavedZoom;
   /* :0x36 */ s16 dstZoom;
   /* :0x38 */ s16 dstRotY;
   /* :0x3A */ s16 timer;
   /* :0x3C */ s16 savedGeomOfsY;
   /* :0x3E */ s16 geomOfsY;
   /* :0x40 */ s16 dstGeomOfsY;
   /* :0x42 */ u8 unk_0x42[30];
} Object_571;

/* Battle Items List */
typedef struct Object_573 {
   /* :0x24 */ struct UnitStatus *unit;
   /* :0x28 */ s8 item;
   /* :0x29 */ u8 unk_0x29[55];
} Object_573;

/* Status Portrait */
typedef struct Object_575 {
   /* :0x24 */ u8 unk_0x24[4];
   /* :0x28 */ s16 gfxIdx;
   /* :0x2A */ u8 unk_0x2A[2];
   /* :0x2C */ s16 clut;
   /* :0x2E */ u8 unk_0x2E[4];
   /* :0x32 */ s16 otOfs;
   /* :0x34 */ u8 unk_0x34[8];
   /* :0x3C */ s16 portraitId;
   /* :0x3E */ u8 unk_0x3E[34];
} Object_575;

/* Tavern */
typedef struct Object_576 {
   /* :0x24 */ s8 needSpeak[4];
   /* :0x28 */ s8 textPtrIdx;
   /* :0x29 */ u8 unk_0x29[35];
   /* :0x4C */ s8 timer;
   /* :0x4D */ u8 unk_0x4D[19];
} Object_576;

/* Dynamic Icon */
typedef struct Object_577 {
   /* :0x24 */ u8 unk_0x24[4];
   /* :0x28 */ s16 gfxIdx;
   /* :0x2A */ u8 unk_0x2A[18];
   /* :0x3C */ s16 idx;
   /* :0x3E */ u8 unk_0x3E[34];
} Object_577;

/* Dojo */
typedef struct Object_578 {
   /* :0x24 */ s8 timer;
   /* :0x25 */ s8 chosePathB;
   /* :0x26 */ u8 unk_0x26[6];
   /* :0x2C */ s8 partyNeedsRedraw;
   /* :0x2D */ u8 unk_0x2D[2];
   /* :0x2F */ s8 partyTop;
   /* :0x30 */ u8 unk_0x30[2];
   /* :0x32 */ s8 partyIdx;
   /* :0x33 */ u8 unk_0x33[41];
   /* :0x5C */ s16 advancementTimer;
   /* :0x5E */ u8 unk_0x5E[2];
} Object_578;

/* World Map */
typedef struct Object_579 {
   /* :0x24 */ u8 unk_0x24[56];
   /* :0x5C */ s8 timer;
   /* :0x5D */ u8 unk_0x5D[3];
} Object_579;

/* Town */
typedef struct Object_580 {
   /* :0x24 */ u8 unk_0x24[4];
   /* :0x28 */ s8 textPtrIdx;
   /* :0x29 */ u8 unk_0x29;
   /* :0x2A */ s16 portrait;
   /* :0x2C */ u8 unk_0x2C[32];
   /* :0x4C */ s8 timer;
   /* :0x4D */ u8 unk_0x4D[15];
   /* :0x5C */ s8 fadeTimer;
   /* :0x5D */ u8 unk_0x5D[3];
} Object_580;

/* Audio Command */
typedef struct Object_581 {
   /* :0x24 */ s16 cmd;
   /* :0x26 */ u8 unk_0x26[2];
   /* :0x28 */ s8 delay;
   /* :0x29 */ u8 unk_0x29[55];
} Object_581;

/* Battle - Player Event */
typedef struct Object_585 {
   /* :0x24 */ u8 unk_0x24[56];
   /* :0x5C */ s8 timer;
   /* :0x5D */ u8 unk_0x5D[3];
} Object_585;

/* Battle - MsgBox */
typedef struct Object_586 {
   /* :0x24 */ s16 nameIdx;
   /* :0x26 */ s16 portrait;
   /* :0x28 */ s16 textIdx;
   /* :0x2A */ s16 omitTail;
   /* :0x2C */ u8 unk_0x2C[48];
   /* :0x5C */ s8 timer;
   /* :0x5D */ u8 unk_0x5D[3];
} Object_586;

/* Battle - Enemy Event */
typedef struct Object_587 {
   /* :0x24 */ s8 timer;
   /* :0x25 */ u8 unk_0x25[3];
   /* :0x28 */ s8 xenoSpawnZ;
   /* :0x29 */ u8 unk_0x29;
   /* :0x2A */ s8 spawnZ;
   /* :0x2B */ s8 spawnX;
   /* :0x2C */ u8 unk_0x2C[52];
} Object_587;

/* Map Object - Boulder */
typedef struct Object_591 {
   /* :0x24 */ s8 hidden;
   /* :0x25 */ u8 unk_0x25[3];
   /* :0x28 */ s16 gfxIdx;
   /* :0x2A */ u8 unk_0x2A[10];
   /* :0x34 */ s16 boxIdx;
   /* :0x36 */ u8 unk_0x36[22];
   /* :0x4C */ s16 dstCamRotY;
   /* :0x4E */ s16 savedCamRotY;
   /* :0x50 */ s8 timer;
   /* :0x51 */ u8 unk_0x51[3];
   /* :0x54 */ s8 gridZ;
   /* :0x55 */ s8 gridX;
   /* :0x56 */ u8 unk_0x56[10];
} Object_591;

/* Battle - Turn Start */
typedef struct Object_592 {
   /* :0x24 */ s8 timer;
   /* :0x25 */ s8 team;
   /* :0x26 */ s8 targets;
   /* :0x27 */ u8 unk_0x27[57];
} Object_592;

/* Battle - Results - Unit (Reward or Penalty) */
typedef struct Object_593 {
   /* :0x24 */ u8 unk_0x24[4];
   /* :0x28 */ s16 gfxIdx;
   /* :0x2A */ u8 unk_0x2A[2];
   /* :0x2C */ s16 clut;
   /* :0x2E */ u8 unk_0x2E[4];
   /* :0x32 */ s16 otOfs;
   /* :0x34 */ u8 unk_0x34[8];
   /* :0x3C */ u8 unitId;
   /* :0x3D */ s8 slot;
   /* :0x3E */ s8 isPenalty;
   /* :0x3F */ u8 unk_0x3F;
   /* :0x40 */ s16 unitGfxIdx;
   /* :0x42 */ s16 unitClut;
   /* :0x44 */ u8 unk_0x44[28];
} Object_593;

/* Status Window Manager */
typedef struct Object_595 {
   /* :0x24 */ struct UnitStatus *unit;
   /* :0x28 */ s8 multiple;
   /* :0x29 */ u8 unk_0x29[55];
} Object_595;

/* Battle - Intro */
typedef struct Object_597 {
   /* :0x24 */ s8 timer;
   /* :0x25 */ u8 unk_0x25[59];
} Object_597;

/* Map 32 - Car Release */
typedef struct Object_650 {
   /* :0x24 */ s16 camOffsetX;
   /* :0x26 */ s16 camOffsetAccum;
   /* :0x28 */ s16 startX;
   /* :0x2A */ u8 unk_0x2A[2];
   /* :0x2C */ s16 carIdx;
   /* :0x2E */ u8 unk_0x2E[2];
   /* :0x30 */ s16 endX;
   /* :0x32 */ u8 unk_0x32[2];
   /* :0x34 */ Camera camera;
   /* :0x42 */ u8 unk_0x42[10];
   /* :0x4C */ s16 tileX;
   /* :0x4E */ s16 tileZ;
   /* :0x50 */ s16 timer;
   /* :0x52 */ u8 unk_0x52[10];
   /* :0x5C */ s16 hasCasualties;
   /* :0x5E */ u8 unk_0x5E[2];
} Object_650;

/* Map 33 - Lava Pit Platform */
typedef struct Object_651 {
   /* :0x24 */ s16 step;
   /* :0x26 */ u8 unk_0x26[14];
   /* :0x34 */ Camera camera;
   /* :0x42 */ u8 unk_0x42[30];
} Object_651;

/* Map 35 - Button */
typedef struct Object_652 {
   /* :0x24 */ u8 unk_0x24[16];
   /* :0x34 */ Camera camera;
   /* :0x42 */ u8 unk_0x42[10];
   /* :0x4C */ s16 tileX;
   /* :0x4E */ s16 tileZ;
   /* :0x50 */ u8 unk_0x50[4];
   /* :0x54 */ s16 smokeTimer;
   /* :0x56 */ u8 unk_0x56[2];
   /* :0x58 */ s16 hitPlayerUnit;
   /* :0x5A */ s16 smokeElapsed;
   /* :0x5C */ struct Object *buttonDepress;
} Object_652;

/* Exploding Tile */
typedef struct Object_653 {
   /* :0x24 */ SVECTOR translation;
   /* :0x2C */ SVECTOR rotation;
   /* :0x34 */ u8 unk_0x34[8];
   /* :0x3C */ SVECTOR savedTranslation;
   /* :0x44 */ u8 unk_0x44[16];
   /* :0x54 */ s16 riverVelocity;
   /* :0x56 */ u8 unk_0x56[2];
   /* :0x58 */ struct MapTileModel *tileModel;
   /* :0x5C */ struct MapTileModel *tileModel_unused;
} Object_653;

/* Map 38 - Wash Away Unit */
typedef struct Object_654 {
   /* :0x24 */ s16 tileX;
   /* :0x26 */ s16 tileZ;
   /* :0x28 */ u8 unk_0x28[52];
   /* :0x5C */ struct Object *unitSprite;
} Object_654;

/* Map 38 - Raise Floodgate */
typedef struct Object_655 {
   /* :0x24 */ s16 waterFaceY;
   /* :0x26 */ u8 unk_0x26[58];
} Object_655;

/* Map 39 */
typedef struct Object_656 {
   /* :0x24 */ u8 unk_0x24[16];
   /* :0x34 */ Camera camera;
   /* :0x42 */ u8 unk_0x42[30];
} Object_656;

/* Map 38 - Floodgate */
typedef struct Object_657 {
   /* :0x24 */ u8 unk_0x24[16];
   /* :0x34 */ Camera camera;
   /* :0x42 */ u8 unk_0x42[22];
   /* :0x58 */ struct Object *casualty;
   /* :0x5C */ struct Object *child; // button or water
} Object_657;

/* Map 38 - Floodwater */
typedef struct Object_658 {
   /* :0x24 */ u8 unk_0x24[4];
   /* :0x28 */ s16 theta;
   /* :0x2A */ u8 unk_0x2A[2];
   /* :0x2C */ s16 timer;
   /* :0x2E */ u8 unk_0x2E[2];
   /* :0x30 */ s16 waveAccum;
   /* :0x32 */ u8 unk_0x32[46];
} Object_658;

/* Map 28 - Button */
typedef struct Object_663 {
   /* :0x24 */ u8 unk_0x24[12];
   /* :0x30 */ Camera camera;
   /* :0x3E */ u8 unk_0x3E[30];
   /* :0x5C */ struct Object *child; // button or door
} Object_663;

/* Map 27 - Buttons */
typedef struct Object_665 {
   /* :0x24 */ s16 buttonPressed[4];
   /* :0x2C */ u8 unk_0x2C[4];
   /* :0x30 */ Camera camera;
   /* :0x3E */ u8 unk_0x3E[30];
   /* :0x5C */ struct Object *child; // button or door
} Object_665;

/* Map 14 - Lower Sand Mound */
typedef struct Object_666 {
   /* :0x24 */ s16 count;
   /* :0x26 */ s16 offset;
   /* :0x28 */ u8 unk_0x28[56];
} Object_666;

/* Map 14 - Raise Sand Mound */
typedef struct Object_668 {
   /* :0x24 */ s16 count;
   /* :0x26 */ s16 offset;
   /* :0x28 */ u8 unk_0x28[56];
} Object_668;

/* Map 14 - Sand */
typedef struct Object_670 {
   /* :0x24 */ u8 unk_0x24[56];
   /* :0x5C */ struct Object *mound; // objf666/objf668
} Object_670;

/* Map 39 - Splashing Tile */
typedef struct Object_672 {
   /* :0x24 */ u8 unk_0x24[56];
   /* :0x5C */ struct MapTileModel *tileModel;
} Object_672;

/* Map 32 - Scene 63 - Cinematic */
typedef struct Object_673 {
   /* :0x24 */ Camera camera;
   /* :0x32 */ u8 unk_0x32[10];
   /* :0x3C */ s16 unused_0x3C;
   /* :0x3E */ u8 unk_0x3E[30];
   /* :0x5C */ struct Object *focus;
} Object_673;

/* Leena's Forcefield */
typedef struct Object_675 {
#ifdef PC_PORT_LP64
   /* PC_PORT LP64 fix (same shape as Object_719): targetSprite is a LEADING pointer at 0x24, whose 8
    * LP64 bytes shift gfxIdx/clut/boxIdx off Object_Sprite's alias. Keep 0x24 a 4-byte gap and move
    * the pointer to the 0x38 animData slot so all aliased fields (gfxIdx/clut/boxIdx/coords) line up. */
   u8 pc_leadPtrGap[4];
#else
   /* :0x24 */ struct Object *targetSprite;
#endif
   /* :0x28 */ s16 gfxIdx;
   /* :0x2A */ u8 unk_0x2A[2];
   /* :0x2C */ s16 clut;
   /* :0x2E */ u8 unk_0x2E[3];
   /* :0x31 */ s8 semiTrans;
   /* :0x32 */ u8 unk_0x32[2];
   /* :0x34 */ s16 boxIdx;
#ifdef PC_PORT_LP64
   /* :0x36 */ u8 unk_0x36[2];
   /* :0x38 */ struct Object *targetSprite;   /* relocated to the animData slot */
#else
   /* :0x36 */ u8 unk_0x36[6];
   PC_PORT_COORDS_ALIAS_PAD4
#endif
   /* :0x3C */ SVectorXZY coords[4];
   /* :0x54 */ u8 unk_0x54[12];
} Object_675;

/* Entity Flasher */
typedef struct Object_679 {
   /* :0x24 */ struct Object *entitySprite;
   /* :0x28 */ s16 timer;
   /* :0x2A */ u8 unk_0x2A[50];
   /* :0x5C */ struct Object *sprite;
} Object_679;

/* Lit Dummy Sprite */
typedef struct Object_680 {
   /* :0x24 */ struct Object *entitySprite;
   /* :0x28 */ u8 unk_0x28[52];
   /* :0x5C */ struct Object *sprite;
} Object_680;

/* Stat Buff FX */
typedef struct Object_681 {
   /* :0x24 */ struct Object *unitSprite;
   /* :0x28 */ s16 spinTheta;
   /* :0x2A */ u8 unk_0x2A[34];
   /* :0x4C */ s16 clut;
   /* :0x4E */ s16 fade;
   /* :0x50 */ s16 phase;
   /* :0x52 */ u8 unk_0x52[14];
} Object_681;

/* FX - TBD */
typedef struct Object_689 {
   /* :0x24 */ struct Object *entitySprite;
   /* :0x28 */ struct Object *sprite;
   /* :0x2C */ SVectorXZY coords[3];
   /* :0x3E */ u8 unk_0x3E[34];
} Object_689;

/* Campfire */
typedef struct Object_692 {
   /* :0x24 */ struct Object *entitySprite;
   /* :0x28 */ u8 unk_0x28[52];
   /* :0x5C */ struct Object *sprite;
} Object_692;

/* FX - TBD */
typedef struct Object_695_696 {
   /* :0x24 */ union {
      struct Object *entitySpriteParam;
      s16 unk;
   } variant_0x24;
   /* :0x28 */ s16 intensity;
   /* :0x2A */ u8 unk_0x2A[50];
   /* :0x5C */ struct Object *entitySprite;
} Object_695_696;

/* Map 43 - Scene 93 - Flame Sphere */
typedef struct Object_697 {
   /* :0x24 */ struct Object *entitySprite;
   /* :0x28 */ u8 unk_0x28[52];
   /* :0x5C */ struct Object *sphere;
} Object_697;

/* Map 61 - Scene 83 - Eleni's Spell - Sparkle Rings */
typedef struct Object_698 {
   /* :0x24 */ struct Object *entitySprite;
   /* :0x28 */ s16 firstLaunchTimer;
   /* :0x2A */ s16 launchNum;
   /* :0x2C */ s16 launchTimer;
   /* :0x2E */ u8 unk_0x2E[46];
   /* :0x5C */ struct Object *sparkleSprite;
} Object_698;

/* FX - TBD */
typedef struct Object_707 {
   /* :0x24 */ u8 unk_0x24[4];
   /* :0x28 */ s16 gfxIdx;
   /* :0x2A */ u8 unk_0x2A[2];
   /* :0x2C */ s16 clut;
   /* :0x2E */ u8 unk_0x2E[6];
   /* :0x34 */ s16 boxIdx;
   /* :0x36 */ u8 unk_0x36[6];
   /* :0x3C */ s16 halfWidth;
   /* :0x3E */ s16 halfHeight;
   /* :0x40 */ u8 unk_0x40[32];
} Object_707;

/* Spellbind - FX2 / FX3 */
typedef struct Object_715_to_718 {
   /* :0x24 */ u8 unk_0x24[2];
   /* :0x26 */ s16 unused_0x26;
   /* :0x28 */ u8 unk_0x28[56];
} Object_715_to_718;

/* Dimensional Rift */
typedef struct Object_719 {
#ifdef PC_PORT_LP64
   /* PC_PORT LP64 fix (see the note at PC_PORT_COORDS_ALIAS_PAD): entitySpriteParam is a LEADING
    * pointer at 0x24 in the retail struct; on LP64 its 8 bytes shift gfxIdx/clut off Object_Sprite's
    * alias, so AddObjPrim4 reads a garbage gfxIdx and the Dimensional Rift renders as white planes.
    * Keep 0x24 a 4-byte gap (gfxIdx stays at 0x28) and relocate the pointer to the 0x38 animData slot
    * like the correct Object_396 -- an 8-byte pointer there shifts coords identically to
    * Object_Sprite, so coords lands at 0x40 with no extra pad. */
   u8 pc_leadPtrGap[4];
#else
   /* :0x24 */ struct Object *entitySpriteParam;
#endif
   /* :0x28 */ s16 gfxIdx;
   /* :0x2A */ u8 unk_0x2A[2];
   /* :0x2C */ s16 clut;
   /* :0x2E */ u8 unk_0x2E[3];
   /* :0x31 */ s8 semiTrans;
#ifdef PC_PORT_LP64
   /* :0x32 */ u8 unk_0x32[6];
   /* :0x38 */ struct Object *entitySpriteParam;   /* relocated to the animData slot */
#else
   /* :0x32 */ u8 unk_0x32[10];
   PC_PORT_COORDS_ALIAS_PAD4
#endif
   /* :0x3C */ SVectorXZY coords[4];
   /* :0x54 */ u8 unk_0x54[4];
   /* :0x58 */ struct Object *entitySprite;
   /* :0x5C */ struct Object *sphere;
} Object_719;

/* Map 61 - Scene 83 - Xeno's Casting Cylinder */
typedef struct Object_720 {
   /* :0x24 */ struct Object *entitySprite;
   /* :0x28 */ u8 unk_0x28[52];
   /* :0x5C */ struct Object *dataStore;
} Object_720;

/* Map 61 - Scene 83 - Xeno's Casting Cylinder - Crest */
typedef struct Object_721 {
   /* :0x24 */ u8 unk_0x24[4];
   /* :0x28 */ s16 gfxIdx;
   /* :0x2A */ u8 unk_0x2A[2];
   /* :0x2C */ s16 clut;
   /* :0x2E */ u8 unk_0x2E[3];
   /* :0x31 */ s8 semiTrans;
   /* :0x32 */ u8 unk_0x32[42];
   /* :0x5C */ struct Object *sprite;
} Object_721;

/* FX - TBD */
typedef struct Object_724 {
   /* :0x24 */ struct Object *entitySprite;
   /* :0x28 */ u8 unk_0x28[52];
   /* :0x5C */ struct Object *sparkles;
} Object_724;

/* Stat Buff Icon */
typedef struct Object_733 {
   /* :0x24 */ u8 unk_0x24[4];
   /* :0x28 */ s16 spinTheta;
   /* :0x2A */ u8 unk_0x2A[34];
   /* :0x4C */ s16 type;
   /* :0x4E */ s16 clut;
   /* :0x50 */ s16 phase;
   /* :0x52 */ u8 unk_0x52[10];
   /* :0x5C */ struct Object *sprite;
} Object_733;

/* FX - TBD */
typedef struct Object_734 {
   /* :0x24 */ struct Object *unitSprite;
   /* :0x28 */ u8 unk_0x28[52];
   /* :0x5C */ struct Object *sprite;
} Object_734;

/* Wyrmfang - Flames */
typedef struct Object_747_748 {
   /* :0x24 */ s16 theta;
   /* :0x26 */ s16 radius;
   /* :0x28 */ s16 amplitude;
   /* :0x2A */ s16 timer;
   /* :0x2C */ s16 amplitudeTheta;
   /* :0x2E */ u8 unk_0x2E[50];
} Object_747_748;

/* Wyrmfang - Flame */
typedef struct Object_749 {
   /* :0x24 */ s16 theta;
   /* :0x26 */ s16 radius;
   /* :0x28 */ s16 yTheta;
   /* :0x2A */ s16 amplitude;
   /* :0x2C */ u8 unk_0x2C[32];
   /* :0x4C */ struct Object *sprite;
   /* :0x50 */ u8 unk_0x50[16];
} Object_749;

/* Map 33 - Lower Platform */
typedef struct Object_750_751 {
   /* :0x24 */ union {
      struct Object *entitySpriteParam;
      s16 unk;
   } variant_0x24;
   /* :0x28 */ s16 ydrop;
   /* :0x2A */ u8 unk_0x2A[54];
} Object_750_751;

/* Map 39 - Scene 82 */
typedef struct Object_754 {
   /* :0x24 */ u8 unk_0x24[16];
   /* :0x34 */ Camera camera;
   /* :0x42 */ u8 unk_0x42[26];
   /* :0x5C */ struct Object *focus;
} Object_754;

/* Elite Melee Sparkles */
typedef struct Object_760 {
   /* :0x24 */ u8 unk_0x24[56];
   /* :0x5C */ struct Object *unitSprite;
} Object_760;

/* Event Fade */
typedef struct Object_795 {
   /* :0x24 */ s16 fade;
   /* :0x26 */ u8 unk_0x26[14];
   /* :0x34 */ s16 delta;
   /* :0x36 */ u8 unk_0x36[14];
   /* :0x44 */ s16 max;
   /* :0x46 */ u8 unk_0x46[26];
} Object_795;

/* Main Menu */
typedef struct Object_796 {
   /* :0x24 */ s32 error;
   /* :0x28 */ s32 numFreeBlocks;
   /* :0x2C */ u8 unk_0x2C[32];
   /* :0x4C */ s16 menuMem_main;
   /* :0x4E */ s16 menuMem_options;
   /* :0x50 */ u8 unk_0x50[16];
} Object_796;

/* Map Object - Generic */
typedef struct Object_MapObject {
   /* :0x24 */ s16 param;
   /* :0x26 */ u8 unk_0x26[2];
   /* :0x28 */ s16 gfxIdx;
   /* :0x2A */ u8 unk_0x2A[10];
   /* :0x34 */ s16 boxIdx;
   /* :0x36 */ u8 unk_0x36[2];
   /* :0x38 */ void *animData;
   /* :0x3C */ u8 unk_0x3C[36];
} Object_MapObject;

/* Entity Spawn - Generic */
// When an event entity spawns an object, it passes it the entity's sprite
typedef struct Object_EntitySpawn {
   /* :0x24 */ struct Object *entitySpriteParam;
   /* :0x28 */ u8 unk_0x28[56];
} Object_EntitySpawn;

typedef struct Object_Unk_8006183c {
   /* :0x24 */ s16 clut;
   /* :0x26 */ s16 scale;
   /* :0x28 */ u8 unk_0x28[52];
   /* :0x5C */ struct Object *unitSprite;
} Object_Unk_8006183c;

typedef struct Object_Unk_80080924 {
   /* :0x24 */ s16 theta;
   /* :0x26 */ u8 unk_0x26[2];
   /* :0x28 */ SVectorXZY position1;
   /* :0x2E */ s16 position7_x;
   /* :0x30 */ SVectorXZY position2;
   /* :0x36 */ s16 position7_z;
   /* :0x38 */ SVectorXZY position3;
   /* :0x3E */ s16 position7_y;
   /* :0x40 */ SVectorXZY position4;
   /* :0x46 */ s16 position8_x;
   /* :0x48 */ SVectorXZY position5;
   /* :0x4E */ s16 position8_z;
   /* :0x50 */ SVectorXZY position6;
   /* :0x56 */ s16 position8_y;
   /* :0x58 */ s16 halfSize;
   /* :0x5A */ u8 unk_0x5A[2];
   /* :0x5C */ struct Object *sprite;
} Object_Unk_80080924;

typedef struct Object_Unk_80087b58 {
   /* :0x24 */ u8 unk_0x24[56];
   /* :0x5C */ struct Object *dataStore; // cylinder
} Object_Unk_80087b58;

typedef struct Object_Unk_8008a364 {
   /* :0x24 */ u8 unk_0x24[56];
   /* :0x5C */ struct Object *sprite;
} Object_Unk_8008a364;

typedef struct Object_Unk_8008d1f0 {
   /* :0x24 */ u8 unk_0x24[4];
   /* :0x28 */ s16 gfxIdx;
   /* :0x2A */ u8 unk_0x2A[2];
   /* :0x2C */ s16 clut;
   /* :0x2E */ u8 unk_0x2E[3];
   /* :0x31 */ s8 semiTrans;
   /* :0x32 */ u8 unk_0x32[2];
   /* :0x34 */ s16 boxIdx;
   /* :0x36 */ u8 unk_0x36[6];
   PC_PORT_COORDS_ALIAS_PAD8
   /* :0x3C */ SVectorXZY coords[4];
   /* :0x54 */ u8 unk_0x54[8];
   /* :0x5C */ struct Object *unitSprite;
} Object_Unk_8008d1f0;

typedef struct Object_Unk_UsedBy392 {
   /* :0x24 */ s32 mode;
   /* :0x28 */ s32 killFlag;
   /* :0x2C */ s16 theta;
   /* :0x2E */ u8 unk_0x2E[50];
} Object_Unk_UsedBy392;

typedef struct Object {
   /* 0x00 */ SVECTOR vec;
   /* 0x08 */ s16 functionIndex;
   /* 0x0A */ s16 mem;
   /* 0x0C */ s16 state;
   /* 0x0E */ s16 state2;
   /* 0x10 */ s16 state3;
   /* 0x12 */ CoordinateValue x1;
   /* 0x14 */ CoordinateValue y1;
   /* 0x16 */ CoordinateValue z1;
   /* 0x18 */ CoordinateValue x2;
   /* 0x1A */ CoordinateValue y2;
   /* 0x1C */ CoordinateValue z2;
   /* 0x1E */ CoordinateValue x3;
   /* 0x20 */ CoordinateValue y3;
   /* 0x22 */ CoordinateValue z3;
   /* 0x24: */
   union {
      s8 bytes[60];
      Object_Sprite sprite;
      Object_Sprite2 sprite2;
      Object_DataStore dataStore;
      Object_003 objf003;         /* Battle - Actions */
      Object_004_005_408 objf004; /* Window */
      Object_004_005_408 objf005; /* Window */
      Object_007 objf007;         /* Apply Poison */
      Object_008 objf008;         /* Battle Portrait */
      Object_009 objf009;         /* Item Icon Manager */
      Object_013 objf013;         /* Battle Manager */
      Object_014 objf014;         /* Battle Unit */
      Object_016 objf016;         /* Choose Done Direction */
      Object_017 objf017;         /* Camera - TBD */
      Object_020 objf020;         /* Pushed Boulder */
      Object_021 objf021;         /* Unit Attacking */
      Object_022_029 objf022;     /* Projectile */
      Object_023 objf023;         /* Camera - Ranged Target */
      Object_024 objf024;         /* Camera - Bounce Zoom */
      Object_025 objf025;         /* Overhead Map View */
      Object_026_588 objf026;     /* Camera - TBD */
      Object_027 objf027;         /* Targeting Spell */
      Object_028 objf028;         /* Unit Casting Spell */
      Object_030 objf030;         /* Unit/Field Info */
      Object_031 objf031;         /* Battle Spells List */
      Object_032_033 objf032;     /* Display Damage */
      Object_MapObject objf035;   /* Map Object - Tree */
      Object_MapObject objf036;   /* Map Object - Grave Marker */
      Object_MapObject objf037;   /* Map Object - Fountain */
      Object_MapObject objf038;   /* Map Object - Lamp Post */
      Object_MapObject objf039;   /* Map Object - Flag */
      Object_040 objf040;         /* Map Object - Chest */
      Object_046 objf046;         /* Map Object - Crate */
      Object_048 objf048;         /* Push */
      Object_050 objf050;         /* Unit Sprites Decoder */
      Object_051 objf051;         /* Floating Damage Text */
      Object_052 objf052;         /* Attack Info Marker */
      Object_059 objf059;         /* Debug - VRAM */
      Object_062 objf062;         /* Stretch Warp Sprite */
      Object_070 objf070;         /* Fade From Black */
      Object_071 objf071;         /* Fade To Black */
      Object_072 objf072;         /* Fade From White */
      Object_073 objf073;         /* Fade To White */
      Object_074 objf074;         /* Fade In Sprite */
      Object_075 objf075;         /* Fade Out Sprite */
      Object_076 objf076;         /* FX - Circles (TBD) */
      Object_077 objf077;         /* FX - Circle (TBD) */
      Object_078 objf078;         /* Damage - FX2 */
      Object_079 objf079;         /* Slay - FX3 */
      Object_080 objf080;         /* Roman Fire - FX1 */
      Object_082 objf082;         /* FX - TBD */
      Object_087 objf087;         /* Map 20 - Scene 30 - Arrow Spawner */
      Object_088 objf088;         /* FX - TBD */
      Object_089 objf089;         /* Map 15 - Scene 17 - Cinematic */
      Object_090 objf090;         /* Dagger Storm - FX2 */
      Object_091 objf091;         /* Dagger Storm - Dagger */
      Object_094 objf094;         /* Mood Ring - FX1 */
      Object_095 objf095;         /* Mood Ring - Ring */
      Object_096 objf096;         /* Mood Ring - FX2 */
      Object_097 objf097;         /* Mood Ring - FX3 */
      Object_098 objf098;         /* Map 20 - Scene 30 - Arrow */
      Object_099 objf099;         /* FX - TBD */
      Object_100 objf100;         /* Healing - FX2 */
      Object_101 objf101;         /* Healing Sparkle */
      Object_102_227 objf102;     /* Poison - FX2 */
      Object_103 objf103;         /* Poison Bubbles */
      Object_104 objf104;         /* Cure - FX2 */
      Object_106 objf106;         /* Magic Charge - FX3 */
      Object_107 objf107;         /* Magic Charge - Glyph Ring */
      Object_108 objf108;         /* Harmful Wave - FX2 */
      Object_109 objf109;         /* Harmful Wave - Ring */
      Object_110 objf110;         /* Casting Stat Buff */
      Object_115 objf115;         /* Faerie - FX2 */
      Object_116 objf116;         /* Faerie - Sparkle */
      Object_117 objf117;         /* Faerie - Sparkle Trail */
      Object_118 objf118;         /* Faerie - Target */
      Object_119 objf119;         /* FX - TBD */
      Object_122 objf122;         /* Dark Fire - FX1 */
      Object_149 objf149;         /* Flashing Unit Sprite */
      Object_130 objf130;         /* FX - TBD */
      Object_131 objf131;         /* Slay Unit */
      Object_132_Etc objf132;     /* FX - TBD */
      Object_133_Etc objf133;     /* FX - TBD */
      Object_132_Etc objf134;     /* FX - TBD */
      Object_132_Etc objf136;     /* FX - TBD */
      Object_133_Etc objf137;     /* FX - TBD */
      Object_132_Etc objf140;     /* FX - TBD */
      Object_133_Etc objf141;     /* FX - TBD */
      Object_144 objf144;         /* Holy Lightning - FX2 */
      Object_145 objf145;         /* Holy Lightning - Electric Orb */
      Object_147 objf147;         /* FX - TBD */
      Object_148 objf148;         /* FX - TBD */
      Object_155 objf155;         /* Spellbind - Glyph */
      Object_156 objf156;         /* Delta Mirage - FX1 */
      Object_157 objf157;         /* Delta Mirage - Ray */
      Object_158 objf158;         /* Explosion - FX1 */
      Object_159 objf159;         /* Explosion - Rays */
      Object_160 objf160;         /* Ice Storm - Target */
      Object_161 objf161;         /* Piercing Light - FX1 */
      Object_162 objf162;         /* Ice Storm - Splash */
      Object_163 objf163;         /* Stone Shower - FX1 */
      Object_164 objf164;         /* Stone Shower - Rock */
      Object_165 objf165;         /* Stone Shower - FX2 */
      Object_167 objf167;         /* Rainbow Swirl */
      Object_168 objf168;         /* Rainbow Stroke - Rainbow Swirl */
      Object_169 objf169;         /* Evil Stream - FX1 */
      Object_170 objf170;         /* Piercing Ray, Plasma Wave - FX1 */
      Object_171 objf171;         /* Homing Ray */
      Object_172 objf172;         /* Holy Pressure - Cube */
      Object_173 objf173;         /* Fire Gem - Beam */
      Object_175 objf175;         /* Rainbow Storm - FX1 */
      Object_176 objf176;         /* Rainbow Stroke - FX1 */
      Object_177 objf177;         /* Holy Pressure - FX1 */
      Object_178 objf178;         /* Thunder Flash - FX1 */
      Object_179 objf179;         /* Thunder Flash - Ray */
      Object_180 objf180;         /* Spread Force - FX1 */
      Object_182 objf182;         /* FX - TBD */
      Object_183 objf183;         /* FX - TBD */
      Object_184 objf184;         /* Avalanche - FX1 */
      Object_185 objf185;         /* Avalanche - Rock */
      Object_186 objf186;         /* FX - TBD */
      Object_188 objf188;         /* Dark Fire - Ray */
      Object_189 objf189;         /* Ice Storm - FX1 */
      Object_191 objf191;         /* Magic Arrow - Arrow */
      Object_192 objf192;         /* Perfect Guard - FX2 */
      Object_193 objf193;         /* Dark Star - FX3 */
      Object_195 objf195;         /* Rolling Thunder - FX2 */
      Object_196 objf196;         /* Rolling Thunder - Orb Pair */
      Object_197 objf197;         /* Rolling Thunder - FX1 */
      Object_198 objf198;         /* Rolling Thunder - Casting Bolt */
      Object_199 objf199;         /* Magic Arrow - FX1 */
      Object_201 objf201;         /* Unit Struck */
      Object_202_746 objf202;     /* Unit Blocking */
      Object_203 objf203;         /* Blocking Impact */
      Object_204 objf204;         /* Summon Crest */
      Object_205 objf205;         /* Blood Spurt */
      Object_206 objf206;         /* Blood Spurt Particle */
      Object_208 objf208;         /* Holy Lightning - FX1 */
      Object_211 objf211;         /* Avalanche - Boulder */
      Object_212 objf212;         /* Holy Lightning - Casting Bolt */
      Object_213 objf213;         /* Dust Cloud Spawner */
      Object_214 objf214;         /* Dust Cloud */
      Object_215 objf215;         /* Cloud (Sand, Dust, etc.) */
      Object_220 objf220;         /* Explosion - FX2 */
      Object_222 objf222;         /* FX - TBD */
      Object_223 objf223;         /* FX - TBD */
      Object_224 objf224;         /* Thunder Ball - FX1 */
      Object_225 objf225;         /* Thunder Ball - Initial Orb */
      Object_226 objf226;         /* Thunder Ball - Child Orb */
      Object_256_258 objf256;     /* Map 36 - Leena */
      Object_265_266_729 objf265; /* Shrink & Warp Sprite */
      Object_269 objf269;         /* FX - TBD */
      Object_270 objf270;         /* FX - TBD */
      Object_271 objf271;         /* Map 36 - Scene 74 - Leena Casting Shield */
      Object_272 objf272;         /* Stat Raised */
      Object_273 objf273;         /* Outward Ray */
      Object_275 objf275;         /* FX - TBD */
      Object_276 objf276;         /* FX - TBD */
      Object_277 objf277;         /* Zoom - TBD */
      Object_278 objf278;         /* Faint Sparkles */
      Object_279 objf279;         /* Ice Storm - Camera */
      Object_280 objf280;         /* Dark Hurricane - Target */
      Object_281_282 objf281;     /* Dark Hurricane - FX2 / FX3 */
      Object_283 objf283;         /* Dark Hurricane - Cloud */
      Object_284 objf284;         /* FX - TBD */
      Object_287 objf287;         /* FX - TBD */
      Object_288 objf288;         /* Map 13 - Bridge Explosion (Battle) */
      Object_290_294_761 objf290; /* Reveal Chest Item */
      Object_290_294_761 objf294; /* Reveal Item */
      Object_292 objf292;         /* Blue Item Sparkles */
      Object_303 objf303;         /* Map 31 - Scene 61 - Xeno's Flames */
      Object_304_661 objf304;     /* Flame */
      Object_305_328 objf305;     /* Magic Stone - FX (Map 43 - Scene 93 / Map 44 - Scene 00) */
      Object_306_Etc objf306;     /* Healing - FX2 */
      Object_307_324 objf307;     /* Evil Stream - FX2 / FX3 */
      Object_310 objf310;         /* Life Orb - Beam */
      Object_312 objf312;         /* Bubble Swirl - Bubble */
      Object_313 objf313;         /* Life Orb - FX1 */
      Object_314 objf314;         /* Inward Ray */
      Object_315 objf315;         /* Vertical Ray */
      Object_317_338 objf317;     /* Avalanche - FX2 / FX3 */
      Object_319 objf319;         /* Map 67 - Scene 34 - TBD */
      Object_321 objf321;         /* FX - TBD */
      Object_322_Etc objf322;     /* Magic Restoration (Magic Charge, etc.) - FX2 */
      Object_323_713 objf323;     /* FX - TBD */
      Object_326 objf326;         /* FX - TBD */
      Object_327 objf327;         /* Healing Circle - FX2 */
      Object_330 objf330;         /* Magic Restoration (Magic Charge, etc.) - FX1 */
      Object_331 objf331;         /* FX - TBD */
      Object_332 objf332;         /* Rolling Fire - FX1 */
      Object_334 objf334;         /* Salamander - FX1 */
      Object_335_336 objf335;     /* Salamander - Head */
      Object_335_336 objf336;     /* Salamander - Segment */
      Object_132_Etc objf344;     /* Roman Fire - FX2 / FX3 */
      Object_341_342_353 objf341; /* File Save Menu */
      Object_343_Etc objf343;     /* File Load Menu */
      Object_346 objf346;         /* Button Depress */
      Object_347 objf347;         /* Map 26 */
      Object_348 objf348;         /* FX - TBD */
      Object_351 objf351;         /* MsgBox Text */
      Object_352 objf352;         /* Map 26 */
      Object_355_356 objf355;     /* Map 19 - Elevator (TBD) */
      Object_355_356 objf356;     /* Map 19 - Elevator (TBD) */
      Object_357 objf357;         /* Map 19 */
      Object_358 objf358;         /* Map 19 - Elevator (TBD) */
      Object_361 objf361;         /* Map 13 - Bridge Explosion (Scene) */
      Object_362 objf362;         /* Drawbridge Button */
      Object_363 objf363;         /* Wyrmfang - FX1 */
      Object_365 objf365;         /* Map 17 - Floodgate */
      Object_366 objf366;         /* Map 17 - Button */
      Object_369 objf369;         /* Screen Effect */
      Object_377 objf377;         /* FX - TBD */
      Object_378 objf378;         /* Phase Shift - FX1 */
      Object_379 objf379;         /* Evil Stream - Rock */
      Object_380 objf380;         /* Level Up - FX */
      Object_382 objf382;         /* Flame Breath */
      Object_386 objf386;         /* Healing Sparkle */
      Object_388 objf388;         /* Dark Hurricane - FX1 */
      Object_389 objf389;         /* Dark Hurricane - Vortex */
      Object_390 objf390;         /* Dark Hurricane - Vortex Layer */
      Object_392 objf392;         /* FX - TBD */
      Object_393 objf393;         /* Map 44 - Scene 00 - Explosion Rays */
      Object_394 objf394;         /* Dynamo Hum - FX1 */
      Object_395 objf395;         /* Dynamo Hum - Electric Orb */
      Object_399 objf399;         /* Map 11 */
      Object_396 objf396;         /* Dynamo Hum - Orb Electricity */
      Object_400 objf400;         /* AI - TBD */
      Object_401 objf401;         /* AI - TBD */
      Object_402_Etc objf402;     /* AI - TBD */
      Object_402_Etc objf403;     /* AI - TBD */
      Object_402_Etc objf404;     /* AI - TBD */
      Object_405 objf405;         /* Panorama */
      Object_406 objf406;         /* Shop or Depot */
      Object_409 objf409;         /* Event Entity */
      Object_410 objf410;         /* Camera - Event Zoom */
      Object_413 objf413;         /* MsgBox Portrait */
      Object_MapObject objf415;   /* Map Object - Torch */
      Object_420_423 objf420;     /* Battle Victory/Defeat */
      Object_421_422 objf421;     /* MsgBox Tail (Upper) */
      Object_424 objf424;         /* Battle Ender */
      Object_425 objf425;         /* Battle - Options */
      Object_438 objf438;         /* Evaluate Battle 08 */
      Object_446 objf446;         /* Battle - Victory/Defeat Particle */
      Object_447 objf447;         /* Unit Portrait (in depot, dojo, etc.) */
      Object_564_565_566 objf564; /* Map Object - Rippling Water */
      Object_530 objf530;         /* Map 61 - Scene 83 - Vandal Heart Forcefield */
      Object_567 objf567;         /* Opening Chest */
      Object_571 objf571;         /* Level Up - Camera Control, Sound */
      Object_573 objf573;         /* Battle Items List */
      Object_575 objf575;         /* Status Portrait */
      Object_576 objf576;         /* Tavern */
      Object_577 objf577;         /* Dynamic Icon */
      Object_578 objf578;         /* Dojo */
      Object_579 objf579;         /* World Map */
      Object_580 objf580;         /* Town */
      Object_581 objf581;         /* Audio Command */
      Object_585 objf585;         /* Battle - Player Event */
      Object_586 objf586;         /* Battle - MsgBox */
      Object_587 objf587;         /* Battle - Enemy Event */
      Object_026_588 objf588;     /* Camera - TBD */
      Object_402_Etc objf589;     /* AI - TBD */
      Object_591 objf591;         /* Map Object - Boulder */
      Object_592 objf592;         /* Battle - Turn Start */
      Object_593 objf593;         /* Battle - Results - Unit (Reward or Penalty) */
      Object_595 objf595;         /* Status Window Manager */
      Object_597 objf597;         /* Battle - Intro */
      Object_650 objf650;         /* Map 32 - Car Release */
      Object_651 objf651;         /* Map 33 - Lava Pit Platform */
      Object_652 objf652;         /* Map 35 - Button */
      Object_653 objf653;         /* Exploding Tile */
      Object_654 objf654;         /* Map 38 - Wash Away Unit */
      Object_655 objf655;         /* Map 38 - Raise Floodgate */
      Object_656 objf656;         /* Map 39 */
      Object_657 objf657;         /* Map 38 - Floodgate */
      Object_658 objf658;         /* Map 38 - Floodwater */
      Object_663 objf663;         /* Map 28 - Button */
      Object_665 objf665;         /* Map 27 - Buttons */
      Object_666 objf666;         /* Map 14 - Lower Sand Mound */
      Object_668 objf668;         /* Map 14 - Raise Sand Mound */
      Object_670 objf670;         /* Map 14 - Sand */
      Object_672 objf672;         /* Map 39 - Splashing Tile */
      Object_673 objf673;         /* Map 32 - Scene 63 - Cinematic */
      Object_675 objf675;         /* Leena's Forcefield */
      Object_679 objf679;         /* Entity Flasher */
      Object_680 objf680;         /* Lit Dummy Sprite */
      Object_681 objf681;         /* Stat Buff FX */
      Object_689 objf689;         /* FX - TBD */
      Object_692 objf692;         /* Campfire */
      Object_695_696 objf695;     /* FX - TBD */
      Object_697 objf697;         /* Map 43 - Scene 93 - Flame Sphere */
      Object_698 objf698;         /* Map 61 - Scene 83 - Eleni's Spell - Sparkle Rings */
      Object_707 objf707;         /* FX - TBD */
      Object_719 objf719;         /* Dimensional Rift */
      Object_720 objf720;         /* Map 61 - Scene 83 - Xeno's Casting Cylinder */
      Object_721 objf721;         /* Map 61 - Scene 83 - Xeno's Casting Cylinder - Crest */
      Object_724 objf724;         /* FX - TBD */
      Object_715_to_718 objf715;  /* Spellbind - FX2 / FX3 */
      Object_733 objf733;         /* Stat Buff Icon */
      Object_734 objf734;         /* FX - TBD */
      Object_747_748 objf747;     /* Wyrmfang - Flames */
      Object_749 objf749;         /* Wyrmfang - Flame */
      Object_750_751 objf750;     /* Map 33 - Lower Platform */
      Object_754 objf754;         /* Map 39 - Scene 82 */
      Object_760 objf760;         /* Elite Melee Sparkles */
      Object_290_294_761 objf761; /* Reveal Used Item */
      Object_795 objf795;         /* Event Fade */
      Object_796 objf796;         /* Main Menu */
      Object_133_Etc objf801;     /* FX - TBD */
      Object_133_Etc objf802;     /* FX - TBD */
      Object_133_Etc objf803;     /* FX - TBD */
      Object_MapObject mapObj;    /* Map Object - Generic */
      Object_EntitySpawn entitySpawn;
      Object_Unk_8006183c objfUnk8006183c;
      Object_Unk_80080924 objfUnk80080924;
      Object_Unk_80087b58 objfUnk80087b58;
      Object_Unk_8008a364 objfUnk8008a364;
      Object_Unk_8008d1f0 objfUnk8008d1f0;
      Object_Unk_UsedBy392 objfUnkUsedBy392;
   } d;
} Object;

typedef void (*ObjFunction)(Object *obj);

extern Object gObjectArray[OBJ_DATA_CT];
extern ObjFunction gObjFunctionPointers[804];
extern Object gUnitSpritesDecoder; // objf050
extern Object *gTempGfxObj;
extern Object *gTempObj;
extern Object *gScreenFade;

void Obj_ResetByFunction(s16);
Object *Obj_FindByFunction(s16);
void Obj_Execute(void);
void Obj_ResetFromIdx10(void);
void Obj_ResetAll(void);
Object *Obj_GetUnused(void);
Object *Obj_GetFirstUnused(void);
Object *Obj_GetLastUnused(void);
Object *Obj_GetLastUnusedSkippingTail(s32);
s32 Obj_CountUnused(void);

Object *CreatePositionedObj(Object *, s16);
Object *SnapToUnit(Object *);

#endif
