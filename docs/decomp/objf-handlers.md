# Object-function handlers (`gObjFunctionPointers`)

Generated from `src/core/obj_function_pointers.c`, the retail data tables (`gSpellsEx`, `gSceneLoaders`, `gBattleEvaluator`, `sMapObjects_*`) and a parse of all 93 retail EVDATA event scripts -- do not edit by hand, regenerate with `tools/gen_decomp_docs.py`.

Every object in the game runs one handler per frame, selected by `obj->functionIndex` through this 804-slot table (`src/core/obj_function_pointers.c`). One function may serve several indices; the aliases are visible in the name (`Objf004_005_408_Window` serves slots 4, 5 and 408). The "reached via" column says how retail selects the index:

- **spell (id:slot)** -- a `gSpellsEx` dispatch slot; M/T/D = MAIN/TARGET/DEFEAT (see [spell-fx-dispatch.md](spell-fx-dispatch.md))
- **event 0x1d** -- spawned by index from a retail EVDATA event script
- **scene loader / evaluator / map table** -- the `gSceneLoaders`, `gBattleEvaluator` or `sMapObjects_*` data tables
- **code** -- a C spawn site sets `functionIndex` to this handler's `OBJF_` constant
- **internal/child** -- only ever spawned by its parent handler (no external entry)
- **cut content** -- reachable by nothing on the retail disc (suffix `_Unused`)

