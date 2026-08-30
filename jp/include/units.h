#ifndef UNITS_H
#define UNITS_H

#include "common.h"

typedef enum SpellIdx {
   SPELL_NULL = 0,
   SPELL_FAERIE_LIGHT = 1,
   SPELL_ICE_STORM = 2,
   SPELL_ROLLING_FIRE = 3,
   SPELL_FAERIE_STAR = 4,
   SPELL_DELTA_MIRAGE = 5,
   SPELL_DARK_STAR = 6,
   SPELL_SPELLBIND = 7,
   SPELL_PIERCING_RAY = 8,
   SPELL_ENVENOM = 9,
   SPELL_PHASE_SHIFT = 10,
   SPELL_ROMAN_FIRE = 11,
   SPELL_POISON_CLOUD = 12,
   SPELL_SPREAD_FORCE = 13,
   SPELL_AVALANCHE = 14,
   SPELL_SALAMANDER = 15,
   SPELL_HEALING = 16,
   SPELL_MYSTIC_SHIELD = 17,
   SPELL_CURE = 18,
   SPELL_HEALING_PLUS = 19,
   SPELL_BLESS_WEAPON = 20,
   SPELL_HOLY_LIGHTNING = 21,
   SPELL_ULTRA_HEALING = 22,
   SPELL_MAGIC_CHARGE = 23,
   SPELL_HOLY_PRESSURE = 24,
   SPELL_SUPREME_HEALING = 25,
   SPELL_STONE_SHOWER = 26,
   SPELL_CURE_WIDE = 27,
   SPELL_HEALING_CIRCLE = 28,
   SPELL_PERFECT_GUARD = 29,
   SPELL_THUNDER_FLASH = 30,
   SPELL_HEALING_WAVE = 31,
   SPELL_MYSTIC_ENERGY = 32,
   SPELL_SELF_HEALING = 33,
   SPELL_HEALING_2 = 34,
   SPELL_EXTRA_HEALING = 35,
   SPELL_HYPER_HEALING = 36,
   SPELL_HEALING_CIRCLE_2 = 37,
   SPELL_PIERCING_LIGHT = 38,
   SPELL_RAINBOW_STROKE = 39,
   SPELL_MAGIC_ARROW = 40,
   SPELL_THUNDER_BALL = 41,
   SPELL_DARK_HURRICANE = 42,
   SPELL_RAINBOW_STORM = 43,
   SPELL_DAGGER_STORM = 44,
   SPELL_PLASMA_WAVE = 45,
   SPELL_DARK_FIRE = 46,
   SPELL_EXPLOSION = 47,
   SPELL_DYNAMO_HUM = 48,
   SPELL_HERB = 49,
   SPELL_MEGAHERB = 50,
   SPELL_ELIXIR = 51,
   SPELL_MAGE_OIL = 52,
   SPELL_MAGE_GEM = 53,
   SPELL_LIFE_ORB = 54,
   SPELL_HOLY_H2O = 55,
   SPELL_FIRE_GEM = 56,
   SPELL_MOODRING = 57,
   SPELL_AURA_GEM = 58,
   SPELL_WYRMFANG = 59,
   SPELL_ROLLING_THUNDER = 60,
   SPELL_HARMFUL_WAVE = 61,
   SPELL_EVIL_STREAM = 62,
   SPELL_MAD_BOOK = 63,
   SPELL_MUSHROOM = 64,
   SPELL_MOON_PIE = 65,
   SPELL_IRONBOOT = 66,
   SPELL_UNICORN = 67,
   SPELL_KINGFOIL = 68,
   SPELL_HELSTONE = 69,
   SPELL_SHIVBOOK = 70,
   SPELL_NCKLACE = 71
} SpellIdx;

