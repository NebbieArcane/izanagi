#include "shell.hpp"

#include "cli_parse.hpp"

#include "nebbie/edit.hpp"
#include "nebbie/edit.hpp"
#include "nebbie/io.hpp"
#include "nebbie/validate.hpp"

#include <filesystem>
#include <iostream>
#include <map>

namespace nebbiedit {

namespace {

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

const char* inbound_exit_status(const std::string& exit_description, const std::string& destination_name) {
    if (trim_copy(exit_description).empty()) {
        return "vuota";
    }
    if (strings_equal_ci(exit_description, destination_name)) {
        return "allineata";
    }
    if (nebbie::is_custom_exit_look_text(exit_description)) {
        return "personalizzata";
    }
    return "da allineare";
}

void print_validation(const nebbie::ValidationReport& report) {
    for (const auto& issue : report.issues) {
        const char* level = issue.severity == nebbie::ValidationSeverity::error ? "ERROR" : "WARN";
        std::cout << level << " [" << issue.category << "] " << issue.message << '\n';
    }
    std::cout << report.error_count() << " error(s), " << report.warning_count() << " warning(s)\n";
}

void print_info(const nebbie::World& world) {
    std::cout << "Zones: " << world.zones.size() << '\n';
    std::cout << "Rooms: " << world.rooms.size() << '\n';
    std::cout << "Mobiles: " << world.mobiles.size() << '\n';
    std::cout << "Objects: " << world.objects.size() << '\n';
    std::cout << "Shops: " << world.shops.size() << '\n';
    std::cout << "Special procs: " << world.special_procs.size() << '\n';
}

bool persist_world(nebbie::World& world, const nebbie::LibContext& context, bool force) {
    const nebbie::ValidationReport report = nebbie::validate_world(world);
    if (!report.issues.empty()) {
        print_validation(report);
    }
    if (!report.ok() && !force) {
        std::cerr << "Save aborted. Use --force to save anyway.\n";
        return false;
    }
    nebbie::save_lib(world, context, [](const std::string& msg) {
        std::cout << msg << '\n';
    });
    std::cout << "Saved to " << context.root << '\n';
    return true;
}

void print_shell_help() {
    std::cout
        << "Commands:\n"
        << "  help\n"
        << "  info\n"
        << "  validate\n"
        << "  save [--force]\n"
        << "  room show <vnum>\n"
        << "  room inbound <vnum>\n"
        << "  room set <vnum> --name <text> [--desc <text>] [--sector N] [--flags N]\n"
        << "  mob show <vnum>\n"
        << "  mob set <vnum> --short <text> [--long <text>] [--level N] [--alignment N]\n"
        << "  obj show <vnum>\n"
        << "  obj set <vnum> --short <text> [--cost N] [--weight N]\n"
        << "  quit\n";
}

bool handle_shell_command(const std::vector<std::string>& tokens,
                          nebbie::World& world,
                          nebbie::LibContext& context) {
    if (tokens.empty()) {
        return true;
    }

    const std::string& cmd = tokens[0];
    if (cmd == "help" || cmd == "?") {
        print_shell_help();
        return true;
    }
    if (cmd == "quit" || cmd == "exit") {
        return false;
    }
    if (cmd == "info") {
        print_info(world);
        return true;
    }
    if (cmd == "validate") {
        print_validation(nebbie::validate_world(world));
        return true;
    }
    if (cmd == "save") {
        const auto flags = parse_flags(tokens, 1);
        const bool force = flags.count("force") > 0;
        return persist_world(world, context, force);
    }

    if (cmd == "room" && tokens.size() >= 3) {
        const long vnum = std::stol(tokens[2]);
        if (tokens[1] == "show") {
            print_room_show(world, vnum);
            return true;
        }
        if (tokens[1] == "inbound") {
            print_room_inbound(world, vnum);
            return true;
        }
        if (tokens[1] == "set") {
            const auto flags = parse_flags(tokens, 3);
            const nebbie::RoomEdit edit = nebbie::room_edit_from_flags(flags);
            if (!nebbie::edit_room(world, vnum, edit)) {
                std::cerr << "Room not found: " << vnum << '\n';
                return true;
            }
            std::cout << "Updated room " << vnum << '\n';
            return true;
        }
    }

    if (cmd == "mob" && tokens.size() >= 3) {
        const long vnum = std::stol(tokens[2]);
        if (tokens[1] == "show") {
            const nebbie::Mobile* mob = world.find_mobile(vnum);
            if (!mob) {
                std::cerr << "Mobile not found: " << vnum << '\n';
                return true;
            }
            std::cout << "#" << mob->vnum << " " << mob->short_descr << '\n';
            std::cout << mob->long_descr << '\n';
            std::cout << "Level: " << mob->level << " Alignment: " << mob->alignment << '\n';
            return true;
        }
        if (tokens[1] == "set") {
            const auto flags = parse_flags(tokens, 3);
            const nebbie::MobEdit edit = nebbie::mob_edit_from_flags(flags);
            if (!nebbie::edit_mob(world, vnum, edit)) {
                std::cerr << "Mobile not found: " << vnum << '\n';
                return true;
            }
            std::cout << "Updated mobile " << vnum << '\n';
            return true;
        }
    }

    if (cmd == "obj" && tokens.size() >= 3) {
        const long vnum = std::stol(tokens[2]);
        if (tokens[1] == "show") {
            const nebbie::GameObject* obj = world.find_object(vnum);
            if (!obj) {
                std::cerr << "Object not found: " << vnum << '\n';
                return true;
            }
            std::cout << "#" << obj->vnum << " " << obj->short_descr << '\n';
            std::cout << "Cost: " << obj->cost << " Weight: " << obj->weight << '\n';
            return true;
        }
        if (tokens[1] == "set") {
            const auto flags = parse_flags(tokens, 3);
            const nebbie::ObjEdit edit = nebbie::obj_edit_from_flags(flags);
            if (!nebbie::edit_object(world, vnum, edit)) {
                std::cerr << "Object not found: " << vnum << '\n';
                return true;
            }
            std::cout << "Updated object " << vnum << '\n';
            return true;
        }
    }

    std::cerr << "Unknown command. Type 'help'.\n";
    return true;
}

} // namespace

void print_room_show(const nebbie::World& world, const long vnum) {
    const nebbie::Room* room = world.find_room(vnum);
    if (!room) {
        std::cerr << "Room not found: " << vnum << '\n';
        return;
    }
    std::cout << "#" << room->vnum << " " << room->name << '\n';
    std::cout << room->description << '\n';
    std::cout << "Flags: " << room->room_flags << " Sector: " << room->sector_type << '\n';
    std::cout << "Exits: " << room->exits.size() << '\n';
    for (const auto& exit : room->exits) {
        const nebbie::Room* target = world.find_room(exit.to_room);
        std::cout << "  " << nebbie::exit_direction_name(exit.direction) << " -> #"
                  << exit.to_room;
        if (target) {
            std::cout << " \"" << target->name << "\"";
        }
        std::cout << "  desc=\"" << exit.description << "\"";
        if (!exit.keyword.empty()) {
            std::cout << "  kw=\"" << exit.keyword << "\"";
        }
        if (exit.key >= 0) {
            std::cout << "  key=" << exit.key;
        }
        std::cout << '\n';
    }
}

void print_room_inbound(const nebbie::World& world, const long vnum) {
    const nebbie::Room* destination = world.find_room(vnum);
    if (!destination) {
        std::cerr << "Room not found: " << vnum << '\n';
        return;
    }
    std::cout << "Inbound exits to #" << vnum << " \"" << destination->name << "\":\n";
    std::size_t count = 0;
    for (const auto& [from_vnum, room] : world.rooms) {
        for (const auto& exit : room.exits) {
            if (exit.to_room != vnum) {
                continue;
            }
            ++count;
            std::cout << "  #" << from_vnum << " \"" << room.name << "\" "
                      << nebbie::exit_direction_name(exit.direction) << " -> #" << vnum
                      << "  desc=\"" << exit.description << "\""
                      << "  [" << inbound_exit_status(exit.description, destination->name)
                      << "]\n";
        }
        (void)from_vnum;
    }
    if (count == 0) {
        std::cout << "  (nessuna)\n";
    }
}

bool run_room_show(const std::filesystem::path& lib_root, const long vnum) {
    nebbie::World world;
    nebbie::load_lib(world, lib_root, [](const std::string& msg) {
        std::cout << msg << '\n';
    });
    print_room_show(world, vnum);
    return world.find_room(vnum) != nullptr;
}

bool run_room_inbound(const std::filesystem::path& lib_root, const long vnum) {
    nebbie::World world;
    nebbie::load_lib(world, lib_root, [](const std::string& msg) {
        std::cout << msg << '\n';
    });
    print_room_inbound(world, vnum);
    return world.find_room(vnum) != nullptr;
}

bool run_room_list(const std::filesystem::path& lib_root, const std::string& prefix) {
    nebbie::World world;
    nebbie::load_lib(world, lib_root, [](const std::string& msg) {
        std::cout << msg << '\n';
    });
    std::cout << "Rooms: " << world.rooms.size() << '\n';
    for (const auto& [vnum, room] : world.rooms) {
        if (!prefix.empty()) {
            const std::string vnum_text = std::to_string(vnum);
            if (vnum_text.rfind(prefix, 0) != 0) {
                continue;
            }
        }
        std::cout << "#" << vnum << " " << room.name << '\n';
    }
    return true;
}

int run_shell(const std::filesystem::path& lib_root) {
    nebbie::World world;
    nebbie::LibContext context;
    nebbie::load_lib(world, lib_root, context, [](const std::string& msg) {
        std::cout << msg << '\n';
    });

    std::cout << "Nebbie Editor MVP — loaded " << lib_root << '\n';
    print_info(world);
    print_shell_help();

    std::string line;
    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, line)) {
            break;
        }
        if (line.empty()) {
            continue;
        }
        const auto tokens = split_command_line(line);
        if (!handle_shell_command(tokens, world, context)) {
            break;
        }
    }

    return 0;
}

