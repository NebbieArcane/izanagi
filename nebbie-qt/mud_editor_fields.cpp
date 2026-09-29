#include "mud_editor_fields.hpp"

#include <QApplication>
#include <QClipboard>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QTextEdit>

namespace nebbie::qt {

void applyMudFieldSettings(const MudFieldList& fields, const int max_line_length, const bool show_color_codes) {
    for (MudColorTextEdit* field : fields) {
        if (!field) {
            continue;
        }
        field->setMaxLineLength(max_line_length);
        field->setShowColorCodes(show_color_codes);
    }
}

MudColorTextEdit* focusedMudField(const MudFieldList& fields) {
    for (MudColorTextEdit* field : fields) {
        if (field && field->hasFocus()) {
            return field;
        }
    }
    return nullptr;
}

void pastePlainTextIntoWidget(QWidget* widget) {
    if (!widget) {
        return;
    }

    if (auto* mud_field = qobject_cast<MudColorTextEdit*>(widget)) {
        mud_field->pastePlainText();
        return;
    }

    const QClipboard* clipboard = QApplication::clipboard();
    if (!clipboard) {
        return;
    }
    const QString text = clipboard->text();
    if (text.isEmpty()) {
        return;
    }

    if (auto* plain = qobject_cast<QPlainTextEdit*>(widget)) {
        plain->insertPlainText(text);
    } else if (auto* line = qobject_cast<QLineEdit*>(widget)) {
        line->insert(text);
    } else if (auto* rich = qobject_cast<QTextEdit*>(widget)) {
        rich->insertPlainText(text);
    }
}

} // namespace nebbie::qt
