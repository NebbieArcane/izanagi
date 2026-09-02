#include "release_update_util.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <iostream>
#include <stdexcept>

namespace {

void expect(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

QByteArray sampleReleaseJson(const QString& prefix, const QStringList& asset_names) {
    QJsonObject root;
    root.insert(QStringLiteral("html_url"),
                QStringLiteral("https://github.com/NebbieArcane/izanagi/releases/tag/%1").arg(prefix));
    QJsonArray assets;
    for (const QString& name : asset_names) {
        QJsonObject asset;
        asset.insert(QStringLiteral("name"), name);
        asset.insert(QStringLiteral("browser_download_url"),
                     QStringLiteral("https://example.com/%1").arg(name));
        assets.append(asset);
    }
    root.insert(QStringLiteral("assets"), assets);
    return QJsonDocument(root).toJson();
}

} // namespace

int main() {
    try {
        using nebbie::qt::ReleaseProduct;
        using nebbie::qt::compareVersions;
        using nebbie::qt::parseReleaseResponse;
        using nebbie::qt::parseVersionFromAssetName;
        using nebbie::qt::platformAssetSuffix;
        using nebbie::qt::shouldCheckForUpdates;

        expect(compareVersions(QStringLiteral("0.1.0"), QStringLiteral("0.1.1")) < 0, "0.1.0 < 0.1.1");
        expect(compareVersions(QStringLiteral("1.0.0"), QStringLiteral("0.9.9")) > 0, "1.0.0 > 0.9.9");
        expect(compareVersions(QStringLiteral("0.2.10"), QStringLiteral("0.2.9")) > 0, "0.2.10 > 0.2.9");
        expect(compareVersions(QStringLiteral("0.1.0"), QStringLiteral("0.1.0")) == 0, "equal versions");

        QString parsed;
        expect(parseVersionFromAssetName(QStringLiteral("izanagi_0.1.0_amd64.deb"),
                                         QStringLiteral("izanagi"),
                                         &parsed),
               "parse izanagi deb");
        expect(parsed == QStringLiteral("0.1.0"), "parsed version 0.1.0");
        expect(parseVersionFromAssetName(QStringLiteral("cypher_1.2.3_macos.dmg"),
                                         QStringLiteral("cypher"),
                                         &parsed),
               "parse cypher dmg");
        expect(parsed == QStringLiteral("1.2.3"), "parsed version 1.2.3");

        const QString platform_suffix = platformAssetSuffix();
        const QByteArray release_body = sampleReleaseJson(
            QStringLiteral("izanagi"),
            {QStringLiteral("izanagi_0.1.0%1").arg(platform_suffix),
             QStringLiteral("izanagi_0.2.0%1").arg(platform_suffix)});
        const auto info =
            parseReleaseResponse(release_body, ReleaseProduct::Izanagi, QStringLiteral("0.1.0"));
        expect(info.ok, "release parse ok");
        expect(info.latest_version == QStringLiteral("0.2.0"), "latest version 0.2.0");
        expect(info.update_available, "update available");

        const auto up_to_date =
            parseReleaseResponse(release_body, ReleaseProduct::Izanagi, QStringLiteral("0.2.0"));
        expect(up_to_date.ok, "up to date parse ok");
        expect(!up_to_date.update_available, "no update when current");

        const QByteArray same_version_body = sampleReleaseJson(
            QStringLiteral("izanagi"),
            {QStringLiteral("izanagi_0.1.13%1").arg(platform_suffix)});
        QJsonDocument same_doc = QJsonDocument::fromJson(same_version_body);
        QJsonObject same_root = same_doc.object();
        same_root.insert(QStringLiteral("published_at"), QStringLiteral("2026-09-02T12:00:00Z"));
        const auto same_version = parseReleaseResponse(QJsonDocument(same_root).toJson(),
                                                       ReleaseProduct::Izanagi,
                                                       QStringLiteral("0.1.13"),
                                                       QStringLiteral("2026-09-01T00:00:00Z"));
        expect(same_version.ok, "same version parse ok");
        expect(!same_version.update_available, "no update when versions match");

        const auto run_number_update = parseReleaseResponse(
            release_body, ReleaseProduct::Izanagi, QStringLiteral("0.1.100"));
        expect(run_number_update.update_available, "higher patch detects update");

        expect(shouldCheckForUpdates(true, true, QStringLiteral("2026-08-24T00:00:00Z")), "interactive always checks");
        expect(!shouldCheckForUpdates(false, false, {}), "disabled startup check");
        expect(shouldCheckForUpdates(true, false, {}), "first startup check");

        std::cout << "OK\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "FAILED: " << ex.what() << '\n';
        return 1;
    }
}
