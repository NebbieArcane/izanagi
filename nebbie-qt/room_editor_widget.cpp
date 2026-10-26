#include "room_editor_widget.hpp"

#include "flag_group_widget.hpp"
#include <QInputDialog>
#include <QMessageBox>
#include "nebbie/zone_catalog.hpp"
#include "nebbie/edit.hpp"
#include "entity_picker_dialog.hpp"
#include "mud_color_widgets.hpp"
#include "nebbie/mob_catalog.hpp"
#include "nebbie/mud_text.hpp"
#include "nebbie/room_catalog.hpp"
#include "room_text_monitors.hpp"

#include "mud_editor_fields.hpp"
#include "spin_box_wheel_guard.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>

namespace {

void configureLineField(QLineEdit* field) {
    field->setMinimumWidth(420);
    field->setMinimumHeight(30);
    field->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void configureExtraDescriptionField(nebbie::qt::MudColorTextEdit* field) {
    nebbie::qt::configureMudDescriptionField(field);
    field->setMinimumHeight(90);
}

QLabel* makeLegend(const QString& text, QWidget* parent) {
    auto* label = new QLabel(text, parent);
    label->setWordWrap(true);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    return label;
}

void fillCombo(QComboBox* combo, const std::vector<std::pair<int, std::string>>& choices) {
    combo->clear();
    for (const auto& [value, label] : choices) {
        combo->addItem(QString::fromStdString(label), value);
    }
}

int comboIntValue(QComboBox* combo) {
    return combo->currentData().toInt();
}

nebbie::Exit readExitItem(const QListWidgetItem* item) {
    nebbie::Exit exit;
    exit.direction = item->data(Qt::UserRole).toInt();
    exit.to_room = item->data(Qt::UserRole + 1).toLongLong();
    exit.description = item->data(Qt::UserRole + 2).toString().toStdString();
    exit.keyword = item->data(Qt::UserRole + 3).toString().toStdString();
    exit.exit_info = item->data(Qt::UserRole + 4).toLongLong();
    exit.key = item->data(Qt::UserRole + 5).toLongLong();
    exit.open_cmd = item->data(Qt::UserRole + 6).toLongLong();
    return exit;
}

QString formatActiveFlags(const long value, const std::vector<nebbie::MobFlagDef>& defs) {
    QStringList names;
    for (const nebbie::MobFlagDef& def : defs) {
        if ((value & def.value) != 0) {
            names.push_back(QString::fromUtf8(def.label));
        }
    }
    return names.join(", ");
}

void writeExitItem(QListWidgetItem* item, const nebbie::Exit& exit) {
    item->setData(Qt::UserRole, exit.direction);
    item->setData(Qt::UserRole + 1, static_cast<qlonglong>(exit.to_room));
    item->setData(Qt::UserRole + 2, QString::fromStdString(exit.description));
    item->setData(Qt::UserRole + 3, QString::fromStdString(exit.keyword));
    item->setData(Qt::UserRole + 4, static_cast<qlonglong>(exit.exit_info));
    item->setData(Qt::UserRole + 5, static_cast<qlonglong>(exit.key));
    item->setData(Qt::UserRole + 6, static_cast<qlonglong>(exit.open_cmd));

    QString details;
    if (!exit.keyword.empty()) {
        details += QString(" \"%1\"").arg(QString::fromStdString(exit.keyword));
    }
    if (exit.key > 0) {
        details += QString(" key #%1").arg(exit.key);
    }
    const QString exit_flags = formatActiveFlags(exit.exit_info, nebbie::exit_flag_defs());
    if (!exit_flags.isEmpty()) {
        details += QString(" [%1]").arg(exit_flags);
    }
    if (exit.open_cmd >= 0) {
        details += QString(" [%1]")
                       .arg(QString::fromStdString(nebbie::exit_open_cmd_label(static_cast<int>(exit.open_cmd))));
    }

    item->setText(QString("%1 -> #%2%3")
                      .arg(QString::fromStdString(nebbie::exit_direction_label(exit.direction)))
                      .arg(exit.to_room)
                      .arg(details));
}

QFrame* makeSectionSeparator() {
    auto* line = new QFrame;
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    return line;
}

QLineEdit* makeReadonlySummaryField(QWidget* parent) {
    auto* field = new QLineEdit(parent);
    field->setReadOnly(true);
    field->setFocusPolicy(Qt::ClickFocus);
    return field;
}

QLabel* makeSectionTitle(const QString& text, QWidget* parent) {
    auto* label = new QLabel(text, parent);
    QFont font = label->font();
    font.setBold(true);
    label->setFont(font);
    return label;
}

} // namespace

void RoomEditorWidget::setComboIntValue(QComboBox* combo, const int value) const {
    const int index = combo->findData(value);
    combo->setCurrentIndex(index >= 0 ? index : 0);
}

RoomEditorWidget::RoomEditorWidget(QWidget* parent) : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(4, 4, 4, 4);
    root->setSpacing(4);
    auto* tabs = new QTabWidget;
    tabs->setObjectName(QStringLiteral("smaugStyleTabs"));

    // --- Summary (layout mirrors Area Editor IDD_ROOM_SUMMARY) ---
    summary_section_ = new QWidget;
    auto* summary_layout = new QVBoxLayout(summary_section_);
    summary_layout->setContentsMargins(8, 6, 8, 6);
    summary_layout->setSpacing(4);

    auto* header = new QGridLayout;
    header->setHorizontalSpacing(8);
    header->setVerticalSpacing(4);
    summary_vnum_ = makeReadonlySummaryField(summary_section_);
    summary_vnum_->setMaximumWidth(80);
    summary_vnum_->setAlignment(Qt::AlignRight);
    summary_type_ = makeReadonlySummaryField(summary_section_);
    summary_name_ = makeReadonlySummaryField(summary_section_);
    summary_flags_ = makeReadonlySummaryField(summary_section_);
    auto* vnum_label = new QLabel(QStringLiteral("Vnum:"), summary_section_);
    vnum_label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    auto* type_label = new QLabel(QStringLiteral("Type:"), summary_section_);
    type_label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    auto* name_label = new QLabel(QStringLiteral("Name:"), summary_section_);
    name_label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    auto* flags_label = new QLabel(QStringLiteral("Flags:"), summary_section_);
    flags_label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    header->addWidget(vnum_label, 0, 0);
    header->addWidget(summary_vnum_, 0, 1);
    header->addWidget(type_label, 0, 2);
    header->addWidget(summary_type_, 0, 3);
    header->addWidget(name_label, 1, 0);
    header->addWidget(summary_name_, 1, 1, 1, 3);
    header->addWidget(flags_label, 2, 0);
    header->addWidget(summary_flags_, 2, 1, 1, 3);
    header->setColumnStretch(3, 1);
    summary_layout->addLayout(header);

    summary_layout->addWidget(new QLabel(QStringLiteral("Entered from:"), summary_section_));
    auto* entered_row = new QHBoxLayout;
    entered_row->setSpacing(6);
    entered_from_list_ = new QListWidget(summary_section_);
    entered_from_list_->setMinimumHeight(80);
    entered_from_list_->setMaximumHeight(110);
    entered_row->addWidget(entered_from_list_, 1);
    auto* entered_buttons = new QVBoxLayout;
    entered_buttons->setSpacing(4);
    entered_goto_button_ = new QPushButton(QStringLiteral("Go to"), summary_section_);
    add_exit_button_ = new QPushButton(QStringLiteral("Add exit..."), summary_section_);
    entered_goto_button_->setFixedWidth(96);
    add_exit_button_->setFixedWidth(96);
    entered_buttons->addWidget(entered_goto_button_);
    entered_buttons->addWidget(add_exit_button_);
    entered_buttons->addStretch();
    entered_row->addLayout(entered_buttons);
    summary_layout->addLayout(entered_row);

