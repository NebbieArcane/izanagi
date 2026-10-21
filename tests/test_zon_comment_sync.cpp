#include "nebbie/zon_comment_sync.hpp"

#include "nebbie/edit.hpp"
#include "nebbie/io.hpp"
#include "nebbie/types.hpp"
#include "nebbie/world.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

std::string read_file(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

void expect(const bool cond, const std::string& message) {
    if (!cond) {
        throw std::runtime_error(message);
    }
}

nebbie::World make_fixture_world() {
    nebbie::World world;

    nebbie::Mobile mob;
    mob.vnum = 1500;
    mob.short_descr = "Un guardiano dell'ombra";
    world.mobiles.emplace(mob.vnum, mob);

    nebbie::GameObject oven;
    oven.vnum = 1501;
    oven.short_descr = "Un forno";
    world.objects.emplace(oven.vnum, oven);

    nebbie::GameObject pizza;
    pizza.vnum = 1504;
    pizza.short_descr = "Una fetta di pizza";
    world.objects.emplace(pizza.vnum, pizza);

    nebbie::GameObject wrong_named;
    wrong_named.vnum = 1499;
    wrong_named.short_descr = "Fontana della Vita";
    world.objects.emplace(wrong_named.vnum, wrong_named);

    nebbie::Room grove;
    grove.vnum = 1504;
    grove.name = "Il Bosco delle Ombre";
    world.rooms.emplace(grove.vnum, grove);

    nebbie::Room kitchen;
    kitchen.vnum = 1520;
    kitchen.name = "la cucina";
    world.rooms.emplace(kitchen.vnum, kitchen);

    nebbie::Room common;
    common.vnum = 1514;
    common.name = "La sala comune";
    world.rooms.emplace(common.vnum, common);

    nebbie::Zone zone;
    zone.num = 15;
    zone.name = "Test High Tower";
    zone.top = 1699;
    zone.lifespan = 20;
    zone.reset_mode = 2;

    nebbie::ResetCommand init;
    init.command = '*';
    init.raw_line = "! Inizializzazione della zona 15\n";
    zone.commands.push_back(init);

    nebbie::ResetCommand section;
    section.command = '*';
    section.raw_line = "***** Sezione test *****\n";
    zone.commands.push_back(section);

    nebbie::ResetCommand legacy_o;
    legacy_o.command = '*';
    legacy_o.raw_line = "O:An oven R:the kitchen MW:1 MR:0\n";
    zone.commands.push_back(legacy_o);

    nebbie::ResetCommand legacy_door;
    legacy_door.command = '*';
    legacy_door.raw_line = " Door in room the kitchen -> E is closed\n";
    zone.commands.push_back(legacy_door);

    nebbie::ResetCommand blank;
    blank.command = 0;
    blank.raw_line = "\n";
    zone.commands.push_back(blank);

    nebbie::ResetCommand load_mob;
    load_mob.command = 'M';
    load_mob.if_flag = 0;
    load_mob.arg1 = 1500;
    load_mob.arg2 = 8;
    load_mob.arg3 = 1504;
    load_mob.arg4 = 0;
    load_mob.raw_line =
        " 0 1500 8 1504 0 *! Mob 1500 (A shadow guardian) nella stanza 1504 (The Shadow Grove)\n";
    zone.commands.push_back(load_mob);

    nebbie::ResetCommand shadow;
    shadow.command = '*';
    shadow.raw_line = " *M:A shadow guardian R:The Shadow Grove MW:8 MR:0\n";
    zone.commands.push_back(shadow);

    // Wrong vnum in comment (claims 1500 / Fountain) while command loads 1499.
    nebbie::ResetCommand load_obj;
    load_obj.command = 'O';
    load_obj.if_flag = 0;
    load_obj.arg1 = 1499;
    load_obj.arg2 = 1;
    load_obj.arg3 = 1514;
    load_obj.arg4 = 0;
    load_obj.raw_line =
        " 0 1499 1 1514 0  *! Carica oggetto 1500 (Fountain of Life) nella stanza 1514 "
        "(Common Room)\n";
    zone.commands.push_back(load_obj);

    nebbie::ResetCommand put_pizza;
    put_pizza.command = 'P';
    put_pizza.if_flag = 1;
    put_pizza.arg1 = 1504;
    put_pizza.arg2 = 20;
    put_pizza.arg3 = 1501;
    put_pizza.arg4 = 0;
    put_pizza.raw_line =
        " 1 1504 20 1501 0 *! Dentro 001501 (An oven): 001504 (A slice of pizza)\n";
    zone.commands.push_back(put_pizza);

    nebbie::ResetCommand door;
    door.command = 'D';
    door.if_flag = 0;
    door.arg1 = 1520;
    door.arg2 = 1;
    door.arg3 = 1;
    door.arg4 = 0;
    door.raw_line = " 0 1520 1 1 0 *! Uscita east della stanza 1520 (the kitchen): chiusa\n";
    zone.commands.push_back(door);

    world.zones.push_back(zone);
    return world;
}

} // namespace

