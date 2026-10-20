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

        // Single-line room description must still use OnOwnLine so zone/D records follow ~ safely.
        {
            nebbie::World single;
            nebbie::Zone zone;
            zone.num = 99;
            zone.bottom = 9900;
            zone.top = 9999;
            single.zones.push_back(zone);
            nebbie::Room room;
            room.vnum = 9901;
            room.name = "Test";
            room.description = "Una sola riga senza invio interno.";
            room.sector_type = 0;
            room.zone_index = 0;
            nebbie::Exit exit;
            exit.direction = 0;
            exit.to_room = 9902;
            exit.description = "Nord.";
            room.exits.push_back(exit);
            single.rooms.emplace(room.vnum, room);

            const auto single_out = std::filesystem::temp_directory_path() / "nebbie-tilde-single-line-room";
            std::filesystem::create_directories(single_out);
            nebbie::save_myst_wld(single, single_out / "myst.wld");
            const std::string single_wld = read_file(single_out / "myst.wld");
            const std::string block = extract_block(single_wld, 9901);
            expect(block.find("senza invio interno.\n~") != std::string::npos,
                   "single-line room description must use newline then ~");
            expect(block.find("senza invio interno.~") == std::string::npos,
                   "single-line room description must not use inline ~");
            expect(block.find("\n~\n") != std::string::npos && block.find("\nD0\n") != std::string::npos,
                   "zone/exit records must follow description ~ on later lines");
            expect(block.find("Nord.\n~") != std::string::npos,
                   "exit look description must use newline then ~ on disk");
            expect(block.find("Nord.~") == std::string::npos,
                   "exit look description must not use inline ~");

            nebbie::World reloaded;
            nebbie::load_myst_wld(reloaded, single_out / "myst.wld");
            const nebbie::Room* loaded = reloaded.find_room(9901);
            expect(loaded != nullptr && !loaded->exits.empty(), "reloaded exit room missing");
            expect(loaded->exits.front().description == "Nord.",
                   "exit look description must strip trailing newline after OnOwnLine load");
            expect(loaded->exits.front().description.find('\n') == std::string::npos,
                   "exit look description must not keep file tilde newline in memory");
        }

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

        // Real Aree deploy_aree.php (no strip): concat order is _head, areas by zone top, _tail.
        // _head.wld owns room #0 (real room, not EOF). _tail.wld is empty — no trailing #0.
        // Area saves must omit #0; a mid-file "#0\\n#next" would be misread as room 0.
        {
            const auto area_out = std::filesystem::temp_directory_path() / "nebbie-tilde-aree-concat";
            std::filesystem::create_directories(area_out);

            // Minimal _head.wld shaped like NebbieArcane/Aree/src/_head/_head.wld
            {
                std::ofstream head(area_out / "head.wld");
                head << "#0\n"
                     << "La stanza dei segreti~\n"
                     << "VERSION LINE\n"
                     << "\n"
                     << "~\n"
                     << "0 0 1\n"
                     << "D4\n"
                     << "Vedi il Tempio.\n"
                     << "~\n"
                     << "~\n"
                     << "0 0 98 -1\n"
                     << "S\n";
            }

            nebbie::World area;
            nebbie::Zone zone;
            zone.num = 1;
            zone.bottom = 1000;
            zone.top = 1099;
            area.zones.push_back(zone);
            nebbie::Room r1;
            r1.vnum = 1000;
            r1.name = "Prima";
            r1.description = "Descrizione con tilde a capo.";
            r1.sector_type = 0;
            r1.zone_index = 0;
            area.rooms.emplace(r1.vnum, r1);
            nebbie::Room r2;
            r2.vnum = 1001;
            r2.name = "Seconda";
            r2.description = "Altra stanza.";
            r2.sector_type = 0;
            r2.zone_index = 0;
            area.rooms.emplace(r2.vnum, r2);

            nebbie::MystSaveOptions no_eof;
            no_eof.write_eof_markers = false;
            nebbie::save_myst_wld(area, area_out / "area.wld", {}, no_eof);
            const std::string area_wld = read_file(area_out / "area.wld");
            expect(area_wld.find("#0") == std::string::npos,
                   "Aree area .wld must omit #0 (room 0 lives only in _head; mid #0 breaks concat)");
            expect(area_wld.find("a capo.\n~\n") != std::string::npos, "Aree room desc must use OnOwnLine ~");
            expect(area_wld.find("\nS\n#1001\n") != std::string::npos,
                   "next room #vnum must follow S, not be swallowed by description");

            // Mimic deploy_aree.php .wld branch: rtrim(chunk)+"\\n" for head, area, empty _tail.
            auto rtrim_copy = [](std::string value) {
                while (!value.empty()
                       && (value.back() == '\n' || value.back() == '\r' || value.back() == ' '
                           || value.back() == '\t')) {
                    value.pop_back();
                }
                return value;
            };
            const std::string head_wld = read_file(area_out / "head.wld");
            const std::string tail_wld; // real _tail.wld is empty
            const std::string mono =
                rtrim_copy(head_wld) + "\n" + rtrim_copy(area_wld) + "\n" + rtrim_copy(tail_wld) + "\n";
            {
                std::ofstream out(area_out / "myst.wld", std::ios::binary);
                out << mono;
            }

            nebbie::World mono_world;
            nebbie::load_myst_wld(mono_world, area_out / "myst.wld");
            expect(mono_world.find_room(0) != nullptr, "concat must keep _head room #0");
            expect(mono_world.find_room(1000) != nullptr && mono_world.find_room(1001) != nullptr,
                   "concat must load area rooms after _head");
            expect(mono_world.find_room(0)->name == "La stanza dei segreti",
                   "room #0 name must survive OnOwnLine fields around it");
        }

        std::cout << "OK\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "FAILED: " << ex.what() << '\n';
        return 1;
    }
}