    summary_layout->addWidget(new QLabel(QStringLiteral("References:"), summary_section_));
    auto* refs_row = new QHBoxLayout;
    refs_row->setSpacing(6);
    references_list_ = new QListWidget(summary_section_);
    references_list_->setMinimumHeight(180);
    refs_row->addWidget(references_list_, 1);
    auto* refs_buttons = new QVBoxLayout;
    refs_buttons->setSpacing(4);
    ref_goto_button_ = new QPushButton(QStringLiteral("Go to"), summary_section_);
    add_mob_button_ = new QPushButton(QStringLiteral("Add mob..."), summary_section_);
    add_object_button_ = new QPushButton(QStringLiteral("Add object..."), summary_section_);
    remove_reset_button_ = new QPushButton(QStringLiteral("Remove..."), summary_section_);
    equip_button_ = new QPushButton(QStringLiteral("Equip..."), summary_section_);
    give_button_ = new QPushButton(QStringLiteral("Give..."), summary_section_);
    for (QPushButton* button : {ref_goto_button_, add_mob_button_, add_object_button_, remove_reset_button_,
                                equip_button_, give_button_}) {
        button->setFixedWidth(96);
        refs_buttons->addWidget(button);
    }
    refs_buttons->addStretch();
    refs_row->addLayout(refs_buttons);
    summary_layout->addLayout(refs_row, 1);
    tabs->addTab(summary_section_, QStringLiteral("Summary"));

    // --- Name (SMAUG Name page + Nebbie Teleport/Environment/Extras folded in) ---
    text_section_ = new QWidget;
    auto* text_layout = new QVBoxLayout(text_section_);
    text_layout->setContentsMargins(8, 6, 8, 6);
    text_layout->setSpacing(6);
    text_layout->addWidget(makeLegend(
        "Nome e descrizione (ASCII). Teleport, ambiente e extra descriptions sono sotto. "
        "Descrizione: Invio finale una volta; nome senza Invio finale.",
        text_section_));

    sector_type_ = new QComboBox(text_section_);
    fillCombo(sector_type_, nebbie::room_sector_choices());
    name_ = new nebbie::qt::MudColorTextEdit;
    nebbie::qt::configureMudSingleLineField(name_);
    name_line_info_ = nebbie::qt::makeTextMonitorLabel();
    name_ascii_info_ = nebbie::qt::makeTextMonitorLabel();
    name_english_info_ = nebbie::qt::makeTextMonitorLabel();
    description_ = new nebbie::qt::MudColorTextEdit;
    nebbie::qt::configureMudDescriptionField(description_);
    description_line_info_ = nebbie::qt::makeTextMonitorLabel();
    description_ascii_info_ = nebbie::qt::makeTextMonitorLabel();
    description_english_info_ = nebbie::qt::makeTextMonitorLabel();
    auto* text_form = new QFormLayout;
    text_form->addRow(QStringLiteral("Type:"), sector_type_);
    text_form->addRow(QStringLiteral("Name:"), name_);
    text_form->addRow(QString(), name_line_info_);
    text_form->addRow(QString(), name_ascii_info_);
    text_form->addRow(QString(), name_english_info_);
    text_form->addRow(QStringLiteral("Description:"), description_);
    text_form->addRow(QString(), description_line_info_);
    text_form->addRow(QString(), description_ascii_info_);
    text_form->addRow(QString(), description_english_info_);
    text_layout->addLayout(text_form);
    auto* fix_ascii_btn = new QPushButton(QStringLiteral("Correggi non-ASCII (accenti e apostrofi)"));
    fix_ascii_btn->setToolTip(
        "Sostituisce a'/e'/i'/o'/u' (e maiuscole) e gli apostrofi tipografici con l'apice ASCII '. "
        "Vale per nome, descrizione, extra, uscite e campi luce di questa stanza.");
    text_layout->addWidget(fix_ascii_btn);
    connect(fix_ascii_btn, &QPushButton::clicked, this, [this]() { normalizeMudAsciiAllFields(); });

    text_layout->addWidget(makeSectionSeparator());
    teleport_section_ = new QWidget(text_section_);
    auto* tele_layout = new QVBoxLayout(teleport_section_);
    tele_layout->setContentsMargins(0, 0, 0, 0);
    tele_layout->setSpacing(4);
    tele_layout->addWidget(makeSectionTitle(QStringLiteral("Teleport"), teleport_section_));
    tele_layout->addWidget(makeLegend(
        "Attiva solo per stanze teleport. Nel .wld: sector -1 + tele_time / tele_targ / tele_mask; "
        "con TELE_COUNT anche tele_cnt.",
        teleport_section_));
    teleport_enabled_ = new QCheckBox(QStringLiteral("Stanza teleport (scrive sector -1 nel .wld)"),
                                      teleport_section_);
    teleport_status_ = new QLabel(teleport_section_);
    teleport_status_->setWordWrap(true);
    teleport_status_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    teleport_fields_ = new QWidget(teleport_section_);
    auto* tele_form = new QFormLayout(teleport_fields_);
    tele_time_ = new QSpinBox;
    tele_targ_ = new QSpinBox;
    tele_mask_ = new QSpinBox;
    tele_cnt_ = new QSpinBox;
    for (QSpinBox* spin : {tele_time_, tele_targ_, tele_mask_, tele_cnt_}) {
        spin->setRange(0, 2000000000);
        nebbie::qt::installSpinBoxWheelGuard(spin);
    }
    tele_form->addRow(QStringLiteral("tele_time:"), tele_time_);
    tele_form->addRow(QStringLiteral("tele_targ:"), tele_targ_);
    tele_form->addRow(QStringLiteral("tele_mask:"), tele_mask_);
    tele_form->addRow(QStringLiteral("tele_cnt:"), tele_cnt_);
    tele_layout->addWidget(teleport_enabled_);
    tele_layout->addWidget(teleport_status_);
    tele_layout->addWidget(teleport_fields_);
    text_layout->addWidget(teleport_section_);

    text_layout->addWidget(makeSectionSeparator());
    environment_section_ = new QWidget(text_section_);
    auto* env_layout = new QVBoxLayout(environment_section_);
    env_layout->setContentsMargins(0, 0, 0, 0);
    env_layout->setSpacing(4);
    env_layout->addWidget(makeSectionTitle(QStringLiteral("Environment"), environment_section_));
    env_layout->addWidget(makeLegend(
        "Fiume per settori acqua. moblim con flag TUNNEL. L = stringhe luminosita' opzionali.",
        environment_section_));
    river_panel_ = new QWidget(environment_section_);
    auto* river_form = new QFormLayout(river_panel_);
    river_speed_ = new QSpinBox;
    river_dir_ = new QSpinBox;
    river_speed_->setRange(0, 2000000000);
    river_dir_->setRange(0, 5);
    river_dir_->setToolTip("River direction 0=north .. 5=down (Arcane redit).");
    nebbie::qt::installSpinBoxWheelGuard(river_speed_);
    nebbie::qt::installSpinBoxWheelGuard(river_dir_);
    river_form->addRow(QStringLiteral("River speed:"), river_speed_);
    river_form->addRow(QStringLiteral("River direction (0-5):"), river_dir_);
    moblim_panel_ = new QWidget(environment_section_);
    auto* moblim_form = new QFormLayout(moblim_panel_);
    moblim_ = new QSpinBox;
    moblim_->setRange(1, 2000000000);
    nebbie::qt::installSpinBoxWheelGuard(moblim_);
    moblim_form->addRow(QStringLiteral("moblim (TUNNEL):"), moblim_);
    bright_at_night_ = new nebbie::qt::MudColorTextEdit;
    bright_at_day_ = new nebbie::qt::MudColorTextEdit;
    nebbie::qt::configureMudSingleLineField(bright_at_night_);
    nebbie::qt::configureMudSingleLineField(bright_at_day_);
    auto* bright_form = new QFormLayout;
    bright_form->addRow(QStringLiteral("Bright at night (L):"), bright_at_night_);
    bright_form->addRow(QStringLiteral("Bright at day (L):"), bright_at_day_);
    env_layout->addWidget(river_panel_);
    env_layout->addWidget(moblim_panel_);
    env_layout->addLayout(bright_form);
    text_layout->addWidget(environment_section_);

