#pragma once

#include "eleutheria/types.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace eleutheria {

struct ValidationIssue {
  enum class Severity { Info, Warning, Error };
  Severity severity = Severity::Warning;
  std::string kind;   // room/mob/obj/zone/shop
  std::int64_t vnum = -1;
  std::string message;
};

std::vector<ValidationIssue> validate_area(const Area& area);

/** Rimuove $cNNNN e simili per liste UI su una sola riga. */
std::string strip_mud_colors(std::string_view text);

std::string room_list_label(const Room& room);
std::string mob_list_label(const Mobile& mob);
std::string obj_list_label(const GameObject& obj);
std::string zone_list_label(const Zone& zone);

Zone* zone_for_room(Area& area, std::int64_t room_vnum);
const Zone* zone_for_room(const Area& area, std::int64_t room_vnum);

/** Se i bit non sono cambiati, riusa lo spelling originale (1|2|4 vs 7). */
void preserve_flag_tokens(Room& edited, const Room& previous);
void preserve_flag_tokens(Mobile& edited, const Mobile& previous);
void preserve_flag_tokens(GameObject& edited, const GameObject& previous);

}  // namespace eleutheria
