#include "nebbie/validate.hpp"

#include "nebbie/edit.hpp"
#include "nebbie/mud_text.hpp"
#include "nebbie/special_proc_catalog.hpp"
#include "nebbie/text_lines.hpp"

#include <algorithm>
#include <cctype>
#include <map>
#include <set>

namespace nebbie {

namespace {

constexpr long kNowhere = -1;

void add_issue(ValidationReport& report,
               ValidationSeverity severity,
               const std::string& category,
               const std::string& message,
               ValidationTarget target = ValidationTarget::none,
               long target_vnum = 0,
               int zone_num = 0,
               int reset_index = -1) {
    ValidationIssue issue;
    issue.severity = severity;
    issue.category = category;
    issue.message = message;
    issue.target = target;
    issue.target_vnum = target_vnum;
    issue.zone_num = zone_num;
    issue.reset_index = reset_index;
    report.issues.push_back(std::move(issue));
}

bool has_mobile(const World& world, long vnum) {
    return vnum > 0 && world.mobiles.find(vnum) != world.mobiles.end();
}

bool has_object(const World& world, long vnum) {
    return vnum > 0 && world.objects.find(vnum) != world.objects.end();
}

bool has_room(const World& world, long vnum) {
    return vnum > 0 && world.rooms.find(vnum) != world.rooms.end();
}

bool room_in_zone(long vnum, const Zone& zone) {
    return vnum >= zone.bottom && vnum <= zone.top;
}

std::string trim_copy(const std::string& value) {
    std::size_t start = 0;
    while (start < value.size()
           && std::isspace(static_cast<unsigned char>(value[start])) != 0) {
        ++start;
    }
    std::size_t end = value.size();
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1])) != 0) {
        --end;
    }
    return value.substr(start, end - start);
}

bool strings_equal_ci(const std::string& left, const std::string& right) {
    const std::string a = trim_copy(left);
    const std::string b = trim_copy(right);
    if (a.size() != b.size()) {
        return false;
    }
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i]))
            != std::tolower(static_cast<unsigned char>(b[i]))) {
            return false;
        }
    }
    return true;
}

bool should_validate_room(long vnum, const std::vector<long>* room_vnums) {
    if (room_vnums == nullptr || room_vnums->empty()) {
        return true;
    }
    return std::find(room_vnums->begin(), room_vnums->end(), vnum) != room_vnums->end();
}

