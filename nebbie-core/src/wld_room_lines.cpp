#include "nebbie/wld_room_lines.hpp"

#include "nebbie/fread.hpp"
#include "nebbie/legacy_format.hpp"

#include <ostream>

namespace nebbie {

namespace {

constexpr long TELE_COUNT = 1;

std::optional<std::string> second_whitespace_token(const std::string& line) {
    std::size_t index = 0;
    while (index < line.size() && (line[index] == ' ' || line[index] == '\t')) {
        ++index;
    }
    while (index < line.size() && line[index] != ' ' && line[index] != '\t') {
        ++index;
    }
    while (index < line.size() && (line[index] == ' ' || line[index] == '\t')) {
        ++index;
    }
    if (index >= line.size()) {
        return std::nullopt;
    }
    const std::size_t start = index;
    while (index < line.size() && line[index] != ' ' && line[index] != '\t') {
        ++index;
    }
    return line.substr(start, index - start);
}

} // namespace

long room_zone_field_for_save(const Room& room, const World& world) {
    if (room.zone_line_primary.has_value()) {
        return *room.zone_line_primary;
    }
    if (room.zone_index >= 0 && room.zone_index < static_cast<int>(world.zones.size())) {
        return static_cast<long>(world.zones[static_cast<std::size_t>(room.zone_index)].num);
    }
    return 0L;
}

bool zone_data_line_matches_room(const std::string& raw, const Room& room, const World& world) {
    const auto nums = parse_numbers(raw);
    if (nums.size() < 3) {
        return false;
    }
    if (nums[0] != room_zone_field_for_save(room, world)) {
        return false;
    }
    if (nums[1] != room.room_flags) {
        return false;
    }

    if (room.tele_time || room.tele_targ || room.tele_mask) {
        if (nums.size() < 6 || nums[2] != -1) {
            return false;
        }
        if (nums[3] != room.tele_time || nums[4] != room.tele_targ || nums[5] != room.tele_mask) {
            return false;
        }
        std::size_t sector_index = 6;
        if (room.tele_mask & TELE_COUNT) {
            if (nums.size() < 8 || nums[6] != room.tele_cnt) {
                return false;
            }
            sector_index = 7;
        }
        return nums.size() > sector_index && nums[sector_index] == room.sector_type;
    }

    return nums[2] == room.sector_type;
}

bool exit_data_line_matches(const Exit& exit, const std::string& raw) {
    const auto nums = parse_numbers(raw);
    if (nums.size() < 3) {
        return false;
    }
    if (nums[0] != exit.exit_info || nums[1] != exit.key || nums[2] != exit.to_room) {
        return false;
    }
    const long open_cmd = nums.size() >= 4 ? nums[3] : -1L;
    return open_cmd == exit.open_cmd;
}

void write_zone_data_line(std::ostream& out, const Room& room, const World& world) {
    if (room.zone_data_line_raw && zone_data_line_matches_room(*room.zone_data_line_raw, room, world)) {
        out << *room.zone_data_line_raw << '\n';
        return;
    }

    const long zone_field = room_zone_field_for_save(room, world);
    const std::optional<std::string> flag_style =
        room.zone_data_line_raw ? second_whitespace_token(*room.zone_data_line_raw) : std::nullopt;
    const std::string room_flags = format_nebbie_bit_mask_for_file(room.room_flags, flag_style);

    if (room.tele_time || room.tele_targ || room.tele_mask) {
        out << zone_field << ' ' << room_flags << " -1 " << room.tele_time << ' ' << room.tele_targ << ' '
            << room.tele_mask;
        if (room.tele_mask & TELE_COUNT) {
            out << ' ' << room.tele_cnt;
        }
        out << ' ' << room.sector_type << '\n';
    } else {
        out << zone_field << ' ' << room_flags << ' ' << room.sector_type << '\n';
    }
}

void write_exit_data_line(std::ostream& out, const Exit& exit) {
    if (exit.data_line_raw && exit_data_line_matches(exit, *exit.data_line_raw)) {
        out << *exit.data_line_raw << '\n';
        return;
    }

    std::optional<std::string> flag_style;
    if (exit.data_line_raw) {
        const auto end_pos = exit.data_line_raw->find_first_of(" \t");
        flag_style = end_pos == std::string::npos ? *exit.data_line_raw
                                                  : exit.data_line_raw->substr(0, end_pos);
    }
    const std::string exit_flags = format_nebbie_bit_mask_for_file(exit.exit_info, flag_style);
    out << exit_flags << ' ' << exit.key << ' ' << exit.to_room << ' ' << exit.open_cmd << '\n';
}

} // namespace nebbie