typedef enum ItemIdx {
   ITEM_NULL = 0,
   ITEM_HERB = 1,
   ITEM_MEGAHERB = 2,
   ITEM_MAGE_OIL = 3,
   ITEM_MAGE_GEM = 4,
   ITEM_ELIXIR = 5,
   ITEM_HOLY_H2O = 6,
   ITEM_LIFE_ORB = 7,
   ITEM_FIRE_GEM = 8,
   ITEM_MOODRING = 9,
   ITEM_AURA_GEM = 10,
   ITEM_WYRMFANG = 11,
   ITEM_S_SWORD = 12,
   ITEM_L_SWORD = 13,
   ITEM_I_SWORD = 14,
   ITEM_G_SWORD = 15,
   ITEM_M_SWORD = 16,
   ITEM_CALIBURN = 17,
   ITEM_V_HEART = 18,
   ITEM_H_BOW = 19,
   ITEM_L_BOW = 20,
   ITEM_IRON_BOW = 21,
   ITEM_STL_BOW = 22,
   ITEM_KILL_BOW = 23,
   ITEM_GRT_BOW = 24,
   ITEM_RUNE_BOW = 25,
   ITEM_STAFF = 26,
   ITEM_L_STAFF = 27,
   ITEM_I_STAFF = 28,
   ITEM_M_STAFF = 29,
   ITEM_S_STAFF = 30,
   ITEM_ARKSTAFF = 31,
   ITEM_RUNEWAND = 32,
   ITEM_IRON_AXE = 33,
   ITEM_BATTLEAX = 34,
   ITEM_BLOODAXE = 35,
   ITEM_GRT_AXE = 36,
   ITEM_RAGNAROK = 37,
   ITEM_I_SPEAR = 38,
   ITEM_S_LANCE = 39,
   ITEM_HALBERD = 40,
   ITEM_D_SPEAR = 41,
   ITEM_IRONCLAW = 42,
   ITEM_STL_CLAW = 43,
   ITEM_P_CLAWS = 44,
   ITEM_D_CLAWS = 45,
   ITEM_LTHRHELM = 46,
   ITEM_LGHTHELM = 47,
   ITEM_I_HELM = 48,
   ITEM_STL_MASK = 49,
   ITEM_GRIMHELM = 50,
   ITEM_DRGNHELM = 51,
   ITEM_BANDANNA = 52,
   ITEM_L_BAND = 53,
   ITEM_IRON_CAP = 54,
   ITEM_STL_BAND = 55,
   ITEM_L_MASK = 56,
   ITEM_R_CROWN = 57,
   ITEM_LEATHER = 58,
   ITEM_PLEATHER = 59,
   ITEM_CHAIN = 60,
   ITEM_SCALE = 61,
   ITEM_PLATE = 62,
   ITEM_KEVLAR = 63,
   ITEM_CAPE = 64,
   ITEM_WOOLROBE = 65,
   ITEM_MSTRROBE = 66,
   ITEM_MYSTROBE = 67,
   ITEM_ZEN_ROBE = 68,
   ITEM_MOJOROBE = 69,
   ITEM_NOVA_KEY = 70,
   ITEM_EARTH_K_ = 71,
   ITEM_MANA_KEY = 72,
   ITEM_CHAOS_K_ = 73,
   ITEM_LOGOS_K_ = 74,
   ITEM_HEAVEN_K = 75,
   ITEM_NOVA_P_ = 76,
   ITEM_ICE_P_ = 77,
   ITEM_MANA_P_ = 78,
   ITEM_CHAOS_P_ = 79,
   ITEM_LOGO_P_ = 80,
   ITEM_HEAVEN_P = 81,
   ITEM_V_HELM = 82,
   ITEM_V_ARMOR = 83,
   ITEM_V_HEART_2 = 84,
   ITEM_GOLDCOIN = 85,
   ITEM_MITHRIL = 86,
   ITEM_GOLD_AXE = 87,
   ITEM_MAD_BOOK = 88,
   ITEM_MUSHROOM = 89,
   ITEM_MOON_PIE = 90,
   ITEM_IRONBOOT = 91,
   ITEM_UNICORN = 92,
   ITEM_KINGFOIL = 93,
   ITEM_HELSTONE = 94,
   ITEM_SHIVBOOK = 95,
   ITEM_NCKLACE = 96,
   ITEM_BANANA = 97,
   ITEM_MACROMAN = 98,
   ITEM_TAROT = 99,
   ITEM_RAMEN = 100,
   ITEM_THIEF_SWORD = 101,
   ITEM_THIEF_HELM = 102,
   ITEM_THIEF_MAIL = 103,
   ITEM_IMP_PANTS = 104,
   ITEM_LARGE_SPEAR = 105,
   ITEM_CAPTAIN_SUIT = 106,
   ITEM_RED_BANDANNA = 107,
   ITEM_PIRATE_SUIT = 108,
   ITEM_FARM_TOOL = 109,
   ITEM_CLOTHES = 110,
   ITEM_DARK_HELM = 111,
   ITEM_MAD_ARMOR = 112,
   ITEM_DEATH_MASK = 113,
   ITEM_LORD_ARMOR = 114,
   ITEM_EMPIRE_HELM = 115,
   ITEM_EMPIRE_MAIL = 116,
   ITEM_BOMB = 117,
   ITEM_EMPIRE_HOOD = 118,
   ITEM_EMPIRE_ROBE = 119,
   ITEM_COMMAND_HELM = 120,
   ITEM_COMMANDARMOR = 121,
   ITEM_CRIMSON_BAND = 122,
   ITEM_CRIMSON_MAIL = 123,
   ITEM_GUARD_HELM = 124,
   ITEM_GUARD_MAIL = 125,
   ITEM_VORPAL_CHAIN = 126,
   ITEM_COMMAND_HELM_2 = 127,
   ITEM_CRIMSON_HOOD = 128,
   ITEM_CRIMSON_CAPE = 129,
   ITEM_BLACK_HELM = 130,
   ITEM_BLACK_ARMOR = 131,
   ITEM_R_CROWN_2 = 132,
   ITEM_DOOM_SWORD = 133,
   ITEM_DOOM_CROWN = 134,
   ITEM_DOOM_MAIL = 135,
   ITEM_SAINT_BUSTER = 136,
   ITEM_HOLY_HELM = 137,
   ITEM_VALHALA_MAIL = 138,
} ItemIdx;

