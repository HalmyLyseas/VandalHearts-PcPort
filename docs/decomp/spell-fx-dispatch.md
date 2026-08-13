# Spell FX dispatch (`gSpellsEx`)

How the game turns "unit casts spell N" into visuals — decoded from the retail data and
validated live: an instrumented build logged every FX dispatch across all 70 castable
spells/items in-game, and all 221 dispatches matched this table.

## The model

Spell visuals have **no static spawn sites**. When the spell executor
(`Objf028_UnitCasting`, `src/battle/executors.c`) commits a cast, it dispatches object
handlers data-driven out of `gSpellsEx[spellId]` (73 rows of 5 `s16`, ids 1–71 live):

| field | meaning |
|---|---|
| `SPELL_EX_OBJF_MAIN` | the **FX1** handler — the main casting visual, spawned once at the caster/target |
| `SPELL_EX_OBJF_TARGET` | the **FX2** handler — spawned per surviving target |
| `SPELL_EX_OBJF_DEFEAT` | the **FX3** handler — spawned per slain target |
| `SPELL_EX_MP_BONUS` | MP cost/bonus datum |
| `SPELL_EX_EFFECT` | effect class (branched on by `CalculateSpellPowerAndExp`, `src/battle/math.c`) |

Handler names carry the slot as a suffix (`_FX1`/`_FX2`/`_FX3`); one function can serve
several table indices, visible in the name (`Objf344_345_RomanFire_FX2_FX3`). FX1 drivers
own the completion handshake: they raise `gSignal3` when the presentation is done and the
executor may proceed; child objects never do.

Notable rows:

- Many support spells share `Objf060_Healing_FX1`, a stub that raises `gSignal3`
  immediately — the per-target FX2 carries the whole visual.
- The ENGULF family: `Objf132_EngulfUnit` is one handler serving indices 132–142 plus
  799/800, parameterized by which index spawned it (element pairs + damage/slay variant).
- Spells 1, 4 and 32 name **defeat slot 143, a NULL table entry**. Harmless in retail:
  all three are buffs and can never slay a target, so the slot never dispatches.
- Spell 18/27 (Cure/Cure Wide) reuse the FX2 handler in the defeat slot — curing a unit
  to death is impossible, the row just mirrors the pair.

## The retail table

Generated from the retail binary (`gSpellsEx` + `gSpellNames`); handler locations are in
[objf-handlers.md](objf-handlers.md).

