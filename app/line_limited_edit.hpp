#pragma once

#include <QPlainTextEdit>

/** QPlainTextEdit con barra verticale di limite caratteri per riga (stile Izanagi). */
class LineLimitedPlainTextEdit : public QPlainTextEdit {
  Q_OBJECT
 public:
  explicit LineLimitedPlainTextEdit(QWidget* parent = nullptr);

  void setMaxLineLength(int max_length);
  int maxLineLength() const { return max_line_length_; }

 protected:
  void paintEvent(QPaintEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;

 private:
  void refreshRulerState();
  bool anyLineExceedsLimit() const;

  int max_line_length_ = 0;
  bool ruler_exceeded_ = false;
};

void configure_mud_text_field(LineLimitedPlainTextEdit* field);
