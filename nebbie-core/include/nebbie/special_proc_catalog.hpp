#pragma once

#include <string>
#include <vector>

namespace nebbie {

constexpr long kMobActSpecFlag = 1;

const std::vector<std::string>& mob_object_special_proc_names();
const std::vector<std::string>& room_special_proc_names();

const std::vector<std::string>& special_proc_names_for_type(char type);

bool special_proc_names_equal(const std::string& left, const std::string& right);
bool is_known_special_proc(char type, const std::string& procedure);

} // namespace nebbie
