#include "nebbie/io.hpp"
#include "nebbie/world.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void write_file(const std::filesystem::path& path, const std::string& content) {
    std::ofstream out(path, std::ios::binary);
    out << content;
}

void expect(const bool cond, const std::string& message) {
    if (!cond) {
        throw std::runtime_error(message);
    }
}

} // namespace

int main() {
    try {
        const auto dir = std::filesystem::temp_directory_path() / "nebbie-mob-load-skip";
        std::filesystem::create_directories(dir);
        const auto mob_path = dir / "broken.mob";

        // Mob 100 OK, 101 has double tilde in description (historical Felix bug), 102 OK.
        write_file(mob_path,
                   "#100\n"
                   "ok~\n"
                   "ok one~\n"
                   "Ok one is here.\n"
                   "~\n"
                   "Desc one.\n"
                   "~\n"
                   "4 0 0 S\n"
                   "1 20 10 1d8+0 1d4+0\n"
                   "-1 10 0 0\n"
                   "8 8 0\n"
                   "#101\n"
                   "bad~\n"
                   "bad mob~\n"
                   "Bad is here.\n"
                   "~\n"
                   "Broken desc ends here.~\n"
                   "~\n"
                   "4 0 0 S\n"
                   "1 20 10 1d8+0 1d4+0\n"
                   "-1 10 0 0\n"
                   "8 8 0\n"
                   "#102\n"
                   "ok2~\n"
                   "ok two~\n"
                   "Ok two is here.\n"
                   "~\n"
                   "Desc two.\n"
                   "~\n"
                   "4 0 0 S\n"
                   "1 20 10 1d8+0 1d4+0\n"
                   "-1 20 0 1\n"
                   "8 8 1\n");

        nebbie::World world;
        std::vector<std::string> warnings;
        nebbie::load_myst_mob(world, mob_path, {}, true, &warnings);

        expect(world.find_mobile(100) != nullptr, "mob 100 should load");
        expect(world.find_mobile(101) != nullptr,
               "mob 101 should load after recovering extra empty '~'");
        expect(world.find_mobile(102) != nullptr, "mob 102 should load");
        expect(!warnings.empty(), "expected warning about extra tilde");
        expect(warnings.front().find("101") != std::string::npos
                   || warnings.front().find("extra empty") != std::string::npos,
               "warning should mention extra tilde / mob 101: " + warnings.front());

        // Broken combat line with valid ~ fields: skip 200, keep 201.
        write_file(mob_path,
                   "#200\n"
                   "broken~\n"
                   "broken short~\n"
                   "Broken is here.\n"
                   "~\n"
                   "Desc.\n"
                   "~\n"
                   "4 0 0 S\n"
                   "THIS IS NOT A COMBAT LINE\n"
                   "-1 5 0 0\n"
                   "8 8 0\n"
                   "#201\n"
                   "fine~\n"
                   "fine mob~\n"
                   "Fine is here.\n"
                   "~\n"
                   "Desc.\n"
                   "~\n"
                   "4 0 0 S\n"
                   "1 20 10 1d8+0 1d4+0\n"
                   "-1 5 0 0\n"
                   "8 8 0\n");

        nebbie::World world2;
        std::vector<std::string> warnings2;
        nebbie::load_myst_mob(world2, mob_path, {}, true, &warnings2);
        expect(world2.find_mobile(200) == nullptr, "broken mob 200 must be skipped");
        expect(world2.find_mobile(201) != nullptr, "mob 201 must still load");
        expect(!warnings2.empty(), "expected skip warning for mob 200");
        bool saw_skip = false;
        for (const auto& w : warnings2) {
            if (w.find("200") != std::string::npos && w.find("skipped") != std::string::npos) {
                saw_skip = true;
            }
        }
        expect(saw_skip, "warning should say mob 200 was skipped");

        std::cout << "OK\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "FAIL: " << ex.what() << '\n';
        return 1;
    }
}