void append_room_validation(const World& world,
                            ValidationReport& report,
                            const ValidationOptions& options,
                            const std::vector<long>* room_vnums) {
    for (const auto& [vnum, room] : world.rooms) {
        if (!should_validate_room(vnum, room_vnums)) {
            continue;
        }
        if (vnum != room.vnum) {
            add_issue(report,
                      ValidationSeverity::error,
                      "room",
                      "room map key " + std::to_string(vnum)
                          + " differs from entry vnum " + std::to_string(room.vnum),
                      ValidationTarget::room,
                      vnum);
        }

        bool in_any_zone = world.zones.empty();
        for (const auto& zone : world.zones) {
            if (room_in_zone(vnum, zone)) {
                in_any_zone = true;
                break;
            }
        }
        if (!in_any_zone && !world.zones.empty()) {
            add_issue(report,
                      ValidationSeverity::warning,
                      "room",
                      "room " + std::to_string(vnum) + " is outside all zone ranges",
                      ValidationTarget::room,
                      vnum);
        }

        if (options.max_line_length > 0) {
            auto check_field = [&](const std::string& text, const std::string& field_label) {
                const TextLineLengthReport line_report =
                    check_text_line_lengths(text, options.max_line_length);
                for (const auto& issue : line_report.overlong) {
                    add_issue(report,
                              ValidationSeverity::warning,
                              "room_text",
                              "room " + std::to_string(vnum) + " " + field_label + " line "
                                  + std::to_string(issue.line_number) + " has "
                                  + std::to_string(issue.length) + " characters (max "
                                  + std::to_string(options.max_line_length) + ")",
                              ValidationTarget::room,
                              vnum);
                }
            };

            check_field(room.name, "name");
            check_field(room.description, "description");
            for (std::size_t extra_index = 0; extra_index < room.extra_descs.size(); ++extra_index) {
                const auto& extra = room.extra_descs[extra_index];
                const TextLineLengthReport line_report =
                    check_text_line_lengths(extra.description, options.max_line_length);
                for (const auto& issue : line_report.overlong) {
                    add_issue(report,
                              ValidationSeverity::warning,
                              "room_text",
                              "room " + std::to_string(vnum) + " extra desc " + std::to_string(extra_index)
                                  + " line " + std::to_string(issue.line_number) + " has "
                                  + std::to_string(issue.length) + " characters (max "
                                  + std::to_string(options.max_line_length) + ")",
                              ValidationTarget::room,
                              vnum);
                }
            }

            for (std::size_t exit_index = 0; exit_index < room.exits.size(); ++exit_index) {
                const TextLineLengthReport line_report =
                    check_text_line_lengths(room.exits[exit_index].description, options.max_line_length);
                for (const auto& issue : line_report.overlong) {
                    add_issue(report,
                              ValidationSeverity::warning,
                              "room_text",
                              "room " + std::to_string(vnum) + " exit " + std::to_string(exit_index)
                                  + " description line " + std::to_string(issue.line_number) + " has "
                                  + std::to_string(issue.length) + " characters (max "
                                  + std::to_string(options.max_line_length) + ")",
                              ValidationTarget::room,
                              vnum);
                }
            }
        }

        auto warn_non_ascii = [&](const std::string& text, const std::string& field_label) {
            if (is_mud_ascii_text(text)) {
                return;
            }
            add_issue(report,
                      ValidationSeverity::warning,
                      "room_text",
                      "room " + std::to_string(vnum) + " " + field_label + " contains "
                          + std::to_string(count_non_mud_ascii_chars(text))
                          + " non-ASCII character(s); the game client expects e'/a'/o' style text",
                      ValidationTarget::room,
                      vnum);
        };

        warn_non_ascii(room.name, "name");
        warn_non_ascii(room.description, "description");
        for (std::size_t extra_index = 0; extra_index < room.extra_descs.size(); ++extra_index) {
            warn_non_ascii(room.extra_descs[extra_index].description,
                           "extra desc " + std::to_string(extra_index));
        }
        for (std::size_t exit_index = 0; exit_index < room.exits.size(); ++exit_index) {
            warn_non_ascii(room.exits[exit_index].description,
                           "exit " + std::to_string(exit_index) + " description");
        }

        for (std::size_t i = 0; i < room.exits.size(); ++i) {
            const auto& exit = room.exits[i];
            if (exit.to_room <= 0 || exit.to_room == kNowhere) {
                continue;
            }
            if (!has_room(world, exit.to_room)) {
                add_issue(report,
                          ValidationSeverity::error,
                          "room",
                          "room " + std::to_string(vnum) + " exit " + std::to_string(i)
                              + " points to missing room " + std::to_string(exit.to_room),
                          ValidationTarget::room,
                          vnum);
                continue;
            }
        }
    }
}