typedef enum UnitNameIdx {
   UNIT_NULL = 0,
   UNIT_ASH = 1,
   UNIT_CLINT = 2,
   UNIT_DIEGO = 3,
   UNIT_ELENI = 4,
   UNIT_HUXLEY = 5,
   UNIT_KIRA = 6,
   UNIT_GROG = 7,
   UNIT_DOLAN = 8,
   UNIT_AMON = 9,
   UNIT_SARA = 10,
   UNIT_ZOHAR = 11,
   UNIT_DARIUS = 12,
   UNIT_END_OF_PARTY = 12,
   UNIT_ZOOT = 13,
   UNIT_HASSAN = 15,
   UNIT_MAGNUS = 17,
   UNIT_LANDO = 19,
   UNIT_DUMAS = 20,
   UNIT_KANE = 21,
   UNIT_DALLAS = 22,
   UNIT_KURTZ = 23,
   UNIT_SABINA = 24,
   UNIT_XENO = 27, /* ?: Not in gCharacterNames? */
   UNIT_DOLF = 29,
   UNIT_LEENA = 30,
   UNIT_CLIVE = 31,
   UNIT_KIRA_2 = 34,
   UNIT_INVALID = 255
} UnitNameIdx;

/* Partial; defining as needed */
typedef enum UnitId {
   UNIT_ID_ELENI_MAGE = 4,
   UNIT_ID_DIEGO_BOWMAN = 15,
   UNIT_ID_KIRA_BOWMAN = 18,
   UNIT_ID_AMON_BOWMAN = 21,
   UNIT_ID_ZOHAR_SORCEROR = 23,
   UNIT_ID_DARIUS_BOWMAN = 24,
   UNIT_ID_DIEGO_SNIPER = 39,
   UNIT_ID_KIRA_SNIPER = 42,
   UNIT_ID_AMON_SNIPER = 45,
   UNIT_ID_DARIUS_SNIPER = 48,
   UNIT_ID_ASH_VANDALIER = 49,
   UNIT_ID_DARIUS_SKY_LORD = 60,
   UNIT_ID_END_OF_PARTY = 60,
   UNIT_ID_EGGWORM = 63,
   UNIT_ID_GOLD_GOLEM = 64,
   UNIT_ID_MIMIC = 66,
   UNIT_ID_BUGABOO = 75,
   UNIT_ID_EVILSTATUE = 77,
   UNIT_ID_LANDO = 90,
   UNIT_ID_SABINA = 91,
   UNIT_ID_MAGE_TOWER = 93,
   UNIT_ID_JUGGERNAUT = 94,
   UNIT_ID_DEATH_DEV = 95,
   UNIT_ID_DARK_MAGE = 96,
   UNIT_ID_DUMAS = 106,
   UNIT_ID_DOLF_DARK_ANGEL = 109,
   UNIT_ID_GRENADIER = 111,
   UNIT_ID_CLAY_GOLEM = 113,
   UNIT_ID_DARK_GOLEM = 122,
   UNIT_ID_SALAMANDER = 125,
   UNIT_ID_GUARDIAN = 126,
   UNIT_ID_BASILISK = 127,
   UNIT_ID_MEGA_GUARD = 128,
   UNIT_ID_M_CANNON = 129,
   UNIT_ID_BAHAMUT = 130,
   UNIT_ID_LEENA = 132,
} UnitId;

