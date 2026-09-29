#include "nebbie/io.hpp"
#include "nebbie/legacy_format.hpp"
#include "nebbie/overlay_io.hpp"
#include "nebbie/wld_room_lines.hpp"

#include "nebbie/fread.hpp"
#include "nebbie/file_io.hpp"
#include "nebbie/nebbie_string_field.hpp"

#include <cstdio>
#include <cstdlib>
#include <sstream>

namespace nebbie {

namespace {

constexpr long SECT_WATER_NOSWIM = 7;
constexpr long SECT_UNDERWATER = 8;
constexpr long TUNNEL = 64;
constexpr long TELE_COUNT = 1;

std::string trim_line(std::string line) {
    while (!line.empty() && (line.back() == ' ' || line.back() == '\t')) {
        line.pop_back();
    }
    std::size_t start = 0;
    while (start < line.size() && (line[start] == ' ' || line[start] == '\t')) {
        ++start;
    }
    return line.substr(start);
}

std::string read_data_line(FILE* fp) {
    while (true) {
        const std::string line = trim_line(fread_line(fp));
        if (!line.empty() && line != "~") {
            return line;
        }
    }
}

bool looks_like_exit_data_line(const std::string& line) {
    if (line.empty() || line == "~") {
        return false;
    }
    switch (line[0]) {
    case 'D':
    case 'E':
    case 'L':
    case 'S':
    case 'C':
        return false;
    default:
        return true;
    }
}

std::string read_exit_data_line(FILE* fp) {
    while (true) {
        const std::string line = trim_line(fread_line(fp));
        if (looks_like_exit_data_line(line)) {
            return line;
        }
        if (line.empty()) {
            continue;
        }
        throw ParseError("Unexpected line in exit record (found '" + line + "')");
    }
}

void read_exit(FILE* fp, Room& room, int direction, const World& world) {
    Exit exit;
    exit.direction = direction;
    exit.description = fread_string(fp);
    exit.keyword = fread_string(fp);

    const std::string data_line = read_exit_data_line(fp);
    exit.data_line_raw = data_line;
    const auto nums = parse_numbers(data_line);
    if (nums.size() < 3) {
        throw ParseError("Room " + std::to_string(room.vnum) + " exit D" + std::to_string(direction)
                         + ": expected flags, key, and to_room");
    }
    exit.exit_info = nums[0];
    exit.key = nums[1];
    exit.to_room = nums[2];
    exit.open_cmd = nums.size() >= 4 ? nums[3] : -1;

    if (exit_data_line_looks_like_zone_line(room, data_line, world)) {
        exit.data_line_raw.reset();
        exit.exit_info = 0;
        exit.key = 0;
        exit.to_room = 0;
        exit.open_cmd = -1;
    }

    room.exits.push_back(exit);
}

void read_room_zone_line(FILE* fp, Room& room) {
    const std::string raw = read_data_line(fp);
    room.zone_data_line_raw = raw;
    const auto nums = parse_numbers(raw);
    if (nums.size() < 3) {
        throw ParseError("Room " + std::to_string(room.vnum) + ": expected zone, flags, and sector");
    }

    room.zone_line_primary = nums[0];
    room.room_flags = nums[1];
    const long sector_field = nums[2];

    room.tele_time = 0;
    room.tele_targ = 0;
    room.tele_mask = 0;
    room.tele_cnt = 0;

    if (sector_field != -1) {
        room.sector_type = static_cast<int>(sector_field);
        return;
    }

    if (nums.size() >= 7) {
        room.tele_time = nums[3];
        room.tele_targ = nums[4];
        room.tele_mask = nums[5];
        room.sector_type = static_cast<int>(nums[6]);
        if (nums.size() >= 8) {
            room.tele_cnt = nums[6];
            room.sector_type = static_cast<int>(nums[7]);
        }
        return;
    }

    if (nums.size() == 4) {
        room.tele_time = nums[3];
        room.sector_type = 0;
        return;
    }

    if (nums.size() == 3) {
        const auto tele = parse_numbers(read_data_line(fp));
        if (tele.size() >= 4) {
            room.tele_time = tele[0];
            room.tele_targ = tele[1];
            room.tele_mask = tele[2];
            if (tele.size() >= 5) {
                room.tele_cnt = tele[3];
                room.sector_type = static_cast<int>(tele[4]);
            } else {
                room.sector_type = static_cast<int>(tele[3]);
            }
            return;
        }
    }

    throw ParseError("Room " + std::to_string(room.vnum) + ": invalid teleport zone line");
}

void read_room_body(FILE* fp, Room& room, World& world) {
    room.name = fread_string(fp);
    room.description = fread_string(fp);

    read_room_zone_line(fp, room);

    const long sector = room.sector_type;

    if (sector == SECT_WATER_NOSWIM || sector == SECT_UNDERWATER) {
        room.river_speed = fread_if_number(fp);
        room.river_dir = fread_if_number(fp);
    }

    if (room.room_flags & TUNNEL) {
        room.moblim = fread_if_number(fp);
        if (room.moblim < 1) {
            room.moblim = 1;
        }
    }

    if (const Zone* zone = world.zone_for_vnum(room.vnum)) {
        for (size_t i = 0; i < world.zones.size(); ++i) {
            if (&world.zones[i] == zone) {
                room.zone_index = static_cast<int>(i);
                break;
            }
        }
    }

    char token[161];
    while (std::fscanf(fp, " %160s", token) == 1) {
        switch (token[0]) {
        case 'D':
            read_exit(fp, room, std::atoi(token + 1), world);
            break;
        case 'E': {
            ExtraDesc extra;
            extra.keyword = fread_string(fp);
            extra.description = fread_string(fp);
            room.extra_descs.push_back(extra);
            break;
        }
        case 'L':
            room.bright_at_night = fread_string(fp);
            room.bright_at_day = fread_string(fp);
            break;
        case 'S':
            return;
        case 'C':
            fread_to_eol(fp);
            break;
        default:
            fread_to_eol(fp);
            break;
        }
    }

    throw ParseError("Room " + std::to_string(room.vnum) + " missing terminating S");
}

void write_room_body(FILE* fp, const Room& room, const World& world) {
    fwrite_nebbie_string_field(fp, room.name, NebbieTildeStyle::Inline);
    fwrite_nebbie_string_field(fp, room.description, nebbie_paragraph_tilde_style(room.description));

    {
        std::ostringstream zone_line;
        write_zone_data_line(zone_line, room, world);
        std::fputs(zone_line.str().c_str(), fp);
    }

    if (room.sector_type == SECT_WATER_NOSWIM || room.sector_type == SECT_UNDERWATER) {
        if (room.river_speed || room.river_dir) {
            std::fprintf(fp, "%ld %ld\n", room.river_speed, room.river_dir);
        }
    }

    // NebbieArcane load_one_room() does not read a standalone moblim line before
    // D/E/L/S aux records; emitting one breaks boot with "unknown auxiliary code '1'".

    for (const auto& exit : room.exits) {
        std::fprintf(fp, "D%d\n", exit.direction);
        fwrite_nebbie_string_field(fp, exit.description, NebbieTildeStyle::OnOwnLine);
        fwrite_nebbie_string_field(fp, exit.keyword, NebbieTildeStyle::Inline);
        std::ostringstream exit_line;
        write_exit_data_line(exit_line, room, world, exit);
        std::fputs(exit_line.str().c_str(), fp);
    }

    for (const auto& extra : room.extra_descs) {
        std::fprintf(fp, "E\n");
        fwrite_nebbie_string_field(fp, extra.keyword, NebbieTildeStyle::Inline);
        fwrite_nebbie_string_field(fp, extra.description, nebbie_paragraph_tilde_style(extra.description));
    }

    if (!room.bright_at_night.empty() || !room.bright_at_day.empty()) {
        std::fprintf(fp, "L\n");
        fwrite_nebbie_string_field(fp, room.bright_at_night, nebbie_paragraph_tilde_style(room.bright_at_night));
        fwrite_nebbie_string_field(fp, room.bright_at_day, nebbie_paragraph_tilde_style(room.bright_at_day));
    }

    std::fprintf(fp, "S\n");
}

char read_wld_marker(FILE* fp) {
    while (true) {
        int c = std::fgetc(fp);
        if (c == EOF) {
            return '\0';
        }
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            continue;
        }
        if (c == '*') {
            fread_to_eol(fp);
            continue;
        }
        return static_cast<char>(c);
    }
}

bool peek_is_eof(FILE* fp) {
    const long pos = std::ftell(fp);
    int c = std::fgetc(fp);
    while (c != EOF && (c == ' ' || c == '\t' || c == '\r' || c == '\n')) {
        c = std::fgetc(fp);
    }
    const bool eof = (c == EOF);
    std::fseek(fp, pos, SEEK_SET);
    return eof;
}

} // namespace

