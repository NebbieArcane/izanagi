#include "nebbie/monolith_audit.hpp"
#include "nebbie/validate.hpp"

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

void write_file(const std::filesystem::path& path, const std::string& content) {
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        throw std::runtime_error("unable to write " + path.string());
    }
    out << content;
}

} // namespace

int main() {
    try {
        const auto temp = std::filesystem::temp_directory_path() / "nebbie-monolith-audit-test";
        std::filesystem::remove_all(temp);
        std::filesystem::create_directories(temp);

        write_file(temp / "myst.zon",
                   "#1\nzone one~\n1 0 0 0\n#2\nzone two~\n2 0 0 0\n#$\n#3\nzone three~\n3 0 0 0\n#$");
        write_file(temp / "myst.mob",
                   "#100\nmob one~\n1 0 0 0\nS\n%%\n#200\nmob two~\n1 0 0 0\nS\n%%~");
        write_file(temp / "myst.obj",
                   "#300\nobj one~\n1 0 0 0\nE\n%%\n#400\nobj two~\n1 0 0 0\nE\n%%");

        const auto zon_issues = nebbie::find_premature_monolith_terminators(temp / "myst.zon");
        expect(zon_issues.size() == 1, "expected one premature #$ in myst.zon");
        expect(zon_issues.front().line == 7, "premature #$ should be on line 7");
        expect(zon_issues.front().last_vnum == 2, "last vnum before premature #$ should be 2");

        const auto mob_issues = nebbie::find_premature_monolith_terminators(temp / "myst.mob");
        expect(mob_issues.size() == 1, "expected one premature %% in myst.mob");
        expect(mob_issues.front().last_vnum == 100, "last vnum before premature %% should be 100");

        nebbie::ValidationReport report;
        nebbie::append_monolith_validation(report, temp);
        expect(report.error_count() == 3, "expected three monolith validation errors");
        expect(!report.ok(), "report should not be ok with monolith errors");

        expect(nebbie::repair_premature_monolith_terminators(temp / "myst.zon") == 1,
               "repair should remove one premature #$");
        expect(nebbie::find_premature_monolith_terminators(temp / "myst.zon").empty(),
               "myst.zon should have no premature terminators after repair");
        expect(nebbie::repair_premature_monolith_terminators(temp / "myst.mob") == 1,
               "repair should remove one premature %%");
        expect(nebbie::repair_premature_monolith_terminators(temp / "myst.obj") == 1,
               "repair should remove one premature %%");
        expect(nebbie::repair_premature_monolith_terminators(temp / "myst.zon") == 0,
               "second repair should be a no-op");

        std::cout << "OK\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "FAILED: " << ex.what() << '\n';
        return 1;
    }
}
