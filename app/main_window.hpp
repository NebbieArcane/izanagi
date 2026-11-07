#pragma once

#include "app_theme.hpp"
#include "eleutheria/catalog.hpp"
#include "eleutheria/types.hpp"
#include "eleutheria/workspace.hpp"

#include <QMainWindow>
#include <QString>
#include <optional>
#include <vector>

class QAction;
class QActionGroup;
class QMenu;
class QListWidget;
class QTableWidget;
class QTableWidgetItem;
class QTabWidget;
class QLabel;
class QDockWidget;
class RoomEditor;
class MobEditor;
class ObjEditor;
class ZoneEditor;

class MainWindow : public QMainWindow {
  Q_OBJECT
 public:
  explicit MainWindow(QWidget* parent = nullptr);
  void openPath(const QString& path);

 protected:
  void closeEvent(QCloseEvent* event) override;

 private slots:
  void newArea();
  void openArea();
  void openAreaFile();
  void openWorkspace();
  void saveArea();
  void saveAreaAs();
  void validateArea();
  void about();

  void onRoomSelected();
  void onMobSelected();
  void onObjSelected();
  void onZoneSelected();
  void onWorkspaceAreaActivated(QTableWidgetItem* item);
  void applyCurrentEditor();
  void addRoom();
  void addMob();
  void addObj();
  void deleteCurrent();
  void setTheme(AppTheme theme);
  void setWorkspaceColumnVisible(int logical_column, bool visible);
  void editLineLengthLimit();

 private:
  void buildUi();
  void applyCurrentTheme();
  void applyMaxLineLength();
  void loadMaxLineLength();
  void saveMaxLineLength();
  void setArea(eleutheria::Area area);
  void refreshLists();
  void refreshWorkspaceList();
  void setupWorkspaceColumnsMenu(QMenu* parent_menu);
  void applyWorkspaceColumnVisibility();
  void saveWorkspaceTableState();
  void restoreWorkspaceTableState();
  void updateTitle();
  bool maybeSave();
  void markDirty();
  void loadWorkspace(const QString& path);
  void openAreaFromWorkspace(const QString& area_path);
  void saveLastWorkspacePath(const QString& path);
  void restoreLastWorkspace();

  eleutheria::Catalog catalog_;
  std::optional<eleutheria::Area> area_;
  QString current_base_;

  std::optional<eleutheria::Workspace> workspace_;
  std::vector<eleutheria::WorkspaceArea> workspace_areas_;

  QTabWidget* tabs_ = nullptr;
  QListWidget* room_list_ = nullptr;
  QListWidget* mob_list_ = nullptr;
  QListWidget* obj_list_ = nullptr;
  QListWidget* zone_list_ = nullptr;
  QTableWidget* workspace_table_ = nullptr;
  QDockWidget* workspace_dock_ = nullptr;
  QAction* workspace_toggle_action_ = nullptr;
  std::vector<QAction*> workspace_column_actions_;
  RoomEditor* room_editor_ = nullptr;
  MobEditor* mob_editor_ = nullptr;
  ObjEditor* obj_editor_ = nullptr;
  ZoneEditor* zone_editor_ = nullptr;
  QLabel* status_counts_ = nullptr;
  QActionGroup* theme_group_ = nullptr;

  AppTheme current_theme_ = AppTheme::Dark;
  int max_line_length_ = 0;

  std::optional<std::int64_t> current_room_;
  std::optional<std::int64_t> current_mob_;
  std::optional<std::int64_t> current_obj_;
  std::optional<int> current_zone_;
};
