#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace eleutheria {

struct WorkspaceArea {
  std::string folder_name;
  std::filesystem::path path;
  int zone_num = 0;
  std::string zone_name;
  int top_vnum = 0;
  int bottom_vnum = 0;
};

struct Workspace {
  std::filesystem::path root;
  /** Directory che contiene le sottocartelle area (root oppure root/src). */
  std::filesystem::path areas_container;
};

bool is_area_directory(const std::filesystem::path& dir);
bool is_workspace_root(const std::filesystem::path& dir);

/** Se path e' un'area, restituisce il workspace parent (Aree o Aree/src → Aree). */
std::filesystem::path guess_workspace_root(const std::filesystem::path& path);

Workspace make_workspace(const std::filesystem::path& root);
std::vector<WorkspaceArea> scan_workspace_areas(const Workspace& workspace);

}  // namespace eleutheria
