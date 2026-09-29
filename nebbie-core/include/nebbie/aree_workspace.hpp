#pragma once

#include "io.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace nebbie {

/** Extensions merged by deploy_aree.php (per-area files). */
constexpr const char* AREE_AREA_EXTENSIONS[] = {".wld", ".mob", ".obj", ".shp", ".spe", ".zon"};

struct AreeAreaInfo {
    std::string folder_name;
    std::filesystem::path path;
    int zone_num = 0;
    std::string zone_name;
    int top_vnum = 0;
};

struct AreeWorkspace {
    /** Folder chosen by user; immediate children are area directories. */
    std::filesystem::path root;
    std::filesystem::path izanagi_dir;
    std::filesystem::path archives_dir;
};

struct AreeArchiveInfo {
    std::string id;
    std::filesystem::path path;
    std::string label;
};

bool is_aree_reserved_folder(const std::string& folder_name);

/** True when <dir>/<basename(dir)>.zon exists (Aree build layout). */
bool is_aree_area_directory(const std::filesystem::path& area_dir);

/** Directory scanned for area folders: workspace root or workspace/src when areas live there. */
std::filesystem::path aree_areas_container(const std::filesystem::path& workspace_root);

/** True when {@link scan_aree_areas} would find at least one area under {@code workspace_root}. */
bool aree_workspace_has_areas(const std::filesystem::path& workspace_root);

/** Resolved path to {@code area_folder} inside an open workspace (handles {@code src/<area>}). */
std::filesystem::path aree_area_path(const AreeWorkspace& workspace, const std::string& area_folder);

enum class AreeLibOpenBlockReason {
    none,
    area_directory,
    workspace_root,
};

struct AreeLibOpenGuard {
    AreeLibOpenBlockReason reason = AreeLibOpenBlockReason::none;
    /** Workspace root to pass to {@link make_aree_workspace} when blocked. */
    std::filesystem::path suggested_workspace_root;
};

/** Detect opening a monolith lib on Aree paths (wrong save mode / EOF). */
AreeLibOpenGuard classify_aree_lib_open_guard(const std::filesystem::path& resolved_lib_directory);

AreeWorkspace make_aree_workspace(const std::filesystem::path& root);

std::vector<AreeAreaInfo> scan_aree_areas(const AreeWorkspace& workspace);

/** Parse #N and zone name from area/<area>.zon (first zone block). */
bool read_aree_zone_header(const std::filesystem::path& area_dir, int& zone_num, std::string& zone_name,
                           int& top_vnum);

std::filesystem::path aree_archive_destination(const AreeWorkspace& workspace, const std::string& area_folder,
                                               const std::string& label = {});

/** Copy area folder to archives/<area>/<timestamp>[_label]. Returns archive path. */
std::filesystem::path archive_aree_area(const AreeWorkspace& workspace, const std::string& area_folder,
                                        const std::string& label = {}, ProgressCallback progress = {});

/** Restore archives/<area>/<archive_id> over workspace/<area>/. */
void restore_aree_area_from_archive(const AreeWorkspace& workspace, const std::string& area_folder,
                                    const std::filesystem::path& archive_path, ProgressCallback progress = {});

std::vector<AreeArchiveInfo> list_aree_archives(const AreeWorkspace& workspace, const std::string& area_folder);

std::filesystem::path aree_session_storage_root(const AreeWorkspace& workspace, const std::string& area_folder);

} // namespace nebbie
