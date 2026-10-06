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

        std::cout << "OK\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "FAILED: " << ex.what() << '\n';
        return 1;
    }
}
