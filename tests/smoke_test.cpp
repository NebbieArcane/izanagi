#include "eleutheria/area_io.hpp"
#include "eleutheria/workspace.hpp"
#include "eleutheria/world.hpp"

#include <filesystem>
#include <iostream>

int main(int argc, char** argv) {
  if(argc < 2) {
    std::cerr << "usage: eleutheria_smoke <area-dir-or-file>\n";
    return 2;
  }
  try {
    const auto loaded = eleutheria::load_area(argv[1]);
    std::cout << "Base: " << loaded.area.base_path << "\n";
    std::cout << "Area: " << loaded.area.meta.name << " by " << loaded.area.meta.author << "\n";
    std::cout << "Rooms: " << loaded.area.rooms.size()
              << " Mobs: " << loaded.area.mobiles.size()
              << " Objs: " << loaded.area.objects.size()
              << " Zones: " << loaded.area.zones.size()
              << " Shops: " << loaded.area.shops.size() << "\n";
    if(argc >= 3) {
      const auto again = eleutheria::load_area(argv[2]);
      std::cout << "Second path rooms: " << again.area.rooms.size() << "\n";
      if(again.area.rooms.empty()) {
        std::cerr << "Second path loaded empty\n";
        return 1;
      }
    }
    for(const auto& w : loaded.warnings) {
      std::cout << "WARN: " << w << "\n";
    }
    if(loaded.area.rooms.empty() && loaded.area.mobiles.empty()) {
      std::cerr << "Nothing loaded\n";
      return 1;
    }

    const auto tmp = std::filesystem::temp_directory_path() / "eleutheria_smoke_out";
    std::filesystem::create_directories(tmp);
    const auto base = tmp / "roundtrip";
    auto area_copy = loaded.area;
    eleutheria::save_area(area_copy, base);
    const auto again = eleutheria::load_area(base);
    if(again.area.rooms.size() != loaded.area.rooms.size() ||
       again.area.mobiles.size() != loaded.area.mobiles.size() ||
       again.area.objects.size() != loaded.area.objects.size()) {
      std::cerr << "Roundtrip size mismatch\n";
      return 1;
    }
    const auto issues = eleutheria::validate_area(loaded.area);
    std::cout << "Validation issues: " << issues.size() << "\n";

    // Se il path e' sotto Aree, verifica scan workspace
    const auto ws_root = eleutheria::guess_workspace_root(argv[1]);
    if(eleutheria::is_workspace_root(ws_root)) {
      const auto ws = eleutheria::make_workspace(ws_root);
      const auto areas = eleutheria::scan_workspace_areas(ws);
      std::cout << "Workspace " << ws.root << " areas: " << areas.size() << "\n";
      if(areas.empty()) {
        std::cerr << "Workspace scan empty\n";
        return 1;
      }
    }

    std::cout << "OK\n";
    return 0;
  } catch(const std::exception& e) {
    std::cerr << "FAIL: " << e.what() << "\n";
    return 1;
  }
}
