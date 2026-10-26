#include "obj_editor_widget.hpp"

#include "combo_group_util.hpp"
#include "flag_group_widget.hpp"
#include "mud_color_widgets.hpp"
#include "mud_editor_fields.hpp"
#include "nebbie/mob_catalog.hpp"
#include "nebbie/obj_catalog.hpp"
#include "nebbie/obj_value_catalog.hpp"
#include "room_text_monitors.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QSpinBox>
#include <QSplitter>
#include <QStackedWidget>
#include <QTabWidget>
#include <QTextEdit>
#include <QVBoxLayout>

#include <string>
#include <vector>

namespace {

void configureLineField(QLineEdit* field) {
    field->setMinimumWidth(280);
    field->setMinimumHeight(28);
    field->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void configureShortTextField(nebbie::qt::MudColorTextEdit* field) {
    field->setMinimumHeight(44);
    field->setMaximumHeight(72);
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
    label->setStyleSheet(QStringLiteral("color: palette(mid);"));
    return label;
}

void fillCombo(QComboBox* combo, const std::vector<std::pair<int, std::string>>& choices) {
    combo->clear();
    for (const auto& [value, label] : choices) {
        combo->addItem(QString::fromStdString(label), value);
    }
}

void fillAffectTypeCombo(QComboBox* combo) {
    const std::vector<nebbie::qt::ComboIntSection> sections = {
        {"Attributi", {1, 2, 3, 4, 5, 6}},
        {"Vitalita' e regen", {12, 13, 14, 50, 51, 52}},
        {"Combattimento (hit/dam/SP)", {17, 18, 19, 40, 65, 66, 41, 42, 43}},
        {"Salvezze", {20, 21, 22, 23, 24, 25}},
        {"Difese e affect (bit)", {26, 27, 28, 29}},
        {"Incantesimi su oggetto", {30, 31}},
        {"Skill %", {32, 33, 34, 35, 36, 37, 38, 39, 47}},
        {"Slayer", {48, 49}},
        {"Fisico (raro)", {9, 10, 11}},
    };
    nebbie::qt::fillGroupedIntCombo(combo, sections, [](const int location) {
        return QString::fromStdString(nebbie::obj_apply_type_label(location));
    });
}

void fillObjTypeCombo(QComboBox* combo) {
    const std::vector<nebbie::qt::ComboIntSection> sections = {
        {"Luce / scritto / chiave", {1, 2, 16, 18, 21, 24}},
        {"Armi e proiettili", {5, 6, 7}},
        {"Armature e indossabili", {9, 11}},
        {"Pozioni / cibo / bevande", {10, 19, 17}},
        {"Contenitori e denaro", {15, 20}},
        {"Ambiente / risorse", {23, 25, 26, 27, 28, 29, 30}},
        {"Altro", {0, 3, 4, 8, 12, 13, 14, 22}},
    };
    nebbie::qt::fillGroupedIntCombo(combo, sections, [](const int type_flag) {
        return QString::fromStdString(nebbie::obj_type_name(type_flag));
    });
}

void fillRaceCombo(QComboBox* combo) {
    // Indici allineati a kRaceNames in mob_catalog.cpp.
    const std::vector<nebbie::qt::ComboIntSection> sections = {
        {"Umanoidi base", {0, 1, 2, 3, 4, 5, 36, 74, 75, 76, 77, 79, 80, 82, 83, 84, 85}},
        {"Mostri e creature",
         {6, 7, 8, 11, 12, 13, 14, 15, 16, 18, 19, 20, 22, 23, 24, 25, 30, 31, 32, 33, 34, 35, 37,
          38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 78, 81}},
        {"Giganti", {17, 50, 51, 52, 53, 54, 55}},
        {"Draghi", {9, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65}},
        {"Non-morti", {10, 66, 67, 68, 69, 70, 71, 72, 73}},
        {"Planari / elementali / divini", {21, 26, 27, 28, 29, 48, 49}},
    };
    const auto races = nebbie::mob_race_choices();
    nebbie::qt::fillGroupedIntCombo(combo, sections, [&races](const int race) {
        for (const auto& [value, label] : races) {
            if (value == race) {
                return QString::fromStdString(label);
            }
        }
        return QStringLiteral("race #%1").arg(race);
    });
}

int comboIntValue(QComboBox* combo) {
    const QVariant data = combo->currentData();
    if (!data.isValid()) {
        // Skip section headers / separators if somehow selected.
        for (int i = 0; i < combo->count(); ++i) {
            if (combo->itemData(i).isValid()) {
                combo->setCurrentIndex(i);
                return combo->itemData(i).toInt();
            }
        }
        return 0;
    }
    return data.toInt();
}

QScrollArea* wrapFlagsInScroll(QWidget* content) {
    auto* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidget(content);
    return scroll;
}

QWidget* wrapFlagGroup(FlagGroupWidget* flags, QWidget* parent, const int viewport_height = 140) {
    auto* wrap = new QWidget(parent);
    auto* layout = new QVBoxLayout(wrap);
    layout->setContentsMargins(0, 0, 0, 0);
    auto* scroll = new QScrollArea(wrap);
    scroll->setWidgetResizable(true);
    scroll->setFixedHeight(viewport_height);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setFrameShape(QFrame::StyledPanel);
    scroll->setWidget(flags);
    layout->addWidget(scroll);
    wrap->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    return wrap;
}

/** Compact wrap for small flag sets (e.g. Align slayer): no tall empty scroll box. */
QWidget* wrapFlagGroupCompact(FlagGroupWidget* flags, QWidget* parent) {
    auto* wrap = new QWidget(parent);
    auto* layout = new QVBoxLayout(wrap);
    layout->setContentsMargins(0, 2, 0, 2);
    layout->setSpacing(0);
    layout->addWidget(flags);
    wrap->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    return wrap;
}

void shrinkStackedWidgetToCurrent(QStackedWidget* stack) {
    if (!stack) {
        return;
    }
    for (int i = 0; i < stack->count(); ++i) {
        QWidget* page = stack->widget(i);
        if (!page) {
            continue;
        }
        const bool current = i == stack->currentIndex();
        page->setSizePolicy(current ? QSizePolicy::Preferred : QSizePolicy::Ignored,
                            current ? QSizePolicy::Preferred : QSizePolicy::Ignored);
        if (!current) {
            page->setMaximumHeight(0);
        } else {
            page->setMaximumHeight(QWIDGETSIZE_MAX);
        }
    }
    stack->adjustSize();
}

} // namespace

void ObjEditorWidget::setComboIntValue(QComboBox* combo, const int value) const {
    const int index = combo->findData(value);
    if (index >= 0) {
        combo->setCurrentIndex(index);
        nebbie::qt::syncGroupedComboEditText(combo);
        return;
    }
    // Prefer first item with valid userData (skip Affect section headers).
    for (int i = 0; i < combo->count(); ++i) {
        if (combo->itemData(i).isValid()) {
            combo->setCurrentIndex(i);
            nebbie::qt::syncGroupedComboEditText(combo);
            return;
        }
    }
    combo->setCurrentIndex(0);
}

ObjEditorWidget::ObjEditorWidget(QWidget* parent) : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    auto* tabs = new QTabWidget;

