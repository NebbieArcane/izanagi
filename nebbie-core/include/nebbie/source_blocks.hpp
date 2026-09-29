#pragma once

#include "io.hpp"
#include "file_source_blocks.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace nebbie {

/** Capture #vnum … blocks from myst.{wld,mob,obj} (excludes #0 and %% terminators). */
std::optional<FileSourceBlocks> capture_vnum_hash_file(const std::filesystem::path& path);

struct PreserveEntitySaveOptions {
    bool enabled = false;
    const FileSourceBlocks* sources = nullptr;
    const std::unordered_set<long>* dirty_vnums = nullptr;
};

void save_myst_mob_preserve(const World& world,
                            const std::filesystem::path& path,
                            ProgressCallback progress,
                            MystSaveOptions options,
                            PreserveEntitySaveOptions preserve);

void save_myst_wld_preserve(const World& world,
                            const std::filesystem::path& path,
                            ProgressCallback progress,
                            MystSaveOptions options,
                            PreserveEntitySaveOptions preserve);

void save_myst_obj_preserve(const World& world,
                            const std::filesystem::path& path,
                            ProgressCallback progress,
                            MystSaveOptions options,
                            PreserveEntitySaveOptions preserve);

} // namespace nebbie