int main() {
    try {
        nebbie::World world = make_fixture_world();
        nebbie::Zone& zone = world.zones.front();

        expect(nebbie::sync_zone_reset_comments(zone, world), "first sync should change zone");

        bool saw_legacy = false;
        bool saw_shadow = false;
        bool saw_init = false;
        bool saw_section = false;
        for (const auto& cmd : zone.commands) {
            if (cmd.command != '*') {
                continue;
            }
            const std::string body = cmd.raw_line;
            if (body.find("O:An oven") != std::string::npos
                || body.find("Door in room") != std::string::npos) {
                saw_legacy = true;
            }
            if (body.find("*M:A shadow") != std::string::npos
                || body.find("M:A shadow") != std::string::npos) {
                saw_shadow = true;
            }
            if (body.find("Inizializzazione") != std::string::npos) {
                saw_init = true;
            }
            if (body.find("*****") != std::string::npos) {
                saw_section = true;
            }
        }
        expect(!saw_legacy, "legacy *O: / Door comments must be stripped");
        expect(!saw_shadow, "legacy * *M: shadow comments must be stripped");
        expect(saw_init, "initialization comment must be kept");
        expect(saw_section, "section header must be kept");

        std::string found_mob;
        std::string found_obj;
        std::string found_put;
        std::string found_door;
        for (const auto& cmd : zone.commands) {
            if (cmd.command == 'M') {
                found_mob = nebbie::reset_inline_note(cmd);
            } else if (cmd.command == 'O') {
                found_obj = nebbie::reset_inline_note(cmd);
            } else if (cmd.command == 'P') {
                found_put = nebbie::reset_inline_note(cmd);
            } else if (cmd.command == 'D') {
                found_door = nebbie::reset_inline_note(cmd);
            }
        }

        expect(found_mob.find("Un guardiano dell'ombra") != std::string::npos, found_mob);
        expect(found_mob.find("Il Bosco delle Ombre") != std::string::npos, found_mob);
        expect(found_obj.find("Carica oggetto 1499") != std::string::npos, found_obj);
        expect(found_obj.find("Fontana della Vita") != std::string::npos, found_obj);
        expect(found_obj.find("La sala comune") != std::string::npos, found_obj);
        expect(found_obj.find("1500 (Fountain") == std::string::npos, found_obj);
        expect(found_put.find("Un forno") != std::string::npos, found_put);
        expect(found_put.find("Una fetta di pizza") != std::string::npos, found_put);
        expect(found_door.find("la cucina") != std::string::npos, found_door);

        expect(!nebbie::sync_zone_reset_comments(zone, world), "second sync must be idempotent");

        const auto dir = std::filesystem::temp_directory_path() / "nebbie-zon-comment-sync";
        std::filesystem::create_directories(dir);
        const auto out_path = dir / "roundtrip.zon";
        nebbie::MystSaveOptions opts;
        opts.write_eof_markers = false;
        nebbie::save_myst_zon(world, out_path, {}, opts);
        const std::string saved = read_file(out_path);
        expect(saved.find("Un guardiano dell'ombra") != std::string::npos, saved);
        expect(saved.find("O:An oven") == std::string::npos, saved);
        expect(saved.find("A shadow guardian") == std::string::npos, saved);
        expect(saved.find("Carica oggetto 1499 (Fontana della Vita)") != std::string::npos, saved);

        std::cout << "OK\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "FAIL: " << ex.what() << '\n';
        return 1;
    }
}
