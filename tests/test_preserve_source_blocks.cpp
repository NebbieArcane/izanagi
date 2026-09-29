#include "nebbie/io.hpp"
#include "nebbie/source_blocks.hpp"

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

std::filesystem::path fixture_mob() {
    for (const char* candidate : {"tests/fixtures/aree/myst/myst.mob", "../tests/fixtures/aree/myst/myst.mob",
                                  "../../tests/fixtures/aree/myst/myst.mob"}) {
        if (std::filesystem::is_regular_file(candidate)) {
            return candidate;
        }
    }
    throw std::runtime_error("tests/fixtures/aree/myst/myst.mob not found");
}

std::string read_file(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

} // namespace

int main() {
    try {
        const auto source = fixture_mob();
        const auto work = std::filesystem::temp_directory_path() / "nebbie-preserve-mob-test";
        if (std::filesystem::exists(work)) {
            std::filesystem::remove_all(work);
        }
        std::filesystem::create_directories(work);
        std::filesystem::copy_file(source, work / "myst.mob");

        nebbie::World world;
        nebbie::LibContext context;
        context.write_eof_markers_on_save = false;
        nebbie::load_lib(world, work, context);
        expect(!context.mob_source_blocks_by_file.empty(), "expected captured mob source blocks");

        const std::string before = read_file(work / "myst.mob");
        nebbie::save_lib(world, context);
        const std::string after = read_file(work / "myst.mob");
        expect(before == after, "preserve save with no dirty entities must not change mob file");

        context.dirty_mobile_vnums.insert(3000);
        nebbie::save_lib(world, context);
        const std::string touched = read_file(work / "myst.mob");
        expect(touched.find("2|64|1048576|2097152") != std::string::npos,
               "dirty mob rewrite should use pipe-separated flags");
        expect(touched.find("3145794") == std::string::npos, "dirty mob rewrite must not use summed act mask");

        std::filesystem::path wld_fixture;
        for (const char* candidate : {"tests/fixtures/aree/castelli/castelli.wld",
                                      "../tests/fixtures/aree/castelli/castelli.wld",
                                      "../../tests/fixtures/aree/castelli/castelli.wld"}) {
            if (std::filesystem::is_regular_file(candidate)) {
                wld_fixture = candidate;
                break;
            }
        }
        if (wld_fixture.empty()) {
            throw std::runtime_error("castelli.wld fixture not found");
        }

        const auto wld_work = std::filesystem::temp_directory_path() / "nebbie-preserve-wld-test";
        if (std::filesystem::exists(wld_work)) {
            std::filesystem::remove_all(wld_work);
        }
        std::filesystem::create_directories(wld_work);
        std::filesystem::copy_file(wld_fixture, wld_work / "castelli.wld");

        nebbie::World wld_world;
        nebbie::LibContext wld_context;
        wld_context.write_eof_markers_on_save = false;
        nebbie::load_lib(wld_world, wld_work, wld_context);
        expect(!wld_context.wld_source_blocks_by_file.empty(), "expected captured wld source blocks");

        const std::string wld_before = read_file(wld_work / "castelli.wld");
        nebbie::save_lib(wld_world, wld_context);
        const std::string wld_after = read_file(wld_work / "castelli.wld");
        expect(wld_before == wld_after, "preserve save with no dirty rooms must not change wld file");

        const nebbie::Room* room34116 = wld_world.find_room(34116);
        expect(room34116 != nullptr, "expected room 34116");
        expect(room34116->zone_line_primary.has_value() && *room34116->zone_line_primary == -1,
               "Aree wld zone line primary should stay -1");

        wld_context.dirty_room_vnums.insert(34116);
        nebbie::save_lib(wld_world, wld_context);
        const std::string wld_dirty = read_file(wld_work / "castelli.wld");
        expect(wld_dirty.find("1|32|128|512 0 34115 -1") != std::string::npos,
               "dirty room rewrite must keep pipe-separated exit flags including bit 512");
        expect(wld_dirty.find("\n-1 8|64|32768 0\n") != std::string::npos
                   || wld_dirty.find("-1 8|64|32768 0\n") != std::string::npos,
               "dirty room rewrite must preserve -1 zone line prefix");

        std::filesystem::path castelli_fixture;
        for (const char* candidate : {"tests/fixtures/aree/castelli", "../tests/fixtures/aree/castelli",
                                      "../../tests/fixtures/aree/castelli"}) {
            if (std::filesystem::is_directory(candidate)) {
                castelli_fixture = candidate;
                break;
            }
        }
        if (castelli_fixture.empty()) {
            throw std::runtime_error("castelli fixture not found");
        }
        const auto zon_work = std::filesystem::temp_directory_path() / "nebbie-preserve-zon-test";
        if (std::filesystem::exists(zon_work)) {
            std::filesystem::remove_all(zon_work);
        }
        std::filesystem::create_directories(zon_work);
        for (const auto& entry : std::filesystem::directory_iterator(castelli_fixture)) {
            if (entry.is_regular_file()) {
                std::filesystem::copy_file(entry.path(), zon_work / entry.path().filename(),
                                           std::filesystem::copy_options::overwrite_existing);
            }
        }
        nebbie::World zon_world;
        nebbie::LibContext zon_context;
        zon_context.write_eof_markers_on_save = false;
        nebbie::load_lib(zon_world, zon_work, zon_context);
        const std::string zon_before = read_file(zon_work / "castelli.zon");
        expect(zon_before.find("*!") != std::string::npos, "fixture zon should contain *! annotations");
        nebbie::save_lib(zon_world, zon_context);
        const std::string zon_after = read_file(zon_work / "castelli.zon");
        expect(zon_before == zon_after, "preserve save must keep castelli.zon byte-identical including *! lines");

        std::cout << "OK\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "FAILED: " << ex.what() << '\n';
        return 1;
    }
}
