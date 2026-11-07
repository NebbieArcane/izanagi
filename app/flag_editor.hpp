#pragma once

#include "eleutheria/catalog.hpp"

#include <QWidget>
#include <cstdint>
#include <vector>

class QCheckBox;
class QGridLayout;

class FlagEditor : public QWidget {
  Q_OBJECT
 public:
  explicit FlagEditor(QWidget* parent = nullptr);

  void setFlags(const std::vector<eleutheria::FlagDef>& defs);
  void setValue(std::uint64_t value);
  std::uint64_t value() const;

 signals:
  void valueChanged(quint64 value);

 private:
  void rebuild();

  std::vector<eleutheria::FlagDef> defs_;
  std::vector<QCheckBox*> boxes_;
  QGridLayout* layout_ = nullptr;
  bool updating_ = false;
};
