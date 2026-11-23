#pragma once

#include "eleutheria/types.hpp"

#include <QDialog>
#include <QString>
#include <QWidget>
#include <vector>

class QLineEdit;
class QTableWidget;
class QPushButton;
class LineLimitedPlainTextEdit;

/** Spezza le righe lunghe a max_len (default 79) senza spezzare le parole. */
QString wrap_mud_description(const QString& text, int max_len);

/** Dialog keywords + descrizione; al OK applica a capo automatici per il MUD. */
class ExtraDescDialog : public QDialog {
  Q_OBJECT
 public:
  explicit ExtraDescDialog(QWidget* parent = nullptr, int max_line_length = 0);

  void setExtra(const eleutheria::ExtraDesc& extra);
  eleutheria::ExtraDesc extra() const;

 private:
  QLineEdit* keywords_ = nullptr;
  LineLimitedPlainTextEdit* description_ = nullptr;
  int max_line_length_ = 0;
};

/**
 * Lista extra description (E) con Aggiungi / Modifica / Rimuovi.
 * Stile simile alle uscite: tabella + pulsanti, niente pipe manuali.
 */
class ExtraDescListEditor : public QWidget {
  Q_OBJECT
 public:
  explicit ExtraDescListEditor(QWidget* parent = nullptr);

  void setExtras(const std::vector<eleutheria::ExtraDesc>& extras);
  std::vector<eleutheria::ExtraDesc> extras() const;
  void setMaxLineLength(int max_length);
  void clear();

 signals:
  void edited();

 private slots:
  void addExtra();
  void editSelected();
  void removeSelected();

 private:
  int selectedRow() const;
  void setRowExtra(int row, const eleutheria::ExtraDesc& extra);

  QTableWidget* table_ = nullptr;
  QPushButton* add_btn_ = nullptr;
  QPushButton* edit_btn_ = nullptr;
  QPushButton* remove_btn_ = nullptr;
  int max_line_length_ = 0;
};
