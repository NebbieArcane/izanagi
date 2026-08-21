#pragma once

#include "nebbie/types.hpp"

#include <QWidget>

class QLabel;
class QLineEdit;
class QListWidget;

namespace nebbie::qt {
class MudColorTextEdit;
}

class TranslatorRoomWidget : public QWidget {
    Q_OBJECT

public:
    explicit TranslatorRoomWidget(QWidget* parent = nullptr);

    void setMaxLineLength(int max_length);
    int maxLineLength() const { return max_line_length_; }
    void setShowColorCodes(bool show);
    bool showColorCodes() const { return show_color_codes_; }

    void loadFromRoom(const nebbie::Room& room);
    void saveTranslatableFields(const nebbie::Room& original, nebbie::Room& room) const;
    bool currentFieldsHaveLineLengthIssues() const;

    nebbie::qt::MudColorTextEdit* focusedMudField() const;

private slots:
    void onExtraDescSelected();
    void addExtraDesc();
    void removeExtraDesc();
    void onExitSelected();
    void applyExitDescription();
    void updateLineLengthIndicators();
    void convertAccentsOnTextSection();
    void convertAccentsOnExtraSection();
    void convertAccentsOnExitSection();

private:
    void refreshExtraDescForm();
    void refreshExitForm();
    void applyColorSettingsToFields();
    void hookMudField(nebbie::qt::MudColorTextEdit* field);

    int max_line_length_ = 0;
    bool show_color_codes_ = false;
    bool loading_ = false;

    nebbie::qt::MudColorTextEdit* name_ = nullptr;
    QLabel* name_line_info_ = nullptr;
    QLabel* name_ascii_info_ = nullptr;
    nebbie::qt::MudColorTextEdit* description_ = nullptr;
    QLabel* description_line_info_ = nullptr;
    QLabel* description_ascii_info_ = nullptr;

    QListWidget* extra_desc_list_ = nullptr;
    QLineEdit* extra_desc_keyword_ = nullptr;
    nebbie::qt::MudColorTextEdit* extra_desc_description_ = nullptr;
    QLabel* extra_desc_line_info_ = nullptr;
    QLabel* extra_desc_ascii_info_ = nullptr;

    QListWidget* exit_list_ = nullptr;
    QLabel* exit_info_ = nullptr;
    nebbie::qt::MudColorTextEdit* exit_description_ = nullptr;
    QLabel* exit_line_info_ = nullptr;
    QLabel* exit_ascii_info_ = nullptr;
};