typedef enum UnitTeam {
   TEAM_NULL = 0,
   TEAM_PLAYER = 1,
   TEAM_ENEMY = 2,
   TEAM_NEUTRAL = 100, //?
   TEAM_CHEST = 101,
   TEAM_BOULDER = 102
} UnitTeam;

typedef enum UnitType {
   UNIT_TYPE_NULL = 0,
   UNIT_TYPE_HERO = 1,
   UNIT_TYPE_CHAMPION = 2,
   UNIT_TYPE_PARAGON = 3,
   UNIT_TYPE_SOLDIER = 4,
   UNIT_TYPE_SWORDSMAN = 5,
   UNIT_TYPE_DUELIST = 6,
   UNIT_TYPE_GUARDSMAN = 7,
   UNIT_TYPE_DRAGOON = 8,
   UNIT_TYPE_ARCHER = 9,
   UNIT_TYPE_BOWMAN = 10,
   UNIT_TYPE_SNIPER = 11,
   UNIT_TYPE_HAWKNIGHT = 12,
   UNIT_TYPE_SKY_LORD = 13,
   UNIT_TYPE_MAGE = 14,
   UNIT_TYPE_SORCERER = 15,
   UNIT_TYPE_ENCHANTER = 16,
   UNIT_TYPE_MONK = 17,
   UNIT_TYPE_NINJA = 18,
   UNIT_TYPE_HEALER = 19,
   UNIT_TYPE_BISHOP = 20,
   UNIT_TYPE_ARCHBISHOP = 21,
   UNIT_TYPE_THIEF_LORD = 22,
   UNIT_TYPE_BRIGAND = 23,
   UNIT_TYPE_HUNTER = 24,
   UNIT_TYPE_HUNTERIMP = 25,
   UNIT_TYPE_WAR_GHOST = 26,
   UNIT_TYPE_CLAY_GOLEM = 27,
   UNIT_TYPE_HELL_BAT = 28,
   UNIT_TYPE_DEATH_ANT = 29,
   UNIT_TYPE_ANT_ARM = 30,
   UNIT_TYPE_MADCAPTAIN = 31,
   UNIT_TYPE_EVILSTATUE = 32,
   UNIT_TYPE_VILLAGER = 33,
   UNIT_TYPE_MADSOLDIER = 34,
   UNIT_TYPE_GUARDDOG = 35,
   UNIT_TYPE_SHOOTERIMP = 36,
   UNIT_TYPE_DARK_GOLEM = 37,
   UNIT_TYPE_BLOODGHOST = 38,
   UNIT_TYPE_DARK_LORD = 39,
   UNIT_TYPE_SPARKIE = 40,
   UNIT_TYPE_CENTURION = 41,
   UNIT_TYPE_GRENADIER = 42,
   UNIT_TYPE_BLOOD_BAT = 43,
   UNIT_TYPE_DEACON = 44,
   UNIT_TYPE_HITMAN = 45,
   UNIT_TYPE_C_ARCHER = 46,
   UNIT_TYPE_JUGGERNAUT = 47,
   UNIT_TYPE_MIMIC = 48,
   UNIT_TYPE_JAILMASTER = 49,
   UNIT_TYPE_ASSASSIN = 50,
   UNIT_TYPE_GREATARMOR = 51,
   UNIT_TYPE_WAR_MASTER = 52,
   UNIT_TYPE_VALKYRIE = 53,
   UNIT_TYPE_C_SHOOTER = 54,
   UNIT_TYPE_C_KNIGHT = 55,
   UNIT_TYPE_C_HAWK = 56,
   UNIT_TYPE_BUGABOO = 57,
   UNIT_TYPE_WIGHT = 58,
   UNIT_TYPE_SKELETON = 59,
   UNIT_TYPE_EGGWORM = 60,
   UNIT_TYPE_GOLD_GOLEM = 61,
   UNIT_TYPE_ACID_GHOST = 62,
   UNIT_TYPE_DEATHANGEL = 63,
   UNIT_TYPE_C_WARLOCK = 64,
   UNIT_TYPE_C_PRIEST = 65,
   UNIT_TYPE_SALAMANDER = 66,
   UNIT_TYPE_GUARDIAN = 67,
   UNIT_TYPE_BASILISK = 68,
   UNIT_TYPE_BLK_KNIGHT = 69,
   UNIT_TYPE_XENO = 70,
   UNIT_TYPE_HELLKNIGHT = 71,
   UNIT_TYPE_BAHAMUT = 72,
   UNIT_TYPE_DARK_MAGE = 73,
   UNIT_TYPE_DOOM_LORD = 74,
   UNIT_TYPE_DARK_ANGEL = 75,
   UNIT_TYPE_MAGE_TOWER = 76,
   UNIT_TYPE_DEATH_DEV_ = 77,
   UNIT_TYPE_VANDALIER = 78,
   UNIT_TYPE_NPC = 79,
   UNIT_TYPE_BUCCANEER = 80,
   UNIT_TYPE_CORSAIR = 81,
   UNIT_TYPE_WARLOCK = 82,
   UNIT_TYPE_C_ARMOR = 83,
   UNIT_TYPE_MEGA_GUARD = 84,
   UNIT_TYPE_M_CANNON = 85
} UnitType;

