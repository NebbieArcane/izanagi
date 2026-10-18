#include "world_data_editor_widget.hpp"

#include "mud_color_widgets.hpp"
#include "mud_editor_fields.hpp"
#include "nebbie/constants.hpp"
#include "nebbie/edit.hpp"
#include "nebbie/lib_context.hpp"
#include "nebbie/special_proc_catalog.hpp"

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QTabBar>
#include <QTabWidget>
#include <QVBoxLayout>

namespace {

nebbie::qt::MudColorTextEdit* makeSingleLineMudField() {
    auto* field = new nebbie::qt::MudColorTextEdit;
    nebbie::qt::configureMudSingleLineField(field);
    return field;
}

nebbie::qt::MudColorTextEdit* makeMultiLineMudField(const int max_height = 72) {
    auto* field = new nebbie::qt::MudColorTextEdit;
    field->setMaximumHeight(max_height);
    return field;
}

QString textFromEdit(const nebbie::qt::MudColorTextEdit* field) {
    return field ? field->storageText() : QString();
}

void setTextEdit(nebbie::qt::MudColorTextEdit* field, const std::string& value) {
    if (field) {
        field->setStorageText(QString::fromStdString(value));
    }
}

int findComboTextInsensitive(const QComboBox* combo, const QString& text) {
    if (!combo) {
        return -1;
    }
    const std::string needle = text.trimmed().toStdString();
    for (int i = 0; i < combo->count(); ++i) {
        if (nebbie::special_proc_names_equal(combo->itemText(i).toStdString(), needle)) {
            return i;
        }
    }
    return -1;
}

QWidget* wrapScroll(QWidget* content) {
    auto* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setWidget(content);
    return scroll;
}

QLabel* makeWrappedLabel(const QString& text, QWidget* parent = nullptr) {
    auto* label = new QLabel(text, parent);
    label->setWordWrap(true);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    return label;
}

QLabel* makeEmptyLabel(const QString& text) {
    auto* label = makeWrappedLabel(text);
    label->setStyleSheet("color: palette(mid); font-style: italic;");
    label->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    return label;
}

QWidget* makeListColumn(QListWidget* list, QLabel* empty_label, const int list_width = 220) {
    auto* column = new QWidget;
    auto* layout = new QVBoxLayout(column);
    layout->setContentsMargins(0, 0, 0, 0);
    list->setMinimumWidth(list_width);
    layout->addWidget(list, 1);
    layout->addWidget(empty_label);
    return column;
}

QHBoxLayout* makeListEditorRow(QListWidget* list, QLabel* empty_label, QWidget* editor,
                               const int list_width = 220) {
    auto* row = new QHBoxLayout;
    row->addWidget(makeListColumn(list, empty_label, list_width));
    row->addWidget(editor, 1);
    return row;
}

QHBoxLayout* makeSpinRow(QSpinBox* const (&spins)[5]) {
    auto* row = new QHBoxLayout;
    for (int i = 0; i < 5; ++i) {
        row->addWidget(spins[i]);
    }
    row->addStretch();
    return row;
}

QString shopListLabel(const long vnum, const int keeper, const int room) {
    return QString("Negozio #%1 · mob %2 · stanza %3").arg(vnum).arg(keeper).arg(room);
}

QString specialTypeName(const char type) {
    switch (type) {
    case 'm':
    case 'M':
        return QStringLiteral("Mob");
    case 'o':
    case 'O':
        return QStringLiteral("Oggetto");
    case 'r':
    case 'R':
        return QStringLiteral("Stanza");
    default:
        return QString(QChar(type));
    }
}

QString specialListLabel(const char type, const long vnum, const std::string& procedure) {
    return QString("%1 #%2 → %3")
        .arg(specialTypeName(type))
        .arg(vnum)
        .arg(QString::fromStdString(procedure));
}

QString damageListLabel(const int attack_type) {
    return QString("Danno tipo %1").arg(attack_type);
}

QString socialListLabel(const int act_nr) {
    return QString("Social #%1").arg(act_nr);
}

QString poseListLabel(const int level) {
    return QString("Pose livello %1").arg(level);
}

QString guildListLabel(const std::string& base_filename) {
    return base_filename.empty() ? QStringLiteral("(gilda senza nome)")
                                 : QString::fromStdString(base_filename);
}

QString activeFileText(const nebbie::LibContext* context, const bool has_file,
                       const std::filesystem::path& path, const char* default_name) {
    if (!context) {
        return QString::fromLatin1(default_name);
    }
    if (!has_file) {
        return QString("%1 (non presente: verrà creato al salvataggio)").arg(QString::fromLatin1(default_name));
    }
    const std::string name = path.empty() ? std::string(default_name) : path.filename().string();
    return QString::fromStdString(name);
}

} // namespace

WorldDataEditorWidget::WorldDataEditorWidget(const WorldDataScope scope, QWidget* parent)
    : QWidget(parent), scope_(scope) {
    auto* root = new QVBoxLayout(this);
    banner_ = makeWrappedLabel(QString());
    root->addWidget(banner_);
    files_label_ = makeWrappedLabel(QString());
    files_label_->setStyleSheet("color: palette(mid);");
    root->addWidget(files_label_);
    tabs_ = new QTabWidget;
    if (scope_ == WorldDataScope::Shops) {
        buildShopTab(tabs_);
    } else if (scope_ == WorldDataScope::Specials) {
        buildSpecialTab(tabs_);
    } else {
        buildSocialTab(tabs_);
        buildDamageTab(tabs_);
        buildPoseTab(tabs_);
        buildGuildTab(tabs_);
    }
    // Single-page scopes don't need a nested tab bar chrome.
    if (scope_ == WorldDataScope::Shops || scope_ == WorldDataScope::Specials) {
        tabs_->tabBar()->hide();
    }
    root->addWidget(tabs_, 1);
    updateBanner();
}

void WorldDataEditorWidget::setWorld(nebbie::World* world) {
    world_ = world;
    refresh();
}

void WorldDataEditorWidget::setLibContext(const nebbie::LibContext* context) {
    context_ = context;
    updateBanner();
}

