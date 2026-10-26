#include "flag_group_widget.hpp"

#include <QAbstractItemView>
#include <QCheckBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QSizePolicy>
#include <QVBoxLayout>

#include <algorithm>

FlagGroupWidget::FlagGroupWidget(const std::vector<nebbie::MobFlagDef>& defs,
                                 QWidget* parent,
                                 const int columns,
                                 const Mode mode)
    : QWidget(parent), defs_(defs), mode_(mode) {
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    dual_selected_.assign(defs_.size(), false);

    if (mode_ == Mode::DualList && !defs_.empty()) {
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
        auto* root = new QHBoxLayout(this);
        root->setContentsMargins(0, 0, 0, 0);
        root->setSpacing(8);

        // SMAUG Area Editor layout: Set | << Insert / Delete >> | Not set
        auto* set_col = new QVBoxLayout;
        set_col->addWidget(new QLabel(QStringLiteral("Set")));
        set_list_ = new QListWidget;
        set_list_->setSelectionMode(QAbstractItemView::ExtendedSelection);
        set_list_->setMinimumHeight(180);
        set_col->addWidget(set_list_, 1);

        auto* buttons = new QVBoxLayout;
        buttons->addStretch(1);
        add_button_ = new QPushButton(QStringLiteral("<< Insert"));
        remove_button_ = new QPushButton(QStringLiteral("Delete >>"));
        buttons->addWidget(add_button_);
        buttons->addWidget(remove_button_);
        buttons->addStretch(1);

        auto* available_col = new QVBoxLayout;
        available_col->addWidget(new QLabel(QStringLiteral("Not set")));
        available_list_ = new QListWidget;
        available_list_->setSelectionMode(QAbstractItemView::ExtendedSelection);
        available_list_->setMinimumHeight(180);
        available_col->addWidget(available_list_, 1);

        root->addLayout(set_col, 1);
        root->addLayout(buttons);
        root->addLayout(available_col, 1);

        connect(add_button_, &QPushButton::clicked, this, [this]() {
            moveSelected(available_list_, true);
        });
        connect(remove_button_, &QPushButton::clicked, this, [this]() {
            moveSelected(set_list_, false);
        });
        connect(available_list_, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem*) {
            moveSelected(available_list_, true);
        });
        connect(set_list_, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem*) {
            moveSelected(set_list_, false);
        });
        rebuildDualLists();
        return;
    }

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

void FlagGroupWidget::rebuildDualLists() {
    if (!available_list_ || !set_list_) {
        return;
    }
    available_list_->clear();
    set_list_->clear();
    for (std::size_t i = 0; i < defs_.size(); ++i) {
        // Numbered like SMAUG ("01. Npc") for quick scanning.
        const QString label =
            QStringLiteral("%1. %2")
                .arg(static_cast<int>(i) + 1, 2, 10, QLatin1Char('0'))
                .arg(QString::fromUtf8(defs_[i].label));
        auto* item = new QListWidgetItem(label);
        item->setData(Qt::UserRole, static_cast<qlonglong>(i));
        item->setToolTip(QString::fromUtf8(defs_[i].name));
        if (i < dual_selected_.size() && dual_selected_[i]) {
            set_list_->addItem(item);
        } else {
            available_list_->addItem(item);
        }
    }
}

void FlagGroupWidget::moveSelected(QListWidget* from, const bool to_set) {
    if (!from) {
        return;
    }
    const QList<QListWidgetItem*> selected = from->selectedItems();
    if (selected.isEmpty()) {
        return;
    }
    bool changed = false;
    for (QListWidgetItem* item : selected) {
        const int index = static_cast<int>(item->data(Qt::UserRole).toLongLong());
        if (index < 0 || index >= static_cast<int>(dual_selected_.size())) {
            continue;
        }
        if (dual_selected_[static_cast<std::size_t>(index)] != to_set) {
            dual_selected_[static_cast<std::size_t>(index)] = to_set;
            changed = true;
        }
    }
    if (changed) {
        rebuildDualLists();
        emit valueChanged();
    }
}

void FlagGroupWidget::setValue(const long flags) {
    long known_mask = 0;
    for (const nebbie::MobFlagDef& def : defs_) {
        known_mask |= def.value;
    }
    preserved_bits_ = flags & ~known_mask;

    const auto selected = nebbie::flags_selection_from_value(defs_, flags);
    if (mode_ == Mode::DualList) {
        dual_selected_ = selected;
        if (dual_selected_.size() < defs_.size()) {
            dual_selected_.resize(defs_.size(), false);
        }
        rebuildDualLists();
        return;
    }

    for (std::size_t i = 0; i < boxes_.size() && i < selected.size(); ++i) {
        boxes_[i]->setChecked(selected[i]);
    }
}

long FlagGroupWidget::value() const {
    if (mode_ == Mode::DualList) {
        return preserved_bits_ | nebbie::flags_value_from_selection(defs_, dual_selected_);
    }
    std::vector<bool> selected;
    selected.reserve(boxes_.size());
    for (const auto* box : boxes_) {
        selected.push_back(box->isChecked());
    }
    return preserved_bits_ | nebbie::flags_value_from_selection(defs_, selected);
}
