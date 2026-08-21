#include "nebbie/io.hpp"

#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace {

void expect(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
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

        nebbie::World world;
        nebbie::load_myst_wld(world, source);
        const nebbie::Room* room5 = world.find_room(5);
        expect(room5 != nullptr, "room #5 missing");
        expect(room5->tele_time == 60, "room #5 tele_time");
        expect(room5->tele_targ == 9, "room #5 tele_targ");
        expect(room5->tele_mask == 1, "room #5 tele_mask");

        const auto out = std::filesystem::temp_directory_path() / "nebbie-wld-teleport-test";
        std::filesystem::create_directories(out);
        nebbie::save_myst_wld(world, out / "myst.wld");

        nebbie::World roundtrip;
        nebbie::load_myst_wld(roundtrip, out / "myst.wld");
        const nebbie::Room* reloaded = roundtrip.find_room(5);
        expect(reloaded != nullptr, "room #5 missing after roundtrip");
        expect(reloaded->tele_time == room5->tele_time, "tele_time roundtrip");
        expect(reloaded->tele_targ == room5->tele_targ, "tele_targ roundtrip");
        expect(reloaded->tele_mask == room5->tele_mask, "tele_mask roundtrip");
        expect(reloaded->sector_type == room5->sector_type, "sector roundtrip");

        std::cout << "OK\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "FAILED: " << ex.what() << '\n';
        return 1;
    }
}