    text_layout->addWidget(makeSectionSeparator());
    extra_section_ = new QWidget(text_section_);
    auto* extra_layout = new QVBoxLayout(extra_section_);
    extra_layout->setContentsMargins(0, 0, 0, 0);
    extra_layout->setSpacing(4);
    extra_layout->addWidget(makeSectionTitle(QStringLiteral("Extra descriptions"), extra_section_));
    extra_desc_list_ = new QListWidget(extra_section_);
    extra_desc_list_->setMaximumHeight(120);
    extra_desc_keyword_ = new QLineEdit(extra_section_);
    configureLineField(extra_desc_keyword_);
    extra_desc_description_ = new nebbie::qt::MudColorTextEdit;
    configureExtraDescriptionField(extra_desc_description_);
    extra_desc_keyword_english_info_ = nebbie::qt::makeTextMonitorLabel();
    extra_desc_line_info_ = nebbie::qt::makeTextMonitorLabel();
    extra_desc_ascii_info_ = nebbie::qt::makeTextMonitorLabel();
    extra_desc_english_info_ = nebbie::qt::makeTextMonitorLabel();
    auto* extra_desc_form = new QFormLayout;
    extra_desc_form->addRow(QStringLiteral("Keyword:"), extra_desc_keyword_);
    extra_desc_form->addRow(QString(), extra_desc_keyword_english_info_);
    extra_desc_form->addRow(QStringLiteral("Description:"), extra_desc_description_);
    extra_desc_form->addRow(QString(), extra_desc_line_info_);
    extra_desc_form->addRow(QString(), extra_desc_ascii_info_);
    extra_desc_form->addRow(QString(), extra_desc_english_info_);
    auto* extra_desc_buttons = new QHBoxLayout;
    auto* extra_desc_add = new QPushButton(QStringLiteral("Add"));
    auto* extra_desc_update = new QPushButton(QStringLiteral("Update"));
    auto* extra_desc_remove = new QPushButton(QStringLiteral("Remove"));
    extra_desc_buttons->addWidget(extra_desc_add);
    extra_desc_buttons->addWidget(extra_desc_update);
    extra_desc_buttons->addWidget(extra_desc_remove);
    extra_desc_buttons->addStretch();
    extra_layout->addWidget(extra_desc_list_);
    extra_layout->addLayout(extra_desc_form);
    extra_layout->addLayout(extra_desc_buttons);
    text_layout->addWidget(extra_section_);
    text_layout->addStretch(1);

    auto* name_scroll = new QScrollArea;
    name_scroll->setWidgetResizable(true);
    name_scroll->setWidget(text_section_);
    tabs->addTab(name_scroll, QStringLiteral("Name"));

    // --- Flags (dual-list only; sector lives on Name like SMAUG) ---
    sector_section_ = new QWidget;
    auto* sector_layout = new QVBoxLayout(sector_section_);
    sector_layout->setContentsMargins(8, 6, 8, 6);
    sector_layout->addWidget(makeLegend(
        "Flag stanza. TUNNEL abilita moblim (campo in Name / Environment).",
        sector_section_));
    room_flags_ = new FlagGroupWidget(nebbie::room_flag_defs(), sector_section_, 3,
                                      FlagGroupWidget::Mode::DualList);
    sector_layout->addWidget(room_flags_, 1);
    auto* flags_scroll = new QScrollArea;
    flags_scroll->setWidgetResizable(true);
    flags_scroll->setWidget(sector_section_);
    tabs->addTab(flags_scroll, QStringLiteral("Flags"));

    exits_section_ = new QWidget;
    auto* exit_layout = new QVBoxLayout(exits_section_);
    exit_layout->addWidget(makeLegend(
        "Direzioni: 0=nord, 1=est, 2=sud, 3=ovest, 4=su, 5=giù. Description = testo look <dir>; "
        "se vuota il server usa il name della destinazione. Key vnum = oggetto chiave (-1 = nessuna). "
        "Per porte secret/special: scegli il comando in-game (pull/push/…) nel menu open_cmd.",
        exits_section_));
    exit_list_ = new QListWidget;
    exit_list_->setMaximumHeight(140);
    exit_direction_ = new QComboBox;
    fillCombo(exit_direction_, nebbie::exit_direction_choices());
    exit_to_room_ = new QSpinBox;
    exit_to_room_->setRange(0, 999999);
    nebbie::qt::installSpinBoxWheelGuard(exit_to_room_);
    exit_description_ = new nebbie::qt::MudColorTextEdit;
    nebbie::qt::configureMudSingleLineField(exit_description_);
    exit_keyword_ = new QLineEdit;
    configureLineField(exit_keyword_);
    exit_line_info_ = nebbie::qt::makeTextMonitorLabel();
    exit_ascii_info_ = nebbie::qt::makeTextMonitorLabel();
    exit_english_info_ = nebbie::qt::makeTextMonitorLabel();
    exit_keyword_english_info_ = nebbie::qt::makeTextMonitorLabel();
    exit_flags_ = new FlagGroupWidget(nebbie::exit_flag_defs(), exits_section_, 3);
    exit_key_ = new QSpinBox;
    exit_key_->setRange(-1, 999999);
    exit_key_->setValue(-1);
    exit_key_->setToolTip("VNUM dell'oggetto chiave per porte bloccate. Usare -1 se la porta non richiede chiave.");
    nebbie::qt::installSpinBoxWheelGuard(exit_key_);
    exit_open_cmd_ = new QComboBox;
    fillCombo(exit_open_cmd_, nebbie::exit_open_cmd_choices());
    exit_open_cmd_->setToolTip(
        "Comando richiesto per aprire/chiudere questa uscita (campo open_cmd in .wld). "
        "Per secret tipici: pull, push, twist, turn, lift, dig, cut. "
        "open (default) = comportamento normale (open/close).");
    auto* exit_form = new QFormLayout;
    exit_form->addRow("Direction:", exit_direction_);
    exit_form->addRow("To room #:", exit_to_room_);
    exit_form->addRow("Description (look <dir>):", exit_description_);
    exit_form->addRow("", exit_line_info_);
    exit_form->addRow("", exit_ascii_info_);
    exit_form->addRow("", exit_english_info_);
    exit_form->addRow("Keyword (porta):", exit_keyword_);
    exit_form->addRow("", exit_keyword_english_info_);
    exit_form->addRow("Exit flags (exit_info):", exit_flags_);
    exit_form->addRow("Key vnum (oggetto):", exit_key_);
    exit_form->addRow("Comando secret (open_cmd):", exit_open_cmd_);
    auto* exit_buttons = new QHBoxLayout;
    auto* exit_apply = new QPushButton("Add / update exit");
    auto* exit_remove = new QPushButton("Remove exit");
    auto* exit_align_label = new QPushButton("Nome = destinazione");
    exit_align_label->setToolTip(
        "Forza Description al name esatto della stanza di destinazione (azione esplicita). "
        "Non modifica keyword, flag o altri campi dell'uscita.");
    exit_buttons->addWidget(exit_apply);
    exit_buttons->addWidget(exit_remove);
    exit_buttons->addWidget(exit_align_label);
    exit_buttons->addStretch();
    exit_layout->addWidget(exit_list_);
    exit_layout->addLayout(exit_form);
    exit_layout->addLayout(exit_buttons);
    auto* exits_scroll = new QScrollArea;
    exits_scroll->setWidgetResizable(true);
    exits_scroll->setWidget(exits_section_);
    tabs->addTab(exits_scroll, QStringLiteral("Exits"));
    root->addWidget(tabs, 1);
    room_tabs_ = tabs;

