#include "nebbie/zon_comment_sync.hpp"

#include "nebbie/edit.hpp"
#include "nebbie/mob_catalog.hpp"
#include "nebbie/room_catalog.hpp"

#include <cctype>
#include <cstdio>
#include <sstream>
#include <string>

namespace nebbie {

namespace {

constexpr int kOpSex = 1;
constexpr int kOpRace = 2;
constexpr int kOpClass = 4;
constexpr int kOpEvil = 5;
constexpr int kOpGood = 6;
constexpr int kOpVnum = 7;

const char* kEquipSlotLabels[] = {
    "Come luce",
    "Come anello destro",
    "Come anello sinistro",
    "Al collo (1)",
    "Al collo (2)",
    "Sul corpo",
    "Sulla testa",
    "Sulle gambe",
    "Ai piedi",
    "Alle mani",
    "Sulle braccia",
    "Come scudo",
    "Intorno al corpo",
    "Alla vita",
    "Al polso destro",
    "Al polso sinistro",
    "Impugnato",
    "Tenuto",
    "Sulla schiena",
    "All'orecchio destro",
    "All'orecchio sinistro",
    "Agli occhi",
};

const char* kPcClassNames[] = {
    "Mages",     "Clerics",  "Warriors", "Thieves",  "Druids",
    "Monks",     "Barbarians", "Sorcerers", "Paladins", "Rangers",
    "Psionists",
};

const char* kDoorStateLabels[] = {
    "aperta",
    "chiusa",
    "chiusa a chiave",
};

std::string trim_ascii(std::string value) {
    while (!value.empty()
           && (value.front() == ' ' || value.front() == '\t' || value.front() == '\r'
               || value.front() == '\n')) {
        value.erase(value.begin());
    }
    while (!value.empty()
           && (value.back() == ' ' || value.back() == '\t' || value.back() == '\r'
               || value.back() == '\n')) {
        value.pop_back();
    }
    return value;
}

bool starts_with(const std::string& text, const std::string& prefix) {
    return text.size() >= prefix.size() && text.compare(0, prefix.size(), prefix) == 0;
}

bool starts_with(const std::string& text, const char* prefix) {
    return starts_with(text, std::string(prefix));
}

bool is_section_or_init_comment(const std::string& body) {
    if (body.find("*****") != std::string::npos) {
        return true;
    }
    std::string trimmed = trim_ascii(body);
    if (!trimmed.empty() && trimmed.front() == '!') {
        trimmed.erase(trimmed.begin());
        trimmed = trim_ascii(trimmed);
    }
    return starts_with(trimmed, "Inizializzazione");
}

bool is_legacy_name_comment(const std::string& body) {
    const std::string trimmed = trim_ascii(body);
    if (trimmed.empty() || is_section_or_init_comment(trimmed)) {
        return false;
    }

    std::string scan = trimmed;
    // "* *M:..." / "* *!M:..." stored after leading '*' was consumed → " *M:..." or "*M:..."
    while (!scan.empty() && (scan.front() == '*' || scan.front() == ' ' || scan.front() == '\t')) {
        scan.erase(scan.begin());
    }
    if (!scan.empty() && scan.front() == '!') {
        scan.erase(scan.begin());
    }

    if (starts_with(scan, "Door in room") || starts_with(scan, "door in room")) {
        return true;
    }
    if (starts_with(scan, "O:") || starts_with(scan, "M:") || starts_with(scan, "P:")
        || starts_with(scan, "E:") || starts_with(scan, "G:") || starts_with(scan, "D:")) {
        return true;
    }
    return false;
}

std::string lookup_mob_short(const World& world, const long vnum) {
    if (const Mobile* mob = world.find_mobile(vnum)) {
        if (!mob->short_descr.empty()) {
            return mob->short_descr;
        }
    }
    return "?";
}

std::string lookup_obj_short(const World& world, const long vnum) {
    if (const GameObject* obj = world.find_object(vnum)) {
        if (!obj->short_descr.empty()) {
            return obj->short_descr;
        }
    }
    return "?";
}

std::string lookup_room_name(const World& world, const long vnum) {
    if (const Room* room = world.find_room(vnum)) {
        if (!room->name.empty()) {
            return room->name;
        }
    }
    return "?";
}

std::string padded_obj_vnum(const long vnum) {
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%06ld", vnum);
    return buf;
}

std::string equip_slot_label(const int slot) {
    const std::size_t count = sizeof(kEquipSlotLabels) / sizeof(kEquipSlotLabels[0]);
    if (slot >= 0 && static_cast<std::size_t>(slot) < count) {
        return kEquipSlotLabels[slot];
    }
    return "Slot #" + std::to_string(slot);
}

std::string door_state_label(const int state) {
    const std::size_t count = sizeof(kDoorStateLabels) / sizeof(kDoorStateLabels[0]);
    if (state >= 0 && static_cast<std::size_t>(state) < count) {
        return kDoorStateLabels[state];
    }
    return "stato #" + std::to_string(state);
}

std::string class_label(const int class_id) {
    const std::size_t count = sizeof(kPcClassNames) / sizeof(kPcClassNames[0]);
    if (class_id >= 0 && static_cast<std::size_t>(class_id) < count) {
        return kPcClassNames[class_id];
    }
    return "#" + std::to_string(class_id);
}

std::string sex_label_it(const int sex) {
    switch (sex) {
    case 1:
        return "Maschile";
    case 2:
        return "Femminile";
    default:
        return "Neutro";
    }
}

std::string hate_fear_note(const char command, const int op_type, const int value) {
    const char* verb = (command == 'F') ? "Paura" : "Odio";
    switch (op_type) {
    case kOpSex:
        return std::string(verb) + " verso il sesso " + sex_label_it(value);
    case kOpRace:
        return std::string(verb) + " verso la razza " + mob_race_name(value);
    case kOpClass:
        return std::string(verb) + " verso la classe " + class_label(value);
    case kOpEvil:
        return std::string(verb) + " verso pc/mob con allineamento inferiore o uguale a "
               + std::to_string(value);
    case kOpGood:
        return std::string(verb) + " verso pc/mob con allineamento superiore o uguale a "
               + std::to_string(value);
    case kOpVnum:
        return std::string(verb) + " verso il vnum " + std::to_string(value);
    default:
        return std::string(verb) + " tipo " + std::to_string(op_type) + " valore "
               + std::to_string(value);
    }
}

/** MW / MR from the numeric fields already on the reset line (arg2 / arg4). */
std::string max_world_room_suffix(const int max_world, const int max_room) {
    return " MW:" + std::to_string(max_world) + " MR:" + std::to_string(max_room);
}

std::string max_world_suffix(const int max_world) {
    return " MW:" + std::to_string(max_world);
}

std::string build_inline_note(const ResetCommand& cmd, const World& world) {
    switch (cmd.command) {
    case 'M': {
        // M: if_flag mob_vnum max_world room_vnum max_room
        const long mob_vnum = cmd.arg1;
        const long room_vnum = cmd.arg3;
        return "Mob " + std::to_string(mob_vnum) + " (" + lookup_mob_short(world, mob_vnum)
               + ") nella stanza " + std::to_string(room_vnum) + " ("
               + lookup_room_name(world, room_vnum) + ")"
               + max_world_room_suffix(cmd.arg2, cmd.arg4);
    }
    case 'O': {
        // O: if_flag obj_vnum max_world room_vnum max_room
        const long obj_vnum = cmd.arg1;
        const long room_vnum = cmd.arg3;
        return "Carica oggetto " + std::to_string(obj_vnum) + " ("
               + lookup_obj_short(world, obj_vnum) + ") nella stanza "
               + std::to_string(room_vnum) + " (" + lookup_room_name(world, room_vnum) + ")"
               + max_world_room_suffix(cmd.arg2, cmd.arg4);
    }
    case 'E': {
        // E: if_flag obj_vnum max_world equip_pos
        const long obj_vnum = cmd.arg1;
        const int slot = cmd.arg3;
        return equip_slot_label(slot) + ": " + padded_obj_vnum(obj_vnum) + " ("
               + lookup_obj_short(world, obj_vnum) + ")" + max_world_suffix(cmd.arg2);
    }
    case 'G': {
        // G: if_flag obj_vnum max_world
        const long obj_vnum = cmd.arg1;
        return "In inventario: " + padded_obj_vnum(obj_vnum) + " ("
               + lookup_obj_short(world, obj_vnum) + ")" + max_world_suffix(cmd.arg2);
    }
    case 'P': {
        // P: if_flag obj_vnum max_world container_vnum
        const long obj_vnum = cmd.arg1;
        const long container_vnum = cmd.arg3;
        return "Dentro " + padded_obj_vnum(container_vnum) + " ("
               + lookup_obj_short(world, container_vnum) + "): " + padded_obj_vnum(obj_vnum)
               + " (" + lookup_obj_short(world, obj_vnum) + ")" + max_world_suffix(cmd.arg2);
    }
    case 'D': {
        const long room_vnum = cmd.arg1;
        const int direction = cmd.arg2;
        const int state = cmd.arg3;
        return "Uscita " + exit_direction_label(direction) + " della stanza "
               + std::to_string(room_vnum) + " (" + lookup_room_name(world, room_vnum)
               + "): " + door_state_label(state);
    }
    case 'H':
    case 'F':
        return hate_fear_note(cmd.command, cmd.arg1, cmd.arg2);
    default:
        return {};
    }
}

bool is_syncable_reset(const char command) {
    switch (command) {
    case 'M':
    case 'O':
    case 'E':
    case 'G':
    case 'P':
    case 'D':
    case 'H':
    case 'F':
        return true;
    default:
        return false;
    }
}

/**
 * Replace or append the *! note while preserving any free text between numeric
 * args and the previous *! (e.g. hate messages on H/F lines).
 */
bool apply_inline_note(ResetCommand& cmd, const std::string& note) {
    const std::string cleaned = trim_ascii(note);
    if (cleaned.empty()) {
        return false;
    }

    std::string prefix = cmd.raw_line;
    const std::size_t bang = prefix.find("*!");
    if (bang != std::string::npos) {
        prefix = prefix.substr(0, bang);
    }
    while (!prefix.empty()
           && (prefix.back() == ' ' || prefix.back() == '\t' || prefix.back() == '\r'
               || prefix.back() == '\n')) {
        prefix.pop_back();
    }

    if (prefix.empty()) {
        // Fall back to structured rebuild from parsed args.
        ResetCommand rebuilt = cmd;
        set_reset_inline_note(rebuilt, cleaned);
        if (rebuilt.raw_line == cmd.raw_line) {
            return false;
        }
        cmd.raw_line = rebuilt.raw_line;
        return true;
    }

    std::string next = prefix + "  *! " + cleaned + "\n";
    if (next == cmd.raw_line) {
        return false;
    }
    cmd.raw_line = next;
    return true;
}

bool sync_zone_reset_comments_impl(Zone& zone, const World& world) {
    bool changed = false;
    std::vector<ResetCommand> kept;
    kept.reserve(zone.commands.size());

    for (ResetCommand cmd : zone.commands) {
        if (cmd.command == '*') {
            if (is_legacy_name_comment(cmd.raw_line)) {
                changed = true;
                continue;
            }
            kept.push_back(std::move(cmd));
            continue;
        }

        if (is_syncable_reset(cmd.command)) {
            const std::string generated = build_inline_note(cmd, world);
            const std::string existing = reset_inline_note(cmd);
            // Keep Castelli free-text tails after a matching structured template.
            const bool already_ok =
                !generated.empty()
                && (existing == generated || starts_with(existing, generated + " ")
                    || starts_with(existing, generated));
            if (!already_ok && apply_inline_note(cmd, generated)) {
                changed = true;
            }
        }
        kept.push_back(std::move(cmd));
    }

    if (changed) {
        zone.commands = std::move(kept);
    }
    return changed;
}

} // namespace

bool sync_zone_reset_comments(Zone& zone, const World& world) {
    return sync_zone_reset_comments_impl(zone, world);
}

bool sync_all_zone_reset_comments(World& world) {
    bool changed = false;
    for (Zone& zone : world.zones) {
        if (sync_zone_reset_comments(zone, world)) {
            changed = true;
        }
    }
    return changed;
}

bool zone_reset_comments_need_sync(const Zone& zone, const World& world) {
    Zone copy = zone;
    return sync_zone_reset_comments(copy, world);
}

} // namespace nebbie
