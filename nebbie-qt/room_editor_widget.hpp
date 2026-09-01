#pragma once

#include "nebbie/types.hpp"

#include <QWidget>

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QSpinBox;
class QFrame;
class FlagGroupWidget;

namespace nebbie::qt {
class MudColorTextEdit;
}

class RoomEditorWidget : public QWidget {
    Q_OBJECT

public:
    explicit RoomEditorWidget(QWidget* parent = nullptr);
    ~RoomEditorWidget() override;

    void loadFromRoom(const nebbie::Room& room);
    void saveToRoom(nebbie::Room& room) const;
    void setMaxLineLength(int max_length);
    void setShowColorCodes(bool show);
    bool showColorCodes() const { return show_color_codes_; }

    nebbie::qt::MudColorTextEdit* focusedMudField() const;

    long selectedExitToRoom() const;
    int selectedExitDirection() const;
    void focusExitTab(int direction);

signals:
    void alignExitLabelRequested();

private slots:
    void onExtraDescSelected();
    void addExtraDesc();
    void removeExtraDesc();
    void onExitSelected();
    void addOrUpdateExit();
    void removeExit();
    void updateTextMonitors();

private:
    void updateConditionalFields();
    void refreshExtraDescForm();
    void refreshExitForm(int row = -1);
    void setComboIntValue(QComboBox* combo, int value) const;
    void applyColorSettingsToFields();
    void hookMudField(nebbie::qt::MudColorTextEdit* field);
    void scrollToSection(QWidget* section);

    int max_line_length_ = 0;
    bool show_color_codes_ = false;
    bool loading_ = false;
    QWidget* text_section_ = nullptr;
    QWidget* sector_section_ = nullptr;
    QWidget* teleport_section_ = nullptr;
    QWidget* environment_section_ = nullptr;
    QWidget* extra_section_ = nullptr;
    QWidget* exits_section_ = nullptr;

    nebbie::qt::MudColorTextEdit* name_ = nullptr;
    QLabel* name_line_info_ = nullptr;
    QLabel* name_ascii_info_ = nullptr;
    nebbie::qt::MudColorTextEdit* description_ = nullptr;
    QLabel* description_line_info_ = nullptr;
    QLabel* description_ascii_info_ = nullptr;

    QComboBox* sector_type_ = nullptr;
    FlagGroupWidget* room_flags_ = nullptr;

    QSpinBox* tele_time_ = nullptr;
    QSpinBox* tele_targ_ = nullptr;
    QSpinBox* tele_mask_ = nullptr;
    QSpinBox* tele_cnt_ = nullptr;

    QWidget* river_panel_ = nullptr;
    QSpinBox* river_speed_ = nullptr;
    QSpinBox* river_dir_ = nullptr;
    QWidget* moblim_panel_ = nullptr;
    QSpinBox* moblim_ = nullptr;

    QLineEdit* bright_at_night_ = nullptr;
    QLineEdit* bright_at_day_ = nullptr;

    QListWidget* extra_desc_list_ = nullptr;
    QLineEdit* extra_desc_keyword_ = nullptr;
    nebbie::qt::MudColorTextEdit* extra_desc_description_ = nullptr;
    QLabel* extra_desc_line_info_ = nullptr;
    QLabel* extra_desc_ascii_info_ = nullptr;

    QListWidget* exit_list_ = nullptr;
    QComboBox* exit_direction_ = nullptr;
    QSpinBox* exit_to_room_ = nullptr;
    nebbie::qt::MudColorTextEdit* exit_description_ = nullptr;
    QLabel* exit_line_info_ = nullptr;
    QLabel* exit_ascii_info_ = nullptr;
    QLineEdit* exit_keyword_ = nullptr;
    FlagGroupWidget* exit_flags_ = nullptr;
    QSpinBox* exit_key_ = nullptr;
    QSpinBox* exit_open_cmd_ = nullptr;
};