| id | spell | MAIN (FX1) | TARGET (FX2) | DEFEAT (FX3) |
|---:|---|---|---|---|
| 1 | Faerie Light | `Objf060_Healing_FX1` | `Objf115_Faerie_FX2` | null slot 143 (never fires) |
| 2 | Ice Storm | `Objf189_IceStorm_FX1` | `Objf078_Damage_FX2` | `Objf079_Slay_FX3` |
| 3 | Rolling Fire | `Objf332_RollingFire_FX1` | `Objf132_EngulfUnit` | `Objf132_EngulfUnit` |
| 4 | Faerie Star | `Objf060_Healing_FX1` | `Objf115_Faerie_FX2` | null slot 143 (never fires) |
| 5 | Delta Mirage | `Objf156_DeltaMirage_FX1` | `Objf132_EngulfUnit` | `Objf132_EngulfUnit` |
| 6 | Dark Star | `Objf207_SummonRedCrest` | `Objf193_DarkStar_FX2` | `Objf194_DarkStar_FX3` |
| 7 | Spellbind | `Objf181_Spellbind_FX1` | `Objf715_to_718_Spellbind_FX2_FX3` | `Objf715_to_718_Spellbind_FX2_FX3` |
| 8 | Piercing Ray | `Objf170_PiercingRay_Etc_FX1` | `Objf078_Damage_FX2` | `Objf079_Slay_FX3` |
| 9 | Envenom | `Objf210_SummonGreenCrest` | `Objf102_227_Poison_FX2` | `Objf102_227_Poison_FX2` |
| 10 | Phase Shift | `Objf378_PhaseShift_FX1` | `Objf078_Damage_FX2` | `Objf079_Slay_FX3` |
| 11 | Roman Fire | `Objf080_RomanFire_FX1` | `Objf344_345_RomanFire_FX2_FX3` | `Objf344_345_RomanFire_FX2_FX3` |
| 12 | Poison Cloud | `Objf210_SummonGreenCrest` | `Objf102_227_Poison_FX2` | `Objf102_227_Poison_FX2` |
| 13 | Spread Force | `Objf180_SpreadForce_FX1` | `Objf078_Damage_FX2` | `Objf079_Slay_FX3` |
| 14 | Avalanche | `Objf184_Avalanche_FX1` | `Objf317_338_Avalanche_FX2_FX3` | `Objf317_338_Avalanche_FX2_FX3` |
| 15 | Salamander | `Objf334_Salamander_FX1` | `Objf132_EngulfUnit` | `Objf132_EngulfUnit` |
| 16 | Healing | `Objf060_Healing_FX1` | `Objf100_Healing_FX2` | `Objf079_Slay_FX3` |
| 17 | Mystic Shield | `Objf060_Healing_FX1` | `Objf112_MysticShield_FX2` | `Objf079_Slay_FX3` |
| 18 | Cure | `Objf060_Healing_FX1` | `Objf104_Cure_FX2` | `Objf104_Cure_FX2` |
| 19 | Healing Plus | `Objf060_Healing_FX1` | `Objf306_791_792_793_Healing_FX2` | `Objf079_Slay_FX3` |
| 20 | Bless Weapon | `Objf060_Healing_FX1` | `Objf111_BlessWeapon_FX2` | `Objf110_CastingStatBuff` |
| 21 | Holy Lightning | `Objf208_HolyLightning_FX1` | `Objf144_HolyLightning_FX2` | `Objf146_HolyLightning_FX3` |
| 22 | Ultra Healing | `Objf060_Healing_FX1` | `Objf306_791_792_793_Healing_FX2` | `Objf079_Slay_FX3` |
| 23 | Magic Charge | `Objf330_MagicRestoration_FX1` | `Objf322_370_371_372_MagicRestoration_FX2` | `Objf106_MagicCharge_FX3` |
| 24 | Holy Pressure | `Objf177_HolyPressure_FX1` | `Objf078_Damage_FX2` | `Objf079_Slay_FX3` |
| 25 | Supreme Healing | `Objf060_Healing_FX1` | `Objf306_791_792_793_Healing_FX2` | `Objf079_Slay_FX3` |
| 26 | Stone Shower | `Objf163_StoneShower_FX1` | `Objf165_StoneShower_FX2` | `Objf166_StoneShower_FX3` |
| 27 | Cure Wide | `Objf060_Healing_FX1` | `Objf104_Cure_FX2` | `Objf104_Cure_FX2` |
| 28 | Healing Circle | `Objf381_HealingCircle_FX1` | `Objf327_HealingCircle_FX2` | `Objf327_HealingCircle_FX2` |
| 29 | Perfect Guard | `Objf209_SummonBlueCrest` | `Objf192_PerfectGuard_FX2` | `Objf192_PerfectGuard_FX2` |
| 30 | Thunder Flash | `Objf178_ThunderFlash_FX1` | `Objf078_Damage_FX2` | `Objf079_Slay_FX3` |
| 31 | Healing Wave | `Objf381_HealingCircle_FX1` | `Objf327_HealingCircle_FX2` | `Objf327_HealingCircle_FX2` |
| 32 | Mystic Energy | `Objf060_Healing_FX1` | `Objf113_MysticEnergy_FX2` | null slot 143 (never fires) |
| 33 | Self Healing | `Objf060_Healing_FX1` | `Objf100_Healing_FX2` | `Objf079_Slay_FX3` |
| 34 | Healing | `Objf060_Healing_FX1` | `Objf100_Healing_FX2` | `Objf079_Slay_FX3` |
| 35 | Extra Healing | `Objf060_Healing_FX1` | `Objf306_791_792_793_Healing_FX2` | `Objf079_Slay_FX3` |
| 36 | Hyper Healing | `Objf060_Healing_FX1` | `Objf306_791_792_793_Healing_FX2` | `Objf079_Slay_FX3` |
| 37 | Healing Circle | `Objf381_HealingCircle_FX1` | `Objf327_HealingCircle_FX2` | `Objf327_HealingCircle_FX2` |
| 38 | Piercing Light | `Objf161_PiercingLight_FX1` | `Objf078_Damage_FX2` | `Objf079_Slay_FX3` |
| 39 | Rainbow Stroke | `Objf176_RainbowStroke_FX1` | `Objf078_Damage_FX2` | `Objf079_Slay_FX3` |
| 40 | Magic Arrow | `Objf199_MagicArrow_FX1` | `Objf078_Damage_FX2` | `Objf079_Slay_FX3` |
| 41 | Thunder Ball | `Objf224_ThunderBall_FX1` | `Objf128_ThunderBall_FX2` | `Objf129_ThunderBall_FX3` |
| 42 | Dark Hurricane | `Objf388_DarkHurricane_FX1` | `Objf281_282_DarkHurricane_FX2_FX3` | `Objf281_282_DarkHurricane_FX2_FX3` |
| 43 | Rainbow Storm | `Objf175_RainbowStorm_FX1` | `Objf078_Damage_FX2` | `Objf079_Slay_FX3` |
| 44 | Dagger Storm | `Objf092_DaggerStorm_FX1` | `Objf090_DaggerStorm_FX2` | `Objf093_DaggerStorm_FX3` |
| 45 | Plasma Wave | `Objf170_PiercingRay_Etc_FX1` | `Objf078_Damage_FX2` | `Objf079_Slay_FX3` |
| 46 | Dark Fire | `Objf122_DarkFire_FX1` | `Objf132_EngulfUnit` | `Objf132_EngulfUnit` |
| 47 | Explosion | `Objf158_Explosion_FX1` | `Objf220_Explosion_FX2` | `Objf221_Explosion_FX3` |
| 48 | Dynamo Hum | `Objf394_DynamoHum_FX1` | `Objf132_EngulfUnit` | `Objf132_EngulfUnit` |
| 49 | Herb | `Objf060_Healing_FX1` | `Objf100_Healing_FX2` | `Objf079_Slay_FX3` |
| 50 | Megaherb | `Objf060_Healing_FX1` | `Objf100_Healing_FX2` | `Objf079_Slay_FX3` |
| 51 | Elixir | `Objf060_Healing_FX1` | `Objf104_Cure_FX2` | `Objf079_Slay_FX3` |
| 52 | Mage Oil | `Objf330_MagicRestoration_FX1` | `Objf322_370_371_372_MagicRestoration_FX2` | `Objf079_Slay_FX3` |
| 53 | Mage Gem | `Objf330_MagicRestoration_FX1` | `Objf322_370_371_372_MagicRestoration_FX2` | `Objf079_Slay_FX3` |
| 54 | Life Orb | `Objf313_LifeOrb_FX1` | `Objf327_HealingCircle_FX2` | `Objf079_Slay_FX3` |
| 55 | Holy H2O | `Objf060_Healing_FX1` | `Objf306_791_792_793_Healing_FX2` | `Objf100_Healing_FX2` |
| 56 | Fire Gem | `Objf151_FireGem_FX1` | `Objf132_EngulfUnit` | `Objf132_EngulfUnit` |
| 57 | MoodRing | `Objf094_MoodRing_FX1` | `Objf096_MoodRing_FX2` | `Objf097_MoodRing_FX3` |
| 58 | Aura Gem | `Objf170_PiercingRay_Etc_FX1` | `Objf078_Damage_FX2` | `Objf079_Slay_FX3` |
| 59 | WyrmFang | `Objf363_Wyrmfang_FX1` | `Objf132_EngulfUnit` | `Objf132_EngulfUnit` |
| 60 | Rolling Thunder | `Objf197_RollingThunder_FX1` | `Objf195_RollingThunder_FX2` | `Objf200_RollingThunder_FX3` |
| 61 | Harmful Wave | `Objf210_SummonGreenCrest` | `Objf108_HarmfulWave_FX2` | `Objf108_HarmfulWave_FX2` |
| 62 | Evil Stream | `Objf169_EvilStream_FX1` | `Objf307_324_EvilStream_FX2_FX3` | `Objf307_324_EvilStream_FX2_FX3` |
| 63 | Mad Book | `Objf181_Spellbind_FX1` | `Objf715_to_718_Spellbind_FX2_FX3` | `Objf715_to_718_Spellbind_FX2_FX3` |
| 64 | Mushroom | `Objf210_SummonGreenCrest` | `Objf102_227_Poison_FX2` | `Objf102_227_Poison_FX2` |
| 65 | Moon Pie | `Objf060_Healing_FX1` | `Objf100_Healing_FX2` | `Objf079_Slay_FX3` |
| 66 | IronBoot | `Objf209_SummonBlueCrest` | `Objf192_PerfectGuard_FX2` | `Objf192_PerfectGuard_FX2` |
| 67 | Unicorn | `Objf175_RainbowStorm_FX1` | `Objf078_Damage_FX2` | `Objf079_Slay_FX3` |
| 68 | Kingfoil | `Objf381_HealingCircle_FX1` | `Objf327_HealingCircle_FX2` | `Objf327_HealingCircle_FX2` |
| 69 | HelStone | `Objf224_ThunderBall_FX1` | `Objf128_ThunderBall_FX2` | `Objf129_ThunderBall_FX3` |
| 70 | ShivBook | `Objf092_DaggerStorm_FX1` | `Objf090_DaggerStorm_FX2` | `Objf093_DaggerStorm_FX3` |
| 71 | Ncklace | `Objf388_DarkHurricane_FX1` | `Objf281_282_DarkHurricane_FX2_FX3` | `Objf281_282_DarkHurricane_FX2_FX3` |