    connect(entered_goto_button_, &QPushButton::clicked, this, &RoomEditorWidget::onEnteredFromGoto);
    connect(add_exit_button_, &QPushButton::clicked, this, &RoomEditorWidget::onAddExitFromSummary);
    connect(ref_goto_button_, &QPushButton::clicked, this, &RoomEditorWidget::onReferenceGoto);
    connect(add_mob_button_, &QPushButton::clicked, this, &RoomEditorWidget::onAddMobReset);
    connect(add_object_button_, &QPushButton::clicked, this, &RoomEditorWidget::onAddObjectReset);
    connect(remove_reset_button_, &QPushButton::clicked, this, &RoomEditorWidget::onRemoveReset);
    connect(equip_button_, &QPushButton::clicked, this, &RoomEditorWidget::onEquipMob);
    connect(give_button_, &QPushButton::clicked, this, &RoomEditorWidget::onGiveToMob);
    connect(entered_from_list_, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem*) {
        onEnteredFromGoto();
    });
    connect(references_list_, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem*) {
        onReferenceGoto();
    });

    connect(sector_type_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        updateConditionalFields();
        syncSummaryHeaderFromEditors();
    });
    connect(room_flags_, &FlagGroupWidget::valueChanged, this, [this]() {
        updateConditionalFields();
        syncSummaryHeaderFromEditors();
    });
    connect(name_, &nebbie::qt::MudColorTextEdit::storageTextChanged, this, [this]() {
        if (!loading_) {
            syncSummaryHeaderFromEditors();
        }
    });
    connect(teleport_enabled_, &QCheckBox::toggled, this, [this](bool enabled) {
        if (!enabled && !loading_) {
            const QSignalBlocker b0(tele_time_);
            const QSignalBlocker b1(tele_targ_);
            const QSignalBlocker b2(tele_mask_);
            const QSignalBlocker b3(tele_cnt_);
            tele_time_->setValue(0);
            tele_targ_->setValue(0);
            tele_mask_->setValue(0);
            tele_cnt_->setValue(0);
        }
        updateConditionalFields();
    });
    for (QSpinBox* spin : {tele_time_, tele_targ_, tele_mask_, tele_cnt_}) {
        connect(spin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int) {
            if (!loading_) {
                updateConditionalFields();
            }
        });
    }
    connect(extra_desc_list_, &QListWidget::currentRowChanged, this, [this](int) { onExtraDescSelected(); });
    connect(extra_desc_add, &QPushButton::clicked, this, &RoomEditorWidget::addExtraDesc);
    connect(extra_desc_update, &QPushButton::clicked, this, &RoomEditorWidget::updateExtraDesc);
    connect(extra_desc_remove, &QPushButton::clicked, this, &RoomEditorWidget::removeExtraDesc);
    connect(exit_list_, &QListWidget::currentRowChanged, this, [this](int) { onExitSelected(); });
    connect(exit_apply, &QPushButton::clicked, this, &RoomEditorWidget::addOrUpdateExit);
    connect(exit_remove, &QPushButton::clicked, this, &RoomEditorWidget::removeExit);
    connect(exit_align_label, &QPushButton::clicked, this, &RoomEditorWidget::alignExitLabelRequested);

    hookMudField(name_);
    hookMudField(description_);
    hookMudField(extra_desc_description_);
    hookMudField(exit_description_);
    hookMudField(bright_at_night_);
    hookMudField(bright_at_day_);
    connect(extra_desc_keyword_, &QLineEdit::textChanged, this, [this]() { updateTextMonitors(); });
    connect(exit_keyword_, &QLineEdit::textChanged, this, [this]() { updateTextMonitors(); });

    river_panel_->setVisible(false);
    moblim_panel_->setVisible(false);
    teleport_fields_->setVisible(false);
    updateConditionalFields();
}

RoomEditorWidget::~RoomEditorWidget() {
    disconnect(this);
}

void RoomEditorWidget::scrollToSection(QWidget* section) {
    if (!section) {
        return;
    }
    QWidget* parent = section->parentWidget();
    while (parent) {
        if (auto* scroll = qobject_cast<QScrollArea*>(parent)) {
            scroll->ensureWidgetVisible(section);
            return;
        }
        parent = parent->parentWidget();
    }
}

int RoomEditorWidget::selectedExitDirection() const {
    const auto* item = exit_list_->currentItem();
    if (item) {
        return item->data(Qt::UserRole).toInt();
    }
    if (exit_to_room_->value() > 0) {
        return comboIntValue(exit_direction_);
    }
    return -1;
}

void RoomEditorWidget::focusExitTab(const int direction) {
    if (room_tabs_) {
        for (int i = 0; i < room_tabs_->count(); ++i) {
            if (room_tabs_->tabText(i) == QStringLiteral("Exits")) {
                room_tabs_->setCurrentIndex(i);
                break;
            }
        }
    } else {
        scrollToSection(exits_section_);
    }
    for (int i = 0; i < exit_list_->count(); ++i) {
        auto* item = exit_list_->item(i);
        if (item->data(Qt::UserRole).toInt() == direction) {
            exit_list_->setCurrentItem(item);
            return;
        }
    }
    setComboIntValue(exit_direction_, direction);
}

void RoomEditorWidget::updateConditionalFields() {
    const int sector = comboIntValue(sector_type_);
    river_panel_->setVisible(nebbie::room_sector_uses_river(sector));
    moblim_panel_->setVisible(nebbie::room_flags_use_moblim(room_flags_->value()));

    const bool tele_on = teleport_enabled_->isChecked();
    teleport_fields_->setVisible(tele_on);
    if (!tele_on) {
        teleport_status_->setText(
            QStringLiteral("Teleport disattivo: lo scroll della maschera non altera tele_*. "
                           "Attiva la spunta solo per stanze teleport."));
        teleport_status_->setStyleSheet(QStringLiteral("color: #8a8a8a;"));
        return;
    }
    teleport_status_->setText(
        QStringLiteral("Teleport attivo: tele_time=%1  tele_targ=#%2  tele_mask=%3  tele_cnt=%4")
            .arg(tele_time_->value())
            .arg(tele_targ_->value())
            .arg(tele_mask_->value())
            .arg(tele_cnt_->value()));
    teleport_status_->setStyleSheet(QStringLiteral("color: #c9a227; font-weight: 600;"));
}

void RoomEditorWidget::setWorld(nebbie::World* world) {
    world_ = world;
    refreshSummaryFromWorld();
}

void RoomEditorWidget::refreshSummaryFromWorld() {
    syncSummaryHeaderFromEditors();
    rebuildSummaryLists();
}

void RoomEditorWidget::syncSummaryHeaderFromEditors() {
    if (!summary_vnum_) {
        return;
    }
    summary_vnum_->setText(current_vnum_ > 0 ? QString::number(current_vnum_) : QStringLiteral("-"));
    summary_type_->setText(sector_type_ ? sector_type_->currentText() : QString());
    summary_name_->setText(name_ ? name_->storageText() : QString());
    if (room_flags_) {
        const long flags = room_flags_->value();
        QStringList parts;
        for (const auto& def : nebbie::room_flag_defs()) {
            if ((flags & def.value) != 0) {
                parts << QString::fromUtf8(def.label);
            }
        }
        summary_flags_->setText(parts.isEmpty() ? QStringLiteral("(none)") : parts.join(QLatin1Char(' ')));
    }
}

