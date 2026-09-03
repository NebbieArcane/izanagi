#include "nebbie/session.hpp"

#include "nebbie/constants.hpp"
#include "nebbie/io.hpp"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <sstream>
#include "nebbie/time_compat.hpp"

namespace nebbie {

namespace {

std::string timestamp_id() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
    if (!utc_tm_from_time_t(t, tm)) {
        return "unknown";
    }
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y%m%dT%H%M%SZ");
    return oss.str();
}

void write_version_meta(const std::filesystem::path& dir, const std::string& label) {
    std::ofstream meta(dir / "version.txt");
    meta << label << '\n';
    meta << timestamp_id() << '\n';
}

void copy_if_exists(const std::filesystem::path& from, const std::filesystem::path& to) {
    std::error_code ec;
    if (!std::filesystem::exists(from, ec)) {
        return;
    }
    std::filesystem::create_directories(to.parent_path(), ec);
    std::filesystem::copy_file(from, to, std::filesystem::copy_options::overwrite_existing, ec);
}

void copy_overlay_directory(const std::filesystem::path& from_root,
                            const std::filesystem::path& to_root,
                            const char* subdir) {
    const std::filesystem::path from = from_root / subdir;
    std::error_code ec;
    if (!std::filesystem::exists(from, ec)) {
        return;
    }
    std::filesystem::copy(from,
                          to_root / subdir,
                          std::filesystem::copy_options::recursive
                              | std::filesystem::copy_options::overwrite_existing,
                          ec);
}

void copy_overlay_directories(const std::filesystem::path& from_root,
                              const std::filesystem::path& to_root) {
    copy_overlay_directory(from_root, to_root, OVERLAY_ROOMS_DIR);
    copy_overlay_directory(from_root, to_root, OVERLAY_OBJECTS_DIR);
    copy_overlay_directory(from_root, to_root, OVERLAY_MOBILES_DIR);
    copy_overlay_directory(from_root, to_root, OVERLAY_ZONES_DIR);
}

void copy_lib_files(const LibContext& context,
                    const std::filesystem::path& from_root,
                    const std::filesystem::path& to_root) {
    std::error_code ec;
    std::filesystem::create_directories(to_root, ec);

    const auto copy_path = [&](const std::filesystem::path& filename) {
        if (filename.empty()) {
            return;
        }
        copy_if_exists(from_root / filename, to_root / filename);
    };

    const auto copy_tracked = [&](const auto& sources, const std::filesystem::path& fallback) {
        if (sources.empty()) {
            copy_path(fallback);
            return;
        }
        for (const auto& [_, source_path] : sources) {
            copy_path(source_path);
        }
    };

    if (context.has_zon) {
        copy_tracked(context.zone_sources, context.zon_path);
    }
    if (context.has_wld) {
        copy_tracked(context.room_sources, context.wld_path);
    }
    if (context.has_mob) {
        copy_tracked(context.mobile_sources, context.mob_path);
    }
    if (context.has_obj) {
        copy_tracked(context.object_sources, context.obj_path);
    }
    if (context.has_shp) {
        copy_path(context.shp_path);
    }
    if (context.has_spe) {
        copy_path(context.spe_path);
    }
    if (context.has_dam) {
        copy_path(context.dam_path);
    }
    if (context.has_act) {
        copy_path(context.act_path);
    }
    if (context.has_pos) {
        copy_path(context.pos_path);
    }
    if (context.has_gui) {
        copy_path(context.gui_path);
    }

    copy_overlay_directories(from_root, to_root);
}

std::filesystem::path version_path(const std::filesystem::path& versions_root,
                                   const std::string& id,
                                   const std::string& label) {
    std::string folder = id;
    if (!label.empty()) {
        folder += '_';
        for (char c : label) {
            if (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_') {
                folder.push_back(c);
            } else if (c == ' ') {
                folder.push_back('-');
            }
        }
    }
    return versions_root / folder;
}

} // namespace

std::filesystem::path session_root(const std::filesystem::path& lib_root) {
    return lib_root / ".nebbie";
}

std::filesystem::path workspace_dir(const std::filesystem::path& lib_root) {
    return session_root(lib_root) / "workspace";
}

