#pragma once

#include "mud_color_common.hpp"

#include <QMimeData>
#include <QTextEdit>

namespace nebbie::qt {

class MudColorTextEdit : public QTextEdit {
    Q_OBJECT

public:
    explicit MudColorTextEdit(QWidget* parent = nullptr);

    QString storageText() const { return storage_; }
    void setStorageText(const QString& text);
    void setShowColorCodes(bool show);
    bool showColorCodes() const { return show_color_codes_; }
    void setMaxLineLength(int max_length);
    int maxLineLength() const { return max_line_length_; }
    void setSingleLineMode(bool single_line);
    bool isSingleLineMode() const { return single_line_mode_; }
    /**
     * When true, a final empty line is expected (OnOwnLine: text then \\n then ~) and shown green.
     * When false on multiline fields, a trailing empty line is warned in red (Inline: text~).
     */
    void setRequireTrailingNewline(bool required);
    bool requireTrailingNewline() const { return require_trailing_newline_; }
    /** Refresh the field tooltip that explains green/red ↵ for this tilde policy. */
    void refreshTrailingNewlineToolTip();

    void insertColorCode(const QString& code);
    void pastePlainText();
    /** Accents → a'/e'/… ; curly apostrophes → ' ; returns true if storage changed. */
    bool normalizeMudAsciiText();

signals:
    void storageTextChanged(const QString& text);

protected:
    void paintEvent(QPaintEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    bool event(QEvent* event) override;
    void insertFromMimeData(const QMimeData* source) override;

private slots:
    void onContentsChanged(int position, int charsRemoved, int charsAdded);

private:
    void replaceSelectionWithStorageText(const QString& text);
    void insertStorageTextAtCursor(const QString& text);
    void rebuildDocument(int preferred_storage_cursor = -1);
    void updateRulerState();
    void paintNewlineGuides(QPainter& painter) const;
    QTextCharFormat formatForState(const MudDisplayColorState& state) const;
    QTextCharFormat formatForDisplayedChar(const MudDisplayColorState& state, QChar ch) const;
    MudDisplayColorState color_at_storage_position(int storage_pos) const;
    QString tooltipForStoragePosition(int storage_pos) const;

    QString storage_;
    QString previous_display_;
    bool show_color_codes_ = false;
    bool single_line_mode_ = false;
    bool require_trailing_newline_ = false;
    int max_line_length_ = 0;
    bool ruler_exceeded_ = false;
    bool rebuilding_ = false;
};

void configureMudDescriptionField(MudColorTextEdit* field);
void configureMudSingleLineField(MudColorTextEdit* field);
/** Multiline field that always uses OnOwnLine tilde (e.g. mob long_descr). */
void configureMudOnOwnLineField(MudColorTextEdit* field, int min_height);
/**
 * Inline if the text has no '\\n', OnOwnLine (required trailing newline) if it has paragraphs
 * — matches nebbie_paragraph_tilde_style.
 */
void syncMudParagraphTrailingNewlinePolicy(MudColorTextEdit* field);

} // namespace nebbie::qt