void WorldDataEditorWidget::updateBanner() {
    if (!banner_ || !files_label_) {
        return;
    }
    if (scope_ == WorldDataScope::Shops) {
        banner_->setText(
            "File di AREA negozi (.shp): es. castelli.shp nel workspace Aree, oppure myst.shp nel "
            "monolite. In Aree il file non deve terminare con $~ (come i mob senza %%). "
            "Le modifiche si applicano in memoria; usa Salva per scrivere il file.");
        files_label_->setText(
            QString("File attivo: %1")
                .arg(activeFileText(context_, context_ && context_->has_shp,
                                    context_ ? context_->shp_path : std::filesystem::path(),
                                    nebbie::SHOP_FILE)));
    } else if (scope_ == WorldDataScope::Specials) {
        banner_->setText(
            "File di AREA special proc (.spe): assegna procedure del server a mob/oggetti/stanze. "
            "In Aree il file non deve terminare con $ (come i mob senza %%). "
            "Le modifiche si applicano in memoria; usa Salva per scrivere il file.");
        files_label_->setText(
            QString("File attivo: %1")
                .arg(activeFileText(context_, context_ && context_->has_spe,
                                    context_ ? context_->spe_path : std::filesystem::path(),
                                    nebbie::SPECIAL_FILE)));
    } else {
        banner_->setText(
            "Tabelle GLOBALI del server (non di area): myst.act = social, myst.dam = messaggi di danno, "
            "myst.pos = pose, myst.gui = gilde. Valgono per tutto il mondo e vivono solo nel "
            "monolite (lib); non cambiano da un'area all'altra. "
            "Le modifiche si applicano in memoria; usa Salva per scrivere i file.");
        const QString act = activeFileText(context_, context_ && context_->has_act,
                                           context_ ? context_->act_path : std::filesystem::path(),
                                           nebbie::SOCIAL_FILE);
        const QString dam = activeFileText(context_, context_ && context_->has_dam,
                                           context_ ? context_->dam_path : std::filesystem::path(),
                                           nebbie::DAMAGE_FILE);
        const QString pos = activeFileText(context_, context_ && context_->has_pos,
                                           context_ ? context_->pos_path : std::filesystem::path(),
                                           nebbie::POSE_FILE);
        const QString gui = activeFileText(context_, context_ && context_->has_gui,
                                           context_ ? context_->gui_path : std::filesystem::path(),
                                           nebbie::GUILD_FILE);
        files_label_->setText(
            QString("File attivi — Social: %1 · Danni: %2 · Pose: %3 · Gilde: %4").arg(act, dam, pos, gui));
    }
}

void WorldDataEditorWidget::refresh() {
    updateBanner();
    if (scope_ == WorldDataScope::Shops) {
        shop_list_->clear();
    } else if (scope_ == WorldDataScope::Specials) {
        special_list_->clear();
    } else {
        damage_list_->clear();
        social_list_->clear();
        pose_list_->clear();
        guild_list_->clear();
    }
    if (!world_) {
        if (scope_ == WorldDataScope::Shops) {
            shop_empty_->setVisible(true);
        } else if (scope_ == WorldDataScope::Specials) {
            special_empty_->setVisible(true);
        } else {
            damage_empty_->setVisible(true);
            social_empty_->setVisible(true);
            pose_empty_->setVisible(true);
            guild_empty_->setVisible(true);
        }
        return;
    }

    if (scope_ == WorldDataScope::Shops) {
        for (const auto& shop : world_->shops) {
            shop_list_->addItem(shopListLabel(shop.vnum, shop.keeper, shop.in_room));
            shop_list_->item(shop_list_->count() - 1)->setData(Qt::UserRole, static_cast<qlonglong>(shop.vnum));
        }
        shop_empty_->setVisible(shop_list_->count() == 0);
    } else if (scope_ == WorldDataScope::Specials) {
        for (std::size_t i = 0; i < world_->special_procs.size(); ++i) {
            const auto& spe = world_->special_procs[i];
            special_list_->addItem(specialListLabel(spe.type, spe.vnum, spe.procedure));
            special_list_->item(special_list_->count() - 1)->setData(Qt::UserRole, static_cast<qlonglong>(i));
        }
        special_empty_->setVisible(special_list_->count() == 0);
    } else {
        for (std::size_t i = 0; i < world_->damage_messages.size(); ++i) {
            const auto& msg = world_->damage_messages[i];
            damage_list_->addItem(damageListLabel(msg.attack_type));
            damage_list_->item(damage_list_->count() - 1)->setData(Qt::UserRole, static_cast<qlonglong>(i));
        }
        for (std::size_t i = 0; i < world_->social_messages.size(); ++i) {
            const auto& msg = world_->social_messages[i];
            social_list_->addItem(socialListLabel(msg.act_nr));
            social_list_->item(social_list_->count() - 1)->setData(Qt::UserRole, static_cast<qlonglong>(i));
        }
        for (std::size_t i = 0; i < world_->pose_entries.size(); ++i) {
            const auto& pose = world_->pose_entries[i];
            pose_list_->addItem(poseListLabel(pose.level));
            pose_list_->item(pose_list_->count() - 1)->setData(Qt::UserRole, static_cast<qlonglong>(i));
        }
        for (std::size_t i = 0; i < world_->guilds.size(); ++i) {
            const auto& guild = world_->guilds[i];
            guild_list_->addItem(guildListLabel(guild.base_filename));
            guild_list_->item(guild_list_->count() - 1)->setData(Qt::UserRole, static_cast<qlonglong>(i));
        }
        damage_empty_->setVisible(damage_list_->count() == 0);
        social_empty_->setVisible(social_list_->count() == 0);
        pose_empty_->setVisible(pose_list_->count() == 0);
        guild_empty_->setVisible(guild_list_->count() == 0);
    }
}

void WorldDataEditorWidget::selectShop(const long vnum) {
    if (!shop_list_) {
        return;
    }
    for (int row = 0; row < shop_list_->count(); ++row) {
        if (shop_list_->item(row)->data(Qt::UserRole).toLongLong() == vnum) {
            tabs_->setCurrentIndex(0);
            shop_list_->setCurrentRow(row);
            return;
        }
    }
}