    auto* text_tab = new QWidget;
    auto* text_layout = new QVBoxLayout(text_tab);
    auto* text_form = new QFormLayout;
    english_lexicon_banner_ = nebbie::qt::makeTextMonitorLabel();
    name_ = new nebbie::qt::MudColorTextEdit;
    nebbie::qt::configureMudSingleLineField(name_);
    short_descr_ = new nebbie::qt::MudColorTextEdit;
    configureShortTextField(short_descr_);
    short_descr_->setRequireTrailingNewline(false);
    short_descr_->refreshTrailingNewlineToolTip();
    description_ = new nebbie::qt::MudColorTextEdit;
    configureTextField(description_, 72);
    description_->setRequireTrailingNewline(false);
    description_->refreshTrailingNewlineToolTip();
    description_tilde_hint_ = nebbie::qt::makeTextMonitorLabel();
    action_description_ = new nebbie::qt::MudColorTextEdit;
    configureTextField(action_description_, 56);
    action_description_->setRequireTrailingNewline(false);
    action_description_->refreshTrailingNewlineToolTip();
    action_description_->setPlaceholderText("Testo azione (es. armi/pozioni); lascia vuoto se non serve");
    action_tilde_hint_ = nebbie::qt::makeTextMonitorLabel();
    text_form->addRow(makeLegend(
        "Marker \u21b5: descrizione / azione oggetto = niente Invio finale "
        "(\u21b5 rosso in coda = toglilo; ~ sulla stessa riga). "
        "Solo la descrizione extra (sezione E) multilinea vuole un Invio finale (\u21b5 verde).",
        text_tab));
    text_form->addRow(english_lexicon_banner_);
    text_form->addRow("Nome (parole chiave~):", name_);
    text_form->addRow("Descrizione breve:", short_descr_);
    text_form->addRow("Descrizione:", description_);
    text_form->addRow("", description_tilde_hint_);
    text_form->addRow("Testo azione:", action_description_);
    text_form->addRow("", action_tilde_hint_);
    text_layout->addLayout(text_form, 1);

    auto* economy_box = new QGroupBox("Peso e prezzi");
    auto* economy_layout = new QVBoxLayout(economy_box);
    economy_layout->addWidget(makeLegend(
        "Valore = prezzo negozio (obj.cost). Affitto/giorno = costo di rent (obj.cost_per_day), "
        "come voci 8 e 9 dell'OLC.",
        economy_box));
    auto* economy_row = new QHBoxLayout;
    weight_ = new QSpinBox;
    weight_->setRange(0, 2000000000);
    cost_ = new QSpinBox;
    cost_->setRange(0, 2000000000);
    cost_per_day_ = new QSpinBox;
    cost_per_day_->setRange(0, 2000000000);
    economy_row->addWidget(new QLabel("Peso:"));
    economy_row->addWidget(weight_);
    economy_row->addWidget(new QLabel("Valore:"));
    economy_row->addWidget(cost_);
    economy_row->addWidget(new QLabel("Affitto/giorno:"));
    economy_row->addWidget(cost_per_day_);
    economy_row->addStretch();
    economy_layout->addLayout(economy_row);
    text_layout->addWidget(economy_box);
    tabs->addTab(text_tab, QStringLiteral("Name"));

