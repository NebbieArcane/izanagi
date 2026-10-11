#include "obj_editor_widget.hpp"

#include "flag_group_widget.hpp"
#include "mud_color_widgets.hpp"
#include "mud_editor_fields.hpp"
#include "nebbie/obj_catalog.hpp"
#include "nebbie/obj_value_catalog.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QSignalBlocker>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QScrollArea>
#include <QSizePolicy>
#include <QSpinBox>
#include <QTabWidget>
#include <QTextEdit>
#include <QVBoxLayout>

namespace {

void configureLineField(QLineEdit* field) {
    field->setMinimumWidth(420);
    field->setMinimumHeight(30);
    field->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void configureShortTextField(nebbie::qt::MudColorTextEdit* field) {
    field->setMinimumHeight(56);
    field->setMaximumHeight(80);
    field->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    field->setLineWrapMode(QTextEdit::WidgetWidth);
}

void configureTextField(nebbie::qt::MudColorTextEdit* field, const int min_height) {
    field->setMinimumHeight(min_height);
    field->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
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

} // namespace

void ObjEditorWidget::setComboIntValue(QComboBox* combo, const int value) const {
    const int index = combo->findData(value);
    combo->setCurrentIndex(index >= 0 ? index : 0);
}

ObjEditorWidget::ObjEditorWidget(QWidget* parent) : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    auto* tabs = new QTabWidget;

    auto* text_tab = new QWidget;
    auto* text_form = new QFormLayout(text_tab);
    name_ = new nebbie::qt::MudColorTextEdit;
    nebbie::qt::configureMudSingleLineField(name_);
    short_descr_ = new nebbie::qt::MudColorTextEdit;
    configureShortTextField(short_descr_);
    description_ = new nebbie::qt::MudColorTextEdit;
    configureTextField(description_, 110);
    action_description_ = new nebbie::qt::MudColorTextEdit;
    configureTextField(action_description_, 100);
    action_description_->setPlaceholderText("Action description (e.g. weapons/potions); leave empty if unused");
    text_form->addRow("Name (keywords~):", name_);
    text_form->addRow("Short description:", short_descr_);
    text_form->addRow("Description:", description_);
    text_form->addRow("Action description:", action_description_);
    tabs->addTab(text_tab, "Text");

    auto* type_tab = new QWidget;
    auto* type_layout = new QVBoxLayout(type_tab);
    value_legend_ = makeLegend(
        "I quattro valori (value[0..3]) dipendono dal tipo oggetto. Legenda allineata a "
        "NebbieArcane/Server (create.obj / stat). Il prezzo negozio è nel tab Economy (cost).",
        type_tab);
    type_layout->addWidget(value_legend_);
    auto* type_form = new QFormLayout;
    type_flag_ = new QComboBox;
    fillCombo(type_flag_, nebbie::obj_type_choices());
    type_form->addRow("Tipo oggetto:", type_flag_);

    auto make_value_row = [&](int index, QSpinBox*& spin, QComboBox*& combo, QLabel*& hint, QLabel*& row_label) {
        row_label = new QLabel(QString("value[%1]:").arg(index), type_tab);
        auto* row = new QWidget(type_tab);
        auto* row_layout = new QVBoxLayout(row);
        row_layout->setContentsMargins(0, 0, 0, 0);
        auto* editors = new QHBoxLayout;
        spin = new QSpinBox(row);
        spin->setRange(-2000000000, 2000000000);
        combo = new QComboBox(row);
        combo->setMinimumWidth(280);
        combo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        editors->addWidget(spin);
        editors->addWidget(combo, 1);
        row_layout->addLayout(editors);
        hint = new QLabel(type_tab);
        hint->setWordWrap(true);
        hint->setStyleSheet("color: palette(mid);");
        row_layout->addWidget(hint);
        type_form->addRow(row_label, row);
    };

    make_value_row(0, value0_, value_combo0_, value_hint0_, value_label0_);
    make_value_row(1, value1_, value_combo1_, value_hint1_, value_label1_);
    make_value_row(2, value2_, value_combo2_, value_hint2_, value_label2_);
    make_value_row(3, value3_, value_combo3_, value_hint3_, value_label3_);

    type_layout->addLayout(type_form);
    tabs->addTab(type_tab, "Tipo / Valori");

    auto* flags_tab = new QWidget;
    auto* flags_layout = new QVBoxLayout(flags_tab);
    wear_flags_ = new FlagGroupWidget(nebbie::obj_wear_flags(), flags_tab);
    extra_flags_ = new FlagGroupWidget(nebbie::obj_extra_flags(), flags_tab);
    has_extra_flags2_ = new QCheckBox("Section F (extra_flags2)");
    extra_flags2_panel_ = new QWidget(flags_tab);
    auto* extra_flags2_layout = new QVBoxLayout(extra_flags2_panel_);
    extra_flags2_ = new FlagGroupWidget(nebbie::obj_extra_flags2(), extra_flags2_panel_);
    extra_flags2_layout->addWidget(extra_flags2_);
    flags_layout->addWidget(new QLabel("Wear flags"));
    flags_layout->addWidget(wear_flags_);
    flags_layout->addWidget(new QLabel("Extra flags"));
    flags_layout->addWidget(extra_flags_);
    flags_layout->addWidget(has_extra_flags2_);
    flags_layout->addWidget(extra_flags2_panel_);
    tabs->addTab(new QScrollArea, "Flags");
    {
        auto* scroll = qobject_cast<QScrollArea*>(tabs->widget(2));
        scroll->setWidgetResizable(true);
        scroll->setWidget(flags_tab);
    }

    auto* economy_tab = new QWidget;
    auto* economy_layout = new QVBoxLayout(economy_tab);
    economy_layout->addWidget(makeLegend(
        "Value is the object shop value (obj.cost). Rent cost is the rental cost per day "
        "(obj.cost_per_day), as in OLC menu items 8 and 9.",
        economy_tab));
    auto* economy_form = new QFormLayout;
    weight_ = new QSpinBox;
    weight_->setRange(0, 2000000000);
    cost_ = new QSpinBox;
    cost_->setRange(0, 2000000000);
    cost_per_day_ = new QSpinBox;
    cost_per_day_->setRange(0, 2000000000);
    economy_form->addRow("Weight:", weight_);
    economy_form->addRow("Value:", cost_);
    economy_form->addRow("Rent cost per day:", cost_per_day_);
    economy_layout->addLayout(economy_form);
    tabs->addTab(economy_tab, "Economy");

    auto* affect_tab = new QWidget;
    auto* affect_layout = new QVBoxLayout(affect_tab);
    affect_list_ = new QListWidget;
    affect_location_ = new QComboBox;
    fillCombo(affect_location_, nebbie::obj_apply_type_choices());
    affect_modifier_ = new QSpinBox;
    affect_modifier_->setRange(-2000000000, 2000000000);
    auto* affect_form = new QFormLayout;
    affect_form->addRow("Apply type:", affect_location_);
    affect_form->addRow("Modifier:", affect_modifier_);
    auto* affect_buttons = new QHBoxLayout;
    auto* affect_add = new QPushButton("Add");
    auto* affect_remove = new QPushButton("Remove");
    affect_buttons->addWidget(affect_add);
    affect_buttons->addWidget(affect_remove);
    affect_buttons->addStretch();
    affect_layout->addWidget(new QLabel("Extra affects (section A)"));
    affect_layout->addWidget(affect_list_, 1);
    affect_layout->addLayout(affect_form);
    affect_layout->addLayout(affect_buttons);
    tabs->addTab(affect_tab, "Extra affects");

    auto* extra_tab = new QWidget;
    auto* extra_layout = new QVBoxLayout(extra_tab);
    extra_desc_list_ = new QListWidget;
    extra_desc_keyword_ = new QLineEdit;
    configureLineField(extra_desc_keyword_);
    extra_desc_description_ = new nebbie::qt::MudColorTextEdit;
    configureTextField(extra_desc_description_, 90);
    auto* extra_desc_form = new QFormLayout;
    extra_desc_form->addRow("Keyword:", extra_desc_keyword_);
    extra_desc_form->addRow("Description:", extra_desc_description_);
    auto* extra_desc_buttons = new QHBoxLayout;
    auto* extra_desc_add = new QPushButton("Add");
    auto* extra_desc_remove = new QPushButton("Remove");
    extra_desc_buttons->addWidget(extra_desc_add);
    extra_desc_buttons->addWidget(extra_desc_remove);
    extra_desc_buttons->addStretch();
    forbidden_char_ = new QLineEdit;
    forbidden_room_ = new QLineEdit;
    auto* forbidden_form = new QFormLayout;
    configureLineField(forbidden_char_);
    configureLineField(forbidden_room_);
    forbidden_form->addRow("Forbidden char (P):", forbidden_char_);
    forbidden_form->addRow("Forbidden room (P):", forbidden_room_);
    extra_layout->addWidget(new QLabel("Extra descriptions (section E)"));
    extra_layout->addWidget(extra_desc_list_, 1);
    extra_layout->addLayout(extra_desc_form);
    extra_layout->addLayout(extra_desc_buttons);
    extra_layout->addLayout(forbidden_form);
    tabs->addTab(extra_tab, "Extra / Forbidden");

    root->addWidget(tabs);

    connect(has_extra_flags2_, &QCheckBox::toggled, extra_flags2_panel_, &QWidget::setVisible);
    connect(extra_desc_list_, &QListWidget::currentRowChanged, this, [this](int) { onExtraDescSelected(); });
    connect(extra_desc_add, &QPushButton::clicked, this, &ObjEditorWidget::addExtraDesc);
    connect(extra_desc_remove, &QPushButton::clicked, this, &ObjEditorWidget::removeExtraDesc);
    connect(affect_list_, &QListWidget::currentRowChanged, this, [this](int) { onAffectSelected(); });
    connect(affect_add, &QPushButton::clicked, this, &ObjEditorWidget::addAffect);
    connect(affect_remove, &QPushButton::clicked, this, &ObjEditorWidget::removeAffect);

    connect(type_flag_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int) { updateObjectValueFields(); });

