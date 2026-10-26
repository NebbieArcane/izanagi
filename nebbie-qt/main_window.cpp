#include "main_window.hpp"

#include "app_config.hpp"
#include "app_i18n.hpp"
#include "application_log.hpp"
#include "coordinator_client.hpp"
#include "edit_menu.hpp"
#include "help_menu_guide.hpp"
#include "mob_editor_widget.hpp"
#include "mud_color_dialogs.hpp"
#include "mud_color_common.hpp"
#include "mud_color_list_delegate.hpp"
#include "mud_color_widgets.hpp"
#include "obj_editor_widget.hpp"
#include "path_util.hpp"
#include "room_editor_widget.hpp"
#include "special_proc_catalog_dialog.hpp"
#include "zone_editor_widget.hpp"
#include "world_data_editor_widget.hpp"
#include "world_zone_map_widget.hpp"
#include "zone_map_widget.hpp"
#include "validation_report_ui.hpp"
#include "nebbie/edit.hpp"
#include "nebbie/aree_workspace.hpp"
#include "nebbie/constants.hpp"
#include "nebbie/overlay_io.hpp"
#include "nebbie/io.hpp"
#include "nebbie/monolith_audit.hpp"
#include "nebbie/special_proc_catalog.hpp"
#include "nebbie/validate.hpp"
#include "nebbie/world_index.hpp"
#include "nebbie/zon_comment_sync.hpp"
#include "nebbie/zone_graph.hpp"

#include <QAbstractItemView>
#include <QAction>
#include <QSignalBlocker>
#include <QActionGroup>
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QCheckBox>
#include <QCoreApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QDockWidget>
#include <QFileInfo>
#include <QHeaderView>
#include <QMetaType>
#include <QVariant>
#include <QFile>
#include <QNetworkAccessManager>
#include <QTabWidget>
#include <QComboBox>
#include <QFileDialog>
#include <QFont>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QDateTime>
#include <QIcon>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QStackedWidget>
#include <QMenuBar>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QSplitter>
#include <QStatusBar>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>
#include <QTableWidget>
#include <QTableWidgetItem>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <map>
#include <stdexcept>
#include <vector>

using nebbie::qt::appTr;

namespace {

constexpr int kNavKindRole = Qt::UserRole + 10;
constexpr int kStackRooms = 0;
constexpr int kStackMobs = 1;
constexpr int kStackObjects = 2;
constexpr int kStackZones = 3;
constexpr int kStackShops = 4;
constexpr int kStackSpecials = 5;
constexpr int kStackGlobal = 6;
constexpr int kStackMap = 7;
constexpr int kStackValidation = 8;

enum class NavKind : int {
    Root = 0,
    CatRooms,
    CatMobs,
    CatObjects,
    CatZones,
    CatWorld,
    Room,
    RoomExit,
    Mob,
    Object,
    Zone,
    PageShops,
    PageSpecials,
    PageGlobal,
    PageMap,
    PageValidation,
};

void setNavKind(QTreeWidgetItem* item, NavKind kind) {
    if (item) {
        item->setData(0, kNavKindRole, static_cast<int>(kind));
    }
}

NavKind navKindOf(const QTreeWidgetItem* item) {
    if (!item) {
        return NavKind::Root;
    }
    return static_cast<NavKind>(item->data(0, kNavKindRole).toInt());
}

QTreeWidgetItem* makeCategoryItem(QTreeWidgetItem* parent, const QString& label, NavKind kind) {
    auto* item = new QTreeWidgetItem(parent);
    item->setText(0, label);
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    setNavKind(item, kind);
    item->setExpanded(kind == NavKind::CatRooms || kind == NavKind::CatWorld);
    return item;
}

QTreeWidgetItem* makeLeafPageItem(QTreeWidgetItem* parent, const QString& label, NavKind kind) {
    auto* item = new QTreeWidgetItem(parent);
    item->setText(0, label);
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    setNavKind(item, kind);
    return item;
}

bool room_editor_snapshot_matches(const nebbie::Room& before, const nebbie::Room& after) {
    if (before.name != after.name || before.description != after.description
        || before.room_flags != after.room_flags || before.sector_type != after.sector_type
        || before.tele_time != after.tele_time || before.tele_targ != after.tele_targ
        || before.tele_mask != after.tele_mask || before.tele_cnt != after.tele_cnt
        || before.river_speed != after.river_speed || before.river_dir != after.river_dir
        || before.moblim != after.moblim || before.bright_at_night != after.bright_at_night
        || before.bright_at_day != after.bright_at_day || before.exits.size() != after.exits.size()
        || before.extra_descs.size() != after.extra_descs.size()) {
        return false;
    }

    for (std::size_t i = 0; i < before.exits.size(); ++i) {
        const nebbie::Exit& lhs = before.exits[i];
        const nebbie::Exit& rhs = after.exits[i];
        if (lhs.direction != rhs.direction || lhs.description != rhs.description
            || lhs.keyword != rhs.keyword || lhs.exit_info != rhs.exit_info || lhs.key != rhs.key
            || lhs.to_room != rhs.to_room || lhs.open_cmd != rhs.open_cmd) {
            return false;
        }
    }

    for (std::size_t i = 0; i < before.extra_descs.size(); ++i) {
        if (before.extra_descs[i].keyword != after.extra_descs[i].keyword
            || before.extra_descs[i].description != after.extra_descs[i].description) {
            return false;
        }
    }

    return true;
}

nebbie::qt::MudColorListDelegate* roomNavDelegate(QTreeWidget* tree) {
    return qobject_cast<nebbie::qt::MudColorListDelegate*>(tree->itemDelegate());
}

constexpr int kAreeColName = 0;
constexpr int kAreeColZone = 1;
constexpr int kAreeColRooms = 2;
constexpr int kAreeColFolder = 3;

std::optional<nebbie::AreeAreaInfo> selectedAreeArea(const QTableWidget* table,
                                                     const std::vector<nebbie::AreeAreaInfo>& areas) {
    const int row = table->currentRow();
    if (row < 0 || !table) {
        return std::nullopt;
    }
    const QTableWidgetItem* folder_item = table->item(row, kAreeColFolder);
    if (!folder_item) {
        return std::nullopt;
    }
    const std::string folder = folder_item->data(Qt::UserRole).toString().toStdString();
    const std::string folder_text = folder.empty() ? folder_item->text().toStdString() : folder;
    for (const nebbie::AreeAreaInfo& area : areas) {
        if (area.folder_name == folder_text) {
            return area;
        }
    }
    return std::nullopt;
}

class SortRoleTableItem : public QTableWidgetItem {
public:
    using QTableWidgetItem::QTableWidgetItem;

    bool operator<(const QTableWidgetItem& other) const override {
        const QVariant left = data(Qt::UserRole + 1);
        const QVariant right = other.data(Qt::UserRole + 1);
        if (left.isValid() && right.isValid()) {
            if (left.userType() == QMetaType::LongLong || left.userType() == QMetaType::Int) {
                return left.toLongLong() < right.toLongLong();
            }
            return left.toString().localeAwareCompare(right.toString()) < 0;
        }
        return QTableWidgetItem::operator<(other);
    }
};

SortRoleTableItem* makeSortableTextItem(const QString& text) {
    auto* item = new SortRoleTableItem(text);
    item->setData(Qt::UserRole + 1, text.toLower());
    return item;
}

SortRoleTableItem* makeSortableNumericItem(const qlonglong value, const QString& display) {
    auto* item = new SortRoleTableItem(display);
    item->setData(Qt::UserRole + 1, value);
    item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    return item;
}

} // namespace

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    app_config_ = nebbie::qt::read_config();
    nebbie::qt::setAppLanguage(nebbie::qt::parseLanguageCode(app_config_.ui_language));
    network_ = new QNetworkAccessManager(this);
    update_checker_ = new nebbie::qt::ReleaseUpdateChecker(nebbie::qt::ReleaseProduct::Izanagi, this);
    connect(update_checker_, &nebbie::qt::ReleaseUpdateChecker::checkFinished, this,
            &MainWindow::onUpdateCheckFinished);
    applySpecialProcCatalogFromConfig();
    setupUi();
    setupAreeDock();
    setupMenus();
    retranslateUi();
    applyTextEditorSettings();
    if (auto* delegate = roomNavDelegate(nav_tree_)) {
        delegate->setShowColorCodes(app_config_.show_color_codes);
    }
    setWindowIcon(QIcon(QStringLiteral(":/app-icon.png")));
    resize(1200, 760);
    scheduleStartupUpdateCheck();
}

void MainWindow::setupUi() {
    auto* central = new QWidget;
    auto* root_layout = new QVBoxLayout(central);

    lib_label_ = new QLabel;
    lib_label_->setWordWrap(true);
    root_layout->addWidget(lib_label_);

    auto* main_splitter = new QSplitter;
    main_splitter->setObjectName(QStringLiteral("mainNavSplitter"));

    auto* nav_panel = new QWidget;
    auto* nav_layout = new QVBoxLayout(nav_panel);
    nav_layout->setContentsMargins(0, 0, 0, 0);

    auto* nav_toolbar = new QWidget;
    nav_toolbar->setObjectName(QStringLiteral("navToolbar"));
    auto* nav_top = new QHBoxLayout(nav_toolbar);
    nav_top->setContentsMargins(4, 4, 4, 4);
    nav_top->setSpacing(4);
    entity_search_ = new QLineEdit;
    entity_search_->setPlaceholderText(appTr("ui.search_vnum"));
    entity_create_button_ = new QPushButton;
    nav_top->addWidget(entity_search_, 1);
    nav_top->addWidget(entity_create_button_);
    nav_layout->addWidget(nav_toolbar);

    nav_tree_ = new QTreeWidget;
    nav_tree_->setObjectName(QStringLiteral("navTree"));
    nav_tree_->setHeaderHidden(true);
    nav_tree_->setUniformRowHeights(true);
    nav_tree_->setAnimated(false);
    nav_tree_->setIndentation(16);
    nav_tree_->setRootIsDecorated(true);
    nav_tree_->setItemsExpandable(true);
    nav_tree_->setExpandsOnDoubleClick(true);
    nav_tree_->setMinimumWidth(260);
    nav_tree_->setItemDelegate(new nebbie::qt::MudColorListDelegate(nav_tree_));
    nav_layout->addWidget(nav_tree_, 1);

    // Flat top-level categories (no useless "Area" folder for a single area).
    nav_root_ = nullptr;
    nav_rooms_ = makeCategoryItem(nav_tree_->invisibleRootItem(), QString(), NavKind::CatRooms);
    nav_mobs_ = makeCategoryItem(nav_tree_->invisibleRootItem(), QString(), NavKind::CatMobs);
    nav_objects_ = makeCategoryItem(nav_tree_->invisibleRootItem(), QString(), NavKind::CatObjects);
    nav_zones_ = makeCategoryItem(nav_tree_->invisibleRootItem(), QString(), NavKind::CatZones);
    nav_world_ = makeCategoryItem(nav_tree_->invisibleRootItem(), QStringLiteral("World"), NavKind::CatWorld);
    nav_shops_ = makeLeafPageItem(nav_world_, QString(), NavKind::PageShops);
    nav_specials_ = makeLeafPageItem(nav_world_, QString(), NavKind::PageSpecials);
    nav_global_ = makeLeafPageItem(nav_world_, QString(), NavKind::PageGlobal);
    nav_map_ = makeLeafPageItem(nav_tree_->invisibleRootItem(), QString(), NavKind::PageMap);
    nav_validation_ = makeLeafPageItem(nav_tree_->invisibleRootItem(), QString(), NavKind::PageValidation);
    nav_rooms_->setExpanded(true);

    editor_stack_ = new QStackedWidget;

    room_editor_ = new RoomEditorWidget;
    auto* room_panel = new QWidget;
    auto* room_panel_layout = new QVBoxLayout(room_panel);
    room_panel_layout->setContentsMargins(0, 0, 0, 0);
    room_panel_layout->addWidget(room_editor_, 1);
    auto* room_buttons = new QHBoxLayout;
    auto* room_apply = new QPushButton;
    auto* room_sync_exits = new QPushButton;
    auto* room_align_all_exits = new QPushButton;
    auto* room_goto_exit = new QPushButton;
    room_apply_button_ = room_apply;
    room_sync_exits_button_ = room_sync_exits;
    room_align_all_exits_button_ = room_align_all_exits;
    room_goto_exit_button_ = room_goto_exit;
    room_buttons->addWidget(room_apply);
    room_buttons->addWidget(room_sync_exits);
    room_buttons->addWidget(room_align_all_exits);
    room_buttons->addWidget(room_goto_exit);
    room_buttons->addStretch();
    room_panel_layout->addLayout(room_buttons);
    editor_stack_->addWidget(room_panel);

    auto* mob_scroll = new QScrollArea;
    mob_scroll->setWidgetResizable(true);
    mob_editor_ = new MobEditorWidget;
    mob_scroll->setWidget(mob_editor_);
    auto* mob_panel = new QWidget;
    auto* mob_panel_layout = new QVBoxLayout(mob_panel);
    mob_panel_layout->setContentsMargins(0, 0, 0, 0);
    mob_panel_layout->addWidget(mob_scroll, 1);
    auto* mob_apply = new QPushButton;
    mob_apply_button_ = mob_apply;
    mob_panel_layout->addWidget(mob_apply);
    editor_stack_->addWidget(mob_panel);

    auto* obj_scroll = new QScrollArea;
    obj_scroll->setWidgetResizable(true);
    obj_editor_ = new ObjEditorWidget;
    obj_scroll->setWidget(obj_editor_);
    auto* obj_panel = new QWidget;
    auto* obj_panel_layout = new QVBoxLayout(obj_panel);
    obj_panel_layout->setContentsMargins(0, 0, 0, 0);
    obj_panel_layout->addWidget(obj_scroll, 1);
    auto* obj_apply = new QPushButton;
    obj_apply_button_ = obj_apply;
    obj_panel_layout->addWidget(obj_apply);
    editor_stack_->addWidget(obj_panel);

    auto* zone_scroll = new QScrollArea;
    zone_scroll->setWidgetResizable(true);
    zone_editor_ = new ZoneEditorWidget;
    zone_scroll->setWidget(zone_editor_);
    auto* zone_panel = new QWidget;
    auto* zone_panel_layout = new QVBoxLayout(zone_panel);
    zone_panel_layout->setContentsMargins(0, 0, 0, 0);
    zone_panel_layout->addWidget(zone_scroll, 1);
    auto* zone_buttons = new QHBoxLayout;
    auto* zone_apply = new QPushButton;
    zone_apply_button_ = zone_apply;
    zone_buttons->addWidget(zone_apply);
    zone_buttons->addStretch();
    zone_panel_layout->addLayout(zone_buttons);
    zone_tab_ = zone_panel;
    editor_stack_->addWidget(zone_panel);

    shops_editor_ = new WorldDataEditorWidget(WorldDataScope::Shops);
    shops_editor_->setLibContext(&context_);
    shops_tab_ = shops_editor_;
    editor_stack_->addWidget(shops_tab_);
    specials_editor_ = new WorldDataEditorWidget(WorldDataScope::Specials);
    specials_editor_->setLibContext(&context_);
    specials_tab_ = specials_editor_;
    editor_stack_->addWidget(specials_tab_);
    global_tables_editor_ = new WorldDataEditorWidget(WorldDataScope::GlobalTables);
    global_tables_editor_->setLibContext(&context_);
    global_tables_tab_ = global_tables_editor_;
    editor_stack_->addWidget(global_tables_tab_);
    connect(shops_editor_, &WorldDataEditorWidget::modified, this, [this]() {
        if (!world_.shops.empty()) {
            context_.has_shp = true;
            if (context_.shp_path.empty()) {
                context_.shp_path = nebbie::default_lib_filename_for_extension(
                    context_, ".shp", nebbie::SHOP_FILE);
            }
        }
        shops_editor_->setLibContext(&context_);
        markDirty();
    });
    connect(specials_editor_, &WorldDataEditorWidget::modified, this, [this]() {
        if (!world_.special_procs.empty()) {
            context_.has_spe = true;
            if (context_.spe_path.empty()) {
                context_.spe_path = nebbie::default_lib_filename_for_extension(
                    context_, ".spe", nebbie::SPECIAL_FILE);
            }
        }
        specials_editor_->setLibContext(&context_);
        markDirty();
    });
    connect(global_tables_editor_, &WorldDataEditorWidget::modified, this, [this]() {
        markDirty();
    });

    map_tab_ = new QWidget;
    auto* map_layout = new QVBoxLayout(map_tab_);
    auto* map_tabs = new QTabWidget;
    map_tabs_ = map_tabs;

    auto* map_zone_page = new QWidget;
    auto* map_zone_layout = new QVBoxLayout(map_zone_page);
    map_zone_layout->addWidget(
        new QLabel("Stanze della zona. Piano Z separato; clic su su/giù cambia piano ed evidenzia la destinazione."));
    auto* map_top = new QHBoxLayout;
    map_zone_ = new QComboBox;
    map_floor_ = new QComboBox;
    map_broken_only_ = new QCheckBox("Solo link rotti");
    auto* map_refresh = new QPushButton("Aggiorna");
    auto* map_export_dot = new QPushButton("Esporta DOT");
    auto* map_export_png = new QPushButton("Esporta PNG");
    map_top->addWidget(new QLabel("Zona:"));
    map_top->addWidget(map_zone_, 1);
    map_top->addWidget(new QLabel("Piano Z:"));
    map_top->addWidget(map_floor_);
    map_top->addWidget(map_broken_only_);
    map_top->addWidget(map_refresh);
    map_top->addWidget(map_export_dot);
    map_top->addWidget(map_export_png);
    map_zone_layout->addLayout(map_top);

    map_view_ = new ZoneMapWidget;
    map_view_->setMinimumHeight(320);
    map_zone_layout->addWidget(map_view_, 1);

    map_stats_ = new QLabel;
    map_stats_->setWordWrap(true);
    map_zone_layout->addWidget(map_stats_);
    map_tabs->addTab(map_zone_page, QString());

    auto* map_world_page = new QWidget;
    auto* map_world_layout = new QVBoxLayout(map_world_page);
    map_world_layout->addWidget(
        new QLabel("Collegamenti tra zone (non stanze). U=vnum usati, L=vnum liberi nel range. Clic seleziona, doppio clic apre Zone."));
    auto* world_top = new QHBoxLayout;
    world_map_broken_only_ = new QCheckBox("Solo link rotti");
    auto* world_refresh = new QPushButton("Aggiorna");
    auto* world_export_dot = new QPushButton("Esporta DOT");
    auto* world_export_png = new QPushButton("Esporta PNG");
    auto* world_open_zone_map = new QPushButton("Mappa stanze zona");
    world_top->addWidget(world_map_broken_only_);
    world_top->addWidget(world_open_zone_map);
    world_top->addStretch(1);
    world_top->addWidget(world_refresh);
    world_top->addWidget(world_export_dot);
    world_top->addWidget(world_export_png);
    map_world_layout->addLayout(world_top);

    world_map_view_ = new WorldZoneMapWidget;
    world_map_view_->setMinimumHeight(320);
    map_world_layout->addWidget(world_map_view_, 2);

    world_map_details_ = new QPlainTextEdit;
    world_map_details_->setReadOnly(true);
    world_map_details_->setPlaceholderText("Clic su una zona per vedere vnum usati e liberi.");
    world_map_details_->setMaximumHeight(140);
    map_world_layout->addWidget(world_map_details_, 1);

    world_map_stats_ = new QLabel;
    world_map_stats_->setWordWrap(true);
    map_world_layout->addWidget(world_map_stats_);
    map_tabs->addTab(map_world_page, QString());

    map_layout->addWidget(map_tabs, 1);
    editor_stack_->addWidget(map_tab_);

    validation_tab_ = new QWidget;
    auto* validation_layout = new QVBoxLayout(validation_tab_);

    auto* validation_top = new QHBoxLayout;
    auto* validate_button = new QPushButton("Valida ora");
    auto* validation_summary = new QLabel("Esegui Valida per vedere errori e avvisi. Doppio clic per andare all'entità.");
    validation_summary->setWordWrap(true);
    validation_top->addWidget(validate_button);
    validation_layout->addWidget(validation_summary);
    validation_layout->addLayout(validation_top);

    validation_list_ = new QListWidget;
    validation_layout->addWidget(validation_list_, 1);
    editor_stack_->addWidget(validation_tab_);

    main_splitter->addWidget(nav_panel);
    main_splitter->addWidget(editor_stack_);
    main_splitter->setStretchFactor(0, 1);
    main_splitter->setStretchFactor(1, 4);
    main_splitter->setSizes({280, 820});
    root_layout->addWidget(main_splitter, 1);
    setCentralWidget(central);

    connect(nav_tree_, &QTreeWidget::currentItemChanged, this,
            [this](QTreeWidgetItem* /*current*/, QTreeWidgetItem* previous) {
                if (previous && navKindOf(previous) == NavKind::Room) {
                    persistRoomEditorState(previous);
                }
                onNavTreeSelectionChanged();
            });
    connect(room_apply, &QPushButton::clicked, this, &MainWindow::applyRoomChanges);
    connect(room_sync_exits, &QPushButton::clicked, this, &MainWindow::syncInboundExitLabels);
    connect(room_align_all_exits, &QPushButton::clicked, this, &MainWindow::alignAllInboundExitDescriptions);
    connect(room_goto_exit, &QPushButton::clicked, this, &MainWindow::goToExitTarget);
    connect(room_editor_, &RoomEditorWidget::alignExitLabelRequested, this, &MainWindow::alignCurrentExitLabel);
    connect(mob_apply, &QPushButton::clicked, this, &MainWindow::applyMobChanges);
    connect(obj_apply, &QPushButton::clicked, this, &MainWindow::applyObjChanges);
    connect(entity_create_button_, &QPushButton::clicked, this, &MainWindow::onEntityCreateClicked);
    connect(entity_search_, &QLineEdit::textChanged, this, &MainWindow::onEntitySearchChanged);
    connect(zone_apply, &QPushButton::clicked, this, &MainWindow::applyZoneChanges);
    connect(zone_editor_, &ZoneEditorWidget::zoneModified, this, [this]() {
        markDirty();
        const int zone_num = currentZoneNum();
        if (zone_num > 0) {
            dirty_zone_nums_.insert(zone_num);
        }
    });
    connect(zone_editor_, &ZoneEditorWidget::gotoRoomRequested, this, [this](long vnum) {
        if (!world_.find_room(vnum)) {
            QMessageBox::information(this, "Zone", QString("Room #%1 does not exist in this library.").arg(vnum));
            return;
        }
        selectRoomByVnum(vnum);
    });
    connect(zone_editor_, &ZoneEditorWidget::gotoMobRequested, this, [this](long vnum) {
        selectMobByVnum(vnum);
    });
    connect(zone_editor_, &ZoneEditorWidget::gotoObjectRequested, this, [this](long vnum) {
        selectObjectByVnum(vnum);
    });
    connect(validate_button, &QPushButton::clicked, this, &MainWindow::validateLib);
    connect(validation_list_, &QListWidget::itemDoubleClicked, this, &MainWindow::onValidationIssueActivated);
    connect(map_refresh, &QPushButton::clicked, this, &MainWindow::refreshZoneMap);
    connect(map_zone_, &QComboBox::currentIndexChanged, this, [this](int) { refreshZoneMap(); });
    connect(map_broken_only_, &QCheckBox::toggled, this, [this](bool enabled) {
        if (map_view_) {
            map_view_->setShowBrokenOnly(enabled);
            updateMapStats();
        }
    });
    connect(map_floor_, &QComboBox::currentIndexChanged, this, [this](int index) {
        if (index < 0 || !map_view_) {
            return;
        }
        map_view_->setActiveZLevel(map_floor_->currentData().toInt());
        updateMapStats();
    });
    connect(map_view_, &ZoneMapWidget::roomActivated, this, [this](long vnum) {
        selectRoomByVnum(vnum);
        setStatus(QString("Mappa: selezionata stanza #%1.").arg(vnum));
    });
    connect(map_view_, &ZoneMapWidget::floorLinkActivated, this, [this](long target_vnum, int target_z) {
        const int index = map_floor_->findData(target_z);
        if (index >= 0) {
            map_floor_->setCurrentIndex(index);
        } else {
            map_view_->setActiveZLevel(target_z);
        }
        map_view_->setHighlightedVnum(target_vnum);
        setStatus(QString("Mappa: piano Z=%1, evidenciata stanza #%2.").arg(target_z).arg(target_vnum));
    });
    connect(map_export_dot, &QPushButton::clicked, this, [this]() {
        if (map_zone_->count() == 0) {
            return;
        }
        const int zone_num = map_zone_->currentData().toInt();
        const auto known = workspaceKnownRoomVnums();
        const nebbie::ZoneGraph graph =
            nebbie::build_zone_graph(world_, zone_num, known.empty() ? nullptr : &known);
        QApplication::clipboard()->setText(QString::fromStdString(nebbie::zone_graph_to_dot(graph)));
        setStatus("DOT zona copiato negli appunti.");
    });
    connect(map_export_png, &QPushButton::clicked, this, [this]() {
        const int zone_num = map_zone_->count() > 0 ? map_zone_->currentData().toInt() : 0;
        exportMapPng(map_view_, QString("zona-%1.png").arg(zone_num));
    });
    connect(world_refresh, &QPushButton::clicked, this, &MainWindow::refreshWorldZoneMap);
    connect(world_map_broken_only_, &QCheckBox::toggled, this, [this](bool enabled) {
        if (world_map_view_) {
            world_map_view_->setShowBrokenOnly(enabled);
        }
    });
    connect(world_map_view_, &WorldZoneMapWidget::zoneSelected, this, [this](int zone_num) {
        selected_world_zone_ = zone_num;
        updateWorldZoneDetails(zone_num);
    });
    connect(world_map_view_, &WorldZoneMapWidget::zoneActivated, this, [this](int zone_num) {
        selected_world_zone_ = zone_num;
        updateWorldZoneDetails(zone_num);
        openZoneRoomMap(zone_num);
    });
    connect(world_open_zone_map, &QPushButton::clicked, this, [this]() {
        if (selected_world_zone_ < 0) {
            QMessageBox::information(this, "Mappa", "Seleziona prima una zona nella mappa mondo.");
            return;
        }
        openZoneRoomMap(selected_world_zone_);
    });
    connect(world_export_dot, &QPushButton::clicked, this, [this]() {
        const auto known = workspaceKnownRoomVnums();
        const nebbie::WorldZoneGraph graph =
            nebbie::build_world_zone_graph(world_, known.empty() ? nullptr : &known);
        QApplication::clipboard()->setText(QString::fromStdString(nebbie::world_zone_graph_to_dot(graph)));
        setStatus("DOT mondo (zone) copiato negli appunti.");
    });
    connect(world_export_png, &QPushButton::clicked, this, [this]() {
        exportMapPng(world_map_view_, QStringLiteral("mondo-zone.png"));
    });
    zone_editor_->setWorld(&world_);
    room_editor_->setWorld(&world_);
    connect(room_editor_, &RoomEditorWidget::gotoRoomRequested, this, [this](long vnum) {
        selectRoomByVnum(vnum);
    });
    connect(room_editor_, &RoomEditorWidget::gotoMobRequested, this, [this](long vnum) {
        selectMobByVnum(vnum);
    });
    connect(room_editor_, &RoomEditorWidget::gotoObjectRequested, this, [this](long vnum) {
        selectObjectByVnum(vnum);
    });
    connect(room_editor_, &RoomEditorWidget::zoneResetsModified, this, [this](int zone_num) {
        markDirty();
        if (zone_num > 0) {
            dirty_zone_nums_.insert(zone_num);
        }
        if (zone_editor_ && currentZoneNum() == zone_num) {
            onZoneSelected();
        }
    });

    autosave_timer_ = new QTimer(this);
    autosave_timer_->setInterval(session_config_.autosave_interval_sec * 1000);
    connect(autosave_timer_, &QTimer::timeout, this, &MainWindow::onAutosaveTick);
}

