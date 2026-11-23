#include "extra_desc_editor.hpp"

#include "line_limited_edit.hpp"

#include <QAbstractItemView>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

namespace {

constexpr int kDefaultMudLineLength = 79;

QString preview_text(const QString& description) {
  QString one_line = description;
  one_line.replace(QLatin1Char('\n'), QLatin1Char(' '));
  one_line = one_line.simplified();
  if(one_line.size() > 72) {
    return one_line.left(69) + QStringLiteral("...");
  }
  return one_line;
}

}  // namespace

QString wrap_mud_description(const QString& text, int max_len) {
  const int limit = max_len > 0 ? max_len : kDefaultMudLineLength;
  QString normalized = text;
  normalized.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
  normalized.replace(QLatin1Char('\r'), QLatin1Char('\n'));
  QStringList out_lines;
  const QStringList paragraphs = normalized.split(QLatin1Char('\n'));
  for(const QString& paragraph : paragraphs) {
    QString trimmed_right = paragraph;
    trimmed_right.replace(QChar(0x00a0), QLatin1Char(' '));
    if(trimmed_right.trimmed().isEmpty()) {
      out_lines << QString();
      continue;
    }
    const QStringList words = trimmed_right.split(QRegularExpression(QStringLiteral("\\s+")),
                                                  Qt::SkipEmptyParts);
    QString current;
    for(const QString& word : words) {
      if(current.isEmpty()) {
        current = word;
        continue;
      }
      if(current.size() + 1 + word.size() <= limit) {
        current += QLatin1Char(' ');
        current += word;
      } else {
        out_lines << current;
        current = word;
      }
    }
    if(!current.isEmpty()) {
      out_lines << current;
    }
  }
  while(!out_lines.isEmpty() && out_lines.back().isEmpty()) {
    out_lines.removeLast();
  }
  return out_lines.join(QLatin1Char('\n'));
}

ExtraDescDialog::ExtraDescDialog(QWidget* parent, int max_line_length)
    : QDialog(parent), max_line_length_(max_line_length) {
  setWindowTitle(QStringLiteral("Extra description"));
  setModal(true);
  resize(520, 360);

  auto* root = new QVBoxLayout(this);
  auto* form = new QFormLayout();
  keywords_ = new QLineEdit(this);
  keywords_->setPlaceholderText(QStringLiteral("es. alberi secolari  (parole per look/examine)"));
  description_ = new LineLimitedPlainTextEdit(this);
  configure_mud_text_field(description_);
  description_->setMaxLineLength(max_line_length_ > 0 ? max_line_length_ : kDefaultMudLineLength);
  description_->setPlaceholderText(
      QStringLiteral("Testo mostrato nel MUD. A capo automatici al conferma."));
  form->addRow(QStringLiteral("Keywords"), keywords_);
  form->addRow(QStringLiteral("Descrizione"), description_);
  root->addLayout(form, 1);
  root->addWidget(new QLabel(
      QStringLiteral(
          "Le keywords sono le parole con cui il giocatore fa look/examine. "
          "La descrizione viene spezzata automaticamente alle %1 colonne.")
          .arg(max_line_length_ > 0 ? max_line_length_ : kDefaultMudLineLength),
      this));

  auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  root->addWidget(buttons);
  connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
    if(keywords_->text().trimmed().isEmpty()) {
      QMessageBox::warning(this, QStringLiteral("Extra description"),
                           QStringLiteral("Inserisci almeno una keyword."));
      return;
    }
    accept();
  });
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void ExtraDescDialog::setExtra(const eleutheria::ExtraDesc& extra) {
  keywords_->setText(QString::fromStdString(extra.keywords));
  description_->setPlainText(QString::fromStdString(extra.description));
}

eleutheria::ExtraDesc ExtraDescDialog::extra() const {
  eleutheria::ExtraDesc ed;
  ed.keywords = keywords_->text().trimmed().toStdString();
  const QString wrapped =
      wrap_mud_description(description_->toPlainText(), max_line_length_);
  ed.description = wrapped.toStdString();
  return ed;
}

