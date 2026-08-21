#pragma once

#include "mud_color_widgets.hpp"
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

inline QString formatLongestLineStatus(const std::string& text, int max_length) {
    const nebbie::LongestVisibleLine longest = nebbie::find_longest_visible_line(text);
    if (longest.line_number <= 0) {
        return QStringLiteral("Riga più lunga: 0 caratteri");
    }

    QString status = QStringLiteral("Riga più lunga: %1 caratteri (riga %2)")
                         .arg(static_cast<qlonglong>(longest.visible_length))
                         .arg(longest.line_number);
  if (max_length > 0) {
        status += QStringLiteral(" — limite %1").arg(max_length);
    }
    return status;
}

inline void updateLineLengthMonitor(QLabel* label, const std::string& text, int max_length) {
    if (!label) {
        return;
    }
    if (max_length <= 0) {
        label->setText(formatLongestLineStatus(text, max_length));
        label->setStyleSheet(QStringLiteral("color: #606060;"));
        return;
    }

    QString status = formatLongestLineStatus(text, max_length);
    const nebbie::TextLineLengthReport report = nebbie::check_visible_text_line_lengths(text, max_length);
    if (!report.overlong.empty()) {
        QStringList details;
        for (const auto& issue : report.overlong) {
            details << QStringLiteral("riga %1 (%2)")
                           .arg(issue.line_number)
                           .arg(static_cast<qlonglong>(issue.length));
        }
        status += QStringLiteral(" — oltre limite: ") + details.join(QStringLiteral(", "));
    }
    label->setText(status);

    if (!report.ok()) {
        label->setStyleSheet(QStringLiteral("color: #b03000; font-weight: bold;"));
    } else {
        label->setStyleSheet(QStringLiteral("color: #406040;"));
    }
}

inline void updateLineLengthMonitor(QLabel* label, const QLineEdit* field, int max_length) {
    if (!field) {
        return;
    }
    updateLineLengthMonitor(label, field->text().toStdString(), max_length);
}

inline void updateLineLengthMonitor(QLabel* label, const QTextEdit* field, int max_length) {
    if (!field) {
        return;
    }
    updateLineLengthMonitor(label, field->toPlainText().toStdString(), max_length);
}

inline void updateLineLengthMonitor(QLabel* label, const MudColorTextEdit* field, int max_length) {
    if (!field) {
        return;
    }
    updateLineLengthMonitor(label, field->storageText().toStdString(), max_length);
}

} // namespace nebbie::qt
