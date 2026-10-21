#include "nebbie/obj_catalog.hpp"

#include "nebbie/obj_value_catalog.hpp"

#include <algorithm>

namespace nebbie {

namespace {

const char* kItemTypes[] = {
    "UNDEFINED", "LIGHT", "SCROLL", "WAND", "STAFF", "WEAPON", "FIRE WEAPON", "MISSILE",
    "TREASURE", "ARMOR", "POTION", "WORN", "OTHER", "TRASH", "TRAP", "CONTAINER", "NOTE",
    "LIQUID CONTAINER", "KEY", "FOOD", "MONEY", "PEN", "BOAT", "AUDIO", "BOARD", "TREE",
    "ROCK", "MINED GEM", "MINED MINERAL", "BAR", "JEWEL",
};

const char* kWearBits[] = {
    "TAKE", "FINGER", "NECK", "BODY", "HEAD", "LEGS", "FEET", "HANDS", "ARMS",
    "SHIELD", "ABOUT", "WAIST", "WRIST", "WIELD", "HOLD", "THROW", "LIGHT-SOURCE",
    "BACK", "EARS", "EYES",
};

const char* kExtraBits[] = {
    "GLOW", "HUM", "METAL", "MINERAL", "ORGANIC", "INVISIBLE", "MAGIC", "NODROP",
    "BLESS", "ANTI-GOOD", "ANTI-EVIL", "ANTI-NEUTRAL", "ANTI-CLERIC", "ANTI-MAGE",
    "ANTI-THIEF", "ANTI-WARRIOR", "BRITTLE", "RESISTANT", "ARTIFACT", "ANTI-MEN",
    "ANTI-WOMEN", "ANTI-SUN", "ANTI-BARBARIAN", "ANTI-RANGER", "ANTI-PALADIN",
    "ANTI-PSIONIST", "ANTI-MONK", "ANTI-DRUID", "ONLY-CLASS", "DIG", "SCYTHE",
    "ANTI-SORCERER",
};

const char* kExtraBits2[] = {
    "QUEST-ITEM", "EDIT", "NO-LOCATE", "PERSONAL", "HAS-GEMS", "NO-PRINCE", "ONLY-PRINCE",
    "PROCAREA-REWARD", "DUSTED", "PAID-MALUS",
};

/** Technical short names aligned with Server APPLY_* order. */
const char* kApplyTypes[] = {
    "NONE", "STR", "DEX", "INT", "WIS", "CON", "CHR", "SEX", "LEVEL", "AGE",
    "CHAR_WEIGHT", "CHAR_HEIGHT", "MANA", "HIT", "MOVE", "GOLD", "EXP", "ARMOR",
    "HITROLL", "DAMROLL", "SAVING_PARA", "SAVING_ROD", "SAVING_PETRI", "SAVING_BREATH",
    "SAVING_SPELL", "SAVING_ALL", "IMMUNE", "SUSCEPTIBILITY", "M-IMMUNE",
    "SPELL AFFECT", "WEAPON SPELL", "EAT SPELL", "BACKSTAB", "KICK", "SNEAK", "HIDE",
    "BASH", "PICK", "STEAL", "TRACK", "HIT-N-DAM", "SPELLFAIL", "ATTACKS", "HASTE",
    "SLOW", "SPELL AFFECT 2", "FIND-TRAPS", "RIDE", "RACE-SLAYER", "ALIGN-SLAYER",
    "MANA-REGEN", "HIT-REGEN", "MOVE-REGEN", "MOD-THIRST", "MOD-HUNGER", "MOD-DRUNK",
    "T_STR", "T_INT", "T_DEX", "T_WIS", "T_CON", "T_CHR", "T_HPS", "T_MOVE", "T_MANA",
    "SPELLPOWER", "HIT-N-SP",
};

/** Italian UI labels (same order as kApplyTypes). */
const char* kApplyTypesUi[] = {
    "Nessuno",
    "Forza (STR)",
    "Destrezza (DEX)",
    "Intelligenza (INT)",
    "Saggezza (WIS)",
    "Costituzione (CON)",
    "Carisma (CHR)",
    "Sesso",
    "Livello",
    "Eta'",
    "Peso personaggio",
    "Altezza personaggio",
    "Mana",
    "Punti ferita (HIT)",
    "Movimento",
    "Oro",
    "Esperienza",
    "Armatura (AC)",
    "Hitroll",
    "Damroll",
    "Salvezza paralisi",
    "Salvezza bacchetta",
    "Salvezza pietrificazione",
    "Salvezza soffio",
    "Salvezza incantesimo",
    "Salvezza (tutte)",
    "Immunita' (bit)",
    "Suscettibilita' (bit)",
    "Resistenza meta (bit)",
    "Affect magici (bit AFF)",
    "Incantesimo su arma",
    "Incantesimo se mangiato",
    "Backstab %",
    "Kick %",
    "Sneak %",
    "Hide %",
    "Bash %",
    "Pick %",
    "Steal %",
    "Track %",
    "Hit & Dam (HIT-N-DAM)",
    "Spellfail",
    "Attacchi extra",
    "Haste",
    "Slow",
    "Affect magici 2 (bit AFF2)",
    "Find traps %",
    "Ride %",
    "Race slayer",
    "Align slayer",
    "Rigenerazione mana",
    "Rigenerazione hit",
    "Rigenerazione move",
    "Mod. sete",
    "Mod. fame",
    "Mod. ubriachezza",
    "Temp Forza",
    "Temp Intelligenza",
    "Temp Destrezza",
    "Temp Saggezza",
    "Temp Costituzione",
    "Temp Carisma",
    "Temp Hit",
    "Temp Move",
    "Temp Mana",
    "Spellpower",
    "Hit & Spellpower (HIT-N-SP)",
};

std::vector<MobFlagDef> make_bit_flags(const char* const* names, std::size_t count) {
    std::vector<MobFlagDef> defs;
    defs.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        MobFlagDef def;
        def.value = 1L << i;
        def.name = names[i];
        def.label = names[i];
        defs.push_back(def);
    }
    return defs;
}

std::string join_set_flag_labels(const std::vector<MobFlagDef>& defs, const long value) {
    std::string out;
    for (const MobFlagDef& def : defs) {
        if ((value & def.value) == 0) {
            continue;
        }
        if (!out.empty()) {
            out += ", ";
        }
        out += def.label;
    }
    if (out.empty()) {
        return "(nessuno)";
    }
    return out;
}

} // namespace