std::filesystem::path versions_dir(const std::filesystem::path& lib_root) {
    return session_root(lib_root) / "versions";
}

void save_snapshot(const World& world,
                   const LibContext& context,
                   const std::filesystem::path& destination,
                   const std::string& label) {
    LibContext snapshot_context = context;
    snapshot_context.root = destination;
    std::error_code ec;
    std::filesystem::create_directories(destination, ec);
    save_lib(world, snapshot_context);
    copy_overlay_directories(context.root, destination);
    if (!label.empty()) {
        write_version_meta(destination, label);
    }
}

void backup_lib_on_disk(const LibContext& context,
                        const std::filesystem::path& destination,
                        const std::string& label) {
    std::error_code ec;
    std::filesystem::create_directories(destination, ec);
    copy_lib_files(context, context.root, destination);
    write_version_meta(destination, label);
}

std::vector<VersionInfo> list_versions(const std::filesystem::path& lib_root) {
    std::vector<VersionInfo> versions;
    const auto root = versions_dir(lib_root);
    std::error_code ec;
    if (!std::filesystem::exists(root, ec)) {
        return versions;
    }

    for (const auto& entry : std::filesystem::directory_iterator(root, ec)) {
        if (!entry.is_directory()) {
            continue;
        }
        VersionInfo info;
        info.id = entry.path().filename().string();
        info.modified = entry.last_write_time(ec);

        const auto meta = entry.path() / "version.txt";
        if (std::filesystem::exists(meta, ec)) {
            std::ifstream in(meta);
            std::getline(in, info.label);
        }
        if (info.label.empty()) {
            const auto underscore = info.id.find('_');
            info.label = underscore == std::string::npos ? "snapshot" : info.id.substr(underscore + 1);
        }
        versions.push_back(std::move(info));
    }

    std::sort(versions.begin(), versions.end(), [](const VersionInfo& a, const VersionInfo& b) {
        return a.modified > b.modified;
    });
    return versions;
}

std::string create_version(const World& world,
                           const LibContext& context,
                           const std::filesystem::path& lib_root,
                           const std::string& label) {
    const std::string id = timestamp_id();
    const auto dest = version_path(versions_dir(lib_root), id, label);
    save_snapshot(world, context, dest, label);
    return dest.filename().string();
}

void restore_snapshot(World& world, LibContext& context, const std::filesystem::path& snapshot_dir) {
    load_lib(world, snapshot_dir, context);
}

void prune_versions(const std::filesystem::path& lib_root, std::size_t max_versions) {
    auto versions = list_versions(lib_root);
    if (versions.size() <= max_versions) {
        return;
    }

    std::error_code ec;
    for (std::size_t i = max_versions; i < versions.size(); ++i) {
        std::filesystem::remove_all(versions_dir(lib_root) / versions[i].id, ec);
    }
}

AutosaveResult run_autosave(const World& world,
                            const LibContext& context,
                            const std::filesystem::path& lib_root,
                            const SessionConfig& config,
                            std::chrono::system_clock::time_point last_version_time,
                            const std::filesystem::path& session_root) {
    const std::filesystem::path storage_root = session_root.empty() ? lib_root : session_root;
    AutosaveResult result;
    save_snapshot(world, context, workspace_dir(storage_root), "workspace");
    result.workspace_updated = true;

    const auto now = std::chrono::system_clock::now();
    const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_version_time).count();
    if (elapsed >= config.version_interval_sec) {
        result.version_id = create_version(world, context, storage_root, "autosave");
        result.version_created = true;
        prune_versions(storage_root, config.max_versions);
    }

    return result;
}

void save_lib_with_backup(const World& world,
                          const LibContext& context,
                          const std::filesystem::path& lib_root,
                          ProgressCallback progress,
                          const std::filesystem::path& session_root) {
    const std::filesystem::path storage_root = session_root.empty() ? lib_root : session_root;
    const std::string id = timestamp_id();
    const auto backup_dir = version_path(versions_dir(storage_root), id, "pre-save");
    backup_lib_on_disk(context, backup_dir, "pre-save");
    prune_versions(storage_root, SessionConfig{}.max_versions);

    LibContext live_context = context;
    live_context.root = lib_root;
    save_lib(world, live_context, progress);
}

} // namespace nebbie
