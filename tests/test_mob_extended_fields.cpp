#include "nebbie/io.hpp"
#include "nebbie/world.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void write_file(const std::filesystem::path& path, const std::string& content) {
    std::ofstream out(path, std::ios::binary);
    out << content;
}

std::string read_file(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

void expect(const bool cond, const std::string& message) {
    if (!cond) {
        throw std::runtime_error(message);
    }
}

} // namespace

int main() {
    try {
        const auto dir = std::filesystem::temp_directory_path() / "nebbie-mob-extended-fields";
        std::filesystem::create_directories(dir);
        const auto mob_path = dir / "test.mob";

        // 1) Short -1 gold line (missing exp/race) must load without eating pos/sex.
        // 2) Extended sex with immunities on the following line (Felix / magman style).
        write_file(mob_path,
                   "#100\n"
                   "bum~\n"
                   "A old bum~\n"
                   "A bum is here.\n"
                   "~\n"
                   "Desc.\n"
                   "~\n"
                   "4 0 0 S\n"
                   "7 14 7 7d8+3 1d6+2\n"
                   "-1 100\n"
                   "8 8 1\n"
                   "#101\n"
                   "magman~\n"
                   "the magman~\n"
                   "A magman is here.\n"
                   "~\n"
                   "Hot.\n"
                   "~\n"
                   "134 128 0 S\n"
                   "4 17 3 4d8+4 2d8+4\n"
                   "-1 500 100 26\n"
                   "8 8 3\n"
                   "0 1 4\n"
                   "#102\n"
                   "ok~\n"
                   "ok mob~\n"
                   "Ok is here.\n"
                   "~\n"
                   "Ok.\n"
                   "~\n"
                   "4 0 0 S\n"
                   "1 20 10 1d8+0 1d4+0\n"
                   "-1 0 0 0\n"
                   "8 8 0\n");

        nebbie::World world;
        nebbie::load_myst_mob(world, mob_path);

        const nebbie::Mobile* bum = world.find_mobile(100);
        expect(bum != nullptr, "mob 100 missing");
        expect(bum->extended_gold, "mob 100 extended_gold");
        expect(bum->gold == 100, "mob 100 gold");
        expect(bum->exp == 0, "mob 100 exp default 0");
        expect(bum->race == 0, "mob 100 race default 0");
        expect(bum->position == 8 && bum->default_pos == 8 && bum->sex == 1, "mob 100 pos/sex");

        const nebbie::Mobile* mag = world.find_mobile(101);
        expect(mag != nullptr, "mob 101 missing");
        expect(mag->extended_sex, "mob 101 extended_sex");
        expect(mag->sex == 0, "mob 101 sex (3-3)");
        expect(mag->immune == 0, "mob 101 immune");
        expect(mag->meta_immune == 1, "mob 101 meta_immune");
        expect(mag->susceptible == 4, "mob 101 susceptible");

        expect(world.find_mobile(102) != nullptr, "mob 102 must load after magman");

        const auto out_path = dir / "roundtrip.mob";
        nebbie::MystSaveOptions opts;
        opts.write_eof_markers = false;
        nebbie::save_myst_mob(world, out_path, {}, opts);
        const std::string saved = read_file(out_path);

        expect(saved.find("-1 100 0 0\n") != std::string::npos,
               "short gold line must expand to -1 gold exp race on save:\n" + saved);
        expect(saved.find("-1 100\n") == std::string::npos,
               "must not rewrite server-broken short -1 gold line:\n" + saved);
        expect(saved.find("8 8 3 0 1 4\n") != std::string::npos
                   || saved.find("8 8 3 0|1|4\n") != std::string::npos
                   || (saved.find("8 8 3 ") != std::string::npos && saved.find("0 1 4") != std::string::npos),
               "extended sex immunities must be written:\n" + saved);

        // Reload saved file: round-trip must stay valid.
        nebbie::World again;
        nebbie::load_myst_mob(again, out_path);
        expect(again.find_mobile(100) && again.find_mobile(101) && again.find_mobile(102),
               "round-trip reload");

        std::cout << "OK\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "FAIL: " << ex.what() << '\n';
        return 1;
    }
}
