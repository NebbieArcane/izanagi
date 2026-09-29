#pragma once

#include "mud_editor_fields.hpp"
#include "nebbie/types.hpp"

#include <QWidget>

#include <vector>

class QCheckBox;
class QComboBox;
class QSpinBox;
class FlagGroupWidget;

namespace nebbie::qt {
class MudColorTextEdit;
}

class MobEditorWidget : public QWidget {
    Q_OBJECT

public:
    explicit MobEditorWidget(QWidget* parent = nullptr);

    void loadFromMobile(const nebbie::Mobile& mob);
    void saveToMobile(nebbie::Mobile& mob) const;

    void setMaxLineLength(int max_length);
    void setShowColorCodes(bool show);
    nebbie::qt::MudColorTextEdit* focusedMudField() const;
    void focusPrimaryTextField();

private:
    void updateTypeDependentFields();
    void applyMudFieldSettings();
    nebbie::qt::MudFieldList mudFields() const;

    int max_line_length_ = 0;
    bool show_color_codes_ = false;

    nebbie::qt::MudColorTextEdit* name_ = nullptr;
    nebbie::qt::MudColorTextEdit* short_descr_ = nullptr;
    nebbie::qt::MudColorTextEdit* long_descr_ = nullptr;
    nebbie::qt::MudColorTextEdit* description_ = nullptr;

    QComboBox* mobtype_ = nullptr;
    QSpinBox* mult_att_ = nullptr;
    QSpinBox* level_ = nullptr;
    QSpinBox* hitroll_ = nullptr;
    QSpinBox* ac_ = nullptr;
    QSpinBox* hit_bonus_ = nullptr;
    QSpinBox* hit_num_ = nullptr;
    QSpinBox* hit_size_ = nullptr;
    QSpinBox* hit_plus_ = nullptr;
    QSpinBox* dam_num_ = nullptr;
    QSpinBox* dam_size_ = nullptr;
    QSpinBox* dam_plus_ = nullptr;
    QWidget* hit_dice_row_ = nullptr;
    QWidget* hit_bonus_row_ = nullptr;

    QSpinBox* alignment_ = nullptr;
    QSpinBox* gold_ = nullptr;
    QSpinBox* exp_ = nullptr;
    QCheckBox* extended_gold_ = nullptr;
    QComboBox* race_ = nullptr;
    QWidget* race_row_ = nullptr;

    QComboBox* position_ = nullptr;
    QComboBox* default_pos_ = nullptr;
    QComboBox* sex_ = nullptr;
    QCheckBox* extended_sex_ = nullptr;
    QWidget* immunity_panel_ = nullptr;

    FlagGroupWidget* act_flags_ = nullptr;
    FlagGroupWidget* affected_flags_ = nullptr;
    FlagGroupWidget* immune_flags_ = nullptr;
    FlagGroupWidget* meta_immune_flags_ = nullptr;
    FlagGroupWidget* susceptible_flags_ = nullptr;

    nebbie::qt::MudColorTextEdit* sounds_ = nullptr;
    nebbie::qt::MudColorTextEdit* distant_sounds_ = nullptr;
    nebbie::qt::MudColorTextEdit* extra_sounds_ = nullptr;
    QWidget* sounds_panel_ = nullptr;
};
