#include "flag_editor.hpp"

#include <QCheckBox>
#include <QGridLayout>
#include <QSizePolicy>

FlagEditor::FlagEditor(QWidget* parent) : QWidget(parent) {
  layout_ = new QGridLayout(this);
  layout_->setContentsMargins(0, 0, 0, 0);
  layout_->setHorizontalSpacing(6);
  layout_->setVerticalSpacing(0);
  setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
}

void FlagEditor::setFlags(const std::vector<eleutheria::FlagDef>& defs) {
  defs_ = defs;
  rebuild();
}

void FlagEditor::rebuild() {
  for(auto* box : boxes_) {
    layout_->removeWidget(box);
    box->deleteLater();
  }
  boxes_.clear();

  // Griglia compatta: piu' colonne, niente stretch verticale.
  constexpr int kColumns = 4;
  int row = 0;
  int col = 0;
  for(const auto& def : defs_) {
    auto* box = new QCheckBox(QString::fromStdString(def.label), this);
    box->setToolTip(QString::fromStdString(def.key + " (bit " + std::to_string(def.bit_index) + ")"));
    box->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    connect(box, &QCheckBox::toggled, this, [this](bool) {
      if(!updating_) {
        emit valueChanged(value());
      }
    });
    layout_->addWidget(box, row, col, Qt::AlignLeft | Qt::AlignTop);
    boxes_.push_back(box);
    ++col;
    if(col >= kColumns) {
      col = 0;
      ++row;
    }
  }
  for(int c = 0; c < kColumns; ++c) {
    layout_->setColumnStretch(c, 1);
  }
  layout_->setRowStretch(row + 1, 0);
}

void FlagEditor::setValue(std::uint64_t value) {
  updating_ = true;
  for(std::size_t i = 0; i < boxes_.size() && i < defs_.size(); ++i) {
    const auto bit = eleutheria::Catalog::bit(defs_[i].bit_index);
    boxes_[i]->setChecked((value & bit) != 0);
  }
  updating_ = false;
}

std::uint64_t FlagEditor::value() const {
  std::uint64_t v = 0;
  for(std::size_t i = 0; i < boxes_.size() && i < defs_.size(); ++i) {
    if(boxes_[i]->isChecked()) {
      v |= eleutheria::Catalog::bit(defs_[i].bit_index);
    }
  }
  return v;
}