void validate_resets(const World& world, ValidationReport& report) {
    if (world.zones.empty()) {
        return;
    }

    for (const auto& zone : world.zones) {
        for (std::size_t i = 0; i < zone.commands.size(); ++i) {
            const auto& cmd = zone.commands[i];
            if (cmd.command == '*' || cmd.command == ';') {
                continue;
            }

            const std::string where = "zone " + std::to_string(zone.num)
                                      + " reset[" + std::to_string(i) + "] ";

            switch (cmd.command) {
            case 'M':
                if (!world.mobiles.empty() && cmd.arg1 > 0 && !has_mobile(world, cmd.arg1)) {
                    add_issue(report,
                              ValidationSeverity::error,
                              "reset",
                              where + "M references missing mobile " + std::to_string(cmd.arg1),
                              ValidationTarget::zone,
                              cmd.arg1,
                              zone.num,
                              static_cast<int>(i));
                }
                if (!world.rooms.empty() && cmd.arg3 > 0 && !has_room(world, cmd.arg3)) {
                    add_issue(report,
                              ValidationSeverity::error,
                              "reset",
                              where + "M references missing room " + std::to_string(cmd.arg3),
                              ValidationTarget::zone,
                              cmd.arg3,
                              zone.num,
                              static_cast<int>(i));
                }
                break;
            case 'C':
                if (!world.mobiles.empty() && cmd.arg1 > 0 && !has_mobile(world, cmd.arg1)) {
                    add_issue(report,
                              ValidationSeverity::error,
                              "reset",
                              where + "C references missing mobile " + std::to_string(cmd.arg1));
                }
                break;
            case 'O':
                if (!world.objects.empty() && cmd.arg1 > 0 && !has_object(world, cmd.arg1)) {
                    add_issue(report,
                              ValidationSeverity::error,
                              "reset",
                              where + "O references missing object " + std::to_string(cmd.arg1));
                }
                if (!world.rooms.empty() && cmd.arg3 > 0 && !has_room(world, cmd.arg3)) {
                    add_issue(report,
                              ValidationSeverity::error,
                              "reset",
                              where + "O references missing room " + std::to_string(cmd.arg3));
                }
                break;
            case 'G':
            case 'E':
                if (!world.objects.empty() && cmd.arg1 > 0 && !has_object(world, cmd.arg1)) {
                    add_issue(report,
                              ValidationSeverity::error,
                              "reset",
                              where + std::string(1, cmd.command) + " references missing object "
                                  + std::to_string(cmd.arg1));
                }
                break;
            case 'P':
                if (!world.objects.empty() && cmd.arg1 > 0 && !has_object(world, cmd.arg1)) {
                    add_issue(report,
                              ValidationSeverity::error,
                              "reset",
                              where + "P references missing object " + std::to_string(cmd.arg1));
                }
                if (!world.objects.empty() && cmd.arg3 > 0 && !has_object(world, cmd.arg3)) {
                    add_issue(report,
                              ValidationSeverity::error,
                              "reset",
                              where + "P references missing container object "
                                  + std::to_string(cmd.arg3));
                }
                break;
            case 'D':
                if (!world.rooms.empty() && cmd.arg1 > 0 && !has_room(world, cmd.arg1)) {
                    add_issue(report,
                              ValidationSeverity::error,
                              "reset",
                              where + "D references missing room " + std::to_string(cmd.arg1));
                }
                break;
            default:
                break;
            }
        }
    }
}

void validate_shops(const World& world, ValidationReport& report) {
    for (const auto& shop : world.shops) {
        const std::string where = "shop " + std::to_string(shop.vnum) + " ";

        if (!world.mobiles.empty() && shop.keeper > 0 && !has_mobile(world, shop.keeper)) {
            add_issue(report,
                      ValidationSeverity::error,
                      "shop",
                      where + "keeper mobile " + std::to_string(shop.keeper) + " not found",
                      ValidationTarget::shop,
                      shop.vnum);
        }
        if (!world.rooms.empty() && shop.in_room > 0 && !has_room(world, shop.in_room)) {
            add_issue(report,
                      ValidationSeverity::error,
                      "shop",
                      where + "room " + std::to_string(shop.in_room) + " not found",
                      ValidationTarget::shop,
                      shop.vnum);
        }
        if (!world.objects.empty()) {
            for (int obj_vnum : shop.producing) {
                if (obj_vnum > 0 && !has_object(world, obj_vnum)) {
                    add_issue(report,
                              ValidationSeverity::error,
                              "shop",
                              where + "produces missing object " + std::to_string(obj_vnum),
                              ValidationTarget::shop,
                              shop.vnum);
                }
            }
        }
    }
}

void validate_guilds(const World& world, ValidationReport& report) {
    for (const auto& guild : world.guilds) {
        const std::string where = "guild " + guild.base_filename + " ";

        if (!world.mobiles.empty()) {
            if (guild.guard_mob > 0 && !has_mobile(world, guild.guard_mob)) {
                add_issue(report,
                          ValidationSeverity::error,
                          "guild",
                          where + "guard mob " + std::to_string(guild.guard_mob) + " not found");
            }
            if (guild.banker_mob > 0 && !has_mobile(world, guild.banker_mob)) {
                add_issue(report,
                          ValidationSeverity::error,
                          "guild",
                          where + "banker mob " + std::to_string(guild.banker_mob) + " not found");
            }
            if (guild.banker_xp_mob > 0 && !has_mobile(world, guild.banker_xp_mob)) {
                add_issue(report,
                          ValidationSeverity::error,
                          "guild",
                          where + "xp banker mob " + std::to_string(guild.banker_xp_mob)
                              + " not found");
            }
        }
        if (!world.objects.empty() && guild.member_book_obj > 0
            && !has_object(world, guild.member_book_obj)) {
            add_issue(report,
                      ValidationSeverity::error,
                      "guild",
                      where + "member book object " + std::to_string(guild.member_book_obj)
                          + " not found");
        }
        if (!world.rooms.empty()) {
            if (guild.guard_room > 0 && !has_room(world, guild.guard_room)) {
                add_issue(report,
                          ValidationSeverity::error,
                          "guild",
                          where + "guard room " + std::to_string(guild.guard_room) + " not found");
            }
            if (guild.bank_room > 0 && !has_room(world, guild.bank_room)) {
                add_issue(report,
                          ValidationSeverity::error,
                          "guild",
                          where + "bank room " + std::to_string(guild.bank_room) + " not found");
            }
            if (guild.bank_xp_room > 0 && !has_room(world, guild.bank_xp_room)) {
                add_issue(report,
                          ValidationSeverity::error,
                          "guild",
                          where + "xp bank room " + std::to_string(guild.bank_xp_room)
                              + " not found");
            }
        }
    }
}

