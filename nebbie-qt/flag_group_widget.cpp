#include "flag_group_widget.hpp"

#include <QCheckBox>
#include <QGridLayout>

FlagGroupWidget::FlagGroupWidget(const std::vector<nebbie::MobFlagDef>& defs, QWidget* parent)
    : QWidget(parent), defs_(defs) {
    auto* layout = new QGridLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    int row = 0;
    int col = 0;
    for (const auto& def : defs_) {
        auto* box = new QCheckBox(QString::fromUtf8(def.label));
        box->setToolTip(QString::fromUtf8(def.name));
        layout->addWidget(box, row, col);
        boxes_.push_back(box);
        connect(box, &QCheckBox::toggled, this, [this](bool) { emit valueChanged(); });
        ++col;
        if (col >= 2) {
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
