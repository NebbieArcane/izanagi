#pragma once

#include "types.hpp"
#include "world.hpp"

#include <iosfwd>
#include <optional>
#include <string>

namespace nebbie {

long room_zone_field_for_save(const Room& room, const World& world);

bool zone_data_line_matches_room(const std::string& raw, const Room& room, const World& world);

bool exit_data_line_matches(const Exit& exit, const std::string& raw);

void write_zone_data_line(std::ostream& out, const Room& room, const World& world);

void write_exit_data_line(std::ostream& out, const Exit& exit);

} // namespace nebbie