    const auto connect_value_slot = [this](int slot, QSpinBox* spin, QComboBox* combo) {
        connect(spin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this, slot](int) {
            syncValueComboFromSpin(slot);
            updateObjectValueFields();
        });
        connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, slot](int) {
            syncValueSpinFromCombo(slot);
            updateObjectValueFields();
        });
    };
    connect_value_slot(0, value0_, value_combo0_);
    connect_value_slot(1, value1_, value_combo1_);
    connect_value_slot(2, value2_, value_combo2_);
    connect_value_slot(3, value3_, value_combo3_);

    extra_flags2_panel_->setVisible(false);
    updateObjectValueFields();
}

nebbie::qt::MudFieldList ObjEditorWidget::mudFields() const {
    return {name_, short_descr_, description_, action_description_, extra_desc_description_};
}

void ObjEditorWidget::applyMudFieldSettings() {
    nebbie::qt::applyMudFieldSettings(mudFields(), max_line_length_, show_color_codes_);
}

void ObjEditorWidget::setMaxLineLength(const int max_length) {
    max_line_length_ = max_length < 0 ? 0 : max_length;
    applyMudFieldSettings();
}

void ObjEditorWidget::setShowColorCodes(const bool show) {
    show_color_codes_ = show;
    applyMudFieldSettings();
}

