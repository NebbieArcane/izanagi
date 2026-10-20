#include "nebbie/special_proc_catalog.hpp"
#include "nebbie/edit.hpp"
#include "nebbie/io.hpp"
#include "nebbie/types.hpp"
#include "nebbie/validate.hpp"
#include "nebbie/world.hpp"

#include <filesystem>
#include <fstream>
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
        // Reset catalog to embedded-only for deterministic tests.
        nebbie::configure_special_proc_catalog({});

        if (!nebbie::is_known_special_proc('m', "puff")) {
            throw std::runtime_error("expected puff in mob/object catalog");
        }
        if (!nebbie::is_known_special_proc('r', "Fountain")) {
            throw std::runtime_error("expected Fountain in room catalog");
        }
        if (nebbie::is_known_special_proc('m', "NotARealProcEver")) {
            throw std::runtime_error("unexpected catalog match for fake proc");
        }

        const std::string sample = "{\n\t{ \"Incastonatore\",(genericspecial_func) Incastonatore },\n"
                                   "\t{ \"Fountain\", Fountain },\n};\n";
        const auto parsed = nebbie::parse_special_proc_names_from_cpp(sample);
        if (parsed.size() != 2) {
            throw std::runtime_error("parse_special_proc_names_from_cpp size mismatch");
        }

        const auto tmp = std::filesystem::temp_directory_path() / "nebbie-special-proc-catalog-test";
        std::filesystem::create_directories(tmp);
        const auto allow = tmp / "extra_special_procs.txt";
        {
            std::ofstream out(allow);
            out << "# test\nLocalOnlyProc\n";
        }

        nebbie::SpecialProcCatalogConfig config;
        config.extra_allowlist_path = allow;
        // Point at real Server if present so dynamic path is exercised.
        const auto server = nebbie::default_nebbie_server_root();
        std::error_code ec;
        if (!server.empty()
            && std::filesystem::is_regular_file(server / "src" / "specialproc_other.cpp", ec)) {
            config.server_root = server;
        }
        nebbie::configure_special_proc_catalog(config);

        if (!nebbie::is_known_special_proc('m', "LocalOnlyProc")) {
            throw std::runtime_error("allowlist name should be known after configure");
        }
        if (!config.server_root.empty() && !nebbie::is_known_special_proc('m', "Incastonatore")) {
            throw std::runtime_error("Incastonatore should come from Server catalog");
        }

        nebbie::set_extra_special_proc_names({"AnotherExtra"});
        if (!nebbie::is_known_special_proc('r', "AnotherExtra")) {
            throw std::runtime_error("set_extra_special_proc_names should update effective catalog");
        }
        if (!nebbie::save_extra_special_procs(allow)) {
            throw std::runtime_error("save_extra_special_procs failed");
        }

        nebbie::World add_world;
        nebbie::SpecialProc new_proc;
        new_proc.type = 'm';
        new_proc.vnum = 3016;
        new_proc.procedure = "Incastonatore";
        std::string add_error;
        if (!nebbie::add_special_proc(add_world, new_proc, &add_error)) {
            throw std::runtime_error("add_special_proc failed: " + add_error);
        }
        if (add_world.special_procs.size() != 1
            || add_world.special_procs.front().procedure != "Incastonatore") {
            throw std::runtime_error("add_special_proc did not append entry");
        }
        if (!nebbie::remove_special_proc(add_world, 0) || !add_world.special_procs.empty()) {
            throw std::runtime_error("remove_special_proc failed");
        }

        const auto spe_out = std::filesystem::path("build/spe-add-roundtrip");
        std::filesystem::create_directories(spe_out);
        nebbie::SpecialProc roundtrip_proc;
        roundtrip_proc.type = 'm';
        roundtrip_proc.vnum = 3016;
        roundtrip_proc.procedure = "Incastonatore";
        if (!nebbie::add_special_proc(add_world, roundtrip_proc, &add_error)) {
            throw std::runtime_error("roundtrip add_special_proc failed: " + add_error);
        }
        nebbie::save_myst_spe(add_world, spe_out / "myst.spe");
        nebbie::World loaded;
        nebbie::load_myst_spe(loaded, spe_out / "myst.spe");
        if (loaded.special_procs.size() != 1
            || loaded.special_procs.front().vnum != 3016
            || loaded.special_procs.front().procedure != "Incastonatore") {
            throw std::runtime_error("myst.spe roundtrip failed for appended special proc");
        }

        nebbie::configure_special_proc_catalog({});

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

        nebbie::set_extra_special_proc_names({"TotallyFakeProc"});
        const nebbie::ValidationReport allowlisted = nebbie::validate_world(world);
        if (has_issue(allowlisted,
                      nebbie::ValidationSeverity::warning,
                      "unknown procedure name")) {
            throw std::runtime_error("allowlist should silence unknown procedure warning");
        }

        std::cout << "OK\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "FAILED: " << ex.what() << '\n';
        return 1;
    }
}
