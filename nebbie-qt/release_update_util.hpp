#pragma once

#include <QByteArray>
#include <QString>

namespace nebbie::qt {

enum class ReleaseProduct {
    Izanagi,
    Cypher,
};

struct ReleaseUpdateInfo {
    bool ok = false;
    QString error;
    bool update_available = false;
    QString current_version;
    QString latest_version;
    QString download_url;
    QString release_page_url;
    QString user_dismissed_version;
};

QString releaseProductPrefix(ReleaseProduct product);
QString releaseProductDisplayName(ReleaseProduct product);
QString releaseTag(ReleaseProduct product);
QString platformAssetSuffix();
int compareVersions(const QString& left, const QString& right);
bool parseVersionFromAssetName(const QString& asset_name, const QString& prefix, QString* version_out);
ReleaseUpdateInfo parseReleaseResponse(const QByteArray& body,
                                       ReleaseProduct product,
                                       const QString& current_version);
bool shouldCheckForUpdates(bool check_enabled,
                           bool interactive,
                           const QString& last_check_iso,
                           int min_interval_hours = 24);

} // namespace nebbie::qt
