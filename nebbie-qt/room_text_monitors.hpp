#pragma once

#include "mud_color_widgets.hpp"
#include "nebbie/english_lexicon.hpp"
#include "nebbie/mud_text.hpp"
#include "nebbie/text_lines.hpp"

#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>

#include <string>
#include <vector>

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

inline void updateEnglishLexiconMonitor(QLabel* label, const std::string& text) {
    if (!label) {
        return;
    }
    const std::vector<std::string> tokens = find_english_tokens(text);
    if (tokens.empty()) {
        clearTextMonitor(label);
        return;
    }
    label->setText(QStringLiteral("Possibili parole inglesi: %1")
                       .arg(QString::fromStdString(join_english_tokens(tokens))));
    label->setStyleSheet(QStringLiteral("color: #8a5a00; font-weight: bold;"));
}

inline void updateEnglishLexiconMonitor(QLabel* label, const MudColorTextEdit* field) {
    if (!field) {
        return;
    }
    updateEnglishLexiconMonitor(label, field->storageText().toStdString());
}

inline void updateEnglishLexiconMonitor(QLabel* label, const QLineEdit* field) {
    if (!field) {
        return;
    }
    updateEnglishLexiconMonitor(label, field->text().toStdString());
}

inline int countTrailingEmptyLines(const QString& text) {
    if (text.isEmpty()) {
        return 0;
    }
    // Match QTextEdit blocks: each trailing \n yields an empty line after content.
    int count = 0;
    for (int i = text.size() - 1; i >= 0 && text.at(i) == QLatin1Char('\n'); --i) {
        ++count;
    }
    return count;
}

enum class TrailingNewlineMonitor {
    /** Trailing blank lines are unusual (warn in red). */
    WarnIfPresent,
    /** .wld paragraph body: final newline before ~ is required (green if present, red if missing). */
    Required,
};

inline QString formatLongestLineStatus(const std::string& text, int max_length,
                                       TrailingNewlineMonitor trailing_policy) {
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
    const int trailing_empty = countTrailingEmptyLines(QString::fromStdString(text));
    if (trailing_policy == TrailingNewlineMonitor::Required) {
        if (trailing_empty == 0) {
            status += QStringLiteral(
                " — premi Invio dopo l'ultima riga (\u21b5 verde; nel file ~ sta sotto)");
        } else if (trailing_empty == 1) {
            status += QStringLiteral(" — Invio finale OK (\u21b5 verde; ~ sulla riga sotto)");
        } else {
            status += QStringLiteral(
                " — troppe righe vuote finali: ne basta 1 prima di ~ (togli le extra)");
        }
    } else if (trailing_empty > 0) {
        status += trailing_empty == 1
                      ? QStringLiteral(
                            " — togli l'Invio finale (\u21b5 rosso; qui ~ sta sulla stessa riga)")
                      : QStringLiteral(
                            " — togli le %1 righe vuote finali (qui ~ sta sulla stessa riga)")
                            .arg(trailing_empty);
    }
    return status;
}

/** Standalone hint under mob/obj fields (no line-length stats). */
inline void updateTildeNewlineHint(QLabel* label, const MudColorTextEdit* field) {
    if (!label || !field) {
        return;
    }
    if (field->isSingleLineMode()) {
        clearTextMonitor(label);
        return;
    }
    const std::string text = field->storageText().toStdString();
    const int trailing_empty = countTrailingEmptyLines(QString::fromStdString(text));
    if (field->requireTrailingNewline()) {
        if (text.empty()) {
            label->setText(QStringLiteral(
                "Quando scrivi: un Invio dopo l'ultima riga (\u21b5 verde). Nel file la ~ sta sotto."));
            label->setStyleSheet(QStringLiteral("color: #606060;"));
            return;
        }
        if (trailing_empty == 0) {
            label->setText(QStringLiteral(
                "Premi Invio dopo l'ultima riga di testo (\u21b5 diventera' verde)."));
            label->setStyleSheet(QStringLiteral("color: #b03000; font-weight: bold;"));
        } else if (trailing_empty == 1) {
            label->setText(QStringLiteral(
                "Invio finale OK (\u21b5 verde). Nel file: testo, a capo, poi ~ da sola."));
            label->setStyleSheet(QStringLiteral("color: #2e7d32; font-weight: bold;"));
        } else {
            label->setText(QStringLiteral(
                "Hai %1 righe vuote in coda: lascine una sola (togli le altre).")
                               .arg(trailing_empty));
            label->setStyleSheet(QStringLiteral("color: #b03000; font-weight: bold;"));
        }
        return;
    }
    if (trailing_empty > 0) {
        label->setText(QStringLiteral(
            "Togli l'Invio finale (\u21b5 rosso). Qui la ~ sta sulla stessa riga del testo."));
        label->setStyleSheet(QStringLiteral("color: #b03000; font-weight: bold;"));
    } else {
        label->setText(QStringLiteral(
            "Niente Invio finale: corretto (nel file testo~ sulla stessa riga)."));
        label->setStyleSheet(QStringLiteral("color: #606060;"));
    }
}

inline void updateLineLengthMonitor(QLabel* label, const std::string& text, int max_length,
                                    TrailingNewlineMonitor trailing_policy = TrailingNewlineMonitor::WarnIfPresent) {
    if (!label) {
        return;
    }

    QString status = formatLongestLineStatus(text, max_length, trailing_policy);
    nebbie::TextLineLengthReport report;
    if (max_length > 0) {
        report = nebbie::check_visible_text_line_lengths(text, max_length);
        if (!report.overlong.empty()) {
            QStringList details;
            for (const auto& issue : report.overlong) {
                details << QStringLiteral("riga %1 (%2)")
                               .arg(issue.line_number)
                               .arg(static_cast<qlonglong>(issue.length));
            }
            status += QStringLiteral(" — oltre limite: ") + details.join(QStringLiteral(", "));
        }
    }
    label->setText(status);

    const int trailing_empty = countTrailingEmptyLines(QString::fromStdString(text));
    const bool trailing_problem = trailing_policy == TrailingNewlineMonitor::Required
                                      ? (!text.empty() && trailing_empty == 0) || trailing_empty > 1
                                      : trailing_empty > 0;
    const bool trailing_ok_required =
        trailing_policy == TrailingNewlineMonitor::Required && !text.empty() && trailing_empty == 1;

    if (!report.ok() || trailing_problem) {
        label->setStyleSheet(QStringLiteral("color: #b03000; font-weight: bold;"));
    } else if (trailing_ok_required) {
        label->setStyleSheet(QStringLiteral("color: #2e7d32; font-weight: bold;"));
    } else if (max_length <= 0) {
        label->setStyleSheet(QStringLiteral("color: #606060;"));
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
    const TrailingNewlineMonitor policy = field->requireTrailingNewline()
                                              ? TrailingNewlineMonitor::Required
                                              : TrailingNewlineMonitor::WarnIfPresent;
    updateLineLengthMonitor(label, field->storageText().toStdString(), max_length, policy);
}

} // namespace nebbie::qt
