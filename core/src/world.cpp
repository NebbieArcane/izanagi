#include "eleutheria/world.hpp"

#include "eleutheria/workspace_index.hpp"

#include <cctype>

namespace eleutheria {
namespace {

std::string describe_external_ref(const char* what, std::int64_t vnum,
                                  const IndexedEntity* found) {
  if(found != nullptr) {
    return std::string(what) + " " + std::to_string(vnum) + " in altra area workspace (" +
           found->area_name + ")";
  }
  return std::string(what) + " " + std::to_string(vnum) +
         " non nel workspace (placeholder accettato)";
}

}  // namespace

std::vector<ValidationIssue> validate_area(const Area& area, const WorkspaceEntityIndex* index) {
  std::vector<ValidationIssue> issues;

  auto add = [&](ValidationIssue::Severity sev, const std::string& kind, std::int64_t vnum,
                 const std::string& msg) {
    issues.push_back(ValidationIssue{sev, kind, vnum, msg});
  };

  for(const auto& [vnum, room] : area.rooms) {
    if(room.name.empty()) {
      add(ValidationIssue::Severity::Warning, "room", vnum, "Nome stanza vuoto");
    }
    for(int d = 0; d < kMaxExits; ++d) {
      const Exit& ex = room.exits[static_cast<std::size_t>(d)];
      if(!ex.present) {
        continue;
      }
      if(ex.to_room >= 0 && area.rooms.find(ex.to_room) == area.rooms.end()) {
        const IndexedEntity* found = index ? index->find_room(ex.to_room) : nullptr;
        add(ValidationIssue::Severity::Info, "room", vnum,
            "Uscita " + std::to_string(d) + ": " +
                describe_external_ref("stanza", ex.to_room, found));
      }
    }
  }

  for(const auto& [num, zone] : area.zones) {
    for(const auto& cmd : zone.commands) {
      if(cmd.is_comment) {
        continue;
      }
      if(cmd.command == 'M' && area.mobiles.find(cmd.arg1) == area.mobiles.end()) {
        const IndexedEntity* found = index ? index->find_mob(cmd.arg1) : nullptr;
        add(ValidationIssue::Severity::Info, "zone", num,
            std::string("Reset M: ") + describe_external_ref("mob", cmd.arg1, found));
      }
      if((cmd.command == 'O' || cmd.command == 'G' || cmd.command == 'E' || cmd.command == 'P') &&
         area.objects.find(cmd.arg1) == area.objects.end()) {
        const IndexedEntity* found = index ? index->find_obj(cmd.arg1) : nullptr;
        add(ValidationIssue::Severity::Info, "zone", num,
            std::string("Reset ") + cmd.command + ": " +
                describe_external_ref("oggetto", cmd.arg1, found));
      }
      if((cmd.command == 'M' || cmd.command == 'O') && cmd.arg3 >= 0 &&
         area.rooms.find(cmd.arg3) == area.rooms.end()) {
        const IndexedEntity* found = index ? index->find_room(cmd.arg3) : nullptr;
        add(ValidationIssue::Severity::Info, "zone", num,
            std::string("Reset ") + cmd.command + ": " +
                describe_external_ref("stanza", cmd.arg3, found));
      }
      if(cmd.command == 'D' && area.rooms.find(cmd.arg1) == area.rooms.end()) {
        const IndexedEntity* found = index ? index->find_room(cmd.arg1) : nullptr;
        add(ValidationIssue::Severity::Info, "zone", num,
            std::string("Reset D: ") + describe_external_ref("stanza", cmd.arg1, found));
      }
    }
  }

  for(const auto& [keeper, shop] : area.shops) {
    if(area.mobiles.find(shop.keeper) == area.mobiles.end()) {
      const IndexedEntity* found = index ? index->find_mob(shop.keeper) : nullptr;
      add(ValidationIssue::Severity::Info, "shop", shop.keeper,
          describe_external_ref("shopkeeper mob", shop.keeper, found));
    }
    for(const auto produce : shop.producing) {
      if(produce < 0) {
        continue;
      }
      if(area.objects.find(produce) == area.objects.end()) {
        const IndexedEntity* found = index ? index->find_obj(produce) : nullptr;
        add(ValidationIssue::Severity::Info, "shop", shop.keeper,
            describe_external_ref("shop produce oggetto", produce, found));
      }
    }
  }

  for(const auto& sa : area.specials) {
    if(sa.is_comment) {
      continue;
    }
    if(sa.kind == 'M' && area.mobiles.find(sa.vnum) == area.mobiles.end()) {
      const IndexedEntity* found = index ? index->find_mob(sa.vnum) : nullptr;
      add(ValidationIssue::Severity::Info, "spe", sa.vnum,
          describe_external_ref("special mob", sa.vnum, found));
    } else if(sa.kind == 'O' && area.objects.find(sa.vnum) == area.objects.end()) {
      const IndexedEntity* found = index ? index->find_obj(sa.vnum) : nullptr;
      add(ValidationIssue::Severity::Info, "spe", sa.vnum,
          describe_external_ref("special oggetto", sa.vnum, found));
    } else if(sa.kind == 'R' && area.rooms.find(sa.vnum) == area.rooms.end()) {
      const IndexedEntity* found = index ? index->find_room(sa.vnum) : nullptr;
      add(ValidationIssue::Severity::Info, "spe", sa.vnum,
          describe_external_ref("special stanza", sa.vnum, found));
    }
  }

  return issues;
}

std::string strip_mud_colors(std::string_view text) {
  std::string out;
  out.reserve(text.size());
  for(std::size_t i = 0; i < text.size(); ++i) {
    if(text[i] == '$' && i + 5 < text.size() && text[i + 1] == 'c') {
      // $cNNNN
      bool digits = true;
      for(int k = 0; k < 4; ++k) {
        if(!std::isdigit(static_cast<unsigned char>(text[i + 2 + static_cast<std::size_t>(k)]))) {
          digits = false;
          break;
        }
      }
      if(digits) {
        i += 5;
        continue;
      }
    }
    if(text[i] == '\n' || text[i] == '\r') {
      out.push_back(' ');
      continue;
    }
    out.push_back(text[i]);
  }
  // compact spaces
  std::string compact;
  bool space = false;
  for(char c : out) {
    if(c == ' ' || c == '\t') {
      if(!space && !compact.empty()) {
        compact.push_back(' ');
        space = true;
      }
      continue;
    }
    space = false;
    compact.push_back(c);
  }
  while(!compact.empty() && compact.back() == ' ') {
    compact.pop_back();
  }
  return compact;
}

namespace {

std::string hash_vnum(std::int64_t vnum) {
  return "#" + std::to_string(vnum);
}

}  // namespace

std::string room_list_label(const Room& room) {
  return hash_vnum(room.vnum) + " " + strip_mud_colors(room.name);
}

std::string mob_list_label(const Mobile& mob) {
  const std::string& raw = mob.short_descr.empty() ? mob.name : mob.short_descr;
  return hash_vnum(mob.vnum) + " " + strip_mud_colors(raw);
}

std::string obj_list_label(const GameObject& obj) {
  const std::string& raw =
      obj.short_description.empty() ? obj.name : obj.short_description;
  return hash_vnum(obj.vnum) + " " + strip_mud_colors(raw);
}

std::string zone_list_label(const Zone& zone) {
  return hash_vnum(zone.number) + " " + strip_mud_colors(zone.name) + " (top " +
         std::to_string(zone.top) + ")";
}

Zone* zone_for_room(Area& area, std::int64_t room_vnum) {
  for(auto& [_, zone] : area.zones) {
    for(const auto& cmd : zone.commands) {
      if(!cmd.is_comment && (cmd.command == 'M' || cmd.command == 'O') &&
         cmd.arg3 == room_vnum) {
        return &zone;
      }
    }
  }
  for(auto& [_, zone] : area.zones) {
    if(room_vnum <= zone.top) {
      return &zone;
    }
  }
  if(!area.zones.empty()) {
    return &area.zones.begin()->second;
  }
  return nullptr;
}

const Zone* zone_for_room(const Area& area, std::int64_t room_vnum) {
  return zone_for_room(const_cast<Area&>(area), room_vnum);
}

namespace {

void keep_token_if_same(std::int64_t edited_value, std::string& edited_token,
                        std::int64_t previous_value, const std::string& previous_token) {
  if(edited_value == previous_value && !previous_token.empty()) {
    edited_token = previous_token;
  }
}

}  // namespace

void preserve_flag_tokens(Room& edited, const Room& previous) {
  keep_token_if_same(edited.room_flags, edited.room_flags_token, previous.room_flags,
                     previous.room_flags_token);
  keep_token_if_same(edited.tele_mask, edited.tele_mask_token, previous.tele_mask,
                     previous.tele_mask_token);
  for(std::size_t i = 0; i < edited.exits.size(); ++i) {
    if(!edited.exits[i].present || !previous.exits[i].present) {
      continue;
    }
    keep_token_if_same(edited.exits[i].exit_info, edited.exits[i].exit_info_token,
                       previous.exits[i].exit_info, previous.exits[i].exit_info_token);
  }
}

void preserve_flag_tokens(Mobile& edited, const Mobile& previous) {
  keep_token_if_same(edited.act, edited.act_token, previous.act, previous.act_token);
  keep_token_if_same(edited.affected_by, edited.affected_by_token, previous.affected_by,
                     previous.affected_by_token);
  keep_token_if_same(edited.immune, edited.immune_token, previous.immune, previous.immune_token);
  keep_token_if_same(edited.m_immune, edited.m_immune_token, previous.m_immune,
                     previous.m_immune_token);
  keep_token_if_same(edited.susc, edited.susc_token, previous.susc, previous.susc_token);
}

void preserve_flag_tokens(GameObject& edited, const GameObject& previous) {
  keep_token_if_same(edited.extra_flags, edited.extra_flags_token, previous.extra_flags,
                     previous.extra_flags_token);
  keep_token_if_same(edited.wear_flags, edited.wear_flags_token, previous.wear_flags,
                     previous.wear_flags_token);
  keep_token_if_same(edited.extra_flags2, edited.extra_flags2_token, previous.extra_flags2,
                     previous.extra_flags2_token);
}

namespace {

void exp_table_for_level(int level, int* base, int* phit, int* sab) {
  // Allineato a Server/src/utility.cpp DetermineExp (case 0..59 + default).
  static constexpr int kBase[] = {
      1,      8,      12,     17,     23,     30,     37,     47,     57,     70,
      84,     101,    122,    146,    175,    209,    251,    301,    362,    435,
      524,    632,    762,    919,    1110,   1339,   1617,   1951,   2354,   2838,
      3421,   4120,   4960,   5968,   7175,   8621,   10351,  12422,  14897,  17857,
      21394,  25619,  30666,  36692,  43889,  52482,  62742,  74992,  89618,  107084,
      127942, 150000, 170000, 190000, 210000, 230000, 250000, 275000, 300000, 325000,
  };
  static constexpr int kPhit[] = {
      1,  2,  2,  3,  3,  3,  3,  4,  4,  4,  4,  5,  5,  5,  6,  6,  6,  7,  7,  8,
      8,  9,  9,  10, 10, 11, 12, 12, 13, 14, 15, 16, 17, 18, 19, 21, 22, 23, 25, 26,
      28, 29, 31, 33, 35, 37, 40, 42, 45, 47, 50, 53, 55, 57, 59, 61, 63, 65, 67, 69,
  };
  static constexpr int kSab[] = {
      1,     2,     3,     4,     6,     7,     9,     12,    14,    17,    21,
      25,    30,    36,    44,    52,    63,    75,    90,    109,   131,   158,
      190,   230,   277,   335,   404,   488,   588,   710,   855,   1030,  1240,
      1492,  1794,  2155,  2588,  3105,  3724,  4464,  5348,  6405,  7666,  9173,
      10975, 13121, 15685, 18748, 22405, 26771, 31985, 37500, 42500, 47500, 52500,
      57500, 62500, 68750, 75000, 71250,
  };
  if(level < 0) {
    *base = 1;
    *phit = 1;
    *sab = 1;
    return;
  }
  if(level <= 59) {
    *base = kBase[level];
    *phit = kPhit[level];
    *sab = kSab[level];
    return;
  }
  *base = 350000;
  *phit = 70;
  *sab = 87500;
}

}  // namespace

MobExpEstimate estimate_mob_exp(const Mobile& mob) {
  MobExpEstimate out;
  out.file_value = mob.exp;
  const int level = static_cast<int>(mob.level);

  if(mob.letter == 'S') {
    out.uses_bonus_formula = false;
    out.estimated_xp = mob.exp < 0 ? -mob.exp : mob.exp;
    return out;
  }

  if(mob.exp < 0) {
    out.uses_bonus_formula = false;
    out.estimated_xp = -mob.exp;
    return out;
  }

  out.uses_bonus_formula = true;
  out.bonus_unusually_high = mob.exp > 32;
  int base = 0;
  int phit = 0;
  int sab = 0;
  exp_table_for_level(level, &base, &phit, &sab);
  out.base = base;
  out.phit = phit;
  out.sab = sab;
  // A/N/B/L: max_hit = dice(level, 8) + hit_bonus → media level*4.5 + hit_bonus
  out.estimated_hit = (static_cast<std::int64_t>(level) * 9) / 2 + mob.hit_bonus;
  if(out.estimated_hit < 0) {
    out.estimated_hit = 0;
  }
  out.estimated_xp = base + phit * out.estimated_hit + sab * mob.exp + mob.gold;
  return out;
}

}  // namespace eleutheria
