#pragma once

#include "mob_catalog.hpp"
#include "types.hpp"

#include <string>
#include <utility>
#include <vector>

namespace nebbie {

/** Match Server autoenums APPLY_* (myst.obj section A locations). */
constexpr int APPLY_SEX = 7;
constexpr int APPLY_IMMUNE = 26;
constexpr int APPLY_SUSC = 27;
constexpr int APPLY_M_IMMUNE = 28;
constexpr int APPLY_SPELL = 29;
constexpr int APPLY_WEAPON_SPELL = 30;
constexpr int APPLY_EAT_SPELL = 31;
constexpr int APPLY_HITNDAM = 40;
constexpr int APPLY_AFF2 = 45;
constexpr int APPLY_RACE_SLAYER = 48;
constexpr int APPLY_ALIGN_SLAYER = 49;
constexpr int APPLY_SPELLPOWER = 65;
constexpr int APPLY_HITNSP = 66;

/** Server structs.hpp MAX_OBJ_AFFECT — only this many A sections are loaded. */
constexpr int MAX_OBJ_AFFECT = 5;

/** How the affect modifier should be edited in the GUI. */
enum class ObjAffectModifierKind {
    integer,
    immunity_bits,
    spell_affect_bits,
    spell_affect2_bits,
    spell_number,
    race,
    sex,
    align_bits,
};

std::vector<std::pair<int, std::string>> obj_type_choices();
std::vector<MobFlagDef> obj_wear_flags();
std::vector<MobFlagDef> obj_extra_flags();
std::vector<MobFlagDef> obj_extra_flags2();
/** APPLY_SPELL / AFF bits (same table as mob affected_by). */
std::vector<MobFlagDef> obj_spell_affect_flags();
/** APPLY_AFF2 bits. */
std::vector<MobFlagDef> obj_spell_affect2_flags();
/** APPLY_ALIGN_SLAYER: GOOD=1 NEUTRAL=2 EVIL=4. */
std::vector<MobFlagDef> obj_align_slayer_flags();

/** Technical short names (stable for tests / logs). */
std::vector<std::pair<int, std::string>> obj_apply_type_choices();
/** Italian readable labels for the affect type dropdown. */
std::vector<std::pair<int, std::string>> obj_apply_type_choices_ui();

std::string obj_type_name(int type_flag);
std::string obj_apply_type_name(int location);
std::string obj_apply_type_label(int location);
ObjAffectModifierKind obj_affect_modifier_kind(int location);
/** Human-readable affect row for list widgets. */
std::string format_obj_affect_label(int location, int modifier);

/** Sum of modifiers for affects with the given location (0 if none). */
int sum_obj_affect_modifier(const std::vector<ObjAffect>& affects, int location);

/**
 * Set a single-location bonus on the affect list: 0 removes all rows with that
 * location; otherwise keep/update the first row and drop duplicates.
 */
void set_obj_affect_modifier(std::vector<ObjAffect>& affects, int location, int modifier);

/**
 * Prepare affects for .obj write like the mud: bitmask APPLY_* rows with the
 * same location are OR-merged into one modifier (never written as 1|2|4), then
 * the list is capped at {@link MAX_OBJ_AFFECT}.
 */
void normalize_obj_affects_for_mud_save(std::vector<ObjAffect>& affects);

} // namespace nebbie