    auto* type_tab = new QWidget;
    auto* type_layout = new QVBoxLayout(type_tab);
    type_layout->setSpacing(10);

    auto* type_header = new QHBoxLayout;
    type_header->addWidget(new QLabel(QStringLiteral("Type:"), type_tab));
    type_flag_ = new QComboBox(type_tab);
    type_flag_->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    type_flag_->setMinimumContentsLength(24);
    type_flag_->setMaximumWidth(420);
    fillObjTypeCombo(type_flag_);
    nebbie::qt::enableGroupedComboSearch(type_flag_, QStringLiteral("Search object type…"));
    type_header->addWidget(type_flag_, 1);
    type_header->addStretch();
    type_layout->addLayout(type_header);

    value_legend_ = new QLabel(type_tab);
    value_legend_->setWordWrap(true);
    value_legend_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    value_legend_->setStyleSheet(
        QStringLiteral("QLabel { padding: 8px 10px; background: palette(base); "
                       "border: 1px solid palette(mid); border-radius: 4px; }"));
    value_legend_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    type_layout->addWidget(value_legend_);

    auto* values_box = new QGroupBox(QStringLiteral("Values (value[0..3])"), type_tab);
    auto* values_layout = new QVBoxLayout(values_box);
    values_layout->setSpacing(6);

    auto make_value_row = [&](int index, QSpinBox*& spin, QComboBox*& combo, QLabel*& hint,
                              QLabel*& row_label) {
        auto* row = new QWidget(values_box);
        row->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
        auto* row_layout = new QHBoxLayout(row);
        row_layout->setContentsMargins(0, 0, 0, 0);
        row_layout->setSpacing(8);

        row_label = new QLabel(QStringLiteral("value[%1]").arg(index), row);
        row_label->setMinimumWidth(168);
        row_label->setMaximumWidth(200);
        row_label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);

        spin = new QSpinBox(row);
        spin->setRange(-2000000000, 2000000000);
        spin->setMinimumWidth(100);
        spin->setMaximumWidth(140);
        spin->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

        combo = new QComboBox(row);
        combo->setMinimumWidth(200);
        combo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        hint = new QLabel(row);
        hint->setMinimumWidth(72);
        hint->setStyleSheet(QStringLiteral("color: palette(mid);"));
        hint->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

