#include "nebbie/source_blocks.hpp"

#include "nebbie/constants.hpp"
#include "nebbie/file_io.hpp"
#include "nebbie/legacy_format.hpp"
#include "nebbie/types.hpp"
#include "nebbie/world.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>
#include <set>
#include <sstream>

namespace nebbie {

namespace {

constexpr long SECT_WATER_NOSWIM = 7;
constexpr long SECT_UNDERWATER = 8;
constexpr long TELE_COUNT = 1;

bool is_line_start_hash(const std::string& content, std::size_t index) {
    if (index >= content.size() || content[index] != '#') {
        return false;
    }
    if (index == 0) {
        return true;
    }
    return content[index - 1] == '\n' || content[index - 1] == '\r';
}

std::optional<long> parse_hash_vnum_at(const std::string& content, std::size_t hash_index) {
    std::size_t i = hash_index + 1;
    if (i >= content.size() || !std::isdigit(static_cast<unsigned char>(content[i]))) {
        return std::nullopt;
    }
    long vnum = 0;
    while (i < content.size() && std::isdigit(static_cast<unsigned char>(content[i]))) {
        vnum = vnum * 10 + (content[i] - '0');
        ++i;
    }
    if (vnum <= 0) {
        return std::nullopt;
    }
    return vnum;
}

std::string ensure_trailing_newline_lf(std::string text) {
    while (!text.empty() && (text.back() == '\n' || text.back() == '\r')) {
        text.pop_back();
    }
    if (!text.empty()) {
        text.push_back('\n');
    }
    return text;
}

void append_string_field(std::ostringstream& out, const std::string& value) {
    out << value << "~\n";
}

bool mob_uses_hit_dice(char mobtype) {
    return mobtype == 'S';
}

std::string format_mobile_block(const Mobile& mob) {
    std::ostringstream out;
    out << '#' << mob.vnum << '\n';
    append_string_field(out, mob.name);
    append_string_field(out, mob.short_descr);
    append_string_field(out, mob.long_descr);
    append_string_field(out, mob.description);

    if (mob.mobtype == 'A' || mob.mobtype == 'B' || mob.mobtype == 'L') {
        out << format_nebbie_bit_mask(mob.act) << ' ' << format_nebbie_bit_mask(mob.affected_by) << ' '
            << mob.alignment << ' ' << mob.mobtype << ' ' << mob.mult_att << '\n';
    } else {
        out << format_nebbie_bit_mask(mob.act) << ' ' << format_nebbie_bit_mask(mob.affected_by) << ' '
            << mob.alignment << ' ' << mob.mobtype << '\n';
    }

    if (mob_uses_hit_dice(mob.mobtype)) {
        out << mob.level << ' ' << mob.hitroll << ' ' << mob.ac << ' ' << mob.hit_dice << ' '
            << mob.dam_dice << '\n';
    } else {
        out << mob.level << ' ' << mob.hitroll << ' ' << mob.ac << ' ' << mob.hit_bonus << ' '
            << mob.dam_dice << '\n';
    }

    if (mob.extended_gold) {
        if (mob.race != 0 || mob.exp != 0) {
            out << "-1 " << mob.gold << ' ' << mob.exp << ' ' << mob.race << '\n';
        } else {
            out << "-1 " << mob.gold << '\n';
        }
    } else {
        out << mob.gold << ' ' << mob.exp << '\n';
    }

    if (mob.extended_sex) {
        out << mob.position << ' ' << mob.default_pos << ' ' << (mob.sex + 3) << ' '
            << format_nebbie_bit_mask(mob.immune) << ' ' << format_nebbie_bit_mask(mob.meta_immune)
            << ' ' << format_nebbie_bit_mask(mob.susceptible) << '\n';
    } else {
        out << mob.position << ' ' << mob.default_pos << ' ' << mob.sex << '\n';
    }

    const bool write_sounds = mob.mobtype == 'L' || !mob.sounds.empty() || !mob.distant_sounds.empty()
                              || !mob.extra_sound_strings.empty();
    if (write_sounds) {
        append_string_field(out, mob.sounds);
        append_string_field(out, mob.distant_sounds);
        for (const auto& extra : mob.extra_sound_strings) {
            append_string_field(out, extra);
        }
    }

    return ensure_trailing_newline_lf(out.str());
}

std::string format_room_block(const Room& room, const World& world) {
    std::ostringstream out;
    out << '#' << room.vnum << '\n';
    append_string_field(out, room.name);
    append_string_field(out, room.description);

    const long zone_field = room.zone_line_primary.has_value()
                                ? *room.zone_line_primary
                                : (room.zone_index >= 0 && room.zone_index < static_cast<int>(world.zones.size())
                                       ? static_cast<long>(world.zones[static_cast<std::size_t>(room.zone_index)].num)
                                       : 0L);

    if (room.tele_time || room.tele_targ || room.tele_mask) {
        out << zone_field << ' ' << format_nebbie_bit_mask(room.room_flags) << " -1 " << room.tele_time << ' '
            << room.tele_targ << ' ' << room.tele_mask;
        if (room.tele_mask & TELE_COUNT) {
            out << ' ' << room.tele_cnt;
        }
        out << ' ' << room.sector_type << '\n';
    } else {
        out << zone_field << ' ' << format_nebbie_bit_mask(room.room_flags) << ' ' << room.sector_type << '\n';
    }

    if (room.sector_type == SECT_WATER_NOSWIM || room.sector_type == SECT_UNDERWATER) {
        if (room.river_speed || room.river_dir) {
            out << room.river_speed << ' ' << room.river_dir << '\n';
        }
    }

    for (const auto& exit : room.exits) {
        out << 'D' << exit.direction << '\n';
        append_string_field(out, exit.description);
        append_string_field(out, exit.keyword);
        out << format_nebbie_bit_mask(exit.exit_info) << ' ' << exit.key << ' ' << exit.to_room << ' '
            << exit.open_cmd << '\n';
    }

    for (const auto& extra : room.extra_descs) {
        out << "E\n";
        append_string_field(out, extra.keyword);
        append_string_field(out, extra.description);
    }

    if (!room.bright_at_night.empty() || !room.bright_at_day.empty()) {
        out << "L\n";
        append_string_field(out, room.bright_at_night);
        append_string_field(out, room.bright_at_day);
    }

    out << "S\n";
    return ensure_trailing_newline_lf(out.str());
}

std::string format_object_block(const GameObject& obj) {
    std::ostringstream out;
    out << '#' << obj.vnum << '\n';
    append_string_field(out, obj.name);
    append_string_field(out, obj.short_descr);
    append_string_field(out, obj.description);
    append_string_field(out, obj.action_description);

    out << obj.type_flag << ' ' << format_nebbie_bit_mask(obj.extra_flags) << ' '
        << format_nebbie_bit_mask(obj.wear_flags) << '\n';
    out << obj.value[0] << ' ' << obj.value[1] << ' ' << obj.value[2] << ' ' << obj.value[3] << '\n';
    out << obj.weight << ' ' << obj.cost << ' ' << obj.cost_per_day << '\n';

    for (const auto& extra : obj.extra_descs) {
        out << "E\n";
        append_string_field(out, extra.keyword);
        append_string_field(out, extra.description);
    }

    for (const auto& affect : obj.affects) {
        out << "A\n" << affect.location << ' ' << affect.modifier << '\n';
    }

    if (obj.has_extra_flags2) {
        out << "F\n" << obj.extra_flags2 << '\n';
    }

    if (!obj.forbidden_char.empty() || !obj.forbidden_room.empty()) {
        out << "P\n";
        append_string_field(out, obj.forbidden_char);
        append_string_field(out, obj.forbidden_room);
    }

    return ensure_trailing_newline_lf(out.str());
}

template <typename EntityMap, typename Formatter>
void write_preserved_hash_file(const World& world,
                               const std::filesystem::path& path,
                               ProgressCallback progress,
                               MystSaveOptions options,
                               PreserveEntitySaveOptions preserve,
                               const EntityMap& entities,
                               Formatter formatter) {
    if (progress) {
        progress("Writing (preserve) " + path.string());
    }

    if (!preserve.enabled || preserve.sources == nullptr) {
        throw std::runtime_error("preserve save requested without source blocks");
    }

    const auto& sources = *preserve.sources;
    const std::unordered_set<long> empty_dirty;
    const auto& dirty = preserve.dirty_vnums != nullptr ? *preserve.dirty_vnums : empty_dirty;

    std::ostringstream body;
    std::unordered_set<long> written;

    for (const long vnum : sources.order) {
        const auto entity_it = entities.find(vnum);
        if (entity_it == entities.end()) {
            continue;
        }

        const bool is_dirty = dirty.find(vnum) != dirty.end();
        const auto block_it = sources.blocks.find(vnum);
        if (!is_dirty && block_it != sources.blocks.end()) {
            body << block_it->second;
            written.insert(vnum);
            continue;
        }
        body << formatter(entity_it->second);
        written.insert(vnum);
    }

    std::vector<long> appended;
    for (const auto& [vnum, _] : entities) {
        if (written.find(vnum) == written.end()) {
            appended.push_back(vnum);
        }
    }
    std::sort(appended.begin(), appended.end());
    for (const long vnum : appended) {
        const auto entity_it = entities.find(vnum);
        if (entity_it != entities.end()) {
            body << formatter(entity_it->second);
        }
    }

    FILE* fp = open_file_write(path, "hash file");
    const std::string output = body.str();
    if (!output.empty()) {
        std::fwrite(output.data(), 1, output.size(), fp);
    }
    if (options.write_eof_markers) {
        if (path.extension().string() == WORLD_EXT) {
            std::fprintf(fp, "#0\n");
        } else {
            std::fprintf(fp, "%%%%\n");
        }
    }
    std::fclose(fp);
}

} // namespace

std::optional<FileSourceBlocks> capture_vnum_hash_file(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return std::nullopt;
    }

