#include "app_config.hpp"

#include "path_util.hpp"

#include "nebbie/io.hpp"

#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QTextStream>

#include <filesystem>

namespace nebbie::translate {

namespace {

QString config_directory() {
    QString base = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (base.isEmpty()) {
        base = QDir::homePath() + QStringLiteral("/.config/Nebbie");
    }
    QDir().mkpath(base);
    return base;
}

int parse_max_line_length(const QString& value) {
    bool ok = false;
    const int parsed = value.trimmed().toInt(&ok);
    if (!ok) {
        return 0;
    }
    if (parsed < 0) {
        return 0;
    }
    if (parsed > 512) {
        return 512;
    }
    return parsed;
}

bool parse_bool(const QString& value) {
    const QString normalized = value.trimmed().toLower();
    return normalized == QStringLiteral("1") || normalized == QStringLiteral("true")
           || normalized == QStringLiteral("yes") || normalized == QStringLiteral("on");
}

} // namespace

QString default_config_path() {
    return config_directory() + QStringLiteral("/nebbie-translate.conf");
}

AppConfig read_config() {
    AppConfig config;
    QFile file(default_config_path());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return config;
    }

    QTextStream in(&file);
    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        if (line.startsWith(QStringLiteral("lib_path="))) {
            config.lib_path = line.mid(QStringLiteral("lib_path=").size()).trimmed();
        } else if (line.startsWith(QStringLiteral("max_line_length="))) {
            config.max_line_length =
                parse_max_line_length(line.mid(QStringLiteral("max_line_length=").size()));
        } else if (line.startsWith(QStringLiteral("show_color_codes="))) {
            config.show_color_codes =
                parse_bool(line.mid(QStringLiteral("show_color_codes=").size()));
        } else if (line.startsWith(QStringLiteral("check_updates="))) {
            config.check_updates =
                parse_bool(line.mid(QStringLiteral("check_updates=").size()));
        } else if (line.startsWith(QStringLiteral("last_update_check="))) {
            config.last_update_check = line.mid(QStringLiteral("last_update_check=").size()).trimmed();
        } else if (line.startsWith(QStringLiteral("dismissed_update_version="))) {
            config.dismissed_update_version =
                line.mid(QStringLiteral("dismissed_update_version=").size()).trimmed();
        } else if (line.startsWith(QStringLiteral("ui_language="))) {
            config.ui_language = line.mid(QStringLiteral("ui_language=").size()).trimmed();
        }
    }
    return config;
}

bool write_config(const AppConfig& config) {
    QFile file(default_config_path());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        return false;
    }

    QTextStream out(&file);
    out << "# Nebbie Translate configuration\n";
    out << "lib_path=" << config.lib_path << '\n';
    out << "max_line_length=" << config.max_line_length << '\n';
    out << "show_color_codes=" << (config.show_color_codes ? 1 : 0) << '\n';
    out << "check_updates=" << (config.check_updates ? 1 : 0) << '\n';
    if (!config.last_update_check.isEmpty()) {
        out << "last_update_check=" << config.last_update_check << '\n';
    }
    if (!config.dismissed_update_version.isEmpty()) {
        out << "dismissed_update_version=" << config.dismissed_update_version << '\n';
    }
    if (!config.ui_language.isEmpty()) {
        out << "ui_language=" << config.ui_language << '\n';
    }
    return true;
}

QString read_lib_path() {
    return read_config().lib_path;
}

bool write_lib_path(const QString& path) {
    AppConfig config = read_config();
    config.lib_path = path;
    return write_config(config);
}

bool lib_path_exists(const QString& path) {
    if (path.isEmpty()) {
        return false;
    }

    std::error_code ec;
    const std::filesystem::path candidate = nebbie::qt::path_from_qstring(path);
    if (!std::filesystem::exists(candidate, ec)) {
        return false;
    }

    const std::filesystem::path resolved = nebbie::resolve_lib_directory(candidate);
    return nebbie::directory_has_lib_files(resolved);
}

} // namespace nebbie::translate
