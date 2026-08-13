# Spell FX dispatch (`gSpellsEx`)

How the game turns "unit casts spell N" into visuals — decoded from the retail data and
the executor code, and validated live: an instrumented build logged every FX dispatch
across all 70 castable spells/items in-game, and every logged dispatch matched this
table. Witnessed cells are marked ✓ below.

## The model

Spell visuals have **no static spawn sites**. When the spell executor
(`Objf028_UnitCasting`, `src/battle/executors.c`) commits a cast, it dispatches object
handlers data-driven out of `gSpellsEx[spellId]` (73 rows of 5 `s16`, ids 1–71 live):

| field | meaning |
|---|---|
| `SPELL_EX_OBJF_MAIN` | spawned once at cast time — the main casting visual |
| `SPELL_EX_OBJF_TARGET` | spawned per target on a normal outcome |
| `SPELL_EX_OBJF_DEFEAT` | the *alternate-outcome* slot — see below, its meaning depends on the effect class |
| `SPELL_EX_MP_BONUS` | MP cost/bonus datum |
| `SPELL_EX_EFFECT` | effect class (`SPELL_EFFECT_*`, `include/units.h`) — selects the executor branch |

### What the third slot really means

The executor reads `SPELL_EX_OBJF_DEFEAT` in only two of its effect-class branches:

- **damage spells** (`SPELL_EFFECT_DAMAGE`): the slay visual, dispatched when the target's
  HP reaches 0 — *unless the target has a defeat speech*, in which case the game plays the
  TARGET visual and the speech instead (`src/battle/executors.c`, the `ATK_RES_DEFEATED`
  path).
- **ailment spells** (`SPELL_EFFECT_POISON` / `SPELL_EFFECT_PARALYZE`): not a defeat at
  all — it is the **"ailment didn't stick"** visual, dispatched when the roll fails
  (the `ix = 1/2` selection in the poison/paralyze branch).

For **every other class** (heal, cure, buffs, MP effects) the executor never reads the
slot: whatever the row carries there is **dormant data** — usually the generic
`Objf079_Slay_FX3`, sometimes a mirror of the TARGET handler, and for spells 1/4/32
literally an empty table slot (index 143 has no handler). None of it can ever run,
because those branches dispatch only TARGET. The table below marks these cells
*dormant*.

### Conventions

Handler names carry their slot as a suffix (`_FX1` = MAIN, `_FX2` = TARGET, `_FX3` =
slot 2). One function can serve several table indices — the engulf family is the big
one: `Objf132_EngulfUnit` backs indices 132–142 plus 799/800, and branches on **which
index spawned it** (element pairs + hit/slay variant). The cells therefore show the
index and the handler: `138 Objf132_EngulfUnit` is the Dynamo Hum slay variant even
though the C function is the same as `137`'s hit variant. FX1 drivers own the
completion handshake — they raise `gSignal3` when the presentation is done; children
never do.

## The retail table

Generated from the retail binary (`gSpellsEx` + `gSpellNames`); handler file locations
are in [objf-handlers.md](objf-handlers.md). ✓ = this exact (spell, slot, index)
dispatch was witnessed in the validation session; unwitnessed live cells are mostly
slay variants that simply require a kill (or a resist) with that specific spell.

