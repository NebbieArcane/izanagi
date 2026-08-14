#include "nebbie/constants.hpp"
#include "nebbie/io.hpp"
#include "nebbie/zone_partition.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {

void copy_fixture_lib(const std::filesystem::path& fixture_root, const std::filesystem::path& out) {
    std::filesystem::create_directories(out);
    for (const auto& entry : std::filesystem::directory_iterator(fixture_root)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        std::filesystem::copy_file(entry.path(), out / entry.path().filename(),
                                   std::filesystem::copy_options::overwrite_existing);
    }
}

} // namespace

int main(int argc, char** argv) {
    try {
        if (argc < 2) {
            std::cerr << "Usage: nebbie-zone-partition-tests <lib-directory>\n";
            return 1;
        }

        const auto fixture_root = std::filesystem::path(argv[1]);
        const auto work = std::filesystem::temp_directory_path() / "nebbie-zone-partition-test";
        if (std::filesystem::exists(work)) {
            std::filesystem::remove_all(work);
        }
        copy_fixture_lib(fixture_root, work);

        nebbie::World world;
        nebbie::load_lib(world, work);
        if (world.zones.empty()) {
            throw std::runtime_error("expected at least one zone");
        }

        const int zone_num = world.zones.front().num;
        const auto entities = nebbie::collect_zone_entities(world, zone_num);
        if (entities.room_vnums.count(3001) == 0) {
            throw std::runtime_error("expected room 3001 in zone entities");
        }
        if (entities.mob_vnums.count(1) == 0) {
            throw std::runtime_error("expected mob 1 from zone reset");
        }
        if (entities.object_vnums.count(1) == 0) {
            throw std::runtime_error("expected object 1 from zone reset");
        }

        const std::string pack_name = nebbie::zone_pack_directory_name(zone_num, world.zones.front().name);
        if (pack_name.find("zone-0001") == std::string::npos) {
            throw std::runtime_error("unexpected zone pack directory name: " + pack_name);
        }

        const auto packs_root = work / "zone-packs";
        const auto report = nebbie::export_zone_pack(world, packs_root, zone_num);
        if (report.zones_written != 1 || report.rooms < 1 || report.mobiles < 1 || report.objects < 1) {
            throw std::runtime_error("export_zone_pack produced empty pack");
        }

        const auto zone_dir = packs_root / pack_name;
        if (!std::filesystem::exists(zone_dir / nebbie::ZONE_FILE)
            || !std::filesystem::exists(zone_dir / nebbie::WORLD_FILE)
            || !std::filesystem::exists(zone_dir / nebbie::MOB_FILE)
            || !std::filesystem::exists(zone_dir / nebbie::OBJ_FILE)) {
            throw std::runtime_error("zone pack missing monolith files");
        }
        if (!std::filesystem::exists(zone_dir / nebbie::OVERLAY_ROOMS_DIR / "3001")
            || !std::filesystem::exists(zone_dir / nebbie::OVERLAY_ZONES_DIR / "1.zon")) {
            throw std::runtime_error("zone pack missing overlay directories");
        }
        if (!std::filesystem::exists(zone_dir / "zone-manifest.txt")) {
            throw std::runtime_error("zone pack missing manifest");
        }

        const auto all_root = work / "all-zone-packs";
        const auto all_report = nebbie::export_all_zone_packs(world, all_root);
        if (all_report.zones_written != static_cast<int>(world.zones.size())) {
            throw std::runtime_error("export_all_zone_packs count mismatch");
        }
        if (!std::filesystem::exists(all_root / "zones-index.txt")) {
            throw std::runtime_error("missing zones-index.txt");
        }

        const auto merged_root = work / "merged-lib";
        const auto merge_report = nebbie::merge_zone_packs(all_root, merged_root);
        if (merge_report.zones_written != static_cast<int>(world.zones.size())) {
            throw std::runtime_error("merge_zone_packs zone count mismatch");
        }

        nebbie::World merged;
        nebbie::load_lib(merged, merged_root);
        if (merged.zones.size() != world.zones.size()) {
            throw std::runtime_error("merged lib zone count mismatch");
        }
        for (const long vnum : entities.room_vnums) {
            if (merged.find_room(vnum) == nullptr) {
                throw std::runtime_error("merged lib missing room " + std::to_string(vnum));
            }
        }
        for (const long vnum : entities.mob_vnums) {
            if (merged.find_mobile(vnum) == nullptr) {
                throw std::runtime_error("merged lib missing mobile " + std::to_string(vnum));
            }
        }
        for (const long vnum : entities.object_vnums) {
            if (merged.find_object(vnum) == nullptr) {
                throw std::runtime_error("merged lib missing object " + std::to_string(vnum));
            }
        }

        std::cout << "OK\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "FAILED: " << ex.what() << '\n';
        return 1;
    }
}
