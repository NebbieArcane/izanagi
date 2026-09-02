#pragma once

#include "io.hpp"
#include "validate.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace nebbie {

struct MonolithTerminatorIssue {
    std::filesystem::path path;
    int line = 0;
    std::string marker;
    long last_vnum = 0;
};

struct LibRepairReport {
    int terminators_removed = 0;
    bool wld_rewritten = false;
};

std::vector<std::string> monolith_terminator_markers(const std::filesystem::path& path);

std::vector<MonolithTerminatorIssue> find_premature_monolith_terminators(
    const std::filesystem::path& path);

int repair_premature_monolith_terminators(const std::filesystem::path& path);

void append_monolith_validation(ValidationReport& report, const std::filesystem::path& lib_dir);

LibRepairReport repair_lib_for_server(const std::filesystem::path& lib_dir,
                                      ProgressCallback progress = {});

} // namespace nebbie
