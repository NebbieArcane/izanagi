#pragma once

#include "nebbie/types.hpp"

#include <QWidget>

class QLabel;
class QLineEdit;
class QListWidget;
class QTabWidget;
class QTextEdit;
class QListWidget;
class QTabWidget;
class QTextEdit;

class TranslatorRoomWidget : public QWidget {
    Q_OBJECT

public:
    explicit TranslatorRoomWidget(QWidget* parent = nullptr);

    void loadFromRoom(const nebbie::Room& room);
    void saveTranslatableFields(const nebbie::Room& original, nebbie::Room& room) const;

private slots:
    void onExtraDescSelected();
    void addExtraDesc();
    void removeExtraDesc();
    void onExitSelected();
    void applyExitDescription();

private:
    void refreshExtraDescForm();
    void refreshExitForm();

    bool loading_ = false;
    QTabWidget* tabs_ = nullptr;

    QLineEdit* name_ = nullptr;
    QTextEdit* description_ = nullptr;

    QListWidget* extra_desc_list_ = nullptr;
    QLineEdit* extra_desc_keyword_ = nullptr;
    QTextEdit* extra_desc_description_ = nullptr;

    QListWidget* exit_list_ = nullptr;
    QLabel* exit_info_ = nullptr;
    QLineEdit* exit_description_ = nullptr;
};