void WorldDataEditorWidget::buildShopTab(QWidget* parent) {
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->addWidget(makeWrappedLabel(
        "Cos'è un negozio? È una riga di myst.shp / area.shp che collega tre cose DIVERSE: "
        "il vnum del negozio (solo l'ID della riga nel file), il mob venditore (keeper, vnum di un mob "
        "che deve esistere) e la stanza (room) dove il venditore è presente. "
        "Vnum negozio ≠ vnum mob venditore ≠ vnum stanza."));
    shop_list_ = new QListWidget;
    shop_empty_ = makeEmptyLabel(
        "Nessun negozio in questo file. Usa «Nuovo negozio» (sopra la lista o in basso) per crearne uno.");
    auto* shop_list_header = new QHBoxLayout;
    shop_list_header->addWidget(new QLabel("Negozi nel file"), 1);
    auto* add_top = new QPushButton("Nuovo negozio");
    shop_list_header->addWidget(add_top);
    auto* form_host = new QWidget;
    auto* form_layout = new QVBoxLayout(form_host);

    shop_vnum_ = new QSpinBox;
    shop_vnum_->setRange(0, 999999);
    shop_keeper_ = new QSpinBox;
    shop_keeper_->setRange(0, 999999);
    shop_room_ = new QSpinBox;
    shop_room_->setRange(0, 999999);
    shop_with_who_ = new QSpinBox;
    shop_with_who_->setRange(0, 999999);
    shop_profit_buy_ = new QDoubleSpinBox;
    shop_profit_buy_->setRange(0.0, 100.0);
    shop_profit_buy_->setDecimals(2);
    shop_profit_sell_ = new QDoubleSpinBox;
    shop_profit_sell_->setRange(0.0, 100.0);
    shop_profit_sell_->setDecimals(2);
    for (int i = 0; i < 5; ++i) {
        shop_producing_[i] = new QSpinBox;
        shop_producing_[i]->setRange(0, 999999);
        shop_trade_[i] = new QSpinBox;
        shop_trade_[i]->setRange(0, 999999);
    }
    shop_temper1_ = new QSpinBox;
    shop_temper1_->setRange(-1, 999);
    shop_temper2_ = new QSpinBox;
    shop_temper2_->setRange(-1, 999);
    shop_open1_ = new QSpinBox;
    shop_open1_->setRange(0, 24);
    shop_close1_ = new QSpinBox;
    shop_close1_->setRange(0, 24);
    shop_open2_ = new QSpinBox;
    shop_open2_->setRange(0, 24);
    shop_close2_ = new QSpinBox;
    shop_close2_->setRange(0, 24);
    shop_msg_buy_ = makeSingleLineMudField();
    shop_msg_sell_ = makeSingleLineMudField();
    shop_no_item1_ = makeSingleLineMudField();
    shop_no_item2_ = makeSingleLineMudField();
    shop_no_buy_ = makeSingleLineMudField();
    shop_no_cash1_ = makeSingleLineMudField();
    shop_no_cash2_ = makeSingleLineMudField();

    auto* identity_box = new QGroupBox("Identità: negozio, venditore e stanza");
    auto* identity_form = new QFormLayout(identity_box);
    identity_form->addRow("Vnum negozio (ID riga):", shop_vnum_);
    identity_form->addRow("Mob venditore (keeper):", shop_keeper_);
    identity_form->addRow("Stanza del negozio (room):", shop_room_);
    identity_form->addRow("Clienti esclusi (with_who):", shop_with_who_);
    form_layout->addWidget(identity_box);

    auto* price_box = new QGroupBox("Prezzi");
    auto* price_form = new QFormLayout(price_box);
    price_form->addRow("Moltiplicatore se il cliente compra (profit_buy):", shop_profit_buy_);
    price_form->addRow("Moltiplicatore se il cliente vende (profit_sell):", shop_profit_sell_);
    form_layout->addWidget(price_box);

    auto* goods_box = new QGroupBox("Merce");
    auto* goods_layout = new QVBoxLayout(goods_box);
    goods_layout->addWidget(new QLabel("Oggetti prodotti/venduti (vnum oggetto, 0 = vuoto):"));
    goods_layout->addLayout(makeSpinRow(shop_producing_));
    goods_layout->addWidget(new QLabel("Tipi oggetto che il negozio compra (tipo oggetto, 0 = vuoto):"));
    goods_layout->addLayout(makeSpinRow(shop_trade_));
    form_layout->addWidget(goods_box);

    auto* hours_box = new QGroupBox("Orari e temperamento");
    auto* hours_form = new QFormLayout(hours_box);
    auto* hours1 = new QHBoxLayout;
    hours1->addWidget(shop_open1_);
    hours1->addWidget(new QLabel("→"));
    hours1->addWidget(shop_close1_);
    hours1->addStretch();
    auto* hours2 = new QHBoxLayout;
    hours2->addWidget(shop_open2_);
    hours2->addWidget(new QLabel("→"));
    hours2->addWidget(shop_close2_);
    hours2->addStretch();
    auto* temper = new QHBoxLayout;
    temper->addWidget(shop_temper1_);
    temper->addWidget(shop_temper2_);
    temper->addStretch();
    hours_form->addRow("Fascia 1 (apre → chiude):", hours1);
    hours_form->addRow("Fascia 2 (apre → chiude):", hours2);
    hours_form->addRow("Temperamento (temper1 / temper2):", temper);
    form_layout->addWidget(hours_box);

    auto* msg_box = new QGroupBox("Messaggi del venditore");
    auto* msg_form = new QFormLayout(msg_box);
    msg_form->addRow("Quando il cliente compra:", shop_msg_buy_);
    msg_form->addRow("Quando il cliente vende:", shop_msg_sell_);
    msg_form->addRow("Oggetto non venduto (1):", shop_no_item1_);
    msg_form->addRow("Oggetto non venduto (2):", shop_no_item2_);
    msg_form->addRow("Non compra questo oggetto:", shop_no_buy_);
    msg_form->addRow("Il negozio non ha soldi (1):", shop_no_cash1_);
    msg_form->addRow("Il negozio non ha soldi (2):", shop_no_cash2_);
    form_layout->addWidget(msg_box);
    form_layout->addStretch();

    auto* buttons = new QHBoxLayout;
    auto* add = new QPushButton("Nuovo negozio");
    auto* apply = new QPushButton("Applica negozio");
    auto* remove = new QPushButton("Rimuovi negozio");
    buttons->addWidget(add);
    buttons->addWidget(apply);
    buttons->addWidget(remove);
    buttons->addStretch();
    connect(add_top, &QPushButton::clicked, this, &WorldDataEditorWidget::addShop);
    connect(add, &QPushButton::clicked, this, &WorldDataEditorWidget::addShop);
    connect(apply, &QPushButton::clicked, this, &WorldDataEditorWidget::applyShop);
    connect(remove, &QPushButton::clicked, this, &WorldDataEditorWidget::removeShop);

    auto* list_column = new QWidget;
    auto* list_column_layout = new QVBoxLayout(list_column);
    list_column_layout->setContentsMargins(0, 0, 0, 0);
    list_column_layout->addLayout(shop_list_header);
    list_column_layout->addWidget(shop_list_, 1);
    list_column_layout->addWidget(shop_empty_);
    shop_list_->setMinimumWidth(220);

    auto* row = new QHBoxLayout;
    row->addWidget(list_column);
    row->addWidget(wrapScroll(form_host), 1);
    layout->addLayout(row, 1);
    layout->addLayout(buttons);
    connect(shop_list_, &QListWidget::currentRowChanged, this, [this](int) { onShopSelected(); });
    static_cast<QTabWidget*>(parent)->addTab(page, "Negozi");
}