ExtraDescListEditor::ExtraDescListEditor(QWidget* parent) : QWidget(parent) {
  auto* root = new QVBoxLayout(this);
  root->setContentsMargins(0, 0, 0, 0);
  root->addWidget(new QLabel(
      QStringLiteral(
          "Extra description (E): parole chiave per look/examine e testo associato."),
      this));

  table_ = new QTableWidget(0, 2, this);
  table_->setHorizontalHeaderLabels(
      {QStringLiteral("Keywords"), QStringLiteral("Descrizione")});
  table_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
  table_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
  table_->verticalHeader()->setVisible(false);
  table_->setSelectionBehavior(QAbstractItemView::SelectRows);
  table_->setSelectionMode(QAbstractItemView::SingleSelection);
  table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  table_->setMinimumHeight(140);
  root->addWidget(table_, 1);

  auto* btns = new QHBoxLayout();
  add_btn_ = new QPushButton(QStringLiteral("Aggiungi extra..."), this);
  edit_btn_ = new QPushButton(QStringLiteral("Modifica..."), this);
  remove_btn_ = new QPushButton(QStringLiteral("Rimuovi"), this);
  btns->addWidget(add_btn_);
  btns->addWidget(edit_btn_);
  btns->addWidget(remove_btn_);
  btns->addStretch();
  root->addLayout(btns);

  connect(add_btn_, &QPushButton::clicked, this, &ExtraDescListEditor::addExtra);
  connect(edit_btn_, &QPushButton::clicked, this, &ExtraDescListEditor::editSelected);
  connect(remove_btn_, &QPushButton::clicked, this, &ExtraDescListEditor::removeSelected);
  connect(table_, &QTableWidget::cellDoubleClicked, this, [this](int, int) { editSelected(); });
}

void ExtraDescListEditor::setMaxLineLength(int max_length) {
  max_line_length_ = max_length < 0 ? 0 : max_length;
}

void ExtraDescListEditor::clear() { setExtras({}); }

void ExtraDescListEditor::setRowExtra(int row, const eleutheria::ExtraDesc& extra) {
  auto* key_item = new QTableWidgetItem(QString::fromStdString(extra.keywords));
  auto* desc_item =
      new QTableWidgetItem(preview_text(QString::fromStdString(extra.description)));
  key_item->setData(Qt::UserRole, QString::fromStdString(extra.keywords));
  key_item->setData(Qt::UserRole + 1, QString::fromStdString(extra.description));
  table_->setItem(row, 0, key_item);
  table_->setItem(row, 1, desc_item);
}

void ExtraDescListEditor::setExtras(const std::vector<eleutheria::ExtraDesc>& extras) {
  table_->setRowCount(0);
  for(const auto& ed : extras) {
    const int row = table_->rowCount();
    table_->insertRow(row);
    setRowExtra(row, ed);
  }
}

std::vector<eleutheria::ExtraDesc> ExtraDescListEditor::extras() const {
  std::vector<eleutheria::ExtraDesc> out;
  out.reserve(static_cast<std::size_t>(table_->rowCount()));
  for(int row = 0; row < table_->rowCount(); ++row) {
    auto* key_item = table_->item(row, 0);
    if(key_item == nullptr) {
      continue;
    }
    eleutheria::ExtraDesc ed;
    ed.keywords = key_item->data(Qt::UserRole).toString().toStdString();
    ed.description = key_item->data(Qt::UserRole + 1).toString().toStdString();
    if(ed.keywords.empty()) {
      ed.keywords = key_item->text().toStdString();
    }
    out.push_back(std::move(ed));
  }
  return out;
}

int ExtraDescListEditor::selectedRow() const { return table_->currentRow(); }

void ExtraDescListEditor::addExtra() {
  ExtraDescDialog dialog(this, max_line_length_);
  if(dialog.exec() != QDialog::Accepted) {
    return;
  }
  const auto ed = dialog.extra();
  const int row = table_->rowCount();
  table_->insertRow(row);
  setRowExtra(row, ed);
  table_->selectRow(row);
  emit edited();
}

void ExtraDescListEditor::editSelected() {
  const int row = selectedRow();
  if(row < 0) {
    QMessageBox::information(this, QStringLiteral("Extra description"),
                             QStringLiteral("Seleziona prima una extra da modificare."));
    return;
  }
  auto* key_item = table_->item(row, 0);
  if(key_item == nullptr) {
    return;
  }
  eleutheria::ExtraDesc current;
  current.keywords = key_item->data(Qt::UserRole).toString().toStdString();
  current.description = key_item->data(Qt::UserRole + 1).toString().toStdString();
  ExtraDescDialog dialog(this, max_line_length_);
  dialog.setExtra(current);
  if(dialog.exec() != QDialog::Accepted) {
    return;
  }
  setRowExtra(row, dialog.extra());
  emit edited();
}

void ExtraDescListEditor::removeSelected() {
  const int row = selectedRow();
  if(row < 0) {
    QMessageBox::information(this, QStringLiteral("Extra description"),
                             QStringLiteral("Seleziona prima una extra da rimuovere."));
    return;
  }
  table_->removeRow(row);
  emit edited();
}