void RoomEditorWidget::onAddExitFromSummary() {
    if (!world_ || current_vnum_ <= 0) {
        return;
    }
    QStringList labels;
    std::vector<int> values;
    for (const auto& [value, label] : nebbie::exit_direction_choices()) {
        labels << QString::fromStdString(label);
        values.push_back(value);
    }
    bool ok = false;
    const QString chosen =
        QInputDialog::getItem(this, QStringLiteral("Add exit"), QStringLiteral("Direction:"), labels, 0,
                              false, &ok);
    if (!ok) {
        return;
    }
    const int dir_index = labels.indexOf(chosen);
    if (dir_index < 0 || dir_index >= static_cast<int>(values.size())) {
        return;
    }
    const int direction = values[static_cast<std::size_t>(dir_index)];

    for (int i = 0; i < exit_list_->count(); ++i) {
        if (readExitItem(exit_list_->item(i)).direction == direction) {
            if (QMessageBox::question(
                    this, QStringLiteral("Add exit"),
                    QStringLiteral("C'e' gia' un'uscita in direzione %1.\nAggiungere comunque?")
                        .arg(chosen))
                != QMessageBox::Yes) {
                return;
            }
            break;
        }
    }

    EntityPickerDialog picker(EntityPickerKind::Room, *world_, this);
    if (picker.exec() != QDialog::Accepted) {
        return;
    }
    const auto to_room = picker.selectedVnum();
    if (!to_room || *to_room <= 0) {
        return;
    }

    nebbie::Exit exit;
    exit.direction = direction;
    exit.to_room = *to_room;
    exit.key = -1;
    exit.open_cmd = -1;
    auto* item = new QListWidgetItem;
    writeExitItem(item, exit);
    exit_list_->addItem(item);
    exit_list_->setCurrentItem(item);
    focusExitTab(direction);
}

void RoomEditorWidget::rebuildSummaryLists() {
    if (!entered_from_list_ || !references_list_) {
        return;
    }
    entered_from_list_->clear();
    references_list_->clear();
    if (!world_ || current_vnum_ <= 0) {
        return;
    }

    for (const auto& [from_vnum, room] : world_->rooms) {
        for (const auto& exit : room.exits) {
            if (exit.to_room != current_vnum_) {
                continue;
            }
            const QString label =
                QStringLiteral("%1 from #%2 (%3)")
                    .arg(QString::fromUtf8(nebbie::exit_direction_name(exit.direction)))
                    .arg(from_vnum)
                    .arg(QString::fromStdString(room.name));
            auto* item = new QListWidgetItem(label);
            item->setData(Qt::UserRole, static_cast<qlonglong>(from_vnum));
            entered_from_list_->addItem(item);
        }
    }

    const nebbie::Zone* zone = world_->zone_for_vnum(current_vnum_);
    if (!zone) {
        return;
    }
    long last_mob_in_room = 0;
    std::size_t last_mob_index = static_cast<std::size_t>(-1);
    for (std::size_t i = 0; i < zone->commands.size(); ++i) {
        const auto& cmd = zone->commands[i];
        if (cmd.command == 'M' && cmd.arg3 == current_vnum_) {
            last_mob_in_room = cmd.arg1;
            last_mob_index = i;
            const nebbie::Mobile* mob = world_->find_mobile(cmd.arg1);
            const QString name = mob ? QString::fromStdString(mob->short_descr) : QStringLiteral("?");
            auto* item = new QListWidgetItem(QStringLiteral("Mobile: #%1 %2").arg(cmd.arg1).arg(name));
            item->setData(Qt::UserRole, QStringLiteral("M"));
            item->setData(Qt::UserRole + 1, static_cast<qlonglong>(i));
            item->setData(Qt::UserRole + 2, static_cast<qlonglong>(cmd.arg1));
            references_list_->addItem(item);
        } else if (cmd.command == 'O' && cmd.arg3 == current_vnum_) {
            last_mob_in_room = 0;
            last_mob_index = static_cast<std::size_t>(-1);
            const nebbie::GameObject* obj = world_->find_object(cmd.arg1);
            const QString name = obj ? QString::fromStdString(obj->short_descr) : QStringLiteral("?");
            auto* item = new QListWidgetItem(QStringLiteral("Object: #%1 %2").arg(cmd.arg1).arg(name));
            item->setData(Qt::UserRole, QStringLiteral("O"));
            item->setData(Qt::UserRole + 1, static_cast<qlonglong>(i));
            item->setData(Qt::UserRole + 2, static_cast<qlonglong>(cmd.arg1));
            references_list_->addItem(item);
        } else if ((cmd.command == 'E' || cmd.command == 'G') && last_mob_in_room > 0
                   && last_mob_index != static_cast<std::size_t>(-1)) {
            const nebbie::GameObject* obj = world_->find_object(cmd.arg1);
            const QString name = obj ? QString::fromStdString(obj->short_descr) : QStringLiteral("?");
            const QString kind = cmd.command == 'E' ? QStringLiteral("Equip") : QStringLiteral("Give");
            auto* item =
                new QListWidgetItem(QStringLiteral("-- %1: #%2 %3").arg(kind).arg(cmd.arg1).arg(name));
            item->setData(Qt::UserRole, QString(QLatin1Char(cmd.command)));
            item->setData(Qt::UserRole + 1, static_cast<qlonglong>(i));
            item->setData(Qt::UserRole + 2, static_cast<qlonglong>(cmd.arg1));
            item->setData(Qt::UserRole + 3, static_cast<qlonglong>(last_mob_index));
            references_list_->addItem(item);
        } else if (cmd.command == 'M' || cmd.command == 'O') {
            last_mob_in_room = 0;
            last_mob_index = static_cast<std::size_t>(-1);
        }
    }
}

int RoomEditorWidget::currentZoneNum() const {
    if (!world_ || current_vnum_ <= 0) {
        return 0;
    }
    const nebbie::Zone* zone = world_->zone_for_vnum(current_vnum_);
    return zone ? zone->num : 0;
}

std::optional<std::size_t> RoomEditorWidget::selectedResetIndex() const {
    const QListWidgetItem* item = references_list_ ? references_list_->currentItem() : nullptr;
    if (!item) {
        return std::nullopt;
    }
    return static_cast<std::size_t>(item->data(Qt::UserRole + 1).toULongLong());
}

std::optional<std::size_t> RoomEditorWidget::selectedMobResetIndex() const {
    const QListWidgetItem* item = references_list_ ? references_list_->currentItem() : nullptr;
    if (!item) {
        return std::nullopt;
    }
    const QString kind = item->data(Qt::UserRole).toString();
    if (kind == QLatin1String("M")) {
        return static_cast<std::size_t>(item->data(Qt::UserRole + 1).toULongLong());
    }
    if (kind == QLatin1String("E") || kind == QLatin1String("G")) {
        return static_cast<std::size_t>(item->data(Qt::UserRole + 3).toULongLong());
    }
    return std::nullopt;
}

void RoomEditorWidget::onEnteredFromGoto() {
    const QListWidgetItem* item = entered_from_list_ ? entered_from_list_->currentItem() : nullptr;
    if (!item) {
        return;
    }
    emit gotoRoomRequested(static_cast<long>(item->data(Qt::UserRole).toLongLong()));
}

void RoomEditorWidget::onReferenceGoto() {
    const QListWidgetItem* item = references_list_ ? references_list_->currentItem() : nullptr;
    if (!item) {
        return;
    }
    const QString kind = item->data(Qt::UserRole).toString();
    const long vnum = static_cast<long>(item->data(Qt::UserRole + 2).toLongLong());
    if (kind == QLatin1String("M")) {
        emit gotoMobRequested(vnum);
    } else {
        emit gotoObjectRequested(vnum);
    }
}

