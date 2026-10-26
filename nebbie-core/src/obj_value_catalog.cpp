#include "nebbie/obj_value_catalog.hpp"

#include "nebbie/obj_catalog.hpp"

#include <cstdio>

namespace nebbie {

namespace {

constexpr int kItemLight = 1;
constexpr int kItemScroll = 2;
constexpr int kItemWand = 3;
constexpr int kItemStaff = 4;
constexpr int kItemWeapon = 5;
constexpr int kItemFireweapon = 6;
constexpr int kItemMissile = 7;
constexpr int kItemArmor = 9;
constexpr int kItemPotion = 10;
constexpr int kItemTrap = 14;
constexpr int kItemContainer = 15;
constexpr int kItemNote = 16;
constexpr int kItemDrinkcon = 17;
constexpr int kItemKey = 18;
constexpr int kItemFood = 19;
constexpr int kItemMoney = 20;
constexpr int kItemClanSymbol = 31;

/** Slot label style: "value[N]  EnglishName" (matches OLC / stat wording). */
std::string value_label(const int index, const char* english_name) {
    return "value[" + std::to_string(index) + "]  " + english_name;
}

ObjValueSlotSchema unused_slot(const int index) {
    ObjValueSlotSchema s;
    s.label_it = value_label(index, "unused");
    s.tooltip_it = "Value not used for this item type (leave 0).";
    s.editor = ObjValueEditorKind::unused;
    return s;
}

ObjValueSlotSchema int_slot(const int index, const char* english_name, const std::string& tooltip,
                            const int min_v = 0, const int max_v = 2000000000) {
    ObjValueSlotSchema s;
    s.label_it = value_label(index, english_name);
    s.tooltip_it = tooltip;
    s.editor = ObjValueEditorKind::integer;
    s.min_value = min_v;
    s.max_value = max_v;
    return s;
}

ObjValueSlotSchema yes_no_slot(const int index, const char* english_name, const std::string& tooltip) {
    ObjValueSlotSchema s;
    s.label_it = value_label(index, english_name);
    s.tooltip_it = tooltip + " 0 = No, 1 = Yes (OLC).";
    s.editor = ObjValueEditorKind::yes_no;
    s.min_value = 0;
    s.max_value = 1;
    return s;
}

ObjValueSlotSchema spell_slot(const int index, const char* english_name, const std::string& tooltip) {
    ObjValueSlotSchema s;
    s.label_it = value_label(index, english_name);
    s.tooltip_it = tooltip;
    s.editor = ObjValueEditorKind::spell_number;
    s.min_value = 0;
    s.max_value = 999;
    return s;
}

ObjValueTypeSchema schema_all_unused(const std::string& legend) {
    ObjValueTypeSchema schema;
    schema.legend_it = legend;
    for (int i = 0; i < 4; ++i) {
        schema.value_slots[i] = unused_slot(i);
    }
    return schema;
}

const char* kWeaponTypes[] = {
    "SMITE", "STAB", "WHIP", "SLASH", "SMASH", "CLEAVE", "CRUSH", "BLUDGEON",
    "CLAW", "BITE", "STING", "PIERCE", "BLAST", "RANGE_WEAPON",
};

/** English drink names aligned with Server drinks[] / OLC list. */
const char* kLiquidNamesEn[] = {
    "water", "beer", "wine", "ale", "dark ale", "whisky", "lemonade", "firebreather",
    "speciality", "slime mold juice", "milk", "tea", "coffee", "blood", "salt water", "coca cola",
};

struct SpellEntry {
    int number;
    const char* name;
};

static const SpellEntry kSpells[] = {
#include "obj_spell_catalog_data.inc"
};

} // namespace

std::vector<std::pair<int, std::string>> obj_weapon_damage_type_choices() {
    std::vector<std::pair<int, std::string>> out;
    const std::size_t count = sizeof(kWeaponTypes) / sizeof(kWeaponTypes[0]);
    for (std::size_t i = 0; i < count; ++i) {
        std::string label = std::to_string(static_cast<int>(i)) + " — " + kWeaponTypes[i];
        out.emplace_back(static_cast<int>(i), label);
    }
    return out;
}

std::vector<std::pair<int, std::string>> obj_liquid_type_choices() {
    std::vector<std::pair<int, std::string>> out;
    const std::size_t count = sizeof(kLiquidNamesEn) / sizeof(kLiquidNamesEn[0]);
    for (std::size_t i = 0; i < count; ++i) {
        std::string label = std::to_string(static_cast<int>(i)) + " — " + kLiquidNamesEn[i];
        out.emplace_back(static_cast<int>(i), label);
    }
    return out;
}

std::vector<std::pair<int, std::string>> obj_spell_number_choices() {
    std::vector<std::pair<int, std::string>> out;
    out.emplace_back(0, "0 — (none)");
    for (const SpellEntry& spell : kSpells) {
        if (spell.number <= 0 || spell.name == nullptr || spell.name[0] == '\0') {
            continue;
        }
        std::string label = std::to_string(spell.number) + " — " + spell.name;
        out.emplace_back(spell.number, label);
    }
    return out;
}

std::string obj_spell_name(const int spell_number) {
    if (spell_number <= 0) {
        return "(none)";
    }
    for (const SpellEntry& spell : kSpells) {
        if (spell.number == spell_number) {
            return spell.name;
        }
    }
    return "spell #" + std::to_string(spell_number);
}

std::string obj_weapon_damage_type_name(const int type_index) {
    if (type_index >= 0 && static_cast<std::size_t>(type_index) < sizeof(kWeaponTypes) / sizeof(kWeaponTypes[0])) {
        return kWeaponTypes[type_index];
    }
    return "type #" + std::to_string(type_index);
}

std::string obj_liquid_type_name(const int liquid_index) {
    if (liquid_index >= 0
        && static_cast<std::size_t>(liquid_index) < sizeof(kLiquidNamesEn) / sizeof(kLiquidNamesEn[0])) {
        return kLiquidNamesEn[liquid_index];
    }
    return "liquid #" + std::to_string(liquid_index);
}

std::string obj_value_slot_display_hint(const int type_flag, const int slot_index, const int value) {
    if (slot_index < 0 || slot_index > 3) {
        return {};
    }
    const ObjValueSlotSchema slot = obj_value_schema_for_type(type_flag).value_slots[slot_index];
    switch (slot.editor) {
    case ObjValueEditorKind::weapon_damage_type:
        return obj_weapon_damage_type_name(value);
    case ObjValueEditorKind::liquid_type:
        return obj_liquid_type_name(value);
    case ObjValueEditorKind::spell_number:
        return obj_spell_name(value);
    case ObjValueEditorKind::yes_no:
        return value != 0 ? "Yes" : "No";
    default:
        return {};
    }
}

ObjValueTypeSchema obj_value_schema_for_type(const int type_flag) {
    ObjValueTypeSchema schema;
    const std::string type_name = obj_type_name(type_flag);

    switch (type_flag) {
    case kItemLight:
        schema.legend_it = "LIGHT — Colour / Type / Hours (OLC Value1..3).";
        schema.value_slots[0] = int_slot(0, "Colour", "Colour of the light source (OLC Value1).");
        schema.value_slots[1] = int_slot(1, "Type", "Light subtype (OLC Value2).");
        schema.value_slots[2] =
            int_slot(2, "Hours", "Hours the light source will last (OLC Value3).", 0, 9999);
        schema.value_slots[3] = unused_slot(3);
        break;
    case kItemScroll:
        schema.legend_it =
            "SCROLL — casting level + up to 3 spells (#spell). Do not use skill numbers "
            "(45–52, 120–127, …): OLC rejects them.";
        schema.value_slots[0] =
            int_slot(0, "Level", "Level of casting this scroll casts (OLC Value1).", 0, 125);
        schema.value_slots[1] = spell_slot(1, "Spell 2", "Second spell this scroll casts (OLC Value2).");
        schema.value_slots[2] = spell_slot(2, "Spell 3", "Third spell this scroll casts (OLC Value3).");
        schema.value_slots[3] = spell_slot(3, "Spell 4", "Fourth spell this scroll casts (OLC Value4).");
        break;
    case kItemWand:
        schema.legend_it = "WAND — Level, mana/use, Charges; Spell in value[3] (as in `stat`).";
        schema.value_slots[0] =
            int_slot(0, "Level", "Level of casting this wand casts (OLC Value1).", 0, 125);
        schema.value_slots[1] =
            int_slot(1, "Mana", "Mana the wand takes to use each time (OLC Value2).", 0, 9999);
        schema.value_slots[2] =
            int_slot(2, "Charges", "Number of charges the wand has (OLC Value3).", 0, 9999);
        schema.value_slots[3] = spell_slot(3, "Spell", "Spell number (#spell). Shown as Spell in `stat`.");
        break;
    case kItemStaff:
        schema.legend_it = "STAFF — same fields as WAND (Level / Mana / Charges / Spell).";
        schema.value_slots[0] =
            int_slot(0, "Level", "Level of casting this staff casts (OLC Value1).", 0, 125);
        schema.value_slots[1] =
            int_slot(1, "Mana", "Mana the staff takes to use each time (OLC Value2).", 0, 9999);
        schema.value_slots[2] =
            int_slot(2, "Charges", "Number of charges the staff has (OLC Value3).", 0, 9999);
        schema.value_slots[3] = spell_slot(3, "Spell", "Spell number (#spell). Shown as Spell in `stat`.");
        break;
    case kItemWeapon:
        schema.legend_it =
            "WEAPON — Todam = value[1]dvalue[2]; Damage type = value[3] (aszWeaponType index).";
        schema.value_slots[0] =
            int_slot(0, "Reserved", "Reserved for now (OLC Value1). Leave 0 unless documented.", 0, 999);
        schema.value_slots[1] =
            int_slot(1, "Todam dice", "Number of times the damage dice is rolled (OLC Value2).", 0, 99);
        schema.value_slots[2] =
            int_slot(2, "Todam sides", "Number of sides the damage dice has (OLC Value3).", 1, 99);
        schema.value_slots[3] = {"value[3]  Damage type",
                                 "Type of weapon damage (aszWeaponType / OLC 1=stab …).",
                                 ObjValueEditorKind::weapon_damage_type, 0, 20};
        break;
    case kItemFireweapon:
        schema.legend_it =
            "FIRE WEAPON — Min Strength / Max range / Bonus range / Type "
            "(Type must match MISSILE value[3]).";
        schema.value_slots = {
            int_slot(0, "Min Strength", "Strength required to draw (OLC Value1).", 0, 25),
            int_slot(1, "Max range", "Max range in rooms (OLC Value2).", 0, 50),
            int_slot(2, "Bonus range", "Bonus range in rooms (OLC Value3).", 0, 50),
            int_slot(3, "Type", "Missile type (must match MISSILE value[3]).", 0, 99),
        };
        break;
    case kItemMissile:
        schema.legend_it =
            "MISSILE — % to break / Todam / Type (Type must match FIRE WEAPON value[3]).";
        schema.value_slots = {
            int_slot(0, "% to break", "% chance that the missile will break (OLC Value1).", 0, 100),
            int_slot(1, "Todam sides", "Number of sides on the damage die (OLC Value2).", 1, 99),
            int_slot(2, "Todam dice", "Times the damage die is rolled (OLC Value3).", 0, 99),
            int_slot(3, "Type", "Weapon type that can load this missile (OLC Value4).", 0, 99),
        };
        break;
    case kItemArmor:
        schema.legend_it = "ARMOR — AC-apply and Full Strength (as in `stat`).";
        schema.value_slots = {
            int_slot(0, "AC-apply", "Armor Class Apply (OLC Value1). Positive = protection.", -100, 100),
            int_slot(1, "Full Strength", "Full Strength of the armor (OLC Value2).", 0, 100),
            unused_slot(2),
            unused_slot(3),
        };
        break;
    case kItemPotion:
        schema.legend_it = "POTION — Level + up to 3 spells (#spell).";
        schema.value_slots[0] =
            int_slot(0, "Level", "Level of the spells this potion casts (OLC Value1).", 0, 125);
        schema.value_slots[1] = spell_slot(1, "Spell 2", "Second spell this potion casts (OLC Value2).");
        schema.value_slots[2] = spell_slot(2, "Spell 3", "Third spell this potion casts (OLC Value3).");
        schema.value_slots[3] = spell_slot(3, "Spell 4", "Fourth spell this potion casts (OLC Value4).");
        break;
    case kItemTrap:
        schema.legend_it =
            "TRAP — damage level / attack type / constant 80 / charges. "
            "Attack types: 10 blasted, 26 seared, 67 corroded, 327 frozen, 331 pounded, "
            "332 pierced, 333 sliced.";
        schema.value_slots = {
            int_slot(0, "Dam level", "Trap damage level (OLC Value1).", 0, 125),
            int_slot(1, "Attack type", "Attack type of the trap (OLC Value2).", 0, 999),
            int_slot(2, "Constant", "Should be set to 80 (OLC Value3).", 0, 999),
            int_slot(3, "Charges", "Number of trap charges left (OLC Value4).", 0, 9999),
        };
        break;
    case kItemContainer:
        schema.legend_it = "CONTAINER — Max-contains / Locktype / reserved / Corpse.";
        schema.value_slots = {
            int_slot(0, "Max-contains", "Max number of items that can fit.", 0, 9999),
            int_slot(1, "Locktype", "Lock type for the container.", 0, 9999),
            int_slot(2, "Reserved", "Reserved (OLC). Leave 0.", 0, 9999),
            yes_no_slot(3, "Corpse", "Whether the container is a corpse."),
        };
        break;
    case kItemDrinkcon:
        schema.legend_it = "LIQUID CONTAINER — Max-contains / Contains / Liquid / Poisoned.";
        schema.value_slots[0] =
            int_slot(0, "Max-contains", "Amount of fluid the container can hold.", 0, 9999);
        schema.value_slots[1] =
            int_slot(1, "Contains", "Amount of fluid left.", 0, 9999);
        schema.value_slots[2] = {"value[2]  Liquid",
                                 "Type of liquid (Server drinks table / OLC list).",
                                 ObjValueEditorKind::liquid_type, 0, 15};
        schema.value_slots[3] = yes_no_slot(3, "Poisoned", "Whether the liquid is poisoned.");
        break;
    case kItemNote:
        schema = schema_all_unused("NOTE — values not used for this item type.");
        break;
    case kItemKey:
        schema.legend_it = "KEY — Keytype in value[0].";
        schema.value_slots = {
            int_slot(0, "Keytype", "Key type (associated locktype).", 0, 99999),
            unused_slot(1),
            unused_slot(2),
            unused_slot(3),
        };
        break;
    case kItemFood:
        schema.legend_it = "FOOD — Makes full / Poisoned.";
        schema.value_slots = {
            int_slot(0, "Makes full", "How full you get from eating the food (OLC Value1).", 0, 999),
            unused_slot(1),
            unused_slot(2),
            yes_no_slot(3, "Poisoned", "Whether the food is poisoned."),
        };
        break;
    case kItemMoney:
        schema.legend_it = "MONEY — gold amount in value[0].";
        schema.value_slots = {
            int_slot(0, "Gold", "Amount of gold coins.", 0, 2000000000),
            unused_slot(1),
            unused_slot(2),
            unused_slot(3),
        };
        break;
    case kItemClanSymbol:
        schema.legend_it = "CLAN SYMBOL — Prince toon_id in value[0] (MySQL toon.id). V1–V3 unused.";
        schema.value_slots = {
            int_slot(0, "Prince toon_id", "Prince toon_id (MySQL toon.id) for the clan.", 0, 2000000000),
            unused_slot(1),
            unused_slot(2),
            unused_slot(3),
        };
        break;
    default:
        schema = schema_all_unused("Type " + type_name
                                   + ": no OLC legend. Values are usually 0 — check Server before "
                                     "using non-standard numbers.");
        break;
    }

    return schema;
}

} // namespace nebbie