void load_myst_wld(World& world, const std::filesystem::path& path, ProgressCallback progress) {
    FILE* fp = open_file_read(path, "world file");
    if (progress) {
        progress("Loading " + path.string());
    }

    while (true) {
        const char marker = read_wld_marker(fp);
        if (marker == '\0') {
            break;
        }
        if (marker != '#') {
            fread_to_eol(fp);
            continue;
        }

        if (!fread_peek_is_number(fp)) {
            fread_to_eol(fp);
            continue;
        }

        const long vnum = fread_number(fp);
        if (vnum == 0 && peek_is_eof(fp)) {
            break;
        }

        Room room;
        room.vnum = vnum;
        read_room_body(fp, room, world);
        world.rooms[vnum] = std::move(room);
    }

    std::fclose(fp);
}

void save_myst_wld(const World& world, const std::filesystem::path& path, ProgressCallback progress,
                   MystSaveOptions options) {
    if (progress) {
        progress("Saving " + path.string());
    }

    FILE* fp = open_file_write(path, "world file");
    for (const auto& [vnum, room] : world.rooms) {
        (void)vnum;
        std::fprintf(fp, "#%ld\n", room.vnum);
        write_room_body(fp, room, world);
    }
    if (options.write_eof_markers) {
        std::fprintf(fp, "#0\n");
    }
    std::fclose(fp);
}

void save_room_overlay(const Room& room, const World& world, const std::filesystem::path& path) {
    FILE* fp = open_file_write(path, "world file");
    write_room_body(fp, room, world);
    std::fclose(fp);
}

void load_room_overlay(World& world, const long vnum, const std::filesystem::path& path) {
    FILE* fp = open_file_read(path, "world file");
    Room room;
    room.vnum = vnum;
    read_room_body(fp, room, world);
    std::fclose(fp);
    world.rooms[vnum] = std::move(room);
}

} // namespace nebbie
