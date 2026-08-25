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
};

QString default_config_path();
AppConfig read_config();
bool write_config(const AppConfig& config);

QString read_lib_path();
bool write_lib_path(const QString& path);
bool lib_path_exists(const QString& path);

} // namespace nebbie::qt
