#include "nebbie/zone_partition.hpp"

#include "nebbie/constants.hpp"
#include "nebbie/edit.hpp"
#include "nebbie/io.hpp"
#include "nebbie/overlay_io.hpp"

#include <cctype>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace nebbie {

namespace {

const Zone* find_zone_by_num(const World& world, const int zone_num) {
    for (const auto& zone : world.zones) {
        if (zone.num == zone_num) {
            return &zone;
        }
    }
    return nullptr;
}

void add_warning(ZoneEntitySet& entities, const std::string& warning) {
    entities.warnings.push_back(warning);
}

void note_external_room(ZoneEntitySet& entities, const long room_vnum, const int zone_num) {
    if (room_vnum <= 0) {
        return;
    }
    if (entities.room_vnums.count(room_vnum) > 0) {
        return;
    }
    add_warning(entities,
                "zone " + std::to_string(zone_num) + " references external room #" + std::to_string(room_vnum));
}

void collect_reset_entities(const Zone& zone, ZoneEntitySet& entities) {
    for (const auto& cmd : zone.commands) {
        switch (cmd.command) {
        case 'M':
            if (cmd.arg1 > 0) {
                entities.mob_vnums.insert(cmd.arg1);
            }
            if (cmd.arg3 > 0) {
                entities.room_vnums.insert(cmd.arg3);
                note_external_room(entities, cmd.arg3, zone.num);
            }
            break;
        case 'C':
            if (cmd.arg1 > 0) {
                entities.mob_vnums.insert(cmd.arg1);
            }
            break;
        case 'O':
            if (cmd.arg1 > 0) {
                entities.object_vnums.insert(cmd.arg1);
            }
            if (cmd.arg3 > 0) {
                entities.room_vnums.insert(cmd.arg3);
                note_external_room(entities, cmd.arg3, zone.num);
            }
            break;
        case 'G':
        case 'E':
            if (cmd.arg1 > 0) {
                entities.object_vnums.insert(cmd.arg1);
            }
            break;
        case 'P':
            if (cmd.arg1 > 0) {
                entities.object_vnums.insert(cmd.arg1);
            }
            if (cmd.arg3 > 0) {
                entities.object_vnums.insert(cmd.arg3);
            }
            break;
        case 'D':
            if (cmd.arg1 > 0) {
                entities.room_vnums.insert(cmd.arg1);
                note_external_room(entities, cmd.arg1, zone.num);
            }
            break;
        default:
            break;
        }
    }
}

void collect_shop_entities(const World& world, ZoneEntitySet& entities) {
    for (const auto& shop : world.shops) {
        if (entities.room_vnums.count(shop.in_room) == 0) {
            continue;
        }
        if (shop.keeper > 0) {
            entities.mob_vnums.insert(shop.keeper);
        }
        for (int i = 0; i < MAX_SHOP_PROD; ++i) {
            if (shop.producing[i] > 0) {
                entities.object_vnums.insert(shop.producing[i]);
            }
        }
    }
}

World build_zone_world(const World& source, const ZoneEntitySet& entities, const Zone& zone) {
    World subset;
    subset.zones.push_back(zone);

    for (const long vnum : entities.room_vnums) {
        const auto it = source.rooms.find(vnum);
        if (it != source.rooms.end()) {
            subset.rooms.emplace(vnum, it->second);
        }
    }
    for (const long vnum : entities.mob_vnums) {
        const auto it = source.mobiles.find(vnum);
        if (it != source.mobiles.end()) {
            subset.mobiles.emplace(vnum, it->second);
        }
    }
    for (const long vnum : entities.object_vnums) {
        const auto it = source.objects.find(vnum);
        if (it != source.objects.end()) {
            subset.objects.emplace(vnum, it->second);
        }
    }

    return subset;
}

void write_zone_manifest(const std::filesystem::path& zone_dir, const ZoneEntitySet& entities) {
    std::ofstream out(zone_dir / "zone-manifest.txt");
    out << "zone=" << entities.zone_num << '\n';
    out << "name=" << entities.zone_name << '\n';
    out << "bottom=" << entities.bottom << '\n';
    out << "top=" << entities.top << '\n';
    out << "rooms=" << entities.room_vnums.size() << '\n';
    out << "mobiles=" << entities.mob_vnums.size() << '\n';
    out << "objects=" << entities.object_vnums.size() << '\n';
    if (!entities.warnings.empty()) {
        out << "warnings:\n";
        for (const auto& warning : entities.warnings) {
            out << "  " << warning << '\n';
        }
    }
}

bool looks_like_zone_pack(const std::filesystem::path& path) {
    std::error_code ec;
    return std::filesystem::is_directory(path, ec) && directory_has_lib_files(path);
}

} // namespace