nebbie::qt::MudColorTextEdit* ObjEditorWidget::focusedMudField() const {
    return nebbie::qt::focusedMudField(mudFields());
}

void ObjEditorWidget::focusPrimaryTextField() {
    short_descr_->setFocus();
}

void ObjEditorWidget::loadFromObject(const nebbie::GameObject& obj) {
    name_->setStorageText(QString::fromStdString(obj.name));
    short_descr_->setStorageText(QString::fromStdString(obj.short_descr));
    description_->setStorageText(QString::fromStdString(obj.description));
    action_description_->setStorageText(QString::fromStdString(obj.action_description));

    setComboIntValue(type_flag_, obj.type_flag);
    const QSignalBlocker b0(value0_);
    const QSignalBlocker b1(value1_);
    const QSignalBlocker b2(value2_);
    const QSignalBlocker b3(value3_);
    value0_->setValue(obj.value[0]);
    value1_->setValue(obj.value[1]);
    value2_->setValue(obj.value[2]);
    value3_->setValue(obj.value[3]);
    updateObjectValueFields();

    wear_flags_->setValue(obj.wear_flags);
    extra_flags_->setValue(obj.extra_flags);
    has_extra_flags2_->setChecked(obj.has_extra_flags2);
    extra_flags2_panel_->setVisible(obj.has_extra_flags2);
    extra_flags2_->setValue(obj.extra_flags2);

    weight_->setValue(obj.weight);
    cost_->setValue(obj.cost);
    cost_per_day_->setValue(obj.cost_per_day);

    affect_list_->clear();
    for (const auto& affect : obj.affects) {
        const QString label = QString("%1 %+2")
                                  .arg(QString::fromStdString(nebbie::obj_apply_type_name(affect.location)))
                                  .arg(affect.modifier);
        auto* item = new QListWidgetItem(label);
        item->setData(Qt::UserRole, affect.location);
        item->setData(Qt::UserRole + 1, affect.modifier);
        affect_list_->addItem(item);
    }

    extra_desc_list_->clear();
    for (const auto& extra : obj.extra_descs) {
        const QString label = QString::fromStdString(extra.keyword);
        auto* item = new QListWidgetItem(label.isEmpty() ? "(no keyword)" : label);
        item->setData(Qt::UserRole, QString::fromStdString(extra.keyword));
        item->setData(Qt::UserRole + 1, QString::fromStdString(extra.description));
        extra_desc_list_->addItem(item);
    }

    forbidden_char_->setText(QString::fromStdString(obj.forbidden_char));
    forbidden_room_->setText(QString::fromStdString(obj.forbidden_room));
}

