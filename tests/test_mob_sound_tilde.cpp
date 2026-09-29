#include "nebbie/io.hpp"
#include "nebbie/mob_trailing_sound.hpp"

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

std::filesystem::path fixture_mob() {
    for (const char* candidate : {"tests/fixtures/aree/myst/myst.mob", "../tests/fixtures/aree/myst/myst.mob",
                                  "../../tests/fixtures/aree/myst/myst.mob"}) {
        if (std::filesystem::is_regular_file(candidate)) {
            return candidate;
        }
    }
    throw std::runtime_error("myst.mob fixture not found");
}

std::string extract_mob_block(const std::string& content, long vnum) {
    const std::string marker = "#" + std::to_string(vnum) + "\n";
    const auto start = content.find(marker);
    if (start == std::string::npos) {
        throw std::runtime_error("mob block missing");
    }
    const auto next = content.find("\n#", start + marker.size());
    return content.substr(start, next == std::string::npos ? std::string::npos : next - start);
}

} // namespace

int main() {
    try {
        const auto source = fixture_mob();
        nebbie::World world;
        nebbie::load_myst_mob(world, source, {}, false);
        const nebbie::Mobile* leoven = world.find_mobile(3015);
        expect(leoven != nullptr, "mob 3015 missing");

        const auto out = std::filesystem::temp_directory_path() / "nebbie-mob-sound-tilde-test";
        std::filesystem::create_directories(out);
        nebbie::save_myst_mob(world, out / "myst.mob");

        std::ifstream in(out / "myst.mob", std::ios::binary);
        std::string saved((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        const std::string block = extract_mob_block(saved, 3015);

        expect(block.find("Arcanscape$c0013] ti dice") != std::string::npos, "expected Leoven speech text");
        expect(block.find("$c0007\n~") != std::string::npos || block.find("$c0007 \n~") != std::string::npos,
               "speech sound must use newline before ~ terminator");
        expect(block.find("testa...\n~") != std::string::npos,
               "distant sound must use newline before ~ terminator");
        expect(block.find("$c0007~") == std::string::npos, "must not place ~ on same line as speech sound");
        expect(block.find("testa...~") == std::string::npos, "must not place ~ on same line as distant sound");

        nebbie::World roundtrip;
        nebbie::load_myst_mob(roundtrip, out / "myst.mob", {}, false);
        const nebbie::Mobile* reloaded = roundtrip.find_mobile(3015);
        expect(reloaded != nullptr, "reload mob 3015");
        expect(reloaded->sounds == leoven->sounds, "speech sound text must round-trip");
        expect(reloaded->distant_sounds == leoven->distant_sounds, "distant sound text must round-trip");

        std::cout << "OK\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "FAILED: " << ex.what() << '\n';
        return 1;
    }
}
