#include "main_window.hpp"

#include "app_theme.hpp"
#include "editors.hpp"
#include "file_dialogs.hpp"

#include "eleutheria/area_io.hpp"
#include "eleutheria/workspace.hpp"
#include "eleutheria/world.hpp"

#include <QApplication>
#include <QCloseEvent>
#include <QColor>
#include <QDockWidget>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QAbstractItemView>
#include <QActionGroup>
#include <QHeaderView>
#include <QListWidget>
#include <QListWidgetItem>
#include <QInputDialog>
#include <QMessageBox>
#include <QMenu>
#include <QMenuBar>
#include <QSettings>
#include <QSignalBlocker>
#include <QSplitter>
#include <QStatusBar>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTabWidget>
#include <QToolBar>
#include <QVariant>
#include <QVBoxLayout>

#include <filesystem>
#include <stdexcept>

namespace {

constexpr int kWorkspaceColumnCount = 6;
constexpr int kWorkspacePathRole = Qt::UserRole + 32;
constexpr const char* kWorkspaceHeaderStateKey = "ui/workspaceHeaderState";
constexpr const char* kWorkspaceColumnVisibleKey = "ui/workspaceColumnVisible";
constexpr const char* kLastWorkspacePathKey = "ui/lastWorkspacePath";
constexpr const char* kMaxLineLengthKey = "ui/maxLineLength";

const QStringList& workspace_column_labels() {
  static const QStringList labels = {
      QStringLiteral("Cartella"), QStringLiteral("Nome"), QStringLiteral("Autore"),
      QStringLiteral("Zona"),     QStringLiteral("Da"),   QStringLiteral("A"),
  };
  return labels;
}

class SortableTableItem : public QTableWidgetItem {
 public:
  using QTableWidgetItem::QTableWidgetItem;
  bool operator<(const QTableWidgetItem& other) const override {
    const QVariant a = data(Qt::UserRole);
    const QVariant b = other.data(Qt::UserRole);
    if(a.isValid() && b.isValid() && a.userType() == b.userType()) {
      if(a.userType() == QMetaType::LongLong || a.userType() == QMetaType::Int) {
        return a.toLongLong() < b.toLongLong();
      }
      return a.toString().localeAwareCompare(b.toString()) < 0;
    }
    return QTableWidgetItem::operator<(other);
  }
};

SortableTableItem* make_workspace_item(const QString& text, const QVariant& sort_key,
                                       const QString& path) {
  auto* item = new SortableTableItem(text);
  item->setData(Qt::UserRole, sort_key);
  item->setData(kWorkspacePathRole, path);
  item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  if(!path.isEmpty()) {
    item->setToolTip(path);
  }
  return item;
}

QString strip_zone_author(QString name, QString* author_out) {
  const int open = name.lastIndexOf(QLatin1Char('['));
  const int close = name.lastIndexOf(QLatin1Char(']'));
  if(open >= 0 && close > open) {
    if(author_out != nullptr) {
      *author_out = name.mid(open + 1, close - open - 1).trimmed();
    }
    name = name.left(open).trimmed();
  } else if(author_out != nullptr) {
    author_out->clear();
  }
  return name;
}

}  // namespace

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), catalog_(eleutheria::Catalog::nebbie_defaults()) {
  setWindowTitle("Eleutheria (Nebbie Arcane)");
  resize(1180, 760);
  current_theme_ = load_saved_theme();
  applyCurrentTheme();
  buildUi();
  loadMaxLineLength();
  applyMaxLineLength();
  newArea();
  restoreLastWorkspace();
}

void MainWindow::applyCurrentTheme() {
  apply_app_theme(this, current_theme_);
}

void MainWindow::setTheme(AppTheme theme) {
  current_theme_ = theme;
  save_theme(theme);
  applyCurrentTheme();
  if(theme_group_ != nullptr) {
    for(auto* action : theme_group_->actions()) {
      if(static_cast<AppTheme>(action->data().toInt()) == theme) {
        action->setChecked(true);
        break;
      }
    }
  }
}

void MainWindow::loadMaxLineLength() {
  QSettings settings(QStringLiteral("NebbieArcane"), QStringLiteral("Eleutheria"));
  int value = settings.value(QString::fromUtf8(kMaxLineLengthKey), 0).toInt();
  if(value < 0) {
    value = 0;
  }
  if(value > 512) {
    value = 512;
  }
  max_line_length_ = value;
}

void MainWindow::saveMaxLineLength() {
  QSettings settings(QStringLiteral("NebbieArcane"), QStringLiteral("Eleutheria"));
  settings.setValue(QString::fromUtf8(kMaxLineLengthKey), max_line_length_);
}

void MainWindow::applyMaxLineLength() {
  if(room_editor_ != nullptr) {
    room_editor_->setMaxLineLength(max_line_length_);
  }
  if(mob_editor_ != nullptr) {
    mob_editor_->setMaxLineLength(max_line_length_);
  }
  if(obj_editor_ != nullptr) {
    obj_editor_->setMaxLineLength(max_line_length_);
  }
}

