#pragma once

#include "mob_catalog.hpp"

#include <string>
#include <vector>

namespace nebbie {

/** ACT_* bits used for damage simulation (same values as mob_act_flags). */
constexpr long ACT_MAGIC_USER = 2097152L;
constexpr long ACT_WARRIOR = 4194304L;
constexpr long ACT_CLERIC = 8388608L;
constexpr long ACT_THIEF = 16777216L;
constexpr long ACT_DRUID = 33554432L;
constexpr long ACT_MONK = 67108864L;
constexpr long ACT_BARBARIAN = 134217728L;
constexpr long ACT_PALADIN = 268435456L;
constexpr long ACT_RANGER = 536870912L;
constexpr long ACT_PSI = 1073741824L;
constexpr long ACT_ARCHER = static_cast<long>(static_cast<unsigned long>(1) << 31);

/** Melee / physical-style class bits (dam dice + mult_att). */
constexpr long ACT_PHYSICAL_CLASSES =
    ACT_WARRIOR | ACT_THIEF | ACT_DRUID | ACT_MONK | ACT_BARBARIAN | ACT_PALADIN | ACT_RANGER |
    ACT_ARCHER;

/** Caster class bits with a dedicated spell reference. */
constexpr long ACT_CASTER_CLASSES = ACT_MAGIC_USER | ACT_CLERIC | ACT_PSI;

enum class MobDamageRole {
    Fighter = 0,
    Mage = 1,
    Cleric = 2,
    Psionic = 3,
};

struct MobRoundDamageEstimate {
    MobDamageRole role = MobDamageRole::Fighter;
    /** Which ACT bits triggered this component (e.g. "ACT_WARRIOR|ACT_RANGER"). */
    std::string act_labels;
    int min_damage = 0;
    int max_damage = 0;
    double avg_damage = 0.0;
    /** Formula + which mud stats were used. */
    std::string summary;
};

struct MobDamageSimulation {
    std::vector<MobRoundDamageEstimate> components;
    /** Global notes (multi-act, hitroll ignored, etc.). */
    std::string note;
};

std::string mob_damage_role_label(MobDamageRole role);

/**
 * Always includes Fighter (dam dice on file). Adds Mage/Cleric/Psionic when
 * the matching ACT class bits are set.
 */
std::vector<MobDamageRole> collect_mob_damage_roles_from_act(long act_flags);

/** Typical NPC STR band at level (Server: 10 + random(0..max(1,level/5)), capped 18). */
void mob_typical_str_range(int level, int& str_min, int& str_max);

/** Server str_app[].todam for STR 0..18 (plain; no 18/xx exceptional). */
int mob_str_todam(int strength);

/**
 * Simulate one combat round per component.
 * Fighter: only file stats (dam dice x mult_att). Casters: level-based reference spell.
 */
MobDamageSimulation simulate_mob_round_damage(long act_flags, char mobtype, int level, int mult_att,
                                              const DiceValues& dam_dice);

/** Single-role estimate (tests / helpers). */
MobRoundDamageEstimate estimate_mob_round_damage(MobDamageRole role, char mobtype, int level,
                                                 int mult_att, const DiceValues& dam_dice);

} // namespace nebbie
