#include "nebbie/io.hpp"
#include "nebbie/wld_room_lines.hpp"

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

std::string read_file(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

} // namespace

int main() {
    try {
        const auto work = std::filesystem::temp_directory_path() / "nebbie-wld-zone-as-exit";
        if (std::filesystem::exists(work)) {
            std::filesystem::remove_all(work);
        }
        std::filesystem::create_directories(work);

        const std::string corrupt =
            "#3006\n"
            "La soffitta dell'alchimista~\n"
            "desc~\n"
            "-1 4|8|16|64|128|32768|8388608|16777216 0\n"
            "D1\n"
            "L'Officina delle Gemme Clandestine~\n"
            "~\n"
            "-1 25198812 0 -1\n"
            "S\n";

        std::ofstream(work / "myst.wld") << corrupt;

        nebbie::World world;
        nebbie::load_myst_wld(world, work / "myst.wld");
        nebbie::Room* room = world.find_room(3006);
        expect(room != nullptr, "room 3006 missing");
        expect(room->exits.size() == 1, "expected one exit");
        const nebbie::Exit& exit = room->exits.front();
        expect(exit.direction == 1, "expected D1");
        expect(exit.to_room == 0, "misplaced zone line must not become a destination vnum");
        expect(!exit.data_line_raw.has_value(), "zone-as-exit raw line must be cleared on load");
        expect(nebbie::exit_data_line_looks_like_zone_line(*room, "-1 25198812 0 -1", world),
               "scalar zone duplicate should be detected as zone line");

        room->exits.front().to_room = 3015;
        room->exits.front().exit_info = 0;
        room->exits.front().key = 0;
        nebbie::save_myst_wld(world, work / "myst.wld");

        const std::string saved = read_file(work / "myst.wld");
        expect(saved.find("0 0 3015 -1") != std::string::npos, "saved exit must use normal open exit line");
        expect(saved.find("-1 25198812 0 -1") == std::string::npos,
               "must not write misplaced zone line under exit");

        std::cout << "OK\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "FAILED: " << ex.what() << '\n';
        return 1;
    }
}
