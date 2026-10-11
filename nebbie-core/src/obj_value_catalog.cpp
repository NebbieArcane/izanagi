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

ObjValueSlotSchema unused_slot(const int index) {
    ObjValueSlotSchema s;
    s.label_it = "value[" + std::to_string(index) + "] — non usato";
    s.tooltip_it =
        "Per questo tipo di oggetto il server non interpreta value[" + std::to_string(index) +
        "]. Lasciare 0 salvo documentazione interna.";
    s.editor = ObjValueEditorKind::unused;
    return s;
}

ObjValueSlotSchema int_slot(const int index, const std::string& label, const std::string& tooltip,
                            const int min_v = 0, const int max_v = 2000000000) {
    ObjValueSlotSchema s;
    s.label_it = label;
    s.tooltip_it = tooltip;
    s.editor = ObjValueEditorKind::integer;
    s.min_value = min_v;
    s.max_value = max_v;
    return s;
}

ObjValueSlotSchema yes_no_slot(const int index, const std::string& label, const std::string& tooltip) {
    ObjValueSlotSchema s;
    s.label_it = label;
    s.tooltip_it = tooltip + " Valori: 0 = no, 1 = sì (come OLC NebbieArcane/Server).";
    s.editor = ObjValueEditorKind::yes_no;
    s.min_value = 0;
    s.max_value = 1;
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

const char* kLiquidNamesIt[] = {
    "acqua", "birra", "vino", "birra chiara", "birra scura", "whisky", "limonata", "acquavite",
    "vodka", "succo di melma", "latte", "tè", "caffè", "sangue", "acqua salata", "coca cola",
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
    const std::size_t count = sizeof(kLiquidNamesIt) / sizeof(kLiquidNamesIt[0]);
    for (std::size_t i = 0; i < count; ++i) {
        std::string label = std::to_string(static_cast<int>(i)) + " — " + kLiquidNamesIt[i];
        out.emplace_back(static_cast<int>(i), label);
    }
    return out;
}

std::vector<std::pair<int, std::string>> obj_spell_number_choices() {
    std::vector<std::pair<int, std::string>> out;
    out.emplace_back(0, "0 — (nessuno)");
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
        return "(nessuno)";
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
    return "tipo #" + std::to_string(type_index);
}

std::string obj_liquid_type_name(const int liquid_index) {
    if (liquid_index >= 0
        && static_cast<std::size_t>(liquid_index) < sizeof(kLiquidNamesIt) / sizeof(kLiquidNamesIt[0])) {
        return kLiquidNamesIt[liquid_index];
    }
    return "liquido #" + std::to_string(liquid_index);
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
        return value != 0 ? "sì" : "no";
    default:
        return {};
    }
}

