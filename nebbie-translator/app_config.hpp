#pragma once

#include <QString>

namespace nebbie::translate {

struct AppConfig {
    QString lib_path;
    int max_line_length = 0;
};

QString default_config_path();
AppConfig read_config();
bool write_config(const AppConfig& config);
QString read_lib_path();
bool write_lib_path(const QString& path);
bool lib_path_exists(const QString& path);

} // namespace nebbie::translate
