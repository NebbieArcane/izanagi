#pragma once

#include "nebbie/types.hpp"

#include <QWidget>

class QLabel;
class QLineEdit;
class QListWidget;
class QTabWidget;
class QTextEdit;

class TranslatorRoomWidget : public QWidget {
    Q_OBJECT

public:
    explicit TranslatorRoomWidget(QWidget* parent = nullptr);

    void setMaxLineLength(int max_length);
    int maxLineLength() const { return max_line_length_; }

    void loadFromRoom(const nebbie::Room& room);
    void saveTranslatableFields(const nebbie::Room& original, nebbie::Room& room) const;
    bool currentFieldsHaveLineLengthIssues() const;

private slots:
    void onExtraDescSelected();
    void addExtraDesc();
    void removeExtraDesc();
    void onExitSelected();
    void applyExitDescription();
    void updateLineLengthIndicators();
    void convertAccentsOnTextTab();
    void convertAccentsOnExtraTab();
    void convertAccentsOnExitTab();

private:
    void refreshExtraDescForm();
    void refreshExitForm();

    int max_line_length_ = 0;
    bool loading_ = false;
    QTabWidget* tabs_ = nullptr;

    QLineEdit* name_ = nullptr;
    QLabel* name_line_info_ = nullptr;
    QLabel* name_ascii_info_ = nullptr;
    QTextEdit* description_ = nullptr;
    QLabel* description_line_info_ = nullptr;
    QLabel* description_ascii_info_ = nullptr;

    QListWidget* extra_desc_list_ = nullptr;
    QLineEdit* extra_desc_keyword_ = nullptr;
    QTextEdit* extra_desc_description_ = nullptr;
    QLabel* extra_desc_line_info_ = nullptr;
    QLabel* extra_desc_ascii_info_ = nullptr;

    QListWidget* exit_list_ = nullptr;
    QLabel* exit_info_ = nullptr;
    QLineEdit* exit_description_ = nullptr;
    QLabel* exit_line_info_ = nullptr;
    QLabel* exit_ascii_info_ = nullptr;
};
