#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace eleutheria {

struct FlagDef {
  int bit_index = 0;  // 0 = 1<<0
  std::string key;
  std::string label;
};

struct NamedValue {
  int value = 0;
  std::string key;
  std::string label;
};

class Catalog {
 public:
  static Catalog nebbie_defaults();

  const std::vector<FlagDef>& room_flags() const { return room_flags_; }
  const std::vector<FlagDef>& exit_flags() const { return exit_flags_; }
  const std::vector<FlagDef>& act_flags() const { return act_flags_; }
  const std::vector<FlagDef>& affect_flags() const { return affect_flags_; }
  const std::vector<FlagDef>& extra_flags() const { return extra_flags_; }
  const std::vector<FlagDef>& wear_flags() const { return wear_flags_; }
  const std::vector<FlagDef>& immunity_flags() const { return immunity_flags_; }

  const std::vector<NamedValue>& sectors() const { return sectors_; }
  const std::vector<NamedValue>& item_types() const { return item_types_; }
  const std::vector<NamedValue>& apply_types() const { return apply_types_; }
  const std::vector<NamedValue>& genders() const { return genders_; }
  const std::vector<NamedValue>& positions() const { return positions_; }

  static std::uint64_t bit(int index) {
    return index < 0 ? 0ULL : (1ULL << static_cast<unsigned>(index));
  }

 private:
  std::vector<FlagDef> room_flags_;
  std::vector<FlagDef> exit_flags_;
  std::vector<FlagDef> act_flags_;
  std::vector<FlagDef> affect_flags_;
  std::vector<FlagDef> extra_flags_;
  std::vector<FlagDef> wear_flags_;
  std::vector<FlagDef> immunity_flags_;
  std::vector<NamedValue> sectors_;
  std::vector<NamedValue> item_types_;
  std::vector<NamedValue> apply_types_;
  std::vector<NamedValue> genders_;
  std::vector<NamedValue> positions_;
};

}  // namespace eleutheria
