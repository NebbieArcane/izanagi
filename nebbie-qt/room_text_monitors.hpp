#pragma once

#include "nebbie/mud_text.hpp"
#include "nebbie/text_lines.hpp"

#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>

namespace nebbie::qt {

inline QLabel* makeTextMonitorLabel() {
    auto* label = new QLabel;
    label->setWordWrap(true);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    return label;
}

inline void clearTextMonitor(QLabel* label) {
    if (!label) {
        return;
    }
    label->clear();
    label->setStyleSheet({});
}

inline void updateAsciiMonitor(QLabel* label, const std::string& text) {
    if (!label) {
        return;
    }
    if (is_mud_ascii_text(text)) {
        clearTextMonitor(label);
        return;
    }

    label->setText(
        QStringLiteral("Attenzione: caratteri non ASCII (%1). Nel client di gioco non si vedono "
                       "correttamente: usa e', a', o', u' come nel resto del mondo.")
            .arg(static_cast<qlonglong>(count_non_mud_ascii_chars(text))));
    label->setStyleSheet(QStringLiteral("color: #b03000; font-weight: bold;"));
}

inline void updateLineLengthMonitor(QLabel* label, const QLineEdit* field, int max_length) {
    if (!label || !field) {
        return;
    }
    if (max_length <= 0) {
        clearTextMonitor(label);
        return;
    }

    const int length = field->text().length();
    label->setText(QStringLiteral("%1/%2 caratteri").arg(length).arg(max_length));
    if (length > max_length) {
        label->setStyleSheet(QStringLiteral("color: #b03000; font-weight: bold;"));
    } else {
        label->setStyleSheet(QStringLiteral("color: #406040;"));
    }
}

inline void updateLineLengthMonitor(QLabel* label, const QTextEdit* field, int max_length) {
    if (!label || !field) {
        return;
    }
    if (max_length <= 0) {
        clearTextMonitor(label);
        return;
    }

    const QString text = field->toPlainText();
    const QStringList lines = text.split('\n');
    const int current_line = field->textCursor().blockNumber() + 1;
    const int current_length =
        (current_line >= 1 && current_line <= lines.size()) ? lines[current_line - 1].length() : 0;

    QString status = QStringLiteral("Riga %1: %2/%3 caratteri")
                         .arg(current_line)
                         .arg(current_length)
                         .arg(max_length);

    QStringList overlong;
    for (int i = 0; i < lines.size(); ++i) {
        if (lines[i].length() > max_length) {
            overlong << QStringLiteral("riga %1 (%2)").arg(i + 1).arg(lines[i].length());
        }
    }
    if (!overlong.isEmpty()) {
        status += QStringLiteral(" — oltre limite: ") + overlong.join(", ");
    }
    label->setText(status);

    const nebbie::TextLineLengthReport report =
        nebbie::check_text_line_lengths(text.toStdString(), max_length);
    if (!report.ok()) {
        label->setStyleSheet(QStringLiteral("color: #b03000; font-weight: bold;"));
    } else {
        label->setStyleSheet(QStringLiteral("color: #406040;"));
    }
}

} // namespace nebbie::qt
