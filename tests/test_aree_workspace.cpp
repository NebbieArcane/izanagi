#include "nebbie/aree_workspace.hpp"
#include "nebbie/io.hpp"

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
            }
            if (area.folder_name == "myst") {
                found_myst = true;
                expect(area.zone_num == 30, "myst zone num");
            }
            expect(nebbie::is_aree_area_directory(area.path), "area dir check");
            expect(!nebbie::is_aree_reserved_folder(area.folder_name), "not reserved");
            expect(!file_contains_eof_marker(area.path / (area.folder_name + ".mob"), "%%"),
                   "fixture mob must not end with %%");
            expect(!file_contains_eof_marker(area.path / (area.folder_name + ".zon"), "#$"),
                   "fixture zon must not contain #$");
        }
        expect(found_castelli && found_myst, "missing expected fixtures");

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

        std::cout << "OK\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "FAILED: " << ex.what() << '\n';
        return 1;
    }
}