void WorldDataEditorWidget::buildSpecialTab(QWidget* parent) {
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->addWidget(makeWrappedLabel(
        "Special proc (.spe): assegna una procedura C++ del server a un mob, oggetto o stanza. "
        "Formato riga: M <vnum> <procedura> [parametri]. Salva per scrivere il file."));
    special_list_ = new QListWidget;
    special_empty_ = makeEmptyLabel(
        "Nessuna special proc in questo file. Usa «Nuova special proc» per aggiungerne una.");
    auto* form_host = new QWidget;
    auto* form_group = new QGroupBox("Assegnazione");
    auto* form = new QFormLayout(form_group);
    auto* form_host_layout = new QVBoxLayout(form_host);
    form_host_layout->addWidget(form_group);
    form_host_layout->addStretch();
    special_type_ = new QComboBox;
    special_type_->addItem("m (mobile)", QVariant(QChar('m')));
    special_type_->addItem("o (oggetto)", QVariant(QChar('o')));
    special_type_->addItem("r (stanza)", QVariant(QChar('r')));
    special_vnum_ = new QSpinBox;
    special_vnum_->setRange(0, 999999);
    special_procedure_ = new QComboBox;
    special_procedure_->setEditable(true);
    special_procedure_->setInsertPolicy(QComboBox::NoInsert);
    special_params_ = new QLineEdit;
    form->addRow("Tipo bersaglio:", special_type_);
    form->addRow("Vnum bersaglio:", special_vnum_);
    form->addRow("Procedura:", special_procedure_);
    form->addRow("Parametri:", special_params_);
    auto* buttons = new QHBoxLayout;
    auto* add = new QPushButton("Nuova special proc");
    auto* apply = new QPushButton("Applica special proc");
    auto* remove = new QPushButton("Rimuovi");
    buttons->addWidget(add);
    buttons->addWidget(apply);
    buttons->addWidget(remove);
    buttons->addStretch();
    connect(add, &QPushButton::clicked, this, &WorldDataEditorWidget::addSpecial);
    connect(apply, &QPushButton::clicked, this, &WorldDataEditorWidget::applySpecial);
    connect(remove, &QPushButton::clicked, this, &WorldDataEditorWidget::removeSpecial);
    layout->addLayout(makeListEditorRow(special_list_, special_empty_, wrapScroll(form_host)), 1);
    layout->addLayout(buttons);
    connect(special_list_, &QListWidget::currentRowChanged, this, [this](int) { onSpecialSelected(); });
    connect(special_type_, &QComboBox::currentIndexChanged, this, [this](int) {
        refreshSpecialProcedureChoices();
    });
    refreshSpecialProcedureChoices();
    static_cast<QTabWidget*>(parent)->addTab(page, "Special");
}

void WorldDataEditorWidget::clearSpecialForm() {
    if (!special_type_ || !special_vnum_ || !special_procedure_ || !special_params_) {
        return;
    }
    special_type_->setCurrentIndex(0);
    special_vnum_->setValue(0);
    refreshSpecialProcedureChoices();
    special_procedure_->setEditText(QString());
    special_params_->clear();
}

nebbie::SpecialProc WorldDataEditorWidget::readSpecialForm() const {
    nebbie::SpecialProc spe;
    spe.type = static_cast<char>(special_type_->currentData().toChar().unicode());
    spe.vnum = special_vnum_->value();
    spe.procedure = special_procedure_->currentText().trimmed().toStdString();
    spe.params = special_params_->text().trimmed().toStdString();
    return spe;
}

void WorldDataEditorWidget::selectSpecialIndex(const std::size_t index) {
    if (!special_list_) {
        return;
    }
    for (int row = 0; row < special_list_->count(); ++row) {
        if (static_cast<std::size_t>(special_list_->item(row)->data(Qt::UserRole).toLongLong()) == index) {
            special_list_->setCurrentRow(row);
            return;
        }
    }
}

void WorldDataEditorWidget::refreshSpecialProcedureChoices() {
    if (!special_procedure_ || !special_type_) {
        return;
    }

    const QString current = special_procedure_->currentText();
    const char type = static_cast<char>(special_type_->currentData().toChar().unicode());
    const auto& names = nebbie::special_proc_names_for_type(type);

    special_procedure_->blockSignals(true);
    special_procedure_->clear();
    for (const auto& name : names) {
        special_procedure_->addItem(QString::fromStdString(name));
    }
    if (!current.isEmpty()) {
        const int index = findComboTextInsensitive(special_procedure_, current);
        if (index >= 0) {
            special_procedure_->setCurrentIndex(index);
        } else {
            special_procedure_->setEditText(current);
        }
    }
    special_procedure_->blockSignals(false);
}

void WorldDataEditorWidget::buildDamageTab(QWidget* parent) {
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->addWidget(makeWrappedLabel(
        "Messaggi di combattimento (myst.dam), uno per tipo di attacco. Per ogni esito (morte, mancato, "
        "colpito, dio) ci sono tre testi: per chi attacca, per la vittima e per la stanza."));
    damage_list_ = new QListWidget;
    damage_empty_ = makeEmptyLabel(
        "Nessun messaggio di danno caricato (myst.dam assente o vuoto).");
    auto* form_host = new QWidget;
    auto* host_layout = new QVBoxLayout(form_host);
    damage_attack_type_ = new QSpinBox;
    damage_attack_type_->setRange(0, 999);
    damage_die_attacker_ = makeMultiLineMudField();
    damage_die_victim_ = makeMultiLineMudField();
    damage_die_room_ = makeMultiLineMudField();
    damage_miss_attacker_ = makeMultiLineMudField();
    damage_miss_victim_ = makeMultiLineMudField();
    damage_miss_room_ = makeMultiLineMudField();
    damage_hit_attacker_ = makeMultiLineMudField();
    damage_hit_victim_ = makeMultiLineMudField();
    damage_hit_room_ = makeMultiLineMudField();
    damage_god_attacker_ = makeMultiLineMudField();
    damage_god_victim_ = makeMultiLineMudField();
    damage_god_room_ = makeMultiLineMudField();

    auto* id_box = new QGroupBox("Tipo di attacco");
    auto* id_form = new QFormLayout(id_box);
    id_form->addRow("Numero tipo attacco (attack_type):", damage_attack_type_);
    host_layout->addWidget(id_box);

    const auto add_group = [&](const QString& title, nebbie::qt::MudColorTextEdit* attacker,
                               nebbie::qt::MudColorTextEdit* victim, nebbie::qt::MudColorTextEdit* room) {
        auto* box = new QGroupBox(title);
        auto* form = new QFormLayout(box);
        form->addRow("A chi attacca:", attacker);
        form->addRow("Alla vittima:", victim);
        form->addRow("Alla stanza:", room);
        host_layout->addWidget(box);
    };
    add_group("Morte", damage_die_attacker_, damage_die_victim_, damage_die_room_);
    add_group("Colpo mancato", damage_miss_attacker_, damage_miss_victim_, damage_miss_room_);
    add_group("Colpo a segno", damage_hit_attacker_, damage_hit_victim_, damage_hit_room_);
    add_group("Colpo di un dio", damage_god_attacker_, damage_god_victim_, damage_god_room_);
    host_layout->addStretch();

    auto* apply = new QPushButton("Applica messaggi danno");
    connect(apply, &QPushButton::clicked, this, &WorldDataEditorWidget::applyDamage);
    layout->addLayout(makeListEditorRow(damage_list_, damage_empty_, wrapScroll(form_host)), 1);
    layout->addWidget(apply);
    connect(damage_list_, &QListWidget::currentRowChanged, this, [this](int) { onDamageSelected(); });
    static_cast<QTabWidget*>(parent)->addTab(page, "Danni");
}

