#include "eleutheria/world.hpp"

#include <cctype>

namespace eleutheria {

std::vector<ValidationIssue> validate_area(const Area& area) {
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
        // May be outside area — warning only
        add(ValidationIssue::Severity::Info, "room", vnum,
            "Uscita " + std::to_string(d) + " verso stanza " + std::to_string(ex.to_room) +
                " non presente in questa area");
      }
    }
  }

  for(const auto& [num, zone] : area.zones) {
    for(const auto& cmd : zone.commands) {
      if(cmd.is_comment) {
        continue;
      }
      if(cmd.command == 'M' && area.mobiles.find(cmd.arg1) == area.mobiles.end()) {
        add(ValidationIssue::Severity::Warning, "zone", num,
            "Reset M riferisce mob " + std::to_string(cmd.arg1) + " assente");
      }
      if((cmd.command == 'O' || cmd.command == 'G' || cmd.command == 'E' || cmd.command == 'P') &&
         area.objects.find(cmd.arg1) == area.objects.end()) {
        add(ValidationIssue::Severity::Warning, "zone", num,
            std::string("Reset ") + cmd.command + " riferisce oggetto " +
                std::to_string(cmd.arg1) + " assente");
      }
      if((cmd.command == 'M' || cmd.command == 'O' || cmd.command == 'D') && cmd.arg3 >= 0 &&
         area.rooms.find(cmd.arg3) == area.rooms.end() && cmd.command != 'D') {
        // M/O: arg3 is room
        add(ValidationIssue::Severity::Info, "zone", num,
            std::string("Reset ") + cmd.command + " stanza " + std::to_string(cmd.arg3) +
                " non in area");
      }
      if(cmd.command == 'D' && area.rooms.find(cmd.arg1) == area.rooms.end()) {
        add(ValidationIssue::Severity::Warning, "zone", num,
            "Reset D stanza " + std::to_string(cmd.arg1) + " assente");
      }
    }
  }

  for(const auto& [keeper, shop] : area.shops) {
    if(area.mobiles.find(shop.keeper) == area.mobiles.end()) {
      add(ValidationIssue::Severity::Warning, "shop", shop.keeper,
          "Shopkeeper mob assente nell'area");
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

}  // namespace eleutheria
