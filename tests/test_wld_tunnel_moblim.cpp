#include "nebbie/io.hpp"

#include <cctype>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

void expect(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

std::string extract_room_block(const std::filesystem::path& path, long vnum) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("unable to open " + path.string());
    }

    const std::string marker = "#" + std::to_string(vnum);
    std::string line;
    bool in_room = false;
    std::ostringstream block;

    while (std::getline(in, line)) {
        if (!in_room) {
            if (line == marker) {
                in_room = true;
                block << line << '\n';
            }
            continue;
        }

        block << line << '\n';
        if (line == "S") {
            break;
        }
    }

    if (!in_room) {
        throw std::runtime_error("room #" + std::to_string(vnum) + " not found");
    }

    return block.str();
}

bool block_has_standalone_moblim_line(const std::string& block) {
    std::istringstream in(block);
    std::string line;
    bool saw_zone_line = false;

    while (std::getline(in, line)) {
        if (line == "S") {
            break;
        }
        if (!saw_zone_line) {
            if (!line.empty() && (line[0] == '-' || std::isdigit(static_cast<unsigned char>(line[0])))) {
                saw_zone_line = true;
            }
            continue;
        }
        if (line == "1" || line == "2" || line == "3") {
            return true;
        }
        if (!line.empty() && (line[0] == 'D' || line[0] == 'E' || line[0] == 'L' || line[0] == 'C')) {
            return false;
        }
    }

    return false;
}

} // namespace

int main() {
    try {
        std::filesystem::path source = "vendor/production-lib/myst.wld";
        if (!std::filesystem::exists(source)) {
            source = "../vendor/production-lib/myst.wld";
        }
        if (!std::filesystem::exists(source)) {
            source = "../../vendor/production-lib/myst.wld";
        }
        if (!std::filesystem::exists(source)) {
            std::cout << "SKIP (vendor/production-lib/myst.wld missing)\n";
            return 0;
        }

        std::filesystem::path zon_source = "vendor/production-lib/myst.zon";
        if (!std::filesystem::exists(zon_source)) {
            zon_source = "../vendor/production-lib/myst.zon";
        }
        if (!std::filesystem::exists(zon_source)) {
            zon_source = "../../vendor/production-lib/myst.zon";
        }

        nebbie::World world;
        nebbie::load_myst_zon(world, zon_source);
        nebbie::load_myst_wld(world, source);

        const nebbie::Room* room = world.find_room(3006);
        expect(room != nullptr, "room #3006 missing");
        expect((room->room_flags & 64) != 0, "room #3006 should keep TUNNEL flag");

        const auto out = std::filesystem::temp_directory_path() / "nebbie-wld-tunnel-moblim-test";
        std::filesystem::create_directories(out);
        nebbie::save_myst_wld(world, out / "myst.wld");

        const std::string block = extract_room_block(out / "myst.wld", 3006);
        expect(!block_has_standalone_moblim_line(block),
               "saved room #3006 must not emit standalone moblim line");

        nebbie::World roundtrip;
        nebbie::load_myst_zon(roundtrip, zon_source);
        nebbie::load_myst_wld(roundtrip, out / "myst.wld");
        const nebbie::Room* reloaded = roundtrip.find_room(3006);
        expect(reloaded != nullptr, "room #3006 missing after roundtrip");
        expect(reloaded->moblim >= 1, "moblim defaults to at least 1 on load");

        std::cout << "OK\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "FAILED: " << ex.what() << '\n';
        return 1;
    }
}
