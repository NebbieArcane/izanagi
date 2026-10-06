#include "nebbie/io.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void expect(bool ok, const char* msg) {
    if (!ok) {
        throw std::runtime_error(msg);
    }
}

std::string read_file(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

std::string extract_block(const std::string& content, long vnum) {
    const std::string marker = "#" + std::to_string(vnum) + "\n";
    const auto start = content.find(marker);
    if (start == std::string::npos) {
        throw std::runtime_error("block missing");
    }
    const auto next = content.find("\n#", start + marker.size());
    return content.substr(start, next == std::string::npos ? std::string::npos : next - start);
}

} // namespace

int main() {
    try {
        std::filesystem::path myst_dir;
        for (const char* candidate : {"tests/fixtures/aree/myst", "../tests/fixtures/aree/myst",
                                      "../../tests/fixtures/aree/myst"}) {
            if (std::filesystem::is_directory(candidate)) {
                myst_dir = candidate;
                break;
            }
        }
        if (myst_dir.empty()) {
            throw std::runtime_error("myst fixture dir missing");
        }

        nebbie::World world;
        nebbie::load_myst_zon(world, myst_dir / "myst.zon");
        nebbie::load_myst_wld(world, myst_dir / "myst.wld");
        nebbie::load_myst_mob(world, myst_dir / "myst.mob", {}, false);

        const auto out = std::filesystem::temp_directory_path() / "nebbie-tilde-roundtrip";
        std::filesystem::create_directories(out);
        nebbie::save_myst_wld(world, out / "myst.wld");
        nebbie::save_myst_mob(world, out / "myst.mob");

        const std::string wld = read_file(out / "myst.wld");
        const std::string mob = read_file(out / "myst.mob");
        const std::string room3006 = extract_block(wld, 3006);
        const std::string mob3015 = extract_block(mob, 3015);

        expect(room3006.find("visibili.\n~") != std::string::npos,
               "wld room description must end with newline then ~ on its own line");
        expect(room3006.find("visibili.~") == std::string::npos,
               "wld room description must not use inline tilde after last paragraph line");

        expect(mob3015.find("inventario.\n~") != std::string::npos,
               "mob long_descr must use tilde on its own line");
        expect(mob3015.find("di lui.\n~") != std::string::npos,
               "mob description must use tilde on its own line");
        expect(mob3015.find("$c0007\n~") != std::string::npos || mob3015.find("$c0007 \n~") != std::string::npos,
               "mob L sound must use tilde on its own line");

        nebbie::World reload;
        nebbie::load_myst_wld(reload, out / "myst.wld");
        nebbie::load_myst_mob(reload, out / "myst.mob", {}, false);
        const nebbie::Room* room = reload.find_room(3006);
        const nebbie::Mobile* leoven = reload.find_mobile(3015);
        expect(room != nullptr && leoven != nullptr, "round-trip entities missing");
        expect(!room->description.empty() && !leoven->sounds.empty(), "round-trip text missing");

        std::cout << "OK\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "FAILED: " << ex.what() << '\n';
        return 1;
    }
}
