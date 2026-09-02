#include "nebbie/io.hpp"
#include "nebbie/overlay_io.hpp"
#include "nebbie/validate.hpp"
#include "nebbie/world.hpp"
#include "nebbie/zone_graph.hpp"
#include "nebbie/zone_partition.hpp"

#include "cli_parse.hpp"
#include "shell.hpp"

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <cctype>

namespace {

nebbie::World g_world;

nebbie::ZonePartitionOptions parse_zone_partition_options(int argc, char** argv, int start_index) {
    nebbie::ZonePartitionOptions options;
    for (int i = start_index; i < argc; ++i) {
        const std::string flag = argv[i];
        if (flag == "--no-overlays") {
            options.write_overlay_dirs = false;
        } else if (flag == "--no-monoliths") {
            options.write_monolith_files = false;
        } else if (flag == "--no-shops") {
            options.include_shop_entities = false;
        } else {
            throw std::runtime_error("Unknown zone partition flag: " + flag);
        }
    }
    return options;
}

void print_zone_partition_report(const nebbie::ZonePartitionReport& report) {
    std::cout << "Zone packs written: " << report.zones_written << '\n';
    std::cout << "Rooms: " << report.rooms
              << " Objects: " << report.objects
              << " Mobiles: " << report.mobiles
              << " Zone resets: " << report.zone_resets << '\n';
    for (const auto& warning : report.warnings) {
        std::cout << "WARN: " << warning << '\n';
    }
}

void usage() {
    std::cout
        << "Nebbie Arcane World Editor (CLI)\n\n"
        << "Usage:\n"
        << "  nebbiedit info <lib-directory>\n"
        << "  nebbiedit load <lib-directory>   (loads in-process; use edit for a session)\n"
        << "  nebbiedit edit <lib-directory> (interactive session)\n"
        << "  nebbiedit zone list\n"
        << "  nebbiedit zone show <zone-number>\n"
        << "  nebbiedit zone rooms <zone-number>\n"
        << "  nebbiedit zone graph <zone-number> [--dot]\n"
        << "  nebbiedit zone split <lib-directory> <output-directory> [--no-overlays|--no-monoliths|--no-shops]\n"
        << "  nebbiedit zone split-one <lib-directory> <zone-number> <output-directory> [--no-overlays|--no-monoliths|--no-shops]\n"
        << "  nebbiedit zone merge <zones-root> <merged-lib-directory>\n"
        << "  nebbiedit room list <lib-directory> [vnum-prefix]\n"
        << "  nebbiedit room show <lib-directory> <vnum>\n"
        << "  nebbiedit room inbound <lib-directory> <vnum>\n"
        << "  nebbiedit mob list\n"
        << "  nebbiedit mob show <vnum>\n"
        << "  nebbiedit obj list\n"
        << "  nebbiedit obj show <vnum>\n"
        << "  nebbiedit shop list\n"
        << "  nebbiedit shop show <vnum>\n"
        << "  nebbiedit spe list\n"
        << "  nebbiedit spe show <vnum>\n"
        << "  nebbiedit dam list\n"
        << "  nebbiedit dam show <attack-type>\n"
        << "  nebbiedit social list\n"
        << "  nebbiedit social show <act-nr>\n"
        << "  nebbiedit pose list\n"
        << "  nebbiedit pose show <level>\n"
        << "  nebbiedit guild list\n"
        << "  nebbiedit guild show <name>\n"
        << "  nebbiedit validate <lib-directory>\n"
        << "  nebbiedit repair-wld <lib-directory>   (fix TUNNEL moblim lines for server boot)\n"
        << "  nebbiedit check mob <myst.mob-path>\n"
        << "  nebbiedit check obj <myst.obj-path>\n"
        << "  nebbiedit check wld <myst.wld-path>\n"
        << "  nebbiedit check lib <lib-directory>\n"
        << "  nebbiedit edit <lib-directory>\n"
        << "  nebbiedit room set <lib-directory> <vnum> [--name T] [--desc T] [--sector N]\n"
        << "  nebbiedit mob set <lib-directory> <vnum> [--short T] [--level N] [--alignment N]\n"
        << "  nebbiedit obj set <lib-directory> <vnum> [--short T] [--cost N] [--weight N]\n"
        << "  (append --force to one-shot set commands to save despite validation errors)\n"
        << "  nebbiedit convert zon roundtrip <lib-directory> <output-directory>\n"
        << "  nebbiedit convert lib roundtrip <lib-directory> <output-directory>\n"
        << "  nebbiedit overlay export <lib-directory> [--rooms|--objects|--mobiles|--zones]\n\n"
        << "Reference server: https://github.com/NebbieArcane/Server\n"
        << "Test server fork: https://github.com/wizardmorgan/nebbietest\n";
}

void print_info(const nebbie::World& world) {
    std::cout << "Zones: " << world.zones.size() << '\n';
    std::cout << "Rooms: " << world.rooms.size() << '\n';
    std::cout << "Mobiles: " << world.mobiles.size() << '\n';
    std::cout << "Objects: " << world.objects.size() << '\n';
    std::cout << "Shops: " << world.shops.size() << '\n';
    std::cout << "Special procs: " << world.special_procs.size() << '\n';
    std::cout << "Damage messages: " << world.damage_messages.size() << '\n';
    std::cout << "Socials: " << world.social_messages.size() << '\n';
    std::cout << "Poses: " << world.pose_entries.size() << '\n';
    std::cout << "Guilds: " << world.guilds.size() << '\n';
}

bool run(int argc, char** argv) {
    if (argc < 2) {
        usage();
        return false;
    }

    const std::string cmd = argv[1];

    try {
        if (cmd == "info" || cmd == "load") {
            if (argc < 3) {
                usage();
                return false;
            }
            nebbie::load_lib(g_world, argv[2], [](const std::string& msg) {
                std::cout << msg << '\n';
            });
            print_info(g_world);
            return true;
        }

        if (cmd == "zone") {
            if (argc < 3) {
                usage();
                return false;
            }
            const std::string sub = argv[2];
            if (sub == "list") {
                for (const auto& zone : g_world.zones) {
                    std::cout << zone.num << " " << zone.name
                              << " [" << zone.bottom << "-" << zone.top << "]"
                              << " resets=" << zone.commands.size() << '\n';
                }
                return true;
            }
            if (sub == "show" && argc >= 4) {
                const int num = std::stoi(argv[3]);
                for (const auto& zone : g_world.zones) {
                    if (zone.num != num) {
                        continue;
                    }
                    std::cout << "#" << zone.num << " " << zone.name << '\n';
                    std::cout << "Range: " << zone.bottom << "-" << zone.top << '\n';
                    std::cout << "Lifespan: " << zone.lifespan
                              << " Reset mode: " << zone.reset_mode << '\n';
                    for (const auto& reset : zone.commands) {
                        if (reset.command == '*') {
                            std::cout << "*" << reset.raw_line;
                            continue;
                        }
                        std::cout << reset.command << ' '
                                  << reset.if_flag << ' '
                                  << reset.arg1 << ' '
                                  << reset.arg2 << ' '
                                  << reset.arg3 << ' '
                                  << reset.arg4 << '\n';
                    }
                    return true;
                }
                std::cerr << "Zone not loaded: " << num << '\n';
                return false;
            }
            if (sub == "rooms" && argc >= 4) {
                const int num = std::stoi(argv[3]);
                const auto rooms = nebbie::rooms_in_zone(g_world, num);
                if (rooms.empty()) {
                    std::cerr << "No rooms in zone " << num << " (zone not loaded?)\n";
                    return false;
                }
                for (long vnum : rooms) {
                    const nebbie::Room* room = g_world.find_room(vnum);
                    std::cout << vnum;
                    if (room) {
                        std::cout << " " << room->name;
                    }
                    std::cout << '\n';
                }
                return true;
            }
            if (sub == "graph" && argc >= 4) {
                const int num = std::stoi(argv[3]);
                const nebbie::ZoneGraph graph = nebbie::build_zone_graph(g_world, num);
                if (graph.nodes.empty()) {
                    std::cerr << "No rooms in zone " << num << " (zone not loaded?)\n";
                    return false;
                }
                const bool dot = argc >= 5 && std::string(argv[4]) == "--dot";
                if (dot) {
                    std::cout << nebbie::zone_graph_to_dot(graph);
                } else {
                    std::cout << "Zone " << graph.zone_num << " " << graph.zone_name
                              << " [" << graph.bottom << "-" << graph.top << "]\n";
                    std::cout << "Rooms: " << graph.nodes.size()
                              << " Edges: " << graph.edges.size() << '\n';
                    std::cout << "Use --dot for Graphviz output.\n";
                }
                return true;
            }
            if (sub == "split" && argc >= 5) {
                nebbie::World world;
                nebbie::load_lib(world, argv[3], [](const std::string& msg) {
                    std::cout << msg << '\n';
                });
                const auto options = parse_zone_partition_options(argc, argv, 5);
                const auto report = nebbie::export_all_zone_packs(world, argv[4], options,
                                                                  [](const std::string& msg) {
                                                                      std::cout << msg << '\n';
                                                                  });
                print_zone_partition_report(report);
                return true;
            }
            if (sub == "split-one" && argc >= 6) {
                nebbie::World world;
                nebbie::load_lib(world, argv[3], [](const std::string& msg) {
                    std::cout << msg << '\n';
                });
                const int zone_num = std::stoi(argv[4]);
                const auto options = parse_zone_partition_options(argc, argv, 6);
                const auto report = nebbie::export_zone_pack(world, argv[5], zone_num, options,
                                                              [](const std::string& msg) {
                                                                  std::cout << msg << '\n';
                                                                  });
                print_zone_partition_report(report);
                return true;
            }
            if (sub == "merge" && argc >= 5) {
                const auto report = nebbie::merge_zone_packs(argv[3], argv[4],
                                                             [](const std::string& msg) {
                                                                 std::cout << msg << '\n';
                                                             });
                print_zone_partition_report(report);
                return true;
            }
        }

        if (cmd == "room" && argc >= 5 && std::string(argv[2]) == "show") {
            const long vnum = std::stol(argv[4]);
            return nebbiedit::run_room_show(argv[3], vnum);
        }

        if (cmd == "room" && argc >= 5 && std::string(argv[2]) == "inbound") {
            const long vnum = std::stol(argv[4]);
            return nebbiedit::run_room_inbound(argv[3], vnum);
        }

        if (cmd == "room" && argc >= 4 && std::string(argv[2]) == "list") {
            const std::string prefix = argc >= 5 ? argv[4] : std::string{};
            return nebbiedit::run_room_list(argv[3], prefix);
        }

        if (cmd == "mob") {
            if (argc < 3) {
                usage();
                return false;
            }
            const std::string sub = argv[2];
            if (sub == "list") {
                for (const auto& [vnum, mob] : g_world.mobiles) {
                    std::cout << vnum << " " << mob.short_descr << '\n';
                }
                return true;
            }
            if (sub == "show" && argc >= 4) {
                const long vnum = std::stol(argv[3]);
                const nebbie::Mobile* mob = g_world.find_mobile(vnum);
                if (!mob) {
                    std::cerr << "Mobile not loaded: " << vnum << '\n';
                    return false;
                }
                std::cout << "#" << mob->vnum << " " << mob->short_descr << '\n';
                std::cout << "Type: " << mob->mobtype
                          << " Level: " << mob->level
                          << " Alignment: " << mob->alignment << '\n';
                std::cout << "Hit: " << mob->hit_dice
                          << " Dam: " << mob->dam_dice << '\n';
                return true;
            }
        }

        if (cmd == "obj") {
            if (argc < 3) {
                usage();
                return false;
            }
            const std::string sub = argv[2];
            if (sub == "list") {
                for (const auto& [vnum, obj] : g_world.objects) {
                    std::cout << vnum << " " << obj.short_descr << '\n';
                }
                return true;
            }
            if (sub == "show" && argc >= 4) {
                const long vnum = std::stol(argv[3]);
                const nebbie::GameObject* obj = g_world.find_object(vnum);
                if (!obj) {
                    std::cerr << "Object not loaded: " << vnum << '\n';
                    return false;
                }
                std::cout << "#" << obj->vnum << " " << obj->short_descr << '\n';
                std::cout << "Type: " << obj->type_flag
                          << " Weight: " << obj->weight
                          << " Cost: " << obj->cost << '\n';
                std::cout << "Affects: " << obj->affects.size()
                          << " Extras: " << obj->extra_descs.size() << '\n';
                return true;
            }
        }

        if (cmd == "shop") {
            if (argc < 3) {
                usage();
                return false;
            }
            const std::string sub = argv[2];
            if (sub == "list") {
                for (const auto& shop : g_world.shops) {
                    std::cout << shop.vnum << " keeper=" << shop.keeper
                              << " room=" << shop.in_room << '\n';
                }
                return true;
            }
            if (sub == "show" && argc >= 4) {
                const long vnum = std::stol(argv[3]);
                for (const auto& shop : g_world.shops) {
                    if (shop.vnum != vnum) {
                        continue;
                    }
                    std::cout << "#" << shop.vnum << " keeper=" << shop.keeper
                              << " room=" << shop.in_room << '\n';
                    std::cout << "Buy x" << shop.profit_buy
                              << " Sell x" << shop.profit_sell << '\n';
                    std::cout << "Hours: " << shop.open1 << "-" << shop.close1
                              << ", " << shop.open2 << "-" << shop.close2 << '\n';
                    return true;
                }
                std::cerr << "Shop not loaded: " << vnum << '\n';
                return false;
            }
        }

        if (cmd == "spe") {
            if (argc < 3) {
                usage();
                return false;
            }
            const std::string sub = argv[2];
            if (sub == "list") {
                for (const auto& spe : g_world.special_procs) {
                    std::cout << static_cast<char>(std::toupper(spe.type))
                              << ' ' << spe.vnum << ' ' << spe.procedure;
                    if (!spe.params.empty()) {
                        std::cout << ' ' << spe.params;
                    }
                    std::cout << '\n';
                }
                return true;
            }
            if (sub == "show" && argc >= 4) {
                const long vnum = std::stol(argv[3]);
                bool found = false;
                for (const auto& spe : g_world.special_procs) {
                    if (spe.vnum != vnum) {
                        continue;
                    }
                    found = true;
                    std::cout << static_cast<char>(std::toupper(spe.type))
                              << ' ' << spe.vnum << ' ' << spe.procedure << '\n';
                    if (!spe.params.empty()) {
                        std::cout << "Params: " << spe.params << '\n';
                    }
                }
                if (!found) {
                    std::cerr << "No special proc for vnum: " << vnum << '\n';
                    return false;
                }
                return true;
            }
        }

        if (cmd == "dam") {
            if (argc < 3) {
                usage();
                return false;
            }
            const std::string sub = argv[2];
            if (sub == "list") {
                for (const auto& msg : g_world.damage_messages) {
                    std::cout << msg.attack_type << " hit="
                              << msg.hit_attacker.substr(0, 40) << "...\n";
                }
                return true;
            }
            if (sub == "show" && argc >= 4) {
                const int type = std::stoi(argv[3]);
                for (const auto& msg : g_world.damage_messages) {
                    if (msg.attack_type != type) {
                        continue;
                    }
                    std::cout << "Attack type " << msg.attack_type << '\n';
                    std::cout << "Die: " << msg.die_attacker << '\n';
                    std::cout << "Hit: " << msg.hit_attacker << '\n';
                    return true;
                }
                std::cerr << "Damage message not loaded: " << type << '\n';
                return false;
            }
        }

        if (cmd == "social") {
            if (argc < 3) {
                usage();
                return false;
            }
            const std::string sub = argv[2];
            if (sub == "list") {
                for (const auto& msg : g_world.social_messages) {
                    std::cout << msg.act_nr << " hide=" << msg.hide
                              << " " << msg.char_no_arg.substr(0, 40) << '\n';
                }
                return true;
            }
            if (sub == "show" && argc >= 4) {
                const int act_nr = std::stoi(argv[3]);
                for (const auto& msg : g_world.social_messages) {
                    if (msg.act_nr != act_nr) {
                        continue;
                    }
                    std::cout << "#" << msg.act_nr
                              << " hide=" << msg.hide
                              << " min_pos=" << msg.min_victim_position << '\n';
                    std::cout << "No arg: " << msg.char_no_arg << '\n';
                    if (!msg.char_found.empty()) {
                        std::cout << "Found: " << msg.char_found << '\n';
                    }
                    return true;
                }
                std::cerr << "Social not loaded: " << act_nr << '\n';
                return false;
            }
        }

        if (cmd == "pose") {
            if (argc < 3) {
                usage();
                return false;
            }
            const std::string sub = argv[2];
            if (sub == "list") {
                for (const auto& entry : g_world.pose_entries) {
                    std::cout << entry.level << " "
                              << entry.poser_msg[0].substr(0, 40) << '\n';
                }
                return true;
            }
            if (sub == "show" && argc >= 4) {
                const int level = std::stoi(argv[3]);
                for (const auto& entry : g_world.pose_entries) {
                    if (entry.level != level) {
                        continue;
                    }
                    std::cout << "Level " << entry.level << '\n';
                    for (int i = 0; i < 4; ++i) {
                        std::cout << "Class " << i << ": " << entry.poser_msg[i] << '\n';
                    }
                    return true;
                }
                std::cerr << "Pose not loaded: level " << level << '\n';
                return false;
            }
        }

        if (cmd == "guild") {
            if (argc < 3) {
                usage();
                return false;
            }
            const std::string sub = argv[2];
            if (sub == "list") {
                for (const auto& guild : g_world.guilds) {
                    std::cout << guild.base_filename
                              << " guard=" << guild.guard_mob
                              << " bank=" << guild.banker_mob << '\n';
                }
                return true;
            }
            if (sub == "show" && argc >= 4) {
                const std::string name = argv[3];
                for (const auto& guild : g_world.guilds) {
                    if (guild.base_filename != name) {
                        continue;
                    }
                    std::cout << guild.base_filename << '\n';
                    std::cout << "Guard: mob=" << guild.guard_mob
                              << " room=" << guild.guard_room
                              << " dir=" << guild.guard_dir << '\n';
                    std::cout << "Bank: mob=" << guild.banker_mob
                              << " room=" << guild.bank_room << '\n';
                    std::cout << "XP bank: mob=" << guild.banker_xp_mob
                              << " room=" << guild.bank_xp_room << '\n';
                    std::cout << "Member book: " << guild.member_book_obj << '\n';
                    return true;
                }
                std::cerr << "Guild not loaded: " << name << '\n';
                return false;
            }
        }

        if (cmd == "check") {
            if (argc < 4) {
                usage();
                return false;
            }
            const std::string kind = argv[2];
            if (kind == "mob") {
                try {
                    nebbie::World world;
                    nebbie::load_myst_mob(world, argv[3]);
                    std::cout << "OK: " << world.mobiles.size() << " mobiles in " << argv[3] << '\n';
                    return true;
                } catch (const std::exception& ex) {
                    std::cerr << "FAILED: " << ex.what() << '\n';
                    return false;
                }
            }
            if (kind == "obj") {
                try {
                    nebbie::World world;
                    nebbie::load_myst_obj(world, argv[3]);
                    std::cout << "OK: " << world.objects.size() << " objects in " << argv[3] << '\n';
                    return true;
                } catch (const std::exception& ex) {
                    std::cerr << "FAILED: " << ex.what() << '\n';
                    return false;
                }
            }
            if (kind == "wld") {
                try {
                    nebbie::World world;
                    nebbie::load_myst_wld(world, argv[3]);
                    std::cout << "OK: " << world.rooms.size() << " rooms in " << argv[3] << '\n';
                    if (const nebbie::Room* room0 = world.find_room(0)) {
                        std::cout << "  room #0: " << room0->name << '\n';
                    }
                    return true;
                } catch (const std::exception& ex) {
                    std::cerr << "FAILED: " << ex.what() << '\n';
                    return false;
                }
            }
            if (kind == "lib") {
                try {
                    nebbie::World world;
                    nebbie::LibContext context;
                    nebbie::load_lib(world, argv[3], context, [](const std::string& msg) {
                        std::cout << msg << '\n';
                    });
                std::cout << "OK: lib loaded from " << argv[3] << '\n';
                std::cout << "  zones=" << world.zones.size()
                          << " rooms=" << world.rooms.size()
                          << " mobiles=" << world.mobiles.size()
                          << " objects=" << world.objects.size() << '\n';
                constexpr const char* kFiles[] = {
                    "myst.zon", "myst.wld", "myst.mob", "myst.obj", "myst.shp", "myst.spe",
                    "myst.dam", "myst.act", "myst.pos", "myst.gui",
                };
                const std::filesystem::path root = argv[3];
                for (const char* file : kFiles) {
                    std::cout << "  " << file << ": "
                              << (std::filesystem::exists(root / file) ? "yes" : "no") << '\n';
                }
                return true;
                } catch (const std::exception& ex) {
                    std::cerr << "FAILED: " << ex.what() << '\n';
                    return false;
                }
            }
            usage();
            return false;
        }

        if (cmd == "repair-wld") {
            if (argc < 3) {
                usage();
                return false;
            }
            const std::filesystem::path lib = argv[2];
            const std::filesystem::path wld_path = lib / "myst.wld";
            if (!std::filesystem::exists(wld_path)) {
                std::cerr << "myst.wld not found in " << lib << '\n';
                return false;
            }

            nebbie::World world;
            nebbie::LibContext context;
            nebbie::load_lib(world, lib, context, [](const std::string& msg) {
                std::cout << msg << '\n';
            });

            const std::filesystem::path backup = lib / "myst.wld.bak";
            std::filesystem::copy_file(wld_path, backup, std::filesystem::copy_options::overwrite_existing);
            nebbie::save_myst_wld(world, wld_path, [](const std::string& msg) {
                std::cout << msg << '\n';
            });
            std::cout << "Repaired myst.wld for server compatibility (" << world.rooms.size()
                      << " rooms). Backup: " << backup << '\n';
            return true;
        }

        if (cmd == "validate") {
            if (argc < 3) {
                usage();
                return false;
            }
            nebbie::World world;
            nebbie::load_lib(world, argv[2], [](const std::string& msg) {
                std::cout << msg << '\n';
            });
            const nebbie::ValidationReport report = nebbie::validate_world(world);
            for (const auto& issue : report.issues) {
                const char* level = issue.severity == nebbie::ValidationSeverity::error
                                        ? "ERROR"
                                        : "WARN";
                std::cout << level << " [" << issue.category << "] " << issue.message << '\n';
            }
            std::cout << report.error_count() << " error(s), "
                      << report.warning_count() << " warning(s)\n";
            return report.ok();
        }

        if (cmd == "edit") {
            if (argc < 3) {
                usage();
                return false;
            }
            return nebbiedit::run_shell(argv[2]) == 0;
        }

        if (cmd == "room" && argc >= 5 && std::string(argv[2]) == "set") {
            const std::vector<std::string> args(argv + 1, argv + argc);
            const auto flags = nebbiedit::parse_flags(args, 4);
            const bool force = flags.count("force") > 0;
            const long vnum = std::stol(argv[4]);
            return nebbiedit::run_room_set(argv[3], vnum, flags, force);
        }

        if (cmd == "mob" && argc >= 5 && std::string(argv[2]) == "set") {
            const std::vector<std::string> args(argv + 1, argv + argc);
            const auto flags = nebbiedit::parse_flags(args, 4);
            const bool force = flags.count("force") > 0;
            const long vnum = std::stol(argv[4]);
            return nebbiedit::run_mob_set(argv[3], vnum, flags, force);
        }

        if (cmd == "obj" && argc >= 5 && std::string(argv[2]) == "set") {
            const std::vector<std::string> args(argv + 1, argv + argc);
            const auto flags = nebbiedit::parse_flags(args, 4);
            const bool force = flags.count("force") > 0;
            const long vnum = std::stol(argv[4]);
            return nebbiedit::run_obj_set(argv[3], vnum, flags, force);
        }

        if (cmd == "convert" && argc >= 5 && std::string(argv[2]) == "zon") {
            const std::string mode = argv[3];
            if (mode == "roundtrip") {
                nebbie::World original;
                nebbie::load_lib(original, argv[4]);
                const std::filesystem::path out = argv[5];
                std::filesystem::create_directories(out);
                nebbie::save_myst_zon(original, out / "myst.zon");
                nebbie::save_myst_wld(original, out / "myst.wld");

                nebbie::World roundtrip;
                nebbie::load_lib(roundtrip, out);
                if (roundtrip.zones.size() != original.zones.size()) {
                    throw std::runtime_error("zone count mismatch after roundtrip");
                }
                if (roundtrip.rooms.size() != original.rooms.size()) {
                    throw std::runtime_error("room count mismatch after roundtrip");
                }
                std::cout << "Round-trip OK in " << out << '\n';
                return true;
            }
        }

        if (cmd == "overlay" && argc >= 4 && std::string(argv[2]) == "export") {
            nebbie::World world;
            nebbie::load_lib(world, argv[3], [](const std::string& msg) {
                std::cout << msg << '\n';
            });

            nebbie::OverlayExportKind kind = nebbie::OverlayExportKind::all;
            for (int i = 4; i < argc; ++i) {
                const std::string flag = argv[i];
                if (flag == "--rooms") {
                    kind = nebbie::OverlayExportKind::rooms;
                } else if (flag == "--objects") {
                    kind = nebbie::OverlayExportKind::objects;
                } else if (flag == "--mobiles") {
                    kind = nebbie::OverlayExportKind::mobiles;
                } else if (flag == "--zones") {
                    kind = nebbie::OverlayExportKind::zone_resets;
                } else {
                    usage();
                    return false;
                }
            }

            const auto report = nebbie::export_myst_to_overlays(world, argv[3], kind,
                                                                [](const std::string& msg) {
                                                                    std::cout << msg << '\n';
                                                                });
            std::cout << "Exported overlays: rooms=" << report.rooms
                      << " objects=" << report.objects
                      << " mobiles=" << report.mobiles
                      << " zone_resets=" << report.zone_resets << '\n';
            for (const auto& warning : report.warnings) {
                std::cout << "WARN: " << warning << '\n';
            }
            return true;
        }

        if (cmd == "convert" && argc >= 5 && std::string(argv[2]) == "lib") {
            const std::string mode = argv[3];
            if (mode == "roundtrip") {
                nebbie::World original;
                nebbie::LibContext context;
                nebbie::load_lib(original, argv[4], context);
                const std::filesystem::path out = argv[5];
                std::filesystem::create_directories(out);
                nebbie::LibContext out_ctx = context;
                out_ctx.root = out;
                nebbie::save_lib(original, out_ctx);

                nebbie::World roundtrip;
                nebbie::load_lib(roundtrip, out);
                if (roundtrip.zones.size() != original.zones.size()
                    || roundtrip.rooms.size() != original.rooms.size()
                    || roundtrip.mobiles.size() != original.mobiles.size()
                    || roundtrip.objects.size() != original.objects.size()
                    || roundtrip.shops.size() != original.shops.size()
                    || roundtrip.special_procs.size() != original.special_procs.size()
                    || roundtrip.damage_messages.size() != original.damage_messages.size()
                    || roundtrip.social_messages.size() != original.social_messages.size()
                    || roundtrip.pose_entries.size() != original.pose_entries.size()
                    || roundtrip.guilds.size() != original.guilds.size()) {
                    throw std::runtime_error("count mismatch after lib roundtrip");
                }
                std::cout << "Lib round-trip OK in " << out << '\n';
                return true;
            }
        }

        usage();
        return false;
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << '\n';
        return false;
    }
}

} // namespace

