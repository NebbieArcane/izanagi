#include "nebbie/io.hpp"
#include "nebbie/obj_catalog.hpp"
#include "nebbie/types.hpp"
#include "nebbie/world.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void expect(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

} // namespace

int main() {
    try {
        expect(nebbie::obj_apply_type_name(nebbie::APPLY_SPELLPOWER) == "SPELLPOWER",
               "APPLY_SPELLPOWER name");
        expect(nebbie::obj_apply_type_name(nebbie::APPLY_HITNSP) == "HIT-N-SP", "APPLY_HITNSP name");

        std::vector<nebbie::ObjAffect> affects;
        affects.push_back({nebbie::APPLY_SPELLPOWER, 3});
        affects.push_back({18, 2}); // HITROLL
        expect(nebbie::sum_obj_affect_modifier(affects, nebbie::APPLY_SPELLPOWER) == 3,
               "sum spellpower from single row");
        expect(nebbie::sum_obj_affect_modifier(affects, nebbie::APPLY_HITNSP) == 0, "no hit-n-sp");

        nebbie::set_obj_affect_modifier(affects, nebbie::APPLY_SPELLPOWER, 5);
        expect(nebbie::sum_obj_affect_modifier(affects, nebbie::APPLY_SPELLPOWER) == 5,
               "updated spellpower");
        expect(affects.size() == 2, "other affects preserved");
        expect(affects[0].location == nebbie::APPLY_SPELLPOWER && affects[0].modifier == 5,
               "first spellpower row updated");
        expect(affects[1].location == 18 && affects[1].modifier == 2, "hitroll untouched");

        nebbie::set_obj_affect_modifier(affects, nebbie::APPLY_SPELLPOWER, 0);
        expect(nebbie::sum_obj_affect_modifier(affects, nebbie::APPLY_SPELLPOWER) == 0,
               "removed spellpower");
        expect(affects.size() == 1, "only hitroll left");
        expect(affects[0].location == 18, "remaining is hitroll");

        nebbie::set_obj_affect_modifier(affects, nebbie::APPLY_HITNSP, 2);
        expect(affects.size() == 2, "hit-n-sp appended");
        expect(nebbie::sum_obj_affect_modifier(affects, nebbie::APPLY_HITNSP) == 2, "hit-n-sp sum");

        affects.push_back({nebbie::APPLY_SPELLPOWER, 1});
        affects.push_back({nebbie::APPLY_SPELLPOWER, 4});
        nebbie::set_obj_affect_modifier(affects, nebbie::APPLY_SPELLPOWER, 7);
        int sp_rows = 0;
        for (const auto& affect : affects) {
            if (affect.location == nebbie::APPLY_SPELLPOWER) {
                ++sp_rows;
                expect(affect.modifier == 7, "deduped spellpower modifier");
            }
        }
        expect(sp_rows == 1, "duplicate spellpower rows collapsed");

        std::vector<nebbie::ObjAffect> bits;
        bits.push_back({nebbie::APPLY_IMMUNE, 1});  // FIRE
        bits.push_back({nebbie::APPLY_IMMUNE, 2});  // COLD
        bits.push_back({nebbie::APPLY_SUSC, 4});    // ELEC
        bits.push_back({18, 3});                    // HITROLL numeric
        nebbie::normalize_obj_affects_for_mud_save(bits);
        expect(bits.size() == 3, "immune rows OR-merged, others kept");
        expect(bits[0].location == nebbie::APPLY_IMMUNE && bits[0].modifier == 3,
               "immune FIRE|COLD as single decimal 3");
        expect(bits[1].location == nebbie::APPLY_SUSC && bits[1].modifier == 4, "susc kept");
        expect(bits[2].location == 18 && bits[2].modifier == 3, "hitroll kept");

        nebbie::World world;
        nebbie::GameObject obj;
        obj.vnum = 99;
        obj.name = "test~";
        obj.short_descr = "test~";
        obj.description = "test~";
        obj.action_description = "~";
        obj.type_flag = 5;
        obj.extra_flags = 1 | 2 | 4;
        obj.wear_flags = 1 | 8192;
        obj.affects = {{nebbie::APPLY_IMMUNE, 1}, {nebbie::APPLY_IMMUNE, 16}};
        world.objects.emplace(obj.vnum, obj);

        const auto dir = std::filesystem::temp_directory_path() / "nebbie-obj-affect-save";
        std::filesystem::create_directories(dir);
        const auto path = dir / "myst.obj";
        nebbie::MystSaveOptions options;
        options.write_eof_markers = true;
        nebbie::save_myst_obj(world, path, {}, options);
        std::ifstream in(path);
        const std::string block((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        expect(block.find("5 1|2|4 1|8192\n") != std::string::npos,
               "wear/extra flags written as pipe-separated powers of two");
        expect(block.find("5 7 8193\n") == std::string::npos,
               "must not collapse flag masks to summed decimals");
        expect(block.find("A\n26 17\n") != std::string::npos, "immune bits OR-merged in A line");

        std::cout << "OK\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "FAILED: " << ex.what() << '\n';
        return 1;
    }
}
