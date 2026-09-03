#include "nebbie/aree_workspace.hpp"

#include "nebbie/constants.hpp"
#include "nebbie/time_compat.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <regex>
#include <sstream>

namespace nebbie {

namespace {

std::string timestamp_id() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
    if (!utc_tm_from_time_t(t, tm)) {
        return "unknown";
    }
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y%m%dT%H%M%SZ");
    return oss.str();
}

std::string sanitize_archive_label(const std::string& label) {
    std::string out;
    for (unsigned char c : label) {
        if (std::isalnum(c) != 0) {
            out.push_back(static_cast<char>(std::tolower(c)));
        } else if (c == '-' || c == '_') {
            out.push_back(static_cast<char>(c));
        } else if (std::isspace(c) != 0) {
            if (!out.empty() && out.back() != '-') {
                out.push_back('-');
            }
        }
    }
    while (!out.empty() && out.back() == '-') {
        out.pop_back();
    }
    return out;
}

void copy_directory_recursive(const std::filesystem::path& from, const std::filesystem::path& to,
                              ProgressCallback progress) {
    std::error_code ec;
    std::filesystem::create_directories(to, ec);
    for (const auto& entry : std::filesystem::recursive_directory_iterator(from, ec)) {
        if (ec) {
            throw std::runtime_error("unable to read " + from.string());
        }
        const auto relative = std::filesystem::relative(entry.path(), from, ec);
        const auto target = to / relative;
        if (entry.is_directory()) {
            std::filesystem::create_directories(target, ec);
            continue;
        }
        if (progress) {
            progress("Archive: " + relative.string());
        }
        std::filesystem::create_directories(target.parent_path(), ec);
        std::filesystem::copy_file(entry.path(), target, std::filesystem::copy_options::overwrite_existing, ec);
        if (ec) {
            throw std::runtime_error("unable to copy " + entry.path().string());
        }
    }
}

void write_archive_meta(const std::filesystem::path& archive_dir, const std::string& area_folder,
                        const std::string& label) {
    std::ofstream meta(archive_dir / "archive.txt");
    meta << "area=" << area_folder << '\n';
    meta << "label=" << label << '\n';
    meta << "created=" << timestamp_id() << '\n';
}

} // namespace

bool is_aree_reserved_folder(const std::string& folder_name) {
    return folder_name.empty() || folder_name.front() == '_' || folder_name == "chiusura";
}

bool is_aree_area_directory(const std::filesystem::path& area_dir) {
    std::error_code ec;
    if (!std::filesystem::is_directory(area_dir, ec)) {
        return false;
    }
    const std::string name = area_dir.filename().string();
    if (is_aree_reserved_folder(name)) {
        return false;
    }
    return std::filesystem::is_regular_file(area_dir / (name + ZONE_EXT), ec);
}

AreeWorkspace make_aree_workspace(const std::filesystem::path& root) {
    AreeWorkspace workspace;
    workspace.root = root;
    workspace.izanagi_dir = root / ".izanagi";
    workspace.archives_dir = workspace.izanagi_dir / "archives";
    return workspace;
}

bool read_aree_zone_header(const std::filesystem::path& area_dir, int& zone_num, std::string& zone_name,
                           int& top_vnum) {
    const std::string name = area_dir.filename().string();
    const std::filesystem::path zon_path = area_dir / (name + ZONE_EXT);
    std::ifstream in(zon_path);
    if (!in) {
        return false;
    }

    std::string line;
    auto strip_cr = [](std::string& s) {
        if (!s.empty() && s.back() == '\r') {
            s.pop_back();
        }
    };

    if (!std::getline(in, line)) {
        return false;
    }
    strip_cr(line);
    static const std::regex k_header(R"(^#(\d+))");
    std::smatch match;
    if (!std::regex_match(line, match, k_header)) {
        return false;
    }
    zone_num = std::stoi(match[1].str());

    if (!std::getline(in, line)) {
        return false;
    }
    strip_cr(line);
    if (!line.empty() && line.back() == '~') {
        line.pop_back();
    }
    zone_name = line;

    if (!std::getline(in, line)) {
        return false;
    }
    strip_cr(line);
    std::istringstream numbers(line);
    numbers >> top_vnum;
    return true;
}

