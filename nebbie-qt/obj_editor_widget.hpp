#pragma once

#include "mud_editor_fields.hpp"
#include "nebbie/obj_value_catalog.hpp"
#include "nebbie/types.hpp"

#include <QWidget>

#include <vector>

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QSpinBox;
class QStackedWidget;
class FlagGroupWidget;

namespace nebbie::qt {
class MudColorTextEdit;
}

class ObjEditorWidget : public QWidget {
    Q_OBJECT

public:
    explicit ObjEditorWidget(QWidget* parent = nullptr);

    void loadFromObject(const nebbie::GameObject& obj);
    void saveToObject(nebbie::GameObject& obj);

    void setMaxLineLength(int max_length);
    void setShowColorCodes(bool show);
    nebbie::qt::MudColorTextEdit* focusedMudField() const;
    void focusPrimaryTextField();

private slots:
    void onExtraDescSelected();
    void addExtraDesc();
    void updateExtraDesc();
    void removeExtraDesc();
    void onAffectSelected();
    void onAffectLocationChanged();
    void addAffect();
    void updateAffect();
    void removeAffect();

private:
    void refreshExtraDescForm();
    void refreshAffectForm();
    void setComboIntValue(QComboBox* combo, int value) const;
    void applyMudFieldSettings();
    void updateEnglishLexiconBanner();
    void updateTildeNewlineHints();
    nebbie::qt::MudFieldList mudFields() const;
    void updateObjectValueFields();
    void syncValueComboFromSpin(int slot_index);
    void syncValueSpinFromCombo(int slot_index);
    void fillValueEnumCombo(int slot_index, nebbie::ObjValueEditorKind kind);
    void updateAffectModifierEditor();
    void ensureAffectLocationInCombo(int location);
    int readAffectModifierFromEditor() const;
    void writeAffectModifierToEditor(int modifier);
    QString affectListLabel(int location, int modifier) const;
    std::vector<nebbie::ObjAffect> affectsFromList() const;
    void replaceAffectList(const std::vector<nebbie::ObjAffect>& affects);
    void writeAffectListItem(QListWidgetItem* item, int location, int modifier) const;

    int max_line_length_ = 0;
    bool show_color_codes_ = false;

    QLabel* english_lexicon_banner_ = nullptr;
    nebbie::qt::MudColorTextEdit* name_ = nullptr;
    nebbie::qt::MudColorTextEdit* short_descr_ = nullptr;
    nebbie::qt::MudColorTextEdit* description_ = nullptr;
    QLabel* description_tilde_hint_ = nullptr;
    nebbie::qt::MudColorTextEdit* action_description_ = nullptr;
    QLabel* action_tilde_hint_ = nullptr;

    QComboBox* type_flag_ = nullptr;
    QLabel* value_legend_ = nullptr;
    QSpinBox* value0_ = nullptr;
    QSpinBox* value1_ = nullptr;
    QSpinBox* value2_ = nullptr;
    QSpinBox* value3_ = nullptr;
    QComboBox* value_combo0_ = nullptr;
    QComboBox* value_combo1_ = nullptr;
    QComboBox* value_combo2_ = nullptr;
    QComboBox* value_combo3_ = nullptr;
    QLabel* value_hint0_ = nullptr;
    QLabel* value_hint1_ = nullptr;
    QLabel* value_hint2_ = nullptr;
    QLabel* value_hint3_ = nullptr;
    QLabel* value_label0_ = nullptr;
    QLabel* value_label1_ = nullptr;
    QLabel* value_label2_ = nullptr;
    QLabel* value_label3_ = nullptr;
    int last_value_type_flag_ = -1;

    FlagGroupWidget* wear_flags_ = nullptr;
    FlagGroupWidget* extra_flags_ = nullptr;
    QCheckBox* has_extra_flags2_ = nullptr;
    FlagGroupWidget* extra_flags2_ = nullptr;
    QWidget* extra_flags2_panel_ = nullptr;

    QSpinBox* weight_ = nullptr;
    QSpinBox* cost_ = nullptr;
    QSpinBox* cost_per_day_ = nullptr;

    QListWidget* affect_list_ = nullptr;
    QComboBox* affect_location_ = nullptr;
    QLabel* affect_modifier_hint_ = nullptr;
    QStackedWidget* affect_modifier_stack_ = nullptr;
    QSpinBox* affect_modifier_ = nullptr;
    FlagGroupWidget* affect_immune_flags_ = nullptr;
    FlagGroupWidget* affect_spell_flags_ = nullptr;
    FlagGroupWidget* affect_spell2_flags_ = nullptr;
    FlagGroupWidget* affect_align_flags_ = nullptr;
    QComboBox* affect_spell_combo_ = nullptr;
    QComboBox* affect_race_combo_ = nullptr;
    QComboBox* affect_sex_combo_ = nullptr;
    int affect_stack_integer_ = 0;
    int affect_stack_immune_ = 1;
    int affect_stack_spell_ = 2;
    int affect_stack_spell2_ = 3;
    int affect_stack_spell_num_ = 4;
    int affect_stack_race_ = 5;
    int affect_stack_sex_ = 6;
    int affect_stack_align_ = 7;

    QListWidget* extra_desc_list_ = nullptr;
    QLineEdit* extra_desc_keyword_ = nullptr;
    nebbie::qt::MudColorTextEdit* extra_desc_description_ = nullptr;
    QLabel* extra_desc_tilde_hint_ = nullptr;

    QLineEdit* forbidden_char_ = nullptr;
    QLineEdit* forbidden_room_ = nullptr;
};
