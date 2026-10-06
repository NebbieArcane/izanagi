#include "nebbie/edit.hpp"
#include "nebbie/io.hpp"
#include "nebbie/source_blocks.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <algorithm>

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

std::vector<long> hash_vnums_in_file(const std::string& content) {
    std::vector<long> vnums;
    for (std::size_t i = 0; i < content.size(); ++i) {
        if (content[i] != '#') {
            continue;
        }
        if (i != 0 && content[i - 1] != '\n' && content[i - 1] != '\r') {
            continue;
        }
        std::size_t j = i + 1;
        if (j >= content.size() || content[j] < '0' || content[j] > '9') {
            continue;
        }
        long vnum = 0;
        while (j < content.size() && content[j] >= '0' && content[j] <= '9') {
            vnum = vnum * 10 + (content[j] - '0');
            ++j;
        }
        if (vnum > 0) {
            vnums.push_back(vnum);
        }
    }
    return vnums;
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

        expect(nebbie::create_mob(world, 3016), "create mob #3016 failed");
        context.dirty_mobile_vnums.insert(3016);
        nebbie::save_lib(world, context);
        const std::vector<long> mob_order = hash_vnums_in_file(read_file(work / "myst.mob"));
        expect(mob_order.size() >= 2, "expected mob hash entries after create");
        expect(std::is_sorted(mob_order.begin(), mob_order.end()),
               "mob file vnums must be in ascending order");
        const auto it_3016 = std::find(mob_order.begin(), mob_order.end(), 3016);
        const auto it_3015 = std::find(mob_order.begin(), mob_order.end(), 3015);
        expect(it_3016 != mob_order.end() && it_3015 != mob_order.end(),
               "expected #3015 and #3016 in saved mob file");
        expect(it_3015 + 1 == it_3016, "new mob #3016 must follow #3015, not append at EOF");

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

        const auto myst_wld_work = std::filesystem::temp_directory_path() / "nebbie-preserve-myst-wld-test";
        if (std::filesystem::exists(myst_wld_work)) {
            std::filesystem::remove_all(myst_wld_work);
        }
        std::filesystem::create_directories(myst_wld_work);
        std::filesystem::copy_file(fixture_mob().parent_path() / "myst.wld", myst_wld_work / "myst.wld");

        nebbie::World myst_wld_world;
        nebbie::LibContext myst_wld_context;
        myst_wld_context.write_eof_markers_on_save = false;
        nebbie::load_lib(myst_wld_world, myst_wld_work, myst_wld_context);
        nebbie::Room* room3016 = myst_wld_world.find_room(3016);
        expect(room3016 != nullptr, "expected room 3016");
        expect(room3016->zone_data_line_raw.has_value(), "expected zone line raw capture");
        expect(room3016->zone_data_line_raw->find("8388608") != std::string::npos,
               "expected scalar room flags in source zone line");
        room3016->description += " ";
        myst_wld_context.dirty_room_vnums.insert(3016);
        nebbie::save_lib(myst_wld_world, myst_wld_context);
        const std::string myst_wld_dirty = read_file(myst_wld_work / "myst.wld");
        const std::string room3016_block = [&]() {
            const std::string marker = "#3016\n";
            const auto start = myst_wld_dirty.find(marker);
            expect(start != std::string::npos, "room 3016 block missing after save");
            const auto next = myst_wld_dirty.find("\n#", start + marker.size());
            return myst_wld_dirty.substr(start, next == std::string::npos ? std::string::npos : next - start);
        }();
        expect(room3016_block.find("-1 8388608 1") != std::string::npos,
               "dirty room with text-only edit must keep scalar zone line from file");
        expect(room3016_block.find('|') == std::string::npos,
               "room 3016 block must not expand flags into pipe vectors on text-only edit");

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