void MainWindow::editLineLengthLimit() {
  bool ok = false;
  const int value = QInputDialog::getInt(
      this, QStringLiteral("Limite caratteri per riga"),
      QStringLiteral(
          "Numero massimo di caratteri per riga nei testi MUD\n"
          "(descrizioni stanze, mob, oggetti).\n"
          "Imposta 0 per disattivare la barra."),
      max_line_length_, 0, 512, 1, &ok);
  if(!ok) {
    return;
  }
  max_line_length_ = value;
  saveMaxLineLength();
  applyMaxLineLength();
  if(max_line_length_ > 0) {
    statusBar()->showMessage(
        QStringLiteral("Limite righe impostato a %1 caratteri.").arg(max_line_length_), 5000);
  } else {
    statusBar()->showMessage(QStringLiteral("Barra limite caratteri disattivata."), 5000);
  }
}

void MainWindow::buildUi() {
  auto* file_menu = menuBar()->addMenu("&File");
  file_menu->addAction("&Nuova area", QKeySequence::New, this, &MainWindow::newArea);
  file_menu->addAction("Apri &workspace Aree...", QKeySequence::Open, this,
                       &MainWindow::openWorkspace);
  file_menu->addAction("Apri cartella &area/lib...", this, &MainWindow::openArea);
  file_menu->addAction("Apri &file area...", this, &MainWindow::openAreaFile);
  file_menu->addAction("&Salva", QKeySequence::Save, this, &MainWindow::saveArea);
  file_menu->addAction("Salva &come...", this, &MainWindow::saveAreaAs);
  file_menu->addSeparator();
  file_menu->addAction("&Esci", QKeySequence::Quit, this, &QWidget::close);

  auto* view_menu = menuBar()->addMenu("&Visualizza");
  view_menu->addAction("Stanze", this, [this]() { tabs_->setCurrentIndex(0); });
  view_menu->addAction("Mob", this, [this]() { tabs_->setCurrentIndex(1); });
  view_menu->addAction("Oggetti", this, [this]() { tabs_->setCurrentIndex(2); });
  view_menu->addAction("Zone", this, [this]() { tabs_->setCurrentIndex(3); });
  view_menu->addSeparator();
  workspace_toggle_action_ =
      view_menu->addAction(QStringLiteral("Aree workspace"), this, [this]() {
        if(workspace_dock_ == nullptr) {
          return;
        }
        workspace_dock_->setVisible(!workspace_dock_->isVisible());
      });
  workspace_toggle_action_->setCheckable(true);
  workspace_toggle_action_->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+A")));
  workspace_toggle_action_->setStatusTip(
      QStringLiteral("Mostra/nascondi il pannello aree (Ctrl+Shift+A)"));
  view_menu->addSeparator();
  auto* theme_menu = view_menu->addMenu(QStringLiteral("Tema"));
  theme_group_ = new QActionGroup(this);
  theme_group_->setExclusive(true);
  const AppTheme themes[] = {AppTheme::Dark, AppTheme::Light, AppTheme::ClassicGray};
  for(AppTheme theme : themes) {
    auto* action = theme_menu->addAction(theme_label(theme));
    action->setCheckable(true);
    action->setData(static_cast<int>(theme));
    theme_group_->addAction(action);
    if(theme == current_theme_) {
      action->setChecked(true);
    }
    connect(action, &QAction::triggered, this, [this, theme]() { setTheme(theme); });
  }
  setupWorkspaceColumnsMenu(view_menu);

  auto* tools_menu = menuBar()->addMenu("&Strumenti");
  tools_menu->addAction("&Valida area", this, &MainWindow::validateArea);
  tools_menu->addAction("Applica modifiche editor", this, &MainWindow::applyCurrentEditor);
  tools_menu->addSeparator();
  tools_menu->addAction("Limite caratteri per &riga...", this, &MainWindow::editLineLengthLimit);

  auto* help_menu = menuBar()->addMenu("&?");
  help_menu->addAction("Informazioni su Eleutheria...", this, &MainWindow::about);

  auto* tb = addToolBar("Main");
  tb->setMovable(false);
  tb->addAction("Nuova", this, &MainWindow::newArea);
  tb->addAction("Apri workspace", this, &MainWindow::openWorkspace);
  if(workspace_toggle_action_ != nullptr) {
    workspace_toggle_action_->setText(QStringLiteral("Aree"));
    tb->addAction(workspace_toggle_action_);
  }
  tb->addAction("Apri area", this, &MainWindow::openArea);
  tb->addAction("Apri file", this, &MainWindow::openAreaFile);
  tb->addAction("Salva", this, &MainWindow::saveArea);
  tb->addSeparator();
  tb->addAction("+ Stanza", this, &MainWindow::addRoom);
  tb->addAction("+ Mob", this, &MainWindow::addMob);
  tb->addAction("+ Oggetto", this, &MainWindow::addObj);
  tb->addAction("Elimina", this, &MainWindow::deleteCurrent);
  tb->addSeparator();
  tb->addAction("Valida", this, &MainWindow::validateArea);

  tabs_ = new QTabWidget(this);
  setCentralWidget(tabs_);

  workspace_dock_ = new QDockWidget(QStringLiteral("Aree (workspace)"), this);
  workspace_dock_->setObjectName(QStringLiteral("workspaceDock"));
  workspace_dock_->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable |
                               QDockWidget::DockWidgetFloatable);
  workspace_table_ = new QTableWidget(0, kWorkspaceColumnCount, workspace_dock_);
  workspace_table_->setHorizontalHeaderLabels(workspace_column_labels());
  workspace_table_->setSelectionBehavior(QAbstractItemView::SelectRows);
  workspace_table_->setSelectionMode(QAbstractItemView::SingleSelection);
  workspace_table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  workspace_table_->setAlternatingRowColors(true);
  workspace_table_->setSortingEnabled(true);
  workspace_table_->verticalHeader()->setVisible(false);
  auto* header = workspace_table_->horizontalHeader();
  header->setStretchLastSection(true);
  header->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  header->setSectionsMovable(true);
  header->setSectionsClickable(true);
  header->setSortIndicatorShown(true);
  header->setSectionResizeMode(QHeaderView::Interactive);
  header->setMinimumSectionSize(48);
  for(int col = 0; col < kWorkspaceColumnCount; ++col) {
    header->resizeSection(col, col == 1 ? 180 : 90);
  }
  workspace_table_->setMinimumWidth(420);
  workspace_dock_->setWidget(workspace_table_);
  addDockWidget(Qt::LeftDockWidgetArea, workspace_dock_);
  workspace_dock_->hide();
  restoreWorkspaceTableState();
  applyWorkspaceColumnVisibility();
  connect(header, &QHeaderView::sectionMoved, this, [this](int, int, int) {
    saveWorkspaceTableState();
  });
  if(workspace_toggle_action_ != nullptr) {
    workspace_toggle_action_->setChecked(false);
    connect(workspace_dock_, &QDockWidget::visibilityChanged, workspace_toggle_action_,
            &QAction::setChecked);
  }
  // Solo itemDoubleClicked: cellDoubleClicked + item(row,0) dopo sort apre l'area sbagliata.
  connect(workspace_table_, &QTableWidget::itemDoubleClicked, this,
          &MainWindow::onWorkspaceAreaActivated);

  auto make_page = [&](QListWidget** list, QWidget* editor) {
    auto* page = new QWidget(this);
    auto* split = new QSplitter(Qt::Horizontal, page);
    *list = new QListWidget(split);
    (*list)->setMinimumWidth(240);
    auto* right = new QWidget(split);
    auto* right_layout = new QVBoxLayout(right);
    right_layout->setContentsMargins(4, 4, 4, 4);
    right_layout->addWidget(editor);
    split->addWidget(*list);
    split->addWidget(right);
    split->setStretchFactor(1, 1);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(split);
    return page;
  };

  room_editor_ = new RoomEditor(catalog_, this);
  mob_editor_ = new MobEditor(catalog_, this);
  obj_editor_ = new ObjEditor(catalog_, this);
  zone_editor_ = new ZoneEditor(this);
  connect(room_editor_, &RoomEditor::areaResetsChanged, this, [this]() {
    markDirty();
    if(current_room_ && area_) {
      area_->dirty_rooms.insert(*current_room_);
    }
    if(area_) {
      // I reset vivono nella zona: marca tutte le zone (di solito una)
      for(const auto& [num, _] : area_->zones) {
        area_->dirty_zones.insert(num);
      }
    }
    if(current_zone_ && area_ && area_->zones.count(*current_zone_)) {
      zone_editor_->load(area_->zones.at(*current_zone_));
    }
  });
  connect(room_editor_, &RoomEditor::navigateRequested, this,
          [this](QChar kind, qint64 vnum) {
            if(!area_) {
              return;
            }
            applyCurrentEditor();
            if(kind == QLatin1Char('R') && area_->rooms.count(vnum)) {
              current_room_ = vnum;
              tabs_->setCurrentIndex(0);
              refreshLists();
              room_editor_->setArea(&*area_);
              room_editor_->load(area_->rooms.at(vnum));
            } else if(kind == QLatin1Char('M') && area_->mobiles.count(vnum)) {
              current_mob_ = vnum;
              tabs_->setCurrentIndex(1);
              refreshLists();
              mob_editor_->load(area_->mobiles.at(vnum));
            } else if(kind == QLatin1Char('O') && area_->objects.count(vnum)) {
              current_obj_ = vnum;
              tabs_->setCurrentIndex(2);
              refreshLists();
              obj_editor_->load(area_->objects.at(vnum));
            }
          });

  tabs_->addTab(make_page(&room_list_, room_editor_), "Stanze");
  tabs_->addTab(make_page(&mob_list_, mob_editor_), "Mob");
  tabs_->addTab(make_page(&obj_list_, obj_editor_), "Oggetti");
  tabs_->addTab(make_page(&zone_list_, zone_editor_), "Zone");

  connect(room_list_, &QListWidget::currentRowChanged, this, &MainWindow::onRoomSelected);
  connect(mob_list_, &QListWidget::currentRowChanged, this, &MainWindow::onMobSelected);
  connect(obj_list_, &QListWidget::currentRowChanged, this, &MainWindow::onObjSelected);
  connect(zone_list_, &QListWidget::currentRowChanged, this, &MainWindow::onZoneSelected);

  connect(room_editor_, &RoomEditor::edited, this, [this]() {
    markDirty();
    if(current_room_ && area_) {
      area_->dirty_rooms.insert(*current_room_);
    }
  });
  connect(mob_editor_, &MobEditor::edited, this, [this]() {
    markDirty();
    if(current_mob_ && area_) {
      area_->dirty_mobs.insert(*current_mob_);
    }
  });
  connect(obj_editor_, &ObjEditor::edited, this, [this]() {
    markDirty();
    if(current_obj_ && area_) {
      area_->dirty_objects.insert(*current_obj_);
    }
  });
  connect(zone_editor_, &ZoneEditor::edited, this, [this]() {
    markDirty();
    if(current_zone_ && area_) {
      area_->dirty_zones.insert(*current_zone_);
    }
  });

  status_counts_ = new QLabel(this);
  statusBar()->addWidget(status_counts_, 1);
}

