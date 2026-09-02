#include "room_editor_widget.hpp"

#include "flag_group_widget.hpp"
#include "mud_color_widgets.hpp"
#include "nebbie/mob_catalog.hpp"
#include "nebbie/room_catalog.hpp"
#include "room_text_monitors.hpp"

#include "mud_editor_fields.hpp"

#include <QComboBox>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QSpinBox>
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
        details += QString(" open_cmd=%1").arg(exit.open_cmd);
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

} // namespace

void RoomEditorWidget::setComboIntValue(QComboBox* combo, const int value) const {
    const int index = combo->findData(value);
    combo->setCurrentIndex(index >= 0 ? index : 0);
}

RoomEditorWidget::RoomEditorWidget(QWidget* parent) : QWidget(parent) {
    auto* root = new QVBoxLayout(this);

    text_section_ = new QWidget;
    auto* text_layout = new QVBoxLayout(text_section_);
    text_layout->addWidget(makeLegend(
        "Nome e descrizione mostrati all'ingresso. Il client legge solo testo ASCII "
        "(usa e', a', o', u' al posto delle lettere accentate).",
        text_section_));
    name_ = new nebbie::qt::MudColorTextEdit;
    nebbie::qt::configureMudSingleLineField(name_);
    name_line_info_ = nebbie::qt::makeTextMonitorLabel();
    name_ascii_info_ = nebbie::qt::makeTextMonitorLabel();
    description_ = new nebbie::qt::MudColorTextEdit;
    nebbie::qt::configureMudDescriptionField(description_);
    description_line_info_ = nebbie::qt::makeTextMonitorLabel();
    description_ascii_info_ = nebbie::qt::makeTextMonitorLabel();
    auto* text_form = new QFormLayout;
    text_form->addRow("Nome:", name_);
    text_form->addRow("", name_line_info_);
    text_form->addRow("", name_ascii_info_);
    text_form->addRow("Descrizione:", description_);
    text_form->addRow("", description_line_info_);
    text_form->addRow("", description_ascii_info_);
    text_layout->addLayout(text_form);
    root->addWidget(text_section_);
    root->addWidget(makeSectionSeparator());

    sector_section_ = new QWidget;
    auto* sector_layout = new QVBoxLayout(sector_section_);
    sector_layout->addWidget(makeLegend(
        "Settore e flag stanza. TUNNEL abilita moblim; i settori acqua usano i campi fiume "
        "nella sezione Ambiente.",
        sector_section_));
    auto* sector_form = new QFormLayout;
    sector_type_ = new QComboBox;
    fillCombo(sector_type_, nebbie::room_sector_choices());
    room_flags_ = new FlagGroupWidget(nebbie::room_flag_defs(), sector_section_);
    sector_form->addRow("Sector type:", sector_type_);
    sector_layout->addLayout(sector_form);
    sector_layout->addWidget(new QLabel("Room flags"));
    sector_layout->addWidget(room_flags_);
    root->addWidget(sector_section_);
    root->addWidget(makeSectionSeparator());

    teleport_section_ = new QWidget;
    auto* tele_layout = new QVBoxLayout(teleport_section_);
    tele_layout->addWidget(makeLegend(
        "Stanze teleport: sector -1 in myst.wld. tele_time / tele_targ / tele_mask controllano "
        "il teletrasporto; con bit 0 (TELE_COUNT) attivo, tele_cnt viene scritto.",
        teleport_section_));
    auto* tele_form = new QFormLayout;
    tele_time_ = new QSpinBox;
    tele_targ_ = new QSpinBox;
    tele_mask_ = new QSpinBox;
    tele_cnt_ = new QSpinBox;
    for (QSpinBox* spin : {tele_time_, tele_targ_, tele_mask_, tele_cnt_}) {
        spin->setRange(0, 2000000000);
    }
    tele_form->addRow("tele_time:", tele_time_);
    tele_form->addRow("tele_targ:", tele_targ_);
    tele_form->addRow("tele_mask:", tele_mask_);
    tele_form->addRow("tele_cnt:", tele_cnt_);
    tele_layout->addLayout(tele_form);
    root->addWidget(teleport_section_);
    root->addWidget(makeSectionSeparator());

    environment_section_ = new QWidget;
    auto* env_layout = new QVBoxLayout(environment_section_);
    river_panel_ = new QWidget(environment_section_);
    auto* river_form = new QFormLayout(river_panel_);
    river_speed_ = new QSpinBox;
    river_dir_ = new QSpinBox;
    river_speed_->setRange(0, 2000000000);
    river_dir_->setRange(0, 5);
    river_dir_->setToolTip("River direction 0=north .. 5=down (Arcane redit).");
    river_form->addRow("River speed:", river_speed_);
    river_form->addRow("River direction (0-5):", river_dir_);
    moblim_panel_ = new QWidget(environment_section_);
    auto* moblim_form = new QFormLayout(moblim_panel_);
    moblim_ = new QSpinBox;
    moblim_->setRange(1, 2000000000);
    moblim_form->addRow("moblim (TUNNEL):", moblim_);
    bright_at_night_ = new nebbie::qt::MudColorTextEdit;
    bright_at_day_ = new nebbie::qt::MudColorTextEdit;
    nebbie::qt::configureMudSingleLineField(bright_at_night_);
    nebbie::qt::configureMudSingleLineField(bright_at_day_);
    auto* bright_form = new QFormLayout;
    bright_form->addRow("Bright at night (L):", bright_at_night_);
    bright_form->addRow("Bright at day (L):", bright_at_day_);
    env_layout->addWidget(makeLegend(
        "Fiume per settori acqua. moblim con flag TUNNEL. L = stringhe luminosità opzionali.",
        environment_section_));
    env_layout->addWidget(river_panel_);
    env_layout->addWidget(moblim_panel_);
    env_layout->addLayout(bright_form);
    root->addWidget(environment_section_);
    root->addWidget(makeSectionSeparator());

    extra_section_ = new QWidget;
    auto* extra_layout = new QVBoxLayout(extra_section_);
    extra_desc_list_ = new QListWidget;
    extra_desc_list_->setMaximumHeight(140);
    extra_desc_keyword_ = new QLineEdit;
    configureLineField(extra_desc_keyword_);
    extra_desc_description_ = new nebbie::qt::MudColorTextEdit;
    configureExtraDescriptionField(extra_desc_description_);
    extra_desc_line_info_ = nebbie::qt::makeTextMonitorLabel();
    extra_desc_ascii_info_ = nebbie::qt::makeTextMonitorLabel();
    auto* extra_desc_form = new QFormLayout;
    extra_desc_form->addRow("Keyword:", extra_desc_keyword_);
    extra_desc_form->addRow("Description:", extra_desc_description_);
    extra_desc_form->addRow("", extra_desc_line_info_);
    extra_desc_form->addRow("", extra_desc_ascii_info_);
    auto* extra_desc_buttons = new QHBoxLayout;
    auto* extra_desc_add = new QPushButton("Add");
    auto* extra_desc_remove = new QPushButton("Remove");
    extra_desc_buttons->addWidget(extra_desc_add);
    extra_desc_buttons->addWidget(extra_desc_remove);
    extra_desc_buttons->addStretch();
    extra_layout->addWidget(new QLabel("Extra descriptions (section E)"));
    extra_layout->addWidget(extra_desc_list_);
    extra_layout->addLayout(extra_desc_form);
    extra_layout->addLayout(extra_desc_buttons);
    root->addWidget(extra_section_);
    root->addWidget(makeSectionSeparator());

    exits_section_ = new QWidget;
    auto* exit_layout = new QVBoxLayout(exits_section_);
    exit_layout->addWidget(makeLegend(
        "Direzioni: 0=nord, 1=est, 2=sud, 3=ovest, 4=su, 5=giù. Description = testo look <dir>; "
        "se vuota il server usa il name della destinazione. Key vnum = oggetto chiave (-1 = nessuna).",
        exits_section_));
    exit_list_ = new QListWidget;
    exit_list_->setMaximumHeight(140);
    exit_direction_ = new QComboBox;
    fillCombo(exit_direction_, nebbie::exit_direction_choices());
    exit_to_room_ = new QSpinBox;
    exit_to_room_->setRange(0, 999999);
    exit_description_ = new nebbie::qt::MudColorTextEdit;
    nebbie::qt::configureMudSingleLineField(exit_description_);
    exit_keyword_ = new QLineEdit;
    configureLineField(exit_keyword_);
    exit_line_info_ = nebbie::qt::makeTextMonitorLabel();
    exit_ascii_info_ = nebbie::qt::makeTextMonitorLabel();
    exit_flags_ = new FlagGroupWidget(nebbie::exit_flag_defs(), exits_section_);
    exit_key_ = new QSpinBox;
    exit_key_->setRange(-1, 999999);
    exit_key_->setValue(-1);
    exit_key_->setToolTip("VNUM dell'oggetto chiave per porte bloccate. Usare -1 se la porta non richiede chiave.");
    exit_open_cmd_ = new QSpinBox;
    exit_open_cmd_->setRange(-1, 999999);
    exit_open_cmd_->setValue(-1);
    exit_open_cmd_->setToolTip("Comando open_cmd opzionale scritto in myst.wld; -1 = omesso.");
    auto* exit_form = new QFormLayout;
    exit_form->addRow("Direction:", exit_direction_);
    exit_form->addRow("To room #:", exit_to_room_);
    exit_form->addRow("Description (look <dir>):", exit_description_);
    exit_form->addRow("", exit_line_info_);
    exit_form->addRow("", exit_ascii_info_);
    exit_form->addRow("Keyword (porta):", exit_keyword_);
    exit_form->addRow("Exit flags (exit_info):", exit_flags_);
    exit_form->addRow("Key vnum (oggetto):", exit_key_);
    exit_form->addRow("open_cmd:", exit_open_cmd_);
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
    root->addWidget(exits_section_, 1);

    connect(sector_type_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int) { updateConditionalFields(); });
    connect(room_flags_, &FlagGroupWidget::valueChanged, this, [this]() { updateConditionalFields(); });
    connect(extra_desc_list_, &QListWidget::currentRowChanged, this, [this](int) { onExtraDescSelected(); });
    connect(extra_desc_add, &QPushButton::clicked, this, &RoomEditorWidget::addExtraDesc);
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

    river_panel_->setVisible(false);
    moblim_panel_->setVisible(false);
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
    scrollToSection(exits_section_);
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
}

