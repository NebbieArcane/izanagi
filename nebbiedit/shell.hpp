#pragma once

#include "nebbie/world.hpp"

#include <filesystem>
#include <map>
#include <string>

namespace nebbiedit {

int run_shell(const std::filesystem::path& lib_root);

bool run_room_set(const std::filesystem::path& lib_root,
                  long vnum,
                  const std::map<std::string, std::string>& flags,
                  bool force);

bool run_mob_set(const std::filesystem::path& lib_root,
                 long vnum,
                 const std::map<std::string, std::string>& flags,
                 bool force);

bool run_obj_set(const std::filesystem::path& lib_root,
                 long vnum,
                 const std::map<std::string, std::string>& flags,
                 bool force);

void print_room_show(const nebbie::World& world, long vnum);
void print_room_inbound(const nebbie::World& world, long vnum);
bool run_room_show(const std::filesystem::path& lib_root, long vnum);
bool run_room_inbound(const std::filesystem::path& lib_root, long vnum);
bool run_room_list(const std::filesystem::path& lib_root, const std::string& prefix = {});

} // namespace nebbiedit
