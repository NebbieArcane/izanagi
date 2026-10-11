#pragma once

#include <array>
#include <string>
#include <utility>
#include <vector>

namespace nebbie {

/** Semantic editor control for one object value[] slot (Server: create.obj / stat). */
enum class ObjValueEditorKind {
    unused,
    integer,
    yes_no,
    weapon_damage_type,
    liquid_type,
    spell_number,
};

struct ObjValueSlotSchema {
    std::string label_it;
    std::string tooltip_it;
    ObjValueEditorKind editor = ObjValueEditorKind::unused;
    int min_value = 0;
    int max_value = 0;
};

struct ObjValueTypeSchema {
    std::string legend_it;
    std::array<ObjValueSlotSchema, 4> value_slots{};
};

/** Full schema for myst.obj type_flag (0–31, aligned with NebbieArcane/Server e_item_type). */
ObjValueTypeSchema obj_value_schema_for_type(int type_flag);

std::vector<std::pair<int, std::string>> obj_weapon_damage_type_choices();
std::vector<std::pair<int, std::string>> obj_liquid_type_choices();
std::vector<std::pair<int, std::string>> obj_spell_number_choices();

std::string obj_spell_name(int spell_number);
std::string obj_weapon_damage_type_name(int type_index);
std::string obj_liquid_type_name(int liquid_index);

/** Human summary for a slot value (e.g. spell name, tipo danno). */
std::string obj_value_slot_display_hint(int type_flag, int slot_index, int value);

} // namespace nebbie