ObjValueTypeSchema obj_value_schema_for_type(const int type_flag) {
    ObjValueTypeSchema schema;
    const std::string type_name = obj_type_name(type_flag);

    switch (type_flag) {
    case kItemLight:
        schema.legend_it =
            "LIGHT: fonte di luce. Riferimento server: create.obj / stat oggetto.";
        schema.value_slots[0] = int_slot(0, "value[0] — colore", "Colore della luce (OLC: Value1).");
        schema.value_slots[1] = int_slot(1, "value[1] — tipo", "Tipo/sottotipo luce (OLC: Value2).");
        schema.value_slots[2] = int_slot(2, "value[2] — ore durata", "Ore di durata della fonte (OLC: Value3).");
        schema.value_slots[3] = unused_slot(3);
        break;
    case kItemScroll:
        schema.legend_it =
            "SCROLL: pergamena. value[0] livello di lancio; value[1..3] incantesimi (#spell). "
            "Non usare numeri skill 45–52, 120–127, … (il server li rifiuta in OLC).";
        schema.value_slots[0] =
            int_slot(0, "value[0] — livello", "Livello di lancio della pergamena (OLC: Value1).", 0, 125);
        schema.value_slots[1] = {"value[1] — incantesimo 2", "Secondo incantesimo (#spell, OLC Value2).",
                           ObjValueEditorKind::spell_number, 0, 999};
        schema.value_slots[2] = {"value[2] — incantesimo 3", "Terzo incantesimo (#spell, OLC Value3).",
                           ObjValueEditorKind::spell_number, 0, 999};
        schema.value_slots[3] = {"value[3] — incantesimo 4", "Quarto incantesimo (#spell, OLC Value4).",
                           ObjValueEditorKind::spell_number, 0, 999};
        break;
    case kItemWand:
        schema.legend_it =
            "WAND: bacchetta. Livello, mana per uso, cariche; value[3] = incantesimo (come stat server).";
        schema.value_slots[0] = int_slot(0, "value[0] — livello", "Livello di lancio (OLC: Value1).", 0, 125);
        schema.value_slots[1] = int_slot(1, "value[1] — mana per uso", "Mana consumato ad ogni uso (OLC: Value2).", 0, 9999);
        schema.value_slots[2] =
            int_slot(2, "value[2] — cariche", "Cariche rimanenti (OLC: Value3; stat: Charges).", 0, 9999);
        schema.value_slots[3] = {"value[3] — incantesimo", "Numero incantesimo (#spell). Stat server: Spell.",
                           ObjValueEditorKind::spell_number, 0, 999};
        break;
    case kItemStaff:
        schema.legend_it = "STAFF: bastone magico. Stessa semantica della WAND.";
        schema.value_slots[0] = int_slot(0, "value[0] — livello", "Livello di lancio (OLC: Value1).", 0, 125);
        schema.value_slots[1] = int_slot(1, "value[1] — mana per uso", "Mana consumato ad ogni uso (OLC: Value2).", 0, 9999);
        schema.value_slots[2] = int_slot(2, "value[2] — cariche", "Cariche rimanenti (OLC: Value3).", 0, 9999);
        schema.value_slots[3] = {"value[3] — incantesimo", "Numero incantesimo (#spell). Stat server: Spell.",
                           ObjValueEditorKind::spell_number, 0, 999};
        break;
    case kItemWeapon:
        schema.legend_it =
            "WEAPON: arma. Danno value[1]dvalue[2]; value[3] = tipo danno (tabella aszWeaponType, indice 0-based). "
            "OLC elenca tipi 1–12 corrispondenti agli indici 1–12 (0 riservato).";
        schema.value_slots[0] =
            int_slot(0, "value[0] — riservato", "Riservato (OLC: Value1). Lasciare 0 salvo uso interno.", 0, 999);
        schema.value_slots[1] =
            int_slot(1, "value[1] — numero dadi", "Quante volte si tira il dado danno (OLC: Value2).", 0, 99);
        schema.value_slots[2] =
            int_slot(2, "value[2] — facce dado", "Facce del dado danno (OLC: Value3).", 1, 99);
        schema.value_slots[3] = {"value[3] — tipo danno", "Tipo colpo (indice in aszWeaponType / OLC 1=stab …).",
                           ObjValueEditorKind::weapon_damage_type, 0, 20};
        break;
    case kItemFireweapon:
        schema.legend_it =
            "FIRE WEAPON: arma da fuoco. value[4] deve combaciare con value[4] del proiettile (MISSILE).";
        schema.value_slots = {
            int_slot(0, "value[0] — forza minima", "Forza minima per armare (OLC: Value1).", 0, 25),
            int_slot(1, "value[1] — gittata max", "Gittata massima in stanze (OLC: Value2).", 0, 50),
            int_slot(2, "value[2] — bonus gittata", "Bonus gittata in stanze (OLC: Value3).", 0, 50),
            int_slot(3, "value[3] — tipo proiettile", "Tipo munizione (deve matchare MISSILE value[3]).", 0, 99),
        };
        break;
    case kItemMissile:
        schema.legend_it = "MISSILE: munizione. value[4] deve matchare FIRE WEAPON value[3].";
        schema.value_slots = {
            int_slot(0, "value[0] — %% rottura", "Probabilità % che si rompa (OLC: Value1).", 0, 100),
            int_slot(1, "value[1] — facce dado", "Facce del dado danno (OLC: Value2).", 1, 99),
            int_slot(2, "value[2] — numero dadi", "Numero di dadi danno (OLC: Value3).", 0, 99),
            int_slot(3, "value[3] — tipo arma", "Tipo arma compatibile (OLC: Value4).", 0, 99),
        };
        break;
    case kItemArmor:
        schema.legend_it = "ARMOR: armatura. AC-apply in value[0] (valore positivo = protezione).";
        schema.value_slots = {
            int_slot(0, "value[0] — AC apply", "Modificatore AC applicato (OLC: Armor Class Apply).", -100, 100),
            int_slot(1, "value[1] — full strength", "Integrità massima armatura (OLC: Full Strength).", 0, 100),
            unused_slot(2),
            unused_slot(3),
        };
        break;
    case kItemPotion:
        schema.legend_it = "POTION: pozione. value[0] livello; value[1..3] incantesimi (#spell).";
        schema.value_slots[0] = int_slot(0, "value[0] — livello", "Livello incantesimi (OLC: Value1).", 0, 125);
        schema.value_slots[1] = {"value[1] — incantesimo 2", "Secondo incantesimo (#spell).", ObjValueEditorKind::spell_number,
                           0, 999};
        schema.value_slots[2] = {"value[2] — incantesimo 3", "Terzo incantesimo (#spell).", ObjValueEditorKind::spell_number,
                           0, 999};
        schema.value_slots[3] = {"value[3] — incantesimo 4", "Quarto incantesimo (#spell).", ObjValueEditorKind::spell_number,
                           0, 999};
        break;
    case kItemTrap:
        schema.legend_it =
            "TRAP: trappola. value[2] di solito 80 (OLC). value[1] tipo attacco (es. 67 acido, 327 gelo…).";
        schema.value_slots = {
            int_slot(0, "value[0] — livello danno", "Livello danno trappola (OLC: Value1).", 0, 125),
            int_slot(1, "value[1] — tipo attacco",
                      "Tipo attacco (OLC: 10 blasted, 26 seared, 67 corroded, 327 frozen, 331 pounded, 332 pierced, "
                      "333 sliced).",
                      0, 999),
            int_slot(2, "value[2] — costante", "Impostare a 80 (OLC: Value3).", 0, 999),
            int_slot(3, "value[3] — cariche", "Cariche trappola rimanenti (OLC: Value4).", 0, 9999),
        };
        break;
    case kItemContainer:
        schema.legend_it = "CONTAINER: contenitore. value[3]=1 se corpo (corpse).";
        schema.value_slots = {
            int_slot(0, "value[0] — capienza", "Numero massimo oggetti contenibili.", 0, 9999),
            int_slot(1, "value[1] — locktype", "Tipo di serratura (chiave/lock).", 0, 9999),
            int_slot(2, "value[2] — riservato", "Riservato (OLC). Lasciare 0.", 0, 9999),
            yes_no_slot(3, "value[3] — corpo", "1 se il contenitore è un corpo (corpse)."),
        };
        break;
    case kItemDrinkcon:
        schema.legend_it = "LIQUID CONTAINER: contenitore bevande.";
        schema.value_slots[0] = int_slot(0, "value[0] — capienza", "Quantità massima di liquido.", 0, 9999);
        schema.value_slots[1] = int_slot(1, "value[1] — contenuto", "Quantità attuale di liquido.", 0, 9999);
        schema.value_slots[2] = {"value[2] — tipo liquido", "Tipo liquido (tabella drinks server, 0=acqua …).",
                           ObjValueEditorKind::liquid_type, 0, 15};
        schema.value_slots[3] = yes_no_slot(3, "value[3] — avvelenato", "Liquido avvelenato.");
        break;
    case kItemNote:
        schema = schema_all_unused("NOTE: value non usati dal server per questo tipo.");
        break;
    case kItemKey:
        schema.legend_it = "KEY: chiave. Solo value[0] = keytype.";
        schema.value_slots = {
            int_slot(0, "value[0] — keytype", "Tipo chiave (locktype associato).", 0, 99999),
            unused_slot(1),
            unused_slot(2),
            unused_slot(3),
        };
        break;
    case kItemFood:
        schema.legend_it = "FOOD: cibo. value[0] sazietà; value[3] avvelenato.";
        schema.value_slots = {
            int_slot(0, "value[0] — sazietà", "Quanto riempie mangiando (OLC: Makes full).", 0, 999),
            unused_slot(1),
            unused_slot(2),
            yes_no_slot(3, "value[3] — avvelenato", "Cibo avvelenato."),
        };
        break;
    case kItemMoney:
        schema.legend_it = "MONEY: monete. value[0] = quantità oro.";
        schema.value_slots = {
            int_slot(0, "value[0] — quantità oro", "Importo in monete.", 0, 2000000000),
            unused_slot(1),
            unused_slot(2),
            unused_slot(3),
        };
        break;
    case kItemClanSymbol:
        schema.legend_it =
            "CLAN SYMBOL: value[0] = toon_id del principe (MySQL toon.id). Altri value a 0.";
        schema.value_slots = {
            int_slot(0, "value[0] — toon_id principe", "ID personaggio principe della casata.", 0, 2000000000),
            unused_slot(1),
            unused_slot(2),
            unused_slot(3),
        };
        break;
    default:
        schema = schema_all_unused(
            "Tipo " + type_name +
            ": nessuna legenda specifica in OLC server. I value sono di solito 0; verificare su NebbieArcane/Server "
            "prima di usare valori non standard.");
        break;
    }

    return schema;
}

} // namespace nebbie
