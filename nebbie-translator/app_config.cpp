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
    return std::filesystem::exists(resolved / "myst.wld", ec)
           || std::filesystem::exists(resolved / "myst.zon", ec);
}

} // namespace nebbie::translate
