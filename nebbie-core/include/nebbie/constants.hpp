#pragma once

#include <string>

namespace nebbie {

constexpr const char* ZONE_FILE = "myst.zon";
constexpr const char* WORLD_FILE = "myst.wld";
constexpr const char* MOB_FILE = "myst.mob";
constexpr const char* OBJ_FILE = "myst.obj";
constexpr const char* SHOP_FILE = "myst.shp";
constexpr const char* SPECIAL_FILE = "myst.spe";
constexpr const char* DAMAGE_FILE = "myst.dam";
constexpr const char* SOCIAL_FILE = "myst.act";
constexpr const char* POSE_FILE = "myst.pos";
constexpr const char* GUILD_FILE = "myst.gui";

constexpr const char* OVERLAY_ROOMS_DIR = "rooms";
constexpr const char* OVERLAY_OBJECTS_DIR = "objects";
constexpr const char* OVERLAY_MOBILES_DIR = "mobiles";
constexpr const char* OVERLAY_ZONES_DIR = "zones";

constexpr const char* ZONE_EXT = ".zon";
constexpr const char* WORLD_EXT = ".wld";
constexpr const char* MOB_EXT = ".mob";
constexpr const char* OBJ_EXT = ".obj";
constexpr const char* SHOP_EXT = ".shp";
constexpr const char* SPECIAL_EXT = ".spe";
constexpr const char* DAMAGE_EXT = ".dam";
constexpr const char* SOCIAL_EXT = ".act";
constexpr const char* POSE_EXT = ".pos";
constexpr const char* GUILD_EXT = ".gui";

inline bool is_lib_file_extension(const std::string& extension) {
    return extension == ZONE_EXT || extension == WORLD_EXT || extension == MOB_EXT
           || extension == OBJ_EXT || extension == SHOP_EXT || extension == SPECIAL_EXT
           || extension == DAMAGE_EXT || extension == SOCIAL_EXT || extension == POSE_EXT
           || extension == GUILD_EXT;
}

constexpr int MAX_DIR = 6;

} // namespace nebbie
