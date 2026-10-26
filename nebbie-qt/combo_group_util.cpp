#include "combo_group_util.hpp"

#include <QBrush>
#include <QColor>
#include <QLineEdit>
#include <QListView>
#include <QSignalBlocker>
#include <QStandardItem>
#include <QStandardItemModel>

namespace nebbie::qt {
namespace {

constexpr int kCleanLabelRole = Qt::UserRole + 1;
constexpr char kSearchInstalledProp[] = "nebbieGroupedSearchInstalled";

QString itemIndent() {
    return QStringLiteral("    ");
}

void setRowHiddenInCombo(QComboBox* combo, const int row, const bool hidden) {
    if (!combo || row < 0 || row >= combo->count()) {
        return;
    }
    // QComboBox popup is a QListView; setRowHidden lives there (not on QAbstractItemView).
    if (auto* list = qobject_cast<QListView*>(combo->view())) {
        list->setRowHidden(row, hidden);
    }
}

void applyGroupedComboFilter(QComboBox* combo, const QString& needle_raw) {
    if (!combo) {
        return;
    }
    const QString needle = needle_raw.trimmed();
    const bool filtering = !needle.isEmpty();

    // First pass: item rows match against clean label (or display text).
    std::vector<bool> row_match(static_cast<std::size_t>(combo->count()), false);
    for (int i = 0; i < combo->count(); ++i) {
        if (!combo->itemData(i).isValid()) {
            continue; // header or separator
        }
        QString hay = combo->itemData(i, kCleanLabelRole).toString();
        if (hay.isEmpty()) {
            hay = combo->itemText(i).trimmed();
        }
        row_match[static_cast<std::size_t>(i)] =
            !filtering || hay.contains(needle, Qt::CaseInsensitive);
    }

    // Second pass: keep a header if any following item (until next header/sep) matches.
    // Keep a separator if the following section has any visible item.
    for (int i = 0; i < combo->count(); ++i) {
        const QVariant data = combo->itemData(i);
        if (data.isValid()) {
            setRowHiddenInCombo(combo, i, filtering && !row_match[static_cast<std::size_t>(i)]);
            continue;
        }

        const QString text = combo->itemText(i);
        const bool is_separator = text.isEmpty();
        bool section_has_match = false;
        for (int j = i + 1; j < combo->count(); ++j) {
            if (!combo->itemData(j).isValid()) {
                // Next header or separator ends this section.
                break;
            }
            if (row_match[static_cast<std::size_t>(j)]) {
                section_has_match = true;
                break;
            }
        }
        if (is_separator) {
            // Hide separator when not filtering, or when the next section is empty.
            setRowHiddenInCombo(combo, i, filtering && !section_has_match);
        } else {
            // Header: visible when not filtering, or when section has a match.
            setRowHiddenInCombo(combo, i, filtering && !section_has_match);
        }
    }
}

} // namespace

void selectFirstValidComboData(QComboBox* combo) {
    if (!combo) {
        return;
    }
    for (int i = 0; i < combo->count(); ++i) {
        if (combo->itemData(i).isValid()) {
            combo->setCurrentIndex(i);
            syncGroupedComboEditText(combo);
            return;
        }
    }
}

void syncGroupedComboEditText(QComboBox* combo) {
    if (!combo || !combo->isEditable() || combo->lineEdit() == nullptr) {
        return;
    }
    const QVariant clean = combo->currentData(kCleanLabelRole);
    if (clean.isValid()) {
        const QSignalBlocker block(combo->lineEdit());
        combo->setEditText(clean.toString());
        return;
    }
    if (combo->currentData().isValid()) {
        const QSignalBlocker block(combo->lineEdit());
        combo->setEditText(combo->currentText().trimmed());
    }
}

void fillGroupedIntCombo(QComboBox* combo,
                         const std::vector<ComboIntSection>& sections,
                         const std::function<QString(int)>& label_for) {
    if (!combo) {
        return;
    }
    combo->clear();
    auto* model = qobject_cast<QStandardItemModel*>(combo->model());
    bool first = true;
    for (const ComboIntSection& section : sections) {
        if (!first) {
            combo->insertSeparator(combo->count());
        }
        first = false;

        const int header_row = combo->count();
        combo->addItem(QStringLiteral("▸ %1").arg(QString::fromUtf8(section.title)));
        combo->setItemData(header_row, QVariant());
        if (model) {
            if (QStandardItem* item = model->item(header_row)) {
                item->setFlags(item->flags() & ~(Qt::ItemIsEnabled | Qt::ItemIsSelectable));
                item->setForeground(QBrush(QColor(110, 110, 110)));
            }
        }
        for (const int id : section.ids) {
            const QString clean = label_for(id);
            const int row = combo->count();
            combo->addItem(itemIndent() + clean, id);
            combo->setItemData(row, clean, kCleanLabelRole);
        }
    }
    selectFirstValidComboData(combo);
}

void enableGroupedComboSearch(QComboBox* combo, const QString& placeholder) {
    if (!combo) {
        return;
    }
    if (combo->property(kSearchInstalledProp).toBool()) {
        return;
    }
    combo->setProperty(kSearchInstalledProp, true);
    combo->setEditable(true);
    combo->setInsertPolicy(QComboBox::NoInsert);
    if (QLineEdit* edit = combo->lineEdit()) {
        edit->setPlaceholderText(placeholder.isEmpty()
                                     ? QStringLiteral("Cerca…")
                                     : placeholder);
        edit->setClearButtonEnabled(true);
        QObject::connect(edit, &QLineEdit::textEdited, combo, [combo](const QString& text) {
            applyGroupedComboFilter(combo, text);
            if (combo->view() && combo->view()->isVisible()) {
                // keep popup open while typing
            } else if (!text.trimmed().isEmpty()) {
                combo->showPopup();
            }
        });
        QObject::connect(edit, &QLineEdit::editingFinished, combo, [combo]() {
            // If the typed text does not match a real item, restore clean label.
            if (!combo->currentData().isValid()) {
                selectFirstValidComboData(combo);
                return;
            }
            syncGroupedComboEditText(combo);
            applyGroupedComboFilter(combo, {});
        });
    }
    QObject::connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), combo,
                     [combo](int) { syncGroupedComboEditText(combo); });
    QObject::connect(combo, QOverload<int>::of(&QComboBox::activated), combo, [combo](int) {
        syncGroupedComboEditText(combo);
        applyGroupedComboFilter(combo, {});
    });
    syncGroupedComboEditText(combo);
}

} // namespace nebbie::qt