void validate_special_procs(const World& world, ValidationReport& report) {
    std::map<std::pair<char, long>, std::size_t> seen_entries;

    for (const auto& spe : world.special_procs) {
        const std::string where = std::string(1, spe.type) + " " + std::to_string(spe.vnum)
                                  + " (" + spe.procedure + ") ";

        const auto key = std::make_pair(static_cast<char>(std::tolower(static_cast<unsigned char>(spe.type))),
                                        spe.vnum);
        const auto [it, inserted] = seen_entries.emplace(key, 1);
        if (!inserted) {
            ++it->second;
            add_issue(report,
                      ValidationSeverity::warning,
                      "special",
                      where + "duplicate special proc assignment for vnum "
                          + std::to_string(spe.vnum));
        }

        if (spe.procedure.empty()) {
            add_issue(report,
                      ValidationSeverity::warning,
                      "special",
                      where + "procedure name is empty");
        } else if (!is_known_special_proc(spe.type, spe.procedure)) {
            add_issue(report,
                      ValidationSeverity::warning,
                      "special",
                      where + "unknown procedure name (not in server catalog)");
        }

        switch (spe.type) {
        case 'm':
        case 'M':
            if (!world.mobiles.empty() && !has_mobile(world, spe.vnum)) {
                add_issue(report,
                          ValidationSeverity::error,
                          "special",
                          where + "mobile not found");
            } else if (has_mobile(world, spe.vnum)) {
                const auto& mob = world.mobiles.at(spe.vnum);
                if ((mob.act & kMobActSpecFlag) == 0) {
                    add_issue(report,
                              ValidationSeverity::warning,
                              "special",
                              where + "mobile #" + std::to_string(spe.vnum)
                                  + " has special proc but ACT_SPEC flag is not set",
                              ValidationTarget::mob,
                              spe.vnum);
                }
            }
            break;
        case 'o':
        case 'O':
            if (!world.objects.empty() && !has_object(world, spe.vnum)) {
                add_issue(report,
                          ValidationSeverity::error,
                          "special",
                          where + "object not found");
            }
            break;
        case 'r':
        case 'R':
            if (!world.rooms.empty() && !has_room(world, spe.vnum)) {
                add_issue(report,
                          ValidationSeverity::error,
                          "special",
                          where + "room not found");
            }
            break;
        default:
            add_issue(report,
                      ValidationSeverity::warning,
                      "special",
                      where + "unsupported type (expected m, o, or r)");
            break;
        }
    }

    if (world.mobiles.empty()) {
        return;
    }

    std::set<long> mobile_proc_vnums;
    for (const auto& spe : world.special_procs) {
        if (std::tolower(static_cast<unsigned char>(spe.type)) == 'm') {
            mobile_proc_vnums.insert(spe.vnum);
        }
    }

    for (const auto& [vnum, mob] : world.mobiles) {
        if ((mob.act & kMobActSpecFlag) == 0) {
            continue;
        }
        if (mobile_proc_vnums.find(vnum) == mobile_proc_vnums.end()) {
            add_issue(report,
                      ValidationSeverity::warning,
                      "special",
                      "mobile #" + std::to_string(vnum) + " (" + mob.short_descr
                          + ") has ACT_SPEC but no M entry in myst.spe",
                      ValidationTarget::mob,
                      vnum);
        }
    }
}

