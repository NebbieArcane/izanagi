#include "release_update_util.hpp"

#include "app_i18n.hpp"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

namespace nebbie::qt {

namespace {

int parseVersionPart(const QString& part) {
    bool ok = false;
    const int value = part.trimmed().toInt(&ok);
    return ok ? value : 0;
}

} // namespace

QString releaseProductPrefix(const ReleaseProduct product) {
    switch (product) {
    case ReleaseProduct::Izanagi:
        return QStringLiteral("izanagi");
    case ReleaseProduct::Cypher:
        return QStringLiteral("cypher");
    }
    return {};
}

QString releaseProductDisplayName(const ReleaseProduct product) {
    switch (product) {
    case ReleaseProduct::Izanagi:
        return QStringLiteral("Izanagi");
    case ReleaseProduct::Cypher:
        return QStringLiteral("Cypher");
    }
    return {};
}

QString releaseTag(const ReleaseProduct product) {
    return releaseProductPrefix(product);
}

QString platformAssetSuffix() {
#if defined(Q_OS_WIN)
    return QStringLiteral("_windows_portable.zip");
#elif defined(Q_OS_MACOS)
    return QStringLiteral("_macos.dmg");
#else
    return QStringLiteral("_amd64.deb");
#endif
}

int compareVersions(const QString& left, const QString& right) {
    const QStringList left_parts = left.split('.');
    const QStringList right_parts = right.split('.');
    const int count = qMax(left_parts.size(), right_parts.size());
    for (int index = 0; index < count; ++index) {
        const int left_value = index < left_parts.size() ? parseVersionPart(left_parts[index]) : 0;
        const int right_value = index < right_parts.size() ? parseVersionPart(right_parts[index]) : 0;
        if (left_value < right_value) {
            return -1;
        }
        if (left_value > right_value) {
            return 1;
        }
    }
    return 0;
}

bool parseVersionFromAssetName(const QString& asset_name,
                               const QString& prefix,
                               QString* version_out) {
    const QRegularExpression pattern(
        QStringLiteral("^(?:%1)_([0-9]+\\.[0-9]+\\.[0-9]+)_").arg(QRegularExpression::escape(prefix)));
    const QRegularExpressionMatch match = pattern.match(asset_name);
    if (!match.hasMatch()) {
        return false;
    }
    if (version_out) {
        *version_out = match.captured(1);
    }
    return true;
}

ReleaseUpdateInfo parseReleaseResponse(const QByteArray& body,
                                       const ReleaseProduct product,
                                       const QString& current_version) {
    ReleaseUpdateInfo info;
    info.current_version = current_version;

    QJsonParseError parse_error;
    const QJsonDocument document = QJsonDocument::fromJson(body, &parse_error);
    if (parse_error.error != QJsonParseError::NoError || !document.isObject()) {
        info.error = appTr("update.invalid_response");
        return info;
    }

    const QJsonObject root = document.object();
    info.release_page_url = root.value(QStringLiteral("html_url")).toString();

    const QString prefix = releaseProductPrefix(product);
    const QString suffix = platformAssetSuffix();
    QString best_version;
    QString best_url;

    const QJsonArray assets = root.value(QStringLiteral("assets")).toArray();
    for (const QJsonValue& asset_value : assets) {
        const QJsonObject asset = asset_value.toObject();
        const QString name = asset.value(QStringLiteral("name")).toString();
        if (!name.startsWith(prefix + QLatin1Char('_')) || !name.endsWith(suffix)) {
            continue;
        }

        QString parsed_version;
        if (!parseVersionFromAssetName(name, prefix, &parsed_version)) {
            continue;
        }

        if (best_version.isEmpty() || compareVersions(parsed_version, best_version) > 0) {
            best_version = parsed_version;
            best_url = asset.value(QStringLiteral("browser_download_url")).toString();
        }
    }

    if (best_version.isEmpty() || best_url.isEmpty()) {
        info.error = appTr("update.no_package", prefix);
        return info;
    }

    info.ok = true;
    info.latest_version = best_version;
    info.download_url = best_url;
    info.update_available = compareVersions(current_version, best_version) < 0;
    return info;
}

bool shouldCheckForUpdates(const bool check_enabled,
                           const bool interactive,
                           const QString& last_check_iso,
                           const int min_interval_hours) {
    if (interactive) {
        return true;
    }
    if (!check_enabled) {
        return false;
    }
    if (last_check_iso.isEmpty()) {
        return true;
    }

    const QDateTime last_check = QDateTime::fromString(last_check_iso, Qt::ISODate);
    if (!last_check.isValid()) {
        return true;
    }

    return last_check.secsTo(QDateTime::currentDateTimeUtc()) >= min_interval_hours * 3600;
}

} // namespace nebbie::qt