| idx | handler | file | reached via |
|---:|---|---|---|
| 1 | `Objf001_Noop` | `src/core/object.c` | internal/child |
| 2 | `Objf002_MenuChoice` | `src/ui/window.c` | code |
| 3 | `Objf003_BattleActions` | `src/battle/field.c` | code |
| 4 | `Objf004_005_408_Window` | `src/ui/window.c` | code |
| 5 | `Objf004_005_408_Window` | `src/ui/window.c` | code |
| 6 | `Objf006_Logo` | `src/core/main.c` | code |
| 7 | `Objf007_ApplyPoison` | `src/battle/executors.c` | code |
| 8 | `Objf008_BattlePortrait` | `src/units/roster.c` | code |
| 9 | `Objf009_ItemIconMgr` | `src/ui/supplies.c` | code |
| 13 | `Objf013_BattleMgr` | `src/battle/field.c` | code |
| 14 | `Objf014_BattleUnit` | `src/units/actor.c` | code |
| 15 | `Objf015_TargetingAttack` | `src/battle/executors.c` | code |
| 16 | `Objf016_ChooseDoneDirection` | `src/battle/presentation.c` | code |
| 17 | `Objf017_AttackCamera` | `src/battle/presentation.c` | code |
| 19 | `Objf019_Compass` | `src/core/engine.c` | code |
| 20 | `Objf020_PushedBoulder` | `src/maps/objects.c` | code |
| 21 | `Objf021_UnitAttacking` | `src/battle/executors.c` | code |
| 22 | `Objf022_029_Projectile` | `src/battle/projectile.c` | code |
| 23 | `Objf023_Camera_RangedTarget` | `src/battle/projectile.c` | code |
| 24 | `Objf024_BounceZoom` | `src/battle/presentation.c` | code |
| 25 | `Objf025_OverheadMapView` | `src/core/engine.c` | code |
| 26 | `Objf026_588_FocusCamera` | `src/battle/presentation.c` | code |
| 27 | `Objf027_TargetingSpell` | `src/battle/spell_targeting.c` | code |
| 28 | `Objf028_UnitCasting` | `src/battle/executors.c` | code |
| 29 | `Objf022_029_Projectile` | `src/battle/projectile.c` | code |
| 30 | `Objf030_FieldInfo` | `src/battle/field.c` | code |
| 31 | `Objf031_BattleSpellsList` | `src/ui/window.c` | code |
| 32 | `Objf032_033_DisplayDamage` | `src/units/roster.c` | code |
| 33 | `Objf032_033_DisplayDamage` | `src/units/roster.c` | code |
| 35 | `Objf035_MapObject_Tree` | `src/maps/objects.c` | map table, code |
| 36 | `Objf036_MapObject_GraveMarker` | `src/maps/objects.c` | map table, code |
| 37 | `Objf037_MapObject_Fountain` | `src/maps/objects.c` | map table, code |
| 38 | `Objf038_MapObject_LampPost` | `src/maps/objects.c` | map table, code |
| 39 | `Objf039_MapObject_Flag` | `src/maps/objects.c` | map table, code |
| 40 | `Objf040_MapObject_Chest` | `src/maps/objects.c` | map table, code |
| 42 | `Objf042_MapObject_FlowingSand` | `src/core/animated_textures.c` | map table, code |
| 43 | `Objf043_SetupMapObjects` | `src/maps/setup_objects.c` | code |
| 45 | `Objf045_BloodSpurtParticleOffset` | `src/maps/objects.c` | code |
| 46 | `Objf046_MapObject_Crate` | `src/maps/objects.c` | map table, code |
| 47 | `Objf047_BattleMapCursor` | `src/core/engine.c` | code |
| 48 | `Objf048_Push` | `src/maps/objects.c` | code |
| 49 | `Objf049_BattleMapCursorControl` | `src/core/engine.c` | code |
| 50 | `Objf050_UnitSpritesDecoder` | `src/core/graphics.c` | code |
| 51 | `Objf051_FloatingDamageText` | `src/core/graphics.c` | code |
| 52 | `Objf052_AttackInfoMarker` | `src/core/graphics.c` | code |
| 59 | `Objf059_DebugVram` | `src/core/graphics.c` | internal/child |
| 60 | `Objf060_Healing_FX1` | `src/spells/support_magic.c` | spell (1:M, 4:M, 16:M, 17:M, …) |
| 62 | `Objf062_StretchWarpSprite` | `src/spells/hit_effects.c` | code |
| 70 | `Objf070_FadeFromBlack` | `src/spells/hit_effects.c` | internal/child |
| 71 | `Objf071_FadeToBlack` | `src/spells/hit_effects.c` | internal/child |
| 72 | `Objf072_FadeFromWhite` | `src/spells/hit_effects.c` | internal/child |
| 73 | `Objf073_FadeToWhite` | `src/spells/hit_effects.c` | internal/child |
| 74 | `Objf074_FadeInSprite` | `src/spells/hit_effects.c` | internal/child |
| 75 | `Objf075_FadeOutSprite` | `src/spells/hit_effects.c` | internal/child |
| 76 | `Objf076_RingBurstOnUnit` | `src/spells/hit_effects.c` | internal/child |
| 77 | `Objf077_ExpandingRing` | `src/spells/hit_effects.c` | code |
| 78 | `Objf078_Damage_FX2` | `src/spells/hit_effects.c` | spell (2:T, 8:T, 10:T, 13:T, …), code |
| 79 | `Objf079_Slay_FX3` | `src/spells/hit_effects.c` | spell (2:D, 8:D, 10:D, 13:D, …), code |
| 80 | `Objf080_RomanFire_FX1` | `src/spells/dark_fire.c` | spell (11:M) |
| 81 | `Objf081_RomanFire_Flame` | `src/spells/dark_fire.c` | code |
| 82 | `Objf082_OrbitingEmberPair_Unused` | `src/spells/dagger_storm.c` | cut content |
| 83 | `Objf083_HomingExplosionSpark_Unused` | `src/spells/dagger_storm.c` | cut content |
| 84 | `Objf084_Avalanche_DustCloud` | `src/spells/avalanche.c` | code |
| 85 | `Objf085_Map13_ExplosionPillar` | `src/maps/map_13_15.c` | code |
| 86 | `Objf086_Map15_HullSplash` | `src/maps/map_13_15.c` | code |
| 87 | `Objf087_Map20_Scn30_ArrowSpawner` | `src/spells/shared_fx.c` | event 0x1d |
| 88 | `Objf088_SparkParticle` | `src/events/fx_scenes.c` | code |
| 89 | `Objf089_Map15_Scn17_Cinematic` | `src/maps/map_13_15.c` | event 0x1d |
| 90 | `Objf090_DaggerStorm_FX2` | `src/spells/dagger_storm.c` | spell (44:T, 70:T), code |
| 91 | `Objf091_DaggerStorm_Dagger` | `src/spells/dagger_storm.c` | code |
| 92 | `Objf092_DaggerStorm_FX1` | `src/spells/dagger_storm.c` | spell (44:M, 70:M) |
| 93 | `Objf093_DaggerStorm_FX3` | `src/spells/dagger_storm.c` | spell (44:D, 70:D) |
| 94 | `Objf094_MoodRing_FX1` | `src/spells/mood_ring.c` | spell (57:M) |
| 95 | `Objf095_MoodRing_Ring` | `src/spells/mood_ring.c` | code |
| 96 | `Objf096_MoodRing_FX2` | `src/spells/mood_ring.c` | spell (57:T), code |
| 97 | `Objf097_MoodRing_FX3` | `src/spells/mood_ring.c` | spell (57:D) |
| 98 | `Objf098_Map20_Scn30_Arrow` | `src/spells/shared_fx.c` | code |
| 99 | `Objf099_StreakParticle` | `src/events/fx_scenes.c` | code |
| 100 | `Objf100_Healing_FX2` | `src/spells/support_magic.c` | spell (16:T, 33:T, 34:T, 49:T, …), code |
| 101 | `Objf101_HealingSparkle` | `src/spells/support_magic.c` | code |
| 102 | `Objf102_227_Poison_FX2` | `src/spells/support_magic.c` | spell (9:T, 12:T, 64:T), code |
| 103 | `Objf103_Poison_Bubbles` | `src/spells/support_magic.c` | code |
| 104 | `Objf104_Cure_FX2` | `src/spells/support_magic.c` | spell (18:T, 18:D, 27:T, 27:D, …) |
| 106 | `Objf106_MagicCharge_FX3` | `src/spells/support_magic.c` | spell (23:D) |
| 107 | `Objf107_MagicCharge_GlyphRing` | `src/spells/support_magic.c` | code |
| 108 | `Objf108_HarmfulWave_FX2` | `src/spells/support_magic.c` | spell (61:T, 61:D) |
| 109 | `Objf109_HarmfulWave_Ring` | `src/spells/support_magic.c` | code |
| 110 | `Objf110_CastingStatBuff` | `src/spells/support_magic.c` | spell (20:D), code |
| 111 | `Objf111_BlessWeapon_FX2` | `src/spells/support_magic.c` | spell (20:T) |
| 112 | `Objf112_MysticShield_FX2` | `src/spells/support_magic.c` | spell (17:T) |
| 113 | `Objf113_MysticEnergy_FX2` | `src/spells/support_magic.c` | spell (32:T) |
| 115 | `Objf115_Faerie_FX2` | `src/spells/faerie.c` | spell (1:T, 4:T) |
| 116 | `Objf116_Faerie_Sparkle` | `src/spells/faerie.c` | code |
| 117 | `Objf117_Faerie_SparkleTrail` | `src/spells/faerie.c` | code |
| 118 | `Objf118_Faerie_Target` | `src/spells/faerie.c` | code |
| 119 | `Objf119_RadialFxSprite` | `src/spells/hit_effects.c` | code |
| 120 | `Objf120_ThunderStrikeDamage_Unused` | `src/spells/lightning.c` | cut content |
| 121 | `Objf121_ThunderStrikeSlay_Unused` | `src/spells/lightning.c` | cut content |
| 122 | `Objf122_DarkFire_FX1` | `src/spells/dark_fire.c` | spell (46:M) |
| 128 | `Objf128_ThunderBall_FX2` | `src/spells/casting_main.c` | spell (41:T, 69:T) |
| 129 | `Objf129_ThunderBall_FX3` | `src/spells/casting_main.c` | spell (41:D, 69:D) |
| 130 | `Objf130_TorusSweepOnUnit` | `src/spells/hit_effects.c` | internal/child |
| 131 | `Objf131_SlayUnit` | `src/spells/hit_effects.c` | code |
| 132 | `Objf132_EngulfUnit` | `src/spells/hit_effects.c` | spell (3:T, 15:T, 46:T, 56:T, …), code |
| 133 | `Objf133_FlameRingEmitter` | `src/spells/hit_effects.c` | code |
| 134 | `Objf132_EngulfUnit` | `src/spells/hit_effects.c` | spell (3:D, 15:D, 46:D, 56:D, …), code |
| 136 | `Objf132_EngulfUnit` | `src/spells/hit_effects.c` | spell (48:T), code |
| 137 | `Objf137_LightningRingEmitter` | `src/spells/hit_effects.c` | code |
| 138 | `Objf132_EngulfUnit` | `src/spells/hit_effects.c` | spell (48:D), code |
| 140 | `Objf132_EngulfUnit` | `src/spells/hit_effects.c` | code |
| 141 | `Objf141_ExplosionRingEmitter` | `src/spells/hit_effects.c` | code |
| 142 | `Objf132_EngulfUnit` | `src/spells/hit_effects.c` | code |
| 144 | `Objf144_HolyLightning_FX2` | `src/spells/lightning.c` | spell (21:T), code |
| 145 | `Objf145_HolyLightning_ElectricOrb` | `src/spells/lightning.c` | code |
| 146 | `Objf146_HolyLightning_FX3` | `src/spells/lightning.c` | spell (21:D) |
| 147 | `Objf147_LightningBolt` | `src/spells/lightning_bolt.c` | code |
| 148 | `Objf148_ThunderStrike` | `src/spells/casting_main.c` | code |
| 149 | `Objf149_FlashingUnitSprite` | `src/spells/hit_effects.c` | code |
| 151 | `Objf151_FireGem_FX1` | `src/spells/casting_main.c` | spell (56:M) |
| 155 | `Objf155_Spellbind_Glyph` | `src/spells/casting_main.c` | code |
| 156 | `Objf156_DeltaMirage_FX1` | `src/spells/delta_mirage.c` | spell (5:M) |
| 157 | `Objf157_DeltaMirage_Ray` | `src/spells/delta_mirage.c` | code |
| 158 | `Objf158_Explosion_FX1` | `src/spells/explosion.c` | spell (47:M) |
| 159 | `Objf159_Explosion_Rays` | `src/spells/explosion.c` | code |
| 160 | `Objf160_IceStorm_Target` | `src/spells/casting_main.c` | code |
| 161 | `Objf161_PiercingLight_FX1` | `src/spells/casting_main.c` | spell (38:M) |
| 162 | `Objf162_IceStorm_Splash` | `src/spells/casting_main.c` | code |
| 163 | `Objf163_StoneShower_FX1` | `src/spells/stone_shower.c` | spell (26:M) |
| 164 | `Objf164_StoneShower_Rock` | `src/spells/stone_shower.c` | code |
| 165 | `Objf165_StoneShower_FX2` | `src/spells/stone_shower.c` | spell (26:T), code |
| 166 | `Objf166_StoneShower_FX3` | `src/spells/stone_shower.c` | spell (26:D) |
| 167 | `Objf167_RainbowSwirl` | `src/spells/casting_main.c` | code |
| 168 | `Objf168_RainbowStroke_RainbowSwirl` | `src/spells/casting_main.c` | code |
| 169 | `Objf169_EvilStream_FX1` | `src/spells/casting_main.c` | spell (62:M) |
| 170 | `Objf170_PiercingRay_Etc_FX1` | `src/spells/casting_main.c` | spell (8:M, 45:M, 58:M) |
| 171 | `Objf171_HomingRay` | `src/spells/casting_main.c` | code |
| 172 | `Objf172_HolyPressure_Cube` | `src/spells/casting_main.c` | code |
| 173 | `Objf173_FireGem_Beam` | `src/spells/fire_gem.c` | code |
| 175 | `Objf175_RainbowStorm_FX1` | `src/spells/casting_main.c` | spell (43:M, 67:M) |
| 176 | `Objf176_RainbowStroke_FX1` | `src/spells/casting_main.c` | spell (39:M) |
| 177 | `Objf177_HolyPressure_FX1` | `src/spells/casting_main.c` | spell (24:M) |
| 178 | `Objf178_ThunderFlash_FX1` | `src/spells/casting_main.c` | spell (30:M) |
| 179 | `Objf179_ThunderFlash_Ray` | `src/spells/casting_main.c` | code |
| 180 | `Objf180_SpreadForce_FX1` | `src/spells/casting_main.c` | spell (13:M) |
| 181 | `Objf181_Spellbind_FX1` | `src/spells/casting_main.c` | spell (7:M, 63:M) |
| 182 | `Objf182_SpikedBallToss_Unused` | `src/spells/casting_main.c` | cut content |
| 183 | `Objf183_SpikedBall_Unused` | `src/spells/casting_main.c` | code |
| 184 | `Objf184_Avalanche_FX1` | `src/spells/casting_main.c` | spell (14:M) |
| 185 | `Objf185_Avalanche_Rock` | `src/spells/casting_main.c` | code |
| 186 | `Objf186_ExplosionStrike_Unused` | `src/spells/casting_main.c` | code |
| 187 | `Objf187_ExplosionStrikeSlay_Unused` | `src/spells/casting_main.c` | cut content |
| 188 | `Objf188_DarkFire_Ray` | `src/spells/dark_fire.c` | code |
| 189 | `Objf189_IceStorm_FX1` | `src/spells/casting_main.c` | spell (2:M) |
| 190 | `Objf190_MagicArrowSingle_Unused` | `src/spells/casting_main.c` | cut content |
| 191 | `Objf191_MagicArrow_Arrow` | `src/spells/casting_main.c` | code |
| 192 | `Objf192_PerfectGuard_FX2` | `src/spells/perfect_guard.c` | spell (29:T, 29:D, 66:T, 66:D) |
| 193 | `Objf193_DarkStar_FX2` | `src/spells/dark_star.c` | spell (6:T), code |
| 194 | `Objf194_DarkStar_FX3` | `src/spells/dark_star.c` | spell (6:D) |
| 195 | `Objf195_RollingThunder_FX2` | `src/spells/lightning.c` | spell (60:T), code |
| 196 | `Objf196_RollingThunder_OrbPair` | `src/spells/lightning.c` | code |
| 197 | `Objf197_RollingThunder_FX1` | `src/spells/summon_crest.c` | spell (60:M), code |
| 198 | `Objf198_RollingThunder_CastingBolt` | `src/spells/summon_crest.c` | code |
| 199 | `Objf199_MagicArrow_FX1` | `src/spells/casting_main.c` | spell (40:M) |
| 200 | `Objf200_RollingThunder_FX3` | `src/spells/lightning.c` | spell (60:D) |
| 201 | `Objf201_UnitStruck` | `src/spells/hit_effects.c` | code |
| 202 | `Objf202_746_UnitBlocking` | `src/spells/hit_effects.c` | code |
| 203 | `Objf203_BlockingImpactParticle` | `src/spells/hit_effects.c` | code |
| 204 | `Objf204_SummonCrest` | `src/spells/summon_crest.c` | code |
| 205 | `Objf205_BloodSpurt` | `src/spells/hit_effects.c` | code |
| 206 | `Objf206_686_BloodSpurtParticle` | `src/spells/hit_effects.c` | code |
| 207 | `Objf207_SummonRedCrest` | `src/spells/summon_crest.c` | spell (6:M) |
| 208 | `Objf208_HolyLightning_FX1` | `src/spells/summon_crest.c` | spell (21:M) |
| 209 | `Objf209_SummonBlueCrest` | `src/spells/summon_crest.c` | spell (29:M, 66:M) |
| 210 | `Objf210_SummonGreenCrest` | `src/spells/summon_crest.c` | spell (9:M, 12:M, 61:M, 64:M) |
| 211 | `Objf211_Avalanche_Boulder` | `src/spells/faerie.c` | code |
| 212 | `Objf212_HolyLightning_CastingBolt` | `src/spells/summon_crest.c` | code |
| 213 | `Objf213_DustCloudSpawner` | `src/spells/hit_effects.c` | code |
| 214 | `Objf214_DustCloud` | `src/spells/hit_effects.c` | code |
| 215 | `Objf215_Cloud` | `src/spells/hit_effects.c` | code |
| 220 | `Objf220_Explosion_FX2` | `src/spells/explosion.c` | spell (47:T), code |
| 221 | `Objf221_Explosion_FX3` | `src/spells/explosion.c` | spell (47:D) |
| 222 | `Objf222_EnergyBallLob_Unused` | `src/spells/lightning.c` | cut content |
| 223 | `Objf223_EnergyBall_Unused` | `src/spells/lightning.c` | code |
| 224 | `Objf224_ThunderBall_FX1` | `src/spells/lightning.c` | spell (41:M, 69:M) |
| 225 | `Objf225_ThunderBall_InitialOrb` | `src/spells/lightning.c` | code |
| 226 | `Objf226_ThunderBall_ChildOrb` | `src/spells/lightning.c` | code |
| 227 | `Objf102_227_Poison_FX2` | `src/spells/support_magic.c` | spell (9:D, 12:D, 64:D) |
| 250 | `Objf250_350_LoadEvent16` | `src/events/scene_loaders.c` | scene loader |
| 251 | `Objf251_LoadEvent14` | `src/events/scene_loaders.c` | scene loader |
| 252 | `Objf252_LoadEvent05` | `src/events/scene_loaders.c` | scene loader |
| 253 | `Objf253_SpawnGraveMarker` | `src/events/fx_scenes.c` | event 0x1d |
| 254 | `Objf254_LoadEvent23` | `src/events/scene_loaders.c` | scene loader |
| 255 | `Objf255_LoadEvent33` | `src/events/scene_loaders.c` | scene loader |
| 256 | `Objf256_258_Map36_LeenaAndVillager` | `src/events/fx_scenes.c` | code |
| 257 | `Objf257_LoadEvent37` | `src/events/scene_loaders.c` | scene loader |
| 258 | `Objf256_258_Map36_LeenaAndVillager` | `src/events/fx_scenes.c` | scene loader, code |
| 259 | `Objf259_LoadEvent44` | `src/events/scene_loaders.c` | scene loader |
| 260 | `Objf260_LoadEvent42` | `src/events/scene_loaders.c` | scene loader |
| 262 | `Objf262_LoadEvent51` | `src/events/scene_loaders.c` | scene loader |
| 263 | `Objf263_LoadEvent54` | `src/events/scene_loaders.c` | scene loader |
| 264 | `Objf264_LoadEvent57` | `src/events/scene_loaders.c` | scene loader |
| 265 | `Objf265_266_729_ShrinkWarpSprite` | `src/events/fx_scenes.c` | event 0x1d, scene loader, code |
| 266 | `Objf265_266_729_ShrinkWarpSprite` | `src/events/fx_scenes.c` | event 0x1d, scene loader, code |
| 267 | `Objf267_LoadEvent62` | `src/events/scene_loaders.c` | scene loader |
| 268 | `Objf268_LoadEvent67` | `src/events/scene_loaders.c` | scene loader |
| 269 | `Objf269_LightningTendril_Unused` | `src/events/fx_scenes.c` | code |
| 270 | `Objf270_LightningPillar_Unused` | `src/events/fx_scenes.c` | cut content |
| 271 | `Objf271_Map36_Scn74_LeenaCastingShield` | `src/events/fx_scenes.c` | event 0x1d |
| 272 | `Objf272_StatRaised` | `src/spells/shared_fx.c` | code |
| 273 | `Objf273_OutwardRay` | `src/events/fx_scenes.c` | code |
| 274 | `Objf274_Noop` | `src/spells/avalanche.c` | internal/child |
| 275 | `Objf275_ConvergingExplosion_Unused` | `src/events/fx_scenes.c` | cut content |
| 276 | `Objf276_RedFlameDome_Unused` | `src/events/fx_scenes.c` | cut content |
| 277 | `Objf277_Zoom` | `src/events/fx_helpers.c` | code |
| 278 | `Objf278_FaintSparkles` | `src/spells/shared_fx.c` | code |
| 279 | `Objf279_IceStorm_Camera` | `src/events/fx_helpers.c` | code |
| 280 | `Objf280_DarkHurricane_Target` | `src/spells/dark_hurricane.c` | code |
| 281 | `Objf281_282_DarkHurricane_FX2_FX3` | `src/spells/dark_hurricane.c` | spell (42:T, 71:T), code |
| 282 | `Objf281_282_DarkHurricane_FX2_FX3` | `src/spells/dark_hurricane.c` | spell (42:D, 71:D), code |
| 283 | `Objf283_DarkHurricane_Cloud` | `src/spells/dark_hurricane.c` | code |
| 284 | `Objf284_ConvergingSparkle_Unused` | `src/spells/dark_hurricane.c` | cut content |
| 285 | `Objf285_CastingFx` | `src/spells/dark_hurricane.c` | code |
| 286 | `Objf286_ShrinkingGroundArc_Unused` | `src/spells/dark_hurricane.c` | cut content |
| 287 | `Objf287_CastingFxSpawner_Unused` | `src/spells/dark_hurricane.c` | cut content |
| 288 | `Objf288_Map13_BridgeExplosion_Battle` | `src/maps/map_13_15.c` | code |
| 289 | `Objf289_ChimneySmokeRing` | `src/maps/ambience.c` | code |
| 290 | `Objf290_294_761_RevealItem` | `src/battle/item_reveal.c` | code |
| 291 | `Objf291_ChestImpact` | `src/spells/dark_fire.c` | code |
| 292 | `Objf292_BlueItemSparkle` | `src/battle/item_reveal.c` | code |
| 293 | `Objf293_Sparkle_Unused` | `src/battle/item_reveal.c` | cut content |
| 294 | `Objf290_294_761_RevealItem` | `src/battle/item_reveal.c` | code |
| 295 | `Objf295_Smoke` | `src/battle/item_reveal.c` | code |
| 296 | `Objf296_Map17_SprayParticle` | `src/maps/map_17_19.c` | code |
| 297 | `Objf297_SplashDroplets` | `src/maps/map_13_15.c` | code |
| 298 | `Objf298_SplashWithDroplets` | `src/maps/map_13_15.c` | code |
| 299 | `Objf299_Map15_Ocean` | `src/maps/map_13_15.c` | code |
| 300 | `Objf300_Map32_Smokestack` | `src/battle/item_reveal.c` | code |
| 301 | `Objf301_Map32_SmokestackParticle` | `src/battle/item_reveal.c` | code |
| 302 | `Objf302_ChimneySmoke` | `src/maps/ambience.c` | code |
| 303 | `Objf303_Map31_Scn61_XenoFlames` | `src/maps/map_28_31.c` | event 0x1d |
| 304 | `Objf304_661_Flame` | `src/maps/map_28_31.c` | code |
| 305 | `Objf305_328_MagicStoneFx` | `src/events/fx_scenes.c` | event 0x1d |
| 306 | `Objf306_791_792_793_Healing_FX2` | `src/spells/healing_items.c` | spell (35:T, 36:T), code |
| 307 | `Objf307_324_EvilStream_FX2_FX3` | `src/spells/evil_stream.c` | spell (62:D), code |
| 309 | `Objf309_Explosion` | `src/spells/dagger_storm.c` | code |
| 310 | `Objf310_LifeOrb_Beam` | `src/spells/life_orb.c` | code |
| 311 | `Objf311_BubbleSwirl` | `src/spells/life_orb.c` | internal/child |
| 312 | `Objf312_BubbleSwirl_Bubble` | `src/spells/life_orb.c` | code |
| 313 | `Objf313_LifeOrb_FX1` | `src/spells/life_orb.c` | spell (54:M) |
| 314 | `Objf314_InwardRay` | `src/spells/life_orb.c` | code |
| 315 | `Objf315_VerticalRay` | `src/spells/life_orb.c` | code |
| 316 | `Objf316_Noop` | `src/core/screen_effects.c` | internal/child |
| 317 | `Objf317_338_Avalanche_FX2_FX3` | `src/spells/avalanche.c` | spell (14:D), code |
| 318 | `Objf318_ExpandingSparkleRings_Unused` | `src/events/fx_scenes.c` | cut content |
| 319 | `Objf319_Map67_Scn34_RiftArcs` | `src/events/fx_scenes.c` | event 0x1d, code |
| 320 | `Objf320_Map67_Scn34_BoltEndpoint` | `src/events/fx_scenes.c` | code |
| 321 | `Objf321_ExpandingExplosionRing_Unused` | `src/spells/dagger_storm.c` | cut content (spawned only by dead `Objf734`) |
| 322 | `Objf322_370_371_372_MagicRestoration_FX2` | `src/spells/life_orb.c` | internal/child |
| 323 | `Objf323_713_SummonRuneColumn` | `src/events/fx_scenes.c` | event 0x1d, code |
| 324 | `Objf307_324_EvilStream_FX2_FX3` | `src/spells/evil_stream.c` | spell (62:T) |
| 325 | `Objf325_ClutCycleFadeSprite_Unused` | `src/spells/evil_stream.c` | cut content |
| 326 | `Objf326_RisingSparklePillar_Unused` | `src/spells/evil_stream.c` | cut content |
| 327 | `Objf327_HealingCircle_FX2` | `src/spells/life_orb.c` | spell (28:T, 28:D, 31:T, 31:D, …) |
| 328 | `Objf305_328_MagicStoneFx` | `src/events/fx_scenes.c` | event 0x1d, code |
| 329 | `Objf329_Noop` | `src/spells/evil_stream.c` | internal/child |
| 330 | `Objf330_MagicRestoration_FX1` | `src/spells/restoration.c` | spell (23:M, 52:M, 53:M) |
| 331 | `Objf331_RedXMark_Unused` | `src/spells/salamander.c` | cut content |
| 332 | `Objf332_RollingFire_FX1` | `src/spells/salamander.c` | spell (3:M) |
| 333 | `Objf333_RuneSpiral_Unused` | `src/spells/salamander.c` | cut content |
| 334 | `Objf334_Salamander_FX1` | `src/spells/salamander.c` | spell (15:M) |
| 335 | `Objf335_Salamander_Head` | `src/spells/salamander.c` | code |
| 336 | `Objf336_Salamander_Segment` | `src/spells/salamander.c` | code |
| 337 | `Objf337_DaggerStorm_BloodSplatter` | `src/spells/dagger_storm.c` | code |
| 338 | `Objf317_338_Avalanche_FX2_FX3` | `src/spells/avalanche.c` | spell (14:T) |
| 339 | `Objf339_349_Rubble` | `src/spells/avalanche.c` | code |
| 340 | `Objf340_Map48_Scn20_LightningFan` | `src/events/fx_scenes.c` | event 0x1d |
| 341 | `Objf341_342_353_FileSaveMenu` | `src/states/main_menu.c` | code |
| 342 | `Objf341_342_353_FileSaveMenu` | `src/states/main_menu.c` | code |
| 343 | `Objf343_Etc_FileLoadMenu` | `src/states/main_menu.c` | code |
| 344 | `Objf344_345_RomanFire_FX2_FX3` | `src/spells/avalanche.c` | spell (11:T), code |
| 345 | `Objf344_345_RomanFire_FX2_FX3` | `src/spells/avalanche.c` | spell (11:D), code |
| 346 | `Objf346_ButtonDepress` | `src/maps/buttons.c` | code |
| 347 | `Objf347_Map26` | `src/maps/buttons.c` | code |
| 348 | `Objf348_BlueFlameDome_Unused` | `src/spells/salamander.c` | cut content |
| 349 | `Objf339_349_Rubble` | `src/spells/avalanche.c` | code |
| 350 | `Objf250_350_LoadEvent16` | `src/events/scene_loaders.c` | internal/child |
| 351 | `Objf351_MsgBoxText` | `src/core/text.c` | code |
| 352 | `Objf352_Map29` | `src/maps/buttons.c` | code |
| 353 | `Objf341_342_353_FileSaveMenu` | `src/states/main_menu.c` | code |
| 354 | `Objf354_Map19_DebugElevators` | `src/maps/map_17_19.c` | internal/child |
| 355 | `Objf355_356_Map19_Elevator` | `src/maps/map_17_19.c` | code |
| 356 | `Objf355_356_Map19_Elevator` | `src/maps/map_17_19.c` | code |
| 357 | `Objf357_Map19` | `src/maps/map_17_19.c` | code |
| 358 | `Objf358_Map19_Elevator` | `src/maps/map_17_19.c` | code |
| 359 | `Objf359_PhaseShift_MapScaler` | `src/spells/shared_fx.c` | code |
| 360 | `Objf343_Etc_FileLoadMenu` | `src/states/main_menu.c` | code |
| 361 | `Objf361_Map13_BridgeExplosion_Scene` | `src/maps/map_13_15.c` | code |
| 362 | `Objf362_DrawbridgeButton` | `src/maps/map_11_26_29.c` | code |
| 363 | `Objf363_Wyrmfang_FX1` | `src/spells/shared_fx.c` | spell (59:M) |
| 364 | `Objf364_Map15_Plank` | `src/maps/map_13_15.c` | code |
| 365 | `Objf365_Map17_Floodgate` | `src/maps/map_17_19.c` | code |
| 366 | `Objf366_Map17_Button` | `src/maps/map_17_19.c` | code |
| 367 | `Objf343_Etc_FileLoadMenu` | `src/states/main_menu.c` | code |
| 368 | `Objf368_Map17_DrainingWater` | `src/maps/map_17_19.c` | code |
| 369 | `Objf369_ScreenEffect` | `src/core/screen_effects.c` | code |
| 370 | `Objf322_370_371_372_MagicRestoration_FX2` | `src/spells/life_orb.c` | spell (52:T), code |
| 371 | `Objf322_370_371_372_MagicRestoration_FX2` | `src/spells/life_orb.c` | code |
| 372 | `Objf322_370_371_372_MagicRestoration_FX2` | `src/spells/life_orb.c` | spell (23:T, 53:T), code |
| 373 | `Objf343_Etc_FileLoadMenu` | `src/states/main_menu.c` | code |
| 374 | `Objf343_Etc_FileLoadMenu` | `src/states/main_menu.c` | code |
| 375 | `Objf375_FlameBreath_Particle` | `src/spells/dynamo_hum.c` | code |
| 376 | `Objf343_Etc_FileLoadMenu` | `src/states/main_menu.c` | code |
| 377 | `Objf377_SalamanderBreathHead_Unused` | `src/spells/salamander.c` | cut content |
| 378 | `Objf378_PhaseShift_FX1` | `src/spells/shared_fx.c` | spell (10:M) |
| 379 | `Objf379_EvilStream_Rock` | `src/spells/evil_stream.c` | code |
| 380 | `Objf380_LevelUpFx` | `src/spells/shared_fx.c` | code |
| 381 | `Objf381_HealingCircle_FX1` | `src/spells/restoration.c` | spell (28:M, 31:M, 37:M, 68:M) |
| 382 | `Objf382_FlameBreath` | `src/spells/dynamo_hum.c` | code |
| 383 | `Objf383_Sparkle` | `src/spells/restoration.c` | code |
| 384 | `Objf384_SkullRise_Unused` | `src/spells/restoration.c` | cut content |
| 385 | `Objf385_RevealMimic` | `src/battle/item_reveal.c` | code |
| 386 | `Objf386_HealingSparkle` | `src/spells/healing_items.c` | code |
| 387 | `Objf387_FullscreenImage` | `src/core/screen_effects.c` | code |
| 388 | `Objf388_DarkHurricane_FX1` | `src/spells/dark_hurricane.c` | spell (42:M, 71:M) |
| 389 | `Objf389_DarkHurricane_Vortex` | `src/spells/dark_hurricane.c` | code |
| 390 | `Objf390_DarkHurricane_VortexLayer` | `src/spells/dark_hurricane.c` | code |
| 391 | `Objf391_DebugStub_Unused` | `src/events/fx_helpers.c` | cut content |
| 392 | `Objf392_MorphMeshNode_Unused` | `src/spells/dark_hurricane.c` | cut content |
| 393 | `Objf393_Map44_Scn00_ExplosionRays` | `src/events/fx_scenes.c` | event 0x1d |
| 394 | `Objf394_DynamoHum_FX1` | `src/spells/dynamo_hum.c` | spell (48:M) |
| 395 | `Objf395_DynamoHum_ElectricOrb` | `src/spells/dynamo_hum.c` | code |
| 396 | `Objf396_DynamoHum_OrbElectricity` | `src/spells/dynamo_hum.c` | code |
| 397 | `Objf397_ExplosionBurst_Unused` | `src/spells/dynamo_hum.c` | cut content |
| 398 | `Objf398_ExplosionBurstParticle_Unused` | `src/spells/dynamo_hum.c` | code |
| 399 | `Objf399_Map11` | `src/maps/common.c` | code |
| 400 | `Objf400_AI_BuildSpellValueGrid` | `src/battle/ai.c` | code |
| 401 | `Objf401_AI_BuildEnemyProximityGrid` | `src/battle/ai.c` | code |
| 402 | `Objf402_AI_PlanSpellCast` | `src/battle/ai.c` | code |
| 403 | `Objf403_AI_PlanAttack` | `src/battle/ai.c` | code |
| 404 | `Objf404_AI_PlanRetreat` | `src/battle/ai.c` | code |
| 405 | `Objf405_Panorama` | `src/core/object.c` | code |
| 406 | `Objf406_ShopOrDepot` | `src/ui/supplies.c` | code |
| 407 | `Objf407_NoopIncState` | `src/core/object.c` | code |
| 408 | `Objf004_005_408_Window` | `src/ui/window.c` | code |
| 409 | `Objf409_EventEntity` | `src/events/entities.c` | code |
| 410 | `Objf410_EventZoom` | `src/battle/presentation.c` | code |
| 411 | `Objf411_MapObject_VileBog` | `src/core/animated_textures.c` | map table, code |
| 412 | `Objf412_EventCamera` | `src/units/actor.c` | code |
| 413 | `Objf413_MsgBoxPortrait` | `src/units/roster.c` | code |
| 414 | `Objf414_DebugMenu` | `src/states/debug_menu.c` | code |
| 415 | `Objf415_MapObject_Torch` | `src/maps/objects.c` | map table, code |
| 416 | `Objf416_LoadEvent00` | `src/events/scene_loaders.c` | scene loader |
| 417 | `Objf417_LoadEvent03` | `src/events/scene_loaders.c` | scene loader |
| 418 | `Objf418_LoadEvent06` | `src/events/scene_loaders.c` | scene loader |
| 419 | `Objf419_Noop` | `src/events/scene_loaders.c` | internal/child |
| 420 | `Objf420_BattleVictory` | `src/battle/evaluators.c` | code |
| 421 | `Objf421_UpperMsgBoxTail` | `src/ui/window.c` | code |
| 422 | `Objf422_LowerMsgBoxTail` | `src/ui/window.c` | code |
| 423 | `Objf423_BattleDefeat` | `src/battle/evaluators.c` | code |
| 424 | `Objf424_BattleEnder` | `src/states/game_setup.c` | code |
| 425 | `Objf425_BattleOptions` | `src/battle/field.c` | code |
| 426 | `Objf426_EvaluateMap10_SlayZoot` | `src/battle/evaluators.c` | evaluator, code |
| 427 | `Objf427_EvaluateMap11_ReachExit` | `src/battle/evaluators.c` | evaluator, code |
| 428 | `Objf428_EvaluateMap12_DefeatAll` | `src/battle/evaluators.c` | evaluator, code |
| 429 | `Objf429_EvaluateMap13_DefeatAll` | `src/battle/evaluators.c` | evaluator, code |
| 430 | `Objf430_EvaluateMap14_SlayDeathAnts` | `src/battle/evaluators.c` | evaluator, code |
| 431 | `Objf431_EvaluateMap15_SlayHassan` | `src/battle/evaluators.c` | evaluator, code |
| 432 | `Objf432_EvaluateMap16_SlayEvilStatues` | `src/battle/evaluators.c` | evaluator, code |
| 433 | `Objf433_EvaluateMap17_ProtectDolan` | `src/battle/evaluators.c` | evaluator, code |
| 434 | `Objf434_EvaluateStandardBattle` | `src/battle/evaluators.c` | evaluator, code |
| 435 | `Objf435_EvaluateMap19_ReachExit` | `src/battle/evaluators.c` | evaluator, code |
| 436 | `Objf436_EvaluateMap20_SlayMagnus` | `src/battle/evaluators.c` | evaluator, code |
| 437 | `Objf437_EvaluateMap21_DefeatAllIn6Turns` | `src/battle/evaluators.c` | evaluator, code |
| 438 | `Objf438_EvaluateMap08_DemoExit` | `src/battle/evaluators.c` | evaluator, code |
| 439 | `Objf439_EvaluateMap23_DefendMageTowers` | `src/battle/evaluators.c` | evaluator, code |
| 442 | `Objf442_EvaluateMap26_StopEscapees` | `src/battle/evaluators.c` | evaluator, code |
| 443 | `Objf443_EvaluateMap27_ProtectClint` | `src/battle/evaluators.c` | evaluator, code |
| 444 | `Objf444_EvaluateMap28_SlayDumas` | `src/battle/evaluators.c` | evaluator, code |
| 445 | `Objf445_EvaluateMap29_ReachExit` | `src/battle/evaluators.c` | evaluator, code |
| 446 | `Objf446_BattleVictoryParticle` | `src/battle/evaluators.c` | code |
| 447 | `Objf447_UnitPortrait` | `src/units/roster.c` | code |
| 448 | `Objf448_UnitPortraitWrapper` | `src/units/roster.c` | internal/child |
| 449 | `Objf449_MapObject_FlowingWater` | `src/core/animated_textures.c` | map table, code |
| 450 | `Objf450_LoadEvent68` | `src/events/scene_loaders.c` | scene loader |
| 451 | `Objf451_LoadEvent70` | `src/events/scene_loaders.c` | scene loader |
| 452 | `Objf452_LoadEvent72` | `src/events/scene_loaders.c` | scene loader |
| 453 | `Objf453_LoadEvent73` | `src/events/scene_loaders.c` | scene loader |
| 454 | `Objf454_LoadEvent75` | `src/events/scene_loaders.c` | scene loader |
| 455 | `Objf455_LoadEvent76` | `src/events/scene_loaders.c` | scene loader |
| 456 | `Objf456_LoadEvent79` | `src/events/scene_loaders.c` | scene loader |
| 457 | `Objf457_LoadEvent80` | `src/events/scene_loaders.c` | scene loader |
| 458 | `Objf458_LoadEvent81` | `src/events/scene_loaders.c` | scene loader |
| 459 | `Objf459_LoadEvent83` | `src/events/scene_loaders.c` | scene loader |
| 460 | `Objf460_LoadEvent01` | `src/events/scene_loaders.c` | scene loader |
| 461 | `Objf461_LoadEvent02` | `src/events/scene_loaders.c` | scene loader |
| 462 | `Objf462_LoadEvent04` | `src/events/scene_loaders.c` | scene loader |
| 463 | `Objf463_LoadEvent08` | `src/events/scene_loaders.c` | scene loader |
| 464 | `Objf464_LoadEvent09` | `src/events/scene_loaders.c` | scene loader |
| 465 | `Objf465_LoadEvent11` | `src/events/scene_loaders.c` | scene loader |
| 466 | `Objf466_LoadEvent12` | `src/events/scene_loaders.c` | scene loader |
| 467 | `Objf467_LoadEvent13` | `src/events/scene_loaders.c` | scene loader |
| 468 | `Objf468_LoadEvent19` | `src/events/scene_loaders.c` | scene loader |
| 469 | `Objf469_LoadEvent21` | `src/events/scene_loaders.c` | scene loader |
| 470 | `Objf470_LoadEvent22` | `src/events/scene_loaders.c` | scene loader |
| 471 | `Objf471_LoadEvent24` | `src/events/scene_loaders.c` | scene loader |
| 472 | `Objf472_LoadEvent28` | `src/events/scene_loaders.c` | scene loader |
| 473 | `Objf473_LoadEvent29` | `src/events/scene_loaders.c` | scene loader |
| 474 | `Objf474_LoadEvent30` | `src/events/scene_loaders.c` | scene loader |
| 475 | `Objf475_LoadEvent31` | `src/events/scene_loaders.c` | scene loader |
| 476 | `Objf476_LoadEvent34` | `src/events/scene_loaders.c` | scene loader |
| 477 | `Objf477_LoadEvent35` | `src/events/scene_loaders.c` | scene loader |
| 478 | `Objf478_LoadEvent36` | `src/events/scene_loaders.c` | scene loader |
| 479 | `Objf479_LoadEvent39` | `src/events/scene_loaders.c` | scene loader |
| 480 | `Objf480_LoadEvent40` | `src/events/scene_loaders.c` | scene loader |
| 481 | `Objf481_LoadEvent41` | `src/events/scene_loaders.c` | scene loader |
| 482 | `Objf482_LoadEvent50` | `src/events/scene_loaders.c` | scene loader |
| 483 | `Objf483_LoadEvent52` | `src/events/scene_loaders.c` | scene loader |
| 484 | `Objf484_LoadEvent55` | `src/events/scene_loaders.c` | scene loader |
| 485 | `Objf485_LoadEvent56` | `src/events/scene_loaders.c` | scene loader |
| 486 | `Objf486_LoadEvent61` | `src/events/scene_loaders.c` | scene loader |
| 487 | `Objf487_LoadEvent64` | `src/events/scene_loaders.c` | scene loader |
| 488 | `Objf488_LoadEvent65` | `src/events/scene_loaders.c` | scene loader |
| 489 | `Objf489_LoadEvent66` | `src/events/scene_loaders.c` | scene loader |
| 490 | `Objf490_LoadEvent71` | `src/events/scene_loaders.c` | scene loader |
| 491 | `Objf491_LoadEvent60` | `src/events/scene_loaders.c` | scene loader |
| 492 | `Objf492_LoadEvent10` | `src/events/scene_loaders.c` | scene loader |
| 493 | `Objf493_LoadEvent18` | `src/events/scene_loaders.c` | scene loader |
| 494 | `Objf494_LoadEvent15` | `src/events/scene_loaders.c` | scene loader |
| 495 | `Objf495_LoadEvent17` | `src/events/scene_loaders.c` | scene loader |
| 496 | `Objf496_LoadEvent27` | `src/events/scene_loaders.c` | scene loader |
| 497 | `Objf497_LoadEvent32` | `src/events/scene_loaders.c` | scene loader |
| 498 | `Objf498_LoadEvent38` | `src/events/scene_loaders.c` | scene loader |
| 499 | `Objf499_LoadEvent43` | `src/events/scene_loaders.c` | scene loader |
| 500 | `Objf500_LoadEvent46` | `src/events/scene_loaders.c` | scene loader |
| 501 | `Objf501_LoadEvent48` | `src/events/scene_loaders.c` | scene loader |
| 502 | `Objf502_LoadEvent49` | `src/events/scene_loaders.c` | scene loader |
| 503 | `Objf503_LoadEvent53` | `src/events/scene_loaders.c` | scene loader |
| 504 | `Objf504_LoadEvent63` | `src/events/scene_loaders.c` | scene loader |
| 505 | `Objf505_LoadEvent69` | `src/events/scene_loaders.c` | scene loader |
| 506 | `Objf506_LoadEvent74` | `src/events/scene_loaders.c` | scene loader |
| 507 | `Objf507_LoadEvent77` | `src/events/scene_loaders.c` | scene loader |
| 508 | `Objf508_LoadEvent78` | `src/events/scene_loaders.c` | scene loader |
| 509 | `Objf509_LoadEvent82` | `src/events/scene_loaders.c` | scene loader |
| 510 | `Objf510_LoadEvent84` | `src/events/scene_loaders.c` | scene loader |
| 511 | `Objf511_LoadEvent85` | `src/events/scene_loaders.c` | scene loader |
| 512 | `Objf512_LoadEvent86` | `src/events/scene_loaders.c` | scene loader |
| 513 | `Objf513_LoadEvent87` | `src/events/scene_loaders.c` | scene loader |
| 514 | `Objf514_LoadEvent88` | `src/events/scene_loaders.c` | scene loader |
| 515 | `Objf515_LoadEvent89` | `src/events/scene_loaders.c` | scene loader |
| 516 | `Objf516_LoadEvent90` | `src/events/scene_loaders.c` | scene loader |
| 517 | `Objf517_LoadEvent91` | `src/events/scene_loaders.c` | scene loader |
| 518 | `Objf518_LoadEvent92` | `src/events/scene_loaders.c` | scene loader |
| 519 | `Objf519_LoadEvent93` | `src/events/scene_loaders.c` | scene loader |
| 520 | `Objf520_LoadEvent25` | `src/events/scene_loaders.c` | scene loader |
| 521 | `Objf521_LoadEvent94` | `src/events/scene_loaders.c` | scene loader |
| 522 | `Objf522_LoadEvent26` | `src/events/scene_loaders.c` | scene loader |
| 523 | `Objf523_LoadEvent07` | `src/events/scene_loaders.c` | scene loader |
| 524 | `Objf524_LoadEvent20` | `src/events/scene_loaders.c` | scene loader |
| 525 | `Objf525_LoadEvent47` | `src/events/scene_loaders.c` | scene loader |
| 530 | `Objf530_Map61_Scn83_VandalHeartForcefield` | `src/events/fx_scenes.c` | event 0x1d |
| 535 | `Objf535_536_FadeLight` | `src/events/fx_scenes.c` | event 0x1d, code |
| 536 | `Objf535_536_FadeLight` | `src/events/fx_scenes.c` | event 0x1d |
| 540 | `Objf540_to_545_Map14_Scn15_CloudSpawner` | `src/events/fx_scenes.c` | event 0x1d |
| 541 | `Objf540_to_545_Map14_Scn15_CloudSpawner` | `src/events/fx_scenes.c` | event 0x1d |
| 542 | `Objf540_to_545_Map14_Scn15_CloudSpawner` | `src/events/fx_scenes.c` | event 0x1d |
| 543 | `Objf540_to_545_Map14_Scn15_CloudSpawner` | `src/events/fx_scenes.c` | event 0x1d |
| 544 | `Objf540_to_545_Map14_Scn15_CloudSpawner` | `src/events/fx_scenes.c` | event 0x1d |
| 545 | `Objf540_to_545_Map14_Scn15_CloudSpawner` | `src/events/fx_scenes.c` | event 0x1d |
| 552 | `Objf552_EvaluateMap32_SlayDallas` | `src/battle/evaluators.c` | evaluator, code |
| 553 | `Objf553_EvaluateMap33_SlayDeathDevs` | `src/battle/evaluators.c` | evaluator, code |
| 555 | `Objf555_EvaluateMap35_SlayKurtz` | `src/battle/evaluators.c` | evaluator, code |
| 557 | `Objf557_EvaluateMap37_SlaySalamanders` | `src/battle/evaluators.c` | evaluator, code |
| 558 | `Objf558_EvaluateMap38_SlaySabina` | `src/battle/evaluators.c` | evaluator, code |
| 559 | `Objf559_EvaluateMap39_EscortLeena` | `src/battle/evaluators.c` | evaluator, code |
| 560 | `Objf560_EvaluateMap40_SlayKane` | `src/battle/evaluators.c` | evaluator, code |
| 562 | `Objf562_EvaluateMap42_SlayXeno` | `src/battle/evaluators.c` | evaluator, code |
| 563 | `Objf563_EvaluateMap43_SlayDolf` | `src/battle/evaluators.c` | evaluator, code |
| 564 | `Objf564_565_566_MapObject_Water` | `src/core/animated_textures.c` | map table, code |
| 565 | `Objf564_565_566_MapObject_Water` | `src/core/animated_textures.c` | map table, code |
| 566 | `Objf564_565_566_MapObject_Water` | `src/core/animated_textures.c` | map table, code |
| 567 | `Objf567_OpeningChest` | `src/battle/executors.c` | code |
| 568 | `Objf568_MapObject_Rail` | `src/core/animated_textures.c` | map table, code |
| 569 | `Objf569_572_MapObject_Lava` | `src/core/animated_textures.c` | map table, code |
| 570 | `Objf570_AI_ChooseAction` | `src/battle/ai.c` | code |
| 571 | `Objf571_LevelUp` | `src/battle/presentation.c` | code |
| 572 | `Objf569_572_MapObject_Lava` | `src/core/animated_textures.c` | map table, code |
| 573 | `Objf573_BattleItemsList` | `src/ui/window.c` | code |
| 574 | `Objf574_DisplayIcon` | `src/ui/window.c` | code |
| 575 | `Objf575_StatusPortrait` | `src/units/roster.c` | code |
| 576 | `Objf576_Tavern` | `src/world/tavern.c` | code |
| 577 | `Objf577_DynamicIcon` | `src/ui/supplies.c` | code |
| 578 | `Objf578_Dojo` | `src/world/dojo.c` | code |
| 579 | `Objf579_WorldMap` | `src/world/map.c` | code |
| 580 | `Objf580_Town` | `src/world/town.c` | code |
| 581 | `Objf581_AudioCommand` | `src/core/object.c` | code |
| 582 | `Objf582_MainMenu_Jpn` | `src/core/main.c` | internal/child |
| 583 | `Objf583_LoadingIndicator` | `src/core/main.c` | internal/child |
| 584 | `Objf584_Noop` | `src/core/main.c` | internal/child |
| 585 | `Objf585_BattlePlayerEvent` | `src/battle/field.c` | code |
| 586 | `Objf586_BattleMsgBox` | `src/battle/field.c` | code |
| 587 | `Objf587_BattleEnemyEvent` | `src/battle/field.c` | code |
| 588 | `Objf026_588_FocusCamera` | `src/battle/presentation.c` | code |
| 589 | `Objf589_AI_MoveToEscapePoint` | `src/battle/ai.c` | code |
| 590 | `Objf590_BattleTurnTicker` | `src/events/entities.c` | code |
| 591 | `Objf591_MapObject_Boulder` | `src/maps/objects.c` | map table, code |
| 592 | `Objf592_BattleTurnStart` | `src/battle/executors.c` | code |
| 593 | `Objf593_BattleResultsUnit` | `src/battle/results.c` | code |
| 594 | `Objf594_BattleResults` | `src/battle/results.c` | code |
| 595 | `Objf595_StatusWindow` | `src/ui/status_window.c` | code |
| 596 | `Objf596_StatusWindowMgr` | `src/ui/status_window.c` | code |
| 597 | `Objf597_BattleIntro` | `src/battle/field.c` | code |
| 598 | `Objf598_WorldActions` | `src/world/actions.c` | code |
| 650 | `Objf650_Map32_CarRelease` | `src/maps/map_32.c` | code |
| 651 | `Objf651_Map33_LavaPitPlatform` | `src/maps/map_33.c` | code |
| 652 | `Objf652_Map35_Button` | `src/maps/map_35.c` | code |
| 653 | `Objf653_ExplodingTile` | `src/maps/map_33.c` | code |
| 654 | `Objf654_Map38_WashAwayUnit` | `src/maps/map_38.c` | code |
| 655 | `Objf655_Map38_RaiseFloodgate` | `src/maps/map_38.c` | code |
| 656 | `Objf656_Map39` | `src/maps/map_39_40.c` | code |
| 657 | `Objf657_Map38_Floodgate` | `src/maps/map_38.c` | code |
| 658 | `Objf658_Map38_Floodwater` | `src/maps/map_38.c` | code |
| 659 | `Objf659_Splash` | `src/maps/map_38.c` | code |
| 661 | `Objf304_661_Flame` | `src/maps/map_28_31.c` | code |
| 662 | `Objf662_Map28_OpenDoor` | `src/maps/map_28_31.c` | code |
| 663 | `Objf663_Map28_Button` | `src/maps/map_28_31.c` | code |
| 664 | `Objf664_Map27_OpenCellDoor` | `src/maps/map_27.c` | code |
| 665 | `Objf665_Map27_Buttons` | `src/maps/map_27.c` | code |
| 666 | `Objf666_Map14_LowerSandMound` | `src/maps/buttons.c` | code |
| 667 | `Objf667_Map14_LowerSandTile` | `src/maps/buttons.c` | code |
| 668 | `Objf668_Map14_RaiseSandMound` | `src/maps/buttons.c` | code |
| 669 | `Objf669_Map14_RaiseSandTile` | `src/maps/buttons.c` | code |
| 670 | `Objf670_Map14_Sand` | `src/maps/buttons.c` | code |
| 672 | `Objf672_Map39_SplashingTile` | `src/maps/map_39_40.c` | code |
| 673 | `Objf673_Map32_Scn63_Cinematic` | `src/maps/map_32.c` | event 0x1d |
| 674 | `Objf674_DebugSounds` | `src/events/fx_helpers.c` | internal/child |
| 675 | `Objf675_LeenaForcefield` | `src/maps/ambience.c` | code |
| 676 | `Objf676_687_Rainfall` | `src/maps/ambience.c` | event 0x1d, code |
| 677 | `Objf677_RainfallDrop` | `src/maps/ambience.c` | code |
| 678 | `Objf678_Ripple` | `src/maps/ambience.c` | code |
| 679 | `Objf679_EntityFlasher` | `src/events/fx_scenes.c` | event 0x1d |
| 680 | `Objf680_LitDummySprite` | `src/events/fx_scenes.c` | event 0x1d |
| 681 | `Objf681_StatBuffFx` | `src/spells/support_magic.c` | code |
| 682 | `Objf682_RaiseFaces_Unused` | `src/events/fx_scenes.c` | cut content |
| 683 | `Objf683_AdjustFaceElevation` | `src/maps/unpack.c` | code |
| 684 | `Objf684_SlidingFace` | `src/maps/unpack.c` | code |
| 685 | `Objf685_RockSpurt` | `src/spells/shared_fx.c` | code |
| 686 | `Objf206_686_BloodSpurtParticle` | `src/spells/hit_effects.c` | code |
| 687 | `Objf676_687_Rainfall` | `src/maps/ambience.c` | event 0x1d, code |
| 688 | `Objf688_Noop` | `src/events/fx_helpers.c` | internal/child |
| 689 | `Objf689_EntityFlashBurstRays` | `src/events/fx_scenes.c` | event 0x1d |
| 690 | `Objf690_MagicStoneExplosion` | `src/events/fx_scenes.c` | code |
| 691 | `Objf691_Map43_Scn93_CameraShake` | `src/events/fx_scenes.c` | event 0x1d |
| 692 | `Objf692_Campfire` | `src/maps/ambience.c` | event 0x1d |
| 693 | `Objf693_EntityRedStripePulse_Unused` | `src/events/fx_scenes.c` | cut content |
| 694 | `Objf694_Map61_Scn83_AshGlow` | `src/events/fx_scenes.c` | event 0x1d |
| 695 | `Objf695_696_EntityBlendFade_Unused` | `src/events/fx_scenes.c` | code |
| 696 | `Objf695_696_EntityBlendFade_Unused` | `src/events/fx_scenes.c` | cut content |
| 697 | `Objf697_Map43_Scn93_FlameSphere` | `src/events/fx_scenes.c` | event 0x1d |
| 698 | `Objf698_Map61_Scn83_EleniSparkleRings` | `src/events/fx_scenes.c` | code |
| 699 | `Objf699_Map61_Scn83_EleniSpell` | `src/events/fx_scenes.c` | event 0x1d |
| 700 | `Objf700_DynamoHum_ColoredBolt` | `src/events/fx_scenes.c` | code |
| 702 | `Objf702_FlamingRock` | `src/maps/map_35.c` | code |
| 703 | `Objf703_Map40_Barricade` | `src/maps/map_39_40.c` | code |
| 705 | `Objf705_732_743_744_Transformation` | `src/events/fx_scenes.c` | internal/child |
| 707 | `Objf707_RisingGlyph` | `src/events/fx_scenes.c` | code |
| 708 | `Objf708_709_Map14_Unused` | `src/maps/buttons.c` | code |
| 709 | `Objf708_709_Map14_Unused` | `src/maps/buttons.c` | code |
| 710 | `Objf710_Particle` | `src/spells/shared_fx.c` | code |
| 711 | `Objf711_712_Noop` | `src/spells/restoration.c` | internal/child |
| 712 | `Objf711_712_Noop` | `src/spells/restoration.c` | internal/child |
| 713 | `Objf323_713_SummonRuneColumn` | `src/events/fx_scenes.c` | event 0x1d, code |
| 714 | `Objf714_DebugCamera` | `src/events/fx_scenes.c` | internal/child |
| 715 | `Objf715_to_718_Spellbind_FX2_FX3` | `src/spells/shared_fx.c` | spell (7:T, 63:T), code |
| 716 | `Objf715_to_718_Spellbind_FX2_FX3` | `src/spells/shared_fx.c` | spell (7:D, 63:D), code |
| 717 | `Objf715_to_718_Spellbind_FX2_FX3` | `src/spells/shared_fx.c` | code |
| 718 | `Objf715_to_718_Spellbind_FX2_FX3` | `src/spells/shared_fx.c` | code |
| 719 | `Objf719_DimensionalRift` | `src/events/fx_scenes.c` | code |
| 720 | `Objf720_Map61_Scn83_XenoCastingCylinder` | `src/events/fx_scenes.c` | event 0x1d |
| 721 | `Objf721_Map61_Scn83_XenoCastingCylinder_Crest` | `src/events/fx_scenes.c` | code |
| 722 | `Objf722_DimensionalRift_Sparkles` | `src/events/fx_scenes.c` | code |
| 723 | `Objf723_HomingParticle` | `src/events/fx_scenes.c` | code |
| 724 | `Objf724_741_DimensionalRift_Open` | `src/events/fx_scenes.c` | event 0x1d |
| 725 | `Objf725_CastingRays` | `src/events/fx_scenes.c` | event 0x1d |
| 726 | `Objf726_CastingRays_Stop` | `src/events/fx_scenes.c` | event 0x1d |
| 727 | `Objf727_RollingThunderOnEntity_Unused` | `src/events/fx_scenes.c` | cut content |
| 728 | `Objf728_FlickeringExpandRing` | `src/events/fx_scenes.c` | code |
| 729 | `Objf265_266_729_ShrinkWarpSprite` | `src/events/fx_scenes.c` | event 0x1d |
| 730 | `Objf730_EnableAdditiveBlending` | `src/events/fx_scenes.c` | event 0x1d |
| 731 | `Objf731_DimensionalRift_Close` | `src/events/fx_scenes.c` | event 0x1d |
| 732 | `Objf705_732_743_744_Transformation` | `src/events/fx_scenes.c` | code |
| 733 | `Objf733_StatBuffIcon` | `src/spells/support_magic.c` | code |
| 734 | `Objf734_MeteorImpact_Unused` | `src/events/fx_scenes.c` | cut content |
| 735 | `Objf735_SparkleDust` | `src/spells/shared_fx.c` | code |
| 736 | `Objf736_RemoveParalysis_Bubble` | `src/spells/shared_fx.c` | code |
| 737 | `Objf737_RemoveParalysis` | `src/spells/shared_fx.c` | code |
| 738 | `Objf738_Map40_LowerBarricade` | `src/maps/map_39_40.c` | code |
| 739 | `Objf739_Particle` | `src/spells/shared_fx.c` | cut content (spawned only by dead `Objf790`) |
| 740 | `Objf740_RemoveParalysis_Sparkles` | `src/spells/shared_fx.c` | code |
| 741 | `Objf724_741_DimensionalRift_Open` | `src/events/fx_scenes.c` | event 0x1d, code |
| 742 | `Objf742_Map67_Scn34_RiftArcs_Separate` | `src/events/fx_scenes.c` | event 0x1d |
| 743 | `Objf705_732_743_744_Transformation` | `src/events/fx_scenes.c` | event 0x1d |
| 744 | `Objf705_732_743_744_Transformation` | `src/events/fx_scenes.c` | event 0x1d |
| 745 | `Objf745_MapUnitTransformation_Unused` | `src/events/fx_scenes.c` | cut content |
| 746 | `Objf202_746_UnitBlocking` | `src/spells/hit_effects.c` | event 0x1d, code |
| 747 | `Objf747_748_Wyrmfang_Flames` | `src/spells/salamander.c` | code |
| 748 | `Objf747_748_Wyrmfang_Flames` | `src/spells/salamander.c` | code |
| 749 | `Objf749_Wyrmfang_Flame` | `src/spells/salamander.c` | code |
| 750 | `Objf750_751_Map33_LowerPlatform` | `src/maps/map_33.c` | event 0x1d, code |
| 751 | `Objf750_751_Map33_LowerPlatform` | `src/maps/map_33.c` | code |
| 752 | `Objf752_Map14_Scn15_SandMoundSpawner` | `src/maps/buttons.c` | event 0x1d |
| 753 | `Objf753_IncrementMapState0` | `src/maps/common.c` | event 0x1d |
| 754 | `Objf754_Map39_Scn82` | `src/maps/map_39_40.c` | event 0x1d |
| 755 | `Objf755_Map15_PirateStandIn` | `src/maps/map_13_15.c` | event 0x1d, code |
| 756 | `Objf756_Map36_Scn75_Cinematic` | `src/events/fx_scenes.c` | event 0x1d |
| 757 | `Objf757_PushedObjectSplash` | `src/spells/shared_fx.c` | code |
| 758 | `Objf758_Map44_Scn00_ShrinkDoorTex` | `src/maps/unpack.c` | event 0x1d |
| 759 | `Objf759_RockSpurtParticle` | `src/spells/shared_fx.c` | code |
| 760 | `Objf760_EliteMeleeSparkles` | `src/spells/shared_fx.c` | code |
| 761 | `Objf290_294_761_RevealItem` | `src/battle/item_reveal.c` | code |
| 762 | `Objf762_Megaherb` | `src/spells/shared_fx.c` | internal/child |
| 763 | `Objf763_BoulderRubble` | `src/spells/shared_fx.c` | code |
| 764 | `Objf764_to_769_ProjectileTrail` | `src/spells/shared_fx.c` | code |
| 765 | `Objf764_to_769_ProjectileTrail` | `src/spells/shared_fx.c` | code |
| 766 | `Objf764_to_769_ProjectileTrail` | `src/spells/shared_fx.c` | code |
| 767 | `Objf764_to_769_ProjectileTrail` | `src/spells/shared_fx.c` | code |
| 768 | `Objf764_to_769_ProjectileTrail` | `src/spells/shared_fx.c` | internal/child |
| 769 | `Objf764_to_769_ProjectileTrail` | `src/spells/shared_fx.c` | internal/child |
| 770 | `Objf770_to_789_ItemSpell` | `src/spells/shared_fx.c` | code |
| 771 | `Objf770_to_789_ItemSpell` | `src/spells/shared_fx.c` | internal/child |
| 772 | `Objf770_to_789_ItemSpell` | `src/spells/shared_fx.c` | internal/child |
| 773 | `Objf770_to_789_ItemSpell` | `src/spells/shared_fx.c` | internal/child |
| 774 | `Objf770_to_789_ItemSpell` | `src/spells/shared_fx.c` | internal/child |
| 775 | `Objf770_to_789_ItemSpell` | `src/spells/shared_fx.c` | internal/child |
| 776 | `Objf770_to_789_ItemSpell` | `src/spells/shared_fx.c` | internal/child |
| 777 | `Objf770_to_789_ItemSpell` | `src/spells/shared_fx.c` | internal/child |
| 778 | `Objf770_to_789_ItemSpell` | `src/spells/shared_fx.c` | internal/child |
| 779 | `Objf770_to_789_ItemSpell` | `src/spells/shared_fx.c` | internal/child |
| 780 | `Objf770_to_789_ItemSpell` | `src/spells/shared_fx.c` | internal/child |
| 781 | `Objf770_to_789_ItemSpell` | `src/spells/shared_fx.c` | internal/child |
| 782 | `Objf770_to_789_ItemSpell` | `src/spells/shared_fx.c` | internal/child |
| 783 | `Objf770_to_789_ItemSpell` | `src/spells/shared_fx.c` | internal/child |
| 784 | `Objf770_to_789_ItemSpell` | `src/spells/shared_fx.c` | internal/child |
| 785 | `Objf770_to_789_ItemSpell` | `src/spells/shared_fx.c` | internal/child |
| 786 | `Objf770_to_789_ItemSpell` | `src/spells/shared_fx.c` | internal/child |
| 787 | `Objf770_to_789_ItemSpell` | `src/spells/shared_fx.c` | internal/child |
| 788 | `Objf770_to_789_ItemSpell` | `src/spells/shared_fx.c` | internal/child |
| 789 | `Objf770_to_789_ItemSpell` | `src/spells/shared_fx.c` | internal/child |
| 790 | `Objf790_EmberEmitter_Unused` | `src/spells/shared_fx.c` | cut content |
| 791 | `Objf306_791_792_793_Healing_FX2` | `src/spells/healing_items.c` | spell (19:T), code |
| 792 | `Objf306_791_792_793_Healing_FX2` | `src/spells/healing_items.c` | spell (22:T), code |
| 793 | `Objf306_791_792_793_Healing_FX2` | `src/spells/healing_items.c` | spell (25:T, 55:T), code |
| 794 | `Objf794_DisableBlending` | `src/events/fx_scenes.c` | event 0x1d |
| 795 | `Objf795_EventFade` | `src/core/screen_effects.c` | code |
| 796 | `Objf796_MainMenu` | `src/states/main_menu.c` | code |
| 797 | `Objf797_Map47_Scn14_Dusk` | `src/spells/shared_fx.c` | event 0x1d |
| 798 | `Objf798_ResetInputState` | `src/core/text.c` | event 0x1d |
| 799 | `Objf132_EngulfUnit` | `src/spells/hit_effects.c` | spell (5:T), code |
| 800 | `Objf132_EngulfUnit` | `src/spells/hit_effects.c` | spell (5:D), code |
| 801 | `Objf801_FlameRingSprite` | `src/spells/hit_effects.c` | code |
| 802 | `Objf802_ExplosionRingSprite` | `src/spells/hit_effects.c` | code |
| 803 | `Objf803_LightningRingSprite` | `src/spells/hit_effects.c` | code |