        row_layout->addWidget(row_label);
        row_layout->addWidget(spin);
        row_layout->addWidget(combo, 1);
        row_layout->addWidget(hint);
        values_layout->addWidget(row);
    };

    make_value_row(0, value0_, value_combo0_, value_hint0_, value_label0_);
    make_value_row(1, value1_, value_combo1_, value_hint1_, value_label1_);
    make_value_row(2, value2_, value_combo2_, value_hint2_, value_label2_);
    make_value_row(3, value3_, value_combo3_, value_hint3_, value_label3_);

    type_layout->addWidget(values_box);
    type_layout->addStretch(1);
    tabs->addTab(type_tab, QStringLiteral("Stats"));

    auto* flags_tab = new QWidget;
    auto* flags_layout = new QVBoxLayout(flags_tab);
    wear_flags_ = new FlagGroupWidget(nebbie::obj_wear_flags(), flags_tab, 3,
                                      FlagGroupWidget::Mode::DualList);
    extra_flags_ = new FlagGroupWidget(nebbie::obj_extra_flags(), flags_tab, 3,
                                       FlagGroupWidget::Mode::DualList);
    has_extra_flags2_ = new QCheckBox("Abilita flag extra 2 (sezione F)");
    extra_flags2_panel_ = new QWidget(flags_tab);
    auto* extra_flags2_layout = new QVBoxLayout(extra_flags2_panel_);
    extra_flags2_layout->setContentsMargins(0, 0, 0, 0);
    extra_flags2_ = new FlagGroupWidget(nebbie::obj_extra_flags2(), extra_flags2_panel_, 3,
                                        FlagGroupWidget::Mode::DualList);
    extra_flags2_layout->addWidget(extra_flags2_);

    flags_layout->addWidget(new QLabel("Dove si puo' indossare (wear)"));
    flags_layout->addWidget(wear_flags_);
    flags_layout->addWidget(new QLabel("Flag extra (include ANTI-classe / allineamento)"));
    flags_layout->addWidget(makeLegend(
        "I flag ANTI-* e ONLY-* qui decidono chi puo' usare l'oggetto. "
        "Non confonderli con i messaggi sotto: quelli sono solo testo mostrato "
        "quando l'indosso e' bloccato.",
        flags_tab));
    flags_layout->addWidget(extra_flags_);
    flags_layout->addWidget(has_extra_flags2_);
    flags_layout->addWidget(extra_flags2_panel_);

    auto* forbidden_box = new QGroupBox("Messaggi se indossare e' vietato (sezione P)");
    auto* forbidden_layout = new QVBoxLayout(forbidden_box);
    forbidden_layout->addWidget(makeLegend(
        "Opzionale. Non e' un flag ANTI: sono stringhe custom mostrate al personaggio "
        "e alla stanza quando qualcuno prova a indossare l'oggetto e non puo'. "
        "Lascia vuoto se non servono.",
        forbidden_box));
    forbidden_char_ = new QLineEdit;
    forbidden_room_ = new QLineEdit;
    configureLineField(forbidden_char_);
    configureLineField(forbidden_room_);
    forbidden_char_->setPlaceholderText("Messaggio al personaggio~");
    forbidden_room_->setPlaceholderText("Messaggio alla stanza~");
    auto* forbidden_form = new QFormLayout;
    forbidden_form->addRow("Al personaggio:", forbidden_char_);
    forbidden_form->addRow("Alla stanza:", forbidden_room_);
    forbidden_layout->addLayout(forbidden_form);
    flags_layout->addWidget(forbidden_box);
    flags_layout->addStretch();
    tabs->addTab(wrapFlagsInScroll(flags_tab), QStringLiteral("Wear / Extra"));

    auto* affect_tab = new QWidget;
    auto* affect_layout = new QVBoxLayout(affect_tab);
    affect_layout->addWidget(makeLegend(
        "Affect (sezione A): lista a sinistra, editor a destra. "
        "Filtra la lista o cerca nel tipo di bonus. "
        "Aggiungi = nuova riga; Aggiorna = modifica la riga selezionata "
        "(senza Aggiorna le bozze a destra non toccano le altre righe). "
        "Max 5 affect per oggetto.",
        affect_tab));

    auto* affect_split = new QSplitter(Qt::Horizontal, affect_tab);

    auto* affect_list_panel = new QWidget(affect_tab);
    auto* affect_list_layout = new QVBoxLayout(affect_list_panel);
    affect_list_layout->setContentsMargins(0, 0, 0, 0);
    affect_list_layout->setSpacing(4);
    affect_list_filter_ = new QLineEdit(affect_list_panel);
    affect_list_filter_->setPlaceholderText(QStringLiteral("Filtra lista affect…"));
    affect_list_filter_->setClearButtonEnabled(true);
    affect_list_ = new QListWidget(affect_list_panel);
    affect_list_->setMinimumWidth(220);
    affect_list_->setAlternatingRowColors(true);
    affect_list_layout->addWidget(affect_list_filter_);
    affect_list_layout->addWidget(affect_list_, 1);
    affect_split->addWidget(affect_list_panel);

    auto* affect_editor = new QWidget;
    auto* affect_editor_layout = new QVBoxLayout(affect_editor);
    affect_editor_layout->setContentsMargins(8, 0, 0, 0);
    auto* affect_form = new QFormLayout;
    affect_location_ = new QComboBox;
    affect_location_->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    affect_location_->setMinimumContentsLength(28);
    fillAffectTypeCombo(affect_location_);
    nebbie::qt::enableGroupedComboSearch(affect_location_,
                                         QStringLiteral("Cerca tipo di bonus…"));
    affect_form->addRow("Tipo di bonus:", affect_location_);
    affect_modifier_hint_ = makeLegend("", affect_editor);
    affect_form->addRow("", affect_modifier_hint_);

    affect_modifier_stack_ = new QStackedWidget;
    affect_modifier_stack_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    affect_modifier_ = new QSpinBox;
    affect_modifier_->setRange(-2000000000, 2000000000);
    affect_stack_integer_ = affect_modifier_stack_->addWidget(affect_modifier_);

    // Flag bit: 3 colonne. Scroll solo dove la lista e' lunga (AFF ~32); gli altri
    // restano a altezza naturale cosi' non resta uno "scatolone" mezzo vuoto.
    affect_immune_flags_ = new FlagGroupWidget(nebbie::mob_immunity_flags(), nullptr, 3,
                                               FlagGroupWidget::Mode::DualList);
    affect_stack_immune_ =
        affect_modifier_stack_->addWidget(wrapFlagGroupCompact(affect_immune_flags_, affect_editor));

    affect_spell_flags_ = new FlagGroupWidget(nebbie::obj_spell_affect_flags(), nullptr, 3,
                                              FlagGroupWidget::Mode::DualList);
    affect_stack_spell_ =
        affect_modifier_stack_->addWidget(wrapFlagGroup(affect_spell_flags_, affect_editor, 168));

    affect_spell2_flags_ = new FlagGroupWidget(nebbie::obj_spell_affect2_flags(), nullptr, 3,
                                               FlagGroupWidget::Mode::DualList);
    affect_stack_spell2_ =
        affect_modifier_stack_->addWidget(wrapFlagGroupCompact(affect_spell2_flags_, affect_editor));

    affect_spell_combo_ = new QComboBox;
    affect_spell_combo_->setEditable(true);
    fillCombo(affect_spell_combo_, nebbie::obj_spell_number_choices());
    affect_stack_spell_num_ = affect_modifier_stack_->addWidget(affect_spell_combo_);

    affect_race_combo_ = new QComboBox;
    affect_race_combo_->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    affect_race_combo_->setMinimumContentsLength(22);
    fillRaceCombo(affect_race_combo_);
    nebbie::qt::enableGroupedComboSearch(affect_race_combo_, QStringLiteral("Cerca razza…"));
    affect_stack_race_ = affect_modifier_stack_->addWidget(affect_race_combo_);

    affect_sex_combo_ = new QComboBox;
    fillCombo(affect_sex_combo_, nebbie::mob_sex_choices());
    affect_stack_sex_ = affect_modifier_stack_->addWidget(affect_sex_combo_);

    affect_align_flags_ = new FlagGroupWidget(nebbie::obj_align_slayer_flags(), nullptr, 0);
    affect_stack_align_ =
        affect_modifier_stack_->addWidget(wrapFlagGroupCompact(affect_align_flags_, affect_editor));

    affect_form->addRow("Valore:", affect_modifier_stack_);
    affect_editor_layout->addLayout(affect_form);

    auto* affect_buttons = new QHBoxLayout;
    auto* affect_add = new QPushButton("Aggiungi");
    auto* affect_update = new QPushButton("Aggiorna");
    auto* affect_remove = new QPushButton("Rimuovi");
    affect_buttons->addWidget(affect_add);
    affect_buttons->addWidget(affect_update);
    affect_buttons->addWidget(affect_remove);
    affect_buttons->addStretch();
    affect_editor_layout->addLayout(affect_buttons);
    affect_editor_layout->addStretch();
    affect_split->addWidget(affect_editor);
    affect_split->setStretchFactor(0, 2);
    affect_split->setStretchFactor(1, 3);
    affect_layout->addWidget(affect_split, 1);
    tabs->addTab(affect_tab, QStringLiteral("Affects"));

    auto* extra_tab = new QWidget;
    auto* extra_layout = new QVBoxLayout(extra_tab);
    extra_layout->addWidget(makeLegend(
        "Descrizioni extra (sezione E): parole chiave per look sull'oggetto "
        "(es. look gemma). Non c'entrano con i flag ANTI ne' con i messaggi di "
        "indosso vietato (sono nel tab Flag).",
        extra_tab));
    extra_desc_list_ = new QListWidget;
    extra_desc_keyword_ = new QLineEdit;
    configureLineField(extra_desc_keyword_);
    extra_desc_description_ = new nebbie::qt::MudColorTextEdit;
    configureTextField(extra_desc_description_, 64);
    extra_desc_description_->setRequireTrailingNewline(false);
    extra_desc_description_->refreshTrailingNewlineToolTip();
    extra_desc_tilde_hint_ = nebbie::qt::makeTextMonitorLabel();
    auto* extra_desc_form = new QFormLayout;
    extra_desc_form->addRow("Parola chiave:", extra_desc_keyword_);
    extra_desc_form->addRow("Descrizione:", extra_desc_description_);
    extra_desc_form->addRow("", extra_desc_tilde_hint_);
    auto* extra_desc_buttons = new QHBoxLayout;
    auto* extra_desc_add = new QPushButton("Aggiungi");
    auto* extra_desc_update = new QPushButton("Aggiorna");
    auto* extra_desc_remove = new QPushButton("Rimuovi");
    extra_desc_buttons->addWidget(extra_desc_add);
    extra_desc_buttons->addWidget(extra_desc_update);
    extra_desc_buttons->addWidget(extra_desc_remove);
    extra_desc_buttons->addStretch();
    extra_layout->addWidget(extra_desc_list_, 1);
    extra_layout->addLayout(extra_desc_form);
    extra_layout->addLayout(extra_desc_buttons);
    tabs->addTab(extra_tab, QStringLiteral("Extras"));

    root->addWidget(tabs);

    connect(has_extra_flags2_, &QCheckBox::toggled, extra_flags2_panel_, &QWidget::setVisible);
    connect(extra_desc_list_, &QListWidget::currentRowChanged, this, [this](int) { onExtraDescSelected(); });
    connect(extra_desc_add, &QPushButton::clicked, this, &ObjEditorWidget::addExtraDesc);
    connect(extra_desc_update, &QPushButton::clicked, this, &ObjEditorWidget::updateExtraDesc);
    connect(extra_desc_remove, &QPushButton::clicked, this, &ObjEditorWidget::removeExtraDesc);
    connect(affect_list_, &QListWidget::currentRowChanged, this, [this](int) { onAffectSelected(); });
    connect(affect_list_filter_, &QLineEdit::textChanged, this, [this](const QString& text) {
        const QString needle = text.trimmed();
        for (int i = 0; i < affect_list_->count(); ++i) {
            QListWidgetItem* item = affect_list_->item(i);
            if (!item) {
                continue;
            }
            const bool visible =
                needle.isEmpty() || item->text().contains(needle, Qt::CaseInsensitive);
            item->setHidden(!visible);
        }
    });
    connect(affect_location_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int) { onAffectLocationChanged(); });
    connect(affect_add, &QPushButton::clicked, this, &ObjEditorWidget::addAffect);
    connect(affect_update, &QPushButton::clicked, this, &ObjEditorWidget::updateAffect);
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
    updateAffectModifierEditor();

    for (nebbie::qt::MudColorTextEdit* field : mudFields()) {
        if (field == nullptr) {
            continue;
        }
        connect(field, &nebbie::qt::MudColorTextEdit::storageTextChanged, this, [this]() {
            updateEnglishLexiconBanner();
            updateTildeNewlineHints();
        });
    }
    connect(extra_desc_keyword_, &QLineEdit::textChanged, this, [this]() { updateEnglishLexiconBanner(); });
    connect(forbidden_char_, &QLineEdit::textChanged, this, [this]() { updateEnglishLexiconBanner(); });
    connect(forbidden_room_, &QLineEdit::textChanged, this, [this]() { updateEnglishLexiconBanner(); });
    updateTildeNewlineHints();
}