void RoomEditorWidget::loadFromRoom(const nebbie::Room& room) {
    loading_ = true;

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

    tele_time_->setValue(static_cast<int>(room.tele_time));
    tele_targ_->setValue(static_cast<int>(room.tele_targ));
    tele_mask_->setValue(static_cast<int>(room.tele_mask));
    tele_cnt_->setValue(static_cast<int>(room.tele_cnt));

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
}

void RoomEditorWidget::commitPendingExit() {
    if (loading_ || exit_to_room_->value() <= 0) {
        return;
    }
    addOrUpdateExit();
}

void RoomEditorWidget::saveToRoom(nebbie::Room& room) {
    commitPendingExit();
    room.name = name_->storageText().toStdString();
    room.description = description_->storageText().toStdString();
    room.sector_type = comboIntValue(sector_type_);
    room.room_flags = room_flags_->value();

    room.tele_time = tele_time_->value();
    room.tele_targ = tele_targ_->value();
    room.tele_mask = tele_mask_->value();
    room.tele_cnt = tele_cnt_->value();

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
    exit.open_cmd = exit_open_cmd_->value();

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
        exit_open_cmd_->setValue(-1);
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
    exit_open_cmd_->setValue(static_cast<int>(exit.open_cmd));
    updateTextMonitors();
}
