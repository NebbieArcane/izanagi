#pragma once

#include "eleutheria/types.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace eleutheria {

struct LoadResult {
  Area area;
  std::vector<std::string> warnings;
};

// Open any companion path (.wld/.mob/.obj/.zon/.shp/.ele) or a directory
// (cartella area, mudroot, mudroot/lib con myst.*).
LoadResult load_area(const std::filesystem::path& path);

void save_area(Area& area, const std::filesystem::path& base_path);

// Risolve path utente → base senza estensione (es. .../B1 oppure .../lib/myst).
std::filesystem::path area_base_from_path(const std::filesystem::path& path);

// true se nella base esistono almeno wld/mob/obj/zon
bool area_base_has_world_files(const std::filesystem::path& base);

void parse_rooms_file(const std::string& text, std::map<std::int64_t, Room>* out,
                      std::vector<std::string>* warnings);
void parse_mobiles_file(const std::string& text, std::map<std::int64_t, Mobile>* out,
                        std::vector<std::string>* warnings);
void parse_objects_file(const std::string& text, std::map<std::int64_t, GameObject>* out,
                        std::vector<std::string>* warnings);
void parse_zones_file(const std::string& text, std::map<int, Zone>* out,
                      std::vector<std::string>* warnings);
void parse_shops_file(const std::string& text, std::map<std::int64_t, Shop>* out,
                      std::vector<std::string>* warnings);
AreaMeta parse_ele_file(const std::string& text);

std::string serialize_rooms(const std::map<std::int64_t, Room>& rooms);
std::string serialize_mobiles(const std::map<std::int64_t, Mobile>& mobiles);
std::string serialize_objects(const std::map<std::int64_t, GameObject>& objects);
std::string serialize_zones(const std::map<int, Zone>& zones);
std::string serialize_shops(const std::map<std::int64_t, Shop>& shops);
std::string serialize_ele(const AreaMeta& meta);

}  // namespace eleutheria
