#include "mud_color_widgets.hpp"

#include "nebbie/mud_text.hpp"
#include "nebbie/text_lines.hpp"

#include <QApplication>
#include <QClipboard>
#include <QFont>
#include <QFontMetrics>
#include <QHelpEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QToolTip>

namespace nebbie::qt {

namespace {

constexpr QColor kRulerOkColor(255, 245, 170, 180);
constexpr QColor kRulerExceededColor(220, 60, 60, 220);
constexpr QColor kNewlineMarkerColor(90, 130, 180, 200);
constexpr QColor kBlankLineFillColor(180, 210, 255, 70);
constexpr QColor kTrailingBlankWarnFillColor(255, 200, 160, 90);
constexpr QColor kTrailingBlankWarnMarkerColor(180, 80, 30, 220);
constexpr QColor kTrailingBlankOkFillColor(160, 220, 170, 90);
constexpr QColor kTrailingBlankOkMarkerColor(40, 130, 60, 220);
constexpr QColor kTrailingBlankMissingMarkerColor(200, 50, 40, 230);

int char_column_width(const QFontMetrics& metrics) {
    return metrics.horizontalAdvance(QLatin1Char('M'));
}

int trailing_empty_block_count(const QTextDocument* document) {
    if (!document) {
        return 0;
    }
    int count = 0;
    for (QTextBlock block = document->lastBlock(); block.isValid(); block = block.previous()) {
        if (!block.text().isEmpty()) {
            break;
        }
        ++count;
        if (block.blockNumber() == 0) {
            break;
        }
    }
    return count;
}

void paint_line_length_ruler(QWidget* widget,
                             const QFont& font,
                             int max_line_length,
                             bool exceeded) {
    if (max_line_length <= 0 || widget == nullptr) {
        return;
    }

    const QFontMetrics metrics(font);
    const int x = 4 + max_line_length * char_column_width(metrics);
    QPainter painter(widget);
    painter.setRenderHint(QPainter::Antialiasing, false);
    QPen pen(exceeded ? kRulerExceededColor : kRulerOkColor, 2, Qt::SolidLine);
    painter.setPen(pen);
    painter.drawLine(x, 0, x, widget->height());
}

} // namespace

void configureMudDescriptionField(MudColorTextEdit* field) {
    if (!field) {
        return;
    }
    field->setMinimumWidth(560);
    field->setMinimumHeight(320);
    field->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    field->setLineWrapMode(QTextEdit::NoWrap);
    field->setSingleLineMode(false);
    // .wld paragraph bodies always write "\n~"; show that final newline as expected.
    field->setRequireTrailingNewline(true);
}

void configureMudOnOwnLineField(MudColorTextEdit* field, int min_height) {
    if (!field) {
        return;
    }
    field->setMinimumHeight(min_height);
    field->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    field->setLineWrapMode(QTextEdit::WidgetWidth);
    field->setSingleLineMode(false);
    field->setRequireTrailingNewline(true);
}

void syncMudParagraphTrailingNewlinePolicy(MudColorTextEdit* field) {
    if (!field) {
        return;
    }
    const bool needs_on_own_line = field->storageText().contains(QLatin1Char('\n'));
    field->setRequireTrailingNewline(needs_on_own_line);
}

void configureMudSingleLineField(MudColorTextEdit* field) {
    if (!field) {
        return;
    }
    field->setMinimumWidth(420);
    field->setMaximumHeight(34);
    field->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    field->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    field->setLineWrapMode(QTextEdit::NoWrap);
    field->setSingleLineMode(true);
    field->setRequireTrailingNewline(false);
    field->refreshTrailingNewlineToolTip();
}

MudColorTextEdit::MudColorTextEdit(QWidget* parent) : QTextEdit(parent) {
    setLineWrapMode(QTextEdit::NoWrap);
    setAcceptRichText(false);
    QFont font = this->font();
    font.setFamily(QStringLiteral("Monospace"));
    font.setStyleHint(QFont::Monospace);
    setFont(font);
    setTextColor(mud_foreground_color(7, false));
    connect(document(), &QTextDocument::contentsChange, this, &MudColorTextEdit::onContentsChanged);
}

void MudColorTextEdit::setStorageText(const QString& text) {
    storage_ = text;
    if (single_line_mode_) {
        while (storage_.endsWith(QLatin1Char('\n')) || storage_.endsWith(QLatin1Char('\r'))) {
            storage_.chop(1);
        }
    }
    rebuildDocument();
    emit storageTextChanged(storage_);
}

void MudColorTextEdit::setShowColorCodes(bool show) {
    if (show_color_codes_ == show) {
        return;
    }
    show_color_codes_ = show;
    rebuildDocument();
}

void MudColorTextEdit::setMaxLineLength(int max_length) {
    max_line_length_ = max_length < 0 ? 0 : max_length;
    updateRulerState();
    viewport()->update();
}

void MudColorTextEdit::refreshTrailingNewlineToolTip() {
    if (single_line_mode_) {
        setToolTip(QStringLiteral(
            "Campo a una riga: la ~ sta sulla stessa riga del testo (niente Invio finale)."));
        return;
    }
    if (require_trailing_newline_) {
        setToolTip(QStringLiteral(
            "Dopo l'ultima riga di testo premi Invio una volta.\n"
            "↵ verde = corretto (nel file la ~ sta sulla riga sotto).\n"
            "↵ rosso sull'ultima riga = manca l'Invio; ↵ rosso su righe vuote extra = toglile."));
    } else {
        setToolTip(QStringLiteral(
            "Niente Invio dopo l'ultima riga di testo.\n"
            "↵ rosso in coda = togli quell'Invio (nel file la ~ sta sulla stessa riga).\n"
            "Se il testo ha piu' paragrafi, l'editor puo' richiedere invece un Invio finale."));
    }
}

void MudColorTextEdit::setRequireTrailingNewline(bool required) {
    if (require_trailing_newline_ == required) {
        refreshTrailingNewlineToolTip();
        return;
    }
    require_trailing_newline_ = required;
    refreshTrailingNewlineToolTip();
    viewport()->update();
}

void MudColorTextEdit::setSingleLineMode(bool single_line) {
    single_line_mode_ = single_line;
}

void MudColorTextEdit::replaceSelectionWithStorageText(const QString& text) {
    QString normalized =
        QString::fromStdString(nebbie::normalize_mud_ascii_text(text.toStdString()));
    if (single_line_mode_) {
        normalized.replace(QLatin1Char('\r'), QLatin1Char(' '));
        normalized.replace(QLatin1Char('\n'), QLatin1Char(' '));
    }

    const QTextCursor cursor = textCursor();
    int storage_from = display_position_to_storage(storage_, cursor.position(), show_color_codes_);
    int storage_to = storage_from;
    if (cursor.hasSelection()) {
        storage_from = display_position_to_storage(storage_, cursor.selectionStart(), show_color_codes_);
        storage_to = display_position_to_storage(storage_, cursor.selectionEnd(), show_color_codes_);
    }

    if (storage_to > storage_from) {
        storage_.remove(storage_from, storage_to - storage_from);
    }
    if (!normalized.isEmpty()) {
        storage_.insert(storage_from, normalized);
    }
    rebuildDocument(storage_from + normalized.size());
    emit storageTextChanged(storage_);
}

bool MudColorTextEdit::normalizeMudAsciiText() {
    const int storage_cursor =
        display_position_to_storage(storage_, textCursor().position(), show_color_codes_);
    const QString fixed =
        QString::fromStdString(nebbie::normalize_mud_ascii_text(storage_.toStdString()));
    if (fixed == storage_) {
        return false;
    }
    storage_ = fixed;
    rebuildDocument(qMin(storage_cursor, storage_.size()));
    emit storageTextChanged(storage_);
    return true;
}

void MudColorTextEdit::insertColorCode(const QString& code) {
    replaceSelectionWithStorageText(code);
}

void MudColorTextEdit::insertStorageTextAtCursor(const QString& text) {
    if (text.isEmpty()) {
        return;
    }
    replaceSelectionWithStorageText(text);
}

void MudColorTextEdit::pastePlainText() {
    const QClipboard* clipboard = QApplication::clipboard();
    if (!clipboard) {
        return;
    }
    insertStorageTextAtCursor(clipboard->text());
}

void MudColorTextEdit::insertFromMimeData(const QMimeData* source) {
    if (!source) {
        return;
    }
    insertStorageTextAtCursor(source->text());
}

void MudColorTextEdit::rebuildDocument(int preferred_storage_cursor) {
    rebuilding_ = true;

    const int storage_cursor = preferred_storage_cursor >= 0
                                   ? preferred_storage_cursor
                                   : display_position_to_storage(storage_, textCursor().position(), show_color_codes_);

    QTextCursor cursor(document());
    cursor.beginEditBlock();
    cursor.select(QTextCursor::Document);
    cursor.removeSelectedText();

    MudDisplayColorState state;
    for (int storage_pos = 0; storage_pos < storage_.size();) {
        const int code_len = mud_color_code_length_at(storage_, storage_pos);
        if (code_len > 0) {
            nebbie::MudColorCode code;
            const std::string raw = storage_.mid(storage_pos, code_len).toStdString();
            nebbie::try_parse_mud_color_code(raw, 0, &code);
            if (show_color_codes_) {
                cursor.setCharFormat(formatForState(state));
                cursor.insertText(QString::fromStdString(code.raw));
            }
            state.foreground = code.foreground;
            state.bold = code.bold;
            state.modifier = code.modifier;
            storage_pos += code_len;
            continue;
        }

        const QChar ch = storage_.at(storage_pos);
        cursor.setCharFormat(formatForDisplayedChar(state, ch));
        cursor.insertText(QString(ch));
        ++storage_pos;
    }
    cursor.endEditBlock();

    previous_display_ = build_display_text(storage_, show_color_codes_);
    const int display_cursor = storage_position_to_display(storage_, storage_cursor, show_color_codes_);
    QTextCursor move(document());
    move.setPosition(display_cursor);
    setTextCursor(move);

    rebuilding_ = false;
    updateRulerState();
    viewport()->update();
}

void MudColorTextEdit::updateRulerState() {
    if (max_line_length_ <= 0) {
        ruler_exceeded_ = false;
        return;
    }
    const nebbie::TextLineLengthReport report =
        nebbie::check_visible_text_line_lengths(storage_.toStdString(), max_line_length_);
    ruler_exceeded_ = !report.ok();
}

QTextCharFormat MudColorTextEdit::formatForState(const MudDisplayColorState& state) const {
    QTextCharFormat format;
    if (!show_color_codes_) {
        format.setForeground(mud_foreground_color(state.foreground, state.bold != 0));
    }
    return format;
}

QTextCharFormat MudColorTextEdit::formatForDisplayedChar(const MudDisplayColorState& state,
                                                         const QChar ch) const {
    QTextCharFormat format = formatForState(state);
    if (is_mud_ascii_qchar(ch)) {
        return format;
    }

    // Warm highlight so accented / Unicode paste stand out against mud colors.
    format.setBackground(QColor(255, 214, 102));
    format.setForeground(QColor(120, 24, 0));
    format.setFontWeight(QFont::Bold);
    return format;
}

MudDisplayColorState MudColorTextEdit::color_at_storage_position(int storage_pos) const {
    MudDisplayColorState state;
    for (int index = 0; index < storage_pos && index < storage_.size();) {
        const int code_len = mud_color_code_length_at(storage_, index);
        if (code_len > 0) {
            nebbie::MudColorCode code;
            const std::string raw = storage_.mid(index, code_len).toStdString();
            if (nebbie::try_parse_mud_color_code(raw, 0, &code)) {
                state.foreground = code.foreground;
                state.bold = code.bold;
                state.modifier = code.modifier;
            }
            index += code_len;
            continue;
        }
        ++index;
    }
    return state;
}

QString MudColorTextEdit::tooltipForStoragePosition(int storage_pos) const {
    if (storage_pos >= 0 && storage_pos < storage_.size()
        && !is_mud_ascii_qchar(storage_.at(storage_pos))) {
        return QStringLiteral(
            "Carattere non ASCII: nel client di gioco non si vede correttamente. "
            "Usa e', a', o', u' come nel resto del mondo.");
    }

    for (int index = storage_pos; index < storage_.size();) {
        const int code_len = mud_color_code_length_at(storage_, index);
        if (code_len > 0) {
            nebbie::MudColorCode code;
            const std::string raw = storage_.mid(index, code_len).toStdString();
            if (nebbie::try_parse_mud_color_code(raw, 0, &code)) {
                return QString::fromStdString(nebbie::mud_color_code_description(code));
            }
        }
        break;
    }

    const MudDisplayColorState state = color_at_storage_position(storage_pos);
    nebbie::MudColorCode code;
    code.modifier = state.modifier;
    code.bold = state.bold;
    code.foreground = state.foreground;
    return QString::fromStdString(nebbie::mud_color_code_description(code));
}

void MudColorTextEdit::onContentsChanged(int position, int charsRemoved, int charsAdded) {
    if (rebuilding_) {
        return;
    }

    if (show_color_codes_) {
        storage_ = QString::fromStdString(nebbie::normalize_mud_ascii_text(toPlainText().toStdString()));
        previous_display_ = storage_;
        rebuildDocument(textCursor().position());
        emit storageTextChanged(storage_);
        return;
    }

    const QString inserted =
        charsAdded > 0
            ? QString::fromStdString(
                  nebbie::normalize_mud_ascii_text(toPlainText().mid(position, charsAdded).toStdString()))
            : QString();
    const int storage_from = display_position_to_storage(storage_, position, false);
    const int storage_to = display_position_to_storage(storage_, position + charsRemoved, false);
    storage_.remove(storage_from, storage_to - storage_from);
    storage_.insert(storage_from, inserted);
    rebuildDocument(storage_from + inserted.size());
    emit storageTextChanged(storage_);
}

void MudColorTextEdit::paintNewlineGuides(QPainter& painter) const {
    if (single_line_mode_) {
        return;
    }

    const QFontMetrics metrics(font());
    const QString marker = QStringLiteral("\u21b5"); // ↵
    const int marker_width = qMax(metrics.horizontalAdvance(marker), metrics.averageCharWidth());
    const int trailing_blanks = trailing_empty_block_count(document());
    const int last_block_number = document()->lastBlock().blockNumber();
    const bool has_content = !storage_.isEmpty();
    const bool missing_required_trailing =
        require_trailing_newline_ && has_content && trailing_blanks == 0;

    for (QTextBlock block = document()->firstBlock(); block.isValid(); block = block.next()) {
        const bool empty_line = block.text().isEmpty();
        const int from_end = last_block_number - block.blockNumber();
        const bool trailing_blank = empty_line && from_end < trailing_blanks;
        const bool last_content_line = !empty_line && block.blockNumber() == last_block_number;

        QTextCursor start_cursor(block);
        start_cursor.movePosition(QTextCursor::StartOfBlock);
        QTextCursor end_cursor(block);
        end_cursor.movePosition(QTextCursor::EndOfBlock);
        const QRect start_rect = cursorRect(start_cursor);
        const QRect end_rect = cursorRect(end_cursor);

        if (empty_line) {
            const int top = start_rect.top();
            const int height = qMax(end_rect.bottom() - top + 1, metrics.lineSpacing());
            QColor fill = kBlankLineFillColor;
            if (trailing_blank) {
                if (require_trailing_newline_) {
                    // Blank closest to content = expected "\n~"; further blanks = extra.
                    const bool is_required_slot = from_end == trailing_blanks - 1;
                    fill = is_required_slot ? kTrailingBlankOkFillColor : kTrailingBlankWarnFillColor;
                } else {
                    fill = kTrailingBlankWarnFillColor;
                }
            }
            painter.fillRect(0, top, viewport()->width(), height, fill);
        }

        QColor marker_color = kNewlineMarkerColor;
        if (trailing_blank) {
            if (require_trailing_newline_) {
                const bool is_required_slot = from_end == trailing_blanks - 1;
                marker_color = is_required_slot ? kTrailingBlankOkMarkerColor : kTrailingBlankWarnMarkerColor;
            } else {
                marker_color = kTrailingBlankWarnMarkerColor;
            }
        } else if (missing_required_trailing && last_content_line) {
            marker_color = kTrailingBlankMissingMarkerColor;
        }

        const int x = empty_line ? 4 : end_rect.right() + 2;
        painter.setPen(marker_color);
        painter.drawText(x, end_rect.top(), marker_width + 4, qMax(end_rect.height(), metrics.height()),
                         Qt::AlignLeft | Qt::AlignVCenter, marker);
    }
}

void MudColorTextEdit::paintEvent(QPaintEvent* event) {
    QTextEdit::paintEvent(event);
    QPainter painter(viewport());
    painter.setFont(font());
    paintNewlineGuides(painter);
    paint_line_length_ruler(viewport(), font(), max_line_length_, ruler_exceeded_);
}

void MudColorTextEdit::keyPressEvent(QKeyEvent* event) {
    if (single_line_mode_ && (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)) {
        event->ignore();
        return;
    }
    QTextEdit::keyPressEvent(event);
}

bool MudColorTextEdit::event(QEvent* event) {
    if (event->type() == QEvent::ToolTip && !storage_.isEmpty()) {
        const auto* help = static_cast<QHelpEvent*>(event);
        const int storage_pos =
            display_position_to_storage(storage_, textCursor().position(), show_color_codes_);
        QToolTip::showText(help->globalPos(), tooltipForStoragePosition(storage_pos), this);
        return true;
    }
    return QTextEdit::event(event);
}

} // namespace nebbie::qt