| id | spell | class | MAIN | TARGET | slot 2 (defeat / no-stick) |
|---:|---|---|---|---|---|
| 1 | Faerie Light | heal | 60 `Objf060_Healing_FX1` ✓ | 115 `Objf115_Faerie_FX2` ✓ | 143 = empty slot (never fires) |
| 2 | Ice Storm | damage | 189 `Objf189_IceStorm_FX1` ✓ | 78 `Objf078_Damage_FX2` ✓ | 79 `Objf079_Slay_FX3` |
| 3 | Rolling Fire | damage | 332 `Objf332_RollingFire_FX1` ✓ | 132 `Objf132_EngulfUnit` ✓ | 134 `Objf132_EngulfUnit` |
| 4 | Faerie Star | heal | 60 `Objf060_Healing_FX1` ✓ | 115 `Objf115_Faerie_FX2` ✓ | 143 = empty slot (never fires) |
| 5 | Delta Mirage | damage | 156 `Objf156_DeltaMirage_FX1` ✓ | 799 `Objf132_EngulfUnit` ✓ | 800 `Objf132_EngulfUnit` |
| 6 | Dark Star | damage | 207 `Objf207_SummonRedCrest` ✓ | 193 `Objf193_DarkStar_FX2` ✓ | 194 `Objf194_DarkStar_FX3` |
| 7 | Spellbind | paralyze | 181 `Objf181_Spellbind_FX1` ✓ | 715 `Objf715_to_718_Spellbind_FX2_FX3` ✓ | 716 `Objf715_to_718_Spellbind_FX2_FX3` |
| 8 | Piercing Ray | damage | 170 `Objf170_PiercingRay_Etc_FX1` ✓ | 78 `Objf078_Damage_FX2` ✓ | 79 `Objf079_Slay_FX3` |
| 9 | Envenom | poison | 210 `Objf210_SummonGreenCrest` ✓ | 102 `Objf102_227_Poison_FX2` ✓ | 227 `Objf102_227_Poison_FX2` |
| 10 | Phase Shift | damage | 378 `Objf378_PhaseShift_FX1` ✓ | 78 `Objf078_Damage_FX2` ✓ | 79 `Objf079_Slay_FX3` |
| 11 | Roman Fire | damage | 80 `Objf080_RomanFire_FX1` ✓ | 344 `Objf344_345_RomanFire_FX2_FX3` ✓ | 345 `Objf344_345_RomanFire_FX2_FX3` |
| 12 | Poison Cloud | poison | 210 `Objf210_SummonGreenCrest` ✓ | 102 `Objf102_227_Poison_FX2` ✓ | 227 `Objf102_227_Poison_FX2` ✓ |
| 13 | Spread Force | damage | 180 `Objf180_SpreadForce_FX1` ✓ | 78 `Objf078_Damage_FX2` ✓ | 79 `Objf079_Slay_FX3` |
| 14 | Avalanche | damage | 184 `Objf184_Avalanche_FX1` ✓ | 338 `Objf317_338_Avalanche_FX2_FX3` ✓ | 317 `Objf317_338_Avalanche_FX2_FX3` |
| 15 | Salamander | damage | 334 `Objf334_Salamander_FX1` ✓ | 132 `Objf132_EngulfUnit` ✓ | 134 `Objf132_EngulfUnit` |
| 16 | Healing | heal | 60 `Objf060_Healing_FX1` ✓ | 100 `Objf100_Healing_FX2` ✓ | *dormant:* 79 `Objf079_Slay_FX3` |
| 17 | Mystic Shield | def up | 60 `Objf060_Healing_FX1` ✓ | 112 `Objf112_MysticShield_FX2` ✓ | *dormant:* 79 `Objf079_Slay_FX3` |
| 18 | Cure | cure | 60 `Objf060_Healing_FX1` ✓ | 104 `Objf104_Cure_FX2` ✓ | *dormant:* 104 `Objf104_Cure_FX2` |
| 19 | Healing Plus | heal | 60 `Objf060_Healing_FX1` ✓ | 791 `Objf306_791_792_793_Healing_FX2` ✓ | *dormant:* 79 `Objf079_Slay_FX3` |
| 20 | Bless Weapon | atk up | 60 `Objf060_Healing_FX1` ✓ | 111 `Objf111_BlessWeapon_FX2` ✓ | *dormant:* 110 `Objf110_CastingStatBuff` |
| 21 | Holy Lightning | damage | 208 `Objf208_HolyLightning_FX1` ✓ | 144 `Objf144_HolyLightning_FX2` ✓ | 146 `Objf146_HolyLightning_FX3` |
| 22 | Ultra Healing | heal | 60 `Objf060_Healing_FX1` ✓ | 792 `Objf306_791_792_793_Healing_FX2` ✓ | *dormant:* 79 `Objf079_Slay_FX3` |
| 23 | Magic Charge | mp transfer | 330 `Objf330_MagicRestoration_FX1` ✓ | 372 `Objf322_370_371_372_MagicRestoration_FX2` ✓ | *dormant:* 106 `Objf106_MagicCharge_FX3` |
| 24 | Holy Pressure | damage | 177 `Objf177_HolyPressure_FX1` ✓ | 78 `Objf078_Damage_FX2` ✓ | 79 `Objf079_Slay_FX3` |
| 25 | Supreme Healing | heal | 60 `Objf060_Healing_FX1` ✓ | 793 `Objf306_791_792_793_Healing_FX2` ✓ | *dormant:* 79 `Objf079_Slay_FX3` |
| 26 | Stone Shower | damage | 163 `Objf163_StoneShower_FX1` ✓ | 165 `Objf165_StoneShower_FX2` ✓ | 166 `Objf166_StoneShower_FX3` |
| 27 | Cure Wide | cure | 60 `Objf060_Healing_FX1` ✓ | 104 `Objf104_Cure_FX2` ✓ | *dormant:* 104 `Objf104_Cure_FX2` |
| 28 | Healing Circle | heal | 381 `Objf381_HealingCircle_FX1` ✓ | 327 `Objf327_HealingCircle_FX2` ✓ | *dormant:* 327 `Objf327_HealingCircle_FX2` |
| 29 | Perfect Guard | agl up | 209 `Objf209_SummonBlueCrest` ✓ | 192 `Objf192_PerfectGuard_FX2` ✓ | *dormant:* 192 `Objf192_PerfectGuard_FX2` |
| 30 | Thunder Flash | damage | 178 `Objf178_ThunderFlash_FX1` ✓ | 78 `Objf078_Damage_FX2` ✓ | 79 `Objf079_Slay_FX3` |
| 31 | Healing Wave | heal | 381 `Objf381_HealingCircle_FX1` ✓ | 327 `Objf327_HealingCircle_FX2` ✓ | *dormant:* 327 `Objf327_HealingCircle_FX2` |
| 32 | Mystic Energy | atk/def up | 60 `Objf060_Healing_FX1` ✓ | 113 `Objf113_MysticEnergy_FX2` ✓ | 143 = empty slot (never fires) |
| 33 | Self Healing | heal | 60 `Objf060_Healing_FX1` ✓ | 100 `Objf100_Healing_FX2` ✓ | *dormant:* 79 `Objf079_Slay_FX3` |
| 34 | Healing | heal | 60 `Objf060_Healing_FX1` ✓ | 100 `Objf100_Healing_FX2` ✓ | *dormant:* 79 `Objf079_Slay_FX3` |
| 35 | Extra Healing | heal | 60 `Objf060_Healing_FX1` ✓ | 306 `Objf306_791_792_793_Healing_FX2` ✓ | *dormant:* 79 `Objf079_Slay_FX3` |
| 36 | Hyper Healing | heal | 60 `Objf060_Healing_FX1` ✓ | 306 `Objf306_791_792_793_Healing_FX2` ✓ | *dormant:* 79 `Objf079_Slay_FX3` |
| 37 | Healing Circle | heal | 381 `Objf381_HealingCircle_FX1` ✓ | 327 `Objf327_HealingCircle_FX2` ✓ | *dormant:* 327 `Objf327_HealingCircle_FX2` |
| 38 | Piercing Light | damage | 161 `Objf161_PiercingLight_FX1` ✓ | 78 `Objf078_Damage_FX2` ✓ | 79 `Objf079_Slay_FX3` |
| 39 | Rainbow Stroke | damage | 176 `Objf176_RainbowStroke_FX1` ✓ | 78 `Objf078_Damage_FX2` ✓ | 79 `Objf079_Slay_FX3` |
| 40 | Magic Arrow | damage | 199 `Objf199_MagicArrow_FX1` ✓ | 78 `Objf078_Damage_FX2` ✓ | 79 `Objf079_Slay_FX3` |
| 41 | Thunder Ball | damage | 224 `Objf224_ThunderBall_FX1` ✓ | 128 `Objf128_ThunderBall_FX2` ✓ | 129 `Objf129_ThunderBall_FX3` |
| 42 | Dark Hurricane | damage | 388 `Objf388_DarkHurricane_FX1` ✓ | 281 `Objf281_282_DarkHurricane_FX2_FX3` ✓ | 282 `Objf281_282_DarkHurricane_FX2_FX3` |
| 43 | Rainbow Storm | damage | 175 `Objf175_RainbowStorm_FX1` ✓ | 78 `Objf078_Damage_FX2` ✓ | 79 `Objf079_Slay_FX3` |
| 44 | Dagger Storm | damage | 92 `Objf092_DaggerStorm_FX1` ✓ | 90 `Objf090_DaggerStorm_FX2` ✓ | 93 `Objf093_DaggerStorm_FX3` |
| 45 | Plasma Wave | damage | 170 `Objf170_PiercingRay_Etc_FX1` ✓ | 78 `Objf078_Damage_FX2` ✓ | 79 `Objf079_Slay_FX3` ✓ |
| 46 | Dark Fire | damage | 122 `Objf122_DarkFire_FX1` ✓ | 132 `Objf132_EngulfUnit` ✓ | 134 `Objf132_EngulfUnit` |
| 47 | Explosion | damage | 158 `Objf158_Explosion_FX1` ✓ | 220 `Objf220_Explosion_FX2` | 221 `Objf221_Explosion_FX3` ✓ |
| 48 | Dynamo Hum | damage | 394 `Objf394_DynamoHum_FX1` ✓ | 136 `Objf132_EngulfUnit` ✓ | 138 `Objf132_EngulfUnit` ✓ |
| 49 | Herb | heal | 60 `Objf060_Healing_FX1` ✓ | 100 `Objf100_Healing_FX2` ✓ | *dormant:* 79 `Objf079_Slay_FX3` |
| 50 | Megaherb | heal | 60 `Objf060_Healing_FX1` ✓ | 100 `Objf100_Healing_FX2` ✓ | *dormant:* 79 `Objf079_Slay_FX3` |
| 51 | Elixir | cure | 60 `Objf060_Healing_FX1` ✓ | 104 `Objf104_Cure_FX2` ✓ | *dormant:* 79 `Objf079_Slay_FX3` |
| 52 | Mage Oil | mp restore | 330 `Objf330_MagicRestoration_FX1` ✓ | 370 `Objf322_370_371_372_MagicRestoration_FX2` ✓ | *dormant:* 79 `Objf079_Slay_FX3` |
| 53 | Mage Gem | mp restore | 330 `Objf330_MagicRestoration_FX1` ✓ | 372 `Objf322_370_371_372_MagicRestoration_FX2` ✓ | *dormant:* 79 `Objf079_Slay_FX3` |
| 54 | Life Orb | heal | 313 `Objf313_LifeOrb_FX1` ✓ | 327 `Objf327_HealingCircle_FX2` ✓ | *dormant:* 79 `Objf079_Slay_FX3` |
| 55 | Holy H2O | heal | 60 `Objf060_Healing_FX1` ✓ | 793 `Objf306_791_792_793_Healing_FX2` ✓ | *dormant:* 100 `Objf100_Healing_FX2` |
| 56 | Fire Gem | damage | 151 `Objf151_FireGem_FX1` ✓ | 132 `Objf132_EngulfUnit` ✓ | 134 `Objf132_EngulfUnit` |
| 57 | MoodRing | damage | 94 `Objf094_MoodRing_FX1` ✓ | 96 `Objf096_MoodRing_FX2` ✓ | 97 `Objf097_MoodRing_FX3` |
| 58 | Aura Gem | damage | 170 `Objf170_PiercingRay_Etc_FX1` ✓ | 78 `Objf078_Damage_FX2` ✓ | 79 `Objf079_Slay_FX3` |
| 59 | WyrmFang | damage | 363 `Objf363_Wyrmfang_FX1` ✓ | 132 `Objf132_EngulfUnit` ✓ | 134 `Objf132_EngulfUnit` |
| 60 | Rolling Thunder | damage | 197 `Objf197_RollingThunder_FX1` ✓ | 195 `Objf195_RollingThunder_FX2` ✓ | 200 `Objf200_RollingThunder_FX3` |
| 61 | Harmful Wave | damage | 210 `Objf210_SummonGreenCrest` ✓ | 108 `Objf108_HarmfulWave_FX2` ✓ | 108 `Objf108_HarmfulWave_FX2` |
| 62 | Evil Stream | damage | 169 `Objf169_EvilStream_FX1` ✓ | 324 `Objf307_324_EvilStream_FX2_FX3` ✓ | 307 `Objf307_324_EvilStream_FX2_FX3` |
| 63 | Mad Book | paralyze | 181 `Objf181_Spellbind_FX1` ✓ | 715 `Objf715_to_718_Spellbind_FX2_FX3` ✓ | 716 `Objf715_to_718_Spellbind_FX2_FX3` |
| 64 | Mushroom | poison | 210 `Objf210_SummonGreenCrest` ✓ | 102 `Objf102_227_Poison_FX2` ✓ | 227 `Objf102_227_Poison_FX2` ✓ |
| 65 | Moon Pie | heal | 60 `Objf060_Healing_FX1` ✓ | 100 `Objf100_Healing_FX2` ✓ | *dormant:* 79 `Objf079_Slay_FX3` |
| 66 | IronBoot | agl up | 209 `Objf209_SummonBlueCrest` ✓ | 192 `Objf192_PerfectGuard_FX2` ✓ | *dormant:* 192 `Objf192_PerfectGuard_FX2` |
| 67 | Unicorn | damage | 175 `Objf175_RainbowStorm_FX1` ✓ | 78 `Objf078_Damage_FX2` ✓ | 79 `Objf079_Slay_FX3` |
| 68 | Kingfoil | heal | 381 `Objf381_HealingCircle_FX1` ✓ | 327 `Objf327_HealingCircle_FX2` ✓ | *dormant:* 327 `Objf327_HealingCircle_FX2` |
| 69 | HelStone | damage | 224 `Objf224_ThunderBall_FX1` ✓ | 128 `Objf128_ThunderBall_FX2` ✓ | 129 `Objf129_ThunderBall_FX3` |
| 70 | ShivBook | damage | 92 `Objf092_DaggerStorm_FX1` ✓ | 90 `Objf090_DaggerStorm_FX2` ✓ | 93 `Objf093_DaggerStorm_FX3` |
| 71 | Ncklace | damage | 388 `Objf388_DarkHurricane_FX1` | 281 `Objf281_282_DarkHurricane_FX2_FX3` | 282 `Objf281_282_DarkHurricane_FX2_FX3` |