void ObjEditorWidget::updateTildeNewlineHints() {
    description_->setRequireTrailingNewline(false);
    action_description_->setRequireTrailingNewline(false);
    nebbie::qt::syncMudParagraphTrailingNewlinePolicy(extra_desc_description_);
    nebbie::qt::updateTildeNewlineHint(description_tilde_hint_, description_);
    nebbie::qt::updateTildeNewlineHint(action_tilde_hint_, action_description_);
    nebbie::qt::updateTildeNewlineHint(extra_desc_tilde_hint_, extra_desc_description_);
}

void ObjEditorWidget::updateEnglishLexiconBanner() {
    std::string blob;
    for (nebbie::qt::MudColorTextEdit* field : mudFields()) {
        if (field == nullptr) {
            continue;
        }
        blob += field->storageText().toStdString();
        blob.push_back(' ');
    }
    if (extra_desc_keyword_ != nullptr) {
        blob += extra_desc_keyword_->text().toStdString();
        blob.push_back(' ');
    }
    if (forbidden_char_ != nullptr) {
        blob += forbidden_char_->text().toStdString();
        blob.push_back(' ');
    }
    if (forbidden_room_ != nullptr) {
        blob += forbidden_room_->text().toStdString();
    }
    nebbie::qt::updateEnglishLexiconMonitor(english_lexicon_banner_, blob);
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

    replaceAffectList(obj.affects);

    extra_desc_list_->clear();
    for (const auto& extra : obj.extra_descs) {
        const QString label = QString::fromStdString(extra.keyword);
        auto* item = new QListWidgetItem(label.isEmpty() ? "(senza parola chiave)" : label);
        item->setData(Qt::UserRole, QString::fromStdString(extra.keyword));
        item->setData(Qt::UserRole + 1, QString::fromStdString(extra.description));
        extra_desc_list_->addItem(item);
    }

    forbidden_char_->setText(QString::fromStdString(obj.forbidden_char));
    forbidden_room_->setText(QString::fromStdString(obj.forbidden_room));
    updateEnglishLexiconBanner();
    updateTildeNewlineHints();
}