void validate_socials(const World& world, ValidationReport& report) {
    for (std::size_t i = 0; i < world.social_messages.size(); ++i) {
        for (std::size_t j = i + 1; j < world.social_messages.size(); ++j) {
            if (world.social_messages[i].act_nr == world.social_messages[j].act_nr) {
                add_issue(report,
                          ValidationSeverity::warning,
                          "social",
                          "duplicate act_nr " + std::to_string(world.social_messages[i].act_nr));
                break;
            }
        }
    }
}

} // namespace

bool ValidationReport::ok() const {
    return error_count() == 0;
}

std::size_t ValidationReport::error_count() const {
    std::size_t count = 0;
    for (const auto& issue : issues) {
        if (issue.severity == ValidationSeverity::error) {
            ++count;
        }
    }
    return count;
}

std::size_t ValidationReport::warning_count() const {
    std::size_t count = 0;
    for (const auto& issue : issues) {
        if (issue.severity == ValidationSeverity::warning) {
            ++count;
        }
    }
    return count;
}

ValidationReport validate_rooms(const World& world,
                                const ValidationOptions& options,
                                const std::vector<long>* room_vnums) {
    ValidationReport report;
    append_room_validation(world, report, options, room_vnums);
    return report;
}

ValidationReport validate_world(const World& world, const ValidationOptions& options) {
    ValidationReport report;
    append_room_validation(world, report, options, nullptr);
    validate_resets(world, report);
    validate_shops(world, report);
    validate_guilds(world, report);
    validate_special_procs(world, report);
    validate_socials(world, report);
    return report;
}

ValidationReport validate_translatable_rooms(const World& world,
                                             const ValidationOptions& options,
                                             const std::vector<long>* room_vnums) {
    ValidationReport report;

    const auto validate_room = [&](long vnum, const Room& room) {
        if (options.max_line_length > 0) {
            auto check_field = [&](const std::string& text, const std::string& field_label) {
                const TextLineLengthReport line_report =
                    check_text_line_lengths(text, options.max_line_length);
                for (const auto& issue : line_report.overlong) {
                    add_issue(report,
                              ValidationSeverity::warning,
                              "room_text",
                              "room " + std::to_string(vnum) + " " + field_label + " line "
                                  + std::to_string(issue.line_number) + " has "
                                  + std::to_string(issue.length) + " characters (max "
                                  + std::to_string(options.max_line_length) + ")",
                              ValidationTarget::room,
                              vnum);
                }
            };

            check_field(room.name, "name");
            check_field(room.description, "description");
            for (std::size_t extra_index = 0; extra_index < room.extra_descs.size(); ++extra_index) {
                check_field(room.extra_descs[extra_index].description,
                            "extra desc " + std::to_string(extra_index));
            }
            for (std::size_t exit_index = 0; exit_index < room.exits.size(); ++exit_index) {
                check_field(room.exits[exit_index].description,
                            "exit " + std::to_string(exit_index) + " description");
            }
        }

        auto warn_non_ascii = [&](const std::string& text, const std::string& field_label) {
            if (is_mud_ascii_text(text)) {
                return;
            }
            add_issue(report,
                      ValidationSeverity::warning,
                      "room_text",
                      "room " + std::to_string(vnum) + " " + field_label + " contains "
                          + std::to_string(count_non_mud_ascii_chars(text))
                          + " non-ASCII characters",
                      ValidationTarget::room,
                      vnum);
        };

        warn_non_ascii(room.name, "name");
        warn_non_ascii(room.description, "description");
        for (std::size_t extra_index = 0; extra_index < room.extra_descs.size(); ++extra_index) {
            warn_non_ascii(room.extra_descs[extra_index].description,
                           "extra desc " + std::to_string(extra_index));
        }
        for (std::size_t exit_index = 0; exit_index < room.exits.size(); ++exit_index) {
            warn_non_ascii(room.exits[exit_index].description,
                           "exit " + std::to_string(exit_index) + " description");
        }
    };

    if (room_vnums == nullptr || room_vnums->empty()) {
        for (const auto& [vnum, room] : world.rooms) {
            validate_room(vnum, room);
        }
        return report;
    }

    for (long vnum : *room_vnums) {
        const Room* room = world.find_room(vnum);
        if (room == nullptr) {
            continue;
        }
        validate_room(vnum, *room);
    }
    return report;
}

} // namespace nebbie