void MainWindow::setupAreeDock() {
    aree_dock_ = new QDockWidget(this);
    aree_dock_->setObjectName(QStringLiteral("areeWorkspaceDock"));
    aree_dock_->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    auto* dock_widget = new QWidget;
    auto* layout = new QVBoxLayout(dock_widget);

    aree_table_ = new QTableWidget(0, 4);
    aree_table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    aree_table_->setSelectionMode(QAbstractItemView::SingleSelection);
    aree_table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    aree_table_->setSortingEnabled(true);
    aree_table_->verticalHeader()->setVisible(false);
    aree_table_->horizontalHeader()->setSectionsClickable(true);
    aree_table_->horizontalHeader()->setSortIndicatorShown(true);
    aree_table_->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    aree_table_->horizontalHeader()->setStretchLastSection(true);
    aree_table_->setColumnWidth(kAreeColName, 140);
    aree_table_->setColumnWidth(kAreeColZone, 64);
    aree_table_->setColumnWidth(kAreeColRooms, 100);
    aree_table_->setColumnWidth(kAreeColFolder, 110);
    layout->addWidget(aree_table_, 1);

    auto* buttons = new QHBoxLayout;
    aree_open_button_ = new QPushButton;
    aree_archive_button_ = new QPushButton;
    aree_restore_button_ = new QPushButton;
    buttons->addWidget(aree_open_button_);
    buttons->addWidget(aree_archive_button_);
    buttons->addWidget(aree_restore_button_);
    layout->addLayout(buttons);

    aree_dock_->setWidget(dock_widget);
    addDockWidget(Qt::LeftDockWidgetArea, aree_dock_);
    aree_dock_->hide();

    connect(aree_open_button_, &QPushButton::clicked, this, &MainWindow::openAreeArea);
    connect(aree_archive_button_, &QPushButton::clicked, this, &MainWindow::archiveAreeArea);
    connect(aree_restore_button_, &QPushButton::clicked, this, &MainWindow::restoreAreeArchive);
    connect(aree_table_, &QTableWidget::cellDoubleClicked, this, [this](int, int) { openAreeArea(); });
    connect(aree_table_, &QTableWidget::itemSelectionChanged, this, &MainWindow::onAreeAreaActivated);
    onAreeAreaActivated();
}

void MainWindow::setupMenus() {
    auto* file_menu = menuBar()->addMenu(appTr("menu.file"));

    auto* open_aree_action = file_menu->addAction(appTr("menu.open_aree_workspace"));
    open_aree_action->setShortcut(QKeySequence::Open);
    open_aree_action->setToolTip(appTr("menu.open_aree_workspace_tip"));
    connect(open_aree_action, &QAction::triggered, this, &MainWindow::openAreeWorkspace);

    auto* open_action = file_menu->addAction(appTr("menu.open_lib"));
    open_action->setToolTip(appTr("menu.open_lib_tip"));
    connect(open_action, &QAction::triggered, this, &MainWindow::openLib);

    auto* open_sample_action = file_menu->addAction(appTr("menu.open_sample_lib"));
    open_sample_action->setToolTip(appTr("menu.open_sample_lib_tip"));
    connect(open_sample_action, &QAction::triggered, this, &MainWindow::openBundledSampleLib);

    file_menu->addSeparator();
    auto* reload_action = file_menu->addAction(appTr("menu.reload_lib"));
    reload_action->setShortcut(QKeySequence::Refresh);
    reload_action->setToolTip(appTr("menu.reload_lib_tip"));
    connect(reload_action, &QAction::triggered, this, &MainWindow::reloadLib);

    auto* save_action = file_menu->addAction(appTr("menu.save"));
    save_action->setShortcut(QKeySequence::Save);
    connect(save_action, &QAction::triggered, this, &MainWindow::saveLib);

    auto* save_force_action = file_menu->addAction(appTr("menu.save_force"));
    connect(save_force_action, &QAction::triggered, this, &MainWindow::saveLibForce);

    auto* history_menu = file_menu->addMenu(appTr("menu.history"));
    auto* restore_workspace_action = history_menu->addAction(appTr("menu.restore_autosave"));
    connect(restore_workspace_action, &QAction::triggered, this, &MainWindow::restoreFromWorkspace);
    auto* restore_version_action = history_menu->addAction(appTr("menu.restore_version"));
    connect(restore_version_action, &QAction::triggered, this, &MainWindow::restoreVersion);
    history_menu->addSeparator();
    auto* archive_area_action = history_menu->addAction(appTr("aree.archive_area"));
    connect(archive_area_action, &QAction::triggered, this, &MainWindow::archiveAreeArea);
    auto* restore_aree_archive_action = history_menu->addAction(appTr("menu.aree_restore_archive"));
    connect(restore_aree_archive_action, &QAction::triggered, this, &MainWindow::restoreAreeArchive);

    file_menu->addSeparator();
    file_menu->addAction(appTr("menu.exit"), this, &QWidget::close);

    nebbie::qt::addStandardEditMenu(menuBar());

    auto* tools_menu = menuBar()->addMenu(appTr("menu.tools"));
    auto* validate_action = tools_menu->addAction(appTr("menu.validate_world"));
    validate_action->setToolTip(appTr("menu.validate_world_tip"));
    validate_action->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R));
    connect(validate_action, &QAction::triggered, this, &MainWindow::validateLib);
    tools_menu->addSeparator();
    auto* export_overlays_action = tools_menu->addAction(appTr("menu.export_overlays"));
    connect(export_overlays_action, &QAction::triggered, this, &MainWindow::exportOverlays);
    auto* align_all_exits_action = tools_menu->addAction(appTr("menu.align_exits"));
    align_all_exits_action->setToolTip(appTr("menu.align_exits_tip"));
    connect(align_all_exits_action, &QAction::triggered, this, &MainWindow::alignAllInboundExitDescriptions);
    auto* sync_zon_comments_action = tools_menu->addAction(appTr("menu.sync_zon_comments"));
    sync_zon_comments_action->setToolTip(appTr("menu.sync_zon_comments_tip"));
    connect(sync_zon_comments_action, &QAction::triggered, this, &MainWindow::syncZoneResetComments);
    tools_menu->addSeparator();
    auto* fix_ascii_field_action = tools_menu->addAction(appTr("menu.fix_mud_ascii_field"));
    fix_ascii_field_action->setToolTip(appTr("menu.fix_mud_ascii_field_tip"));
    fix_ascii_field_action->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_A));
    connect(fix_ascii_field_action, &QAction::triggered, this, &MainWindow::normalizeFocusedMudAscii);
    auto* fix_ascii_room_action = tools_menu->addAction(appTr("menu.fix_mud_ascii_room"));
    fix_ascii_room_action->setToolTip(appTr("menu.fix_mud_ascii_room_tip"));
    connect(fix_ascii_room_action, &QAction::triggered, this, &MainWindow::normalizeRoomMudAscii);
    tools_menu->addSeparator();
    auto* find_dup_vnums_action = tools_menu->addAction(appTr("menu.find_duplicate_vnums"));
    find_dup_vnums_action->setToolTip(appTr("menu.find_duplicate_vnums_tip"));
    connect(find_dup_vnums_action, &QAction::triggered, this, &MainWindow::findDuplicateEntityVnums);
    auto* special_catalog_action = tools_menu->addAction(appTr("menu.special_proc_catalog"));
    special_catalog_action->setToolTip(appTr("menu.special_proc_catalog_tip"));
    connect(special_catalog_action, &QAction::triggered, this, &MainWindow::editSpecialProcCatalog);

    auto* prefs_menu = menuBar()->addMenu(appTr("menu.prefs"));
    auto* line_limit_action = prefs_menu->addAction(appTr("menu.line_limit"));
    connect(line_limit_action, &QAction::triggered, this, &MainWindow::editLineLengthLimit);

    auto* extended_color_action = prefs_menu->addAction(appTr("menu.extended_colors"));
    extended_color_action->setCheckable(true);
    extended_color_action->setChecked(app_config_.show_color_codes);
    connect(extended_color_action, &QAction::toggled, this, &MainWindow::toggleExtendedColorView);

    aree_archive_on_first_save_action_ = prefs_menu->addAction(appTr("aree.archive_on_first_save"));
    aree_archive_on_first_save_action_->setCheckable(true);
    aree_archive_on_first_save_action_->setChecked(app_config_.aree_archive_on_first_save);
    connect(aree_archive_on_first_save_action_, &QAction::toggled, this, [this](bool enabled) {
        app_config_.aree_archive_on_first_save = enabled;
        nebbie::qt::write_config(app_config_);
        if (!enabled) {
            aree_pending_session_archive_ = false;
        }
    });

    auto* language_menu = prefs_menu->addMenu(appTr("menu.language"));
    auto* language_group = new QActionGroup(this);
    language_group->setExclusive(true);
    auto* italian_action = language_menu->addAction(appTr("menu.language_it"));
    italian_action->setCheckable(true);
    italian_action->setData(static_cast<int>(nebbie::qt::AppLanguage::Italian));
    language_group->addAction(italian_action);
    auto* english_action = language_menu->addAction(appTr("menu.language_en"));
    english_action->setCheckable(true);
    english_action->setData(static_cast<int>(nebbie::qt::AppLanguage::English));
    language_group->addAction(english_action);
    if (nebbie::qt::currentAppLanguage() == nebbie::qt::AppLanguage::English) {
        english_action->setChecked(true);
    } else {
        italian_action->setChecked(true);
    }
    connect(language_group, &QActionGroup::triggered, this, [this](QAction* action) {
        setUiLanguage(static_cast<nebbie::qt::AppLanguage>(action->data().toInt()));
    });

    auto* colors_menu = menuBar()->addMenu(appTr("menu.colors"));
    auto* legend_action = colors_menu->addAction(appTr("menu.color_legend"));
    connect(legend_action, &QAction::triggered, this, &MainWindow::showColorLegend);
    auto* insert_color_action = colors_menu->addAction(appTr("menu.insert_color"));
    connect(insert_color_action, &QAction::triggered, this, &MainWindow::insertColorCode);

    auto* coordinator_menu = menuBar()->addMenu(appTr("menu.coordinator"));
    auto* coordinator_config_action = coordinator_menu->addAction(appTr("menu.coordinator_config"));
    connect(coordinator_config_action, &QAction::triggered, this, &MainWindow::configureCoordinator);
    auto* refresh_index_action = coordinator_menu->addAction(appTr("menu.refresh_index"));
    connect(refresh_index_action, &QAction::triggered, this, &MainWindow::refreshWorldIndex);
    auto* load_index_action = coordinator_menu->addAction(appTr("menu.load_index"));
    connect(load_index_action, &QAction::triggered, this, &MainWindow::loadWorldIndexFromFile);
    auto* export_index_action = coordinator_menu->addAction(appTr("menu.export_index"));
    connect(export_index_action, &QAction::triggered, this, &MainWindow::exportLocalWorldIndex);
    coordinator_menu->addSeparator();
    auto* reserve_action = coordinator_menu->addAction(appTr("menu.reserve_vnums"));
    connect(reserve_action, &QAction::triggered, this, &MainWindow::reserveVnums);

    auto* help_menu = menuBar()->addMenu(appTr("menu.help"));
    auto* check_updates_action = help_menu->addAction(appTr("menu.check_updates"));
    connect(check_updates_action, &QAction::triggered, this, &MainWindow::checkForUpdates);
    auto* auto_updates_action = help_menu->addAction(appTr("menu.check_updates_startup"));
    auto_updates_action->setCheckable(true);
    auto_updates_action->setChecked(app_config_.check_updates);
    connect(auto_updates_action, &QAction::toggled, this, &MainWindow::toggleCheckUpdatesOnStartup);
    help_menu->addSeparator();
    help_menu->addAction(appTr("menu.help_guide"), this, [this]() {
        nebbie::qt::showMenuGuideDialog(this);
    });
    help_menu->addAction(appTr("menu.about_izanagi"), this, [this]() {
        QMessageBox::about(this,
                           nebbie::qt::izanagiDisplayName(),
                           nebbie::qt::izanagiAboutText(QApplication::applicationVersion()));
    });
}

