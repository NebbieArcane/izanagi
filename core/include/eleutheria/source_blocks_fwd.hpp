#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace eleutheria {

struct FileSourceBlocks {
  std::string prefix;
  std::vector<std::int64_t> order;
  std::unordered_map<std::int64_t, std::string> blocks;
  std::string trailer;
  bool valid = false;
};

}  // namespace eleutheria