std::vector<AreeAreaInfo> scan_aree_areas(const AreeWorkspace& workspace) {
    std::vector<AreeAreaInfo> areas;
    std::error_code ec;
    if (!std::filesystem::is_directory(workspace.root, ec)) {
        return areas;
    }

    for (const auto& entry : std::filesystem::directory_iterator(workspace.root, ec)) {
        if (!entry.is_directory()) {
            continue;
        }
        if (!is_aree_area_directory(entry.path())) {
            continue;
        }
        AreeAreaInfo info;
        info.folder_name = entry.path().filename().string();
        info.path = entry.path();
        read_aree_zone_header(entry.path(), info.zone_num, info.zone_name, info.top_vnum);
        areas.push_back(std::move(info));
    }

    std::sort(areas.begin(), areas.end(),
              [](const AreeAreaInfo& a, const AreeAreaInfo& b) { return a.folder_name < b.folder_name; });
    return areas;
}

std::filesystem::path aree_archive_destination(const AreeWorkspace& workspace, const std::string& area_folder,
                                               const std::string& label) {
    std::string id = timestamp_id();
    const std::string slug = sanitize_archive_label(label);
    if (!slug.empty()) {
        id += '-' + slug;
    }
    return workspace.archives_dir / area_folder / id;
}

std::filesystem::path archive_aree_area(const AreeWorkspace& workspace, const std::string& area_folder,
                                        const std::string& label, ProgressCallback progress) {
    const std::filesystem::path source = workspace.root / area_folder;
    if (!is_aree_area_directory(source)) {
        throw std::runtime_error("not an area directory: " + source.string());
    }
    const std::filesystem::path destination = aree_archive_destination(workspace, area_folder, label);
    if (progress) {
        progress("Archiving " + source.string() + " -> " + destination.string());
    }
    copy_directory_recursive(source, destination, progress);
    write_archive_meta(destination, area_folder, label);
    return destination;
}

void restore_aree_area_from_archive(const AreeWorkspace& workspace, const std::string& area_folder,
                                    const std::filesystem::path& archive_path, ProgressCallback progress) {
    const std::filesystem::path target = workspace.root / area_folder;
    if (!std::filesystem::is_directory(archive_path)) {
        throw std::runtime_error("archive not found: " + archive_path.string());
    }
    std::error_code ec;
    if (std::filesystem::exists(target, ec)) {
        std::filesystem::remove_all(target, ec);
    }
    if (progress) {
        progress("Restoring " + archive_path.string() + " -> " + target.string());
    }
    copy_directory_recursive(archive_path, target, progress);
}

std::vector<AreeArchiveInfo> list_aree_archives(const AreeWorkspace& workspace, const std::string& area_folder) {
    std::vector<AreeArchiveInfo> archives;
    const std::filesystem::path area_archives = workspace.archives_dir / area_folder;
    std::error_code ec;
    if (!std::filesystem::is_directory(area_archives, ec)) {
        return archives;
    }

    for (const auto& entry : std::filesystem::directory_iterator(area_archives, ec)) {
        if (!entry.is_directory()) {
            continue;
        }
        AreeArchiveInfo info;
        info.id = entry.path().filename().string();
        info.path = entry.path();
        const auto meta = entry.path() / "archive.txt";
        if (std::filesystem::is_regular_file(meta, ec)) {
            std::ifstream in(meta);
            std::string line;
            while (std::getline(in, line)) {
                if (line.rfind("label=", 0) == 0) {
                    info.label = line.substr(6);
                }
            }
        }
        archives.push_back(std::move(info));
    }

    std::sort(archives.begin(), archives.end(),
              [](const AreeArchiveInfo& a, const AreeArchiveInfo& b) { return a.id > b.id; });
    return archives;
}

} // namespace nebbie
