#pragma once

#include "mud_editor_fields.hpp"
#include "nebbie/types.hpp"
#include "nebbie/world.hpp"

#include <QWidget>

class QComboBox;
class QLabel;
class QListWidget;
class QSpinBox;

namespace nebbie::qt {
class MudColorTextEdit;
}

class ZoneEditorWidget : public QWidget {
    Q_OBJECT

public:
    explicit ZoneEditorWidget(QWidget* parent = nullptr);

    void setWorld(nebbie::World* world);
    void loadFromZone(const nebbie::Zone& zone);
    void saveZoneInfoTo(nebbie::Zone& zone) const;
    int zoneNum() const { return zone_num_; }
    void selectResetIndex(int index);

    void setMaxLineLength(int max_length);
    void setShowColorCodes(bool show);
    nebbie::qt::MudColorTextEdit* focusedMudField() const;

signals:
    void zoneModified();
    void gotoRoomRequested(long vnum);
    void gotoMobRequested(long vnum);
    void gotoObjectRequested(long vnum);

private slots:
    void onResetSelected();
    void onResetCommandChanged();
    void addReset();
    void applyReset();
    void removeReset();
    void moveResetUp();
    void moveResetDown();
    void gotoResetRoom();
    void gotoResetEntity();

private:
    void refreshResetList();
    void updateResetFieldLabels();
    void loadResetForm(const nebbie::ResetCommand& cmd);
    nebbie::ResetCommand readResetForm() const;
    int currentResetIndex() const;
    void setComboIntValue(QComboBox* combo, int value) const;
    void applyMudFieldSettings();

    nebbie::World* world_ = nullptr;
    int zone_num_ = 0;
    int max_line_length_ = 0;
    bool show_color_codes_ = false;

    QLabel* zone_num_label_ = nullptr;
    nebbie::qt::MudColorTextEdit* name_ = nullptr;
    QLabel* bottom_label_ = nullptr;
    QSpinBox* top_ = nullptr;
    QSpinBox* lifespan_ = nullptr;
    QComboBox* reset_mode_ = nullptr;

    QListWidget* reset_list_ = nullptr;
    QComboBox* reset_command_ = nullptr;
    QLabel* reset_legend_ = nullptr;
    QLabel* if_flag_label_ = nullptr;
    QLabel* arg1_label_ = nullptr;
    QLabel* arg2_label_ = nullptr;
    QLabel* arg3_label_ = nullptr;
    QLabel* arg4_label_ = nullptr;
    QSpinBox* reset_if_flag_ = nullptr;
    QSpinBox* reset_arg1_ = nullptr;
    QSpinBox* reset_arg2_ = nullptr;
    QSpinBox* reset_arg3_ = nullptr;
    QSpinBox* reset_arg4_ = nullptr;
    QComboBox* equip_position_ = nullptr;
    QComboBox* door_state_ = nullptr;
    QWidget* equip_panel_ = nullptr;
    QWidget* door_state_panel_ = nullptr;
};
