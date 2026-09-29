#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace nebbie {

struct FileSourceBlocks {
    std::vector<long> order;
    std::unordered_map<long, std::string> blocks;
};

} // namespace nebbie
