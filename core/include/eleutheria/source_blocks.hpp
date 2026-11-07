#pragma once

#include "eleutheria/source_blocks_fwd.hpp"
#include "eleutheria/types.hpp"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace eleutheria {

std::optional<FileSourceBlocks> capture_vnum_hash_file(const std::filesystem::path& path);
std::optional<FileSourceBlocks> capture_vnum_hash_text(const std::string& content);

std::string format_room_block(const Room& room);
std::string format_mobile_block(const Mobile& mob);
std::string format_object_block(const GameObject& obj);
std::string format_zone_block(const Zone& zone);

/**
 * Riscrive un file #vnum: blocchi non dirty restano byte-identici;
 * dirty/nuovi vengono serializzati; eliminati spariscono.
 */
std::string build_preserved_hash_file(
    const FileSourceBlocks& sources,
    const std::unordered_set<std::int64_t>& dirty_vnums,
    const std::unordered_map<std::int64_t, std::string>& formatted_dirty_or_new,
    const std::vector<std::int64_t>& living_vnums_sorted);

/** Dopo un salvataggio preserve: aggiorna i blocchi in memoria e svuota dirty. */
void refresh_source_blocks_after_save(FileSourceBlocks* sources,
                                      std::unordered_set<std::int64_t>* dirty,
                                      const std::unordered_map<std::int64_t, std::string>& written_blocks,
                                      const std::vector<std::int64_t>& living_vnums_sorted);

}  // namespace eleutheria
