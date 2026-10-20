#include "nebbie/aree_workspace.hpp"
#include "nebbie/io.hpp"
#include "nebbie/validate.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void expect(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

std::filesystem::path fixture_workspace_root() {
    for (const char* candidate : {"tests/fixtures/aree", "../tests/fixtures/aree",
                                  "../../tests/fixtures/aree"}) {
        if (std::filesystem::is_directory(candidate)) {
            return candidate;
        }
    }
    throw std::runtime_error("tests/fixtures/aree not found");
}

bool file_contains_eof_marker(const std::filesystem::path& path, const std::string& marker) {
    std::ifstream in(path);
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line == marker) {
            return true;
        }
    }
    return false;
}

} // namespace

int main() {
    try {
        const auto source_root = fixture_workspace_root();
        const auto work = std::filesystem::temp_directory_path() / "nebbie-aree-workspace-test";
        if (std::filesystem::exists(work)) {
            std::filesystem::remove_all(work);
        }
        std::filesystem::create_directories(work);
        for (const auto& entry : std::filesystem::directory_iterator(source_root)) {
            if (!entry.is_directory()) {
                continue;
            }
            std::filesystem::copy(entry.path(), work / entry.path().filename(),
                                  std::filesystem::copy_options::recursive);
        }

        const nebbie::AreeWorkspace workspace = nebbie::make_aree_workspace(work);
        const auto areas = nebbie::scan_aree_areas(workspace);
        expect(areas.size() >= 2, "expected castelli and myst fixtures");

        bool found_castelli = false;
        bool found_myst = false;
        for (const auto& area : areas) {
            if (area.folder_name == "castelli") {
                found_castelli = true;
                expect(area.zone_num == 340, "castelli zone num");
                expect(area.top_vnum > area.bottom_vnum, "castelli range");
            }
            if (area.folder_name == "myst") {
                found_myst = true;
                expect(area.zone_num == 30, "myst zone num");
                expect(area.bottom_vnum == 0, "myst is first zone by number in fixtures");
            }
            expect(nebbie::is_aree_area_directory(area.path), "area dir check");
            expect(!nebbie::is_aree_reserved_folder(area.folder_name), "not reserved");
            expect(!file_contains_eof_marker(area.path / (area.folder_name + ".mob"), "%%"),
                   "fixture mob must not end with %%");
            expect(!file_contains_eof_marker(area.path / (area.folder_name + ".zon"), "#$"),
                   "fixture zon must not contain #$");
        }
        expect(found_castelli && found_myst, "missing expected fixtures");

        {
            const auto known = nebbie::collect_aree_workspace_room_vnums(workspace);
            expect(!known.empty(), "workspace room scan should find rooms");
            expect(known.count(34000) > 0, "castelli room #34000 should be known in workspace");
            expect(known.count(3001) > 0, "myst room should be known in workspace");

            nebbie::World only_castelli;
            nebbie::load_lib(only_castelli, work / "castelli");
            // Fake a cross-area exit into myst while only castelli is loaded.
            nebbie::Room* room = only_castelli.find_room(34000);
            expect(room != nullptr, "castelli #34000");
            nebbie::Exit cross;
            cross.direction = 0;
            cross.to_room = 3001;
            room->exits.push_back(cross);

            nebbie::ValidationOptions local_only;
            const auto local_report = nebbie::validate_rooms(only_castelli, local_only);
            bool missing_without_workspace = false;
            for (const auto& issue : local_report.issues) {
                if (issue.message.find("missing room 3001") != std::string::npos) {
                    missing_without_workspace = true;
                    break;
                }
            }
            expect(missing_without_workspace, "without workspace rooms, cross-area exit must error");

            nebbie::ValidationOptions with_workspace;
            with_workspace.known_room_vnums = known;
            const auto workspace_report = nebbie::validate_rooms(only_castelli, with_workspace);
            for (const auto& issue : workspace_report.issues) {
                expect(issue.message.find("missing room 3001") == std::string::npos,
                       "workspace-known destination must not be reported missing");
            }
        }

        expect(nebbie::aree_areas_container(work) == work, "fixture areas at workspace root");
        const nebbie::AreeLibOpenGuard block_root = nebbie::classify_aree_lib_open_guard(work);
        expect(block_root.reason == nebbie::AreeLibOpenBlockReason::workspace_root,
               "workspace root must not open as monolith lib");
        expect(block_root.suggested_workspace_root == work, "suggested workspace root");

        const std::filesystem::path myst_area = work / "myst";
        const nebbie::AreeLibOpenGuard block_area = nebbie::classify_aree_lib_open_guard(myst_area);
        expect(block_area.reason == nebbie::AreeLibOpenBlockReason::area_directory,
               "area folder must not open as monolith lib");
        expect(block_area.suggested_workspace_root == work, "area suggests workspace root");

        const auto archive =
            nebbie::archive_aree_area(workspace, "myst", "test-roundtrip", [](const std::string& msg) {
                std::cout << msg << '\n';
            });
        expect(std::filesystem::is_directory(archive), "archive created");

        nebbie::World loaded;
        nebbie::load_lib(loaded, work / "myst");
        expect(!loaded.rooms.empty(), "myst loads before restore test");

        nebbie::restore_aree_area_from_archive(workspace, "myst", archive);
        nebbie::World restored;
        nebbie::load_lib(restored, work / "myst");
        expect(restored.rooms.size() == loaded.rooms.size(), "restore preserves room count");

        const auto archives = nebbie::list_aree_archives(workspace, "myst");
        expect(!archives.empty(), "archive listed");

        {
            const auto clean = nebbie::find_aree_workspace_duplicate_entity_vnums(workspace);
            expect(clean.empty(), "fixture workspace should have unique mob/obj vnums");

            {
                std::ofstream mob_out(work / "castelli" / "castelli.mob", std::ios::app);
                mob_out << "#3000\n";
            }
            {
                std::ofstream obj_out(work / "castelli" / "castelli.obj", std::ios::app);
                obj_out << "#43\n";
            }

            const auto dups = nebbie::find_aree_workspace_duplicate_entity_vnums(workspace);
            expect(dups.mobiles.size() == 1, "cross-area mob #3000 should be reported");
            expect(dups.mobiles[0].vnum == 3000, "duplicate mob vnum is 3000");
            expect(dups.mobiles[0].area_folders.size() == 2, "mob #3000 in two areas");
            expect(dups.objects.size() == 1, "cross-area obj #43 should be reported");
            expect(dups.objects[0].vnum == 43, "duplicate obj vnum is 43");
            expect(dups.objects[0].area_folders.size() == 2, "obj #43 in two areas");

            {
                std::ofstream mob_out(work / "castelli" / "castelli.mob", std::ios::app);
                mob_out << "#34001\n";
            }
            const auto same_area = nebbie::find_aree_workspace_duplicate_entity_vnums(workspace);
            bool found_same_area_mob = false;
            for (const auto& group : same_area.mobiles) {
                if (group.vnum == 34001) {
                    found_same_area_mob = true;
                    expect(group.area_folders.size() >= 2, "same-area mob duplicate lists area twice");
                    break;
                }
            }
            expect(found_same_area_mob, "same-area mob #34001 duplicate should be reported");
        }

        std::cout << "OK\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "FAILED: " << ex.what() << '\n';
        return 1;
    }
}