std::vector<std::pair<int, std::string>> obj_type_choices() {
    std::vector<std::pair<int, std::string>> choices;
    const std::size_t count = sizeof(kItemTypes) / sizeof(kItemTypes[0]);
    for (std::size_t i = 0; i < count; ++i) {
        choices.emplace_back(static_cast<int>(i), kItemTypes[i]);
    }
    return choices;
}

std::vector<MobFlagDef> obj_wear_flags() {
    return make_bit_flags(kWearBits, sizeof(kWearBits) / sizeof(kWearBits[0]));
}

std::vector<MobFlagDef> obj_extra_flags() {
    return make_bit_flags(kExtraBits, sizeof(kExtraBits) / sizeof(kExtraBits[0]));
}

std::vector<MobFlagDef> obj_extra_flags2() {
    return make_bit_flags(kExtraBits2, sizeof(kExtraBits2) / sizeof(kExtraBits2[0]));
}

std::vector<MobFlagDef> obj_spell_affect_flags() {
    return mob_affected_flags();
}

std::vector<MobFlagDef> obj_spell_affect2_flags() {
    return {
        {1, "AFF2_ANIMAL_INVIS", "Invisible to animals"},
        {2, "AFF2_HEAT_STUFF", "Heat Stuff"},
        {4, "AFF2_LOGGING", "Logging"},
        {8, "AFF2_BERSERK", "Berserk"},
        {16, "AFF2_PARRY", "Parry"},
        {32, "AFF2_GROUP_ORDER", "Group-Order"},
        {64, "AFF2_AFK", "Away from keyboard"},
        {128, "AFF2_PKILLER", "Pkilled someone"},
        {256, "AFF2_DANGER_SENSE", "Danger sense"},
        {512, "AFF2_QUEST", "Quest"},
    };
}