void MainWindow::setArea(eleutheria::Area area) {
  area_ = std::move(area);
  current_room_.reset();
  current_mob_.reset();
  current_obj_.reset();
  current_zone_.reset();
  room_editor_->setArea(&*area_);
  refreshLists();
  updateTitle();
}

void MainWindow::refreshLists() {
  if(!area_) {
    return;
  }
  const QSignalBlocker block_rooms(room_list_);
  const QSignalBlocker block_mobs(mob_list_);
  const QSignalBlocker block_objs(obj_list_);
  const QSignalBlocker block_zones(zone_list_);

  const QFont mono(QStringLiteral("Menlo"), 11);
  room_list_->setFont(mono);
  mob_list_->setFont(mono);
  obj_list_->setFont(mono);
  zone_list_->setFont(mono);
  room_list_->setUniformItemSizes(true);
  mob_list_->setUniformItemSizes(true);
  obj_list_->setUniformItemSizes(true);
  zone_list_->setUniformItemSizes(true);

  room_list_->clear();
  for(const auto& [vnum, room] : area_->rooms) {
    auto* item = new QListWidgetItem(QString::fromStdString(eleutheria::room_list_label(room)));
    item->setData(Qt::UserRole, QVariant::fromValue(static_cast<qint64>(vnum)));
    item->setToolTip(QString::fromStdString(room.name));
    room_list_->addItem(item);
  }
  mob_list_->clear();
  for(const auto& [vnum, mob] : area_->mobiles) {
    auto* item = new QListWidgetItem(QString::fromStdString(eleutheria::mob_list_label(mob)));
    item->setData(Qt::UserRole, QVariant::fromValue(static_cast<qint64>(vnum)));
    mob_list_->addItem(item);
  }
  obj_list_->clear();
  for(const auto& [vnum, obj] : area_->objects) {
    auto* item = new QListWidgetItem(QString::fromStdString(eleutheria::obj_list_label(obj)));
    item->setData(Qt::UserRole, QVariant::fromValue(static_cast<qint64>(vnum)));
    obj_list_->addItem(item);
  }
  zone_list_->clear();
  for(const auto& [num, zone] : area_->zones) {
    auto* item = new QListWidgetItem(QString::fromStdString(eleutheria::zone_list_label(zone)));
    item->setData(Qt::UserRole, num);
    zone_list_->addItem(item);
  }

  auto select_by_id = [](QListWidget* list, qint64 id) {
    for(int i = 0; i < list->count(); ++i) {
      if(list->item(i)->data(Qt::UserRole).toLongLong() == id) {
        list->setCurrentRow(i);
        return;
      }
    }
  };
  if(current_room_) {
    select_by_id(room_list_, *current_room_);
  }
  if(current_mob_) {
    select_by_id(mob_list_, *current_mob_);
  }
  if(current_obj_) {
    select_by_id(obj_list_, *current_obj_);
  }
  if(current_zone_) {
    select_by_id(zone_list_, *current_zone_);
  }

  status_counts_->setText(
      QString("Stanze: %1   Mob: %2   Oggetti: %3   Zone: %4   Shop: %5")
          .arg(area_->rooms.size())
          .arg(area_->mobiles.size())
          .arg(area_->objects.size())
          .arg(area_->zones.size())
          .arg(area_->shops.size()));
}

