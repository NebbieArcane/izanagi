#pragma once

#include "io.hpp"
#include "world.hpp"

#include <filesystem>
#include <set>
#include <string>
#include <vector>

namespace nebbie {

struct ZoneEntitySet {
    int zone_num = 0;
    int bottom = 0;
    int top = 0;
    std::string zone_name;
    std::set<long> room_vnums;
    std::set<long> mob_vnums;
    std::set<long> object_vnums;
    std::vector<std::string> warnings;
};

struct ZonePartitionOptions {
    bool write_monolith_files = true;
    bool write_overlay_dirs = true;
    bool include_shop_entities = true;
    bool include_special_procs = true;
    /** Omit #$ / %% / #0 in per-zone myst.* (Aree deploy_aree.php concat). */
    bool write_eof_markers = false;
    /** Write src/<slug>/<slug>.{zon,wld,...} instead of zone-NNNN-name/myst.* */
    bool aree_layout = false;
};

struct ZonePartitionReport {
    int zones_written = 0;
    int rooms = 0;
    int objects = 0;
    int mobiles = 0;
    int zone_resets = 0;
    std::vector<std::string> warnings;
};

ZoneEntitySet collect_zone_entities(const World& world,
                                    int zone_num,
                                    const ZonePartitionOptions& options = {});

std::string zone_pack_directory_name(int zone_num, const std::string& zone_name);

/** Filesystem slug for Aree src/<slug>/<slug>.* (no zone- prefix). */
std::string aree_area_slug(int zone_num, const std::string& zone_name);

ZonePartitionReport export_zone_pack(const World& world,
                                     const std::filesystem::path& output_root,
                                     int zone_num,
                                     ZonePartitionOptions options = {},
                                     ProgressCallback progress = {});

ZonePartitionReport export_all_zone_packs(const World& world,
                                          const std::filesystem::path& output_root,
                                          ZonePartitionOptions options = {},
                                          ProgressCallback progress = {});

ZonePartitionReport merge_zone_packs(const std::filesystem::path& zones_root,
                                     const std::filesystem::path& merged_lib,
                                     ProgressCallback progress = {});

} // namespace nebbie
