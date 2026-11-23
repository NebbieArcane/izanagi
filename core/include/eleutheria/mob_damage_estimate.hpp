#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace eleutheria {

struct DiceValues {
  int number = 1;
  int size = 1;
  int plus = 0;
};

/** ACT_* bits usati per la simulazione danni (allineati al catalogo). */
constexpr std::int64_t kActMagicUser = 1LL << 21;
constexpr std::int64_t kActWarrior = 1LL << 22;
constexpr std::int64_t kActCleric = 1LL << 23;
constexpr std::int64_t kActThief = 1LL << 24;
constexpr std::int64_t kActDruid = 1LL << 25;
constexpr std::int64_t kActMonk = 1LL << 26;
constexpr std::int64_t kActBarbarian = 1LL << 27;
constexpr std::int64_t kActPaladin = 1LL << 28;
constexpr std::int64_t kActRanger = 1LL << 29;
constexpr std::int64_t kActPsi = 1LL << 30;
constexpr std::int64_t kActArcher = 1LL << 31;

enum class MobDamageRole {
  Fighter = 0,
  Mage = 1,
  Cleric = 2,
  Psionic = 3,
};

struct MobRoundDamageEstimate {
  MobDamageRole role = MobDamageRole::Fighter;
  std::string act_labels;
  int min_damage = 0;
  int max_damage = 0;
  double avg_damage = 0.0;
  std::string summary;
};

struct MobDamageSimulation {
  std::vector<MobRoundDamageEstimate> components;
  std::string note;
};

std::string mob_damage_role_label(MobDamageRole role);

bool mob_type_uses_mult_att(char mobtype);

std::vector<MobDamageRole> collect_mob_damage_roles_from_act(std::int64_t act_flags);

MobRoundDamageEstimate estimate_mob_round_damage(MobDamageRole role, char mobtype, int level,
                                                 int mult_att, const DiceValues& dam_dice);

MobDamageSimulation simulate_mob_round_damage(std::int64_t act_flags, char mobtype, int level,
                                              int mult_att, const DiceValues& dam_dice);

DiceValues parse_dice_text(const std::string& text);

}  // namespace eleutheria