void MainWindow::updateTitle() {
  QString title = "Eleutheria (Nebbie Arcane)";
  if(workspace_) {
    title += " [" + QString::fromStdString(workspace_->root.filename().string()) + "]";
  }
  if(area_) {
    title += " - " + QString::fromStdString(area_->meta.name);
    if(area_->dirty) {
      title += " *";
    }
  }
  setWindowTitle(title);
}

void MainWindow::setupWorkspaceColumnsMenu(QMenu* parent_menu) {
  auto* columns_menu = parent_menu->addMenu(QStringLiteral("Colonne workspace"));
  workspace_column_actions_.clear();
  workspace_column_actions_.reserve(kWorkspaceColumnCount);

  QSettings settings(QStringLiteral("NebbieArcane"), QStringLiteral("Eleutheria"));
  const QVariantList saved =
      settings.value(QString::fromUtf8(kWorkspaceColumnVisibleKey)).toList();

  for(int col = 0; col < kWorkspaceColumnCount; ++col) {
    auto* action = columns_menu->addAction(workspace_column_labels().at(col));
    action->setCheckable(true);
    action->setData(col);
    bool visible = true;
    if(col < saved.size()) {
      visible = saved.at(col).toBool();
    }
    action->setChecked(visible);
    workspace_column_actions_.push_back(action);
    connect(action, &QAction::toggled, this, [this, col](bool checked) {
      setWorkspaceColumnVisible(col, checked);
    });
  }
}