bool run_room_set(const std::filesystem::path& lib_root,
                  long vnum,
                  const std::map<std::string, std::string>& flags,
                  bool force) {
    nebbie::World world;
    nebbie::LibContext context;
    nebbie::load_lib(world, lib_root, context);
    if (!nebbie::edit_room(world, vnum, nebbie::room_edit_from_flags(flags))) {
        std::cerr << "Room not found: " << vnum << '\n';
        return false;
    }
    return persist_world(world, context, force);
}

bool run_mob_set(const std::filesystem::path& lib_root,
                 long vnum,
                 const std::map<std::string, std::string>& flags,
                 bool force) {
    nebbie::World world;
    nebbie::LibContext context;
    nebbie::load_lib(world, lib_root, context);
    if (!nebbie::edit_mob(world, vnum, nebbie::mob_edit_from_flags(flags))) {
        std::cerr << "Mobile not found: " << vnum << '\n';
        return false;
    }
    return persist_world(world, context, force);
}

bool run_obj_set(const std::filesystem::path& lib_root,
                 long vnum,
                 const std::map<std::string, std::string>& flags,
                 bool force) {
    nebbie::World world;
    nebbie::LibContext context;
    nebbie::load_lib(world, lib_root, context);
    if (!nebbie::edit_object(world, vnum, nebbie::obj_edit_from_flags(flags))) {
        std::cerr << "Object not found: " << vnum << '\n';
        return false;
    }
    return persist_world(world, context, force);
}

} // namespace nebbiedit
