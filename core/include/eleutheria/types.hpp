#pragma once

#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

#include "eleutheria/source_blocks_fwd.hpp"

namespace eleutheria {

inline constexpr int kMaxExits = 6;
inline constexpr int kMaxObjAffect = 5;
inline constexpr int kMaxShopProduce = 5;
inline constexpr int kMaxShopType = 5;
inline constexpr int kMaxShopMsg = 7;

struct ExtraDesc {
  std::string keywords;
  std::string description;
};

struct Exit {
  bool present = false;
  std::string general_description;
  std::string keyword;
  std::int64_t exit_info = 0;
  std::string exit_info_token;  // spelling originale (es. 1|32|128)
  std::int64_t key = -1;
  std::int64_t to_room = -1;
};

struct Room {
  std::int64_t vnum = 0;
  std::string name;
  std::string description;
  std::int64_t zone_hint = -1;  // first number on flags line (often zone or -1)
  std::int64_t room_flags = 0;
  std::string room_flags_token;
  std::int64_t sector_type = 0;
  bool has_teleport = false;
  std::int64_t tele_time = 0;
  std::int64_t tele_targ = 0;
  std::int64_t tele_mask = 0;
  std::string tele_mask_token;
  std::int64_t tele_cnt = 0;
  bool has_river = false;
  std::int64_t river_speed = 0;
  std::int64_t river_dir = 0;
  bool has_tunnel_limit = false;
  std::int64_t moblim = 1;
  std::array<Exit, kMaxExits> exits{};
  std::vector<ExtraDesc> extras;
  std::string bright_night;
  std::string bright_day;
  bool has_bright = false;
};

struct ObjAffect {
  int location = 0;
  int modifier = 0;
};

struct GameObject {
  std::int64_t vnum = 0;
  std::string name;
  std::string short_description;
  std::string description;
  std::string action_description;
  std::int64_t type_flag = 0;
  std::int64_t extra_flags = 0;
  std::string extra_flags_token;
  std::int64_t wear_flags = 0;
  std::string wear_flags_token;
  std::array<std::int64_t, 4> value{{0, 0, 0, 0}};
  std::int64_t weight = 0;
  std::int64_t cost = 0;
  std::int64_t cost_per_day = 0;
  std::vector<ExtraDesc> extras;
  std::vector<ObjAffect> affects;
  std::int64_t extra_flags2 = 0;
  std::string extra_flags2_token;
  bool has_extra_flags2 = false;
  std::string forbidden_to_char;
  std::string forbidden_to_room;
  bool has_forbidden = false;
};

struct Mobile {
  std::int64_t vnum = 0;
  std::string name;
  std::string short_descr;
  std::string long_descr;
  std::string description;
  std::int64_t act = 0;
  std::string act_token;
  std::int64_t affected_by = 0;
  std::string affected_by_token;
  std::int64_t alignment = 0;
  char letter = 'A';  // S, A, N, B, L
  double mult_att = 1.0;
  std::int64_t level = 1;
  std::int64_t thac0 = 20;       // as stored (20 - hitroll in runtime)
  std::int64_t armor = 0;        // as stored (points.armor/10 in A types)
  std::int64_t hit_bonus = 0;    // for A/N/B/L
  std::int64_t dam_dice_n = 1;
  std::int64_t dam_dice_s = 1;
  std::int64_t dam_bonus = 0;
  // Simple (S) also has hit dice NdS+B
  std::int64_t hit_dice_n = 1;
  std::int64_t hit_dice_s = 8;
  std::int64_t hit_dice_b = 0;
  bool gold_exp_race_form = true;  // starts with -1
  std::int64_t gold = 0;
  std::int64_t exp = 0;  // as written in file (often negative absolute)
  std::int64_t race = 0;
  bool has_spellpower = false;
  std::int64_t spellpower = 0;
  std::int64_t position = 8;
  std::int64_t default_pos = 8;
  std::int64_t sex = 0;
  bool has_immunities = false;
  std::int64_t immune = 0;
  std::string immune_token;
  std::int64_t m_immune = 0;
  std::string m_immune_token;
  std::int64_t susc = 0;
  std::string susc_token;
  std::string sounds;
  std::string distant_sounds;
};

struct ResetCommand {
  char command = '*';
  int if_flag = 0;
  std::int64_t arg1 = -1;
  std::int64_t arg2 = 0;
  std::int64_t arg3 = -1;
  std::int64_t arg4 = 0;
  std::string comment;  // trailing text after args (kept for roundtrip)
  bool is_comment = false;
  std::string raw_line;  // for pure comment lines
};

struct Zone {
  int number = 0;
  std::string name;
  std::int64_t top = 0;
  std::int64_t lifespan = 30;
  std::int64_t reset_mode = 2;
  std::vector<ResetCommand> commands;
};

struct Shop {
  std::int64_t keeper = 0;
  std::array<std::int64_t, kMaxShopProduce> producing{{-1, -1, -1, -1, -1}};
  double profit_buy = 1.0;
  double profit_sell = 1.0;
  std::array<std::int64_t, kMaxShopType> type{{-1, -1, -1, -1, -1}};
  std::array<std::string, kMaxShopMsg> messages;
  std::int64_t temper1 = 0;
  std::int64_t temper2 = 0;
  std::int64_t with_who = 0;
  std::int64_t in_room = 0;
  std::int64_t open1 = 0;
  std::int64_t close1 = 28;
  std::int64_t open2 = 0;
  std::int64_t close2 = 0;
};

struct AreaMeta {
  std::string name;
  std::string author;
  std::string mud_id = "nebbie";
};

struct Area {
  AreaMeta meta;
  std::string base_path;   // without extension
  std::string directory;
  std::map<std::int64_t, Room> rooms;
  std::map<std::int64_t, Mobile> mobiles;
  std::map<std::int64_t, GameObject> objects;
  std::map<int, Zone> zones;
  std::map<std::int64_t, Shop> shops;
  bool dirty = false;

  // Preserve: blocchi originali e vnum modificati (solo quelli vengono riscritti).
  FileSourceBlocks wld_sources;
  FileSourceBlocks mob_sources;
  FileSourceBlocks obj_sources;
  FileSourceBlocks zon_sources;
  std::unordered_set<std::int64_t> dirty_rooms;
  std::unordered_set<std::int64_t> dirty_mobs;
  std::unordered_set<std::int64_t> dirty_objects;
  std::unordered_set<std::int64_t> dirty_zones;
};

}  // namespace eleutheria
