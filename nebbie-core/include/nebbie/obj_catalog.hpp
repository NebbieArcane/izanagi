#pragma once

#include "mob_catalog.hpp"
#include "types.hpp"

#include <string>
#include <utility>
#include <vector>

namespace nebbie {

/** Match Server autoenums APPLY_* (myst.obj section A locations). */
constexpr int APPLY_SPELLPOWER = 65;
constexpr int APPLY_HITNSP = 66;

std::vector<std::pair<int, std::string>> obj_type_choices();
std::vector<MobFlagDef> obj_wear_flags();
std::vector<MobFlagDef> obj_extra_flags();
std::vector<MobFlagDef> obj_extra_flags2();
std::vector<std::pair<int, std::string>> obj_apply_type_choices();

std::string obj_type_name(int type_flag);
std::string obj_apply_type_name(int location);

/** Sum of modifiers for affects with the given location (0 if none). */
int sum_obj_affect_modifier(const std::vector<ObjAffect>& affects, int location);

/**
 * Set a single-location bonus on the affect list: 0 removes all rows with that
 * location; otherwise keep/update the first row and drop duplicates.
 */
void set_obj_affect_modifier(std::vector<ObjAffect>& affects, int location, int modifier);

} // namespace nebbie
