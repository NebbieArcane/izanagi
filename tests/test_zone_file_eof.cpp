#include "nebbie/io.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

std::string strip_file_terminator(std::string content) {
    const std::size_t pos = content.rfind("%%");
    if (pos != std::string::npos) {
        content.erase(pos);
    }
    while (!content.empty() && (content.back() == '\n' || content.back() == '\r' || content.back() == ' ')) {
        content.pop_back();
    }
    content.push_back('\n');
    return content;
}

void write_text(const std::filesystem::path& path, const std::string& content) {
    std::ofstream out(path);
    out << content;
}

} // namespace

int main(int argc, char** argv) {
    try {
        if (argc < 2) {
            std::cerr << "Usage: nebbie-zone-file-eof-tests <fixtures-directory>\n";
            return 1;
        }

        const auto fixtures = std::filesystem::path(argv[1]);
        const auto out = std::filesystem::temp_directory_path() / "nebbie-zone-file-eof-test";
        std::filesystem::remove_all(out);
        std::filesystem::create_directories(out);

        {
            std::ifstream in(fixtures / "myst.mob");
            std::ostringstream buffer;
            buffer << in.rdbuf();
            write_text(out / "castelli.mob", strip_file_terminator(buffer.str()));
        }
        {
            std::ifstream in(fixtures / "myst.obj");
            std::ostringstream buffer;
            buffer << in.rdbuf();
            write_text(out / "castelli.obj", strip_file_terminator(buffer.str()));
        }
        {
            std::ifstream in(fixtures / "myst.zon");
            std::ostringstream buffer;
            buffer << in.rdbuf();
            std::string content = buffer.str();
            const std::size_t pos = content.rfind("#$");
            if (pos != std::string::npos) {
                content.erase(pos);
            }
            write_text(out / "castelli.zon", content);
        }

        nebbie::World world;
        nebbie::LibContext context;
        nebbie::load_lib(world, out, context);

        if (world.mobiles.empty()) {
            throw std::runtime_error("expected mob from file without %% terminator");
        }
        if (world.objects.empty()) {
            throw std::runtime_error("expected object from file without %% terminator");
        }
        if (world.zones.empty()) {
            throw std::runtime_error("expected zone from file without #$ terminator");
        }

        std::cout << "OK\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "FAILED: " << ex.what() << '\n';
        return 1;
    }
}