    const std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    FileSourceBlocks result;

    for (std::size_t i = 0; i < content.size(); ++i) {
        if (!is_line_start_hash(content, i)) {
            continue;
        }

        const auto vnum = parse_hash_vnum_at(content, i);
        if (!vnum.has_value()) {
            continue;
        }

        std::size_t next = i + 1;
        while (next < content.size()) {
            if (is_line_start_hash(content, next)) {
                break;
            }
            ++next;
        }

        std::string block = content.substr(i, next - i);
        result.order.push_back(*vnum);
        result.blocks[*vnum] = block;
        i = next > i ? next - 1 : next;
    }

    return result;
}

void save_myst_mob_preserve(const World& world,
                            const std::filesystem::path& path,
                            ProgressCallback progress,
                            MystSaveOptions options,
                            PreserveEntitySaveOptions preserve) {
    write_preserved_hash_file(
        world, path, progress, options, preserve, world.mobiles,
        [](const Mobile& mob) { return format_mobile_block(mob); });
}

void save_myst_wld_preserve(const World& world,
                            const std::filesystem::path& path,
                            ProgressCallback progress,
                            MystSaveOptions options,
                            PreserveEntitySaveOptions preserve) {
    write_preserved_hash_file(
        world, path, progress, options, preserve, world.rooms,
        [&world](const Room& room) { return format_room_block(room, world); });
}

void save_myst_obj_preserve(const World& world,
                            const std::filesystem::path& path,
                            ProgressCallback progress,
                            MystSaveOptions options,
                            PreserveEntitySaveOptions preserve) {
    write_preserved_hash_file(
        world, path, progress, options, preserve, world.objects,
        [](const GameObject& obj) { return format_object_block(obj); });
}

} // namespace nebbie
