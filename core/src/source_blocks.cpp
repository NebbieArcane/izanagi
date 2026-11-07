#include "eleutheria/source_blocks.hpp"

#include "eleutheria/token_io.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>
#include <sstream>

namespace eleutheria {
namespace {

// Deve restare allineato a area_io.cpp (TELE_COUNT Nebbie = bit 1 → valore 2).
constexpr std::int64_t kTeleCountBit = 2;

bool is_line_start_hash(const std::string& content, std::size_t index) {
  if(index >= content.size() || content[index] != '#') {
    return false;
  }
  if(index == 0) {
    return true;
  }
  return content[index - 1] == '\n' || content[index - 1] == '\r';
}

std::optional<std::int64_t> parse_hash_vnum_at(const std::string& content, std::size_t hash_index) {
  std::size_t i = hash_index + 1;
  if(i >= content.size() || !std::isdigit(static_cast<unsigned char>(content[i]))) {
    return std::nullopt;
  }
  std::int64_t vnum = 0;
  while(i < content.size() && std::isdigit(static_cast<unsigned char>(content[i]))) {
    vnum = vnum * 10 + (content[i] - '0');
    ++i;
  }
  if(vnum <= 0) {
    return std::nullopt;
  }
  return vnum;
}

bool is_zone_terminator_vnum(std::int64_t vnum) {
  return vnum >= 900 && vnum <= 999;
}

std::string ensure_trailing_newline(std::string text) {
  while(!text.empty() && (text.back() == '\n' || text.back() == '\r')) {
    text.pop_back();
  }
  if(!text.empty()) {
    text.push_back('\n');
  }
  return text;
}

}  // namespace

std::optional<FileSourceBlocks> capture_vnum_hash_text(const std::string& content) {
  struct Hit {
    std::int64_t vnum = 0;
    std::size_t start = 0;
    std::size_t end = 0;
  };
  std::vector<Hit> hits;

  for(std::size_t i = 0; i < content.size();) {
    if(!is_line_start_hash(content, i)) {
      ++i;
      continue;
    }
    const auto vnum = parse_hash_vnum_at(content, i);
    if(!vnum || is_zone_terminator_vnum(*vnum)) {
      ++i;
      continue;
    }
    std::size_t next = i + 1;
    while(next < content.size()) {
      if(is_line_start_hash(content, next) && parse_hash_vnum_at(content, next)) {
        break;
      }
      ++next;
    }
    hits.push_back(Hit{*vnum, i, next});
    i = next;
  }

  if(hits.empty()) {
    return std::nullopt;
  }

  FileSourceBlocks result;
  result.valid = true;
  result.prefix = content.substr(0, hits.front().start);
  result.trailer = content.substr(hits.back().end);
  for(const auto& hit : hits) {
    result.order.push_back(hit.vnum);
    result.blocks[hit.vnum] = content.substr(hit.start, hit.end - hit.start);
  }
  return result;
}

std::optional<FileSourceBlocks> capture_vnum_hash_file(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  if(!in) {
    return std::nullopt;
  }
  const std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  return capture_vnum_hash_text(content);
}

std::string format_room_block(const Room& room) {
  std::ostringstream out;
  out << '#' << room.vnum << '\n';
  write_tilde_string(out, room.name);
  write_tilde_string(out, room.description);
  out << room.zone_hint << ' '
      << format_flag_bits_for_file(static_cast<std::uint64_t>(room.room_flags), room.room_flags_token)
      << ' ';
  if(room.has_teleport) {
    out << "-1 " << room.tele_time << ' ' << room.tele_targ << ' '
        << format_flag_bits_for_file(static_cast<std::uint64_t>(room.tele_mask), room.tele_mask_token);
    if(room.tele_mask & kTeleCountBit) {
      out << ' ' << room.tele_cnt;
    }
    out << ' ' << room.sector_type;
  } else {
    out << room.sector_type;
  }
  if(room.has_river) {
    out << ' ' << room.river_speed << ' ' << room.river_dir;
  }
  if(room.has_tunnel_limit) {
    out << ' ' << room.moblim;
  }
  out << " \n";
  for(int d = 0; d < kMaxExits; ++d) {
    const Exit& ex = room.exits[static_cast<std::size_t>(d)];
    if(!ex.present) {
      continue;
    }
    out << 'D' << d << '\n';
    write_tilde_string(out, ex.general_description);
    write_tilde_string(out, ex.keyword);
    out << format_flag_bits_for_file(static_cast<std::uint64_t>(ex.exit_info), ex.exit_info_token)
        << ' ' << ex.key << ' ' << ex.to_room << '\n';
  }
  for(const auto& ed : room.extras) {
    out << "E\n";
    write_tilde_string(out, ed.keywords);
    write_tilde_string(out, ed.description);
  }
  if(room.has_bright) {
    out << "L\n";
    write_tilde_string(out, room.bright_night);
    write_tilde_string(out, room.bright_day);
  }
  out << "S\n";
  return ensure_trailing_newline(out.str());
}

std::string format_mobile_block(const Mobile& mob) {
  std::ostringstream out;
  out << '#' << mob.vnum << '\n';
  write_tilde_string(out, mob.name);
  write_tilde_string(out, mob.short_descr);
  write_tilde_string(out, mob.long_descr);
  write_tilde_string(out, mob.description);
  out << format_flag_bits_for_file(static_cast<std::uint64_t>(mob.act), mob.act_token) << ' '
      << format_flag_bits_for_file(static_cast<std::uint64_t>(mob.affected_by), mob.affected_by_token)
      << ' ' << mob.alignment << ' ' << mob.letter;
  if(mob.letter == 'A' || mob.letter == 'B' || mob.letter == 'L') {
    out << ' ' << static_cast<int>(mob.mult_att);
  }
  out << '\n';
  if(mob.letter == 'S') {
    out << mob.level << ' ' << mob.thac0 << ' ' << mob.armor << ' ' << mob.hit_dice_n << 'd'
        << mob.hit_dice_s << '+' << mob.hit_dice_b << '\n';
    out << mob.dam_dice_n << 'd' << mob.dam_dice_s << '+' << mob.dam_bonus << '\n';
  } else {
    out << mob.level << ' ' << mob.thac0 << ' ' << mob.armor << ' ' << mob.hit_bonus << ' '
        << mob.dam_dice_n << 'd' << mob.dam_dice_s << '+' << mob.dam_bonus << '\n';
  }
  if(mob.gold_exp_race_form) {
    out << "-1 " << mob.gold << ' ' << mob.exp << ' ' << mob.race;
  } else {
    out << mob.gold << ' ' << mob.exp;
  }
  if(mob.has_spellpower) {
    out << ' ' << mob.spellpower;
  }
  out << '\n';
  out << mob.position << ' ' << mob.default_pos << ' ';
  if(mob.has_immunities) {
    out << (mob.sex + 3) << ' '
        << format_flag_bits_for_file(static_cast<std::uint64_t>(mob.immune), mob.immune_token) << ' '
        << format_flag_bits_for_file(static_cast<std::uint64_t>(mob.m_immune), mob.m_immune_token)
        << ' '
        << format_flag_bits_for_file(static_cast<std::uint64_t>(mob.susc), mob.susc_token);
  } else {
    out << mob.sex;
  }
  out << '\n';
  if(mob.letter == 'L') {
    write_tilde_string(out, mob.sounds);
    write_tilde_string(out, mob.distant_sounds);
  }
  return ensure_trailing_newline(out.str());
}

std::string format_object_block(const GameObject& obj) {
  std::ostringstream out;
  out << '#' << obj.vnum << '\n';
  write_tilde_string(out, obj.name);
  write_tilde_string(out, obj.short_description);
  write_tilde_string(out, obj.description);
  write_tilde_string(out, obj.action_description);
  out << obj.type_flag << ' '
      << format_flag_bits_for_file(static_cast<std::uint64_t>(obj.extra_flags), obj.extra_flags_token)
      << ' '
      << format_flag_bits_for_file(static_cast<std::uint64_t>(obj.wear_flags), obj.wear_flags_token)
      << '\n';
  out << obj.value[0] << ' ' << obj.value[1] << ' ' << obj.value[2] << ' ' << obj.value[3] << " \n";
  out << obj.weight << ' ' << obj.cost << ' ' << obj.cost_per_day << '\n';
  for(const auto& ed : obj.extras) {
    out << "E\n";
    write_tilde_string(out, ed.keywords);
    write_tilde_string(out, ed.description);
  }
  for(const auto& aff : obj.affects) {
    out << "A\n" << aff.location << ' ' << aff.modifier << '\n';
  }
  if(obj.has_extra_flags2 || obj.extra_flags2 != 0) {
    out << "F\n"
        << format_flag_bits_for_file(static_cast<std::uint64_t>(obj.extra_flags2),
                                     obj.extra_flags2_token)
        << '\n';
  }
  if(obj.has_forbidden) {
    out << "P\n";
    write_tilde_string(out, obj.forbidden_to_char);
    write_tilde_string(out, obj.forbidden_to_room);
  }
  return ensure_trailing_newline(out.str());
}

std::string format_zone_block(const Zone& zone) {
  std::ostringstream out;
  out << '#' << zone.number << '\n';
  write_tilde_string(out, zone.name);
  out << zone.top << ' ' << zone.lifespan << ' ' << zone.reset_mode << '\n';
  for(const auto& cmd : zone.commands) {
    if(cmd.is_comment) {
      out << cmd.raw_line << '\n';
      continue;
    }
    out << cmd.command << ' ' << cmd.if_flag << ' ' << cmd.arg1 << ' ' << cmd.arg2;
    if(cmd.arg3 != -1 || cmd.arg4 != 0) {
      out << ' ' << cmd.arg3;
    }
    if(cmd.arg4 != 0) {
      out << ' ' << cmd.arg4;
    }
    if(!cmd.comment.empty()) {
      if(cmd.comment[0] != ' ' && cmd.comment[0] != '*') {
        out << ' ';
      }
      out << cmd.comment;
    }
    out << '\n';
  }
  out << "S\n";
  return ensure_trailing_newline(out.str());
}

std::string build_preserved_hash_file(
    const FileSourceBlocks& sources,
    const std::unordered_set<std::int64_t>& dirty_vnums,
    const std::unordered_map<std::int64_t, std::string>& formatted_dirty_or_new,
    const std::vector<std::int64_t>& living_vnums_sorted) {
  std::ostringstream body;
  body << sources.prefix;

  std::unordered_set<std::int64_t> living(living_vnums_sorted.begin(), living_vnums_sorted.end());
  std::unordered_set<std::int64_t> emitted;

  for(const std::int64_t vnum : sources.order) {
    if(!living.count(vnum)) {
      continue;
    }
    emitted.insert(vnum);
    const bool dirty = dirty_vnums.count(vnum) != 0;
    if(!dirty) {
      const auto it = sources.blocks.find(vnum);
      if(it != sources.blocks.end()) {
        body << it->second;
        continue;
      }
    }
    const auto fit = formatted_dirty_or_new.find(vnum);
    if(fit != formatted_dirty_or_new.end()) {
      body << fit->second;
    }
  }

  for(const std::int64_t vnum : living_vnums_sorted) {
    if(emitted.count(vnum)) {
      continue;
    }
    const auto fit = formatted_dirty_or_new.find(vnum);
    if(fit != formatted_dirty_or_new.end()) {
      body << fit->second;
    }
  }

  body << sources.trailer;
  return body.str();
}

void refresh_source_blocks_after_save(FileSourceBlocks* sources,
                                      std::unordered_set<std::int64_t>* dirty,
                                      const std::unordered_map<std::int64_t, std::string>& written_blocks,
                                      const std::vector<std::int64_t>& living_vnums_sorted) {
  if(sources == nullptr) {
    return;
  }
  FileSourceBlocks next;
  next.valid = true;
  next.prefix = sources->prefix;
  next.trailer = sources->trailer;

  std::unordered_set<std::int64_t> living(living_vnums_sorted.begin(), living_vnums_sorted.end());
  for(const std::int64_t vnum : sources->order) {
    if(!living.count(vnum)) {
      continue;
    }
    next.order.push_back(vnum);
    if(dirty != nullptr && dirty->count(vnum) && written_blocks.count(vnum)) {
      next.blocks[vnum] = written_blocks.at(vnum);
    } else if(sources->blocks.count(vnum)) {
      next.blocks[vnum] = sources->blocks.at(vnum);
    } else if(written_blocks.count(vnum)) {
      next.blocks[vnum] = written_blocks.at(vnum);
    }
  }
  for(const std::int64_t vnum : living_vnums_sorted) {
    if(std::find(next.order.begin(), next.order.end(), vnum) != next.order.end()) {
      continue;
    }
    next.order.push_back(vnum);
    if(written_blocks.count(vnum)) {
      next.blocks[vnum] = written_blocks.at(vnum);
    }
  }

  *sources = std::move(next);
  if(dirty != nullptr) {
    dirty->clear();
  }
}

}  // namespace eleutheria
