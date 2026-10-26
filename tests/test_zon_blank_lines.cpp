#include "nebbie/io.hpp"
#include "nebbie/edit.hpp"
#include "nebbie/world.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

std::string read_file(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

void write_file(const std::filesystem::path& path, const std::string& content) {
    std::ofstream out(path, std::ios::binary);
    out << content;
}

} // namespace

int main() {
    try {
        const auto dir = std::filesystem::temp_directory_path() / "nebbie-zon-blank-lines";
        std::filesystem::create_directories(dir);
        const auto zon_path = dir / "test.zon";

        const std::string original =
            "#15\n"
            "Test Zone~\n"
            "1699 20 2\n"
            "*! header note\n"
            "\n"
            "O 0 1499 1 1514 0  *! Carica oggetto 1499 (Fountain of Life) nella stanza 1514 "
            "(Common Room)\n"
            "\n"
            "M 0 1500 8 1504 0\n"
            "\n"
            "S\n";

        write_file(zon_path, original);

        nebbie::World world;
        nebbie::load_myst_zon(world, zon_path);
        if (world.zones.size() != 1) {
            throw std::runtime_error("expected one zone");
        }

        nebbie::GameObject obj;
        obj.vnum = 1500;
        obj.short_descr = "Fontana della Vita";
        world.objects.emplace(obj.vnum, obj);
        nebbie::Room room;
        room.vnum = 1514;
        room.name = "Common Room";
        world.rooms.emplace(room.vnum, room);
        nebbie::Mobile mob;
        mob.vnum = 1500;
        mob.short_descr = "A shadow guardian";
        world.mobiles.emplace(mob.vnum, mob);
        nebbie::Room mob_room;
        mob_room.vnum = 1504;
        mob_room.name = "The Shadow Grove";
        world.rooms.emplace(mob_room.vnum, mob_room);

        const auto& zone = world.zones.front();

        int blanks = 0;
        for (const auto& cmd : zone.commands) {
            if (cmd.command == 0 && (cmd.raw_line.empty() || cmd.raw_line == "\n")) {
                ++blanks;
            }
        }
        if (blanks != 3) {
            throw std::runtime_error("expected 3 preserved blank lines, got "
                                     + std::to_string(blanks));
        }

        // Editing a real reset must keep blanks and rewrite the trailing *! note.
        for (std::size_t i = 0; i < zone.commands.size(); ++i) {
            if (zone.commands[i].command == 'O') {
                nebbie::ResetCommand updated = zone.commands[i];
                updated.arg1 = 1500;
                const std::string note =
                    "Carica oggetto 1500 (Fountain of Life) nella stanza 1514 (Common Room)";
                if (nebbie::reset_inline_note(updated).find("1499") == std::string::npos) {
                    throw std::runtime_error("expected loaded inline *! note");
                }
                nebbie::set_reset_inline_note(updated, note);
                if (!nebbie::update_zone_reset(world, zone.num, i, updated)) {
                    throw std::runtime_error("update_zone_reset failed");
                }
                break;
            }
        }

        const auto out_path = dir / "roundtrip.zon";
        nebbie::MystSaveOptions opts;
        opts.write_eof_markers = false;
        nebbie::save_myst_zon(world, out_path, {}, opts);
        const std::string saved = read_file(out_path);

        if (saved.find("O 0 1499 1 1514 0") != std::string::npos) {
            throw std::runtime_error("object vnum should have been updated to 1500");
        }
        const std::string expected_o =
            "O 0 1500 1 1514 0  *! Carica oggetto 1500 (Fontana della Vita) nella stanza 1514 "
            "(Common Room) MW:1 MR:0\n";
        if (saved.find(expected_o) == std::string::npos) {
            throw std::runtime_error("updated O line with *! note missing:\n" + saved);
        }

        // Blank lines around the O / M blocks must remain; M gets a synced *! on save.
        if (saved.find("*! header note\n\n") == std::string::npos
            || saved.find(expected_o + "\n") == std::string::npos
            || saved.find("M 0 1500 8 1504 0") == std::string::npos
            || saved.find("\nS\n") == std::string::npos) {
            throw std::runtime_error("blank lines not preserved in round-trip:\n" + saved);
        }

        if (nebbie::is_editable_reset_command(0)) {
            throw std::runtime_error("blank/raw entries must not be editable");
        }
        if (!nebbie::is_editable_reset_command('*')) {
            throw std::runtime_error("comment lines should be editable");
        }

        std::cout << "OK\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "FAILED: " << ex.what() << '\n';
        return 1;
    }
}
