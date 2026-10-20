#include "nebbie/obj_catalog.hpp"
#include "nebbie/types.hpp"

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

        std::cout << "OK\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "FAILED: " << ex.what() << '\n';
        return 1;
    }
}
