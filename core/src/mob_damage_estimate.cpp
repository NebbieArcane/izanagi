#include "eleutheria/mob_damage_estimate.hpp"

#include <algorithm>
#include <cstdio>
#include <sstream>

namespace eleutheria {
namespace {

void set_range(MobRoundDamageEstimate& out, int min_v, int max_v, double avg_v) {
  out.min_damage = min_v;
  out.max_damage = max_v;
  out.avg_damage = avg_v;
}

int fighter_attacks(char mobtype, int mult_att) {
  if(!mob_type_uses_mult_att(mobtype)) {
    return 1;
  }
  return std::max(1, mult_att);
}

std::string format_dice_short(const DiceValues& dam_dice) {
  std::ostringstream oss;
  oss << std::max(0, dam_dice.number) << 'd' << std::max(1, dam_dice.size);
  if(dam_dice.plus > 0) {
    oss << '+' << dam_dice.plus;
  } else if(dam_dice.plus < 0) {
    oss << dam_dice.plus;
  }
  return oss.str();
}

void append_physical_act_labels(std::int64_t act_flags, std::vector<const char*>* labels) {
  if((act_flags & kActWarrior) != 0) {
    labels->push_back("WARRIOR");
  }
  if((act_flags & kActThief) != 0) {
    labels->push_back("THIEF");
  }
  if((act_flags & kActDruid) != 0) {
    labels->push_back("DRUID");
  }
  if((act_flags & kActMonk) != 0) {
    labels->push_back("MONK");
  }
  if((act_flags & kActBarbarian) != 0) {
    labels->push_back("BARBARIAN");
  }
  if((act_flags & kActPaladin) != 0) {
    labels->push_back("PALADIN");
  }
  if((act_flags & kActRanger) != 0) {
    labels->push_back("RANGER");
  }
  if((act_flags & kActArcher) != 0) {
    labels->push_back("ARCHER");
  }
}

std::string join_labels(const std::vector<const char*>& labels) {
  if(labels.empty()) {
    return {};
  }
  std::ostringstream oss;
  for(std::size_t i = 0; i < labels.size(); ++i) {
    if(i > 0) {
      oss << '+';
    }
    oss << labels[i];
  }
  return oss.str();
}

MobRoundDamageEstimate estimate_fighter(char mobtype, int /*level*/, int mult_att,
                                        const DiceValues& dam_dice) {
  MobRoundDamageEstimate out;
  out.role = MobDamageRole::Fighter;

  const int number = std::max(0, dam_dice.number);
  const int size = std::max(1, dam_dice.size);
  const int plus = dam_dice.plus;
  const int attacks = fighter_attacks(mobtype, mult_att);

  const int hit_min = number * 1 + plus;
  const int hit_max = number * size + plus;
  const double hit_avg =
      static_cast<double>(number) * (static_cast<double>(size) + 1.0) / 2.0 +
      static_cast<double>(plus);

  set_range(out, hit_min * attacks, hit_max * attacks, hit_avg * static_cast<double>(attacks));

  std::ostringstream oss;
  oss << attacks << "x " << format_dice_short(dam_dice);
  out.summary = oss.str();
  return out;
}

MobRoundDamageEstimate estimate_disintegrate(int level) {
  MobRoundDamageEstimate out;
  out.role = MobDamageRole::Mage;
  const int lvl = std::max(0, level);
  set_range(out, lvl * 1, lvl * 10, static_cast<double>(lvl) * 5.5);
  std::ostringstream oss;
  oss << lvl << "d10";
  out.summary = oss.str();
  return out;
}

MobRoundDamageEstimate estimate_harm(int /*level*/) {
  MobRoundDamageEstimate out;
  out.role = MobDamageRole::Cleric;
  set_range(out, 100, 100, 100.0);
  out.summary = "cap 100";
  return out;
}

MobRoundDamageEstimate estimate_psychic_crush(int level) {
  MobRoundDamageEstimate out;
  out.role = MobDamageRole::Psionic;
  const int lvl = std::max(0, level);
  const int half = lvl / 2;
  set_range(out, lvl * 1 + half, lvl * 6 + half,
            static_cast<double>(lvl) * 3.5 + static_cast<double>(half));
  std::ostringstream oss;
  oss << lvl << "d6+" << half;
  out.summary = oss.str();
  return out;
}

}  // namespace

bool mob_type_uses_mult_att(char mobtype) {
  return mobtype == 'A' || mobtype == 'B' || mobtype == 'L';
}

std::string mob_damage_role_label(MobDamageRole role) {
  switch(role) {
    case MobDamageRole::Fighter:
      return "Fisico";
    case MobDamageRole::Mage:
      return "Mago";
    case MobDamageRole::Cleric:
      return "Chierico";
    case MobDamageRole::Psionic:
      return "Psionico";
  }
  return "Fisico";
}

std::vector<MobDamageRole> collect_mob_damage_roles_from_act(std::int64_t act_flags) {
  std::vector<MobDamageRole> roles;
  roles.push_back(MobDamageRole::Fighter);
  if((act_flags & kActMagicUser) != 0) {
    roles.push_back(MobDamageRole::Mage);
  }
  if((act_flags & kActCleric) != 0) {
    roles.push_back(MobDamageRole::Cleric);
  }
  if((act_flags & kActPsi) != 0) {
    roles.push_back(MobDamageRole::Psionic);
  }
  return roles;
}

MobRoundDamageEstimate estimate_mob_round_damage(MobDamageRole role, char mobtype, int level,
                                                 int mult_att, const DiceValues& dam_dice) {
  switch(role) {
    case MobDamageRole::Fighter:
      return estimate_fighter(mobtype, level, mult_att, dam_dice);
    case MobDamageRole::Mage:
      return estimate_disintegrate(level);
    case MobDamageRole::Cleric:
      return estimate_harm(level);
    case MobDamageRole::Psionic:
      return estimate_psychic_crush(level);
  }
  return estimate_fighter(mobtype, level, mult_att, dam_dice);
}

MobDamageSimulation simulate_mob_round_damage(std::int64_t act_flags, char mobtype, int level,
                                              int mult_att, const DiceValues& dam_dice) {
  MobDamageSimulation sim;
  for(const MobDamageRole role : collect_mob_damage_roles_from_act(act_flags)) {
    MobRoundDamageEstimate est =
        estimate_mob_round_damage(role, mobtype, level, mult_att, dam_dice);
    std::vector<const char*> labels;
    switch(role) {
      case MobDamageRole::Fighter:
        append_physical_act_labels(act_flags, &labels);
        break;
      case MobDamageRole::Mage:
        labels.push_back("MAGIC-USER");
        break;
      case MobDamageRole::Cleric:
        labels.push_back("CLERIC");
        break;
      case MobDamageRole::Psionic:
        labels.push_back("PSI");
        break;
    }
    est.act_labels = join_labels(labels);
    sim.components.push_back(std::move(est));
  }
  return sim;
}

DiceValues parse_dice_text(const std::string& text) {
  DiceValues d;
  int n = 1;
  int s = 1;
  int b = 0;
  if(std::sscanf(text.c_str(), "%dd%d+%d", &n, &s, &b) >= 2 ||
     std::sscanf(text.c_str(), "%dd%d%d", &n, &s, &b) >= 2 ||
     std::sscanf(text.c_str(), "%dd%d", &n, &s) >= 2) {
    d.number = n;
    d.size = s;
    d.plus = b;
  }
  return d;
}

}  // namespace eleutheria