void ObjEditorWidget::saveToObject(nebbie::GameObject& obj) {
    if (extra_desc_list_->currentItem() != nullptr) {
        const QString keyword = extra_desc_keyword_->text().trimmed();
        const QString description = extra_desc_description_->storageText();
        if (!(keyword.isEmpty() && description.trimmed().isEmpty())) {
            updateExtraDesc();
        }
    }
    // Non chiamare updateAffect() qui: i campi a destra possono essere una bozza
    // per "Aggiungi"; riscrivere la riga selezionata corromperebbe gli affect.

    obj.name = name_->storageText().toStdString();
    obj.short_descr = short_descr_->storageText().toStdString();
    obj.description = description_->storageText().toStdString();
    obj.action_description = action_description_->storageText().toStdString();

    obj.type_flag = comboIntValue(type_flag_);
    obj.value[0] = value0_->value();
    obj.value[1] = value1_->value();
    obj.value[2] = value2_->value();
    obj.value[3] = value3_->value();

    // Flag tickbox → OR di bit (long). In file: potenze di due separate da | .
    obj.wear_flags = wear_flags_->value();
    obj.extra_flags = extra_flags_->value();
    obj.has_extra_flags2 = has_extra_flags2_->isChecked();
    obj.extra_flags2 = obj.has_extra_flags2 ? extra_flags2_->value() : 0;

    obj.weight = weight_->value();
    obj.cost = cost_->value();
    obj.cost_per_day = cost_per_day_->value();

    // Lista UI cosi' com'e': merge/cap a 5 solo in scrittura file (.obj).
    obj.affects = affectsFromList();

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
    auto* item = new QListWidgetItem(keyword.isEmpty() ? "(senza parola chiave)" : keyword);
    item->setData(Qt::UserRole, keyword);
    item->setData(Qt::UserRole + 1, description);
    extra_desc_list_->addItem(item);
    extra_desc_list_->setCurrentItem(item);
    extra_desc_keyword_->clear();
    extra_desc_description_->setStorageText({});
}