void MainWindow::retranslateUi() {
    updateBranding();
    if (lib_path_.empty()) {
        lib_label_->setText(appTr("status.no_lib"));
        setStatus(appTr("status.open_lib_to_start"));
    }
    if (entity_search_) {
        entity_search_->setPlaceholderText(appTr("ui.search_vnum"));
    }
    room_apply_button_->setText(appTr("ui.apply_changes_en"));
    room_sync_exits_button_->setText(appTr("ui.sync_exit_desc"));
    room_sync_exits_button_->setToolTip(appTr("ui.sync_exit_desc_tip"));
    room_align_all_exits_button_->setText(appTr("ui.align_exits_world"));
    room_align_all_exits_button_->setToolTip(appTr("ui.align_exits_world_tip"));
    room_goto_exit_button_->setText(appTr("ui.goto_exit"));
    mob_apply_button_->setText(appTr("ui.apply_changes"));
    obj_apply_button_->setText(appTr("ui.apply_changes"));
    zone_apply_button_->setText(appTr("ui.apply_changes_en"));
    rebuildNavCategoryLabels();
    updateNavCreateAction();
    if (map_tabs_) {
        map_tabs_->setTabText(0, appTr("tab.map_zone"));
        map_tabs_->setTabText(1, appTr("tab.map_world"));
    }
    if (aree_dock_) {
        aree_dock_->setWindowTitle(appTr("aree.dock_title"));
        const QStringList headers = {appTr("aree.col_name"), appTr("aree.col_zone"), appTr("aree.col_range"),
                                     appTr("aree.col_folder")};
        aree_table_->setHorizontalHeaderLabels(headers);
        aree_open_button_->setText(appTr("aree.open_area"));
        if (aree_archive_button_) {
            aree_archive_button_->setText(appTr("aree.archive_area"));
        }
        aree_restore_button_->setText(appTr("aree.restore_archive"));
        if (aree_archive_on_first_save_action_) {
            aree_archive_on_first_save_action_->setText(appTr("aree.archive_on_first_save"));
            aree_archive_on_first_save_action_->setChecked(app_config_.aree_archive_on_first_save);
        }
    }
    if (lib_path_.empty() && statusBar()->currentMessage().isEmpty()) {
        statusBar()->showMessage(appTr("status.ready"));
    }
}

void MainWindow::updateBranding() {
    QString title = nebbie::qt::izanagiWindowTitle();
    if (dirty_) {
        title += QStringLiteral(" *");
    }
    setWindowTitle(title);
    QApplication::setApplicationDisplayName(nebbie::qt::izanagiWindowTitle());
}

void MainWindow::setUiLanguage(const nebbie::qt::AppLanguage language) {
    if (nebbie::qt::currentAppLanguage() == language) {
        return;
    }
    nebbie::qt::setAppLanguage(language);
    app_config_.ui_language = nebbie::qt::languageCode(language);
    nebbie::qt::write_config(app_config_);
    menuBar()->clear();
    setupMenus();
    retranslateUi();
}

void MainWindow::changeEvent(QEvent* event) {
    QMainWindow::changeEvent(event);
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
}

void MainWindow::rememberLibPath(const std::filesystem::path& path) {
    app_config_.lib_path = nebbie::qt::qstring_from_path(path);
    nebbie::qt::write_config(app_config_);
}

std::unordered_set<long> MainWindow::workspaceKnownRoomVnums() const {
    if (!aree_workspace_) {
        return {};
    }
    return nebbie::collect_aree_workspace_room_vnums(*aree_workspace_);
}

nebbie::ValidationOptions MainWindow::validationOptions() const {
    nebbie::ValidationOptions options;
    options.max_line_length = app_config_.max_line_length;
    // Cross-area exits (e.g. tower -> avamposti) are valid if the destination exists
    // anywhere in the workspace, not only in the currently open area.
    options.known_room_vnums = workspaceKnownRoomVnums();
    return options;
}

void MainWindow::applySpecialProcCatalogFromConfig() {
    nebbie::SpecialProcCatalogConfig config;
    if (!app_config_.server_root.trimmed().isEmpty()) {
        config.server_root = nebbie::qt::path_from_qstring(app_config_.server_root.trimmed());
    } else {
        config.server_root = nebbie::default_nebbie_server_root();
    }
    config.extra_allowlist_path =
        nebbie::qt::path_from_qstring(nebbie::qt::default_extra_special_procs_path());
    nebbie::configure_special_proc_catalog(std::move(config));
}

void MainWindow::editSpecialProcCatalog() {
    nebbie::qt::SpecialProcCatalogDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted || !dialog.saved()) {
        return;
    }

    app_config_.server_root = dialog.serverRoot();
    nebbie::qt::write_config(app_config_);

    const auto allowlist_path =
        nebbie::qt::path_from_qstring(nebbie::qt::default_extra_special_procs_path());
    std::vector<std::string> extras;
    for (const QString& name : dialog.extraNames()) {
        extras.push_back(name.toStdString());
    }
    nebbie::set_extra_special_proc_names(std::move(extras));
    if (!nebbie::save_extra_special_procs(allowlist_path)) {
        QMessageBox::warning(this, appTr("menu.special_proc_catalog"),
                             QStringLiteral("Impossibile salvare l'allowlist in %1")
                                 .arg(nebbie::qt::default_extra_special_procs_path()));
    }

    applySpecialProcCatalogFromConfig();
    if (specials_editor_) {
        specials_editor_->reloadSpecialProcNameChoices();
    }

    const auto status = nebbie::special_proc_catalog_status();
    setStatus(QStringLiteral("Catalogo special proc aggiornato (%1 mob/obj, %2 room, allowlist %3)")
                  .arg(status.effective_mob_object_count)
                  .arg(status.effective_room_count)
                  .arg(status.extra_count));
}

void MainWindow::applyTextEditorSettings() {
    room_editor_->setMaxLineLength(app_config_.max_line_length);
    room_editor_->setShowColorCodes(app_config_.show_color_codes);
    mob_editor_->setMaxLineLength(app_config_.max_line_length);
    mob_editor_->setShowColorCodes(app_config_.show_color_codes);
    obj_editor_->setMaxLineLength(app_config_.max_line_length);
    obj_editor_->setShowColorCodes(app_config_.show_color_codes);
    zone_editor_->setMaxLineLength(app_config_.max_line_length);
    zone_editor_->setShowColorCodes(app_config_.show_color_codes);
    shops_editor_->setMaxLineLength(app_config_.max_line_length);
    shops_editor_->setShowColorCodes(app_config_.show_color_codes);
    specials_editor_->setMaxLineLength(app_config_.max_line_length);
    specials_editor_->setShowColorCodes(app_config_.show_color_codes);
    global_tables_editor_->setMaxLineLength(app_config_.max_line_length);
    global_tables_editor_->setShowColorCodes(app_config_.show_color_codes);
}

void MainWindow::toggleExtendedColorView(bool enabled) {
    app_config_.show_color_codes = enabled;
    nebbie::qt::write_config(app_config_);
    applyTextEditorSettings();
    if (auto* delegate = roomNavDelegate(nav_tree_)) {
        delegate->setShowColorCodes(enabled);
        nav_tree_->viewport()->update();
    }
    if (enabled) {
        setStatus(appTr("status.extended_on"));
    } else {
        setStatus(appTr("status.extended_off"));
    }
}

void MainWindow::showColorLegend() {
    nebbie::qt::MudColorLegendDialog dialog(this);
    dialog.exec();
}

void MainWindow::insertColorCode() {
    nebbie::qt::MudColorInsertDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    nebbie::qt::MudColorTextEdit* field = activeMudTextField();
    if (!field) {
        setStatus("Seleziona un campo di testo prima di inserire un colore.");
        return;
    }
    field->insertColorCode(dialog.selectedCode());
    setStatus(QString("Inserito codice %1.").arg(dialog.selectedCode()));
}

void MainWindow::normalizeFocusedMudAscii() {
    nebbie::qt::MudColorTextEdit* field = activeMudTextField();
    if (!field) {
        setStatus(appTr("status.mud_ascii_need_field"));
        return;
    }
    if (field->normalizeMudAsciiText()) {
        setStatus(appTr("status.mud_ascii_fixed"));
    } else {
        setStatus(appTr("status.mud_ascii_unchanged"));
    }
}

void MainWindow::normalizeRoomMudAscii() {
    if (!room_editor_) {
        return;
    }
    if (room_editor_->normalizeMudAsciiAllFields()) {
        applyRoomChanges();
        setStatus(appTr("status.mud_ascii_fixed"));
    } else {
        setStatus(appTr("status.mud_ascii_unchanged"));
    }
}

