#pragma once

#include "nebbie/types.hpp"
#include "nebbie/world.hpp"

#include <QWidget>

#include <optional>
#include <vector>

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QSpinBox;
class QFrame;
class QTabWidget;
class FlagGroupWidget;

namespace nebbie::qt {
class MudColorTextEdit;
}

class RoomEditorWidget : public QWidget {
    Q_OBJECT

public:
    explicit RoomEditorWidget(QWidget* parent = nullptr);
    ~RoomEditorWidget() override;

    void setWorld(nebbie::World* world);
    void loadFromRoom(const nebbie::Room& room);
    void saveToRoom(nebbie::Room& room);
    void setMaxLineLength(int max_length);
    void setShowColorCodes(bool show);
    bool showColorCodes() const { return show_color_codes_; }
    void refreshSummaryFromWorld();

    nebbie::qt::MudColorTextEdit* focusedMudField() const;
    /** Accents/curly apostrophes → mud ASCII in all room text fields. Returns true if anything changed. */
    bool normalizeMudAsciiAllFields();

    long selectedExitToRoom() const;
    int selectedExitDirection() const;
    void focusExitTab(int direction);
    long currentRoomVnum() const { return current_vnum_; }

signals:
    void alignExitLabelRequested();
    void gotoRoomRequested(long vnum);
    void gotoMobRequested(long vnum);
    void gotoObjectRequested(long vnum);
    void zoneResetsModified(int zone_num);

private slots:
    void onExtraDescSelected();
    void addExtraDesc();
    void updateExtraDesc();
    void removeExtraDesc();
    void onExitSelected();
    void addOrUpdateExit();
    void removeExit();
    void updateTextMonitors();
    void onEnteredFromGoto();
    void onReferenceGoto();
    void onAddExitFromSummary();
    void onAddMobReset();
    void onAddObjectReset();
    void onRemoveReset();
    void onEquipMob();
    void onGiveToMob();

private:
    void updateConditionalFields();
    void refreshExtraDescForm();
    void refreshExitForm(int row = -1);
    void commitPendingExtraDesc();
    void commitPendingExit();
    void setComboIntValue(QComboBox* combo, int value) const;
    void applyColorSettingsToFields();
    void hookMudField(nebbie::qt::MudColorTextEdit* field);
    void scrollToSection(QWidget* section);
    void rebuildSummaryLists();
    void syncSummaryHeaderFromEditors();
    int currentZoneNum() const;
    std::optional<std::size_t> selectedResetIndex() const;
    std::optional<std::size_t> selectedMobResetIndex() const;

    int max_line_length_ = 0;
    bool show_color_codes_ = false;
    bool loading_ = false;
    long current_vnum_ = 0;
    nebbie::World* world_ = nullptr;

    QTabWidget* room_tabs_ = nullptr;
    QWidget* summary_section_ = nullptr;
    QWidget* text_section_ = nullptr;
    QWidget* sector_section_ = nullptr;
    QWidget* teleport_section_ = nullptr;
    QWidget* environment_section_ = nullptr;
    QWidget* extra_section_ = nullptr;
    QWidget* exits_section_ = nullptr;

    QLineEdit* summary_vnum_ = nullptr;
    QLineEdit* summary_type_ = nullptr;
    QLineEdit* summary_name_ = nullptr;
    QLineEdit* summary_flags_ = nullptr;
    QListWidget* entered_from_list_ = nullptr;
    QListWidget* references_list_ = nullptr;
    QPushButton* entered_goto_button_ = nullptr;
    QPushButton* add_exit_button_ = nullptr;
    QPushButton* ref_goto_button_ = nullptr;
    QPushButton* add_mob_button_ = nullptr;
    QPushButton* add_object_button_ = nullptr;
    QPushButton* remove_reset_button_ = nullptr;
    QPushButton* equip_button_ = nullptr;
    QPushButton* give_button_ = nullptr;

    nebbie::qt::MudColorTextEdit* name_ = nullptr;
    QLabel* name_line_info_ = nullptr;
    QLabel* name_ascii_info_ = nullptr;
    QLabel* name_english_info_ = nullptr;
    nebbie::qt::MudColorTextEdit* description_ = nullptr;
    QLabel* description_line_info_ = nullptr;
    QLabel* description_ascii_info_ = nullptr;
    QLabel* description_english_info_ = nullptr;

    QComboBox* sector_type_ = nullptr;
    FlagGroupWidget* room_flags_ = nullptr;

    QCheckBox* teleport_enabled_ = nullptr;
    QWidget* teleport_fields_ = nullptr;
    QLabel* teleport_status_ = nullptr;
    QSpinBox* tele_time_ = nullptr;
    QSpinBox* tele_targ_ = nullptr;
    QSpinBox* tele_mask_ = nullptr;
    QSpinBox* tele_cnt_ = nullptr;

    QWidget* river_panel_ = nullptr;
    QSpinBox* river_speed_ = nullptr;
    QSpinBox* river_dir_ = nullptr;
    QWidget* moblim_panel_ = nullptr;
    QSpinBox* moblim_ = nullptr;

    nebbie::qt::MudColorTextEdit* bright_at_night_ = nullptr;
    nebbie::qt::MudColorTextEdit* bright_at_day_ = nullptr;

    QListWidget* extra_desc_list_ = nullptr;
    QLineEdit* extra_desc_keyword_ = nullptr;
    QLabel* extra_desc_keyword_english_info_ = nullptr;
    nebbie::qt::MudColorTextEdit* extra_desc_description_ = nullptr;
    QLabel* extra_desc_line_info_ = nullptr;
    QLabel* extra_desc_ascii_info_ = nullptr;
    QLabel* extra_desc_english_info_ = nullptr;

    QListWidget* exit_list_ = nullptr;
    QComboBox* exit_direction_ = nullptr;
    QSpinBox* exit_to_room_ = nullptr;
    nebbie::qt::MudColorTextEdit* exit_description_ = nullptr;
    QLabel* exit_line_info_ = nullptr;
    QLabel* exit_ascii_info_ = nullptr;
    QLabel* exit_english_info_ = nullptr;
    QLineEdit* exit_keyword_ = nullptr;
    QLabel* exit_keyword_english_info_ = nullptr;
    FlagGroupWidget* exit_flags_ = nullptr;
    QSpinBox* exit_key_ = nullptr;
    QComboBox* exit_open_cmd_ = nullptr;
};
