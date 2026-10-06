#include "nebbie/edit.hpp"
#include "nebbie/mob_catalog.hpp"
#include "nebbie/validate.hpp"

#include <iostream>
#include <stdexcept>

namespace {

void expect(const bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

} // namespace

int main() {
    try {
        nebbie::World world;
        expect(nebbie::create_mob(world, 9001), "create_mob failed");
        const nebbie::Mobile* mob = world.find_mobile(9001);
        expect(mob != nullptr, "mob missing");
        expect(mob->mobtype == 'A', "new mob should default to type A");
        expect(mob->ac == -10, "new mob should use typical NPC AC field");
        expect(nebbie::mob_server_hit_estimate(mob->mobtype, mob->level, mob->hit_bonus, mob->hit_dice) >= 1,
               "default mob hit estimate must be positive");

        nebbie::Mobile broken = *mob;
        broken.mobtype = 'S';
        broken.level = 55;
        broken.hit_dice = "55d8+-13463";
        broken.hit_bonus = 0;
        world.mobiles[9001] = broken;
        const nebbie::ValidationReport report = nebbie::validate_world(world);
        expect(!report.ok(), "expected validation error for negative HP mob");
        expect(report.error_count() >= 1, "expected mob_combat error");

        std::cout << "OK\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "FAILED: " << ex.what() << '\n';
        return 1;
    }
}