void MainWindow::setWorkspaceColumnVisible(int logical_column, bool visible) {
  if(workspace_table_ == nullptr || logical_column < 0 ||
     logical_column >= kWorkspaceColumnCount) {
    return;
  }
  if(!visible) {
    int visible_count = 0;
    for(int col = 0; col < kWorkspaceColumnCount; ++col) {
      if(!workspace_table_->isColumnHidden(col)) {
        ++visible_count;
      }
    }
    if(visible_count <= 1) {
      if(logical_column < static_cast<int>(workspace_column_actions_.size())) {
        const QSignalBlocker block(workspace_column_actions_[static_cast<std::size_t>(logical_column)]);
        workspace_column_actions_[static_cast<std::size_t>(logical_column)]->setChecked(true);
      }
      return;
    }
  }
  workspace_table_->setColumnHidden(logical_column, !visible);
  if(logical_column < static_cast<int>(workspace_column_actions_.size())) {
    const QSignalBlocker block(workspace_column_actions_[static_cast<std::size_t>(logical_column)]);
    workspace_column_actions_[static_cast<std::size_t>(logical_column)]->setChecked(visible);
  }
  saveWorkspaceTableState();
}

void MainWindow::applyWorkspaceColumnVisibility() {
  if(workspace_table_ == nullptr) {
    return;
  }
  for(std::size_t i = 0; i < workspace_column_actions_.size(); ++i) {
    const bool visible = workspace_column_actions_[i]->isChecked();
    workspace_table_->setColumnHidden(static_cast<int>(i), !visible);
  }
}

void MainWindow::saveWorkspaceTableState() {
  if(workspace_table_ == nullptr) {
    return;
  }
  QSettings settings(QStringLiteral("NebbieArcane"), QStringLiteral("Eleutheria"));
  settings.setValue(QString::fromUtf8(kWorkspaceHeaderStateKey),
                    workspace_table_->horizontalHeader()->saveState());
  QVariantList visible;
  visible.reserve(kWorkspaceColumnCount);
  for(int col = 0; col < kWorkspaceColumnCount; ++col) {
    visible.push_back(!workspace_table_->isColumnHidden(col));
  }
  settings.setValue(QString::fromUtf8(kWorkspaceColumnVisibleKey), visible);
}

void MainWindow::restoreWorkspaceTableState() {
  if(workspace_table_ == nullptr) {
    return;
  }
  QSettings settings(QStringLiteral("NebbieArcane"), QStringLiteral("Eleutheria"));
  const QByteArray state =
      settings.value(QString::fromUtf8(kWorkspaceHeaderStateKey)).toByteArray();
  if(!state.isEmpty()) {
    workspace_table_->horizontalHeader()->restoreState(state);
  }
  // Allinea di nuovo a sinistra: restoreState puo' ripristinare allineamenti vecchi
  workspace_table_->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  workspace_table_->horizontalHeader()->setSectionsMovable(true);
}

void MainWindow::refreshWorkspaceList() {
  if(workspace_table_ == nullptr) {
    return;
  }
  const QSignalBlocker block(workspace_table_);
  workspace_table_->setSortingEnabled(false);
  workspace_table_->setRowCount(0);
  workspace_table_->setRowCount(static_cast<int>(workspace_areas_.size()));

  int row = 0;
  for(const auto& area : workspace_areas_) {
    QString author;
    const QString zone_name =
        strip_zone_author(QString::fromStdString(area.zone_name), &author);
    const QString path = QString::fromStdString(area.path.string());
    const QString folder = QString::fromStdString(area.folder_name);

    workspace_table_->setItem(
        row, 0, make_workspace_item(folder, folder.toLower(), path));
    workspace_table_->setItem(
        row, 1, make_workspace_item(zone_name, zone_name.toLower(), path));
    workspace_table_->setItem(
        row, 2, make_workspace_item(author, author.toLower(), path));
    workspace_table_->setItem(
        row, 3,
        make_workspace_item(area.zone_num > 0 ? QString::number(area.zone_num) : QString(),
                            static_cast<qlonglong>(area.zone_num), path));
    workspace_table_->setItem(
        row, 4,
        make_workspace_item(
            area.bottom_vnum > 0 ? QString::number(area.bottom_vnum) : QString(),
            static_cast<qlonglong>(area.bottom_vnum), path));
    workspace_table_->setItem(
        row, 5,
        make_workspace_item(area.top_vnum > 0 ? QString::number(area.top_vnum) : QString(),
                            static_cast<qlonglong>(area.top_vnum), path));
    ++row;
  }

  applyWorkspaceColumnVisibility();
  workspace_table_->setSortingEnabled(true);
  workspace_table_->sortByColumn(0, Qt::AscendingOrder);

  if(workspace_dock_) {
    workspace_dock_->setWindowTitle(
        QStringLiteral("Aree (%1)").arg(workspace_areas_.size()));
    workspace_dock_->show();
    workspace_dock_->raise();
    if(workspace_toggle_action_ != nullptr) {
      workspace_toggle_action_->setChecked(true);
    }
  }
}

