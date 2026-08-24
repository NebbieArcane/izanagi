#pragma once

#include "app_config.hpp"
#include "release_update_checker.hpp"
#include "nebbie/lib_context.hpp"
#include "nebbie/session.hpp"
#include "nebbie/validate.hpp"
#include "nebbie/world.hpp"

#include <QMainWindow>
#include <QTimer>

class QNetworkAccessManager;

#include <chrono>
#include <filesystem>
#include <set>

class QCloseEvent;
class QLabel;
class QLineEdit;
class QListWidget;
class TranslatorRoomWidget;

class TranslatorWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit TranslatorWindow(QWidget* parent = nullptr);
    void openLibPath(const QString& path);
    void openStartupLib();
    bool promptForLibPath(const QString& reason = {});

public slots:
    void openLib();
    void reloadLib();
    void saveLib();
    void saveLibForce();
    void validateLib();
    void onAutosaveTick();

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onRoomSelected();
    void applyRoomChanges();
    void onRoomSearchChanged(const QString& text);
    void editLineLengthLimit();
    void toggleExtendedColorView(bool enabled);
    void showColorLegend();
    void insertColorCode();
    void checkForUpdates();
    void onUpdateCheckFinished(const nebbie::qt::ReleaseUpdateInfo& info);
    void toggleCheckUpdatesOnStartup(bool enabled);

private:
    void setupUi();
    void setupMenus();
    void scheduleStartupUpdateCheck();
    void loadLib(const std::filesystem::path& path);
    void rememberLibPath(const std::filesystem::path& path);
    void refreshRoomList();
    void selectRoomByVnum(long vnum);
    long currentRoomVnum() const;
    void markDirty();
    void markClean();
    bool confirmSaveIfDirty();
    void setStatus(const QString& text);
    void showValidation(const nebbie::ValidationReport& report);
    nebbie::ValidationOptions validationOptions() const;
    std::vector<long> roomsPendingSaveValidation() const;

    nebbie::translate::AppConfig app_config_;
    nebbie::World world_;
    nebbie::LibContext context_;
    std::filesystem::path lib_path_;
    nebbie::SessionConfig session_config_;
    std::chrono::system_clock::time_point last_version_time_;
    bool dirty_ = false;
    std::set<long> dirty_room_vnums_;
    QNetworkAccessManager* network_ = nullptr;
    nebbie::qt::ReleaseUpdateChecker* update_checker_ = nullptr;

    QLabel* lib_label_ = nullptr;
    QLineEdit* room_search_ = nullptr;
    QListWidget* room_list_ = nullptr;
    TranslatorRoomWidget* room_editor_ = nullptr;
    QTimer* autosave_timer_ = nullptr;
    QString room_filter_;
};