#if defined(_WIN32)
#include <windows.h>
#include <vector>

namespace {

std::vector<std::string> utf8_argv_storage;
std::vector<char*> utf8_argv_ptrs;

void prepare_utf8_argv(const int argc, wchar_t** wargv) {
    utf8_argv_storage.clear();
    utf8_argv_ptrs.clear();
    utf8_argv_storage.reserve(static_cast<std::size_t>(argc));
    utf8_argv_ptrs.reserve(static_cast<std::size_t>(argc));

    for (int i = 0; i < argc; ++i) {
        const wchar_t* warg = wargv[i];
        if (warg == nullptr) {
            utf8_argv_storage.emplace_back();
            continue;
        }
        const int size = WideCharToMultiByte(CP_UTF8, 0, warg, -1, nullptr, 0, nullptr, nullptr);
        std::string utf8(size > 0 ? static_cast<std::size_t>(size - 1) : 0, '\0');
        if (size > 0) {
            WideCharToMultiByte(CP_UTF8, 0, warg, -1, utf8.data(), size, nullptr, nullptr);
        }
        utf8_argv_storage.push_back(std::move(utf8));
    }

    for (auto& arg : utf8_argv_storage) {
        utf8_argv_ptrs.push_back(arg.data());
    }
}

} // namespace

int wmain(int argc, wchar_t** wargv) {
    prepare_utf8_argv(argc, wargv);
    return run(argc, utf8_argv_ptrs.data()) ? 0 : 1;
}
#else
int main(int argc, char** argv) {
    return run(argc, argv) ? 0 : 1;
}
#endif
