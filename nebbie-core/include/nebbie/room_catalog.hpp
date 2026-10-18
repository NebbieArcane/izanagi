#pragma once

#include "mob_catalog.hpp"

#include <string>
#include <utility>
#include <vector>

namespace nebbie {

std::vector<std::pair<int, std::string>> room_sector_choices();
std::vector<MobFlagDef> room_flag_defs();
std::vector<MobFlagDef> exit_flag_defs();
std::vector<std::pair<int, std::string>> exit_direction_choices();
/** open_cmd values for secret/special doors (cmd id → label). Includes -1 = default open. */
std::vector<std::pair<int, std::string>> exit_open_cmd_choices();

std::string room_sector_name(int sector_type);
std::string exit_direction_label(int direction);
std::string exit_open_cmd_label(int open_cmd);

bool room_sector_uses_river(int sector_type);
bool room_flags_use_moblim(long room_flags);

} // namespace nebbie