typedef enum UnitClass {
   CLASS_KNIGHT = 0,
   CLASS_ARMOR = 1,
   CLASS_MONK = 2,
   CLASS_ARCHER = 3,
   CLASS_AIRMAN = 4,
   CLASS_MAGE = 5,
   CLASS_PRIEST = 6,
   CLASS_7 = 7,
   CLASS_VANDALIER = 8
} UnitClass;

typedef struct UnitStatus {
   u8 idx;
   s8 level;
   s8 team;
   u8 field3_0x3;
   struct Object *battler;
   struct Object *sprite;
   BigInt experience;
   u8 field7_0x1c;
   u8 field8_0x1d;
   u8 expMulti;
   u8 field10_0x1f;
   s16 hpFrac;
   u8 field12_0x22;
   u8 field13_0x23;
   u32 atkVar10000;
   u32 defVar10000;
   u32 agiVar10000;
   u16 hp;
   u16 maxHp;
   u16 attack;
   u16 defense;
   u16 agility;
   u16 exp;
   u16 mp;
   u16 maxMp;
   u16 hpVar100;
   u16 atkVar100;
   u16 defVar100;
   u16 agiVar100;
   s16 direction;
   u8 stripIdx;
   u8 spells[10];
   u8 done;
   u8 field33_0x56;
   u8 field34_0x57;
   s16 items[2];
   s16 droppedItem;
   s16 helmet;
   s16 armor;
   s16 weapon;
   s16 unitId; // ?: 0..143
   u8 travelRange;
   u8 attackRange;
   u8 name;
   u8 unitType;  // See: {@sym 800eb050}
   u8 advantage; // See: {@sym 800f16e4}
   u8 class;
   u8 step;
   u8 magicSusceptibility;
   u8 ailmentSusceptibility;
   u8 hideMarker;
   u8 poisoned;
   u8 paralyzed;
   u8 atkBoosted;
   u8 defBoosted;
   u8 aglBoosted;
   u8 tileX;
   u8 tileZ;
   u8 field59_0x77;
} UnitStatus;

