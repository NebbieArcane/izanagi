#include "line_limited_edit.hpp"

#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QScrollBar>
#include <QTextCursor>

namespace {

constexpr QColor kRulerOkColor(255, 245, 170, 180);
constexpr QColor kRulerExceededColor(220, 60, 60, 220);

int char_column_width(const QFontMetrics& metrics) {
  return metrics.horizontalAdvance(QLatin1Char('M'));
}

}  // namespace

LineLimitedPlainTextEdit::LineLimitedPlainTextEdit(QWidget* parent) : QPlainTextEdit(parent) {
  connect(this, &QPlainTextEdit::textChanged, this, [this]() {
    refreshRulerState();
    viewport()->update();
  });
  connect(horizontalScrollBar(), &QScrollBar::valueChanged, this, [this]() { viewport()->update(); });
}

void LineLimitedPlainTextEdit::setMaxLineLength(int max_length) {
  max_line_length_ = max_length < 0 ? 0 : max_length;
  refreshRulerState();
  viewport()->update();
}

void LineLimitedPlainTextEdit::refreshRulerState() {
  ruler_exceeded_ = anyLineExceedsLimit();
}

bool LineLimitedPlainTextEdit::anyLineExceedsLimit() const {
  if(max_line_length_ <= 0) {
    return false;
  }
  const QString text = toPlainText();
  int start = 0;
  while(start <= text.size()) {
    int end = text.indexOf(QLatin1Char('\n'), start);
    if(end < 0) {
      end = text.size();
    }
    if((end - start) > max_line_length_) {
      return true;
    }
    if(end >= text.size()) {
      break;
    }
    start = end + 1;
  }
  return false;
}

void LineLimitedPlainTextEdit::paintEvent(QPaintEvent* event) {
  QPlainTextEdit::paintEvent(event);
  if(max_line_length_ <= 0) {
    return;
  }

  QTextCursor cursor(document());
  cursor.movePosition(QTextCursor::Start);
  const int left = cursorRect(cursor).left();
  const QFontMetrics metrics(font());
  const int x = left + max_line_length_ * char_column_width(metrics);

  QPainter painter(viewport());
  painter.setRenderHint(QPainter::Antialiasing, false);
  QPen pen(ruler_exceeded_ ? kRulerExceededColor : kRulerOkColor, 2, Qt::SolidLine);
  painter.setPen(pen);
  painter.drawLine(x, 0, x, viewport()->height());
}

void LineLimitedPlainTextEdit::resizeEvent(QResizeEvent* event) {
  QPlainTextEdit::resizeEvent(event);
  viewport()->update();
}

void configure_mud_text_field(LineLimitedPlainTextEdit* field) {
  if(field == nullptr) {
    return;
  }
  field->setFont(QFont(QStringLiteral("Courier New"), 10));
  field->setLineWrapMode(QPlainTextEdit::NoWrap);
  field->setTabStopDistance(4 * char_column_width(QFontMetrics(field->font())));
}
