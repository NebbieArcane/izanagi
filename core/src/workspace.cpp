#include "eleutheria/workspace.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <regex>

namespace eleutheria {
namespace {

bool is_reserved_folder(const std::string& name) {
  if(name.empty() || name[0] == '.') {
    return true;
  }
  static const char* kReserved[] = {"_head", "_tail", "mudroot", "build", "src", "docs",
                                    "scripts", "node_modules"};
  for(const char* r : kReserved) {
    if(name == r) {
      return true;
    }
  }
  return false;
}

bool read_zone_header(const std::filesystem::path& area_dir, int& zone_num, std::string& zone_name,
                      int& top_vnum) {
  const auto zon = area_dir / (area_dir.filename().string() + ".zon");
  std::ifstream in(zon);
  if(!in) {
    return false;
  }
  std::string line;
  if(!std::getline(in, line)) {
    return false;
  }
  if(!line.empty() && line.back() == '\r') {
    line.pop_back();
  }
  static const std::regex k_header(R"(^#(\d+))");
  std::smatch match;
  if(!std::regex_match(line, match, k_header)) {
    return false;
  }
  zone_num = std::stoi(match[1].str());
  if(!std::getline(in, line)) {
    return false;
  }
  if(!line.empty() && line.back() == '\r') {
    line.pop_back();
  }
  if(!line.empty() && line.back() == '~') {
    line.pop_back();
  }
  zone_name = line;
  if(!std::getline(in, line)) {
    return false;
  }
  if(!line.empty() && line.back() == '\r') {
    line.pop_back();
  }
  std::sscanf(line.c_str(), "%d", &top_vnum);
  return true;
}

bool directory_has_area_children(const std::filesystem::path& dir) {
  std::error_code ec;
  if(!std::filesystem::is_directory(dir, ec)) {
    return false;
  }
  for(const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
    if(entry.is_directory() && is_area_directory(entry.path())) {
      return true;
    }
  }
  return false;
}

}  // namespace

bool is_area_directory(const std::filesystem::path& dir) {
  std::error_code ec;
  if(!std::filesystem::is_directory(dir, ec)) {
    return false;
  }
  const std::string name = dir.filename().string();
  if(is_reserved_folder(name)) {
    return false;
  }
  // area/area.zon oppure nome/nome.zon oppure area.wld
  if(std::filesystem::exists(dir / (name + ".zon"), ec) ||
     std::filesystem::exists(dir / (name + ".wld"), ec) ||
     std::filesystem::exists(dir / (name + ".ele"), ec)) {
    return true;
  }
  if(std::filesystem::exists(dir / "area" / "area.zon", ec) ||
     std::filesystem::exists(dir / "area" / "area.wld", ec)) {
    return true;
  }
  return false;
}

bool is_workspace_root(const std::filesystem::path& dir) {
  return directory_has_area_children(dir) || directory_has_area_children(dir / "src");
}

std::filesystem::path guess_workspace_root(const std::filesystem::path& path) {
  std::error_code ec;
  auto p = std::filesystem::weakly_canonical(path, ec);
  if(ec) {
    p = path;
  }
  if(std::filesystem::is_regular_file(p, ec)) {
    p = p.parent_path();
  }
  if(is_area_directory(p)) {
    const auto parent = p.parent_path();
    if(parent.filename() == "src" && is_workspace_root(parent.parent_path())) {
      return parent.parent_path();
    }
    if(is_workspace_root(parent)) {
      return parent;
    }
    return parent;
  }
  if(is_workspace_root(p)) {
    return p;
  }
  if(p.filename() == "src" && is_workspace_root(p.parent_path())) {
    return p.parent_path();
  }
  return p;
}

Workspace make_workspace(const std::filesystem::path& root) {
  Workspace ws;
  ws.root = root;
  if(directory_has_area_children(root)) {
    ws.areas_container = root;
  } else if(directory_has_area_children(root / "src")) {
    ws.areas_container = root / "src";
  } else {
    ws.areas_container = root;
  }
  return ws;
}

std::vector<WorkspaceArea> scan_workspace_areas(const Workspace& workspace) {
  std::vector<WorkspaceArea> areas;
  std::error_code ec;
  if(!std::filesystem::is_directory(workspace.areas_container, ec)) {
    return areas;
  }
  for(const auto& entry : std::filesystem::directory_iterator(workspace.areas_container, ec)) {
    if(!entry.is_directory() || !is_area_directory(entry.path())) {
      continue;
    }
    WorkspaceArea info;
    info.folder_name = entry.path().filename().string();
    info.path = entry.path();
    // Prefer area/ subfolder content if present
    if(std::filesystem::exists(entry.path() / "area" / "area.zon", ec)) {
      info.path = entry.path() / "area";
      read_zone_header(info.path, info.zone_num, info.zone_name, info.top_vnum);
    } else {
      read_zone_header(entry.path(), info.zone_num, info.zone_name, info.top_vnum);
    }
    areas.push_back(std::move(info));
  }

  std::vector<std::size_t> order(areas.size());
  for(std::size_t i = 0; i < order.size(); ++i) {
    order[i] = i;
  }
  std::sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) {
    if(areas[a].zone_num != areas[b].zone_num) {
      return areas[a].zone_num < areas[b].zone_num;
    }
    return areas[a].folder_name < areas[b].folder_name;
  });
  int next_bottom = 0;
  for(const std::size_t index : order) {
    areas[index].bottom_vnum = next_bottom;
    next_bottom = areas[index].top_vnum + 1;
  }
  std::sort(areas.begin(), areas.end(),
            [](const WorkspaceArea& a, const WorkspaceArea& b) {
              return a.folder_name < b.folder_name;
            });
  return areas;
}

}  // namespace eleutheria
