#include "edit_menu.hpp"

#include "app_i18n.hpp"
#include "mud_color_widgets.hpp"
#include "mud_editor_fields.hpp"

#include <QApplication>
#include <QComboBox>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QPlainTextEdit>
#include <QTextEdit>

namespace nebbie::qt {

namespace {

QWidget* findTextInputWidget(QWidget* start) {
    for (QWidget* widget = start; widget; widget = widget->parentWidget()) {
        if (qobject_cast<QTextEdit*>(widget) || qobject_cast<QPlainTextEdit*>(widget)
            || qobject_cast<QLineEdit*>(widget)) {
            return widget;
        }
        if (auto* combo = qobject_cast<QComboBox*>(widget)) {
            if (combo->isEditable() && combo->lineEdit()) {
                return combo->lineEdit();
            }
        }
    }
    return nullptr;
}

template <typename Editor, typename Method>
void dispatchToEditor(Method method) {
    QWidget* widget = findTextInputWidget(QApplication::focusWidget());
    if (!widget) {
        return;
    }
    if (auto* editor = qobject_cast<Editor*>(widget)) {
        (editor->*method)();
    }
}

void undoFocusedText() {
    dispatchToEditor<QTextEdit>(&QTextEdit::undo);
    dispatchToEditor<QPlainTextEdit>(&QPlainTextEdit::undo);
    dispatchToEditor<QLineEdit>(&QLineEdit::undo);
}

void redoFocusedText() {
    dispatchToEditor<QTextEdit>(&QTextEdit::redo);
    dispatchToEditor<QPlainTextEdit>(&QPlainTextEdit::redo);
    dispatchToEditor<QLineEdit>(&QLineEdit::redo);
}

void cutFocusedText() {
    dispatchToEditor<QTextEdit>(&QTextEdit::cut);
    dispatchToEditor<QPlainTextEdit>(&QPlainTextEdit::cut);
    dispatchToEditor<QLineEdit>(&QLineEdit::cut);
}

void copyFocusedText() {
    QWidget* widget = findTextInputWidget(QApplication::focusWidget());
    if (!widget) {
        return;
    }
    if (auto* editor = qobject_cast<QTextEdit*>(widget)) {
        editor->copy();
    } else if (auto* editor = qobject_cast<QPlainTextEdit*>(widget)) {
        editor->copy();
    } else if (auto* editor = qobject_cast<QLineEdit*>(widget)) {
        editor->copy();
    }
}

void pastePlainTextFocused() {
    QWidget* widget = findTextInputWidget(QApplication::focusWidget());
    pastePlainTextIntoWidget(widget);
}

void selectAllFocusedText() {
    dispatchToEditor<QTextEdit>(&QTextEdit::selectAll);
    dispatchToEditor<QPlainTextEdit>(&QPlainTextEdit::selectAll);
    dispatchToEditor<QLineEdit>(&QLineEdit::selectAll);
}

} // namespace

void addStandardEditMenu(QMenuBar* menu_bar) {
    if (!menu_bar) {
        return;
    }

    auto* edit_menu = menu_bar->addMenu(appTr("menu.edit"));

    auto* undo_action = edit_menu->addAction(appTr("menu.undo"));
    undo_action->setShortcut(QKeySequence::Undo);
    QObject::connect(undo_action, &QAction::triggered, undo_action, []() { undoFocusedText(); });

    auto* redo_action = edit_menu->addAction(appTr("menu.redo"));
    redo_action->setShortcut(QKeySequence::Redo);
    QObject::connect(redo_action, &QAction::triggered, redo_action, []() { redoFocusedText(); });

    edit_menu->addSeparator();

    auto* cut_action = edit_menu->addAction(appTr("menu.cut"));
    cut_action->setShortcut(QKeySequence::Cut);
    QObject::connect(cut_action, &QAction::triggered, cut_action, []() { cutFocusedText(); });

    auto* copy_action = edit_menu->addAction(appTr("menu.copy"));
    copy_action->setShortcut(QKeySequence::Copy);
    QObject::connect(copy_action, &QAction::triggered, copy_action, []() { copyFocusedText(); });

    auto* paste_action = edit_menu->addAction(appTr("menu.paste"));
    paste_action->setShortcut(QKeySequence::Paste);
    QObject::connect(paste_action, &QAction::triggered, paste_action, []() { pastePlainTextFocused(); });

    auto* paste_plain_action = edit_menu->addAction(appTr("menu.paste_plain"));
    paste_plain_action->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_V));
    QObject::connect(paste_plain_action, &QAction::triggered, paste_plain_action,
                     []() { pastePlainTextFocused(); });

    edit_menu->addSeparator();

    auto* select_all_action = edit_menu->addAction(appTr("menu.select_all"));
    select_all_action->setShortcut(QKeySequence::SelectAll);
    QObject::connect(select_all_action, &QAction::triggered, select_all_action, []() { selectAllFocusedText(); });
}

} // namespace nebbie::qt