std::vector<MobFlagDef> obj_align_slayer_flags() {
    return {
        {1, "ALIGN_GOOD", "GOOD"},
        {2, "ALIGN_NEUTRAL", "NEUTRAL"},
        {4, "ALIGN_EVIL", "EVIL"},
    };
}

std::vector<std::pair<int, std::string>> obj_apply_type_choices() {
    std::vector<std::pair<int, std::string>> choices;
    const std::size_t count = sizeof(kApplyTypes) / sizeof(kApplyTypes[0]);
    for (std::size_t i = 0; i < count; ++i) {
        choices.emplace_back(static_cast<int>(i), kApplyTypes[i]);
    }
    return choices;
}

std::vector<std::pair<int, std::string>> obj_apply_type_choices_ui() {
    // Tipi davvero usati nei .obj delle Aree (piu' SPELLPOWER / HIT-N-SP).
    // Esclusi: NONE, SEX, LEVEL, GOLD, EXP, SLOW, AFF2, FIND-TRAPS, MOD_*, T_*.
    static constexpr int kEditorApply[] = {
        1,  2,  3,  4,  5,  6,           // attributi
        12, 13, 14,                      // mana / hit / move
        17, 18, 19, 40,                  // AC / hitroll / damroll / hit-n-dam
        20, 21, 22, 23, 24, 25,          // salvezze
        26, 27, 28, 29,                  // immune / susc / m-imm / spell AFF
        30, 31,                          // weapon spell / eat spell
        32, 33, 34, 35, 36, 37, 38, 39, 47,  // skill %
        41, 42, 43,                      // spellfail / attacks / haste
        48, 49,                          // race / align slayer
        50, 51, 52,                      // regen
        9,  10, 11,                      // age / peso / altezza (rari ma in area)
        65, 66,                          // spellpower / hit-n-sp
    };
    std::vector<std::pair<int, std::string>> choices;
    choices.reserve(sizeof(kEditorApply) / sizeof(kEditorApply[0]));
    const std::size_t ui_count = sizeof(kApplyTypesUi) / sizeof(kApplyTypesUi[0]);
    for (const int location : kEditorApply) {
        if (location >= 0 && static_cast<std::size_t>(location) < ui_count) {
            choices.emplace_back(location, kApplyTypesUi[location]);
        }
    }
    return choices;
}

std::string obj_type_name(const int type_flag) {
    const std::size_t count = sizeof(kItemTypes) / sizeof(kItemTypes[0]);
    if (type_flag >= 0 && static_cast<std::size_t>(type_flag) < count) {
        return kItemTypes[type_flag];
    }
    return "Type #" + std::to_string(type_flag);
}

std::string obj_apply_type_name(const int location) {
    const std::size_t count = sizeof(kApplyTypes) / sizeof(kApplyTypes[0]);
    if (location >= 0 && static_cast<std::size_t>(location) < count) {
        return kApplyTypes[location];
    }
    return "Apply #" + std::to_string(location);
}

std::string obj_apply_type_label(const int location) {
    const std::size_t count = sizeof(kApplyTypesUi) / sizeof(kApplyTypesUi[0]);
    if (location >= 0 && static_cast<std::size_t>(location) < count) {
        return kApplyTypesUi[location];
    }
    return obj_apply_type_name(location);
}

ObjAffectModifierKind obj_affect_modifier_kind(const int location) {
    switch (location) {
    case APPLY_IMMUNE:
    case APPLY_SUSC:
    case APPLY_M_IMMUNE:
        return ObjAffectModifierKind::immunity_bits;
    case APPLY_SPELL:
        return ObjAffectModifierKind::spell_affect_bits;
    case APPLY_AFF2:
        return ObjAffectModifierKind::spell_affect2_bits;
    case APPLY_WEAPON_SPELL:
    case APPLY_EAT_SPELL:
        return ObjAffectModifierKind::spell_number;
    case APPLY_RACE_SLAYER:
        return ObjAffectModifierKind::race;
    case APPLY_SEX:
        return ObjAffectModifierKind::sex;
    case APPLY_ALIGN_SLAYER:
        return ObjAffectModifierKind::align_bits;
    default:
        return ObjAffectModifierKind::integer;
    }
}

