#include "nebbie/obj_value_catalog.hpp"

#include <cassert>
#include <iostream>

int main() {
    const nebbie::ObjValueTypeSchema weapon = nebbie::obj_value_schema_for_type(5);
    assert(weapon.value_slots[3].editor == nebbie::ObjValueEditorKind::weapon_damage_type);
    assert(!weapon.legend_it.empty());

    const nebbie::ObjValueTypeSchema armor = nebbie::obj_value_schema_for_type(9);
    assert(armor.value_slots[0].editor == nebbie::ObjValueEditorKind::integer);
    assert(armor.value_slots[2].editor == nebbie::ObjValueEditorKind::unused);

    const std::string spell = nebbie::obj_spell_name(1);
    assert(spell == "armor");

    const std::string hint = nebbie::obj_value_slot_display_hint(5, 3, 3);
    assert(hint == "SLASH");

    std::cout << "obj_value_catalog ok\n";
    return 0;
}