void ObjEditorWidget::updateExtraDesc() {
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
    item->setText(keyword.isEmpty() ? QStringLiteral("(senza parola chiave)") : keyword);
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

QString ObjEditorWidget::affectListLabel(const int location, const int modifier) const {
    return QString::fromStdString(nebbie::format_obj_affect_label(location, modifier));
}

void ObjEditorWidget::writeAffectListItem(QListWidgetItem* item, const int location,
                                         const int modifier) const {
    item->setData(Qt::UserRole, location);
    item->setData(Qt::UserRole + 1, modifier);
    item->setText(affectListLabel(location, modifier));
    item->setToolTip(QStringLiteral("%1 (#%2) = %3")
                         .arg(QString::fromStdString(nebbie::obj_apply_type_name(location)))
                         .arg(location)
                         .arg(modifier));
}

void ObjEditorWidget::ensureAffectLocationInCombo(const int location) {
    if (affect_location_->findData(location) >= 0) {
        return;
    }
    const QString clean =
        QString::fromStdString(nebbie::obj_apply_type_label(location) + " (raro)");
    const int row = affect_location_->count();
    affect_location_->addItem(QStringLiteral("    ") + clean, location);
    affect_location_->setItemData(row, clean, Qt::UserRole + 1);
}

void ObjEditorWidget::updateAffectModifierEditor() {
    const int location = comboIntValue(affect_location_);
    const nebbie::ObjAffectModifierKind kind = nebbie::obj_affect_modifier_kind(location);
    QString hint;
    switch (kind) {
    case nebbie::ObjAffectModifierKind::immunity_bits:
        affect_modifier_stack_->setCurrentIndex(affect_stack_immune_);
        hint = QStringLiteral("Seleziona una o piu' immunita'/resistenze. "
                              "Vale per Immunita', Suscettibilita' e Resistenza meta.");
        break;
    case nebbie::ObjAffectModifierKind::spell_affect_bits:
        affect_modifier_stack_->setCurrentIndex(affect_stack_spell_);
        hint = QStringLiteral("Flag affect magici (AFF) applicati indossando l'oggetto.");
        break;
    case nebbie::ObjAffectModifierKind::spell_affect2_bits:
        affect_modifier_stack_->setCurrentIndex(affect_stack_spell2_);
        hint = QStringLiteral("Flag affect secondari (AFF2).");
        break;
    case nebbie::ObjAffectModifierKind::spell_number:
        affect_modifier_stack_->setCurrentIndex(affect_stack_spell_num_);
        hint = QStringLiteral("Scegli l'incantesimo dal menu (o digita il numero spell).");
        break;
    case nebbie::ObjAffectModifierKind::race:
        affect_modifier_stack_->setCurrentIndex(affect_stack_race_);
        hint = QStringLiteral("Razza bersaglio del race-slayer.");
        break;
    case nebbie::ObjAffectModifierKind::sex:
        affect_modifier_stack_->setCurrentIndex(affect_stack_sex_);
        hint = QStringLiteral("Sesso risultante (additivo rispetto al personaggio).");
        break;
    case nebbie::ObjAffectModifierKind::align_bits:
        affect_modifier_stack_->setCurrentIndex(affect_stack_align_);
        hint = QStringLiteral("Allineamenti colpiti dall'align-slayer (si possono combinare).");
        break;
    case nebbie::ObjAffectModifierKind::integer:
    default:
        affect_modifier_stack_->setCurrentIndex(affect_stack_integer_);
        hint = QStringLiteral("Modificatore numerico (positivo o negativo).");
        break;
    }
    shrinkStackedWidgetToCurrent(affect_modifier_stack_);
    affect_modifier_hint_->setText(hint);
}

int ObjEditorWidget::readAffectModifierFromEditor() const {
    const int location = comboIntValue(affect_location_);
    switch (nebbie::obj_affect_modifier_kind(location)) {
    case nebbie::ObjAffectModifierKind::immunity_bits:
        return static_cast<int>(affect_immune_flags_->value());
    case nebbie::ObjAffectModifierKind::spell_affect_bits:
        return static_cast<int>(affect_spell_flags_->value());
    case nebbie::ObjAffectModifierKind::spell_affect2_bits:
        return static_cast<int>(affect_spell2_flags_->value());
    case nebbie::ObjAffectModifierKind::align_bits:
        return static_cast<int>(affect_align_flags_->value());
    case nebbie::ObjAffectModifierKind::spell_number: {
        const QVariant data = affect_spell_combo_->currentData();
        if (data.isValid()) {
            return data.toInt();
        }
        bool ok = false;
        const int typed = affect_spell_combo_->currentText().trimmed().section(' ', 0, 0).toInt(&ok);
        return ok ? typed : 0;
    }
    case nebbie::ObjAffectModifierKind::race:
        return comboIntValue(affect_race_combo_);
    case nebbie::ObjAffectModifierKind::sex:
        return comboIntValue(affect_sex_combo_);
    case nebbie::ObjAffectModifierKind::integer:
    default:
        return affect_modifier_->value();
    }
}

void ObjEditorWidget::writeAffectModifierToEditor(const int modifier) {
    const int location = comboIntValue(affect_location_);
    switch (nebbie::obj_affect_modifier_kind(location)) {
    case nebbie::ObjAffectModifierKind::immunity_bits:
        affect_immune_flags_->setValue(modifier);
        break;
    case nebbie::ObjAffectModifierKind::spell_affect_bits:
        affect_spell_flags_->setValue(modifier);
        break;
    case nebbie::ObjAffectModifierKind::spell_affect2_bits:
        affect_spell2_flags_->setValue(modifier);
        break;
    case nebbie::ObjAffectModifierKind::align_bits:
        affect_align_flags_->setValue(modifier);
        break;
    case nebbie::ObjAffectModifierKind::spell_number:
        setComboIntValue(affect_spell_combo_, modifier);
        if (affect_spell_combo_->currentData().toInt() != modifier) {
            affect_spell_combo_->setEditText(QString::number(modifier));
        }
        break;
    case nebbie::ObjAffectModifierKind::race:
        setComboIntValue(affect_race_combo_, modifier);
        break;
    case nebbie::ObjAffectModifierKind::sex:
        setComboIntValue(affect_sex_combo_, modifier);
        break;
    case nebbie::ObjAffectModifierKind::integer:
    default:
        affect_modifier_->setValue(modifier);
        break;
    }
}

void ObjEditorWidget::onAffectLocationChanged() {
    updateAffectModifierEditor();
    // Cambio tipo manuale: azzera il valore (evita bit/numero della riga precedente).
    writeAffectModifierToEditor(0);
}

void ObjEditorWidget::onAffectSelected() {
    refreshAffectForm();
}

void ObjEditorWidget::addAffect() {
    const int location = comboIntValue(affect_location_);
    const int modifier = readAffectModifierFromEditor();
    if (location == 0 && modifier == 0) {
        return;
    }
    if (affect_list_->count() >= nebbie::MAX_OBJ_AFFECT) {
        affect_modifier_hint_->setText(
            QStringLiteral("Il mud carica al massimo %1 affect (A) per oggetto. "
                           "Rimuovi una riga oppure unisci i bit nello stesso tipo.")
                .arg(nebbie::MAX_OBJ_AFFECT));
        return;
    }
    auto* item = new QListWidgetItem;
    writeAffectListItem(item, location, modifier);
    {
        // Blocca refresh mentre aggiungiamo: la selezione non deve riscrivere altre righe.
        const QSignalBlocker blocker(affect_list_);
        affect_list_->addItem(item);
        affect_list_->setCurrentItem(item);
    }
    refreshAffectForm();
}

void ObjEditorWidget::updateAffect() {
    auto* item = affect_list_->currentItem();
    if (!item) {
        return;
    }
    writeAffectListItem(item, comboIntValue(affect_location_), readAffectModifierFromEditor());
}

void ObjEditorWidget::removeAffect() {
    const int row = affect_list_->currentRow();
    if (row < 0) {
        return;
    }
    delete affect_list_->takeItem(row);
    refreshAffectForm();
}

std::vector<nebbie::ObjAffect> ObjEditorWidget::affectsFromList() const {
    std::vector<nebbie::ObjAffect> affects;
    affects.reserve(static_cast<std::size_t>(affect_list_->count()));
    for (int i = 0; i < affect_list_->count(); ++i) {
        const auto* item = affect_list_->item(i);
        nebbie::ObjAffect affect;
        affect.location = item->data(Qt::UserRole).toInt();
        affect.modifier = item->data(Qt::UserRole + 1).toInt();
        affects.push_back(affect);
    }
    return affects;
}

void ObjEditorWidget::replaceAffectList(const std::vector<nebbie::ObjAffect>& affects) {
    const QSignalBlocker blocker(affect_list_);
    affect_list_->clear();
    for (const auto& affect : affects) {
        auto* item = new QListWidgetItem;
        writeAffectListItem(item, affect.location, affect.modifier);
        affect_list_->addItem(item);
    }
    if (affect_list_->count() > 0) {
        affect_list_->setCurrentRow(0);
    }
    refreshAffectForm();
}

void ObjEditorWidget::refreshAffectForm() {
    const auto* item = affect_list_->currentItem();
    if (!item) {
        return;
    }
    const int location = item->data(Qt::UserRole).toInt();
    const QSignalBlocker location_block(affect_location_);
    ensureAffectLocationInCombo(location);
    setComboIntValue(affect_location_, location);
    updateAffectModifierEditor();
    writeAffectModifierToEditor(item->data(Qt::UserRole + 1).toInt());
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
        combo->addItem("0 — No", 0);
        combo->addItem("1 — Yes", 1);
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

    value_legend_->setText(QStringLiteral("%1\n%2 (#%3)")
                               .arg(QString::fromStdString(schema.legend_it))
                               .arg(QString::fromStdString(nebbie::obj_type_name(type_flag)))
                               .arg(type_flag));

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
        ui.label->setToolTip(QString::fromStdString(slot_schema.tooltip_it));
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
        if (!hint.empty() && show_combo) {
            ui.hint->setText(QStringLiteral("= %1").arg(QString::fromStdString(hint)));
            ui.hint->show();
        } else {
            ui.hint->clear();
            ui.hint->hide();
        }
    }
}
