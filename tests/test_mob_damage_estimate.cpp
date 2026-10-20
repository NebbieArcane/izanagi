#include "nebbie/mob_damage_estimate.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {

void expect(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

bool near(const double a, const double b) {
    return std::fabs(a - b) < 0.01;
}

bool has_role(const nebbie::MobDamageSimulation& sim, const nebbie::MobDamageRole role) {
    for (const auto& component : sim.components) {
        if (component.role == role) {
            return true;
        }
    }
    return false;
}

} // namespace

int main() {
    try {
        nebbie::DiceValues dam;
        dam.number = 2;
        dam.size = 8;
        dam.plus = 5;

        {
            const auto roles = nebbie::collect_mob_damage_roles_from_act(0);
            expect(roles.size() == 1 && roles[0] == nebbie::MobDamageRole::Fighter,
                   "always physical");
        }
        {
            const auto roles = nebbie::collect_mob_damage_roles_from_act(nebbie::ACT_MAGIC_USER);
            expect(roles.size() == 2, "physical + mage");
            expect(roles[0] == nebbie::MobDamageRole::Fighter, "physical first");
            expect(roles[1] == nebbie::MobDamageRole::Mage, "mage second");
        }
        {
            const auto roles = nebbie::collect_mob_damage_roles_from_act(nebbie::ACT_WARRIOR |
                                                                        nebbie::ACT_MAGIC_USER);
            expect(roles.size() == 2, "warrior+mage still two (physical once)");
        }

        {
            // dice only, no invented STR: 7..21 x3
            const auto est = nebbie::estimate_mob_round_damage(nebbie::MobDamageRole::Fighter, 'A',
                                                              40, 3, dam);
            expect(est.min_damage == 21, "fighter min");
            expect(est.max_damage == 63, "fighter max");
            expect(near(est.avg_damage, 42.0), "fighter avg");
            expect(est.summary == "3x 2d8+5", "fighter summary");
        }

        {
            const auto sim =
                nebbie::simulate_mob_round_damage(nebbie::ACT_WARRIOR | nebbie::ACT_MAGIC_USER, 'A',
                                                  40, 2, dam);
            expect(sim.components.size() == 2, "sim size");
            expect(has_role(sim, nebbie::MobDamageRole::Fighter), "has fisico");
            expect(has_role(sim, nebbie::MobDamageRole::Mage), "has mago");
            expect(sim.components[1].min_damage == 40 && sim.components[1].max_damage == 400,
                   "disintegrate");
        }

        {
            const auto est =
                nebbie::estimate_mob_round_damage(nebbie::MobDamageRole::Cleric, 'A', 40, 1, dam);
            expect(est.min_damage == 100 && est.max_damage == 100, "harm");
        }

        {
            const auto est = nebbie::estimate_mob_round_damage(nebbie::MobDamageRole::Psionic, 'A',
                                                              40, 1, dam);
            expect(est.min_damage == 60 && est.max_damage == 260, "crush");
        }

        std::cout << "OK\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "FAILED: " << ex.what() << '\n';
        return 1;
    }
}
