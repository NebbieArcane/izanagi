#include "flag_group_widget.hpp"

#include <QCheckBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QSizePolicy>
#include <algorithm>

FlagGroupWidget::FlagGroupWidget(const std::vector<nebbie::MobFlagDef>& defs,
                                 QWidget* parent,
                                 const int columns)
    : QWidget(parent), defs_(defs) {
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);

    // Tiny sets (align slayer, …) or explicit columns<=0 → one readable row.
    const bool use_row = columns <= 0 || defs_.size() <= 4;

    if (use_row) {
        auto* layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(16);
        for (const auto& def : defs_) {
            auto* box = new QCheckBox(QString::fromUtf8(def.label));
            box->setToolTip(QString::fromUtf8(def.name));
            layout->addWidget(box);
            boxes_.push_back(box);
            connect(box, &QCheckBox::toggled, this, [this](bool) { emit valueChanged(); });
        }
        layout->addStretch(1);
        return;
    }

    auto* layout = new QGridLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setHorizontalSpacing(12);
    layout->setVerticalSpacing(4);

    const int cols = std::max(1, columns);
    int row = 0;
    int col = 0;
    for (const auto& def : defs_) {
        auto* box = new QCheckBox(QString::fromUtf8(def.label));
        box->setToolTip(QString::fromUtf8(def.name));
        layout->addWidget(box, row, col);
        boxes_.push_back(box);
        connect(box, &QCheckBox::toggled, this, [this](bool) { emit valueChanged(); });
        ++col;
        if (col >= cols) {
            col = 0;
            ++row;
        }
    }
}

void FlagGroupWidget::setValue(const long flags) {
    long known_mask = 0;
    for (const nebbie::MobFlagDef& def : defs_) {
        known_mask |= def.value;
    }
    preserved_bits_ = flags & ~known_mask;

    const auto selected = nebbie::flags_selection_from_value(defs_, flags);
    for (std::size_t i = 0; i < boxes_.size() && i < selected.size(); ++i) {
        boxes_[i]->setChecked(selected[i]);
    }
}

long FlagGroupWidget::value() const {
    std::vector<bool> selected;
    selected.reserve(boxes_.size());
    for (const auto* box : boxes_) {
        selected.push_back(box->isChecked());
    }
    return preserved_bits_ | nebbie::flags_value_from_selection(defs_, selected);
}