void MainWindow::syncZoneResetComments() {
    if (lib_path_.empty() || !context_.has_zon || world_.zones.empty()) {
        QMessageBox::information(this, appTr("tools.zon_sync.title"), appTr("tools.zon_sync.need_lib"));
        return;
    }

    const auto confirm = QMessageBox::question(this, appTr("tools.zon_sync.title"),
                                               appTr("tools.zon_sync.confirm"),
                                               QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
    if (confirm != QMessageBox::Yes) {
        return;
    }

    if (!maybeArchiveAreeBeforeFirstSave()) {
        return;
    }

    try {
        flushPendingEditorChanges();

        std::map<std::filesystem::path, std::string> zon_file_backup;
        const auto capture_zon = [&](const std::filesystem::path& path) {
            if (zon_file_backup.count(path) != 0 || !std::filesystem::exists(path)) {
                return;
            }
            std::ifstream in(path, std::ios::binary);
            if (!in) {
                return;
            }
            zon_file_backup[path] =
                std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        };
        if (context_.zone_sources.empty()) {
            const std::filesystem::path primary =
                context_.zon_path.empty() ? std::filesystem::path(nebbie::ZONE_FILE) : context_.zon_path;
            capture_zon(lib_path_ / primary);
        } else {
            for (const auto& entry : context_.zone_sources) {
                capture_zon(lib_path_ / entry.second);
            }
        }
        if (zon_file_backup.empty()) {
            throw std::runtime_error("nessun file .zon trovato su disco da cui fare rollback");
        }

        std::unordered_set<int> zones_to_touch;
        for (const nebbie::Zone& zone : world_.zones) {
            if (nebbie::zone_reset_comments_need_sync(zone, world_)) {
                zones_to_touch.insert(zone.num);
            }
        }
        if (zones_to_touch.empty()) {
            QMessageBox::information(this, appTr("tools.zon_sync.title"), appTr("tools.zon_sync.noop"));
            return;
        }

        const std::vector<nebbie::Zone> zones_backup = world_.zones;
        std::size_t cmds_before = 0;
        for (const nebbie::Zone& zone : world_.zones) {
            cmds_before += zone.commands.size();
        }

        nebbie::sync_all_zone_reset_comments(world_);

        std::size_t cmds_after = 0;
        for (const nebbie::Zone& zone : world_.zones) {
            cmds_after += zone.commands.size();
        }
        const std::size_t removed =
            cmds_before > cmds_after ? (cmds_before - cmds_after) : std::size_t{0};

        nebbie::LibContext save_ctx = context_;
        save_ctx.root = lib_path_;
        save_ctx.dirty_room_vnums.clear();
        save_ctx.dirty_mobile_vnums.clear();
        save_ctx.dirty_object_vnums.clear();
        save_ctx.dirty_zone_nums = zones_to_touch;
        nebbie::save_lib(world_, save_ctx, {});
        context_.zon_file_snapshot_by_leaf = save_ctx.zon_file_snapshot_by_leaf;

        for (const int zone_num : zones_to_touch) {
            dirty_zone_nums_.erase(zone_num);
        }
        if (dirty_room_vnums_.empty() && dirty_mobile_vnums_.empty() && dirty_object_vnums_.empty()
            && dirty_zone_nums_.empty()) {
            dirty_ = false;
            updateBranding();
        }

        refreshZoneList();

        QMessageBox box(this);
        box.setIcon(QMessageBox::Question);
        box.setWindowTitle(appTr("tools.zon_sync.title"));
        box.setText(appTr("tools.zon_sync.done", QString::number(static_cast<qulonglong>(cmds_before)),
                          QString::number(static_cast<qulonglong>(cmds_after)),
                          QString::number(static_cast<qulonglong>(removed)),
                          QString::number(static_cast<int>(zones_to_touch.size()))));
        auto* keep_button = box.addButton(appTr("tools.zon_sync.keep"), QMessageBox::AcceptRole);
        auto* undo_button = box.addButton(appTr("tools.zon_sync.undo"), QMessageBox::RejectRole);
        box.setDefaultButton(keep_button);
        box.exec();

        if (box.clickedButton() == undo_button) {
            world_.zones = zones_backup;
            for (const auto& [path, bytes] : zon_file_backup) {
                std::ofstream out(path, std::ios::binary | std::ios::trunc);
                if (!out) {
                    throw std::runtime_error("impossibile ripristinare " + path.string());
                }
                out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
                if (!out) {
                    throw std::runtime_error("scrittura rollback fallita: " + path.string());
                }
                context_.zon_file_snapshot_by_leaf[path.filename().string()] = bytes;
            }
            refreshZoneList();
            setStatus(appTr("tools.zon_sync.undone"));
            QMessageBox::information(this, appTr("tools.zon_sync.title"),
                                     appTr("tools.zon_sync.undone"));
            return;
        }

        setStatus(appTr("tools.zon_sync.kept"));
    } catch (const std::exception& ex) {
        QMessageBox::critical(this, appTr("tools.zon_sync.title"),
                              appTr("tools.zon_sync.failed", QString::fromUtf8(ex.what())));
    }
}

void MainWindow::findDuplicateEntityVnums() {
    if (!aree_workspace_) {
        QMessageBox::information(this, appTr("tools.dup_vnums.title"),
                                 appTr("tools.dup_vnums.need_workspace"));
        return;
    }

    const nebbie::AreeDuplicateVnumReport report =
        nebbie::find_aree_workspace_duplicate_entity_vnums(*aree_workspace_);
    const int area_count = static_cast<int>(aree_areas_.size());

    QDialog dialog(this);
    dialog.setWindowTitle(appTr("tools.dup_vnums.title"));
    auto* layout = new QVBoxLayout(&dialog);
    auto* summary = new QLabel(&dialog);
    summary->setWordWrap(true);
    if (report.empty()) {
        summary->setText(appTr("tools.dup_vnums.summary_ok", QString::number(area_count)));
    } else {
        summary->setText(appTr("tools.dup_vnums.summary_found",
                               QString::number(report.mobiles.size()),
                               QString::number(report.objects.size()), QString::number(area_count)));
    }
    layout->addWidget(summary);

    auto* list = new QListWidget(&dialog);
    list->setMinimumHeight(360);
    list->setAlternatingRowColors(true);

    constexpr int kRoleKind = Qt::UserRole;
    constexpr int kRoleVnum = Qt::UserRole + 1;
    constexpr int kRoleArea = Qt::UserRole + 2;
    constexpr int kKindMob = 1;
    constexpr int kKindObj = 2;

    const auto append_hits = [&](const std::vector<nebbie::AreeDuplicateVnumGroup>& groups, int kind,
                                 const char* label_key) {
        for (const auto& group : groups) {
            for (const std::string& area_folder : group.area_folders) {
                auto* item = new QListWidgetItem(
                    appTr(label_key, QString::number(group.vnum), QString::fromStdString(area_folder)),
                    list);
                item->setData(kRoleKind, kind);
                item->setData(kRoleVnum, static_cast<qlonglong>(group.vnum));
                item->setData(kRoleArea, QString::fromStdString(area_folder));
            }
        }
    };
    append_hits(report.mobiles, kKindMob, "tools.dup_vnums.mob_hit");
    append_hits(report.objects, kKindObj, "tools.dup_vnums.obj_hit");

    layout->addWidget(list, 1);

    connect(list, &QListWidget::itemDoubleClicked, &dialog, [this, &dialog](QListWidgetItem* item) {
        if (!item || !aree_workspace_) {
            return;
        }
        const int kind = item->data(Qt::UserRole).toInt();
        const long vnum = static_cast<long>(item->data(Qt::UserRole + 1).toLongLong());
        const std::string area_folder = item->data(Qt::UserRole + 2).toString().toStdString();

        const nebbie::AreeAreaInfo* area = nullptr;
        for (const nebbie::AreeAreaInfo& candidate : aree_areas_) {
            if (candidate.folder_name == area_folder) {
                area = &candidate;
                break;
            }
        }
        if (!area) {
            QMessageBox::warning(&dialog, appTr("tools.dup_vnums.title"),
                                 appTr("tools.dup_vnums.area_missing", QString::fromStdString(area_folder)));
            return;
        }

        dialog.accept();
        loadAreeArea(*area);
        if (kind == 1) {
            selectMobByVnum(vnum);
        } else if (kind == 2) {
            selectObjectByVnum(vnum);
        }
    });

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    layout->addWidget(buttons);

    dialog.resize(720, 520);
    dialog.exec();
}

nebbie::qt::MudColorTextEdit* MainWindow::activeMudTextField() const {
    const auto pick = [](nebbie::qt::MudColorTextEdit* field) -> nebbie::qt::MudColorTextEdit* {
        return field && field->hasFocus() ? field : nullptr;
    };
    if (auto* field = pick(room_editor_->focusedMudField())) {
        return field;
    }
    if (auto* field = pick(mob_editor_->focusedMudField())) {
        return field;
    }
    if (auto* field = pick(obj_editor_->focusedMudField())) {
        return field;
    }
    if (auto* field = pick(zone_editor_->focusedMudField())) {
        return field;
    }
    if (auto* field = pick(shops_editor_->focusedMudField())) {
        return field;
    }
    if (auto* field = pick(specials_editor_->focusedMudField())) {
        return field;
    }
    if (auto* field = pick(global_tables_editor_->focusedMudField())) {
        return field;
    }
    return nullptr;
}

void MainWindow::scheduleStartupUpdateCheck() {
    if (!nebbie::qt::shouldCheckForUpdates(app_config_.check_updates,
                                           false,
                                           app_config_.last_update_check)) {
        return;
    }
    QTimer::singleShot(3000, this, [this]() {
        update_checker_->checkForUpdates(*network_,
                                         false,
                                         this,
                                         app_config_.dismissed_update_version);
    });
}

void MainWindow::checkForUpdates() {
    update_checker_->checkForUpdates(*network_,
                                     true,
                                     this,
                                     app_config_.dismissed_update_version);
}

void MainWindow::onUpdateCheckFinished(const nebbie::qt::ReleaseUpdateInfo& info) {
    if (!info.ok) {
        return;
    }
    app_config_.last_update_check = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    if (!info.user_dismissed_version.isEmpty()) {
        app_config_.dismissed_update_version = info.user_dismissed_version;
    }
    nebbie::qt::write_config(app_config_);
}

void MainWindow::toggleCheckUpdatesOnStartup(const bool enabled) {
    app_config_.check_updates = enabled;
    nebbie::qt::write_config(app_config_);
    if (enabled) {
        setStatus("Controllo aggiornamenti all'avvio attivo.");
    } else {
        setStatus("Controllo aggiornamenti all'avvio disattivato.");
    }
}

void MainWindow::editLineLengthLimit() {
    bool ok = false;
    const int value = QInputDialog::getInt(
        this,
        "Limite caratteri per riga",
        "Numero massimo di caratteri per riga nei testi MUD "
        "(stanze, mob, oggetti, zone e dati mondo).\n"
        "Imposta 0 per disattivare.\n\n"
        "Salvato in:\n" + nebbie::qt::default_config_path(),
        app_config_.max_line_length,
        0,
        512,
        1,
        &ok);
    if (!ok) {
        return;
    }

    app_config_.max_line_length = value;
    nebbie::qt::write_config(app_config_);
    applyTextEditorSettings();
    if (app_config_.max_line_length > 0) {
        setStatus(QString("Limite righe impostato a %1 caratteri.").arg(app_config_.max_line_length));
    } else {
        setStatus("Controllo lunghezza righe disattivato.");
    }
}

void MainWindow::openLibPath(const QString& path) {
    if (path.isEmpty()) {
        return;
    }
    if (!lib_path_.empty()) {
        const std::filesystem::path requested = nebbie::qt::path_from_qstring(path);
        const std::filesystem::path resolved = nebbie::resolve_lib_directory(requested);
        if (resolved != lib_path_ && !confirmSaveIfDirty()) {
            return;
        }
    }
    try {
        const std::filesystem::path requested = nebbie::qt::path_from_qstring(path);
        const std::filesystem::path resolved = nebbie::resolve_lib_directory(requested);
        if (!std::filesystem::exists(resolved)) {
            QMessageBox::warning(this, "Errore caricamento libreria",
                                 QString("Percorso non valido:\n%1").arg(path));
            return;
        }
        if (!nebbie::directory_has_lib_files(resolved)) {
            QMessageBox::warning(
                this,
                "Errore caricamento libreria",
                QString("La cartella selezionata non contiene file libreria Nebbie "
                        "(.zon, .wld, .mob, .obj, ...).\n\n"
                        "Percorso richiesto: %1\n"
                        "Percorso risolto: %2")
                    .arg(path)
                    .arg(nebbie::qt::qstring_from_path(resolved)));
            return;
        }
        const nebbie::AreeLibOpenGuard guard = nebbie::classify_aree_lib_open_guard(resolved);
        if (guard.reason != nebbie::AreeLibOpenBlockReason::none) {
            const QString resolved_qs = nebbie::qt::qstring_from_path(resolved);
            const QString workspace_qs = nebbie::qt::qstring_from_path(guard.suggested_workspace_root);
            const QString body =
                guard.reason == nebbie::AreeLibOpenBlockReason::area_directory
                    ? appTr("aree.lib_open_block_area", resolved_qs, workspace_qs)
                    : appTr("aree.lib_open_block_workspace", resolved_qs);
            QMessageBox box(this);
            box.setIcon(QMessageBox::Warning);
            box.setWindowTitle(appTr("aree.lib_open_block_title"));
            box.setText(body);
            QPushButton* open_workspace =
                box.addButton(appTr("aree.lib_open_use_workspace"), QMessageBox::AcceptRole);
            box.addButton(QMessageBox::Cancel);
            box.exec();
            if (box.clickedButton() == open_workspace) {
                const QString select_area =
                    guard.reason == nebbie::AreeLibOpenBlockReason::area_directory
                        ? QString::fromStdString(resolved.filename().string())
                        : QString();
                openAreeWorkspaceFromPath(workspace_qs, select_area);
            }
            return;
        }
        loadLib(resolved);
        rememberLibPath(resolved);
        if (resolved != requested) {
            setStatus(QString("Libreria risolta in: %1").arg(nebbie::qt::qstring_from_path(resolved)));
        }
        showLoadWarningsIfAny();
    } catch (const std::exception& ex) {
        const std::filesystem::path resolved = nebbie::resolve_lib_directory(nebbie::qt::path_from_qstring(path));
        const QString detail = QString::fromUtf8(ex.what())
                               + QString("\n\nPercorso richiesto: %1").arg(path)
                               + QString("\nPercorso risolto: %1")
                                     .arg(nebbie::qt::qstring_from_path(resolved));
        QMessageBox::critical(this, "Errore caricamento libreria", detail);
    }
}

void MainWindow::openBundledSampleLib() {
    if (!confirmSaveIfDirty()) {
        return;
    }

    const QStringList candidates = {
#if defined(Q_OS_MACOS) || defined(Q_OS_MAC)
        QDir(QCoreApplication::applicationDirPath())
            .absoluteFilePath(QStringLiteral("../Resources/sample-mudroot/lib")),
#endif
        QStringLiteral("/usr/share/nebbie-editor/sample-mudroot/lib"),
        QStringLiteral("/usr/share/nebbie-translate/sample-mudroot/lib"),
        QDir(QCoreApplication::applicationDirPath())
            .absoluteFilePath(QStringLiteral("sample-mudroot/lib")),
        QDir(QCoreApplication::applicationDirPath())
            .absoluteFilePath(QStringLiteral("../sample-mudroot/lib")),
    };

    for (const QString& candidate : candidates) {
        const QFileInfo info(candidate);
        if (info.isDir()) {
            openLibPath(info.canonicalFilePath());
            return;
        }
    }

    QMessageBox::information(this, appTr("menu.open_sample_lib"), appTr("dialog.sample_lib_missing"));
}

void MainWindow::openLib() {
    if (!confirmSaveIfDirty()) {
        return;
    }

    const QString initial = nebbie::qt::read_lib_path();
    const QString dir = QFileDialog::getExistingDirectory(
        this, appTr("dialog.open_lib_title"), initial.isEmpty() ? QDir::homePath() : initial,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (dir.isEmpty()) {
        return;
    }
    openLibPath(dir);
}

void MainWindow::reloadLib() {
    if (lib_path_.empty()) {
        QMessageBox::information(this, "Aggiorna libreria", "Apri prima una libreria.");
        return;
    }
    if (!confirmSaveIfDirty()) {
        return;
    }

    try {
        loadLib(lib_path_);
        setStatus(QString("Libreria ricaricata da %1.")
                      .arg(nebbie::qt::qstring_from_path(lib_path_)));
    } catch (const std::exception& ex) {
        QMessageBox::critical(this, "Aggiorna libreria", QString::fromUtf8(ex.what()));
    }
}

void MainWindow::openStartupLib() {
    if (tryRestoreAreeSessionOnStartup()) {
        return;
    }

    const QString saved = nebbie::qt::read_lib_path();
    if (!saved.isEmpty()) {
        const std::filesystem::path path = nebbie::qt::path_from_qstring(saved);
        const std::filesystem::path resolved = nebbie::resolve_lib_directory(path);
        if (nebbie::aree_workspace_has_areas(resolved)) {
            openAreeWorkspaceFromPath(nebbie::qt::qstring_from_path(resolved), app_config_.aree_last_area);
            tryLoadLastAreeAreaWithoutArchive();
            return;
        }
        const nebbie::AreeLibOpenGuard guard = nebbie::classify_aree_lib_open_guard(resolved);
        if (guard.reason == nebbie::AreeLibOpenBlockReason::workspace_root) {
            openAreeWorkspaceFromPath(nebbie::qt::qstring_from_path(resolved), app_config_.aree_last_area);
            tryLoadLastAreeAreaWithoutArchive();
            return;
        }
        if (guard.reason == nebbie::AreeLibOpenBlockReason::area_directory) {
            openAreeWorkspaceFromPath(nebbie::qt::qstring_from_path(guard.suggested_workspace_root),
                                      QString::fromStdString(resolved.filename().string()));
            tryLoadLastAreeAreaWithoutArchive();
            return;
        }
    }

    promptStartupLibrary();
}

bool MainWindow::tryRestoreAreeSessionOnStartup() {
    if (app_config_.aree_workspace_root.isEmpty()) {
        return false;
    }

    const std::filesystem::path root = nebbie::qt::path_from_qstring(app_config_.aree_workspace_root);
    if (!nebbie::aree_workspace_has_areas(root)) {
        return false;
    }

    openAreeWorkspaceFromPath(app_config_.aree_workspace_root, app_config_.aree_last_area);
    tryLoadLastAreeAreaWithoutArchive();
    return true;
}

void MainWindow::tryLoadLastAreeAreaWithoutArchive() {
    if (!aree_workspace_ || app_config_.aree_last_area.isEmpty()) {
        return;
    }

    const std::string target = app_config_.aree_last_area.toStdString();
    for (const nebbie::AreeAreaInfo& area : aree_areas_) {
        if (area.folder_name == target) {
            loadAreeArea(area, false, {});
            return;
        }
    }
}

void MainWindow::promptStartupLibrary() {
    QMessageBox box(this);
    box.setIcon(QMessageBox::Information);
    box.setWindowTitle(appTr("dialog.startup_title"));
    box.setText(appTr("dialog.open_startup_aree_first", nebbie::qt::default_config_path()));

    auto* aree_button = box.addButton(appTr("dialog.startup_open_aree"), QMessageBox::AcceptRole);
    auto* monolith_button = box.addButton(appTr("dialog.startup_open_monolith"), QMessageBox::ActionRole);
    box.addButton(QMessageBox::Cancel);
    box.setDefaultButton(aree_button);

    box.exec();
    if (box.clickedButton() == aree_button) {
        openAreeWorkspace();
    } else if (box.clickedButton() == monolith_button) {
        openLib();
    } else {
        setStatus(appTr("status.open_lib_cancelled"));
    }
}

bool MainWindow::promptForLibPath(const QString& reason) {
    if (!reason.isEmpty()) {
        QMessageBox::information(this, "Libreria Nebbie", reason);
    }
    openLib();
    return !lib_path_.empty();
}

void MainWindow::loadLib(const std::filesystem::path& path) {
    clearAreeMode();
    nebbie::World loaded_world;
    nebbie::LibContext loaded_context;
    nebbie::load_lib(loaded_world, path, loaded_context);

    world_ = std::move(loaded_world);
    context_ = std::move(loaded_context);
    lib_path_ = path;
    session_storage_path_ = path;
    if (entity_search_) {
        entity_search_->clear();
    }
    room_filter_.clear();
    mob_filter_.clear();
    object_filter_.clear();
    zone_filter_.clear();
    markClean();
    for (nebbie::Zone& zone : world_.zones) {
        if (nebbie::sync_zone_reset_comments(zone, world_)) {
            dirty_zone_nums_.insert(zone.num);
            markDirty();
        }
    }
    refreshRoomList();
    refreshMobList();
    refreshObjectList();
    refreshZoneList();
    refreshZoneMap();
    refreshWorldZoneMap();
    zone_editor_->setWorld(&world_);
    room_editor_->setWorld(&world_);
    refreshWorldDataEditors();

    const QString label = QString("Libreria: %1 — %2 zone, %3 stanze, %4 mob, %5 oggetti, %6 negozi")
                              .arg(nebbie::qt::qstring_from_path(path))
                              .arg(world_.zones.size())
                              .arg(world_.rooms.size())
                              .arg(world_.mobiles.size())
                              .arg(world_.objects.size())
                              .arg(world_.shops.size());
    lib_label_->setText(label);
    last_version_time_ = std::chrono::system_clock::now();
    autosave_timer_->start();
}

std::filesystem::path MainWindow::sessionStorageRoot() const {
    if (!session_storage_path_.empty()) {
        return session_storage_path_;
    }
    return lib_path_;
}

void MainWindow::clearAreeMode() {
    aree_workspace_.reset();
    aree_area_folder_.reset();
    aree_areas_.clear();
    aree_pending_session_archive_ = false;
    if (aree_table_) {
        aree_table_->setRowCount(0);
    }
    if (aree_dock_) {
        aree_dock_->hide();
    }
}

void MainWindow::refreshAreeAreaList() {
    if (!aree_workspace_) {
        return;
    }

    aree_areas_ = nebbie::scan_aree_areas(*aree_workspace_);
    const bool was_sorting = aree_table_->isSortingEnabled();
    aree_table_->setSortingEnabled(false);
    aree_table_->setRowCount(static_cast<int>(aree_areas_.size()));
    const auto align_left = [](QTableWidgetItem* item) {
        if (item) {
            item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        }
    };
    for (int row = 0; row < static_cast<int>(aree_areas_.size()); ++row) {
        const nebbie::AreeAreaInfo& area = aree_areas_[static_cast<std::size_t>(row)];
        auto* name_item = makeSortableTextItem(QString::fromStdString(area.zone_name));
        align_left(name_item);
        aree_table_->setItem(row, kAreeColName, name_item);

        auto* zone_item = makeSortableNumericItem(area.zone_num, QString::number(area.zone_num));
        align_left(zone_item);
        aree_table_->setItem(row, kAreeColZone, zone_item);

        const QString range_text =
            QStringLiteral("%1-%2").arg(area.bottom_vnum).arg(area.top_vnum);
        auto* rooms_item = makeSortableNumericItem(area.bottom_vnum, range_text);
        align_left(rooms_item);
        aree_table_->setItem(row, kAreeColRooms, rooms_item);

        auto* folder_item = makeSortableTextItem(QString::fromStdString(area.folder_name));
        folder_item->setData(Qt::UserRole, QString::fromStdString(area.folder_name));
        align_left(folder_item);
        aree_table_->setItem(row, kAreeColFolder, folder_item);
    }
    aree_table_->resizeColumnsToContents();
    aree_table_->setSortingEnabled(was_sorting);

    lib_label_->setText(appTr("aree.status_workspace",
                              nebbie::qt::qstring_from_path(aree_workspace_->root),
                              QString::number(aree_areas_.size())));
}

void MainWindow::openAreeWorkspace() {
    const QString initial = app_config_.aree_workspace_root.isEmpty() ? nebbie::qt::read_lib_path()
                                                                      : app_config_.aree_workspace_root;
    const QString dir = QFileDialog::getExistingDirectory(
        this, appTr("menu.open_aree_workspace"), initial.isEmpty() ? QDir::homePath() : initial,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (dir.isEmpty()) {
        return;
    }
    openAreeWorkspaceFromPath(dir);
}

void MainWindow::openAreeWorkspaceFromPath(const QString& dir, const QString& select_area_folder) {
    if (dir.isEmpty()) {
        return;
    }

    if (dirty_ && !confirmSaveIfDirty()) {
        return;
    }

    const std::filesystem::path root = nebbie::qt::path_from_qstring(dir);
    if (!nebbie::aree_workspace_has_areas(root)) {
        QMessageBox::warning(this, appTr("menu.open_aree_workspace"),
                             appTr("aree.no_workspace"));
        return;
    }

    aree_workspace_ = nebbie::make_aree_workspace(root);
    app_config_.aree_workspace_root = dir;
    nebbie::qt::write_config(app_config_);

    aree_dock_->show();
    refreshAreeAreaList();
    setStatus(appTr("aree.status_workspace", dir, QString::number(aree_areas_.size())));

    QString area_to_select = select_area_folder;
    if (area_to_select.isEmpty() && !app_config_.aree_last_area.isEmpty()) {
        area_to_select = app_config_.aree_last_area;
    }
    if (!area_to_select.isEmpty()) {
        for (int row = 0; row < static_cast<int>(aree_areas_.size()); ++row) {
            const QTableWidgetItem* folder_item = aree_table_->item(row, kAreeColFolder);
            if (folder_item && folder_item->text() == area_to_select) {
                aree_table_->selectRow(row);
                break;
            }
        }
    }
}

void MainWindow::onAreeAreaActivated() {
    const bool has_selection = aree_table_->currentRow() >= 0;
    const bool has_workspace = aree_workspace_.has_value();
    aree_open_button_->setEnabled(has_selection);
    if (aree_archive_button_) {
        aree_archive_button_->setEnabled(has_workspace && (aree_area_folder_.has_value() || has_selection));
    }
    aree_restore_button_->setEnabled(has_selection && has_workspace);
}

bool MainWindow::promptArchiveLabel(const QString& area_name, QString& archive_label) {
    bool ok = false;
    archive_label = QInputDialog::getText(this, appTr("aree.archive_title"),
                                          appTr("aree.archive_prompt", area_name), QLineEdit::Normal,
                                          {}, &ok)
                        .trimmed();
    return ok;
}

bool MainWindow::archiveAreeFolder(const std::string& area_folder, const std::string& archive_label) {
    if (!aree_workspace_) {
        return false;
    }
    try {
        nebbie::archive_aree_area(*aree_workspace_, area_folder, archive_label);
    } catch (const std::exception& ex) {
        QMessageBox::critical(this, appTr("aree.archive_title"),
                              appTr("aree.archive_failed", QString::fromUtf8(ex.what())));
        return false;
    }
    if (aree_area_folder_ && *aree_area_folder_ == area_folder) {
        aree_pending_session_archive_ = false;
    }
    setStatus(appTr("aree.archive_ok", QString::fromStdString(area_folder)));
    return true;
}

bool MainWindow::maybeArchiveAreeBeforeFirstSave() {
    if (!aree_pending_session_archive_ || !app_config_.aree_archive_on_first_save || !aree_workspace_
        || !aree_area_folder_) {
        return true;
    }
    return archiveAreeFolder(*aree_area_folder_, {});
}

const nebbie::AreeAreaInfo* MainWindow::findAreeAreaForRoomVnum(const long room_vnum) const {
    if (room_vnum <= 0) {
        return nullptr;
    }
    for (const nebbie::AreeAreaInfo& area : aree_areas_) {
        if (room_vnum >= area.bottom_vnum && room_vnum <= area.top_vnum) {
            return &area;
        }
    }
    // Fallback: match the #vnum header in each area .wld (range table can miss edge cases).
    const std::string needle = "#" + std::to_string(room_vnum);
    for (const nebbie::AreeAreaInfo& area : aree_areas_) {
        const std::filesystem::path wld = area.path / (area.folder_name + ".wld");
        std::ifstream in(wld);
        if (!in) {
            continue;
        }
        std::string line;
        while (std::getline(in, line)) {
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            if (line == needle || line.rfind(needle, 0) == 0) {
                const std::size_t n = needle.size();
                if (line.size() == n || !std::isdigit(static_cast<unsigned char>(line[n]))) {
                    return &area;
                }
            }
        }
    }
    return nullptr;
}

void MainWindow::showLoadWarningsIfAny() {
    if (context_.load_warnings.empty()) {
        return;
    }
    QString warning_text;
    for (const std::string& warning : context_.load_warnings) {
        if (!warning_text.isEmpty()) {
            warning_text += "\n\n";
        }
        warning_text += QString::fromUtf8(warning.c_str());
    }
    QMessageBox::warning(
        this,
        "Avvisi caricamento",
        QString("Il caricamento e' riuscito, ma ci sono problemi da controllare "
                "(alcuni record potrebbero essere stati saltati):\n\n%1")
            .arg(warning_text));
}

void MainWindow::loadAreeArea(const nebbie::AreeAreaInfo& area, bool archive_first,
                              const std::string& archive_label, const long select_room_vnum) {
    if (!aree_workspace_) {
        return;
    }

    if (archive_first) {
        if (!archiveAreeFolder(area.folder_name, archive_label)) {
            return;
        }
    }

    if (dirty_ && !confirmSaveIfDirty()) {
        return;
    }

    try {
        nebbie::World loaded_world;
        nebbie::LibContext loaded_context;
        loaded_context.write_eof_markers_on_save = false;
        nebbie::load_lib(loaded_world, area.path, loaded_context);

        world_ = std::move(loaded_world);
        context_ = std::move(loaded_context);
        context_.write_eof_markers_on_save = false;
        // Preserve workspace-derived bottoms so suggest/create use the area block, not 0..top.
        for (nebbie::Zone& zone : world_.zones) {
            if (zone.num == area.zone_num) {
                zone.bottom = area.bottom_vnum;
            }
        }
        lib_path_ = area.path;
        session_storage_path_ = nebbie::aree_session_storage_root(*aree_workspace_, area.folder_name);
        aree_area_folder_ = area.folder_name;
        aree_pending_session_archive_ = app_config_.aree_archive_on_first_save;

        app_config_.aree_last_area = QString::fromStdString(area.folder_name);
        nebbie::qt::write_config(app_config_);

        if (entity_search_) {
            entity_search_->clear();
        }
        room_filter_.clear();
        mob_filter_.clear();
        object_filter_.clear();
        zone_filter_.clear();
        markClean();
        // Refresh *! notes (names + MW/MR from .zon args). If they change, mark zone dirty
        // so the next save writes the updated comments.
        for (nebbie::Zone& zone : world_.zones) {
            if (nebbie::sync_zone_reset_comments(zone, world_)) {
                dirty_zone_nums_.insert(zone.num);
                markDirty();
            }
        }
        refreshRoomList();
        refreshMobList();
        refreshObjectList();
        refreshZoneList();
        refreshZoneMap();
        refreshWorldZoneMap();
        zone_editor_->setWorld(&world_);
        room_editor_->setWorld(&world_);
        refreshWorldDataEditors();
        onAreeAreaActivated();

        if (select_room_vnum > 0) {
            selectRoomByVnum(select_room_vnum);
        }

        const QString label = appTr("aree.status_loaded", nebbie::qt::qstring_from_path(area.path),
                                    QString::number(world_.zones.size()),
                                    QString::number(world_.rooms.size()));
        lib_label_->setText(label);
        last_version_time_ = std::chrono::system_clock::now();
        autosave_timer_->start();
        setStatus(label);
        showLoadWarningsIfAny();
    } catch (const std::exception& ex) {
        QMessageBox::critical(
            this,
            appTr("aree.dock_title"),
            QString("Impossibile aprire l'area \"%1\":\n\n%2\n\n"
                    "Percorso: %3")
                .arg(QString::fromStdString(area.folder_name))
                .arg(QString::fromUtf8(ex.what()))
                .arg(nebbie::qt::qstring_from_path(area.path)));
    }
}

void MainWindow::openAreeArea() {
    if (!aree_workspace_) {
        QMessageBox::information(this, appTr("aree.dock_title"), appTr("aree.no_workspace"));
        return;
    }

    const auto selected = selectedAreeArea(aree_table_, aree_areas_);
    if (!selected) {
        QMessageBox::information(this, appTr("aree.dock_title"), appTr("aree.select_area"));
        return;
    }

    loadAreeArea(*selected);
}

void MainWindow::archiveAreeArea() {
    if (!aree_workspace_) {
        QMessageBox::information(this, appTr("aree.dock_title"), appTr("aree.no_workspace"));
        return;
    }

    std::string folder;
    if (aree_area_folder_) {
        folder = *aree_area_folder_;
    } else {
        const auto selected = selectedAreeArea(aree_table_, aree_areas_);
        if (!selected) {
            QMessageBox::information(this, appTr("aree.archive_title"), appTr("aree.no_open_area"));
            return;
        }
        folder = selected->folder_name;
    }

    QString label;
    if (!promptArchiveLabel(QString::fromStdString(folder), label)) {
        return;
    }
    if (archiveAreeFolder(folder, label.toStdString())) {
        QMessageBox::information(this, appTr("aree.archive_title"),
                                 appTr("aree.archive_ok", QString::fromStdString(folder)));
    }
}

void MainWindow::restoreAreeArchive() {
    if (!aree_workspace_) {
        QMessageBox::information(this, appTr("aree.dock_title"), appTr("aree.no_workspace"));
        return;
    }

    const auto selected = selectedAreeArea(aree_table_, aree_areas_);
    if (!selected) {
        QMessageBox::information(this, appTr("aree.dock_title"), appTr("aree.select_area"));
        return;
    }

    const auto archives = nebbie::list_aree_archives(*aree_workspace_, selected->folder_name);
    if (archives.empty()) {
        QMessageBox::information(this, appTr("aree.restore_title"), appTr("aree.no_archives"));
        return;
    }

    QStringList labels;
    labels.reserve(static_cast<int>(archives.size()));
    for (const auto& archive : archives) {
        QString label = QString::fromStdString(archive.id);
        if (!archive.label.empty()) {
            label += QStringLiteral(" — ") + QString::fromStdString(archive.label);
        }
        labels << label;
    }

    bool ok = false;
    const QString choice = QInputDialog::getItem(this, appTr("aree.restore_title"),
                                                 appTr("aree.restore_prompt", QString::fromStdString(selected->folder_name)),
                                                 labels, 0, false, &ok);
    if (!ok || choice.isEmpty()) {
        return;
    }

    const int index = labels.indexOf(choice);
    if (index < 0 || index >= static_cast<int>(archives.size())) {
        return;
    }

    const auto& archive = archives[static_cast<std::size_t>(index)];
    const auto answer = QMessageBox::question(
        this, appTr("aree.restore_title"),
        appTr("aree.restore_confirm", QString::fromStdString(selected->folder_name),
              QString::fromStdString(archive.id)),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes) {
        return;
    }

    try {
        nebbie::restore_aree_area_from_archive(*aree_workspace_, selected->folder_name, archive.path);
    } catch (const std::exception& ex) {
        QMessageBox::critical(this, appTr("aree.restore_title"),
                              appTr("aree.restore_failed", QString::fromUtf8(ex.what())));
        return;
    }

    if (aree_area_folder_ && *aree_area_folder_ == selected->folder_name && !lib_path_.empty()) {
        reloadLib();
    }

    setStatus(appTr("aree.restore_ok"));
    QMessageBox::information(this, appTr("aree.restore_title"), appTr("aree.restore_ok"));
}

void MainWindow::rebuildNavCategoryLabels() {
    if (nav_rooms_) {
        nav_rooms_->setText(0, appTr("tab.rooms"));
    }
    if (nav_mobs_) {
        nav_mobs_->setText(0, appTr("tab.mob"));
    }
    if (nav_objects_) {
        nav_objects_->setText(0, appTr("tab.objects"));
    }
    if (nav_zones_) {
        nav_zones_->setText(0, appTr("tab.zone"));
    }
    if (nav_world_) {
        nav_world_->setText(0, appTr("nav.world"));
    }
    if (nav_shops_) {
        nav_shops_->setText(0, appTr("tab.shops"));
    }
    if (nav_specials_) {
        nav_specials_->setText(0, appTr("tab.specials"));
    }
    if (nav_global_) {
        nav_global_->setText(0, appTr("tab.global_tables"));
    }
    if (nav_map_) {
        nav_map_->setText(0, appTr("tab.map"));
    }
    if (nav_validation_) {
        nav_validation_->setText(0, appTr("tab.validation"));
    }
}

void MainWindow::showEditorPage(const int stack_index) {
    if (editor_stack_ && stack_index >= 0 && stack_index < editor_stack_->count()) {
        editor_stack_->setCurrentIndex(stack_index);
    }
    updateNavCreateAction();
}

void MainWindow::updateNavCreateAction() {
    if (!entity_create_button_) {
        return;
    }
    QTreeWidgetItem* item = nav_tree_ ? nav_tree_->currentItem() : nullptr;
    NavKind kind = navKindOf(item);
    if (kind == NavKind::Room || kind == NavKind::CatRooms) {
        entity_create_button_->setEnabled(true);
        entity_create_button_->setText(appTr("ui.new_room"));
    } else if (kind == NavKind::Mob || kind == NavKind::CatMobs) {
        entity_create_button_->setEnabled(true);
        entity_create_button_->setText(appTr("ui.new_mob"));
    } else if (kind == NavKind::Object || kind == NavKind::CatObjects) {
        entity_create_button_->setEnabled(true);
        entity_create_button_->setText(appTr("ui.new_object"));
    } else if (kind == NavKind::Zone || kind == NavKind::CatZones) {
        entity_create_button_->setEnabled(true);
        entity_create_button_->setText(appTr("ui.new_zone"));
    } else {
        entity_create_button_->setEnabled(false);
        entity_create_button_->setText(appTr("ui.new_room"));
    }
}

QTreeWidgetItem* MainWindow::findEntityTreeItem(QTreeWidgetItem* category, const long id) const {
    if (!category) {
        return nullptr;
    }
    for (int i = 0; i < category->childCount(); ++i) {
        QTreeWidgetItem* child = category->child(i);
        if (child && child->data(0, Qt::UserRole).toLongLong() == id) {
            return child;
        }
    }
    return nullptr;
}

void MainWindow::selectNavCategory(QTreeWidgetItem* category) {
    if (!category || !nav_tree_) {
        return;
    }
    category->setExpanded(true);
    nav_tree_->setCurrentItem(category);
}

void MainWindow::refreshRoomList() {
    if (!nav_rooms_) {
        return;
    }
    const long selected = currentRoomVnum();
    const QSignalBlocker blocker(nav_tree_);
    while (nav_rooms_->childCount() > 0) {
        delete nav_rooms_->takeChild(0);
    }
    const std::string query = room_filter_.toStdString();
    for (const auto& [vnum, room] : world_.rooms) {
        if (!nebbie::entity_matches(vnum, room.name, query)) {
            continue;
        }
        auto* item = new QTreeWidgetItem(nav_rooms_);
        setNavKind(item, NavKind::Room);
        nebbie::qt::setRoomTreeItemData(item, vnum, room.name);
        for (const auto& exit : room.exits) {
            const nebbie::Room* dest = world_.find_room(exit.to_room);
            const QString dest_name =
                dest ? QString::fromStdString(dest->name) : QStringLiteral("?");
            auto* exit_item = new QTreeWidgetItem(item);
            setNavKind(exit_item, NavKind::RoomExit);
            exit_item->setData(0, Qt::UserRole, static_cast<qlonglong>(exit.to_room));
            exit_item->setData(0, Qt::UserRole + 1, exit.direction);
            exit_item->setText(
                0, QStringLiteral("%1 to #%2 (%3)")
                       .arg(QString::fromUtf8(nebbie::exit_direction_name(exit.direction)))
                       .arg(exit.to_room)
                       .arg(dest_name));
        }
    }
    nav_rooms_->setExpanded(true);
    if (selected > 0) {
        if (QTreeWidgetItem* item = findEntityTreeItem(nav_rooms_, selected)) {
            nav_tree_->setCurrentItem(item);
            selected_room_item_ = item;
            onRoomSelected();
        }
    }
}

void MainWindow::refreshMobList() {
    if (!nav_mobs_) {
        return;
    }
    const long selected = selected_mob_item_
                              ? selected_mob_item_->data(0, Qt::UserRole).toLongLong()
                              : 0;
    const QSignalBlocker blocker(nav_tree_);
    while (nav_mobs_->childCount() > 0) {
        delete nav_mobs_->takeChild(0);
    }
    const std::string query = mob_filter_.toStdString();
    for (const auto& [vnum, mob] : world_.mobiles) {
        if (!nebbie::entity_matches(vnum, mob.short_descr, query)) {
            continue;
        }
        auto* item = new QTreeWidgetItem(nav_mobs_);
        setNavKind(item, NavKind::Mob);
        item->setData(0, Qt::UserRole, static_cast<qlonglong>(vnum));
        item->setText(0, QString("#%1 %2").arg(vnum).arg(QString::fromStdString(mob.short_descr)));
    }
    nav_mobs_->setExpanded(true);
    if (selected > 0) {
        if (QTreeWidgetItem* item = findEntityTreeItem(nav_mobs_, selected)) {
            selected_mob_item_ = item;
            nav_tree_->setCurrentItem(item);
            onMobSelected();
        } else {
            selected_mob_item_ = nullptr;
        }
    }
}

void MainWindow::refreshObjectList() {
    if (!nav_objects_) {
        return;
    }
    const long selected = selected_obj_item_
                              ? selected_obj_item_->data(0, Qt::UserRole).toLongLong()
                              : 0;
    const QSignalBlocker blocker(nav_tree_);
    while (nav_objects_->childCount() > 0) {
        delete nav_objects_->takeChild(0);
    }
    const std::string query = object_filter_.toStdString();
    for (const auto& [vnum, obj] : world_.objects) {
        if (!nebbie::entity_matches(vnum, obj.short_descr, query)) {
            continue;
        }
        auto* item = new QTreeWidgetItem(nav_objects_);
        setNavKind(item, NavKind::Object);
        item->setData(0, Qt::UserRole, static_cast<qlonglong>(vnum));
        item->setText(0, QString("#%1 %2").arg(vnum).arg(QString::fromStdString(obj.short_descr)));
    }
    nav_objects_->setExpanded(true);
    if (selected > 0) {
        if (QTreeWidgetItem* item = findEntityTreeItem(nav_objects_, selected)) {
            selected_obj_item_ = item;
            nav_tree_->setCurrentItem(item);
            onObjSelected();
        } else {
            selected_obj_item_ = nullptr;
        }
    }
}

void MainWindow::refreshZoneList() {
    if (!nav_zones_) {
        return;
    }
    const int previous_selected = currentZoneNum();
    const QSignalBlocker blocker(nav_tree_);
    while (nav_zones_->childCount() > 0) {
        delete nav_zones_->takeChild(0);
    }
    const std::string query = zone_filter_.toStdString();
    for (const auto& zone : world_.zones) {
        if (!query.empty()
            && !nebbie::entity_matches(zone.num, zone.name, query)) {
            continue;
        }
        auto* item = new QTreeWidgetItem(nav_zones_);
        setNavKind(item, NavKind::Zone);
        item->setData(0, Qt::UserRole, static_cast<qlonglong>(zone.num));
        item->setText(0,
                      QString("#%1 %2 [%3-%4]")
                          .arg(zone.num)
                          .arg(QString::fromStdString(zone.name))
                          .arg(zone.bottom)
                          .arg(zone.top));
    }
    nav_zones_->setExpanded(true);
    if (previous_selected > 0) {
        if (QTreeWidgetItem* item = findEntityTreeItem(nav_zones_, previous_selected)) {
            selected_zone_item_ = item;
            nav_tree_->setCurrentItem(item);
        } else if (nav_zones_->childCount() > 0) {
            selected_zone_item_ = nav_zones_->child(0);
            nav_tree_->setCurrentItem(selected_zone_item_);
        } else {
            selected_zone_item_ = nullptr;
        }
    } else if (nav_zones_->childCount() > 0) {
        selected_zone_item_ = nav_zones_->child(0);
        nav_tree_->setCurrentItem(selected_zone_item_);
    }
    onZoneSelected();

    const int previous_zone = map_zone_->currentData().toInt();
    map_zone_->clear();
    for (const auto& zone : world_.zones) {
        map_zone_->addItem(QString("%1 - %2").arg(zone.num).arg(QString::fromStdString(zone.name)), zone.num);
    }
    if (map_zone_->count() > 0) {
        int index = map_zone_->findData(previous_zone);
        if (index < 0) {
            index = 0;
        }
        map_zone_->setCurrentIndex(index);
    }
}

void MainWindow::openZoneRoomMap(int zone_num) {
    showEditorPage(kStackMap);
    if (nav_map_) {
        nav_tree_->setCurrentItem(nav_map_);
    }
    if (map_tabs_) {
        map_tabs_->setCurrentIndex(0);
    }

    const int zone_index = map_zone_->findData(zone_num);
    if (zone_index >= 0) {
        map_zone_->setCurrentIndex(zone_index);
    }

    refreshZoneMap();
    setStatus(QString("Mappa stanze: zona #%1.").arg(zone_num));
}

void MainWindow::exportMapPng(ZoneMapWidget* view, const QString& suggested_name) {
    if (!view) {
        return;
    }

    QString path = QFileDialog::getSaveFileName(
        this, "Esporta mappa PNG", suggested_name, QStringLiteral("PNG (*.png)"));
    if (path.isEmpty()) {
        return;
    }
    if (!path.endsWith(QStringLiteral(".png"), Qt::CaseInsensitive)) {
        path += QStringLiteral(".png");
    }

    if (!view->exportSceneToPng(path)) {
        QMessageBox::warning(this, "Esporta PNG", "Impossibile esportare la mappa (vuota?).");
        return;
    }
    setStatus(QString("Mappa esportata in %1").arg(path));
}

void MainWindow::exportMapPng(WorldZoneMapWidget* view, const QString& suggested_name) {
    if (!view) {
        return;
    }

    QString path = QFileDialog::getSaveFileName(
        this, "Esporta mappa PNG", suggested_name, QStringLiteral("PNG (*.png)"));
    if (path.isEmpty()) {
        return;
    }
    if (!path.endsWith(QStringLiteral(".png"), Qt::CaseInsensitive)) {
        path += QStringLiteral(".png");
    }

    if (!view->exportSceneToPng(path)) {
        QMessageBox::warning(this, "Esporta PNG", "Impossibile esportare la mappa (vuota?).");
        return;
    }
    setStatus(QString("Mappa esportata in %1").arg(path));
}

void MainWindow::refreshZoneMap() {
    if (!map_view_ || map_zone_->count() == 0) {
        return;
    }

    const int zone_num = map_zone_->currentData().toInt();
    const auto known = workspaceKnownRoomVnums();
    const nebbie::ZoneGraph graph =
        nebbie::build_zone_graph(world_, zone_num, known.empty() ? nullptr : &known);
    if (graph.nodes.empty()) {
        map_view_->clearGraph();
        map_floor_->clear();
        if (map_stats_) {
            map_stats_->setText("Nessuna stanza in questa zona.");
        }
        return;
    }

    map_view_->setGraph(graph);

    const int previous_floor = map_floor_->currentData().toInt();
    map_floor_->clear();
    for (int level : map_view_->availableZLevels()) {
        map_floor_->addItem(QString("Z = %1").arg(level), level);
    }
    int floor_index = map_floor_->findData(previous_floor);
    if (floor_index < 0) {
        floor_index = 0;
    }
    map_floor_->setCurrentIndex(floor_index);
    map_view_->setActiveZLevel(map_floor_->currentData().toInt());
    if (map_broken_only_) {
        map_view_->setShowBrokenOnly(map_broken_only_->isChecked());
    }
    updateMapStats();
}

void MainWindow::refreshWorldZoneMap() {
    if (!world_map_view_) {
        return;
    }

    const auto known = workspaceKnownRoomVnums();
    const nebbie::WorldZoneGraph graph =
        nebbie::build_world_zone_graph(world_, known.empty() ? nullptr : &known);
    if (graph.zones.empty()) {
        world_map_view_->clearGraph();
        if (world_map_details_) {
            world_map_details_->clear();
        }
        if (world_map_stats_) {
            world_map_stats_->setText("Nessuna zona caricata.");
        }
        return;
    }

    world_map_view_->setGraph(graph);
    if (world_map_broken_only_) {
        world_map_view_->setShowBrokenOnly(world_map_broken_only_->isChecked());
    }

    int total_used = 0;
    int total_free = 0;
    int broken_edges = 0;
    for (const auto& zone : graph.zones) {
        total_used += zone.used_count;
        total_free += zone.free_count;
    }
    for (const auto& edge : graph.edges) {
        if (edge.broken_count > 0) {
            ++broken_edges;
        }
    }

    if (world_map_stats_) {
        world_map_stats_->setText(
            QString("Zone: %1 | collegamenti inter-zona: %2 | link rotti: %3 | vnum stanza usati: %4 | vnum liberi: %5")
                .arg(graph.zones.size())
                .arg(graph.edges.size())
                .arg(broken_edges)
                .arg(total_used)
                .arg(total_free));
    }
}

void MainWindow::updateWorldZoneDetails(int zone_num) {
    if (!world_map_details_) {
        return;
    }

    const nebbie::Zone* zone = nebbie::find_zone(world_, zone_num);
    if (!zone) {
        world_map_details_->setPlainText(QString("Zona #%1 non trovata.").arg(zone_num));
        return;
    }

    const nebbie::WorldZoneNode node = nebbie::build_world_zone_node(world_, *zone);
    QString used_text;
    if (node.used_vnums.empty()) {
        used_text = "(nessuna stanza nel range)";
    } else if (node.used_vnums.size() <= 40) {
        QStringList parts;
        for (long vnum : node.used_vnums) {
            parts << QString::number(vnum);
        }
        used_text = parts.join(", ");
    } else {
        used_text = QString("%1 … %2 (%3 vnum, elenco troncato)")
                        .arg(node.used_vnums.front())
                        .arg(node.used_vnums.back())
                        .arg(node.used_count);
    }

    const QString details =
        QString("N. area: %1\nNome: %2\nStanze: %3-%4\n\nStanze usate (%5):\n%6\n\nVnum liberi (%7):\n%8")
            .arg(node.zone_num)
            .arg(QString::fromStdString(node.name))
            .arg(node.bottom)
            .arg(node.top)
            .arg(node.used_count)
            .arg(used_text)
            .arg(node.free_count)
            .arg(QString::fromStdString(nebbie::format_vnum_ranges(node.free_ranges, 20)));
    world_map_details_->setPlainText(details);
    if (world_map_view_) {
        world_map_view_->setHighlightedZone(zone_num);
    }
}

void MainWindow::updateMapStats() {
    if (!map_stats_ || map_zone_->count() == 0) {
        return;
    }

    const int zone_num = map_zone_->currentData().toInt();
    const auto known = workspaceKnownRoomVnums();
    const nebbie::ZoneGraph graph =
        nebbie::build_zone_graph(world_, zone_num, known.empty() ? nullptr : &known);
    if (graph.nodes.empty()) {
        return;
    }

    const nebbie::ZoneZLayout z_layout = nebbie::compute_zone_z_levels(graph);
    const int active_z = map_view_ ? map_view_->activeZLevel() : 0;

    std::size_t on_floor = 0;
    std::size_t broken = 0;
    std::size_t vertical = 0;
    for (const auto& node : graph.nodes) {
        const auto it = z_layout.levels.find(node.vnum);
        if (it != z_layout.levels.end() && it->second == active_z) {
            ++on_floor;
        }
    }
    for (const auto& edge : graph.edges) {
        if (edge.broken) {
            ++broken;
        }
        if (edge.direction >= 4) {
            ++vertical;
        }
    }

    map_stats_->setText(QString("Zona %1 %2 [%3-%4] — piano Z=%5: %6 stanze | totali: %7 | archi: %8 | su/giù: %9 | rotti: %10")
                            .arg(graph.zone_num)
                            .arg(QString::fromStdString(graph.zone_name))
                            .arg(graph.bottom)
                            .arg(graph.top)
                            .arg(active_z)
                            .arg(on_floor)
                            .arg(graph.nodes.size())
                            .arg(graph.edges.size())
                            .arg(vertical)
                            .arg(broken));
}

void MainWindow::onZoneSelected() {
    const int zone_num = currentZoneNum();
    if (zone_num <= 0) {
        return;
    }
    const nebbie::Zone* zone = nebbie::find_zone(world_, zone_num);
    if (!zone) {
        return;
    }
    zone_editor_->loadFromZone(*zone);
}

void MainWindow::applyZoneChanges() {
    const int zone_num = currentZoneNum();
    if (zone_num <= 0) {
        QMessageBox::information(this, "Zone", "Select a zone.");
        return;
    }
    nebbie::Zone* zone = nebbie::find_zone(world_, zone_num);
    if (!zone) {
        QMessageBox::warning(this, "Zone", "Zone not found.");
        return;
    }

    nebbie::Zone updated = *zone;
    zone_editor_->saveZoneInfoTo(updated);
    nebbie::assign_zone_fields(*zone, updated);
    nebbie::recompute_zone_bottoms(world_);

    if (QTreeWidgetItem* item = findEntityTreeItem(nav_zones_, zone_num)) {
        item->setText(0,
                      QString("#%1 %2 [%3-%4]")
                          .arg(zone->num)
                          .arg(QString::fromStdString(zone->name))
                          .arg(zone->bottom)
                          .arg(zone->top));
        selected_zone_item_ = item;
    }
    zone_editor_->loadFromZone(*zone);
    markDirty();
    dirty_zone_nums_.insert(zone_num);
    setStatus(QString("Zone %1 updated in memory.").arg(zone_num));
}

int MainWindow::currentZoneNum() const {
    if (selected_zone_item_) {
        return static_cast<int>(selected_zone_item_->data(0, Qt::UserRole).toLongLong());
    }
    if (nav_tree_) {
        if (QTreeWidgetItem* item = nav_tree_->currentItem()) {
            if (navKindOf(item) == NavKind::Zone) {
                return static_cast<int>(item->data(0, Qt::UserRole).toLongLong());
            }
        }
    }
    return 0;
}

void MainWindow::selectZoneByNum(const int zone_num) {
    if (QTreeWidgetItem* item = findEntityTreeItem(nav_zones_, zone_num)) {
        selected_zone_item_ = item;
        showEditorPage(kStackZones);
        nav_tree_->setCurrentItem(item);
        onZoneSelected();
    }
}

int MainWindow::preferredZoneNum() const {
    if (const int zone_num = currentZoneNum(); zone_num > 0) {
        return zone_num;
    }
    if (selected_world_zone_ > 0) {
        return selected_world_zone_;
    }
    const long room_vnum = currentRoomVnum();
    if (room_vnum > 0) {
        const nebbie::Room* room = world_.find_room(room_vnum);
        if (room && room->zone_index >= 0
            && room->zone_index < static_cast<int>(world_.zones.size())) {
            return world_.zones[static_cast<std::size_t>(room->zone_index)].num;
        }
    }
    if (selected_mob_item_) {
        const long mob_vnum = selected_mob_item_->data(0, Qt::UserRole).toLongLong();
        if (const nebbie::Zone* zone = world_.zone_for_vnum(mob_vnum)) {
            return zone->num;
        }
    }
    if (selected_obj_item_) {
        const long obj_vnum = selected_obj_item_->data(0, Qt::UserRole).toLongLong();
        if (const nebbie::Zone* zone = world_.zone_for_vnum(obj_vnum)) {
            return zone->num;
        }
    }
    if (world_.zones.size() == 1) {
        return world_.zones.front().num;
    }
    return 0;
}

nebbie::WorldIndex MainWindow::worldIndexForValidation() const {
    if (world_index_) {
        return *world_index_;
    }
    nebbie::World copy = world_;
    nebbie::recompute_zone_bottoms(copy);
    return nebbie::build_world_index(copy, "local-validation");
}

long MainWindow::suggestRoomVnum() const {
    const nebbie::WorldIndex index = worldIndexForValidation();
    const int zone_num = preferredZoneNum();
    if (zone_num > 0) {
        const auto suggested = nebbie::suggest_room_vnum_in_zone(index, zone_num);
        if (suggested) {
            return *suggested;
        }
    }
    for (const auto& zone : index.zones) {
        if (zone.zone_num <= 0) {
            continue;
        }
        const auto suggested = nebbie::suggest_room_vnum_in_zone(index, zone.zone_num);
        if (suggested) {
            return *suggested;
        }
    }
    return std::max(1L, nebbie::suggest_next_room_vnum(world_));
}

long MainWindow::suggestMobVnum() const {
    const nebbie::WorldIndex index = worldIndexForValidation();
    const int zone_num = preferredZoneNum();
    if (zone_num > 0) {
        const auto suggested = nebbie::suggest_mob_vnum_in_zone(index, zone_num);
        if (suggested) {
            return *suggested;
        }
    }
    const auto suggested = nebbie::suggest_mob_vnum(index);
    if (suggested) {
        return *suggested;
    }
    return nebbie::suggest_next_mob_vnum(world_);
}

long MainWindow::suggestObjectVnum() const {
    const nebbie::WorldIndex index = worldIndexForValidation();
    const int zone_num = preferredZoneNum();
    if (zone_num > 0) {
        const auto suggested = nebbie::suggest_object_vnum_in_zone(index, zone_num);
        if (suggested) {
            return *suggested;
        }
    }
    const auto suggested = nebbie::suggest_object_vnum(index);
    if (suggested) {
        return *suggested;
    }
    return nebbie::suggest_next_object_vnum(world_);
}

bool MainWindow::warnIfRemoteVnumConflict(const QString& kind, const long vnum) const {
    const nebbie::WorldIndex index = worldIndexForValidation();

    bool taken = false;
    if (kind == QStringLiteral("room")) {
        taken = nebbie::room_vnum_taken(index, vnum);
    } else if (kind == QStringLiteral("mob")) {
        taken = nebbie::mob_vnum_taken(index, vnum);
    } else if (kind == QStringLiteral("object")) {
        taken = nebbie::object_vnum_taken(index, vnum);
    }
    if (!taken) {
        return true;
    }

    const auto answer = QMessageBox::warning(
        const_cast<MainWindow*>(this),
        "World index conflict",
        QString("Vnum %1 is used or reserved in the remote world index.\n"
                "Create it in the local library anyway?")
            .arg(vnum),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    return answer == QMessageBox::Yes;
}

void MainWindow::configureCoordinator() {
    QDialog dialog(this);
    dialog.setWindowTitle("Coordinator configuration");
    auto* form = new QFormLayout(&dialog);

    auto* index_url = new QLineEdit(app_config_.index_url);
    auto* coordinator_url = new QLineEdit(app_config_.coordinator_url);
    auto* coordinator_token = new QLineEdit(app_config_.coordinator_token);
    coordinator_token->setEchoMode(QLineEdit::Password);
    auto* builder_name = new QLineEdit(app_config_.builder_name);

    form->addRow("index_url:", index_url);
    form->addRow("coordinator_url:", coordinator_url);
    form->addRow("coordinator_token:", coordinator_token);
    form->addRow("builder_name:", builder_name);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    app_config_.index_url = index_url->text().trimmed();
    app_config_.coordinator_url = coordinator_url->text().trimmed();
    app_config_.coordinator_token = coordinator_token->text().trimmed();
    app_config_.builder_name = builder_name->text().trimmed();
    nebbie::qt::write_config(app_config_);
    setStatus("Coordinator configuration saved.");
}

void MainWindow::refreshWorldIndex() {
    if (app_config_.index_url.isEmpty()) {
        QMessageBox::information(this, "World index", "Configure index_url first.");
        return;
    }

    const auto result = nebbie::qt::fetch_coordinator_sync(*network_,
                                                           app_config_.index_url,
                                                           app_config_.coordinator_url,
                                                           app_config_.coordinator_token);
    if (!result.ok || !result.index) {
        QMessageBox::warning(this, "World index",
                             result.error.isEmpty() ? "Unable to download world index." : result.error);
        return;
    }

    world_index_ = *result.index;
    setStatus(QString("World index updated: %1 zones, %2 active reservations.")
                  .arg(world_index_->zones.size())
                  .arg(world_index_->reservations.size()));
}

void MainWindow::loadWorldIndexFromFile() {
    const QString path = QFileDialog::getOpenFileName(this, "Load world index", {},
                                                      "JSON files (*.json);;All files (*)");
    if (path.isEmpty()) {
        return;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "World index", "Unable to open file.");
        return;
    }

    const auto parsed = nebbie::world_index_from_json(file.readAll().toStdString());
    if (!parsed) {
        QMessageBox::warning(this, "World index", "Invalid world-index JSON.");
        return;
    }

    world_index_ = *parsed;
    if (!app_config_.coordinator_url.isEmpty() && !app_config_.coordinator_token.isEmpty()) {
        const auto result = nebbie::qt::fetch_coordinator_sync(*network_,
                                                               app_config_.index_url,
                                                               app_config_.coordinator_url,
                                                               app_config_.coordinator_token,
                                                               false);
        if (result.ok) {
            nebbie::merge_reservations(*world_index_, result.reservations);
        }
    }

    setStatus(QString("World index loaded from file: %1 zones.").arg(world_index_->zones.size()));
}

void MainWindow::exportOverlays() {
    if (lib_path_.empty()) {
        QMessageBox::information(this, "Export overlay", "Apri una libreria prima di esportare gli overlay.");
        return;
    }

    try {
        const auto report = nebbie::export_myst_to_overlays(world_, lib_path_, nebbie::OverlayExportKind::all);
        QString message = QString("Overlay esportati in %1\n\n"
                                  "Stanze: %2\nOggetti: %3\nMob: %4\nReset zone: %5")
                              .arg(nebbie::qt::qstring_from_path(lib_path_))
                              .arg(report.rooms)
                              .arg(report.objects)
                              .arg(report.mobiles)
                              .arg(report.zone_resets);
        if (!report.warnings.empty()) {
            message += "\n\nAvvisi:";
            for (const auto& warning : report.warnings) {
                message += "\n• " + QString::fromStdString(warning);
            }
        }
        QMessageBox::information(this, "Export overlay", message);
        setStatus(QString("Overlay esportati: %1 stanze, %2 oggetti, %3 mob, %4 reset zone.")
                      .arg(report.rooms)
                      .arg(report.objects)
                      .arg(report.mobiles)
                      .arg(report.zone_resets));
    } catch (const std::exception& ex) {
        QMessageBox::critical(this, "Export overlay", QString::fromUtf8(ex.what()));
    }
}

void MainWindow::exportLocalWorldIndex() {
    if (lib_path_.empty()) {
        QMessageBox::information(this, "World index", "Open a library first.");
        return;
    }

    const QString path = QFileDialog::getSaveFileName(this, "Export world index", "world-index.json",
                                                      "JSON files (*.json)");
    if (path.isEmpty()) {
        return;
    }

    nebbie::recompute_zone_bottoms(world_);
    const nebbie::WorldIndex index = nebbie::build_world_index(world_, "local-export");
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        QMessageBox::warning(this, "World index", "Unable to write file.");
        return;
    }

    QTextStream out(&file);
    out << QString::fromStdString(nebbie::world_index_to_json(index));
    setStatus(QString("Exported world index with %1 zones (bottom/top from myst.zon).")
                  .arg(index.zones.size()));
}

void MainWindow::reserveVnums() {
    if (app_config_.coordinator_url.isEmpty() || app_config_.coordinator_token.isEmpty()) {
        QMessageBox::information(this, "Reservations",
                                 "Configure coordinator_url and coordinator_token.");
        return;
    }
    if (app_config_.builder_name.isEmpty()) {
        QMessageBox::information(this, "Reservations", "Configure builder_name.");
        return;
    }

    const nebbie::WorldIndex validation_index = worldIndexForValidation();
    if (validation_index.zones.empty()) {
        QMessageBox::information(this, "Reservations",
                                 "No zones available. Open a library or refresh the world index.");
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle("Reserve vnums");
    auto* form = new QFormLayout(&dialog);

    auto* zone_combo = new QComboBox;
    for (const auto& zone : validation_index.zones) {
        zone_combo->addItem(QString("#%1 %2 [bottom %3 — top %4]")
                                .arg(zone.zone_num)
                                .arg(QString::fromStdString(zone.name))
                                .arg(zone.bottom)
                                .arg(zone.top),
                            zone.zone_num);
    }
    auto* zone_range = new QLabel;
    zone_range->setWordWrap(true);
    auto* kind = new QComboBox;
    kind->addItem("Rooms", QStringLiteral("room"));
    kind->addItem("Mobs", QStringLiteral("mob"));
    kind->addItem("Objects", QStringLiteral("object"));
    auto* start_vnum = new QSpinBox;
    start_vnum->setRange(1, 999999);
    auto* end_vnum = new QSpinBox;
    end_vnum->setRange(1, 999999);
    auto* note = new QLineEdit;

    form->addRow(new QLabel("Zone selection is required. start_vnum and end_vnum are mandatory."));
    form->addRow("Zone:", zone_combo);
    form->addRow("Zone range:", zone_range);
    form->addRow("kind:", kind);
    form->addRow("start_vnum:", start_vnum);
    form->addRow("end_vnum:", end_vnum);
    form->addRow("note:", note);

    const int preferred_zone = preferredZoneNum();
    if (preferred_zone > 0) {
        const int zone_index = zone_combo->findData(preferred_zone);
        if (zone_index >= 0) {
            zone_combo->setCurrentIndex(zone_index);
        }
    }

    auto update_zone_range = [&]() {
        const int zone_num = zone_combo->currentData().toInt();
        const nebbie::WorldIndexZone* zone = nebbie::find_world_index_zone(validation_index, zone_num);
        if (!zone) {
            zone_range->setText("-");
            return;
        }
        zone_range->setText(QString("bottom %1 — top %2 | free room vnums: %3")
                                .arg(zone->bottom)
                                .arg(zone->top)
                                .arg(zone->rooms_free_count));
        start_vnum->setRange(zone->bottom, zone->top);
        end_vnum->setRange(zone->bottom, zone->top);
    };

    auto update_suggestion = [&]() {
        const int zone_num = zone_combo->currentData().toInt();
        const QString kind_value = kind->currentData().toString();
        if (kind_value == QStringLiteral("room")) {
            const auto range = nebbie::suggest_room_vnum_range_in_zone(validation_index, zone_num, 10);
            if (range) {
                start_vnum->setValue(static_cast<int>(range->start));
                end_vnum->setValue(static_cast<int>(range->end));
            } else {
                const auto single = nebbie::suggest_room_vnum_in_zone(validation_index, zone_num);
                const int value = single ? static_cast<int>(*single) : start_vnum->minimum();
                start_vnum->setValue(value);
                end_vnum->setValue(value);
            }
        } else if (kind_value == QStringLiteral("mob")) {
            const int value = static_cast<int>(suggestMobVnum());
            start_vnum->setValue(value);
            end_vnum->setValue(value);
        } else {
            const int value = static_cast<int>(suggestObjectVnum());
            start_vnum->setValue(value);
            end_vnum->setValue(value);
        }
        if (end_vnum->value() < start_vnum->value()) {
            end_vnum->setValue(start_vnum->value());
        }
        update_zone_range();
    };

    connect(zone_combo, QOverload<int>::of(&QComboBox::currentIndexChanged), &dialog,
            [&](int) { update_suggestion(); });
    connect(kind, QOverload<int>::of(&QComboBox::currentIndexChanged), &dialog,
            [&](int) { update_suggestion(); });
    connect(start_vnum, QOverload<int>::of(&QSpinBox::valueChanged), &dialog, [&](int value) {
        if (end_vnum->value() < value) {
            end_vnum->setValue(value);
        }
    });
    update_suggestion();

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, [&]() {
        if (zone_combo->currentIndex() < 0) {
            QMessageBox::warning(&dialog, "Reservations", "Select a zone.");
            return;
        }
        if (end_vnum->value() < start_vnum->value()) {
            QMessageBox::warning(&dialog, "Reservations", "end_vnum must be >= start_vnum.");
            return;
        }
        if (kind->currentData().toString() != QStringLiteral("room")
            && (start_vnum->value() <= 0 || end_vnum->value() <= 0)) {
            QMessageBox::warning(&dialog, "Reservations", "start_vnum and end_vnum are required.");
            return;
        }

        nebbie::WorldIndexReservation reservation;
        reservation.builder = app_config_.builder_name.toStdString();
        reservation.zone_num = zone_combo->currentData().toInt();
        reservation.kind = kind->currentData().toString().toStdString();
        reservation.start_vnum = start_vnum->value();
        reservation.end_vnum = end_vnum->value();
        reservation.note = note->text().toStdString();
        reservation.status = "active";

        std::string error;
        if (!nebbie::validate_reservation(validation_index, reservation, &error)) {
            QMessageBox::warning(&dialog, "Reservations", QString::fromStdString(error));
            return;
        }
        dialog.accept();
    });

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    nebbie::WorldIndexReservation reservation;
    reservation.builder = app_config_.builder_name.toStdString();
    reservation.zone_num = zone_combo->currentData().toInt();
    reservation.kind = kind->currentData().toString().toStdString();
    reservation.start_vnum = start_vnum->value();
    reservation.end_vnum = end_vnum->value();
    reservation.note = note->text().toStdString();
    reservation.status = "active";

    QString error;
    if (!nebbie::qt::post_reservation_sync(*network_,
                                           app_config_.coordinator_url,
                                           app_config_.coordinator_token,
                                           reservation,
                                           &error)) {
        QMessageBox::warning(this, "Reservations", error.isEmpty() ? "Request failed." : error);
        return;
    }

    refreshWorldIndex();
    setStatus(QString("Reserved vnums %1-%2 (%3) in zone #%4.")
                  .arg(reservation.start_vnum)
                  .arg(reservation.end_vnum)
                  .arg(QString::fromStdString(reservation.kind))
                  .arg(reservation.zone_num));
}

long MainWindow::currentRoomVnum() const {
    if (selected_room_item_) {
        return static_cast<long>(selected_room_item_->data(0, Qt::UserRole).toLongLong());
    }
    if (nav_tree_) {
        if (QTreeWidgetItem* item = nav_tree_->currentItem()) {
            if (navKindOf(item) == NavKind::Room) {
                return static_cast<long>(item->data(0, Qt::UserRole).toLongLong());
            }
        }
    }
    return 0;
}

void MainWindow::selectRoomByVnum(const long vnum) {
    if (QTreeWidgetItem* item = findEntityTreeItem(nav_rooms_, vnum)) {
        selected_room_item_ = item;
        showEditorPage(kStackRooms);
        nav_tree_->setCurrentItem(item);
        onRoomSelected();
    }
}

void MainWindow::selectMobByVnum(const long vnum) {
    if (QTreeWidgetItem* item = findEntityTreeItem(nav_mobs_, vnum)) {
        selected_mob_item_ = item;
        showEditorPage(kStackMobs);
        nav_tree_->setCurrentItem(item);
        onMobSelected();
    }
}

void MainWindow::selectObjectByVnum(const long vnum) {
    if (QTreeWidgetItem* item = findEntityTreeItem(nav_objects_, vnum)) {
        selected_obj_item_ = item;
        showEditorPage(kStackObjects);
        nav_tree_->setCurrentItem(item);
        onObjSelected();
    }
}

void MainWindow::onEntitySearchChanged(const QString& text) {
    const QString query = text.trimmed();
    QTreeWidgetItem* item = nav_tree_ ? nav_tree_->currentItem() : nullptr;
    NavKind kind = navKindOf(item);
    if (item && item->parent()) {
        const NavKind parent_kind = navKindOf(item->parent());
        if (parent_kind == NavKind::CatRooms || parent_kind == NavKind::CatMobs
            || parent_kind == NavKind::CatObjects || parent_kind == NavKind::CatZones) {
            kind = parent_kind;
        }
    }
    if (kind == NavKind::Room || kind == NavKind::CatRooms) {
        room_filter_ = query;
        refreshRoomList();
    } else if (kind == NavKind::Mob || kind == NavKind::CatMobs) {
        mob_filter_ = query;
        refreshMobList();
    } else if (kind == NavKind::Object || kind == NavKind::CatObjects) {
        object_filter_ = query;
        refreshObjectList();
    } else if (kind == NavKind::Zone || kind == NavKind::CatZones) {
        zone_filter_ = query;
        refreshZoneList();
    }
}

void MainWindow::onEntityCreateClicked() {
    QTreeWidgetItem* item = nav_tree_ ? nav_tree_->currentItem() : nullptr;
    NavKind kind = navKindOf(item);
    if (item && item->parent()) {
        const NavKind parent_kind = navKindOf(item->parent());
        if (parent_kind == NavKind::CatRooms || parent_kind == NavKind::CatMobs
            || parent_kind == NavKind::CatObjects || parent_kind == NavKind::CatZones) {
            kind = parent_kind;
        }
    }
    if (kind == NavKind::Room || kind == NavKind::CatRooms) {
        createRoom();
    } else if (kind == NavKind::Mob || kind == NavKind::CatMobs) {
        createMob();
    } else if (kind == NavKind::Object || kind == NavKind::CatObjects) {
        createObject();
    } else if (kind == NavKind::Zone || kind == NavKind::CatZones) {
        createZone();
    }
}

void MainWindow::onNavTreeSelectionChanged() {
    QTreeWidgetItem* item = nav_tree_ ? nav_tree_->currentItem() : nullptr;
    if (!item) {
        return;
    }
    switch (navKindOf(item)) {
    case NavKind::CatRooms:
        showEditorPage(kStackRooms);
        break;
    case NavKind::Room:
        selected_room_item_ = item;
        showEditorPage(kStackRooms);
        onRoomSelected();
        break;
    case NavKind::RoomExit: {
        if (item->parent()) {
            selected_room_item_ = item->parent();
            showEditorPage(kStackRooms);
            onRoomSelected();
            const int direction = item->data(0, Qt::UserRole + 1).toInt();
            room_editor_->focusExitTab(direction);
        }
        break;
    }
    case NavKind::CatMobs:
        showEditorPage(kStackMobs);
        break;
    case NavKind::Mob:
        selected_mob_item_ = item;
        showEditorPage(kStackMobs);
        onMobSelected();
        break;
    case NavKind::CatObjects:
        showEditorPage(kStackObjects);
        break;
    case NavKind::Object:
        selected_obj_item_ = item;
        showEditorPage(kStackObjects);
        onObjSelected();
        break;
    case NavKind::CatZones:
        showEditorPage(kStackZones);
        break;
    case NavKind::Zone:
        selected_zone_item_ = item;
        showEditorPage(kStackZones);
        onZoneSelected();
        break;
    case NavKind::CatWorld:
    case NavKind::PageShops:
        showEditorPage(kStackShops);
        if (nav_shops_ && navKindOf(item) == NavKind::CatWorld) {
            nav_tree_->setCurrentItem(nav_shops_);
        }
        break;
    case NavKind::PageSpecials:
        showEditorPage(kStackSpecials);
        break;
    case NavKind::PageGlobal:
        showEditorPage(kStackGlobal);
        break;
    case NavKind::PageMap:
        showEditorPage(kStackMap);
        break;
    case NavKind::PageValidation:
        showEditorPage(kStackValidation);
        break;
    default:
        break;
    }
    updateNavCreateAction();
}

void MainWindow::onRoomSelected() {
    const long vnum = currentRoomVnum();
    if (vnum <= 0) {
        return;
    }
    const nebbie::Room* room = world_.find_room(vnum);
    if (!room) {
        return;
    }
    room_editor_->loadFromRoom(*room);
}

void MainWindow::persistRoomEditorState(QTreeWidgetItem* item) {
    if (!item) {
        return;
    }
    const long vnum = static_cast<long>(item->data(0, Qt::UserRole).toLongLong());
    nebbie::Room* room = world_.find_room(vnum);
    if (!room) {
        return;
    }

    nebbie::Room updated = *room;
    room_editor_->saveToRoom(updated);
    if (room_editor_snapshot_matches(*room, updated)) {
        return;
    }

    nebbie::assign_room_fields(*room, updated);
    dirty_room_vnums_.insert(vnum);
    nebbie::qt::refreshRoomTreeItemData(item, vnum, room->name);
    markDirty();
}

void MainWindow::onMobSelected() {
    if (!selected_mob_item_) {
        return;
    }
    const long vnum = static_cast<long>(selected_mob_item_->data(0, Qt::UserRole).toLongLong());
    const nebbie::Mobile* mob = world_.find_mobile(vnum);
    if (!mob) {
        return;
    }
    mob_editor_->loadFromMobile(*mob);
}

void MainWindow::onObjSelected() {
    if (!selected_obj_item_) {
        return;
    }
    const long vnum = static_cast<long>(selected_obj_item_->data(0, Qt::UserRole).toLongLong());
    const nebbie::GameObject* obj = world_.find_object(vnum);
    if (!obj) {
        return;
    }
    obj_editor_->loadFromObject(*obj);
}

void MainWindow::goToExitTarget() {
    const long to_room = room_editor_->selectedExitToRoom();
    if (to_room <= 0) {
        QMessageBox::information(this, "Exits", "Select an exit in the Exits tab.");
        return;
    }
    if (world_.find_room(to_room)) {
        selectRoomByVnum(to_room);
        return;
    }

    if (!aree_workspace_) {
        QMessageBox::warning(this, "Exits",
                             QString("Room #%1 does not exist in this library.").arg(to_room));
        return;
    }

    // Refresh area list ranges if needed, then locate destination by workspace vnum range.
    if (aree_areas_.empty()) {
        refreshAreeAreaList();
    }
    const nebbie::AreeAreaInfo* target_area = findAreeAreaForRoomVnum(to_room);
    if (!target_area) {
        const auto known = workspaceKnownRoomVnums();
        if (known.find(to_room) == known.end()) {
            QMessageBox::warning(this, "Exits", appTr("aree.goto_missing_room", QString::number(to_room)));
            return;
        }
        // Room exists in some .wld but range lookup failed: rescan and retry by folder tops.
        refreshAreeAreaList();
        target_area = findAreeAreaForRoomVnum(to_room);
    }
    if (!target_area) {
        QMessageBox::warning(this, "Exits", appTr("aree.goto_missing_room", QString::number(to_room)));
        return;
    }

    if (aree_area_folder_ && *aree_area_folder_ == target_area->folder_name) {
        QMessageBox::warning(this, "Exits",
                             QString("Room #%1 is in this area's range but not loaded (reload?).").arg(to_room));
        return;
    }

    const auto answer = QMessageBox::question(
        this, appTr("ui.goto_exit"),
        appTr("aree.goto_other_area", QString::number(to_room),
              QString::fromStdString(target_area->folder_name)),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
    if (answer != QMessageBox::Yes) {
        return;
    }

    loadAreeArea(*target_area, false, {}, to_room);
    if (!world_.find_room(to_room)) {
        QMessageBox::warning(this, "Exits", appTr("aree.goto_missing_room", QString::number(to_room)));
    }
}

void MainWindow::applyRoomChanges() {
    QTreeWidgetItem* item = selected_room_item_;
    if (!item) {
        QMessageBox::information(this, "Rooms", "Select a room.");
        return;
    }
    const long vnum = static_cast<long>(item->data(0, Qt::UserRole).toLongLong());

    nebbie::Room* room = world_.find_room(vnum);
    if (!room) {
        QMessageBox::warning(this, "Rooms", "Room not found.");
        return;
    }

    const std::string old_name = room->name;
    nebbie::Room updated = *room;
    room_editor_->saveToRoom(updated);
    if (room_editor_snapshot_matches(*room, updated)) {
        return;
    }
    const bool name_changed = old_name != updated.name;
    nebbie::assign_room_fields(*room, updated);

    std::size_t aligned = 0;
    if (name_changed) {
        aligned = nebbie::refresh_inbound_exit_descriptions(
            world_, vnum, nebbie::InboundExitAlignPolicy::SyncDestinationName, &old_name);
        if (aligned > 0) {
            refreshRoomEditorIfInboundExitsChanged(vnum);
            for (const auto& [other_vnum, other_room] : world_.rooms) {
                for (const auto& exit : other_room.exits) {
                    if (exit.to_room == vnum) {
                        dirty_room_vnums_.insert(other_vnum);
                        break;
                    }
                }
            }
        }
    }

    nebbie::qt::refreshRoomTreeItemData(item, vnum, room->name);
    markDirty();
    dirty_room_vnums_.insert(vnum);
    if (aligned > 0) {
        setStatus(QString("Stanza %1 aggiornata: %2 nome/i destinazione aggiornato/i nelle stanze collegate.")
                      .arg(vnum)
                      .arg(static_cast<qlonglong>(aligned)));
    } else {
        setStatus(QString("Stanza %1 aggiornata in memoria.").arg(vnum));
    }
}

void MainWindow::syncInboundExitLabels() {
    const long vnum = currentRoomVnum();
    if (vnum <= 0) {
        QMessageBox::information(this, "Rooms", "Select a room.");
        return;
    }

    const nebbie::Room* room = world_.find_room(vnum);
    if (!room) {
        QMessageBox::warning(this, "Rooms", "Room not found.");
        return;
    }

    const std::size_t aligned = nebbie::refresh_inbound_exit_descriptions(world_, vnum);
    if (aligned > 0) {
        refreshRoomEditorIfInboundExitsChanged(vnum);
        // Other rooms' exit text changed in memory; without dirty vnums, Aree preserve
        // save would keep the old English blocks on disk.
        for (const auto& [other_vnum, other_room] : world_.rooms) {
            for (const auto& exit : other_room.exits) {
                if (exit.to_room == vnum) {
                    dirty_room_vnums_.insert(other_vnum);
                    break;
                }
            }
        }
        markDirty();
        setStatus(QString("Stanza %1: aggiornate %2 description in altre stanze (uscite verso \"%3\").")
                      .arg(vnum)
                      .arg(static_cast<qlonglong>(aligned))
                      .arg(QString::fromStdString(room->name)));
        return;
    }

    setStatus(QString("Stanza %1: nessuna description da sincronizzare verso \"%2\".")
                  .arg(vnum)
                  .arg(QString::fromStdString(room->name)));
}

void MainWindow::alignCurrentExitLabel() {
    const long vnum = currentRoomVnum();
    if (vnum <= 0) {
        QMessageBox::information(this, "Uscite", "Seleziona una stanza.");
        return;
    }

    const int direction = room_editor_->selectedExitDirection();
    if (direction < 0) {
        QMessageBox::information(this, "Uscite", "Seleziona un'uscita nel tab Exits.");
        return;
    }

    const nebbie::ExitLabelAlignResult result =
        nebbie::align_room_exit_description(world_, vnum, direction);
    if (result.exit_not_found) {
        QMessageBox::warning(this, "Uscite", "Uscita non trovata.");
        return;
    }
    if (result.missing_destination) {
        QMessageBox::warning(this, "Uscite", "Destinazione mancante o non valida.");
        return;
    }
    if (result.already_ok) {
        setStatus("Il nome destinazione corrisponde già al name della stanza di destinazione.");
        return;
    }
    if (!result.updated) {
        return;
    }

    dirty_room_vnums_.insert(vnum);
    markDirty();
    reloadRoomEditor(vnum);
    room_editor_->focusExitTab(direction);

    const nebbie::Room* room = world_.find_room(vnum);
    const nebbie::Exit* exit = room ? nebbie::find_room_exit(*room, direction) : nullptr;
    if (exit) {
        setStatus(QString("Uscita %1: nome destinazione impostato a \"%2\".")
                      .arg(QString::fromUtf8(nebbie::exit_direction_name(direction)))
                      .arg(QString::fromStdString(exit->description)));
    }
}

void MainWindow::showExitAlignmentReport(const QString& text,
                                         const nebbie::ExitAlignmentReport& report) {
    QDialog dialog(this);
    dialog.setWindowTitle("Sincronizzazione description uscite");

    auto* layout = new QVBoxLayout(&dialog);
    auto* summary = new QLabel;
    summary->setWordWrap(true);
    if (report.exits_aligned > 0) {
        summary->setText(QString("Aggiornate %1 description (campo look <dir>) su %2 uscite controllate. "
                                   "Doppio clic su una riga per aprire la stanza sorgente e l'uscita.")
                           .arg(static_cast<qlonglong>(report.exits_aligned))
                           .arg(static_cast<qlonglong>(report.exits_checked)));
    } else {
        summary->setText(QString("Nessun nome destinazione da aggiornare su %1 uscite controllate.")
                           .arg(static_cast<qlonglong>(report.exits_checked)));
    }
    layout->addWidget(summary);

    auto* changes_list = new QListWidget(&dialog);
    changes_list->setMinimumHeight(360);
    for (std::size_t i = 0; i < report.changes.size(); ++i) {
        const auto& change = report.changes[i];
        const QString old_label = QString::fromStdString(change.old_description).trimmed().isEmpty()
                                      ? QStringLiteral("(vuota)")
                                      : QString::fromStdString(change.old_description);
        const QString label = QStringLiteral(
                                  "[#%1 \"%2\"] uscita %3 -> #%4, exit.description: \"%5\" -> \"%6\"")
                                  .arg(change.from_vnum)
                                  .arg(QString::fromStdString(change.from_room_name))
                                  .arg(QString::fromUtf8(nebbie::exit_direction_name(change.direction)))
                                  .arg(change.to_vnum)
                                  .arg(old_label)
                                  .arg(QString::fromStdString(change.new_description));
        auto* item = new QListWidgetItem(label, changes_list);
        item->setData(Qt::UserRole, static_cast<qlonglong>(i));
    }
    if (changes_list->count() == 0) {
        auto* item = new QListWidgetItem(QStringLiteral("Nessuna modifica necessaria."), changes_list);
        item->setFlags(Qt::ItemIsEnabled);
    }
    layout->addWidget(changes_list, 1);

    connect(changes_list, &QListWidget::itemDoubleClicked, &dialog, [this, &dialog, &report](QListWidgetItem* item) {
        if (!item) {
            return;
        }
        const int index = static_cast<int>(item->data(Qt::UserRole).toLongLong());
        if (index < 0 || index >= static_cast<int>(report.changes.size())) {
            return;
        }
        const auto& change = report.changes[static_cast<std::size_t>(index)];
        dialog.accept();
        navigateToRoomExit(change.from_vnum, change.direction);
    });

    auto* log_hint = new QLabel(QString("Log applicazione: %1").arg(nebbie::qt::application_log_path()),
                                &dialog);
    log_hint->setWordWrap(true);
    log_hint->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(log_hint);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok, &dialog);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    layout->addWidget(buttons);

    dialog.resize(820, 560);
    dialog.exec();

    (void)text;
}

void MainWindow::navigateToRoomExit(const long vnum, const int direction) {
    if (!world_.find_room(vnum)) {
        setStatus(QString("Stanza #%1 non trovata nella libreria.").arg(vnum));
        return;
    }
    selectRoomByVnum(vnum);
    room_editor_->focusExitTab(direction);
}

void MainWindow::alignAllInboundExitDescriptions() {
    if (lib_path_.empty()) {
        QMessageBox::information(this, "Stanze", "Apri prima una libreria.");
        return;
    }

    const nebbie::ExitAlignmentReport report = nebbie::align_all_inbound_exit_descriptions(world_);
    const QString context = QStringLiteral("Sincronizzazione description uscite (mondo)");
    const QString text = nebbie::qt::format_exit_alignment_report(
        report, context, nebbie::qt::qstring_from_path(lib_path_));
    nebbie::qt::append_application_log(text);
    showExitAlignmentReport(text, report);

    if (report.exits_aligned > 0) {
        for (const auto& change : report.changes) {
            dirty_room_vnums_.insert(change.from_vnum);
        }
        markDirty();
        const long current = currentRoomVnum();
        if (current > 0) {
            reloadRoomEditor(current);
        }
    }

    setStatus(QString("Sincronizzazione description: %1 aggiornate, %2 già corrette.")
                  .arg(static_cast<qlonglong>(report.exits_aligned))
                  .arg(static_cast<qlonglong>(report.exits_already_ok)));
}

void MainWindow::reloadRoomEditor(long vnum) {
    const nebbie::Room* room = world_.find_room(vnum);
    if (room) {
        room_editor_->loadFromRoom(*room);
    }
}

void MainWindow::refreshRoomEditorIfInboundExitsChanged(long target_vnum) {
    const long current = currentRoomVnum();
    if (current <= 0) {
        return;
    }
    if (current == target_vnum) {
        reloadRoomEditor(current);
        return;
    }

    const nebbie::Room* room = world_.find_room(current);
    if (!room) {
        return;
    }
    for (const auto& exit : room->exits) {
        if (exit.to_room == target_vnum) {
            reloadRoomEditor(current);
            return;
        }
    }
}

void MainWindow::flushPendingEditorChanges() {
    if (selected_room_item_) {
        applyRoomChanges();
    }
    if (selected_mob_item_) {
        applyMobChanges();
    }
    if (selected_obj_item_) {
        applyObjChanges();
    }
}

void MainWindow::applyMobChanges() {
    QTreeWidgetItem* item = selected_mob_item_;
    if (!item) {
        QMessageBox::information(this, "Mob", "Seleziona un mobile.");
        return;
    }
    const long vnum = static_cast<long>(item->data(0, Qt::UserRole).toLongLong());

    nebbie::Mobile* mob = world_.find_mobile(vnum);
    if (!mob) {
        QMessageBox::warning(this, "Mob", "Mobile non trovato.");
        return;
    }

    nebbie::Mobile updated = *mob;
    mob_editor_->saveToMobile(updated);
    nebbie::assign_mobile_fields(*mob, updated);

    item->setText(0, QString("#%1 %2").arg(vnum).arg(QString::fromStdString(mob->short_descr)));
    markDirty();
    dirty_mobile_vnums_.insert(vnum);
    setStatus(QString("Mobile %1 aggiornato in memoria.").arg(vnum));
}

void MainWindow::applyObjChanges() {
    QTreeWidgetItem* item = selected_obj_item_;
    if (!item) {
        QMessageBox::information(this, "Oggetti", "Seleziona un oggetto.");
        return;
    }
    const long vnum = static_cast<long>(item->data(0, Qt::UserRole).toLongLong());

    nebbie::GameObject* obj = world_.find_object(vnum);
    if (!obj) {
        QMessageBox::warning(this, "Oggetti", "Oggetto non trovato.");
        return;
    }

    nebbie::GameObject updated = *obj;
    obj_editor_->saveToObject(updated);
    nebbie::assign_object_fields(*obj, updated);

    item->setText(0, QString("#%1 %2").arg(vnum).arg(QString::fromStdString(obj->short_descr)));
    markDirty();
    dirty_object_vnums_.insert(vnum);
    setStatus(QString("Oggetto %1 aggiornato in memoria.").arg(vnum));
}

void MainWindow::createRoom() {
    if (lib_path_.empty()) {
        QMessageBox::information(this, "Stanze", "Apri prima una libreria.");
        return;
    }

    bool ok = false;
    const int suggested = std::max(1, static_cast<int>(suggestRoomVnum()));
    const int vnum = QInputDialog::getInt(
        this, "Nuova stanza",
        QString("Primo vnum stanza disponibile (modificabile):\nSuggerito: %1").arg(suggested), suggested, 1,
        999999, 1, &ok);
    if (!ok) {
        return;
    }
    if (!warnIfRemoteVnumConflict(QStringLiteral("room"), vnum)) {
        return;
    }

    if (!nebbie::create_room(world_, vnum)) {
        QMessageBox::warning(this, "Stanze", QString("Impossibile creare la stanza #%1 (vnum duplicato?).").arg(vnum));
        return;
    }

    refreshRoomList();
    selectRoomByVnum(vnum);
    markDirty();
    dirty_room_vnums_.insert(vnum);
    setStatus(QString("Creata stanza #%1.").arg(vnum));
}

void MainWindow::refreshWorldDataEditors() {
    shops_editor_->setLibContext(&context_);
    specials_editor_->setLibContext(&context_);
    global_tables_editor_->setLibContext(&context_);
    shops_editor_->setWorld(&world_);
    specials_editor_->setWorld(&world_);
    global_tables_editor_->setWorld(&world_);
}

void MainWindow::createZone() {
    if (lib_path_.empty()) {
        QMessageBox::information(this, "Zone", "Apri prima una libreria.");
        return;
    }

    bool ok = false;
    const int suggested = nebbie::suggest_next_zone_num(world_);
    const int zone_num =
        QInputDialog::getInt(this, "Nuova zona", "Numero zona (maggiore delle zone esistenti):", suggested,
                             suggested, 999999, 1, &ok);
    if (!ok) {
        return;
    }

    if (!nebbie::create_zone(world_, zone_num)) {
        QMessageBox::warning(this, "Zone",
                             QString("Impossibile creare la zona #%1 (numero duplicato o non maggiore delle zone esistenti?).")
                                 .arg(zone_num));
        return;
    }

    refreshZoneList();
    refreshZoneMap();
    refreshWorldZoneMap();
    selectZoneByNum(zone_num);
    markDirty();
    dirty_zone_nums_.insert(zone_num);
    setStatus(QString("Creata zona #%1.").arg(zone_num));
}

void MainWindow::createMob() {
    if (lib_path_.empty()) {
        QMessageBox::information(this, "Mob", "Apri prima una libreria.");
        return;
    }

    bool ok = false;
    const int suggested = static_cast<int>(suggestMobVnum());
    const int vnum = QInputDialog::getInt(
        this, "Nuovo mob",
        QString("Primo vnum mob disponibile (modificabile):\nSuggerito: %1").arg(suggested), suggested, 1, 99999,
        1, &ok);
    if (!ok) {
        return;
    }
    if (!warnIfRemoteVnumConflict(QStringLiteral("mob"), vnum)) {
        return;
    }

    if (!nebbie::create_mob(world_, vnum)) {
        QMessageBox::warning(this, "Mob", QString("Impossibile creare il mob #%1 (vnum duplicato?).").arg(vnum));
        return;
    }

    refreshMobList();
    selectMobByVnum(vnum);
    onMobSelected();
    mob_editor_->focusPrimaryTextField();
    markDirty();
    dirty_mobile_vnums_.insert(static_cast<long>(vnum));
    setStatus(QString("Creato mob #%1.").arg(vnum));
}

void MainWindow::createObject() {
    if (lib_path_.empty()) {
        QMessageBox::information(this, "Oggetti", "Apri prima una libreria.");
        return;
    }

    bool ok = false;
    const int suggested = static_cast<int>(suggestObjectVnum());
    const int vnum = QInputDialog::getInt(
        this, "Nuovo oggetto",
        QString("Primo vnum oggetto disponibile (modificabile):\nSuggerito: %1").arg(suggested), suggested, 1,
        99999, 1, &ok);
    if (!ok) {
        return;
    }
    if (!warnIfRemoteVnumConflict(QStringLiteral("object"), vnum)) {
        return;
    }

    if (!nebbie::create_object(world_, vnum)) {
        QMessageBox::warning(this, "Oggetti", QString("Impossibile creare l'oggetto #%1 (vnum duplicato?).").arg(vnum));
        return;
    }

    refreshObjectList();
    selectObjectByVnum(vnum);
    onObjSelected();
    obj_editor_->focusPrimaryTextField();
    markDirty();
    dirty_object_vnums_.insert(static_cast<long>(vnum));
    setStatus(QString("Creato oggetto #%1.").arg(vnum));
}

void MainWindow::showValidation(const nebbie::ValidationReport& report, const QString& context) {
    validation_issues_ = report.issues;
    validation_list_->clear();

    if (validation_issues_.empty()) {
        validation_list_->addItem(QStringLiteral("Nessun problema rilevato."));
    } else {
        std::map<std::string, std::vector<std::size_t>> indices_by_category;
        for (std::size_t i = 0; i < validation_issues_.size(); ++i) {
            indices_by_category[validation_issues_[i].category].push_back(i);
        }
        std::vector<std::string> categories;
        categories.reserve(indices_by_category.size());
        for (const auto& [category, _] : indices_by_category) {
            categories.push_back(category);
        }
        const auto category_key = [](const std::string& category) {
            static const char* order[] = {"english_text", "room_text", "mob_text", "object_text",
                                          "room",         "mob_combat", "mob_economy", "reset",
                                          "shop",         "guild",      "special",     "social"};
            for (int i = 0; i < static_cast<int>(sizeof(order) / sizeof(order[0])); ++i) {
                if (category == order[i]) {
                    return i;
                }
            }
            return 1000;
        };
        std::sort(categories.begin(), categories.end(),
                  [&](const std::string& a, const std::string& b) {
                      const int ka = category_key(a);
                      const int kb = category_key(b);
                      if (ka != kb) {
                          return ka < kb;
                      }
                      return a < b;
                  });

        auto* summary = new QListWidgetItem(
            QStringLiteral("%1 errori, %2 avvisi — doppio clic sulla riga per aprire l'entita'")
                .arg(report.error_count())
                .arg(report.warning_count()));
        summary->setFlags(Qt::NoItemFlags);
        validation_list_->addItem(summary);

        for (const std::string& category : categories) {
            const auto& indices = indices_by_category[category];
            auto* header = new QListWidgetItem(
                QStringLiteral("— %1 (%2) —")
                    .arg(nebbie::qt::validation_category_title(category))
                    .arg(static_cast<qlonglong>(indices.size())));
            header->setFlags(Qt::NoItemFlags);
            QFont header_font = header->font();
            header_font.setBold(true);
            header->setFont(header_font);
            validation_list_->addItem(header);

            for (std::size_t i : indices) {
                const auto& issue = validation_issues_[i];
                const char* level =
                    issue.severity == nebbie::ValidationSeverity::error ? "ERRORE" : "AVVISO";
                const QString text = QStringLiteral("[%1] %2")
                                         .arg(QString::fromUtf8(level))
                                         .arg(QString::fromStdString(issue.message));
                auto* item = new QListWidgetItem(text, validation_list_);
                item->setData(Qt::UserRole, static_cast<qlonglong>(i));
                if (issue.severity == nebbie::ValidationSeverity::error) {
                    item->setForeground(Qt::red);
                } else {
                    item->setForeground(QColor(180, 120, 0));
                }
            }
        }
    }

    showEditorPage(kStackValidation);
    if (nav_validation_) {
        nav_tree_->setCurrentItem(nav_validation_);
    }

    if (report.issues.empty() && context.isEmpty()) {
        return;
    }

    const QString library = lib_path_.empty() ? QString() : nebbie::qt::qstring_from_path(lib_path_);
    const QString dialog_context = context.isEmpty()
                                       ? QStringLiteral("Report validazione Nebbie Editor")
                                       : context;
    nebbie::qt::show_validation_report_dialog(
        this,
        report,
        QStringLiteral("Validazione"),
        dialog_context,
        library,
        nebbie::qt::editor_validation_log_path());
}

void MainWindow::navigateToIssue(const nebbie::ValidationIssue& issue) {
    switch (issue.target) {
    case nebbie::ValidationTarget::room:
        selectRoomByVnum(issue.target_vnum);
        break;
    case nebbie::ValidationTarget::mob:
        selectMobByVnum(issue.target_vnum);
        break;
    case nebbie::ValidationTarget::object:
        selectObjectByVnum(issue.target_vnum);
        break;
    case nebbie::ValidationTarget::zone:
        selectZoneByNum(issue.zone_num);
        if (issue.reset_index >= 0 && zone_editor_) {
            zone_editor_->selectResetIndex(issue.reset_index);
        }
        break;
    case nebbie::ValidationTarget::shop:
        showEditorPage(kStackShops);
        if (nav_shops_) {
            nav_tree_->setCurrentItem(nav_shops_);
        }
        if (shops_editor_) {
            shops_editor_->selectShop(issue.target_vnum);
        }
        break;
    default:
        break;
    }
}

void MainWindow::onValidationIssueActivated(QListWidgetItem* item) {
    if (!item) {
        return;
    }
    const int index = static_cast<int>(item->data(Qt::UserRole).toLongLong());
    if (index < 0 || index >= static_cast<int>(validation_issues_.size())) {
        return;
    }
    navigateToIssue(validation_issues_[static_cast<std::size_t>(index)]);
}

void MainWindow::validateLib() {
    if (lib_path_.empty()) {
        QMessageBox::information(
            this, appTr("menu.validate_world"),
            QStringLiteral(
                "Apri prima un'area dal workspace Aree (File → Apri workspace Aree) "
                "oppure una lib monolite (File → Apri lib monolite)."));
        return;
    }
    nebbie::ValidationReport report = nebbie::validate_world(world_, validationOptions());
    nebbie::append_monolith_validation(report, lib_path_);
    showValidation(report, QStringLiteral("Report controllo area/lib aperta (Izanagi)"));
    if (report.ok()) {
        if (report.warning_count() > 0) {
            setStatus(QString("Controllo area/lib OK con %1 avvisi.").arg(report.warning_count()));
        } else {
            setStatus("Controllo area/lib OK.");
        }
    } else {
        setStatus(QString("Controllo area/lib: %1 errori.").arg(report.error_count()));
    }
}

std::vector<long> MainWindow::roomsPendingSaveValidation() const {
    if (!dirty_room_vnums_.empty()) {
        return {dirty_room_vnums_.begin(), dirty_room_vnums_.end()};
    }
    return {};
}

void MainWindow::saveLib() {
    if (lib_path_.empty()) {
        QMessageBox::information(this, "Salva", "Apri prima una libreria.");
        return;
    }

    if (!maybeArchiveAreeBeforeFirstSave()) {
        return;
    }

    const std::vector<long> rooms_to_check = roomsPendingSaveValidation();
    if (rooms_to_check.empty()) {
        try {
            flushPendingEditorChanges();
            syncDirtyEntityVnumsToContext();
            nebbie::save_lib_with_backup(world_, context_, lib_path_, {}, sessionStorageRoot());
            markClean();
            last_version_time_ = std::chrono::system_clock::now();
            setStatus("Libreria salvata (backup creato in .nebbie/versions).");
            QMessageBox::information(this, "Salva", "Salvataggio completato. Backup pre-salvataggio creato.");
        } catch (const std::exception& ex) {
            QMessageBox::critical(this, "Errore", QString::fromUtf8(ex.what()));
        }
        return;
    }

    const nebbie::ValidationReport report =
        nebbie::validate_rooms(world_, validationOptions(), &rooms_to_check);
    if (!report.ok() || report.warning_count() > 0) {
        showValidation(report, QStringLiteral("Report validazione stanze modificate (salvataggio)"));
        setStatus(QString("Avvisi salvati in %1").arg(nebbie::qt::editor_validation_log_path()));
        const QString scope = rooms_to_check.size() == 1
                                  ? QString("stanza #%1").arg(rooms_to_check.front())
                                  : QString("%1 stanze modificate").arg(rooms_to_check.size());
        if (!report.ok()) {
            const auto answer = QMessageBox::question(
                this, "Errori di validazione",
                QString("Ci sono %1 errori sulle %2. Salvare comunque?\n\n"
                        "Dettaglio nel dialogo precedente e in:\n%3\n\n"
                        "Per un controllo completo su ciò che hai aperto usa "
                        "Strumenti → Controlla l'area (o lib) aperta...")
                    .arg(report.error_count())
                    .arg(scope)
                    .arg(nebbie::qt::editor_validation_log_path()),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            if (answer != QMessageBox::Yes) {
                return;
            }
        } else {
            const auto answer = QMessageBox::question(
                this, "Avvisi sulle stanze modificate",
                QString("Ci sono %1 avvisi sulle %2. Salvare comunque?\n\n"
                        "Dettaglio nel dialogo precedente e in:\n%3")
                    .arg(report.warning_count())
                    .arg(scope)
                    .arg(nebbie::qt::editor_validation_log_path()),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            if (answer != QMessageBox::Yes) {
                return;
            }
        }
    }

    try {
        flushPendingEditorChanges();
        syncDirtyEntityVnumsToContext();
        nebbie::save_lib_with_backup(world_, context_, lib_path_, {}, sessionStorageRoot());
        markClean();
        last_version_time_ = std::chrono::system_clock::now();
        setStatus("Libreria salvata (backup creato in .nebbie/versions).");
        QMessageBox::information(this, "Salva", "Salvataggio completato. Backup pre-salvataggio creato.");
    } catch (const std::exception& ex) {
        QMessageBox::critical(this, "Errore", QString::fromUtf8(ex.what()));
    }
}

void MainWindow::saveLibForce() {
    if (lib_path_.empty()) {
        QMessageBox::information(this, "Salva", "Apri prima una libreria.");
        return;
    }
    if (!maybeArchiveAreeBeforeFirstSave()) {
        return;
    }

    // Aree preserve-save only rewrites dirty vnums. After the stale-source-block bug,
    // memory can hold a full translation while dirty is empty — force must rewrite all.
    if (!context_.write_eof_markers_on_save) {
        const auto answer = QMessageBox::question(
            this, "Salva forzato (area)",
            QString("Riscrivere TUTTA l'area da quello che vedi in Izanagi?\n\n"
                    "Path:\n%1\n\n"
                    "Usa questo se le stanze in editor sono tradotte ma i file .wld/.mob "
                    "sono ancora in inglese. Viene creato un backup pre-salvataggio.")
                .arg(nebbie::qt::qstring_from_path(lib_path_)),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        if (answer != QMessageBox::Yes) {
            return;
        }
        markAllLoadedEntitiesDirty();
    }

    try {
        flushPendingEditorChanges();
        syncDirtyEntityVnumsToContext();
        nebbie::save_lib_with_backup(world_, context_, lib_path_, {}, sessionStorageRoot());
        markClean();
        last_version_time_ = std::chrono::system_clock::now();
        setStatus("Libreria salvata (forzato, backup creato).");
        if (!context_.write_eof_markers_on_save) {
            QMessageBox::information(
                this, "Salva forzato",
                "Area riscritta da memoria. Riapri il .wld in Pulsar per verificare.");
        }
    } catch (const std::exception& ex) {
        QMessageBox::critical(this, "Errore", QString::fromUtf8(ex.what()));
    }
}

void MainWindow::onAutosaveTick() {
    if (!dirty_ || lib_path_.empty()) {
        return;
    }

    try {
        flushPendingEditorChanges();
        syncDirtyEntityVnumsToContext();
        const auto result =
            nebbie::run_autosave(world_, context_, lib_path_, session_config_, last_version_time_, sessionStorageRoot());
        if (result.version_created) {
            last_version_time_ = std::chrono::system_clock::now();
        }
        const QString time = QDateTime::currentDateTime().toString("HH:mm:ss");
        if (result.version_created) {
            setStatus(QString("Autosalvataggio + versione %1 (%2)")
                          .arg(QString::fromStdString(result.version_id))
                          .arg(time));
        } else {
            setStatus(QString("Autosalvataggio workspace (%1)").arg(time));
        }
    } catch (const std::exception& ex) {
        setStatus(QString("Autosalvataggio fallito: %1").arg(QString::fromUtf8(ex.what())));
    }
}

void MainWindow::restoreFromWorkspace() {
    if (lib_path_.empty()) {
        QMessageBox::information(this, "Cronologia", "Apri prima una libreria.");
        return;
    }

    const auto workspace = nebbie::workspace_dir(sessionStorageRoot());
    std::error_code ec;
    if (!std::filesystem::exists(workspace, ec)) {
        QMessageBox::information(this, "Cronologia", "Nessun autosalvataggio workspace trovato.");
        return;
    }

    const auto answer = QMessageBox::question(
        this, "Ripristina autosalvataggio",
        "Ripristinare l'ultimo autosalvataggio workspace? Le modifiche correnti non salvate andranno perse.",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes) {
        return;
    }

    try {
        nebbie::restore_snapshot(world_, context_, workspace);
        // load_lib resets dirty sets; mark every entity so Aree preserve-save writes
        // the restored texts instead of stale source blocks from the previous open.
        context_.root = lib_path_;
        markAllLoadedEntitiesDirty();
        refreshRoomList();
        refreshMobList();
        refreshObjectList();
        refreshZoneList();
        refreshWorldDataEditors();
        setStatus("Ripristinato autosalvataggio workspace.");
    } catch (const std::exception& ex) {
        QMessageBox::critical(this, "Errore", QString::fromUtf8(ex.what()));
    }
}

void MainWindow::restoreVersion() {
    if (lib_path_.empty()) {
        QMessageBox::information(this, "Cronologia", "Apri prima una libreria.");
        return;
    }

    const auto versions = nebbie::list_versions(sessionStorageRoot());
    if (versions.empty()) {
        QMessageBox::information(this, "Cronologia", "Nessuna versione salvata in .nebbie/versions.");
        return;
    }

    QStringList labels;
    for (const auto& version : versions) {
        labels << QString::fromStdString(version.id + " [" + version.label + "]");
    }

    bool ok = false;
    const QString chosen = QInputDialog::getItem(
        this, "Ripristina versione", "Seleziona versione:", labels, 0, false, &ok);
    if (!ok || chosen.isEmpty()) {
        return;
    }

    const int index = labels.indexOf(chosen);
    if (index < 0) {
        return;
    }

    const auto answer = QMessageBox::question(
        this, "Ripristina versione",
        "Ripristinare la versione selezionata? Le modifiche correnti non salvate andranno perse.",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes) {
        return;
    }

    try {
        const auto snapshot = nebbie::versions_dir(sessionStorageRoot()) / versions[static_cast<std::size_t>(index)].id;
        nebbie::restore_snapshot(world_, context_, snapshot);
        context_.root = lib_path_;
        markAllLoadedEntitiesDirty();
        refreshRoomList();
        refreshMobList();
        refreshObjectList();
        refreshZoneList();
        refreshWorldDataEditors();
        setStatus(QString("Ripristinata versione %1.").arg(chosen));
    } catch (const std::exception& ex) {
        QMessageBox::critical(this, "Errore", QString::fromUtf8(ex.what()));
    }
}

void MainWindow::setStatus(const QString& message) {
    statusBar()->showMessage(message);
}

void MainWindow::syncDirtyEntityVnumsToContext() {
    context_.dirty_room_vnums.clear();
    context_.dirty_mobile_vnums.clear();
    context_.dirty_object_vnums.clear();
    context_.dirty_zone_nums.clear();
    context_.dirty_room_vnums.insert(dirty_room_vnums_.begin(), dirty_room_vnums_.end());
    context_.dirty_mobile_vnums.insert(dirty_mobile_vnums_.begin(), dirty_mobile_vnums_.end());
    context_.dirty_object_vnums.insert(dirty_object_vnums_.begin(), dirty_object_vnums_.end());
    context_.dirty_zone_nums.insert(dirty_zone_nums_.begin(), dirty_zone_nums_.end());
}

void MainWindow::markDirty() {
    dirty_ = true;
    updateBranding();
}

void MainWindow::markAllLoadedEntitiesDirty() {
    dirty_room_vnums_.clear();
    dirty_mobile_vnums_.clear();
    dirty_object_vnums_.clear();
    dirty_zone_nums_.clear();
    for (const auto& [vnum, _] : world_.rooms) {
        dirty_room_vnums_.insert(vnum);
    }
    for (const auto& [vnum, _] : world_.mobiles) {
        dirty_mobile_vnums_.insert(vnum);
    }
    for (const auto& [vnum, _] : world_.objects) {
        dirty_object_vnums_.insert(vnum);
    }
    for (const nebbie::Zone& zone : world_.zones) {
        dirty_zone_nums_.insert(zone.num);
    }
    markDirty();
}

void MainWindow::markClean() {
    dirty_ = false;
    dirty_room_vnums_.clear();
    dirty_mobile_vnums_.clear();
    dirty_object_vnums_.clear();
    dirty_zone_nums_.clear();
    updateBranding();
}

bool MainWindow::confirmSaveIfDirty() {
    if (!dirty_) {
        return true;
    }
    const auto answer = QMessageBox::question(
        this, "Modifiche non salvate",
        "Ci sono modifiche non salvate. Salvare prima di continuare?",
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
    if (answer == QMessageBox::Save) {
        saveLib();
        return !dirty_;
    }
    if (answer == QMessageBox::Discard) {
        return true;
    }
    return false;
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (!confirmSaveIfDirty()) {
        event->ignore();
        return;
    }
    event->accept();
}
