#include "nebbie/mob_damage_estimate.hpp"

#include <algorithm>
#include <sstream>

namespace nebbie {

namespace {

// Server constants.cpp str_app[0..18].todam (kept for helpers/tests; not used in fighter estimate).
constexpr int kStrTodam[] = {
    -4, -4, -2, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 2,
};

void set_range(MobRoundDamageEstimate& out, const int min_v, const int max_v, const double avg_v) {
    out.min_damage = min_v;
    out.max_damage = max_v;
    out.avg_damage = avg_v;
}

int fighter_attacks(const char mobtype, const int mult_att) {
    if (!mob_type_uses_mult_att(mobtype)) {
        return 1;
    }
    return std::max(1, mult_att);
}

std::string format_dice_short(const DiceValues& dam_dice) {
    std::ostringstream oss;
    oss << std::max(0, dam_dice.number) << 'd' << std::max(1, dam_dice.size);
    if (dam_dice.plus > 0) {
        oss << '+' << dam_dice.plus;
    } else if (dam_dice.plus < 0) {
        oss << dam_dice.plus;
    }
    return oss.str();
}

void append_physical_act_labels(const long act_flags, std::vector<const char*>& labels) {
    if ((act_flags & ACT_WARRIOR) != 0) {
        labels.push_back("WARRIOR");
    }
    if ((act_flags & ACT_THIEF) != 0) {
        labels.push_back("THIEF");
    }
    if ((act_flags & ACT_DRUID) != 0) {
        labels.push_back("DRUID");
    }
    if ((act_flags & ACT_MONK) != 0) {
        labels.push_back("MONK");
    }
    if ((act_flags & ACT_BARBARIAN) != 0) {
        labels.push_back("BARBARIAN");
    }
    if ((act_flags & ACT_PALADIN) != 0) {
        labels.push_back("PALADIN");
    }
    if ((act_flags & ACT_RANGER) != 0) {
        labels.push_back("RANGER");
    }
    if ((act_flags & ACT_ARCHER) != 0) {
        labels.push_back("ARCHER");
    }
}

std::string join_labels(const std::vector<const char*>& labels) {
    if (labels.empty()) {
        return {};
    }
    std::ostringstream oss;
    for (std::size_t i = 0; i < labels.size(); ++i) {
        if (i > 0) {
            oss << '+';
        }
        oss << labels[i];
    }
    return oss.str();
}

MobRoundDamageEstimate estimate_fighter(const char mobtype, const int /*level*/, const int mult_att,
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

MobRoundDamageEstimate estimate_disintegrate(const int level) {
    MobRoundDamageEstimate out;
    out.role = MobDamageRole::Mage;
    const int lvl = std::max(0, level);
    set_range(out, lvl * 1, lvl * 10, static_cast<double>(lvl) * 5.5);
    std::ostringstream oss;
    oss << lvl << "d10";
    out.summary = oss.str();
    return out;
}

MobRoundDamageEstimate estimate_harm(const int /*level*/) {
    MobRoundDamageEstimate out;
    out.role = MobDamageRole::Cleric;
    set_range(out, 100, 100, 100.0);
    out.summary = "cap 100";
    return out;
}

MobRoundDamageEstimate estimate_psychic_crush(const int level) {
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

} // namespace

std::string mob_damage_role_label(const MobDamageRole role) {
    switch (role) {
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

void mob_typical_str_range(const int level, int& str_min, int& str_max) {
    const int lvl = std::max(0, level);
    const int span = std::max(1, lvl / 5);
    str_min = 10;
    str_max = std::min(10 + span, 18);
}

int mob_str_todam(const int strength) {
    if (strength < 0) {
        return kStrTodam[0];
    }
    if (strength > 18) {
        return kStrTodam[18];
    }
    return kStrTodam[strength];
}

std::vector<MobDamageRole> collect_mob_damage_roles_from_act(const long act_flags) {
    // Always include physical: dam dice / mult_att are on the mob file.
    std::vector<MobDamageRole> roles;
    roles.push_back(MobDamageRole::Fighter);
    if ((act_flags & ACT_MAGIC_USER) != 0) {
        roles.push_back(MobDamageRole::Mage);
    }
    if ((act_flags & ACT_CLERIC) != 0) {
        roles.push_back(MobDamageRole::Cleric);
    }
    if ((act_flags & ACT_PSI) != 0) {
        roles.push_back(MobDamageRole::Psionic);
    }
    return roles;
}

MobRoundDamageEstimate estimate_mob_round_damage(const MobDamageRole role, const char mobtype,
                                                 const int level, const int mult_att,
                                                 const DiceValues& dam_dice) {
    switch (role) {
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

MobDamageSimulation simulate_mob_round_damage(const long act_flags, const char mobtype,
                                              const int level, const int mult_att,
                                              const DiceValues& dam_dice) {
    MobDamageSimulation sim;
    for (const MobDamageRole role : collect_mob_damage_roles_from_act(act_flags)) {
        MobRoundDamageEstimate est =
            estimate_mob_round_damage(role, mobtype, level, mult_att, dam_dice);
        std::vector<const char*> labels;
        switch (role) {
        case MobDamageRole::Fighter:
            append_physical_act_labels(act_flags, labels);
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
    sim.note.clear();
    return sim;
}

} // namespace nebbie