ZoneEntitySet collect_zone_entities(const World& world,
                                    const int zone_num,
                                    const ZonePartitionOptions& options) {
    const Zone* zone = find_zone_by_num(world, zone_num);
    if (zone == nullptr) {
        throw std::runtime_error("Zone #" + std::to_string(zone_num) + " not found");
    }

    ZoneEntitySet entities;
    entities.zone_num = zone->num;
    entities.zone_name = zone->name;
    entities.bottom = zone->bottom;
    entities.top = zone->top;

    for (const auto& [vnum, _] : world.rooms) {
        if (vnum >= zone->bottom && vnum <= zone->top) {
            entities.room_vnums.insert(vnum);
        }
    }

    collect_reset_entities(*zone, entities);
    if (options.include_shop_entities) {
        collect_shop_entities(world, entities);
    }

    return entities;
}

std::string zone_pack_directory_name(const int zone_num, const std::string& zone_name) {
    std::ostringstream prefix;
    prefix << "zone-" << std::setfill('0') << std::setw(4) << zone_num;

    std::string slug;
    for (const unsigned char c : zone_name) {
        if (std::isalnum(c) != 0) {
            slug.push_back(static_cast<char>(std::tolower(c)));
        } else if (!slug.empty() && slug.back() != '-') {
            slug.push_back('-');
        }
    }
    while (!slug.empty() && slug.back() == '-') {
        slug.pop_back();
    }

    if (slug.empty()) {
        return prefix.str();
    }
    return prefix.str() + '-' + slug;
}

std::string aree_area_slug(const int zone_num, const std::string& zone_name) {
    std::string slug;
    for (const unsigned char c : zone_name) {
        if (std::isalnum(c) != 0) {
            slug.push_back(static_cast<char>(std::tolower(c)));
        } else if (!slug.empty() && slug.back() != '-') {
            slug.push_back('-');
        }
    }
    while (!slug.empty() && slug.back() == '-') {
        slug.pop_back();
    }
    if (slug.empty()) {
        return "zone" + std::to_string(zone_num);
    }
    return slug;
}

ZonePartitionReport export_zone_pack(const World& world,
                                     const std::filesystem::path& output_root,
                                     const int zone_num,
                                     ZonePartitionOptions options,
                                     ProgressCallback progress) {
    const Zone* zone = find_zone_by_num(world, zone_num);
    if (zone == nullptr) {
        throw std::runtime_error("Zone #" + std::to_string(zone_num) + " not found");
    }

    ZonePartitionReport report;
    const ZoneEntitySet entities = collect_zone_entities(world, zone_num, options);
    report.warnings.insert(report.warnings.end(), entities.warnings.begin(), entities.warnings.end());

    const auto zone_dir = options.aree_layout
                              ? output_root / aree_area_slug(zone->num, zone->name)
                              : output_root / zone_pack_directory_name(zone->num, zone->name);
    std::error_code ec;
    std::filesystem::create_directories(zone_dir, ec);

    World subset = build_zone_world(world, entities, *zone);

    if (options.write_monolith_files) {
        if (progress) {
            progress("Writing zone pack monoliths in " + zone_dir.string());
        }
        const MystSaveOptions save_options{.write_eof_markers = options.write_eof_markers};
        const std::string area_basename =
            options.aree_layout ? aree_area_slug(zone->num, zone->name) : std::string{};
        const auto zon_path = options.aree_layout ? zone_dir / (area_basename + ".zon") : zone_dir / ZONE_FILE;
        const auto wld_path = options.aree_layout ? zone_dir / (area_basename + ".wld") : zone_dir / WORLD_FILE;
        const auto mob_path = options.aree_layout ? zone_dir / (area_basename + ".mob") : zone_dir / MOB_FILE;
        const auto obj_path = options.aree_layout ? zone_dir / (area_basename + ".obj") : zone_dir / OBJ_FILE;

        save_myst_zon(subset, zon_path, progress, save_options);
        save_myst_wld(subset, wld_path, progress, save_options);
        if (!subset.mobiles.empty()) {
            save_myst_mob(subset, mob_path, progress, save_options);
        }
        if (!subset.objects.empty()) {
            save_myst_obj(subset, obj_path, progress, save_options);
        }
    }

    if (options.write_overlay_dirs) {
        const auto overlay_report = export_myst_to_overlays(subset, zone_dir, OverlayExportKind::all, progress);
        report.rooms += overlay_report.rooms;
        report.objects += overlay_report.objects;
        report.mobiles += overlay_report.mobiles;
        report.zone_resets += overlay_report.zone_resets;
        report.warnings.insert(report.warnings.end(),
                               overlay_report.warnings.begin(),
                               overlay_report.warnings.end());
    } else {
        report.rooms = static_cast<int>(subset.rooms.size());
        report.objects = static_cast<int>(subset.objects.size());
        report.mobiles = static_cast<int>(subset.mobiles.size());
        report.zone_resets = 1;
    }

    write_zone_manifest(zone_dir, entities);
    report.zones_written = 1;
    return report;
}

