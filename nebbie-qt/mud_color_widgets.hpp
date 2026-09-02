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

    void insertColorCode(const QString& code);
    void pastePlainText();

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
    void insertStorageTextAtCursor(const QString& text);
    void rebuildDocument(int preferred_storage_cursor = -1);
    void updateRulerState();
    QTextCharFormat formatForState(const MudDisplayColorState& state) const;
    MudDisplayColorState color_at_storage_position(int storage_pos) const;
    QString tooltipForStoragePosition(int storage_pos) const;

    QString storage_;
    QString previous_display_;
    bool show_color_codes_ = false;
    bool single_line_mode_ = false;
    int max_line_length_ = 0;
    bool ruler_exceeded_ = false;
    bool rebuilding_ = false;
};

void configureMudDescriptionField(MudColorTextEdit* field);
void configureMudSingleLineField(MudColorTextEdit* field);

} // namespace nebbie::qt