typedef struct PartyMember {
   u8 inParty;
   u8 advChosePathB;
   u8 advLevelFirst;
   u8 advLevelSecond;
   u8 helmet;
   u8 armor;
   u8 weapon;
   u8 items[2];
   u8 field9_0x9;
   BigInt experience;
   s16 hpVar100;
   s16 atkVar100;
   s16 defVar100;
   s16 agiVar100;
} PartyMember;

typedef struct UnitInfo {
   s8 name;
   s8 unitType;
   s8 advantage;
   s8 class;
   s8 step;
   s8 magicSusceptibility;
   s8 ailmentSusceptibility;
   s8 attackRange;
   u8 weapon;
   u8 helmet;
   u8 armor;
   u8 heldItem;
   u8 maxMp;
   s8 spell1;
   s8 spell2;
} UnitInfo;

typedef enum SpellArea { SPELL_AREA_NULL = 0, SPELL_AREA_SINGLE = 1, SPELL_AREA_AOE = 2 } SpellArea;

typedef enum SpellTargeting {
   SPELL_TARGET_ENEMY = 0,
   SPELL_TARGET_ALLY = 1,
   SPELL_TARGET_ENEMY_GROUP = 3,
   SPELL_TARGET_ALLY_GROUP = 4
} SpellTargeting;

typedef enum SpellEffect {
   SPELL_EFFECT_DAMAGE = 0,
   SPELL_EFFECT_RESTORE_HP = 1,
   SPELL_EFFECT_RESTORE_HP_UNK = 2,
   SPELL_EFFECT_POISON = 3,
   SPELL_EFFECT_PARALYZE = 4,
   SPELL_EFFECT_SUPPORT_OFS = 5,
   SPELL_EFFECT_BOOST_ATK = 5,
   SPELL_EFFECT_BOOST_DEF = 6,
   SPELL_EFFECT_BOOST_ATK_AND_DEF = 7,
   SPELL_EFFECT_CURE_AILMENTS = 8,
   SPELL_EFFECT_BOOST_AGL = 9,
   SPELL_EFFECT_RESTORE_MP = 10,
   SPELL_EFFECT_TRANSFER_MP = 11
} SpellEffect;

typedef struct Spell {
   u8 area;
   u8 targeting;
   u8 range;
   u8 fieldSize;
   u8 power;
   u8 mpCost;
} Spell;

/* An unused speculative alternate layout for Spell, kept only because gSpellsEx below still
 * references its name in a commented-out extern. */

enum {
   SPELL_EX_OBJF_MAIN = 0,
   SPELL_EX_OBJF_TARGET = 1,
   SPELL_EX_OBJF_DEFEAT = 2,
   SPELL_EX_MP_BONUS = 3,
   SPELL_EX_EFFECT = 4
};

enum { AILMENT_POISON = 0, AILMENT_PARAYLYZE = 1 };

typedef enum InventoryCategory {
   INVENTORY_WEAPON = 0,
   INVENTORY_ARMOR = 1,
   INVENTORY_ITEM = 2,
} InventoryCategory;