void MainWindow::loadWorkspace(const QString& path) {
  const auto root = eleutheria::guess_workspace_root(path.toStdString());
  if(!eleutheria::is_workspace_root(root)) {
    throw std::runtime_error(
        "Cartella non riconosciuta come workspace Aree:\n" + root.string() +
        "\n\nApri la cartella che contiene le aree (es. Aree/ oppure Aree/src/).");
  }
  workspace_ = eleutheria::make_workspace(root);
  workspace_areas_ = eleutheria::scan_workspace_areas(*workspace_);
  if(workspace_areas_.empty()) {
    throw std::runtime_error("Nessuna area trovata in " + root.string());
  }
  saveLastWorkspacePath(QString::fromStdString(root.string()));
  refreshWorkspaceList();
  updateTitle();
  statusBar()->showMessage(
      QString("Workspace: %1  (%2 aree) — doppio clic su un'area per aprirla")
          .arg(QString::fromStdString(workspace_->root.string()))
          .arg(workspace_areas_.size()),
      10000);
}

void MainWindow::openAreaFromWorkspace(const QString& area_path) {
  openPath(area_path);
}

void MainWindow::onWorkspaceAreaActivated(QTableWidgetItem* item) {
  if(item == nullptr) {
    return;
  }
  const QString path = item->data(kWorkspacePathRole).toString();
  if(!path.isEmpty()) {
    openAreaFromWorkspace(path);
  }
}

void MainWindow::saveLastWorkspacePath(const QString& path) {
  QSettings settings(QStringLiteral("NebbieArcane"), QStringLiteral("Eleutheria"));
  settings.setValue(QString::fromUtf8(kLastWorkspacePathKey), path);
}

void MainWindow::restoreLastWorkspace() {
  QSettings settings(QStringLiteral("NebbieArcane"), QStringLiteral("Eleutheria"));
  const QString path = settings.value(QString::fromUtf8(kLastWorkspacePathKey)).toString();
  if(path.isEmpty() || !QFileInfo::exists(path)) {
    return;
  }
  try {
    loadWorkspace(path);
  } catch(const std::exception&) {
    // Path vecchio o non valido: ignora in silenzio
  }
}

void MainWindow::markDirty() {
  if(area_) {
    area_->dirty = true;
    updateTitle();
  }
}

