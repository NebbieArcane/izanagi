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

        std::cout << "OK\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "FAILED: " << ex.what() << '\n';
        return 1;
    }
}
