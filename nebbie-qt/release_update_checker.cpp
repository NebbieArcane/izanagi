#include "release_update_checker.hpp"

#include "version.hpp"

#include <QDesktopServices>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

namespace nebbie::qt {

namespace {

QNetworkRequest makeReleaseRequest(const QUrl& url) {
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("NebbieEditor/%1").arg(QStringLiteral(NEBBIE_VERSION)));
    request.setRawHeader("Accept", "application/vnd.github+json");
    return request;
}

} // namespace

ReleaseUpdateChecker::ReleaseUpdateChecker(const ReleaseProduct product, QObject* parent)
    : QObject(parent), product_(product) {}

void ReleaseUpdateChecker::checkForUpdates(QNetworkAccessManager& network,
                                           const bool interactive,
                                           QWidget* dialog_parent,
                                           const QString& dismissed_version) {
    if (active_reply_) {
        active_reply_->abort();
        active_reply_->deleteLater();
        active_reply_ = nullptr;
    }

    interactive_ = interactive;
    dialog_parent_ = dialog_parent;
    dismissed_version_ = dismissed_version;

    const QUrl url(QStringLiteral("https://api.github.com/repos/wizardmorgan/nebbie-editor/releases/tags/%1")
                       .arg(releaseTag(product_)));
    active_reply_ = network.get(makeReleaseRequest(url));
    connect(active_reply_, &QNetworkReply::finished, this, &ReleaseUpdateChecker::onReplyFinished);
}

void ReleaseUpdateChecker::onReplyFinished() {
    if (!active_reply_) {
        return;
    }

    QNetworkReply* reply = active_reply_;
    active_reply_ = nullptr;

    if (reply->error() != QNetworkReply::NoError) {
        const QString message = reply->errorString();
        reply->deleteLater();
        finishWithError(message);
        return;
    }

    const QByteArray body = reply->readAll();
    reply->deleteLater();

    const ReleaseUpdateInfo info =
        parseReleaseResponse(body, product_, QStringLiteral(NEBBIE_VERSION));
    if (!info.ok) {
        finishWithError(info.error);
        return;
    }

    finishWithInfo(info);
}

void ReleaseUpdateChecker::finishWithError(const QString& message) {
    ReleaseUpdateInfo info;
    info.error = message;
    finishWithInfo(info);
}

void ReleaseUpdateChecker::finishWithInfo(const ReleaseUpdateInfo& info) {
    ReleaseUpdateInfo result = info;
    if (info.ok && info.update_available && info.latest_version == dismissed_version_) {
        emit checkFinished(result);
        return;
    }
    if (interactive_ || info.update_available) {
        result = showResultDialog(info);
    }
    emit checkFinished(result);
}

ReleaseUpdateInfo ReleaseUpdateChecker::showResultDialog(const ReleaseUpdateInfo& info) {
    ReleaseUpdateInfo result = info;
    QWidget* parent = dialog_parent_;
    const QString app_name = releaseProductDisplayName(product_);

    if (!info.ok) {
        if (!interactive_) {
            return result;
        }
        QMessageBox::warning(parent,
                             QStringLiteral("Aggiornamenti %1").arg(app_name),
                             QStringLiteral("Impossibile verificare gli aggiornamenti:\n%1").arg(info.error));
        return result;
    }

    if (info.update_available) {
        QMessageBox dialog(QMessageBox::Information,
                           QStringLiteral("Aggiornamento disponibile"),
                           QStringLiteral("%1 %2 è disponibile (versione attuale: %3).\n\n"
                                            "Scarica il pacchetto e installalo per restare aggiornato.")
                               .arg(app_name, info.latest_version, info.current_version),
                           QMessageBox::NoButton,
                           parent);
        auto* download_button =
            dialog.addButton(QStringLiteral("Scarica aggiornamento"), QMessageBox::AcceptRole);
        dialog.addButton(QStringLiteral("Più tardi"), QMessageBox::RejectRole);
        auto* ignore_button =
            dialog.addButton(QStringLiteral("Ignora questa versione"), QMessageBox::DestructiveRole);
        dialog.setDefaultButton(download_button);
        dialog.exec();

        const auto role = dialog.buttonRole(dialog.clickedButton());
        if (role == QMessageBox::AcceptRole) {
            if (!info.download_url.isEmpty()) {
                QDesktopServices::openUrl(QUrl(info.download_url));
            } else if (!info.release_page_url.isEmpty()) {
                QDesktopServices::openUrl(QUrl(info.release_page_url));
            }
            return result;
        }

        if (role == QMessageBox::DestructiveRole) {
            result.user_dismissed_version = info.latest_version;
        }
        return result;
    }

    if (interactive_) {
        QMessageBox::information(parent,
                                 QStringLiteral("Aggiornamenti %1").arg(app_name),
                                 QStringLiteral("Stai usando l'ultima versione (%1).").arg(info.current_version));
    }
    return result;
}

} // namespace nebbie::qt