bool MainWindow::maybeSave() {
  if(!area_ || !area_->dirty) {
    return true;
  }
  const auto ans = QMessageBox::question(
      this, "Salvare?", "L'area ha modifiche non salvate. Salvare?",
      QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
  if(ans == QMessageBox::Cancel) {
    return false;
  }
  if(ans == QMessageBox::Save) {
    saveArea();
    return !area_->dirty;
  }
  return true;
}

void MainWindow::closeEvent(QCloseEvent* event) {
  if(maybeSave()) {
    saveWorkspaceTableState();
    event->accept();
  } else {
    event->ignore();
  }
}

void MainWindow::newArea() {
  if(!maybeSave()) {
    return;
  }
  eleutheria::Area area;
  area.meta.name = "Nuova area";
  area.meta.author = "";
  area.meta.mud_id = "nebbie";
  area.dirty = false;
  current_base_.clear();
  setArea(std::move(area));
}

void MainWindow::openPath(const QString& path) {
  if(path.isEmpty() || !maybeSave()) {
    return;
  }
  try {
    const std::filesystem::path fs_path(path.toStdString());
    // Se e' un workspace (cartella parent con molte aree), apri la lista aree.
    if(std::filesystem::is_directory(fs_path) && eleutheria::is_workspace_root(fs_path) &&
       !eleutheria::is_area_directory(fs_path)) {
      loadWorkspace(path);
      return;
    }
    // Singola area: se possibile aggancia anche il workspace parent.
    if(std::filesystem::is_directory(fs_path) && eleutheria::is_area_directory(fs_path)) {
      const auto ws_root = eleutheria::guess_workspace_root(fs_path);
      if(eleutheria::is_workspace_root(ws_root)) {
        workspace_ = eleutheria::make_workspace(ws_root);
        workspace_areas_ = eleutheria::scan_workspace_areas(*workspace_);
        refreshWorkspaceList();
      }
    }

    const auto loaded = eleutheria::load_area(path.toStdString());
    current_base_ = QString::fromStdString(loaded.area.base_path);
    auto area = loaded.area;
    area.dirty = false;
    setArea(std::move(area));
    statusBar()->showMessage(
        QString("Aperto: %1  (%2 stanze, %3 mob, %4 obj, %5 zone)")
            .arg(current_base_)
            .arg(area_->rooms.size())
            .arg(area_->mobiles.size())
            .arg(area_->objects.size())
            .arg(area_->zones.size()),
        8000);
    if(!loaded.warnings.empty()) {
      QString msg;
      for(const auto& w : loaded.warnings) {
        msg += QString::fromStdString(w) + "\n";
      }
      QMessageBox::warning(this, "Avvisi caricamento", msg);
    }
  } catch(const std::exception& e) {
    QMessageBox::critical(this, "Errore apertura", QString::fromUtf8(e.what()));
  }
}

void MainWindow::openWorkspace() {
  const QString start = workspace_
                            ? QString::fromStdString(workspace_->root.string())
                            : (current_base_.isEmpty() ? QString()
                                                       : QFileInfo(current_base_).absolutePath());
  const QString path = eleutheria_ui::chooseExistingDirectory(
      this, QStringLiteral("Apri workspace Aree (cartella con tutte le aree)"), start);
  if(path.isEmpty()) {
    return;
  }
  try {
    if(!maybeSave()) {
      return;
    }
    loadWorkspace(path);
  } catch(const std::exception& e) {
    QMessageBox::critical(this, "Errore workspace", QString::fromUtf8(e.what()));
  }
}

void MainWindow::openArea() {
  const QString start =
      current_base_.isEmpty() ? QString() : QFileInfo(current_base_).absolutePath();
  const QString path = eleutheria_ui::chooseExistingDirectory(
      this, QStringLiteral("Apri cartella area / mudroot / lib"), start);
  if(path.isEmpty()) {
    return;
  }
  openPath(path);
}

void MainWindow::openAreaFile() {
  const QString start =
      current_base_.isEmpty() ? QString() : QFileInfo(current_base_).absolutePath();
  const QString path = eleutheria_ui::chooseOpenFile(
      this, QStringLiteral("Apri file area"),
      QStringLiteral("Area Nebbie/Eleutheria (*.ele *.wld *.mob *.obj *.zon *.shp);;Tutti (*)"),
      start);
  if(path.isEmpty()) {
    return;
  }
  openPath(path);
}

void MainWindow::saveArea() {
  if(!area_) {
    return;
  }
  applyCurrentEditor();
  refreshLists();
  if(current_base_.isEmpty()) {
    saveAreaAs();
    return;
  }
  try {
    eleutheria::save_area(*area_, current_base_.toStdString());
    area_->dirty = false;
    updateTitle();
    statusBar()->showMessage("Area salvata", 3000);
  } catch(const std::exception& e) {
    QMessageBox::critical(this, "Errore salvataggio", e.what());
  }
}

void MainWindow::saveAreaAs() {
  if(!area_) {
    return;
  }
  const QString suggested =
      area_ ? QString::fromStdString(area_->meta.name) : QStringLiteral("area");
  const QString start =
      current_base_.isEmpty() ? QString() : QFileInfo(current_base_).absolutePath();
  const QString path = eleutheria_ui::chooseSaveFile(
      this, QStringLiteral("Salva area come"), QStringLiteral("Area (*.ele)"), start,
      suggested + QStringLiteral(".ele"));
  if(path.isEmpty()) {
    return;
  }
  QString base = path;
  if(base.endsWith(".ele", Qt::CaseInsensitive)) {
    base.chop(4);
  }
  current_base_ = base;
  saveArea();
}

void MainWindow::validateArea() {
  if(!area_) {
    return;
  }
  applyCurrentEditor();
  const auto issues = eleutheria::validate_area(*area_);
  if(issues.empty()) {
    QMessageBox::information(this, "Validazione", "Nessun problema rilevato.");
    return;
  }
  QString msg;
  for(const auto& issue : issues) {
    msg += QString("[%1 #%2] %3\n")
               .arg(QString::fromStdString(issue.kind))
               .arg(issue.vnum)
               .arg(QString::fromStdString(issue.message));
  }
  QMessageBox::warning(this, "Validazione", msg);
}

void MainWindow::about() {
  QMessageBox::about(
      this, "Eleutheria",
      "Eleutheria — editor di aree per Nebbie Arcane\n"
      "Rifacimento cross-platform ispirato a Eleutheria di Lavinio Cerquetti.\n"
      "Formato file allineato al server Nebbie Arcane (db.cpp).\n"
      "Compilabile su Windows, macOS e Linux (Qt 6 / C++17).");
}

void MainWindow::applyCurrentEditor() {
  if(!area_) {
    return;
  }
  // Non chiamare refreshLists() qui: distruggerebbe gli item della lista
  // mentre e' ancora in corso currentRowChanged → crash.
  if(current_room_) {
    auto room = room_editor_->save();
    if(room.vnum != *current_room_) {
      area_->rooms.erase(*current_room_);
      area_->dirty_rooms.erase(*current_room_);
      area_->dirty_rooms.insert(room.vnum);
      current_room_ = room.vnum;
    } else if(area_->rooms.count(room.vnum)) {
      eleutheria::preserve_flag_tokens(room, area_->rooms.at(room.vnum));
    }
    area_->rooms[room.vnum] = room;
  }
  if(current_mob_) {
    auto mob = mob_editor_->save();
    if(mob.vnum != *current_mob_) {
      area_->mobiles.erase(*current_mob_);
      area_->dirty_mobs.erase(*current_mob_);
      area_->dirty_mobs.insert(mob.vnum);
      current_mob_ = mob.vnum;
    } else if(area_->mobiles.count(mob.vnum)) {
      eleutheria::preserve_flag_tokens(mob, area_->mobiles.at(mob.vnum));
    }
    area_->mobiles[mob.vnum] = mob;
  }
  if(current_obj_) {
    auto obj = obj_editor_->save();
    if(obj.vnum != *current_obj_) {
      area_->objects.erase(*current_obj_);
      area_->dirty_objects.erase(*current_obj_);
      area_->dirty_objects.insert(obj.vnum);
      current_obj_ = obj.vnum;
    } else if(area_->objects.count(obj.vnum)) {
      eleutheria::preserve_flag_tokens(obj, area_->objects.at(obj.vnum));
    }
    area_->objects[obj.vnum] = obj;
  }
  if(current_zone_) {
    auto zone = zone_editor_->save();
    if(zone.number != *current_zone_) {
      area_->zones.erase(*current_zone_);
      area_->dirty_zones.erase(*current_zone_);
      area_->dirty_zones.insert(zone.number);
      current_zone_ = zone.number;
    }
    area_->zones[zone.number] = zone;
  }
}

void MainWindow::onRoomSelected() {
  if(!area_ || room_list_->currentItem() == nullptr) {
    return;
  }
  const auto vnum = room_list_->currentItem()->data(Qt::UserRole).toLongLong();
  applyCurrentEditor();
  const auto it = area_->rooms.find(vnum);
  if(it == area_->rooms.end()) {
    return;
  }
  current_room_ = vnum;
  room_editor_->setArea(&*area_);
  room_editor_->load(it->second);
}

void MainWindow::onMobSelected() {
  if(!area_ || mob_list_->currentItem() == nullptr) {
    return;
  }
  const auto vnum = mob_list_->currentItem()->data(Qt::UserRole).toLongLong();
  applyCurrentEditor();
  const auto it = area_->mobiles.find(vnum);
  if(it == area_->mobiles.end()) {
    return;
  }
  current_mob_ = vnum;
  mob_editor_->load(it->second);
}

void MainWindow::onObjSelected() {
  if(!area_ || obj_list_->currentItem() == nullptr) {
    return;
  }
  const auto vnum = obj_list_->currentItem()->data(Qt::UserRole).toLongLong();
  applyCurrentEditor();
  const auto it = area_->objects.find(vnum);
  if(it == area_->objects.end()) {
    return;
  }
  current_obj_ = vnum;
  obj_editor_->load(it->second);
}

void MainWindow::onZoneSelected() {
  if(!area_ || zone_list_->currentItem() == nullptr) {
    return;
  }
  const int num = zone_list_->currentItem()->data(Qt::UserRole).toInt();
  applyCurrentEditor();
  const auto it = area_->zones.find(num);
  if(it == area_->zones.end()) {
    return;
  }
  current_zone_ = num;
  zone_editor_->load(it->second);
}

void MainWindow::addRoom() {
  if(!area_) {
    return;
  }
  applyCurrentEditor();
  std::int64_t vnum = area_->rooms.empty() ? 1 : (area_->rooms.rbegin()->first + 1);
  eleutheria::Room room;
  room.vnum = vnum;
  room.name = "Nuova stanza";
  room.description = "Descrizione.\n";
  area_->rooms[vnum] = room;
  area_->dirty_rooms.insert(vnum);
  current_room_ = vnum;
  markDirty();
  refreshLists();
  room_editor_->setArea(&*area_);
  room_editor_->load(area_->rooms.at(vnum));
  tabs_->setCurrentIndex(0);
}

void MainWindow::addMob() {
  if(!area_) {
    return;
  }
  applyCurrentEditor();
  std::int64_t vnum = area_->mobiles.empty() ? 1 : (area_->mobiles.rbegin()->first + 1);
  eleutheria::Mobile mob;
  mob.vnum = vnum;
  mob.name = "nuovo mob";
  mob.short_descr = "un nuovo mob";
  mob.long_descr = "Un nuovo mob e' qui.\n";
  mob.letter = 'A';
  mob.gold_exp_race_form = true;
  area_->mobiles[vnum] = mob;
  area_->dirty_mobs.insert(vnum);
  current_mob_ = vnum;
  markDirty();
  refreshLists();
  mob_editor_->load(area_->mobiles.at(vnum));
  tabs_->setCurrentIndex(1);
}

void MainWindow::addObj() {
  if(!area_) {
    return;
  }
  applyCurrentEditor();
  std::int64_t vnum = area_->objects.empty() ? 1 : (area_->objects.rbegin()->first + 1);
  eleutheria::GameObject obj;
  obj.vnum = vnum;
  obj.name = "nuovo oggetto";
  obj.short_description = "un nuovo oggetto";
  obj.description = "Un nuovo oggetto e' qui.";
  obj.type_flag = 15;
  area_->objects[vnum] = obj;
  area_->dirty_objects.insert(vnum);
  current_obj_ = vnum;
  markDirty();
  refreshLists();
  obj_editor_->load(area_->objects.at(vnum));
  tabs_->setCurrentIndex(2);
}

void MainWindow::deleteCurrent() {
  if(!area_) {
    return;
  }
  const int tab = tabs_->currentIndex();
  if(tab == 0 && current_room_) {
    area_->dirty_rooms.erase(*current_room_);
    area_->rooms.erase(*current_room_);
    current_room_.reset();
    room_editor_->clear();
  } else if(tab == 1 && current_mob_) {
    area_->dirty_mobs.erase(*current_mob_);
    area_->mobiles.erase(*current_mob_);
    current_mob_.reset();
    mob_editor_->clear();
  } else if(tab == 2 && current_obj_) {
    area_->dirty_objects.erase(*current_obj_);
    area_->objects.erase(*current_obj_);
    current_obj_.reset();
    obj_editor_->clear();
  } else if(tab == 3 && current_zone_) {
    area_->dirty_zones.erase(*current_zone_);
    area_->zones.erase(*current_zone_);
    current_zone_.reset();
    zone_editor_->clear();
  }
  markDirty();
  refreshLists();
}
