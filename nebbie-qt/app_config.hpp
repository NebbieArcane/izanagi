#pragma once

#include <QString>

namespace nebbie::qt {

struct AppConfig {
    QString lib_path;
    QString index_url;
    QString coordinator_url;
    QString coordinator_token;
    QString builder_name;
    int max_line_length = 0;
    bool show_color_codes = false;
    bool check_updates = true;
    QString last_update_check;
    QString dismissed_update_version;
    QString ui_language = QStringLiteral("it");
    QString aree_workspace_root;
    QString aree_last_area;
    /** When true, archive the open Aree folder once before the first Save of that session. */
    bool aree_archive_on_first_save = true;
    /** Root of NebbieArcane/Server for dynamic special-proc catalog (empty = default guess). */
    QString server_root;
};

QString default_config_path();
/** Directory that holds nebbieedit.conf and extra_special_procs.txt. */
QString config_directory_path();
QString default_extra_special_procs_path();
AppConfig read_config();
bool write_config(const AppConfig& config);

QString read_lib_path();
bool write_lib_path(const QString& path);
bool lib_path_exists(const QString& path);

} // namespace nebbie::qt