void WorldDataEditorWidget::buildSocialTab(QWidget* parent) {
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->addWidget(makeWrappedLabel(
        "Social (myst.act): testi dei comandi emotivi (es. «sorridi»). Ogni social ha testi per "
        "quando non si indica nessuno, quando il bersaglio c'è, non c'è, o è se stessi."));
    social_list_ = new QListWidget;
    social_empty_ = makeEmptyLabel("Nessun social caricato (myst.act assente o vuoto).");
    auto* form_host = new QWidget;
    auto* host_layout = new QVBoxLayout(form_host);
    social_act_nr_ = new QSpinBox;
    social_act_nr_->setRange(0, 9999);
    social_hide_ = new QSpinBox;
    social_hide_->setRange(0, 1);
    social_min_pos_ = new QSpinBox;
    social_min_pos_->setRange(0, 15);
    social_char_no_arg_ = makeMultiLineMudField(64);
    social_others_no_arg_ = makeMultiLineMudField(64);
    social_char_found_ = makeMultiLineMudField(64);
    social_others_found_ = makeMultiLineMudField(64);
    social_vict_found_ = makeMultiLineMudField(64);
    social_not_found_ = makeMultiLineMudField(64);
    social_char_auto_ = makeMultiLineMudField(64);
    social_others_auto_ = makeMultiLineMudField(64);

    auto* id_box = new QGroupBox("Comando");
    auto* id_form = new QFormLayout(id_box);
    id_form->addRow("Numero comando (act_nr):", social_act_nr_);
    id_form->addRow("Nascondi azione (hide, 0/1):", social_hide_);
    id_form->addRow("Posizione minima vittima:", social_min_pos_);
    host_layout->addWidget(id_box);

    auto* no_arg_box = new QGroupBox("Senza bersaglio");
    auto* no_arg_form = new QFormLayout(no_arg_box);
    no_arg_form->addRow("A chi lo esegue:", social_char_no_arg_);
    no_arg_form->addRow("Agli altri nella stanza:", social_others_no_arg_);
    host_layout->addWidget(no_arg_box);

    auto* found_box = new QGroupBox("Con bersaglio presente");
    auto* found_form = new QFormLayout(found_box);
    found_form->addRow("A chi lo esegue:", social_char_found_);
    found_form->addRow("Agli altri nella stanza:", social_others_found_);
    found_form->addRow("Alla vittima:", social_vict_found_);
    host_layout->addWidget(found_box);

    auto* other_box = new QGroupBox("Bersaglio assente o se stessi");
    auto* other_form = new QFormLayout(other_box);
    other_form->addRow("Bersaglio non trovato:", social_not_found_);
    other_form->addRow("Su se stessi — a chi lo esegue:", social_char_auto_);
    other_form->addRow("Su se stessi — agli altri:", social_others_auto_);
    host_layout->addWidget(other_box);
    host_layout->addStretch();

    auto* apply = new QPushButton("Applica social");
    connect(apply, &QPushButton::clicked, this, &WorldDataEditorWidget::applySocial);
    layout->addLayout(makeListEditorRow(social_list_, social_empty_, wrapScroll(form_host)), 1);
    layout->addWidget(apply);
    connect(social_list_, &QListWidget::currentRowChanged, this, [this](int) { onSocialSelected(); });
    static_cast<QTabWidget*>(parent)->addTab(page, "Social");
}

void WorldDataEditorWidget::buildPoseTab(QWidget* parent) {
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->addWidget(makeWrappedLabel(
        "Pose (myst.pos): messaggi del comando «posa», per livello e per classe (0-3). "
        "Ogni classe ha un testo per chi posa e uno per gli altri nella stanza."));
    pose_list_ = new QListWidget;
    pose_empty_ = makeEmptyLabel("Nessuna posa caricata (myst.pos assente o vuoto).");
    auto* form_host = new QWidget;
    auto* host_layout = new QVBoxLayout(form_host);
    pose_level_ = new QSpinBox;
    pose_level_->setRange(0, 60);
    auto* level_box = new QGroupBox("Livello");
    auto* level_form = new QFormLayout(level_box);
    level_form->addRow("Livello minimo:", pose_level_);
    host_layout->addWidget(level_box);
    for (int i = 0; i < 4; ++i) {
        pose_poser_[i] = makeMultiLineMudField(56);
        pose_room_[i] = makeMultiLineMudField(56);
        auto* box = new QGroupBox(QString("Classe %1").arg(i));
        auto* form = new QFormLayout(box);
        form->addRow("A chi posa:", pose_poser_[i]);
        form->addRow("Agli altri nella stanza:", pose_room_[i]);
        host_layout->addWidget(box);
    }
    host_layout->addStretch();
    auto* apply = new QPushButton("Applica pose");
    connect(apply, &QPushButton::clicked, this, &WorldDataEditorWidget::applyPose);
    layout->addLayout(makeListEditorRow(pose_list_, pose_empty_, wrapScroll(form_host)), 1);
    layout->addWidget(apply);
    connect(pose_list_, &QListWidget::currentRowChanged, this, [this](int) { onPoseSelected(); });
    static_cast<QTabWidget*>(parent)->addTab(page, "Pose");
}

void WorldDataEditorWidget::buildGuildTab(QWidget* parent) {
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->addWidget(makeWrappedLabel(
        "Gilde (myst.gui): per ogni gilda il guardiano che blocca un'uscita, i banchieri e il libro "
        "degli iscritti. I vnum sono di mob, stanze e oggetti già esistenti."));
    guild_list_ = new QListWidget;
    guild_empty_ = makeEmptyLabel("Nessuna gilda caricata (myst.gui assente o vuoto).");
    auto* form_host = new QWidget;
    auto* host_layout = new QVBoxLayout(form_host);
    guild_name_ = makeSingleLineMudField();
    guild_guard_mob_ = new QSpinBox;
    guild_guard_mob_->setRange(0, 999999);
    guild_guard_room_ = new QSpinBox;
    guild_guard_room_->setRange(0, 999999);
    guild_guard_dir_ = new QSpinBox;
    guild_guard_dir_->setRange(0, 5);
    guild_banker_mob_ = new QSpinBox;
    guild_banker_mob_->setRange(0, 999999);
    guild_bank_room_ = new QSpinBox;
    guild_bank_room_->setRange(0, 999999);
    guild_banker_xp_mob_ = new QSpinBox;
    guild_banker_xp_mob_->setRange(0, 999999);
    guild_bank_xp_room_ = new QSpinBox;
    guild_bank_xp_room_->setRange(0, 999999);
    guild_member_book_ = new QSpinBox;
    guild_member_book_->setRange(0, 999999);

    auto* name_box = new QGroupBox("Gilda");
    auto* name_form = new QFormLayout(name_box);
    name_form->addRow("Nome file gilda:", guild_name_);
    name_form->addRow("Oggetto libro iscritti (vnum):", guild_member_book_);
    host_layout->addWidget(name_box);

    auto* guard_box = new QGroupBox("Guardiano");
    auto* guard_form = new QFormLayout(guard_box);
    guard_form->addRow("Mob guardiano (vnum):", guild_guard_mob_);
    guard_form->addRow("Stanza del guardiano (vnum):", guild_guard_room_);
    guard_form->addRow("Direzione bloccata (0-5):", guild_guard_dir_);
    host_layout->addWidget(guard_box);

    auto* bank_box = new QGroupBox("Banca e banca XP");
    auto* bank_form = new QFormLayout(bank_box);
    bank_form->addRow("Mob banchiere (vnum):", guild_banker_mob_);
    bank_form->addRow("Stanza banca (vnum):", guild_bank_room_);
    bank_form->addRow("Mob banchiere XP (vnum):", guild_banker_xp_mob_);
    bank_form->addRow("Stanza banca XP (vnum):", guild_bank_xp_room_);
    host_layout->addWidget(bank_box);
    host_layout->addStretch();

    auto* apply = new QPushButton("Applica gilda");
    connect(apply, &QPushButton::clicked, this, &WorldDataEditorWidget::applyGuild);
    layout->addLayout(makeListEditorRow(guild_list_, guild_empty_, wrapScroll(form_host)), 1);
    layout->addWidget(apply);
    connect(guild_list_, &QListWidget::currentRowChanged, this, [this](int) { onGuildSelected(); });
    static_cast<QTabWidget*>(parent)->addTab(page, "Gilde");
}

