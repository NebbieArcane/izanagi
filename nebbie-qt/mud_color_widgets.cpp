#include "mud_color_widgets.hpp"

#include "nebbie/text_lines.hpp"

#include <QHelpEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QTextCursor>
#include <QToolTip>

namespace nebbie::qt {

namespace {

constexpr QColor kRulerOkColor(255, 245, 170, 180);
constexpr QColor kRulerExceededColor(220, 60, 60, 220);

int char_column_width(const QFontMetrics& metrics) {
    return metrics.horizontalAdvance(QLatin1Char('M'));
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
}

MudColorTextEdit::MudColorTextEdit(QWidget* parent) : QTextEdit(parent) {
    setLineWrapMode(QTextEdit::NoWrap);
    setAcceptRichText(false);
    QFont font = this->font();
    font.setFamily(QStringLiteral("Monospace"));
    font.setStyleHint(QFont::Monospace);
    setFont(font);
    connect(document(), &QTextDocument::contentsChange, this, &MudColorTextEdit::onContentsChanged);
}

void MudColorTextEdit::setStorageText(const QString& text) {
    storage_ = text;
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

void MudColorTextEdit::setSingleLineMode(bool single_line) {
    single_line_mode_ = single_line;
}

void MudColorTextEdit::insertColorCode(const QString& code) {
    const int storage_cursor =
        display_position_to_storage(storage_, textCursor().position(), show_color_codes_);
    storage_.insert(storage_cursor, code);
    rebuildDocument(storage_cursor + code.size());
    emit storageTextChanged(storage_);
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
    const std::string bytes = storage_.toStdString();
    for (int storage_pos = 0; storage_pos < storage_.size();) {
        nebbie::MudColorCode code;
        if (nebbie::try_parse_mud_color_code(bytes, static_cast<std::size_t>(storage_pos), &code)) {
            if (show_color_codes_) {
                cursor.setCharFormat(formatForState(state));
                cursor.insertText(QString::fromStdString(code.raw));
            }
            state.foreground = code.foreground;
            state.bold = code.bold;
            state.modifier = code.modifier;
            storage_pos += static_cast<int>(code.raw.size());
            continue;
        }

        cursor.setCharFormat(formatForState(state));
        cursor.insertText(QString(storage_.at(storage_pos)));
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

MudDisplayColorState MudColorTextEdit::color_at_storage_position(int storage_pos) const {
    MudDisplayColorState state;
    const std::string bytes = storage_.toStdString();
    for (int index = 0; index < storage_pos && index < storage_.size();) {
        nebbie::MudColorCode code;
        if (nebbie::try_parse_mud_color_code(bytes, static_cast<std::size_t>(index), &code)) {
            state.foreground = code.foreground;
            state.bold = code.bold;
            state.modifier = code.modifier;
            index += static_cast<int>(code.raw.size());
            continue;
        }
        ++index;
    }
    return state;
}

QString MudColorTextEdit::tooltipForStoragePosition(int storage_pos) const {
    const std::string bytes = storage_.toStdString();
    for (int index = 0; index < storage_pos && index < storage_.size();) {
        nebbie::MudColorCode code;
        if (nebbie::try_parse_mud_color_code(bytes, static_cast<std::size_t>(index), &code)) {
            index += static_cast<int>(code.raw.size());
            continue;
        }
        ++index;
    }

    for (int index = storage_pos; index < storage_.size();) {
        nebbie::MudColorCode code;
        if (nebbie::try_parse_mud_color_code(bytes, static_cast<std::size_t>(index), &code)) {
            return QString::fromStdString(nebbie::mud_color_code_description(code));
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
        storage_ = toPlainText();
        previous_display_ = storage_;
        updateRulerState();
        viewport()->update();
        emit storageTextChanged(storage_);
        return;
    }

    const QString inserted = charsAdded > 0 ? toPlainText().mid(position, charsAdded) : QString();
    const int storage_from = display_position_to_storage(storage_, position, false);
    const int storage_to = display_position_to_storage(storage_, position + charsRemoved, false);
    storage_.remove(storage_from, storage_to - storage_from);
    storage_.insert(storage_from, inserted);
    rebuildDocument(storage_from + inserted.size());
    emit storageTextChanged(storage_);
}

void MudColorTextEdit::paintEvent(QPaintEvent* event) {
    QTextEdit::paintEvent(event);
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
    if (event->type() == QEvent::ToolTip && !show_color_codes_ && !storage_.isEmpty()) {
        const auto* help = static_cast<QHelpEvent*>(event);
        const int storage_pos =
            display_position_to_storage(storage_, textCursor().position(), show_color_codes_);
        QToolTip::showText(help->globalPos(), tooltipForStoragePosition(storage_pos), this);
        return true;
    }
    return QTextEdit::event(event);
}

} // namespace nebbie::qt
