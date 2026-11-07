#include "eleutheria/area_io.hpp"

#include "eleutheria/source_blocks.hpp"
#include "eleutheria/token_io.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace eleutheria {
namespace {

std::string read_file(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  if(!in) {
    throw IoError("Cannot open file: " + path.string());
  }
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

void write_file(const std::filesystem::path& path, const std::string& content) {
  std::ofstream out(path, std::ios::binary);
  if(!out) {
    throw IoError("Cannot write file: " + path.string());
  }
  out << content;
}

std::string lower_ext(std::filesystem::path path) {
  auto ext = path.extension().string();
  std::transform(ext.begin(), ext.end(), ext.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return ext;
}

void warn(std::vector<std::string>* warnings, const std::string& msg) {
  if(warnings) {
    warnings->push_back(msg);
  }
}

constexpr std::int64_t kTunnelFlag = 1LL << 8;
constexpr std::int64_t kSectWaterNoSwim = 7;
constexpr std::int64_t kSectUnderwater = 9;
// Da Server/src/autoenums.hpp: TELE_LOOK=1, TELE_COUNT=2, TELE_RANDOM=4, TELE_SPIN=8
constexpr std::int64_t kTeleCount = 2;

Room parse_one_room_v2(TokenReader& r, std::int64_t vnum, std::vector<std::string>* warnings) {
  Room room;
  room.vnum = vnum;
  room.name = r.read_tilde_string();
  room.description = r.read_tilde_string();

  room.zone_hint = r.read_number();
  room.room_flags = r.read_flag(&room.room_flags_token);
  std::int64_t sector = r.read_number();

  if(sector == -1) {
    room.has_teleport = true;
    room.tele_time = r.read_number();
    room.tele_targ = r.read_number();
    room.tele_mask = r.read_flag(&room.tele_mask_token);
    if(room.tele_mask & kTeleCount) {
      room.tele_cnt = r.read_number();
    }
    sector = r.read_number();
  }
  room.sector_type = sector;

  if(sector == kSectWaterNoSwim || sector == kSectUnderwater) {
    std::int64_t a = 0;
    if(r.try_read_number_same_line(&a)) {
      room.river_speed = a;
      std::int64_t b = 0;
      if(r.try_read_number_same_line(&b)) {
        room.river_dir = b;
      }
      room.has_river = true;
    }
  }

  // TUNNEL: moblim obbligatorio sul server, ma molte aree lo omettono prima di D/S.
  if(room.room_flags & kTunnelFlag) {
    room.has_tunnel_limit = true;
    room.moblim = 1;
    std::int64_t lim = 0;
    if(r.try_read_number_same_line(&lim)) {
      room.moblim = lim < 1 ? 1 : lim;
    } else {
      r.skip_ws();
      const char p = r.peek();
      if(p == '+' || p == '-' || std::isdigit(static_cast<unsigned char>(p))) {
        lim = r.read_number();
        room.moblim = lim < 1 ? 1 : lim;
      } else {
        warn(warnings, "Room #" + std::to_string(vnum) +
                           ": flag TUNNEL senza moblim, uso 1");
      }
    }
  }

  for(;;) {
    r.skip_ws();
    if(r.eof() || r.peek() == '#') {
      break;
    }
    const std::string tag = r.read_word();
    if(tag.empty()) {
      break;
    }
    const char kind = tag[0];
    if(kind == 'D' && tag.size() >= 2 && std::isdigit(static_cast<unsigned char>(tag[1]))) {
      const int dir = std::stoi(tag.substr(1));
      if(dir < 0 || dir >= kMaxExits) {
        warn(warnings, "Room #" + std::to_string(vnum) + ": bad exit " + tag);
        continue;
      }
      Exit& ex = room.exits[static_cast<std::size_t>(dir)];
      ex.present = true;
      ex.general_description = r.read_tilde_string();
      ex.keyword = r.read_tilde_string();
      ex.exit_info = r.read_flag(&ex.exit_info_token);
      ex.key = r.read_number();
      ex.to_room = r.read_number();
      std::int64_t trailing = 0;
      (void)r.try_read_number_same_line(&trailing);
    } else if(kind == 'E') {
      ExtraDesc ed;
      ed.keywords = r.read_tilde_string();
      ed.description = r.read_tilde_string();
      room.extras.push_back(std::move(ed));
    } else if(kind == 'L') {
      room.has_bright = true;
      room.bright_night = r.read_tilde_string();
      room.bright_day = r.read_tilde_string();
    } else if(kind == 'S') {
      break;
    } else if(kind == 'C') {
      r.skip_line();
    } else {
      warn(warnings, "Room #" + std::to_string(vnum) + ": unknown tag '" + tag + "'");
      r.skip_line();
    }
  }
  return room;
}

Mobile parse_one_mobile(TokenReader& r, std::int64_t vnum, std::vector<std::string>* warnings) {
  Mobile mob;
  mob.vnum = vnum;
  mob.name = r.read_tilde_string();
  mob.short_descr = r.read_tilde_string();
  mob.long_descr = r.read_tilde_string();
  mob.description = r.read_tilde_string();

  mob.act = r.read_flag(&mob.act_token);
  mob.affected_by = r.read_flag(&mob.affected_by_token);
  mob.alignment = r.read_number();

  r.skip_ws();
  mob.letter = r.get();
  if(mob.letter == 'S') {
    r.skip_ws();
    if(r.peek() == '\n' || r.peek() == '\r') {
      r.skip_line();
    }
    mob.level = r.read_number();
    mob.thac0 = r.read_number();
    mob.armor = r.read_number();
    r.read_dice(&mob.hit_dice_n, &mob.hit_dice_s, &mob.hit_dice_b);
    r.read_dice(&mob.dam_dice_n, &mob.dam_dice_s, &mob.dam_bonus);

    const std::int64_t first = r.read_number();
    if(first == -1) {
      mob.gold_exp_race_form = true;
      mob.gold = r.read_number();
      mob.exp = r.read_number();
      mob.race = r.read_number();
      std::int64_t sp = 0;
      if(r.try_read_number_same_line(&sp)) {
        mob.has_spellpower = true;
        mob.spellpower = sp;
      }
    } else {
      mob.gold_exp_race_form = false;
      mob.gold = first;
      mob.exp = r.read_number();
      std::int64_t sp = 0;
      if(r.try_read_number_same_line(&sp)) {
        mob.has_spellpower = true;
        mob.spellpower = sp;
      }
    }
    mob.position = r.read_number();
    mob.default_pos = r.read_number();
    const std::int64_t sexish = r.read_number();
    if(sexish < 3) {
      mob.sex = sexish;
    } else if(sexish < 6) {
      mob.sex = sexish - 3;
      mob.has_immunities = true;
      mob.immune = r.read_flag(&mob.immune_token);
      mob.m_immune = r.read_flag(&mob.m_immune_token);
      mob.susc = r.read_flag(&mob.susc_token);
    } else {
      mob.sex = 0;
      warn(warnings, "Mob #" + std::to_string(vnum) + ": unusual sex field " + std::to_string(sexish));
    }
  } else if(mob.letter == 'A' || mob.letter == 'N' || mob.letter == 'B' || mob.letter == 'L') {
    if(mob.letter == 'A' || mob.letter == 'B' || mob.letter == 'L') {
      mob.mult_att = static_cast<double>(r.read_number());
    }
    r.skip_ws();
    if(r.peek() == '\n' || r.peek() == '\r') {
      r.skip_line();
    }
    mob.level = r.read_number();
    mob.thac0 = r.read_number();
    mob.armor = r.read_number();
    mob.hit_bonus = r.read_number();
    r.read_dice(&mob.dam_dice_n, &mob.dam_dice_s, &mob.dam_bonus);

    const std::int64_t first = r.read_number();
    if(first == -1) {
      mob.gold_exp_race_form = true;
      mob.gold = r.read_number();
      mob.exp = r.read_number();
      mob.race = r.read_number();
      std::int64_t sp = 0;
      if(r.try_read_number_same_line(&sp)) {
        mob.has_spellpower = true;
        mob.spellpower = sp;
      }
    } else {
      mob.gold_exp_race_form = false;
      mob.gold = first;
      mob.exp = r.read_number();
      std::int64_t sp = 0;
      if(r.try_read_number_same_line(&sp)) {
        mob.has_spellpower = true;
        mob.spellpower = sp;
      }
    }
    mob.position = r.read_number();
    mob.default_pos = r.read_number();
    const std::int64_t sexish = r.read_number();
    if(sexish < 3) {
      mob.sex = sexish;
    } else if(sexish < 6) {
      mob.sex = sexish - 3;
      mob.has_immunities = true;
      mob.immune = r.read_flag(&mob.immune_token);
      mob.m_immune = r.read_flag(&mob.m_immune_token);
      mob.susc = r.read_flag(&mob.susc_token);
    }

    if(mob.letter == 'L') {
      mob.sounds = r.read_tilde_string();
      mob.distant_sounds = r.read_tilde_string();
    }
  } else {
    warn(warnings, "Mob #" + std::to_string(vnum) + ": unsupported letter '" +
                        std::string(1, mob.letter) + "', skipping rest heuristically");
    // Skip until next #
    while(!r.eof() && r.peek() != '#') {
      r.skip_line();
      r.skip_ws();
    }
  }
  return mob;
}

GameObject parse_one_object(TokenReader& r, std::int64_t vnum, std::vector<std::string>* warnings) {
  GameObject obj;
  obj.vnum = vnum;
  obj.name = r.read_tilde_string();
  obj.short_description = r.read_tilde_string();
  obj.description = r.read_tilde_string();
  obj.action_description = r.read_tilde_string();

  obj.type_flag = r.read_number();
  obj.extra_flags = r.read_flag(&obj.extra_flags_token);
  obj.wear_flags = r.read_flag(&obj.wear_flags_token);
  obj.value[0] = r.read_number();
  obj.value[1] = r.read_number();
  obj.value[2] = r.read_number();
  obj.value[3] = r.read_number();
  obj.weight = r.read_number();
  obj.cost = r.read_number();
  obj.cost_per_day = r.read_number();

  for(;;) {
    r.skip_ws();
    if(r.eof() || r.peek() == '#') {
      break;
    }
    const std::string tag = r.read_word();
    if(tag.empty()) {
      break;
    }
    if(tag[0] == 'E') {
      ExtraDesc ed;
      ed.keywords = r.read_tilde_string();
      ed.description = r.read_tilde_string();
      obj.extras.push_back(std::move(ed));
    } else if(tag[0] == 'A') {
      ObjAffect aff;
      aff.location = static_cast<int>(r.read_number());
      aff.modifier = static_cast<int>(r.read_number());
      obj.affects.push_back(aff);
    } else if(tag[0] == 'F') {
      obj.has_extra_flags2 = true;
      obj.extra_flags2 = r.read_flag(&obj.extra_flags2_token);
    } else if(tag[0] == 'P') {
      obj.has_forbidden = true;
      obj.forbidden_to_char = r.read_tilde_string();
      obj.forbidden_to_room = r.read_tilde_string();
    } else {
      // Likely start of next entity without '#': put back is hard; warn
      warn(warnings, "Object #" + std::to_string(vnum) + ": stop at tag '" + tag + "'");
      break;
    }
  }
  return obj;
}

}  // namespace

void parse_rooms_file(const std::string& text, std::map<std::int64_t, Room>* out,
                      std::vector<std::string>* warnings) {
  TokenReader r(text);
  while(!r.eof()) {
    r.skip_ws();
    if(r.eof()) {
      break;
    }
    if(r.peek() == '$') {
      break;
    }
    if(r.peek() != '#') {
      r.skip_line();
      continue;
    }
    r.get();  // #
    std::int64_t vnum = -1;
    try {
      vnum = r.read_number();
      if(vnum < 0) {
        break;
      }
      r.skip_ws();
      (*out)[vnum] = parse_one_room_v2(r, vnum, warnings);
    } catch(const IoError& e) {
      warn(warnings, std::string("Room #") + std::to_string(vnum) + ": " + e.what());
      // Riparti dal prossimo #vnum
      while(!r.eof() && r.peek() != '#') {
        r.skip_line();
        r.skip_ws();
      }
    }
  }
}

void parse_mobiles_file(const std::string& text, std::map<std::int64_t, Mobile>* out,
                        std::vector<std::string>* warnings) {
  TokenReader r(text);
  while(!r.eof()) {
    r.skip_ws();
    if(r.eof() || r.peek() == '$') {
      break;
    }
    if(r.peek() != '#') {
      r.skip_line();
      continue;
    }
    r.get();
    const std::int64_t vnum = r.read_number();
    if(vnum < 0) {
      break;
    }
    r.skip_ws();
    try {
      (*out)[vnum] = parse_one_mobile(r, vnum, warnings);
    } catch(const IoError& e) {
      warn(warnings, std::string("Mob #") + std::to_string(vnum) + ": " + e.what());
      while(!r.eof() && r.peek() != '#') {
        r.skip_line();
        r.skip_ws();
      }
    }
  }
}

void parse_objects_file(const std::string& text, std::map<std::int64_t, GameObject>* out,
                        std::vector<std::string>* warnings) {
  TokenReader r(text);
  while(!r.eof()) {
    r.skip_ws();
    if(r.eof() || r.peek() == '$') {
      break;
    }
    if(r.peek() != '#') {
      r.skip_line();
      continue;
    }
    r.get();
    const std::int64_t vnum = r.read_number();
    if(vnum < 0) {
      break;
    }
    r.skip_ws();  // salta il newline dopo #vnum, senza mangiarlo nei keywords
    try {
      (*out)[vnum] = parse_one_object(r, vnum, warnings);
    } catch(const IoError& e) {
      warn(warnings, std::string("Obj #") + std::to_string(vnum) + ": " + e.what());
      while(!r.eof() && r.peek() != '#') {
        r.skip_line();
        r.skip_ws();
      }
    }
  }
}

void parse_zones_file(const std::string& text, std::map<int, Zone>* out,
                      std::vector<std::string>* warnings) {
  TokenReader r(text);
  while(!r.eof()) {
    r.skip_ws();
    if(r.eof()) {
      break;
    }
    if(r.peek() != '#') {
      r.skip_line();
      continue;
    }
    r.get();
    Zone zone;
    zone.number = static_cast<int>(r.read_number());
    zone.name = r.read_tilde_string();
    if(!zone.name.empty() && zone.name[0] == '$') {
      break;
    }
    zone.top = r.read_number();
    zone.lifespan = r.read_number();
    zone.reset_mode = r.read_number();

    for(;;) {
      r.skip_ws();
      if(r.eof()) {
        break;
      }
      // Read one line for reset commands to preserve comments
      std::string line;
      while(!r.eof() && r.peek() != '\n' && r.peek() != '\r') {
        line.push_back(r.get());
      }
      if(!r.eof()) {
        const char c = r.get();
        if(c == '\r' && r.peek() == '\n') {
          r.get();
        }
      }
      if(line.empty()) {
        continue;
      }
      // Trim leading spaces
      std::size_t i = 0;
      while(i < line.size() && (line[i] == ' ' || line[i] == '\t')) {
        ++i;
      }
      if(i >= line.size()) {
        continue;
      }
      const char cmd = line[i];
      if(cmd == 'S') {
        break;
      }
      if(cmd == '*' || cmd == ';') {
        ResetCommand rc;
        rc.is_comment = true;
        rc.command = cmd;
        rc.raw_line = line;
        zone.commands.push_back(std::move(rc));
        continue;
      }
      if(std::string("HFMCOGEPD").find(cmd) == std::string::npos) {
        warn(warnings, "Zone #" + std::to_string(zone.number) + ": skip '" + line + "'");
        continue;
      }
      ResetCommand rc;
      rc.command = cmd;
      TokenReader lr(line.substr(i + 1));
      try {
        rc.if_flag = static_cast<int>(lr.read_number());
        rc.arg1 = lr.read_number();
        rc.arg2 = lr.read_number();
        std::int64_t a3 = -1;
        if(lr.try_read_number_same_line(&a3) || (!lr.eof() && (lr.peek() == '-' ||
                                                               std::isdigit(static_cast<unsigned char>(lr.peek()))))) {
          try {
            if(a3 == -1 && !lr.eof()) {
              // try_read may have failed; read
            }
          } catch(...) {
          }
        }
        // Re-parse args more simply
        lr = TokenReader(line.substr(i + 1));
        rc.if_flag = static_cast<int>(lr.read_number());
        rc.arg1 = lr.read_number();
        rc.arg2 = lr.read_number();
        lr.skip_ws();
        if(!lr.eof() && (lr.peek() == '-' || std::isdigit(static_cast<unsigned char>(lr.peek())))) {
          rc.arg3 = lr.read_number();
        }
        lr.skip_ws();
        if(!lr.eof() && (lr.peek() == '-' || std::isdigit(static_cast<unsigned char>(lr.peek())))) {
          rc.arg4 = lr.read_number();
        }
        lr.skip_ws();
        // Rest is comment
        if(!lr.eof()) {
          rc.comment = lr.text().substr(lr.pos());
        }
      } catch(const IoError& e) {
        warn(warnings, "Zone #" + std::to_string(zone.number) + " cmd: " + e.what());
        rc.raw_line = line;
        rc.is_comment = true;
      }
      zone.commands.push_back(std::move(rc));
    }
    (*out)[zone.number] = std::move(zone);
  }
}

void parse_shops_file(const std::string& text, std::map<std::int64_t, Shop>* out,
                      std::vector<std::string>* warnings) {
  TokenReader r(text);
  while(!r.eof()) {
    r.skip_ws();
    if(r.eof() || r.peek() == '$') {
      break;
    }
    if(r.peek() != '#') {
      r.skip_line();
      continue;
    }
    r.get();
    Shop shop;
    // Format: #keeper~  OR #keeper then newline
    shop.keeper = r.read_number();
    r.skip_ws();
    if(r.peek() == '~') {
      r.get();
    }
    for(int i = 0; i < kMaxShopProduce; ++i) {
      shop.producing[static_cast<std::size_t>(i)] = r.read_number();
    }
    // profits as floats
    r.skip_ws();
    {
      std::string tok = r.read_word();
      shop.profit_buy = std::strtod(tok.c_str(), nullptr);
      tok = r.read_word();
      shop.profit_sell = std::strtod(tok.c_str(), nullptr);
    }
    for(int i = 0; i < kMaxShopType; ++i) {
      shop.type[static_cast<std::size_t>(i)] = r.read_number();
    }
    for(int i = 0; i < kMaxShopMsg; ++i) {
      shop.messages[static_cast<std::size_t>(i)] = r.read_tilde_string();
    }
    shop.temper1 = r.read_number();
    shop.temper2 = r.read_number();
    shop.with_who = r.read_number();
    shop.in_room = r.read_number();
    shop.open1 = r.read_number();
    shop.close1 = r.read_number();
    shop.open2 = r.read_number();
    shop.close2 = r.read_number();
    (*out)[shop.keeper] = std::move(shop);
  }
  (void)warnings;
}

AreaMeta parse_ele_file(const std::string& text) {
  AreaMeta meta;
  std::istringstream in(text);
  std::string line;
  while(std::getline(in, line)) {
    if(!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    const auto eq = line.find('=');
    if(eq == std::string::npos) {
      continue;
    }
    const std::string key = line.substr(0, eq);
    const std::string val = line.substr(eq + 1);
    if(key == "Nome") {
      meta.name = val;
    } else if(key == "Autore") {
      meta.author = val;
    } else if(key == "MudId") {
      meta.mud_id = val;
    }
  }
  return meta;
}

std::string serialize_rooms(const std::map<std::int64_t, Room>& rooms) {
  std::ostringstream out;
  for(const auto& [vnum, room] : rooms) {
    out << '#' << vnum << '\n';
    write_tilde_string(out, room.name);
    write_tilde_string(out, room.description);
    out << room.zone_hint << ' '
        << format_flag_bits_for_file(static_cast<std::uint64_t>(room.room_flags),
                                     room.room_flags_token)
        << ' ';
    if(room.has_teleport) {
      out << "-1 " << room.tele_time << ' ' << room.tele_targ << ' '
          << format_flag_bits_for_file(static_cast<std::uint64_t>(room.tele_mask),
                                       room.tele_mask_token);
      if(room.tele_mask & kTeleCount) {
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
      out << format_flag_bits_for_file(static_cast<std::uint64_t>(ex.exit_info),
                                       ex.exit_info_token)
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
  }
  out << "$~\n";
  return out.str();
}

std::string serialize_mobiles(const std::map<std::int64_t, Mobile>& mobiles) {
  std::ostringstream out;
  for(const auto& [vnum, mob] : mobiles) {
    out << '#' << vnum << '\n';
    write_tilde_string(out, mob.name);
    write_tilde_string(out, mob.short_descr);
    write_tilde_string(out, mob.long_descr);
    write_tilde_string(out, mob.description);
    out << format_flag_bits_for_file(static_cast<std::uint64_t>(mob.act), mob.act_token) << ' '
        << format_flag_bits_for_file(static_cast<std::uint64_t>(mob.affected_by),
                                     mob.affected_by_token)
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
          << format_flag_bits_for_file(static_cast<std::uint64_t>(mob.immune), mob.immune_token)
          << ' '
          << format_flag_bits_for_file(static_cast<std::uint64_t>(mob.m_immune),
                                       mob.m_immune_token)
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
  }
  out << "$~\n";
  return out.str();
}

std::string serialize_objects(const std::map<std::int64_t, GameObject>& objects) {
  std::ostringstream out;
  for(const auto& [vnum, obj] : objects) {
    out << '#' << vnum << '\n';
    write_tilde_string(out, obj.name);
    write_tilde_string(out, obj.short_description);
    write_tilde_string(out, obj.description);
    write_tilde_string(out, obj.action_description);
    out << obj.type_flag << ' '
        << format_flag_bits_for_file(static_cast<std::uint64_t>(obj.extra_flags),
                                     obj.extra_flags_token)
        << ' '
        << format_flag_bits_for_file(static_cast<std::uint64_t>(obj.wear_flags),
                                     obj.wear_flags_token)
        << '\n';
    out << obj.value[0] << ' ' << obj.value[1] << ' ' << obj.value[2] << ' ' << obj.value[3]
        << " \n";
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
  }
  out << "$~\n";
  return out.str();
}

std::string serialize_zones(const std::map<int, Zone>& zones) {
  std::ostringstream out;
  for(const auto& [num, zone] : zones) {
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
  }
  out << "#999\n$~\n";
  return out.str();
}

std::string serialize_shops(const std::map<std::int64_t, Shop>& shops) {
  std::ostringstream out;
  for(const auto& [keeper, shop] : shops) {
    out << '#' << shop.keeper << "~\n";
    for(int i = 0; i < kMaxShopProduce; ++i) {
      out << shop.producing[static_cast<std::size_t>(i)] << '\n';
    }
    out << shop.profit_buy << '\n' << shop.profit_sell << '\n';
    for(int i = 0; i < kMaxShopType; ++i) {
      out << shop.type[static_cast<std::size_t>(i)] << '\n';
    }
    for(int i = 0; i < kMaxShopMsg; ++i) {
      write_tilde_string(out, shop.messages[static_cast<std::size_t>(i)]);
    }
    out << shop.temper1 << '\n'
        << shop.temper2 << '\n'
        << shop.with_who << '\n'
        << shop.in_room << '\n'
        << shop.open1 << '\n'
        << shop.close1 << '\n'
        << shop.open2 << '\n'
        << shop.close2 << '\n';
  }
  out << "$~\n";
  return out.str();
}

std::string serialize_ele(const AreaMeta& meta) {
  std::ostringstream out;
  out << "[Area]\n";
  out << "Nome=" << meta.name << '\n';
  out << "Autore=" << meta.author << '\n';
  out << "MudId=" << meta.mud_id << '\n';
  return out.str();
}

bool area_base_has_world_files(const std::filesystem::path& base) {
  const auto stem = base.string();
  return std::filesystem::exists(stem + ".wld") || std::filesystem::exists(stem + ".mob") ||
         std::filesystem::exists(stem + ".obj") || std::filesystem::exists(stem + ".zon") ||
         std::filesystem::exists(stem + ".ele") || std::filesystem::exists(stem + ".shp");
}

namespace {

bool looks_like_area_base(const std::filesystem::path& base) {
  return area_base_has_world_files(base);
}

std::filesystem::path find_base_in_directory(const std::filesystem::path& dir) {
  // 1) mudroot → mudroot/lib/myst.*
  const auto lib_myst = dir / "lib" / "myst";
  if(looks_like_area_base(lib_myst)) {
    return lib_myst;
  }
  // 2) lib/ o cartella con myst.*
  const auto myst = dir / "myst";
  if(looks_like_area_base(myst)) {
    return myst;
  }
  // 3) layout Aree: dir/area/area.*
  const auto area_area = dir / "area" / "area";
  if(looks_like_area_base(area_area)) {
    return area_area;
  }
  // 4) file omonimo della cartella (B1/B1.wld, castelli/castelli.wld)
  const auto named = dir / dir.filename();
  if(looks_like_area_base(named)) {
    return named;
  }
  // 5) primo .wld / .ele / .zon trovato (esclusi backup)
  std::filesystem::path best;
  for(const auto& entry : std::filesystem::directory_iterator(dir)) {
    if(!entry.is_regular_file()) {
      continue;
    }
    const auto ext = lower_ext(entry.path());
    if(ext == ".wld" || ext == ".ele" || ext == ".zon" || ext == ".mob" || ext == ".obj") {
      const auto candidate = entry.path().parent_path() / entry.path().stem();
      if(looks_like_area_base(candidate)) {
        if(ext == ".wld" || best.empty()) {
          best = candidate;
          if(ext == ".wld") {
            break;
          }
        }
      }
    }
  }
  if(!best.empty()) {
    return best;
  }
  return named;
}

}  // namespace

std::filesystem::path area_base_from_path(const std::filesystem::path& path) {
  std::error_code ec;
  const auto canonical = std::filesystem::weakly_canonical(path, ec);
  const auto use = ec ? path : canonical;

  if(std::filesystem::is_directory(use)) {
    return find_base_in_directory(use);
  }
  // File: se e' dentro lib/myst.wld → base myst; se utente passa mudroot/lib → gia' dir
  if(use.has_parent_path() && use.stem() == "myst") {
    return use.parent_path() / "myst";
  }
  return use.parent_path() / use.stem();
}

LoadResult load_area(const std::filesystem::path& path) {
  LoadResult result;
  // Accetta: file .wld, cartella, oppure base senza estensione (…/B1 se esiste B1.wld).
  const bool exists_as_is = std::filesystem::exists(path);
  const bool exists_as_base = area_base_has_world_files(path);
  if(!exists_as_is && !exists_as_base) {
    throw IoError("Percorso inesistente: " + path.string());
  }
  const auto base = exists_as_base && !std::filesystem::is_directory(path)
                        ? path
                        : area_base_from_path(path);
  result.area.base_path = base.string();
  result.area.directory = base.parent_path().string();

  if(!area_base_has_world_files(base)) {
    throw IoError(
        "Nessun file area trovato in:\n" + path.string() +
        "\n(risolto come base: " + base.string() +
        ")\n\nApri una cartella area (es. B1/ con B1.wld), "
        "oppure mudroot o mudroot/lib (myst.wld), "
        "oppure un file .ele/.wld.");
  }

  const auto load_if = [&](const char* ext, auto loader) {
    const auto file = std::filesystem::path(base.string() + ext);
    if(std::filesystem::exists(file)) {
      try {
        loader(read_file(file));
      } catch(const std::exception& e) {
        result.warnings.push_back(std::string(ext) + ": " + e.what());
      }
    }
  };

  load_if(".ele", [&](const std::string& t) { result.area.meta = parse_ele_file(t); });
  if(result.area.meta.name.empty()) {
    result.area.meta.name = base.filename().string();
  }
  if(result.area.meta.mud_id.empty()) {
    result.area.meta.mud_id = "nebbie";
  }

  load_if(".wld", [&](const std::string& t) {
    parse_rooms_file(t, &result.area.rooms, &result.warnings);
  });
  load_if(".mob", [&](const std::string& t) {
    parse_mobiles_file(t, &result.area.mobiles, &result.warnings);
  });
  load_if(".obj", [&](const std::string& t) {
    parse_objects_file(t, &result.area.objects, &result.warnings);
  });
  load_if(".zon", [&](const std::string& t) {
    parse_zones_file(t, &result.area.zones, &result.warnings);
  });
  load_if(".shp", [&](const std::string& t) {
    parse_shops_file(t, &result.area.shops, &result.warnings);
  });

  if(const auto blocks = capture_vnum_hash_file(base.string() + ".wld")) {
    result.area.wld_sources = *blocks;
  }
  if(const auto blocks = capture_vnum_hash_file(base.string() + ".mob")) {
    result.area.mob_sources = *blocks;
  }
  if(const auto blocks = capture_vnum_hash_file(base.string() + ".obj")) {
    result.area.obj_sources = *blocks;
  }
  if(const auto blocks = capture_vnum_hash_file(base.string() + ".zon")) {
    result.area.zon_sources = *blocks;
  }

  if(result.area.rooms.empty() && result.area.mobiles.empty() && result.area.objects.empty() &&
     result.area.zones.empty()) {
    result.warnings.push_back(
        "File trovati ma nessun contenuto caricato (parser vuoto o solo errori). Base: " +
        base.string());
  }

  return result;
}

namespace {

template <typename Map, typename FormatFn>
void save_hash_file_preserved(FileSourceBlocks* sources,
                              std::unordered_set<std::int64_t>* dirty,
                              const Map& entities,
                              const std::filesystem::path& path,
                              FormatFn format_one,
                              const std::string& full_fallback) {
  if(sources == nullptr || !sources->valid) {
    write_file(path.string(), full_fallback);
    return;
  }

  std::vector<std::int64_t> living;
  living.reserve(entities.size());
  for(const auto& [vnum, _] : entities) {
    living.push_back(vnum);
  }
  std::sort(living.begin(), living.end());

  std::unordered_map<std::int64_t, std::string> formatted;
  for(const std::int64_t vnum : living) {
    const bool needs_format =
        dirty->count(vnum) != 0 || sources->blocks.find(vnum) == sources->blocks.end();
    if(needs_format) {
      formatted[vnum] = format_one(entities.at(vnum));
    }
  }

  // Anche le entita' solo in sources.order ma dirty gia' coperte; nuove pure.
  write_file(path.string(),
             build_preserved_hash_file(*sources, *dirty, formatted, living));
  refresh_source_blocks_after_save(sources, dirty, formatted, living);
}

}  // namespace

void save_area(Area& area, const std::filesystem::path& base_path) {
  write_file(base_path.string() + ".ele", serialize_ele(area.meta));

  save_hash_file_preserved(&area.wld_sources, &area.dirty_rooms, area.rooms,
                           base_path.string() + ".wld",
                           [](const Room& r) { return format_room_block(r); },
                           serialize_rooms(area.rooms));

  save_hash_file_preserved(&area.mob_sources, &area.dirty_mobs, area.mobiles,
                           base_path.string() + ".mob",
                           [](const Mobile& m) { return format_mobile_block(m); },
                           serialize_mobiles(area.mobiles));

  save_hash_file_preserved(&area.obj_sources, &area.dirty_objects, area.objects,
                           base_path.string() + ".obj",
                           [](const GameObject& o) { return format_object_block(o); },
                           serialize_objects(area.objects));

  {
    std::unordered_map<std::int64_t, Zone> zones_as_int64;
    for(const auto& [num, zone] : area.zones) {
      zones_as_int64[num] = zone;
    }
    save_hash_file_preserved(&area.zon_sources, &area.dirty_zones, zones_as_int64,
                             base_path.string() + ".zon",
                             [](const Zone& z) { return format_zone_block(z); },
                             serialize_zones(area.zones));
  }

  if(!area.shops.empty()) {
    write_file(base_path.string() + ".shp", serialize_shops(area.shops));
  }
}

}  // namespace eleutheria