nebbie::Shop* WorldDataEditorWidget::findShop(const long vnum) {
    if (!world_) {
        return nullptr;
    }
    for (auto& shop : world_->shops) {
        if (shop.vnum == vnum) {
            return &shop;
        }
    }
    return nullptr;
}

void WorldDataEditorWidget::onShopSelected() {
    if (!world_) {
        return;
    }
    auto* item = shop_list_->currentItem();
    if (!item) {
        return;
    }
    const nebbie::Shop* shop = findShop(item->data(Qt::UserRole).toLongLong());
    if (!shop) {
        return;
    }
    shop_vnum_->setValue(static_cast<int>(shop->vnum));
    shop_keeper_->setValue(shop->keeper);
    shop_room_->setValue(shop->in_room);
    shop_with_who_->setValue(shop->with_who);
    shop_profit_buy_->setValue(shop->profit_buy);
    shop_profit_sell_->setValue(shop->profit_sell);
    for (int i = 0; i < 5; ++i) {
        shop_producing_[i]->setValue(shop->producing[i]);
        shop_trade_[i]->setValue(shop->trade_types[i]);
    }
    shop_temper1_->setValue(shop->temper1);
    shop_temper2_->setValue(shop->temper2);
    shop_open1_->setValue(shop->open1);
    shop_close1_->setValue(shop->close1);
    shop_open2_->setValue(shop->open2);
    shop_close2_->setValue(shop->close2);
    shop_msg_buy_->setStorageText(QString::fromStdString(shop->message_buy));
    shop_msg_sell_->setStorageText(QString::fromStdString(shop->message_sell));
    shop_no_item1_->setStorageText(QString::fromStdString(shop->no_such_item1));
    shop_no_item2_->setStorageText(QString::fromStdString(shop->no_such_item2));
    shop_no_buy_->setStorageText(QString::fromStdString(shop->do_not_buy));
    shop_no_cash1_->setStorageText(QString::fromStdString(shop->missing_cash1));
    shop_no_cash2_->setStorageText(QString::fromStdString(shop->missing_cash2));
}

void WorldDataEditorWidget::addShop() {
    if (!world_) {
        QMessageBox::information(this, "Negozi", "Apri prima una libreria.");
        return;
    }

    bool ok = false;
    const int suggested = static_cast<int>(nebbie::suggest_next_shop_vnum(*world_));
    const int vnum = QInputDialog::getInt(
        this, "Nuovo negozio",
        QString("Primo vnum negozio disponibile (ID riga in .shp, non il mob venditore).\n"
                "Suggerito: %1\n"
                "Dopo la creazione imposta mob venditore e stanza nel form.")
            .arg(suggested),
        suggested, 1, 999999, 1, &ok);
    if (!ok) {
        return;
    }

    if (!nebbie::create_shop(*world_, vnum)) {
        QMessageBox::warning(this, "Negozi",
                             QString("Impossibile creare il negozio #%1 (vnum duplicato?).").arg(vnum));
        return;
    }

    refresh();
    selectShop(vnum);
    emit modified();
}

