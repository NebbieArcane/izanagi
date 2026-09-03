#include "app_config.hpp"
#include "path_util.hpp"

#include "nebbie/io.hpp"

#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QTextStream>

#include <filesystem>

namespace nebbie::qt {

namespace {

QString config_directory() {
    QString base = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (base.isEmpty()) {
        base = QDir::homePath() + QStringLiteral("/.config/Nebbie");
    }
    QDir().mkpath(base);
    return base;
}

QString value_after_equals(const QString& line) {
    const int eq = line.indexOf('=');
    if (eq < 0) {
        return {};
    }
    return line.mid(eq + 1).trimmed();
}

bool parse_bool(const QString& value) {
    const QString normalized = value.trimmed().toLower();
    return normalized == QStringLiteral("1") || normalized == QStringLiteral("true")
           || normalized == QStringLiteral("yes") || normalized == QStringLiteral("on");
}

} // namespace

QString default_config_path() {
    return config_directory() + QStringLiteral("/nebbieedit.conf");
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
        if (line.isEmpty() || line.startsWith('#')) {
            continue;
        }
        if (line.startsWith(QStringLiteral("lib_path="))) {
            config.lib_path = value_after_equals(line);
        } else if (line.startsWith(QStringLiteral("index_url="))) {
            config.index_url = value_after_equals(line);
        } else if (line.startsWith(QStringLiteral("coordinator_url="))) {
            config.coordinator_url = value_after_equals(line);
        } else if (line.startsWith(QStringLiteral("coordinator_token="))) {
            config.coordinator_token = value_after_equals(line);
        } else if (line.startsWith(QStringLiteral("builder_name="))) {
            config.builder_name = value_after_equals(line);
        } else if (line.startsWith(QStringLiteral("max_line_length="))) {
            bool ok = false;
            const int parsed = value_after_equals(line).toInt(&ok);
            if (ok && parsed >= 0) {
                config.max_line_length = parsed > 512 ? 512 : parsed;
            }
        } else if (line.startsWith(QStringLiteral("show_color_codes="))) {
            config.show_color_codes = parse_bool(value_after_equals(line));
        } else if (line.startsWith(QStringLiteral("check_updates="))) {
            config.check_updates = parse_bool(value_after_equals(line));
        } else if (line.startsWith(QStringLiteral("last_update_check="))) {
            config.last_update_check = value_after_equals(line);
        } else if (line.startsWith(QStringLiteral("dismissed_update_version="))) {
            config.dismissed_update_version = value_after_equals(line);
        } else if (line.startsWith(QStringLiteral("ui_language="))) {
            config.ui_language = value_after_equals(line);
        } else if (line.startsWith(QStringLiteral("aree_workspace_root="))) {
            config.aree_workspace_root = value_after_equals(line);
        } else if (line.startsWith(QStringLiteral("aree_last_area="))) {
            config.aree_last_area = value_after_equals(line);
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
    out << "# Nebbie Editor configuration\n";
    out << "lib_path=" << config.lib_path << '\n';
    if (!config.index_url.isEmpty()) {
        out << "index_url=" << config.index_url << '\n';
    }
    if (!config.coordinator_url.isEmpty()) {
        out << "coordinator_url=" << config.coordinator_url << '\n';
    }
    if (!config.coordinator_token.isEmpty()) {
        out << "coordinator_token=" << config.coordinator_token << '\n';
    }
    if (!config.builder_name.isEmpty()) {
        out << "builder_name=" << config.builder_name << '\n';
    }
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
    if (!config.aree_workspace_root.isEmpty()) {
        out << "aree_workspace_root=" << config.aree_workspace_root << '\n';
    }
    if (!config.aree_last_area.isEmpty()) {
        out << "aree_last_area=" << config.aree_last_area << '\n';
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
    const std::filesystem::path candidate = path_from_qstring(path);
    if (!std::filesystem::exists(candidate, ec)) {
        return false;
    }

    const std::filesystem::path resolved = nebbie::resolve_lib_directory(candidate);
    return nebbie::directory_has_lib_files(resolved);
}

} // namespace nebbie::qt