ZonePartitionReport export_all_zone_packs(const World& world,
                                          const std::filesystem::path& output_root,
                                          ZonePartitionOptions options,
                                          ProgressCallback progress) {
    ZonePartitionReport total;
    std::filesystem::create_directories(output_root);

    std::ofstream index(output_root / "zones-index.txt");
    index << "zone_packs=" << world.zones.size() << '\n';

    for (const auto& zone : world.zones) {
        const auto report = export_zone_pack(world, output_root, zone.num, options, progress);
        total.zones_written += report.zones_written;
        total.rooms += report.rooms;
        total.objects += report.objects;
        total.mobiles += report.mobiles;
        total.zone_resets += report.zone_resets;
        total.warnings.insert(total.warnings.end(), report.warnings.begin(), report.warnings.end());
        index << zone.num << '\t' << zone_pack_directory_name(zone.num, zone.name) << '\t' << zone.name << '\n';
    }

    return total;
}

ZonePartitionReport merge_zone_packs(const std::filesystem::path& zones_root,
                                     const std::filesystem::path& merged_lib,
                                     ProgressCallback progress) {
    ZonePartitionReport report;
    World merged;
    std::error_code ec;
    std::filesystem::create_directories(merged_lib, ec);

    if (!std::filesystem::exists(zones_root, ec)) {
        throw std::runtime_error("Zones root does not exist: " + zones_root.string());
    }

    for (const auto& entry : std::filesystem::directory_iterator(zones_root, ec)) {
        if (!entry.is_directory() || !looks_like_zone_pack(entry.path())) {
            continue;
        }
        if (progress) {
            progress("Merging zone pack " + entry.path().string());
        }

        World pack;
        LibContext context;
        load_lib(pack, entry.path(), context, progress);

        const auto overlay_report = export_myst_to_overlays(pack, merged_lib, OverlayExportKind::all, progress);
        report.rooms += overlay_report.rooms;
        report.objects += overlay_report.objects;
        report.mobiles += overlay_report.mobiles;
        report.zone_resets += overlay_report.zone_resets;
        report.warnings.insert(report.warnings.end(),
                               overlay_report.warnings.begin(),
                               overlay_report.warnings.end());

        for (auto& zone : pack.zones) {
            merged.zones.push_back(std::move(zone));
        }
        for (auto& [vnum, room] : pack.rooms) {
            merged.rooms.emplace(vnum, std::move(room));
        }
        for (auto& [vnum, mob] : pack.mobiles) {
            merged.mobiles.emplace(vnum, std::move(mob));
        }
        for (auto& [vnum, obj] : pack.objects) {
            merged.objects.emplace(vnum, std::move(obj));
        }

        ++report.zones_written;
    }

    recompute_zone_bottoms(merged);
    LibContext merged_context;
    merged_context.root = merged_lib;
    merged_context.has_zon = !merged.zones.empty();
    merged_context.has_wld = !merged.rooms.empty();
    merged_context.has_mob = !merged.mobiles.empty();
    merged_context.has_obj = !merged.objects.empty();

    save_lib(merged, merged_context, progress);
    report.zones_written = static_cast<int>(merged.zones.size());
    return report;
}

} // namespace nebbie