void ObjEditorWidget::saveToObject(nebbie::GameObject& obj) const {
    obj.name = name_->storageText().toStdString();
    obj.short_descr = short_descr_->storageText().toStdString();
    obj.description = description_->storageText().toStdString();
    obj.action_description = action_description_->storageText().toStdString();

    obj.type_flag = comboIntValue(type_flag_);
    obj.value[0] = value0_->value();
    obj.value[1] = value1_->value();
    obj.value[2] = value2_->value();
    obj.value[3] = value3_->value();

    obj.wear_flags = wear_flags_->value();
    obj.extra_flags = extra_flags_->value();
    obj.has_extra_flags2 = has_extra_flags2_->isChecked();
    obj.extra_flags2 = obj.has_extra_flags2 ? extra_flags2_->value() : 0;

    obj.weight = weight_->value();
    obj.cost = cost_->value();
    obj.cost_per_day = cost_per_day_->value();

    obj.affects.clear();
    for (int i = 0; i < affect_list_->count(); ++i) {
        const auto* item = affect_list_->item(i);
        nebbie::ObjAffect affect;
        affect.location = item->data(Qt::UserRole).toInt();
        affect.modifier = item->data(Qt::UserRole + 1).toInt();
        obj.affects.push_back(affect);
    }

    obj.extra_descs.clear();
    for (int i = 0; i < extra_desc_list_->count(); ++i) {
        const auto* item = extra_desc_list_->item(i);
        nebbie::ExtraDesc extra;
        extra.keyword = item->data(Qt::UserRole).toString().toStdString();
        extra.description = item->data(Qt::UserRole + 1).toString().toStdString();
        obj.extra_descs.push_back(std::move(extra));
    }

    obj.forbidden_char = forbidden_char_->text().toStdString();
    obj.forbidden_room = forbidden_room_->text().toStdString();
}

