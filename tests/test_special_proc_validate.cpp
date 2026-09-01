#include "nebbie/special_proc_catalog.hpp"
#include "nebbie/types.hpp"
#include "nebbie/validate.hpp"
#include "nebbie/world.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

namespace {

bool has_issue(const nebbie::ValidationReport& report,
               nebbie::ValidationSeverity severity,
               const std::string& needle) {
    for (const auto& issue : report.issues) {
        if (issue.severity == severity && issue.message.find(needle) != std::string::npos) {
            return true;
        }
    }
    return false;
}

} // namespace

int main() {
    try {
        if (!nebbie::is_known_special_proc('m', "puff")) {
            throw std::runtime_error("expected puff in mob/object catalog");
        }
        if (!nebbie::is_known_special_proc('r', "Fountain")) {
            throw std::runtime_error("expected Fountain in room catalog");
        }
        if (nebbie::is_known_special_proc('m', "NotARealProcEver")) {
            throw std::runtime_error("unexpected catalog match for fake proc");
        }

        nebbie::World world;
        nebbie::Mobile mob;
        mob.vnum = 42;
        mob.short_descr = "Test guard";
        mob.act = 0;
        world.mobiles.emplace(mob.vnum, mob);

        nebbie::SpecialProc linked_proc;
        linked_proc.type = 'm';
        linked_proc.vnum = 42;
        linked_proc.procedure = "puff";
        world.special_procs.push_back(linked_proc);

        const nebbie::ValidationReport missing_flag_report = nebbie::validate_world(world);
        if (!has_issue(missing_flag_report,
                       nebbie::ValidationSeverity::warning,
                       "ACT_SPEC flag is not set")) {
            throw std::runtime_error("expected warning when proc exists but SPEC flag is off");
        }

        world.mobiles.at(42).act |= nebbie::kMobActSpecFlag;
        const nebbie::ValidationReport ok_report = nebbie::validate_world(world);
        if (has_issue(ok_report,
                      nebbie::ValidationSeverity::warning,
                      "ACT_SPEC flag is not set")) {
            throw std::runtime_error("unexpected SPEC flag warning after enabling flag");
        }

        nebbie::Mobile spec_only;
        spec_only.vnum = 99;
        spec_only.short_descr = "Lonely spec mob";
        spec_only.act = nebbie::kMobActSpecFlag;
        world.mobiles.emplace(spec_only.vnum, spec_only);

        const nebbie::ValidationReport spec_only_report = nebbie::validate_world(world);
        if (!has_issue(spec_only_report,
                       nebbie::ValidationSeverity::warning,
                       "has ACT_SPEC but no M entry")) {
            throw std::runtime_error("expected warning for SPEC mob without myst.spe entry");
        }

        nebbie::SpecialProc unknown_proc;
        unknown_proc.type = 'm';
        unknown_proc.vnum = 42;
        unknown_proc.procedure = "TotallyFakeProc";
        world.special_procs.push_back(unknown_proc);

        const nebbie::ValidationReport unknown_report = nebbie::validate_world(world);
        if (!has_issue(unknown_report,
                       nebbie::ValidationSeverity::warning,
                       "unknown procedure name")) {
            throw std::runtime_error("expected warning for unknown procedure name");
        }

        std::cout << "OK\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "FAILED: " << ex.what() << '\n';
        return 1;
    }
}
