#include "entity_picker_dialog.hpp"

#include "nebbie/edit.hpp"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QVBoxLayout>

#include <cctype>

namespace {

bool matches_filter(long vnum, const std::string& text, const QString& filter) {
    if (filter.isEmpty()) {
        return true;
    }
    return nebbie::entity_matches(vnum, text, filter.toStdString());
}

} // namespace

EntityPickerDialog::EntityPickerDialog(const EntityPickerKind kind, const nebbie::World& world,
                                       QWidget* parent)
    : QDialog(parent), kind_(kind), world_(world) {
    setWindowTitle(kind_ == EntityPickerKind::Mobile   ? QStringLiteral("Choose mobile")
                   : kind_ == EntityPickerKind::Object ? QStringLiteral("Choose object")
                                                       : QStringLiteral("Choose room"));
    resize(420, 480);

    auto* root = new QVBoxLayout(this);
    auto* find_row = new QHBoxLayout;
    find_row->addWidget(new QLabel(QStringLiteral("Find:")));
    filter_ = new QLineEdit;
    find_row->addWidget(filter_, 1);
    root->addLayout(find_row);

    list_ = new QListWidget;
    root->addWidget(list_, 1);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    root->addWidget(buttons);

    connect(filter_, &QLineEdit::textChanged, this, [this](const QString& text) { rebuildList(text); });
    connect(list_, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem*) { accept(); });
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    rebuildList({});
}

void EntityPickerDialog::rebuildList(const QString& filter) {
    list_->clear();
    if (kind_ == EntityPickerKind::Mobile) {
        for (const auto& [vnum, mob] : world_.mobiles) {
            if (!matches_filter(vnum, mob.short_descr, filter)) {
                continue;
            }
            auto* item = new QListWidgetItem(
                QString("#%1 %2").arg(vnum).arg(QString::fromStdString(mob.short_descr)));
            item->setData(Qt::UserRole, static_cast<qlonglong>(vnum));
            list_->addItem(item);
        }
    } else if (kind_ == EntityPickerKind::Object) {
        for (const auto& [vnum, obj] : world_.objects) {
            if (!matches_filter(vnum, obj.short_descr, filter)) {
                continue;
            }
            auto* item = new QListWidgetItem(
                QString("#%1 %2").arg(vnum).arg(QString::fromStdString(obj.short_descr)));
            item->setData(Qt::UserRole, static_cast<qlonglong>(vnum));
            list_->addItem(item);
        }
    } else {
        for (const auto& [vnum, room] : world_.rooms) {
            if (!matches_filter(vnum, room.name, filter)) {
                continue;
            }
            auto* item =
                new QListWidgetItem(QString("#%1 %2").arg(vnum).arg(QString::fromStdString(room.name)));
            item->setData(Qt::UserRole, static_cast<qlonglong>(vnum));
            list_->addItem(item);
        }
    }
    if (list_->count() > 0) {
        list_->setCurrentRow(0);
    }
}

std::optional<long> EntityPickerDialog::selectedVnum() const {
    const QListWidgetItem* item = list_->currentItem();
    if (!item) {
        return std::nullopt;
    }
    return static_cast<long>(item->data(Qt::UserRole).toLongLong());
}
