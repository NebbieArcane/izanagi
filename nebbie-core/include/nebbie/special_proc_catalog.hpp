#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace nebbie {

constexpr long kMobActSpecFlag = 1;

/** Embedded baseline (generated from vendor/nebbietest snapshot). */
const std::vector<std::string>& embedded_mob_object_special_proc_names();
const std::vector<std::string>& embedded_room_special_proc_names();

/**
 * Effective catalogs used by validation and the specials editor combo:
 * embedded ∪ names parsed from Server specialproc_*.cpp ∪ editable allowlist.
 */
const std::vector<std::string>& mob_object_special_proc_names();
const std::vector<std::string>& room_special_proc_names();
const std::vector<std::string>& special_proc_names_for_type(char type);

bool special_proc_names_equal(const std::string& left, const std::string& right);
bool is_known_special_proc(char type, const std::string& procedure);

struct SpecialProcCatalogConfig {
    /** Root of NebbieArcane/Server (src/specialproc_*.cpp are read from here). */
    std::filesystem::path server_root;
    /** One procedure name per line; # comments allowed. */
    std::filesystem::path extra_allowlist_path;
};

struct SpecialProcCatalogStatus {
    bool server_loaded = false;
    std::size_t server_mob_object_count = 0;
    std::size_t server_room_count = 0;
    std::size_t extra_count = 0;
    std::size_t effective_mob_object_count = 0;
    std::size_t effective_room_count = 0;
    std::string server_detail;
};

SpecialProcCatalogConfig special_proc_catalog_config();
SpecialProcCatalogStatus special_proc_catalog_status();

/** Apply paths, reload Server tables + allowlist file, rebuild effective catalogs. */
void configure_special_proc_catalog(SpecialProcCatalogConfig config);

/** Default guess: $NEBBIE_SERVER_ROOT or ~/NebbieArcane/Server. */
std::filesystem::path default_nebbie_server_root();

std::vector<std::string> extra_special_proc_names();
/** Replace in-memory allowlist (does not write disk until save_extra_special_procs). */
void set_extra_special_proc_names(std::vector<std::string> names);
bool load_extra_special_procs(const std::filesystem::path& path);
bool save_extra_special_procs(const std::filesystem::path& path);

/** Extract {"Name", ...} entries from a specialproc_*.cpp table file. */
std::vector<std::string> parse_special_proc_names_from_cpp(const std::string& text);

/**
 * Read Server tables without changing the active catalog (for UI status preview).
 * Returns false if files are missing; fills counts/detail otherwise.
 */
bool probe_server_special_proc_tables(const std::filesystem::path& server_root,
                                      std::size_t* mob_object_count,
                                      std::size_t* room_count,
                                      std::string* detail);

} // namespace nebbie
