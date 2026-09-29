#include "nebbie/monolith_audit.hpp"

#include "nebbie/constants.hpp"
#include "nebbie/io.hpp"

#include <fstream>
#include <regex>
#include <sstream>

namespace nebbie {

namespace {

std::vector<std::string> read_lines(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("unable to open " + path.string());
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        lines.push_back(line);
    }
    return lines;
}

void write_lines(const std::filesystem::path& path, const std::vector<std::string>& lines) {
    std::ofstream output(path, std::ios::binary);
    if (!output) {
        throw std::runtime_error("unable to write " + path.string());
    }

    for (std::size_t i = 0; i < lines.size(); ++i) {
        output << lines[i];
        if (i + 1 < lines.size()) {
            output << '\n';
        }
    }
}

bool is_terminator_line(const std::string& stripped, const std::vector<std::string>& markers) {
    for (const std::string& marker : markers) {
        if (stripped == marker) {
            return true;
        }
    }
    return false;
}

long last_vnum_before_line(const std::vector<std::string>& lines, std::size_t index) {
    static const std::regex k_vnum_line(R"(^#(\d+))");
    long last_vnum = 0;
    for (std::size_t i = 0; i < index; ++i) {
        std::smatch match;
        if (std::regex_match(lines[i], match, k_vnum_line)) {
            last_vnum = std::stol(match[1].str());
        }
    }
    return last_vnum;
}

std::vector<std::size_t> terminator_line_indexes(const std::vector<std::string>& lines,
                                                 const std::vector<std::string>& markers) {
    std::vector<std::size_t> indexes;
    for (std::size_t i = 0; i < lines.size(); ++i) {
        if (is_terminator_line(lines[i], markers)) {
            indexes.push_back(i);
        }
    }
    return indexes;
}

} // namespace

std::vector<std::string> monolith_terminator_markers(const std::filesystem::path& path) {
    const std::string filename = path.filename().string();
    if (filename == ZONE_FILE) {
        return {"#$"};
    }
    if (filename == MOB_FILE || filename == OBJ_FILE) {
        return {"%%", "%%~"};
    }
    return {};
}

std::vector<MonolithTerminatorIssue> find_premature_monolith_terminators(
    const std::filesystem::path& path) {
    const auto markers = monolith_terminator_markers(path);
    if (markers.empty() || !std::filesystem::is_regular_file(path)) {
        return {};
    }

    const std::vector<std::string> lines = read_lines(path);
    const std::vector<std::size_t> indexes = terminator_line_indexes(lines, markers);
    if (indexes.size() <= 1) {
        return {};
    }

    std::vector<MonolithTerminatorIssue> issues;
    for (std::size_t i = 0; i + 1 < indexes.size(); ++i) {
        const std::size_t line_index = indexes[i];
        MonolithTerminatorIssue issue;
        issue.path = path;
        issue.line = static_cast<int>(line_index + 1);
        issue.marker = lines[line_index];
        issue.last_vnum = last_vnum_before_line(lines, line_index);
        issues.push_back(std::move(issue));
    }
    return issues;
}

int repair_premature_monolith_terminators(const std::filesystem::path& path) {
    const auto markers = monolith_terminator_markers(path);
    if (markers.empty() || !std::filesystem::is_regular_file(path)) {
        return 0;
    }

    const std::vector<std::string> lines = read_lines(path);
    const std::vector<std::size_t> indexes = terminator_line_indexes(lines, markers);
    if (indexes.size() <= 1) {
        return 0;
    }

    std::vector<std::string> repaired;
    repaired.reserve(lines.size() - indexes.size() + 1);
    const std::size_t keep_index = indexes.back();
    int removed = 0;

    for (std::size_t i = 0; i < lines.size(); ++i) {
        if (i < keep_index && is_terminator_line(lines[i], markers)) {
            ++removed;
            continue;
        }
        repaired.push_back(lines[i]);
    }

    if (removed > 0) {
        const std::filesystem::path backup = path.string() + ".bak";
        std::error_code ec;
        if (!std::filesystem::exists(backup, ec)) {
            std::filesystem::copy_file(path, backup, std::filesystem::copy_options::overwrite_existing, ec);
        }
    }

    write_lines(path, repaired);
    return removed;
}

void append_monolith_validation(ValidationReport& report, const std::filesystem::path& lib_dir) {
    const std::filesystem::path root = resolve_lib_directory(lib_dir);
    const std::filesystem::path files[] = {
        root / ZONE_FILE,
        root / MOB_FILE,
        root / OBJ_FILE,
    };

    for (const auto& path : files) {
        if (!std::filesystem::is_regular_file(path)) {
            continue;
        }
        for (const auto& issue : find_premature_monolith_terminators(path)) {
            std::ostringstream message;
            message << path.filename().string() << " line " << issue.line << ": premature '"
                    << issue.marker << "' stops NebbieArcane server loading";
            if (issue.last_vnum > 0) {
                message << " (after vnum #" << issue.last_vnum << ")";
            }
            message << " — run: nebbiedit repair-lib " << root.string();

            ValidationTarget target = ValidationTarget::none;
            if (path.filename() == MOB_FILE) {
                target = ValidationTarget::mob;
            } else if (path.filename() == OBJ_FILE) {
                target = ValidationTarget::object;
            } else if (path.filename() == ZONE_FILE) {
                target = ValidationTarget::zone;
            }

            ValidationIssue entry;
            entry.severity = ValidationSeverity::error;
            entry.category = "monolith";
            entry.message = message.str();
            entry.target = target;
            entry.target_vnum = issue.last_vnum;
            report.issues.push_back(std::move(entry));
        }
    }
}

LibRepairReport repair_lib_for_server(const std::filesystem::path& lib_dir, ProgressCallback progress) {
    LibRepairReport report;
    const std::filesystem::path root = resolve_lib_directory(lib_dir);

    const std::filesystem::path monoliths[] = {
        root / ZONE_FILE,
        root / MOB_FILE,
        root / OBJ_FILE,
    };

    for (const auto& path : monoliths) {
        if (!std::filesystem::is_regular_file(path)) {
            continue;
        }
        const int removed = repair_premature_monolith_terminators(path);
        if (removed > 0 && progress) {
            progress("Removed " + std::to_string(removed) + " premature terminator line(s) from "
                     + path.filename().string());
        }
        report.terminators_removed += removed;
    }

    const std::filesystem::path wld_path = root / WORLD_FILE;
    if (!std::filesystem::is_regular_file(wld_path)) {
        return report;
    }

    World world;
    LibContext context;
    load_lib(world, root, context, progress);
    save_myst_wld(world, wld_path, progress);
    report.wld_rewritten = true;
    if (progress) {
        progress("Rewrote myst.wld without standalone TUNNEL moblim lines");
    }
    return report;
}

} // namespace nebbie