void RoomEditorWidget::onAddMobReset() {
    if (!world_ || current_vnum_ <= 0) {
        return;
    }
    const int zone_num = currentZoneNum();
    nebbie::Zone* zone = nebbie::find_zone(*world_, zone_num);
    if (!zone) {
        QMessageBox::warning(this, QStringLiteral("Room"), QStringLiteral("Nessuna zona per questa stanza."));
        return;
    }
    EntityPickerDialog dialog(EntityPickerKind::Mobile, *world_, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    const auto mob = dialog.selectedVnum();
    if (!mob) {
        return;
    }
    nebbie::ResetCommand cmd = nebbie::default_zone_reset('M', *zone, *world_);
    cmd.arg1 = static_cast<int>(*mob);
    cmd.arg3 = static_cast<int>(current_vnum_);
    if (!nebbie::add_zone_reset(*world_, zone_num, cmd)) {
        QMessageBox::warning(this, QStringLiteral("Room"), QStringLiteral("Impossibile aggiungere il reset mob."));
        return;
    }
    rebuildSummaryLists();
    emit zoneResetsModified(zone_num);
}

void RoomEditorWidget::onAddObjectReset() {
    if (!world_ || current_vnum_ <= 0) {
        return;
    }
    const int zone_num = currentZoneNum();
    nebbie::Zone* zone = nebbie::find_zone(*world_, zone_num);
    if (!zone) {
        QMessageBox::warning(this, QStringLiteral("Room"), QStringLiteral("Nessuna zona per questa stanza."));
        return;
    }
    EntityPickerDialog dialog(EntityPickerKind::Object, *world_, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    const auto obj = dialog.selectedVnum();
    if (!obj) {
        return;
    }
    nebbie::ResetCommand cmd = nebbie::default_zone_reset('O', *zone, *world_);
    cmd.arg1 = static_cast<int>(*obj);
    cmd.arg3 = static_cast<int>(current_vnum_);
    if (!nebbie::add_zone_reset(*world_, zone_num, cmd)) {
        QMessageBox::warning(this, QStringLiteral("Room"), QStringLiteral("Impossibile aggiungere il reset oggetto."));
        return;
    }
    rebuildSummaryLists();
    emit zoneResetsModified(zone_num);
}

void RoomEditorWidget::onRemoveReset() {
    if (!world_) {
        return;
    }
    const auto index = selectedResetIndex();
    const int zone_num = currentZoneNum();
    if (!index || zone_num <= 0) {
        QMessageBox::information(this, QStringLiteral("Room"), QStringLiteral("Seleziona un riferimento da rimuovere."));
        return;
    }
    if (!nebbie::remove_zone_reset(*world_, zone_num, *index)) {
        QMessageBox::warning(this, QStringLiteral("Room"), QStringLiteral("Impossibile rimuovere il reset."));
        return;
    }
    rebuildSummaryLists();
    emit zoneResetsModified(zone_num);
}

void RoomEditorWidget::onEquipMob() {
    if (!world_) {
        return;
    }
    const auto mob_index = selectedMobResetIndex();
    const int zone_num = currentZoneNum();
    nebbie::Zone* zone = nebbie::find_zone(*world_, zone_num);
    if (!mob_index || !zone) {
        QMessageBox::information(this, QStringLiteral("Room"),
                                 QStringLiteral("Seleziona un mobile nella lista References."));
        return;
    }
    EntityPickerDialog dialog(EntityPickerKind::Object, *world_, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    const auto obj = dialog.selectedVnum();
    if (!obj) {
        return;
    }
    const auto positions = nebbie::zone_equip_position_choices();
    QStringList labels;
    for (const auto& [value, label] : positions) {
        labels << QString::fromStdString(label);
    }
    bool ok = false;
    const QString chosen = QInputDialog::getItem(this, QStringLiteral("Equip"), QStringLiteral("Wear position:"),
                                                 labels, 0, false, &ok);
    if (!ok) {
        return;
    }
    int wear = 0;
    for (const auto& [value, label] : positions) {
        if (QString::fromStdString(label) == chosen) {
            wear = value;
            break;
        }
    }
    nebbie::ResetCommand cmd = nebbie::default_zone_reset('E', *zone, *world_);
    cmd.arg1 = static_cast<int>(*obj);
    cmd.arg3 = wear;
    // Insert after the selected mob reset so E attaches to that M.
    if (!nebbie::add_zone_reset(*world_, zone_num, cmd)) {
        QMessageBox::warning(this, QStringLiteral("Room"), QStringLiteral("Impossibile aggiungere Equip."));
        return;
    }
    // Move newly appended E right after the mob line.
    const std::size_t last = zone->commands.size() - 1;
    if (last > *mob_index + 1) {
        nebbie::move_zone_reset(*world_, zone_num, last, *mob_index + 1);
    }
    rebuildSummaryLists();
    emit zoneResetsModified(zone_num);
}

void RoomEditorWidget::onGiveToMob() {
    if (!world_) {
        return;
    }
    const auto mob_index = selectedMobResetIndex();
    const int zone_num = currentZoneNum();
    nebbie::Zone* zone = nebbie::find_zone(*world_, zone_num);
    if (!mob_index || !zone) {
        QMessageBox::information(this, QStringLiteral("Room"),
                                 QStringLiteral("Seleziona un mobile nella lista References."));
        return;
    }
    EntityPickerDialog dialog(EntityPickerKind::Object, *world_, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    const auto obj = dialog.selectedVnum();
    if (!obj) {
        return;
    }
    nebbie::ResetCommand cmd = nebbie::default_zone_reset('G', *zone, *world_);
    cmd.arg1 = static_cast<int>(*obj);
    if (!nebbie::add_zone_reset(*world_, zone_num, cmd)) {
        QMessageBox::warning(this, QStringLiteral("Room"), QStringLiteral("Impossibile aggiungere Give."));
        return;
    }
    const std::size_t last = zone->commands.size() - 1;
    if (last > *mob_index + 1) {
        nebbie::move_zone_reset(*world_, zone_num, last, *mob_index + 1);
    }
    rebuildSummaryLists();
    emit zoneResetsModified(zone_num);
}

void RoomEditorWidget::loadFromRoom(const nebbie::Room& room) {
    loading_ = true;
    current_vnum_ = room.vnum;

    int preferred_exit_direction = -1;
    if (const auto* current_exit = exit_list_->currentItem()) {
        preferred_exit_direction = readExitItem(current_exit).direction;
    }
    QString preferred_extra_keyword;
    if (const auto* current_extra = extra_desc_list_->currentItem()) {
        preferred_extra_keyword = current_extra->data(Qt::UserRole).toString();
    }

    name_->setStorageText(QString::fromStdString(room.name));
    description_->setStorageText(QString::fromStdString(room.description));
    setComboIntValue(sector_type_, static_cast<int>(room.sector_type));
    room_flags_->setValue(room.room_flags);

    {
        const QSignalBlocker be(teleport_enabled_);
        const QSignalBlocker b0(tele_time_);
        const QSignalBlocker b1(tele_targ_);
        const QSignalBlocker b2(tele_mask_);
        const QSignalBlocker b3(tele_cnt_);
        tele_time_->setValue(static_cast<int>(room.tele_time));
        tele_targ_->setValue(static_cast<int>(room.tele_targ));
        tele_mask_->setValue(static_cast<int>(room.tele_mask));
        tele_cnt_->setValue(static_cast<int>(room.tele_cnt));
        const bool is_teleport = room.tele_time != 0 || room.tele_targ != 0 || room.tele_mask != 0;
        teleport_enabled_->setChecked(is_teleport);
    }

    river_speed_->setValue(static_cast<int>(room.river_speed));
    river_dir_->setValue(static_cast<int>(room.river_dir));
    moblim_->setValue(room.moblim > 0 ? static_cast<int>(room.moblim) : 1);

    bright_at_night_->setStorageText(QString::fromStdString(room.bright_at_night));
    bright_at_day_->setStorageText(QString::fromStdString(room.bright_at_day));

    extra_desc_list_->blockSignals(true);
    extra_desc_list_->clear();
    for (const auto& extra : room.extra_descs) {
        const QString label = QString::fromStdString(extra.keyword);
        auto* item = new QListWidgetItem(label.isEmpty() ? "(no keyword)" : label);
        item->setData(Qt::UserRole, QString::fromStdString(extra.keyword));
        item->setData(Qt::UserRole + 1, QString::fromStdString(extra.description));
        extra_desc_list_->addItem(item);
    }
    int selected_extra_row = -1;
    if (!preferred_extra_keyword.isEmpty()) {
        for (int i = 0; i < extra_desc_list_->count(); ++i) {
            if (extra_desc_list_->item(i)->data(Qt::UserRole).toString() == preferred_extra_keyword) {
                selected_extra_row = i;
                break;
            }
        }
    }
    if (selected_extra_row < 0 && extra_desc_list_->count() > 0) {
        selected_extra_row = 0;
    }
    extra_desc_list_->setCurrentRow(selected_extra_row);
    extra_desc_list_->blockSignals(false);

    exit_list_->blockSignals(true);
    exit_list_->clear();
    for (const auto& exit : room.exits) {
        auto* item = new QListWidgetItem;
        writeExitItem(item, exit);
        exit_list_->addItem(item);
    }
    int selected_exit_row = -1;
    if (preferred_exit_direction >= 0) {
        for (int i = 0; i < exit_list_->count(); ++i) {
            if (readExitItem(exit_list_->item(i)).direction == preferred_exit_direction) {
                selected_exit_row = i;
                break;
            }
        }
    }
    if (selected_exit_row < 0 && exit_list_->count() > 0) {
        selected_exit_row = 0;
    }
    exit_list_->setCurrentRow(selected_exit_row);
    exit_list_->blockSignals(false);

    updateConditionalFields();
    loading_ = false;
    refreshExtraDescForm();
    refreshExitForm(selected_exit_row);
    updateTextMonitors();
    refreshSummaryFromWorld();
    if (room_tabs_) {
        room_tabs_->setCurrentIndex(0); // Summary
    }
}

void RoomEditorWidget::setMaxLineLength(int max_length) {
    max_line_length_ = max_length < 0 ? 0 : max_length;
    applyColorSettingsToFields();
    updateTextMonitors();
}

void RoomEditorWidget::setShowColorCodes(bool show) {
    show_color_codes_ = show;
    applyColorSettingsToFields();
}

void RoomEditorWidget::applyColorSettingsToFields() {
    const auto apply = [this](nebbie::qt::MudColorTextEdit* field) {
        field->setShowColorCodes(show_color_codes_);
        field->setMaxLineLength(max_line_length_);
    };
    apply(name_);
    apply(description_);
    apply(extra_desc_description_);
    apply(exit_description_);
    apply(bright_at_night_);
    apply(bright_at_day_);
}

void RoomEditorWidget::hookMudField(nebbie::qt::MudColorTextEdit* field) {
    connect(field, &nebbie::qt::MudColorTextEdit::storageTextChanged, this, [this]() {
        updateTextMonitors();
    });
}

nebbie::qt::MudColorTextEdit* RoomEditorWidget::focusedMudField() const {
    return nebbie::qt::focusedMudField(
        {name_, description_, extra_desc_description_, exit_description_, bright_at_night_, bright_at_day_});
}

bool RoomEditorWidget::normalizeMudAsciiAllFields() {
    const auto mud_ascii = [](const QString& text) {
        return QString::fromStdString(nebbie::normalize_mud_ascii_text(text.toStdString()));
    };

    bool changed = false;
    const auto fix_field = [&](nebbie::qt::MudColorTextEdit* field) {
        if (field && field->normalizeMudAsciiText()) {
            changed = true;
        }
    };
    fix_field(name_);
    fix_field(description_);
    fix_field(extra_desc_description_);
    fix_field(exit_description_);
    fix_field(bright_at_night_);
    fix_field(bright_at_day_);

    const QString keyword_fixed = mud_ascii(extra_desc_keyword_->text());
    if (keyword_fixed != extra_desc_keyword_->text()) {
        extra_desc_keyword_->setText(keyword_fixed);
        changed = true;
    }
    const QString exit_keyword_fixed = mud_ascii(exit_keyword_->text());
    if (exit_keyword_fixed != exit_keyword_->text()) {
        exit_keyword_->setText(exit_keyword_fixed);
        changed = true;
    }

    // Push normalized form into the selected list rows, then normalize every stored row.
    if (auto* item = extra_desc_list_->currentItem()) {
        item->setData(Qt::UserRole, extra_desc_keyword_->text());
        item->setData(Qt::UserRole + 1, extra_desc_description_->storageText());
        item->setText(extra_desc_keyword_->text().isEmpty() ? QStringLiteral("(no keyword)")
                                                            : extra_desc_keyword_->text());
    }
    for (int i = 0; i < extra_desc_list_->count(); ++i) {
        auto* item = extra_desc_list_->item(i);
        const QString kw = mud_ascii(item->data(Qt::UserRole).toString());
        const QString desc = mud_ascii(item->data(Qt::UserRole + 1).toString());
        if (kw != item->data(Qt::UserRole).toString()
            || desc != item->data(Qt::UserRole + 1).toString()) {
            item->setData(Qt::UserRole, kw);
            item->setData(Qt::UserRole + 1, desc);
            item->setText(kw.isEmpty() ? QStringLiteral("(no keyword)") : kw);
            changed = true;
        }
    }

    if (exit_to_room_->value() > 0) {
        addOrUpdateExit();
    }
    for (int i = 0; i < exit_list_->count(); ++i) {
        auto* item = exit_list_->item(i);
        nebbie::Exit exit = readExitItem(item);
        const std::string desc = nebbie::normalize_mud_ascii_text(exit.description);
        const std::string keyword = nebbie::normalize_mud_ascii_text(exit.keyword);
        if (desc != exit.description || keyword != exit.keyword) {
            exit.description = desc;
            exit.keyword = keyword;
            writeExitItem(item, exit);
            changed = true;
        }
    }

    if (changed) {
        updateTextMonitors();
    }
    return changed;
}

void RoomEditorWidget::updateTextMonitors() {
    if (loading_) {
        return;
    }
    nebbie::qt::updateLineLengthMonitor(name_line_info_, name_, max_line_length_);
    nebbie::qt::updateLineLengthMonitor(description_line_info_, description_, max_line_length_);
    nebbie::qt::updateLineLengthMonitor(extra_desc_line_info_, extra_desc_description_, max_line_length_);
    nebbie::qt::updateLineLengthMonitor(exit_line_info_, exit_description_, max_line_length_);
    nebbie::qt::updateAsciiMonitor(name_ascii_info_, name_->storageText().toStdString());
    nebbie::qt::updateAsciiMonitor(description_ascii_info_, description_->storageText().toStdString());
    nebbie::qt::updateAsciiMonitor(extra_desc_ascii_info_, extra_desc_description_->storageText().toStdString());
    nebbie::qt::updateAsciiMonitor(exit_ascii_info_, exit_description_->storageText().toStdString());
    nebbie::qt::updateEnglishLexiconMonitor(name_english_info_, name_);
    nebbie::qt::updateEnglishLexiconMonitor(description_english_info_, description_);
    nebbie::qt::updateEnglishLexiconMonitor(extra_desc_keyword_english_info_, extra_desc_keyword_);
    nebbie::qt::updateEnglishLexiconMonitor(extra_desc_english_info_, extra_desc_description_);
    nebbie::qt::updateEnglishLexiconMonitor(exit_english_info_, exit_description_);
    nebbie::qt::updateEnglishLexiconMonitor(exit_keyword_english_info_, exit_keyword_);
}

void RoomEditorWidget::commitPendingExit() {
    if (loading_ || exit_to_room_->value() <= 0) {
        return;
    }
    addOrUpdateExit();
}

void RoomEditorWidget::saveToRoom(nebbie::Room& room) {
    commitPendingExtraDesc();
    commitPendingExit();
    room.name = name_->storageText().toStdString();
    room.description = description_->storageText().toStdString();
    room.sector_type = comboIntValue(sector_type_);
    room.room_flags = room_flags_->value();

    if (teleport_enabled_->isChecked()) {
        room.tele_time = tele_time_->value();
        room.tele_targ = tele_targ_->value();
        room.tele_mask = tele_mask_->value();
        room.tele_cnt = tele_cnt_->value();
    } else {
        room.tele_time = 0;
        room.tele_targ = 0;
        room.tele_mask = 0;
        room.tele_cnt = 0;
    }

    if (nebbie::room_sector_uses_river(room.sector_type)) {
        room.river_speed = river_speed_->value();
        room.river_dir = river_dir_->value();
    } else {
        room.river_speed = 0;
        room.river_dir = 0;
    }

    if (nebbie::room_flags_use_moblim(room.room_flags)) {
        room.moblim = moblim_->value();
    } else {
        room.moblim = 0;
    }

    room.bright_at_night = bright_at_night_->storageText().toStdString();
    room.bright_at_day = bright_at_day_->storageText().toStdString();

    room.extra_descs.clear();
    for (int i = 0; i < extra_desc_list_->count(); ++i) {
        const auto* item = extra_desc_list_->item(i);
        nebbie::ExtraDesc extra;
        extra.keyword = item->data(Qt::UserRole).toString().toStdString();
        extra.description = item->data(Qt::UserRole + 1).toString().toStdString();
        room.extra_descs.push_back(std::move(extra));
    }

    room.exits.clear();
    for (int i = 0; i < exit_list_->count(); ++i) {
        room.exits.push_back(readExitItem(exit_list_->item(i)));
    }
}

long RoomEditorWidget::selectedExitToRoom() const {
    const auto* item = exit_list_->currentItem();
    if (!item) {
        return 0;
    }
    return readExitItem(item).to_room;
}

void RoomEditorWidget::onExtraDescSelected() {
    refreshExtraDescForm();
}

void RoomEditorWidget::addExtraDesc() {
    const QString keyword = extra_desc_keyword_->text().trimmed();
    const QString description = extra_desc_description_->storageText();
    if (keyword.isEmpty() && description.trimmed().isEmpty()) {
        return;
    }
    auto* item = new QListWidgetItem(keyword.isEmpty() ? "(no keyword)" : keyword);
    item->setData(Qt::UserRole, keyword);
    item->setData(Qt::UserRole + 1, description);
    extra_desc_list_->addItem(item);
    extra_desc_list_->setCurrentItem(item);
    extra_desc_keyword_->clear();
    extra_desc_description_->setStorageText({});
}

void RoomEditorWidget::updateExtraDesc() {
    auto* item = extra_desc_list_->currentItem();
    if (!item) {
        return;
    }
    const QString keyword = extra_desc_keyword_->text().trimmed();
    const QString description = extra_desc_description_->storageText();
    if (keyword.isEmpty() && description.trimmed().isEmpty()) {
        return;
    }
    item->setData(Qt::UserRole, keyword);
    item->setData(Qt::UserRole + 1, description);
    item->setText(keyword.isEmpty() ? QStringLiteral("(no keyword)") : keyword);
}

void RoomEditorWidget::commitPendingExtraDesc() {
    if (loading_ || extra_desc_list_->currentItem() == nullptr) {
        return;
    }
    const QString keyword = extra_desc_keyword_->text().trimmed();
    const QString description = extra_desc_description_->storageText();
    if (keyword.isEmpty() && description.trimmed().isEmpty()) {
        return;
    }
    updateExtraDesc();
}

void RoomEditorWidget::removeExtraDesc() {
    const int row = extra_desc_list_->currentRow();
    if (row < 0) {
        return;
    }
    delete extra_desc_list_->takeItem(row);
    refreshExtraDescForm();
}

void RoomEditorWidget::refreshExtraDescForm() {
    const auto* item = extra_desc_list_->currentItem();
    if (!item) {
        extra_desc_keyword_->clear();
        extra_desc_description_->setStorageText({});
        return;
    }
    extra_desc_keyword_->setText(item->data(Qt::UserRole).toString());
    extra_desc_description_->setStorageText(item->data(Qt::UserRole + 1).toString());
    updateTextMonitors();
}

void RoomEditorWidget::onExitSelected() {
    refreshExitForm();
}

void RoomEditorWidget::addOrUpdateExit() {
    nebbie::Exit exit;
    exit.direction = comboIntValue(exit_direction_);
    exit.to_room = exit_to_room_->value();
    exit.description = exit_description_->storageText().toStdString();
    exit.keyword = exit_keyword_->text().toStdString();
    exit.exit_info = exit_flags_->value();
    exit.key = exit_key_->value();
    exit.open_cmd = comboIntValue(exit_open_cmd_);

    for (int i = 0; i < exit_list_->count(); ++i) {
        auto* item = exit_list_->item(i);
        if (item->data(Qt::UserRole).toInt() == exit.direction) {
            writeExitItem(item, exit);
            exit_list_->setCurrentItem(item);
            return;
        }
    }

    auto* item = new QListWidgetItem;
    writeExitItem(item, exit);
    exit_list_->addItem(item);
    exit_list_->setCurrentItem(item);
}

void RoomEditorWidget::removeExit() {
    const int row = exit_list_->currentRow();
    if (row < 0) {
        return;
    }
    delete exit_list_->takeItem(row);
    refreshExitForm();
}

void RoomEditorWidget::refreshExitForm(const int row) {
    const int selected_row = row >= 0 ? row : exit_list_->currentRow();
    const QListWidgetItem* item =
        selected_row >= 0 && selected_row < exit_list_->count() ? exit_list_->item(selected_row) : nullptr;

    const QSignalBlocker block_direction(exit_direction_);
    const QSignalBlocker block_to_room(exit_to_room_);
    const QSignalBlocker block_description(exit_description_);
    const QSignalBlocker block_keyword(exit_keyword_);
    const QSignalBlocker block_flags(exit_flags_);
    const QSignalBlocker block_key(exit_key_);
    const QSignalBlocker block_open_cmd(exit_open_cmd_);

    if (!item) {
        setComboIntValue(exit_direction_, 0);
        exit_to_room_->setValue(0);
        exit_description_->setStorageText({});
        exit_keyword_->clear();
        exit_flags_->setValue(0);
        exit_key_->setValue(-1);
        setComboIntValue(exit_open_cmd_, -1);
        updateTextMonitors();
        return;
    }
    const nebbie::Exit exit = readExitItem(item);
    setComboIntValue(exit_direction_, exit.direction);
    exit_to_room_->setValue(static_cast<int>(exit.to_room));
    exit_description_->setStorageText(QString::fromStdString(exit.description));
    exit_keyword_->setText(QString::fromStdString(exit.keyword));
    exit_flags_->setValue(exit.exit_info);
    exit_key_->setValue(static_cast<int>(exit.key));
    const int open_cmd = static_cast<int>(exit.open_cmd);
    if (exit_open_cmd_->findData(open_cmd) < 0) {
        exit_open_cmd_->addItem(
            QString::fromStdString(nebbie::exit_open_cmd_label(open_cmd)), open_cmd);
    }
    setComboIntValue(exit_open_cmd_, open_cmd);
    updateTextMonitors();
}
