#pragma once

#include <QString>

namespace nebbie::translate {

QString default_config_path();
QString read_lib_path();
bool write_lib_path(const QString& path);
bool lib_path_exists(const QString& path);

} // namespace nebbie::translate