void ObjEditorWidget::onExtraDescSelected() {
    refreshExtraDescForm();
}

void ObjEditorWidget::addExtraDesc() {
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

void ObjEditorWidget::removeExtraDesc() {
    const int row = extra_desc_list_->currentRow();
    if (row < 0) {
        return;
    }
    delete extra_desc_list_->takeItem(row);
    refreshExtraDescForm();
}

void ObjEditorWidget::refreshExtraDescForm() {
    const auto* item = extra_desc_list_->currentItem();
    if (!item) {
        extra_desc_keyword_->clear();
        extra_desc_description_->setStorageText({});
        return;
    }
    extra_desc_keyword_->setText(item->data(Qt::UserRole).toString());
    extra_desc_description_->setStorageText(item->data(Qt::UserRole + 1).toString());
}

void ObjEditorWidget::onAffectSelected() {
    refreshAffectForm();
}

void ObjEditorWidget::addAffect() {
    nebbie::ObjAffect affect;
    affect.location = comboIntValue(affect_location_);
    affect.modifier = affect_modifier_->value();
    const QString label = QString("%1 %+2")
                              .arg(QString::fromStdString(nebbie::obj_apply_type_name(affect.location)))
                              .arg(affect.modifier);
    auto* item = new QListWidgetItem(label);
    item->setData(Qt::UserRole, affect.location);
    item->setData(Qt::UserRole + 1, affect.modifier);
    affect_list_->addItem(item);
    affect_list_->setCurrentItem(item);
}

void ObjEditorWidget::removeAffect() {
    const int row = affect_list_->currentRow();
    if (row < 0) {
        return;
    }
    delete affect_list_->takeItem(row);
    refreshAffectForm();
}

void ObjEditorWidget::refreshAffectForm() {
    const auto* item = affect_list_->currentItem();
    if (!item) {
        return;
    }
    setComboIntValue(affect_location_, item->data(Qt::UserRole).toInt());
    affect_modifier_->setValue(item->data(Qt::UserRole + 1).toInt());
}

void ObjEditorWidget::fillValueEnumCombo(const int slot_index, const nebbie::ObjValueEditorKind kind) {
    QComboBox* combo = nullptr;
    switch (slot_index) {
    case 0:
        combo = value_combo0_;
        break;
    case 1:
        combo = value_combo1_;
        break;
    case 2:
        combo = value_combo2_;
        break;
    case 3:
        combo = value_combo3_;
        break;
    default:
        return;
    }

    combo->clear();
    combo->setEditable(false);
    switch (kind) {
    case nebbie::ObjValueEditorKind::yes_no:
        combo->addItem("0 — no", 0);
        combo->addItem("1 — sì", 1);
        break;
    case nebbie::ObjValueEditorKind::weapon_damage_type:
        for (const auto& [value, label] : nebbie::obj_weapon_damage_type_choices()) {
            combo->addItem(QString::fromStdString(label), value);
        }
        break;
    case nebbie::ObjValueEditorKind::liquid_type:
        for (const auto& [value, label] : nebbie::obj_liquid_type_choices()) {
            combo->addItem(QString::fromStdString(label), value);
        }
        break;
    case nebbie::ObjValueEditorKind::spell_number:
        combo->setEditable(true);
        for (const auto& [value, label] : nebbie::obj_spell_number_choices()) {
            combo->addItem(QString::fromStdString(label), value);
        }
        break;
    default:
        break;
    }
}

void ObjEditorWidget::syncValueComboFromSpin(const int slot_index) {
    QSpinBox* spin = nullptr;
    QComboBox* combo = nullptr;
    switch (slot_index) {
    case 0:
        spin = value0_;
        combo = value_combo0_;
        break;
    case 1:
        spin = value1_;
        combo = value_combo1_;
        break;
    case 2:
        spin = value2_;
        combo = value_combo2_;
        break;
    case 3:
        spin = value3_;
        combo = value_combo3_;
        break;
    default:
        return;
    }
    if (!combo->isVisible()) {
        return;
    }
    const int value = spin->value();
    const int index = combo->findData(value);
    if (index >= 0) {
        combo->setCurrentIndex(index);
    }
}

void ObjEditorWidget::syncValueSpinFromCombo(const int slot_index) {
    QSpinBox* spin = nullptr;
    QComboBox* combo = nullptr;
    switch (slot_index) {
    case 0:
        spin = value0_;
        combo = value_combo0_;
        break;
    case 1:
        spin = value1_;
        combo = value_combo1_;
        break;
    case 2:
        spin = value2_;
        combo = value_combo2_;
        break;
    case 3:
        spin = value3_;
        combo = value_combo3_;
        break;
    default:
        return;
    }
    if (!combo->isVisible()) {
        return;
    }
    spin->setValue(combo->currentData().toInt());
}

void ObjEditorWidget::updateObjectValueFields() {
    const int type_flag = comboIntValue(type_flag_);
    const nebbie::ObjValueTypeSchema schema = nebbie::obj_value_schema_for_type(type_flag);
    const bool type_changed = type_flag != last_value_type_flag_;
    last_value_type_flag_ = type_flag;

    value_legend_->setText(
        QString::fromStdString(schema.legend_it + "\n\nTipo: " + nebbie::obj_type_name(type_flag) + " (#"
                               + std::to_string(type_flag) + ")."));

    struct SlotWidgets {
        QLabel* label;
        QSpinBox* spin;
        QComboBox* combo;
        QLabel* hint;
    };
    SlotWidgets value_ui[] = {
        {value_label0_, value0_, value_combo0_, value_hint0_},
        {value_label1_, value1_, value_combo1_, value_hint1_},
        {value_label2_, value2_, value_combo2_, value_hint2_},
        {value_label3_, value3_, value_combo3_, value_hint3_},
    };

    for (int i = 0; i < 4; ++i) {
        const nebbie::ObjValueSlotSchema& slot_schema = schema.value_slots[i];
        SlotWidgets& ui = value_ui[i];
        ui.label->setText(QString::fromStdString(slot_schema.label_it));
        ui.spin->setToolTip(QString::fromStdString(slot_schema.tooltip_it));
        ui.combo->setToolTip(QString::fromStdString(slot_schema.tooltip_it));

        const bool used = slot_schema.editor != nebbie::ObjValueEditorKind::unused;
        ui.spin->setEnabled(used);
        if (used && slot_schema.max_value >= slot_schema.min_value) {
            ui.spin->setRange(slot_schema.min_value, slot_schema.max_value);
        }

        const bool show_combo = used && slot_schema.editor != nebbie::ObjValueEditorKind::integer;
        ui.combo->setVisible(show_combo);
        if (type_changed && show_combo) {
            fillValueEnumCombo(i, slot_schema.editor);
        }

        if (show_combo) {
            const QSignalBlocker combo_block(ui.combo);
            syncValueComboFromSpin(i);
        }

        const std::string hint = nebbie::obj_value_slot_display_hint(type_flag, i, ui.spin->value());
        if (!hint.empty() && slot_schema.editor != nebbie::ObjValueEditorKind::integer
            && slot_schema.editor != nebbie::ObjValueEditorKind::unused) {
            ui.hint->setText(QString::fromUtf8("→ ") + QString::fromStdString(hint));
        } else if (!used) {
            ui.hint->setText(QString::fromUtf8("Non usato per questo tipo."));
        } else {
            ui.hint->clear();
        }
    }
}