std::string format_obj_affect_label(const int location, const int modifier) {
    const std::string type_label = obj_apply_type_label(location);
    switch (obj_affect_modifier_kind(location)) {
    case ObjAffectModifierKind::immunity_bits:
        return type_label + ": " + join_set_flag_labels(mob_immunity_flags(), modifier);
    case ObjAffectModifierKind::spell_affect_bits:
        return type_label + ": " + join_set_flag_labels(obj_spell_affect_flags(), modifier);
    case ObjAffectModifierKind::spell_affect2_bits:
        return type_label + ": " + join_set_flag_labels(obj_spell_affect2_flags(), modifier);
    case ObjAffectModifierKind::align_bits:
        return type_label + ": " + join_set_flag_labels(obj_align_slayer_flags(), modifier);
    case ObjAffectModifierKind::spell_number: {
        for (const auto& [value, label] : obj_spell_number_choices()) {
            if (value == modifier) {
                return type_label + ": " + label;
            }
        }
        return type_label + ": spell #" + std::to_string(modifier);
    }
    case ObjAffectModifierKind::race:
        return type_label + ": " + mob_race_name(modifier);
    case ObjAffectModifierKind::sex:
        return type_label + ": " + mob_sex_name(modifier);
    case ObjAffectModifierKind::integer:
    default: {
        const std::string sign = modifier >= 0 ? "+" : "";
        return type_label + " " + sign + std::to_string(modifier);
    }
    }
}

int sum_obj_affect_modifier(const std::vector<ObjAffect>& affects, const int location) {
    int total = 0;
    for (const ObjAffect& affect : affects) {
        if (affect.location == location) {
            total += affect.modifier;
        }
    }
    return total;
}

void set_obj_affect_modifier(std::vector<ObjAffect>& affects, const int location,
                             const int modifier) {
    if (modifier == 0) {
        affects.erase(std::remove_if(affects.begin(), affects.end(),
                                     [location](const ObjAffect& affect) {
                                         return affect.location == location;
                                     }),
                      affects.end());
        return;
    }

    bool updated = false;
    std::vector<ObjAffect> kept;
    kept.reserve(affects.size() + 1);
    for (const ObjAffect& affect : affects) {
        if (affect.location != location) {
            kept.push_back(affect);
            continue;
        }
        if (!updated) {
            ObjAffect row = affect;
            row.modifier = modifier;
            kept.push_back(row);
            updated = true;
        }
    }
    if (!updated) {
        ObjAffect row;
        row.location = location;
        row.modifier = modifier;
        kept.push_back(row);
    }
    affects = std::move(kept);
}

namespace {

bool obj_affect_uses_bitmask_modifier(const int location) {
    switch (obj_affect_modifier_kind(location)) {
    case ObjAffectModifierKind::immunity_bits:
    case ObjAffectModifierKind::spell_affect_bits:
    case ObjAffectModifierKind::spell_affect2_bits:
    case ObjAffectModifierKind::align_bits:
        return true;
    default:
        return false;
    }
}

} // namespace

void normalize_obj_affects_for_mud_save(std::vector<ObjAffect>& affects) {
    std::vector<ObjAffect> normalized;
    normalized.reserve(affects.size());

    for (const ObjAffect& affect : affects) {
        if (affect.location == 0 && affect.modifier == 0) {
            continue;
        }
        if (obj_affect_uses_bitmask_modifier(affect.location)) {
            bool merged = false;
            for (ObjAffect& existing : normalized) {
                if (existing.location != affect.location) {
                    continue;
                }
                existing.modifier |= affect.modifier;
                merged = true;
                break;
            }
            if (!merged) {
                normalized.push_back(affect);
            }
            continue;
        }
        normalized.push_back(affect);
    }

    normalized.erase(std::remove_if(normalized.begin(), normalized.end(),
                                    [](const ObjAffect& affect) {
                                        return obj_affect_uses_bitmask_modifier(affect.location)
                                               && affect.modifier == 0;
                                    }),
                     normalized.end());

    if (static_cast<int>(normalized.size()) > MAX_OBJ_AFFECT) {
        normalized.resize(static_cast<std::size_t>(MAX_OBJ_AFFECT));
    }
    affects = std::move(normalized);
}

} // namespace nebbie
