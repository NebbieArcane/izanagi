#pragma once

#include "release_update_util.hpp"

#include <QObject>

class QNetworkAccessManager;
class QNetworkReply;
class QWidget;

namespace nebbie::qt {

class ReleaseUpdateChecker : public QObject {
    Q_OBJECT

public:
    explicit ReleaseUpdateChecker(ReleaseProduct product, QObject* parent = nullptr);

    void checkForUpdates(QNetworkAccessManager& network,
                         bool interactive,
                         QWidget* dialog_parent = nullptr,
                         const QString& dismissed_version = {});

signals:
    void checkFinished(const nebbie::qt::ReleaseUpdateInfo& info);

private slots:
    void onReplyFinished();

private:
    void finishWithError(const QString& message);
    void finishWithInfo(const ReleaseUpdateInfo& info);
    ReleaseUpdateInfo showResultDialog(const ReleaseUpdateInfo& info);

    ReleaseProduct product_;
    bool interactive_ = false;
    QWidget* dialog_parent_ = nullptr;
    QString dismissed_version_;
    ::QNetworkReply* active_reply_ = nullptr;
};

} // namespace nebbie::qt