typedef enum InventorySlot {
   INVENTORY_SLOT_WEAPON = 0,
   INVENTORY_SLOT_HELMET = 1,
   INVENTORY_SLOT_ARMOR = 2,
   INVENTORY_SLOT_ITEM1 = 3,
   INVENTORY_SLOT_ITEM2 = 4,
} InventorySlot;

typedef enum EquipmentType {
   EQUIPMENT_TYPE_SWORD = 0,
   EQUIPMENT_TYPE_BOW = 1,
   EQUIPMENT_TYPE_STAFF = 2,
   EQUIPMENT_TYPE_AXE = 3,
   EQUIPMENT_TYPE_SPEAR = 4,
   EQUIPMENT_TYPE_CLAWS = 5,
   EQUIPMENT_TYPE_V_HEART = 6,
   EQUIPMENT_TYPE_HELM = 7,
   EQUIPMENT_TYPE_BAND = 8,
   EQUIPMENT_TYPE_V_HELM = 9,
   EQUIPMENT_TYPE_ARMOR = 10,
   EQUIPMENT_TYPE_ROBE = 11,
   EQUIPMENT_TYPE_V_ARMOR = 12,
   EQUIPMENT_TYPE_ITEM = 13,
} EquipmentType;

extern s8 gCharacterNames[35][7];
extern s8 gUnitTypeNames[80][11];

extern s8 gItemNames[139][9];
extern u8 gItemNamesSjis[101][17];
extern s8 *gItemDescriptions[101];
extern s16 gItemCosts[101];
extern s8 gItemEquipmentDisplayPower[104];
extern u8 gItemEquipmentPower[104];
extern u8 gItemEquipmentTypes[101]; // See: EquipmentType
extern u8 gItemSpells[104];
extern u8 gEquipmentTypeClassCapability[14][9];

extern Spell gSpells[80];
extern s8 gSpellNames[72][21];
extern s8 *gSpellDescriptions[72];
extern s16 gSpellSounds[72];
extern s16 gSpellSounds2[72];
// extern SpellEx gSpellsEx[73];
extern s16 gSpellsEx[73][5];
extern s32 gSpellLists[PARTY_CT][2][10];
extern u8 gSpellLevelRequirement[36];

extern UnitStatus gUnits[UNIT_CT];
extern UnitInfo gUnitInfo[UNIT_DB_CT];
extern PartyMember gPartyMembers[PARTY_CT];
extern BigInt gExperienceLevels[];
extern u8 gInitialLevels[8][12];
extern u8 gPartyDefaultLevels[PARTY_CT][BATTLE_CT];
extern void *gUnitDataPtr;
extern s16 gCurrentUnitSet[20];
extern s16 gSceneUnitSets[105][20];
extern s32 gPartyUnitIds[PARTY_CT][2][3];
extern u8 gCurrentParty[16]; // List of party indices
extern u8 gPartyListBuffer[232];

extern s16 gUnitPortraitIds[UNIT_DB_CT];

extern s16 gUnitSounds_Attacking1[UNIT_DB_CT][2];
extern s16 gUnitSounds_Attacking2[UNIT_DB_CT][2];
extern u8 gUnitSoundDelays_Attacking1[UNIT_DB_CT][2];
extern u8 gUnitSoundDelays_Attacking2[UNIT_DB_CT][2];
extern s16 gUnitSounds_Struck[UNIT_DB_CT];
extern s16 gUnitSounds_Blocking[UNIT_DB_CT];

extern s16 gClassBaseAttack[10];
extern s16 gClassBaseDefense[10];
extern s16 gClassBaseMaxHp[10];
extern s16 gClassBaseAgility[10];
extern u16 gClassBlockChance[10];
extern u8 gClassMpMultiplier[8];
extern u8 gClassDefaultLoadouts[9][3][BATTLE_CT];

u8 GetItemNameLength(u8);

void ClearUnits(void);
UnitStatus *CreateUnitInNextSlot(void);
UnitStatus *CreateUnitInLastSlot(void);
UnitStatus *CreateUnit(u8);

#endif