void WorldDataEditorWidget::removeShop() {
    if (!world_) {
        return;
    }
    auto* item = shop_list_->currentItem();
    if (!item) {
        QMessageBox::information(this, "Negozi", "Seleziona un negozio da rimuovere.");
        return;
    }
    const long vnum = item->data(Qt::UserRole).toLongLong();
    const auto answer = QMessageBox::question(
        this, "Rimuovi negozio", QString("Rimuovere il negozio #%1?").arg(vnum), QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (answer != QMessageBox::Yes) {
        return;
    }
    if (!nebbie::remove_shop(*world_, vnum)) {
        QMessageBox::warning(this, "Negozi", "Impossibile rimuovere il negozio selezionato.");
        return;
    }
    refresh();
    emit modified();
}

void WorldDataEditorWidget::applyShop() {
    if (!world_) {
        return;
    }
    auto* item = shop_list_->currentItem();
    if (!item) {
        return;
    }
    const long previous_vnum = item->data(Qt::UserRole).toLongLong();
    nebbie::Shop* shop = findShop(previous_vnum);
    if (!shop) {
        return;
    }
    const long new_vnum = shop_vnum_->value();
    if (new_vnum != previous_vnum) {
        for (const auto& other : world_->shops) {
            if (other.vnum == new_vnum) {
                QMessageBox::warning(this, "Negozi",
                                     QString("Il vnum negozio #%1 è già in uso.").arg(new_vnum));
                return;
            }
        }
    }
    shop->vnum = new_vnum;
    shop->keeper = shop_keeper_->value();
    shop->in_room = shop_room_->value();
    shop->with_who = shop_with_who_->value();
    shop->profit_buy = static_cast<float>(shop_profit_buy_->value());
    shop->profit_sell = static_cast<float>(shop_profit_sell_->value());
    for (int i = 0; i < 5; ++i) {
        shop->producing[i] = shop_producing_[i]->value();
        shop->trade_types[i] = shop_trade_[i]->value();
    }
    shop->temper1 = shop_temper1_->value();
    shop->temper2 = shop_temper2_->value();
    shop->open1 = shop_open1_->value();
    shop->close1 = shop_close1_->value();
    shop->open2 = shop_open2_->value();
    shop->close2 = shop_close2_->value();
    shop->message_buy = shop_msg_buy_->storageText().toStdString();
    shop->message_sell = shop_msg_sell_->storageText().toStdString();
    shop->no_such_item1 = shop_no_item1_->storageText().toStdString();
    shop->no_such_item2 = shop_no_item2_->storageText().toStdString();
    shop->do_not_buy = shop_no_buy_->storageText().toStdString();
    shop->missing_cash1 = shop_no_cash1_->storageText().toStdString();
    shop->missing_cash2 = shop_no_cash2_->storageText().toStdString();
    item->setData(Qt::UserRole, static_cast<qlonglong>(shop->vnum));
    item->setText(shopListLabel(shop->vnum, shop->keeper, shop->in_room));
    emit modified();
}

void WorldDataEditorWidget::onSpecialSelected() {
    if (!world_) {
        return;
    }
    auto* item = special_list_->currentItem();
    if (!item) {
        return;
    }
    const std::size_t index = static_cast<std::size_t>(item->data(Qt::UserRole).toLongLong());
    if (index >= world_->special_procs.size()) {
        return;
    }
    const auto& spe = world_->special_procs[index];
    const int type_index = special_type_->findData(QVariant(QChar(spe.type)));
    special_type_->setCurrentIndex(type_index >= 0 ? type_index : 0);
    special_vnum_->setValue(static_cast<int>(spe.vnum));
    refreshSpecialProcedureChoices();
    const QString procedure = QString::fromStdString(spe.procedure);
    const int proc_index = findComboTextInsensitive(special_procedure_, procedure);
    if (proc_index >= 0) {
        special_procedure_->setCurrentIndex(proc_index);
    } else {
        special_procedure_->setEditText(procedure);
    }
    special_params_->setText(QString::fromStdString(spe.params));
}

void WorldDataEditorWidget::applySpecial() {
    if (!world_) {
        return;
    }
    auto* item = special_list_->currentItem();
    if (!item) {
        QMessageBox::information(this, "Special proc", "Seleziona una special proc da modificare.");
        return;
    }
    const std::size_t index = static_cast<std::size_t>(item->data(Qt::UserRole).toLongLong());
    nebbie::SpecialProc spe = readSpecialForm();
    std::string error;
    if (!nebbie::update_special_proc(*world_, index, spe, &error)) {
        QMessageBox::warning(this, "Special proc", QString::fromStdString(error));
        return;
    }
    if (nebbie::special_proc_exists(*world_, spe.type, spe.vnum, index)) {
        QMessageBox::information(
            this,
            "Special proc",
            "Attenzione: esiste gia' un'altra assegnazione per lo stesso tipo e vnum.");
    }
    refresh();
    selectSpecialIndex(index);
    emit modified();
}

void WorldDataEditorWidget::addSpecial() {
    if (!world_) {
        QMessageBox::information(this, "Special proc", "Apri prima una libreria.");
        return;
    }

    nebbie::SpecialProc spe = readSpecialForm();
    std::string error;
    if (!nebbie::add_special_proc(*world_, spe, &error)) {
        QMessageBox::warning(this, "Special proc", QString::fromStdString(error));
        return;
    }
    const std::size_t index = world_->special_procs.size() - 1;
    if (nebbie::special_proc_exists(*world_, spe.type, spe.vnum, index)) {
        QMessageBox::information(
            this,
            "Special proc",
            "Attenzione: esiste gia' un'altra assegnazione per lo stesso tipo e vnum.");
    }
    refresh();
    selectSpecialIndex(index);
    emit modified();
}

void WorldDataEditorWidget::removeSpecial() {
    if (!world_) {
        return;
    }
    auto* item = special_list_->currentItem();
    if (!item) {
        QMessageBox::information(this, "Special proc", "Seleziona una special proc da rimuovere.");
        return;
    }
    const std::size_t index = static_cast<std::size_t>(item->data(Qt::UserRole).toLongLong());
    const QString label = item->text();
    const auto answer = QMessageBox::question(
        this,
        "Rimuovi special proc",
        QString("Rimuovere la special proc «%1»?").arg(label),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (answer != QMessageBox::Yes) {
        return;
    }
    if (!nebbie::remove_special_proc(*world_, index)) {
        QMessageBox::warning(this, "Special proc", "Impossibile rimuovere la special proc selezionata.");
        return;
    }
    refresh();
    if (special_list_->count() > 0) {
        special_list_->setCurrentRow(std::min<int>(static_cast<int>(index), special_list_->count() - 1));
    } else {
        clearSpecialForm();
    }
    emit modified();
}

void WorldDataEditorWidget::onDamageSelected() {
    if (!world_) {
        return;
    }
    auto* item = damage_list_->currentItem();
    if (!item) {
        return;
    }
    const std::size_t index = static_cast<std::size_t>(item->data(Qt::UserRole).toLongLong());
    if (index >= world_->damage_messages.size()) {
        return;
    }
    const auto& msg = world_->damage_messages[index];
    damage_attack_type_->setValue(msg.attack_type);
    setTextEdit(damage_die_attacker_, msg.die_attacker);
    setTextEdit(damage_die_victim_, msg.die_victim);
    setTextEdit(damage_die_room_, msg.die_room);
    setTextEdit(damage_miss_attacker_, msg.miss_attacker);
    setTextEdit(damage_miss_victim_, msg.miss_victim);
    setTextEdit(damage_miss_room_, msg.miss_room);
    setTextEdit(damage_hit_attacker_, msg.hit_attacker);
    setTextEdit(damage_hit_victim_, msg.hit_victim);
    setTextEdit(damage_hit_room_, msg.hit_room);
    setTextEdit(damage_god_attacker_, msg.god_attacker);
    setTextEdit(damage_god_victim_, msg.god_victim);
    setTextEdit(damage_god_room_, msg.god_room);
}

void WorldDataEditorWidget::applyDamage() {
    if (!world_) {
        return;
    }
    auto* item = damage_list_->currentItem();
    if (!item) {
        return;
    }
    const std::size_t index = static_cast<std::size_t>(item->data(Qt::UserRole).toLongLong());
    if (index >= world_->damage_messages.size()) {
        return;
    }
    auto& msg = world_->damage_messages[index];
    msg.attack_type = damage_attack_type_->value();
    msg.die_attacker = textFromEdit(damage_die_attacker_).toStdString();
    msg.die_victim = textFromEdit(damage_die_victim_).toStdString();
    msg.die_room = textFromEdit(damage_die_room_).toStdString();
    msg.miss_attacker = textFromEdit(damage_miss_attacker_).toStdString();
    msg.miss_victim = textFromEdit(damage_miss_victim_).toStdString();
    msg.miss_room = textFromEdit(damage_miss_room_).toStdString();
    msg.hit_attacker = textFromEdit(damage_hit_attacker_).toStdString();
    msg.hit_victim = textFromEdit(damage_hit_victim_).toStdString();
    msg.hit_room = textFromEdit(damage_hit_room_).toStdString();
    msg.god_attacker = textFromEdit(damage_god_attacker_).toStdString();
    msg.god_victim = textFromEdit(damage_god_victim_).toStdString();
    msg.god_room = textFromEdit(damage_god_room_).toStdString();
    item->setText(damageListLabel(msg.attack_type));
    emit modified();
}

void WorldDataEditorWidget::onSocialSelected() {
    if (!world_) {
        return;
    }
    auto* item = social_list_->currentItem();
    if (!item) {
        return;
    }
    const std::size_t index = static_cast<std::size_t>(item->data(Qt::UserRole).toLongLong());
    if (index >= world_->social_messages.size()) {
        return;
    }
    const auto& msg = world_->social_messages[index];
    social_act_nr_->setValue(msg.act_nr);
    social_hide_->setValue(msg.hide);
    social_min_pos_->setValue(msg.min_victim_position);
    setTextEdit(social_char_no_arg_, msg.char_no_arg);
    setTextEdit(social_others_no_arg_, msg.others_no_arg);
    setTextEdit(social_char_found_, msg.char_found);
    setTextEdit(social_others_found_, msg.others_found);
    setTextEdit(social_vict_found_, msg.vict_found);
    setTextEdit(social_not_found_, msg.not_found);
    setTextEdit(social_char_auto_, msg.char_auto);
    setTextEdit(social_others_auto_, msg.others_auto);
}

void WorldDataEditorWidget::applySocial() {
    if (!world_) {
        return;
    }
    auto* item = social_list_->currentItem();
    if (!item) {
        return;
    }
    const std::size_t index = static_cast<std::size_t>(item->data(Qt::UserRole).toLongLong());
    if (index >= world_->social_messages.size()) {
        return;
    }
    auto& msg = world_->social_messages[index];
    msg.act_nr = social_act_nr_->value();
    msg.hide = social_hide_->value();
    msg.min_victim_position = social_min_pos_->value();
    msg.char_no_arg = textFromEdit(social_char_no_arg_).toStdString();
    msg.others_no_arg = textFromEdit(social_others_no_arg_).toStdString();
    msg.char_found = textFromEdit(social_char_found_).toStdString();
    msg.others_found = textFromEdit(social_others_found_).toStdString();
    msg.vict_found = textFromEdit(social_vict_found_).toStdString();
    msg.not_found = textFromEdit(social_not_found_).toStdString();
    msg.char_auto = textFromEdit(social_char_auto_).toStdString();
    msg.others_auto = textFromEdit(social_others_auto_).toStdString();
    item->setText(socialListLabel(msg.act_nr));
    emit modified();
}

void WorldDataEditorWidget::onPoseSelected() {
    if (!world_) {
        return;
    }
    auto* item = pose_list_->currentItem();
    if (!item) {
        return;
    }
    const std::size_t index = static_cast<std::size_t>(item->data(Qt::UserRole).toLongLong());
    if (index >= world_->pose_entries.size()) {
        return;
    }
    const auto& pose = world_->pose_entries[index];
    pose_level_->setValue(pose.level);
    for (int i = 0; i < 4; ++i) {
        setTextEdit(pose_poser_[i], pose.poser_msg[i]);
        setTextEdit(pose_room_[i], pose.room_msg[i]);
    }
}

void WorldDataEditorWidget::applyPose() {
    if (!world_) {
        return;
    }
    auto* item = pose_list_->currentItem();
    if (!item) {
        return;
    }
    const std::size_t index = static_cast<std::size_t>(item->data(Qt::UserRole).toLongLong());
    if (index >= world_->pose_entries.size()) {
        return;
    }
    auto& pose = world_->pose_entries[index];
    pose.level = pose_level_->value();
    for (int i = 0; i < 4; ++i) {
        pose.poser_msg[i] = textFromEdit(pose_poser_[i]).toStdString();
        pose.room_msg[i] = textFromEdit(pose_room_[i]).toStdString();
    }
    item->setText(poseListLabel(pose.level));
    emit modified();
}

void WorldDataEditorWidget::onGuildSelected() {
    if (!world_) {
        return;
    }
    auto* item = guild_list_->currentItem();
    if (!item) {
        return;
    }
    const std::size_t index = static_cast<std::size_t>(item->data(Qt::UserRole).toLongLong());
    if (index >= world_->guilds.size()) {
        return;
    }
    const auto& guild = world_->guilds[index];
    guild_name_->setStorageText(QString::fromStdString(guild.base_filename));
    guild_guard_mob_->setValue(guild.guard_mob);
    guild_guard_room_->setValue(guild.guard_room);
    guild_guard_dir_->setValue(guild.guard_dir);
    guild_banker_mob_->setValue(guild.banker_mob);
    guild_bank_room_->setValue(guild.bank_room);
    guild_banker_xp_mob_->setValue(guild.banker_xp_mob);
    guild_bank_xp_room_->setValue(guild.bank_xp_room);
    guild_member_book_->setValue(guild.member_book_obj);
}

void WorldDataEditorWidget::applyGuild() {
    if (!world_) {
        return;
    }
    auto* item = guild_list_->currentItem();
    if (!item) {
        return;
    }
    const std::size_t index = static_cast<std::size_t>(item->data(Qt::UserRole).toLongLong());
    if (index >= world_->guilds.size()) {
        return;
    }
    auto& guild = world_->guilds[index];
    guild.base_filename = guild_name_->storageText().toStdString();
    guild.guard_mob = guild_guard_mob_->value();
    guild.guard_room = guild_guard_room_->value();
    guild.guard_dir = guild_guard_dir_->value();
    guild.banker_mob = guild_banker_mob_->value();
    guild.bank_room = guild_bank_room_->value();
    guild.banker_xp_mob = guild_banker_xp_mob_->value();
    guild.bank_xp_room = guild_bank_xp_room_->value();
    guild.member_book_obj = guild_member_book_->value();
    item->setText(guildListLabel(guild.base_filename));
    emit modified();
}

nebbie::qt::MudFieldList WorldDataEditorWidget::mudFields() const {
    nebbie::qt::MudFieldList candidates = {shop_msg_buy_,
                                           shop_msg_sell_,
                                           shop_no_item1_,
                                           shop_no_item2_,
                                           shop_no_buy_,
                                           shop_no_cash1_,
                                           shop_no_cash2_,
                                           damage_die_attacker_,
                                           damage_die_victim_,
                                           damage_die_room_,
                                           damage_miss_attacker_,
                                           damage_miss_victim_,
                                           damage_miss_room_,
                                           damage_hit_attacker_,
                                           damage_hit_victim_,
                                           damage_hit_room_,
                                           damage_god_attacker_,
                                           damage_god_victim_,
                                           damage_god_room_,
                                           social_char_no_arg_,
                                           social_others_no_arg_,
                                           social_char_found_,
                                           social_others_found_,
                                           social_vict_found_,
                                           social_not_found_,
                                           social_char_auto_,
                                           social_others_auto_,
                                           guild_name_};
    for (int i = 0; i < 4; ++i) {
        candidates.push_back(pose_poser_[i]);
        candidates.push_back(pose_room_[i]);
    }
    // Only fields built for this scope exist; skip the others.
    nebbie::qt::MudFieldList fields;
    for (auto* field : candidates) {
        if (field) {
            fields.push_back(field);
        }
    }
    return fields;
}

void WorldDataEditorWidget::applyMudFieldSettings() {
    nebbie::qt::applyMudFieldSettings(mudFields(), max_line_length_, show_color_codes_);
}

void WorldDataEditorWidget::setMaxLineLength(const int max_length) {
    max_line_length_ = max_length < 0 ? 0 : max_length;
    applyMudFieldSettings();
}

void WorldDataEditorWidget::setShowColorCodes(const bool show) {
    show_color_codes_ = show;
    applyMudFieldSettings();
}

nebbie::qt::MudColorTextEdit* WorldDataEditorWidget::focusedMudField() const {
    return nebbie::qt::focusedMudField(mudFields());
}
