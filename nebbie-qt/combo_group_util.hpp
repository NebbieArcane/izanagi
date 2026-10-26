#pragma once

#include <QComboBox>
#include <QString>

#include <functional>
#include <vector>

namespace nebbie::qt {

struct ComboIntSection {
    const char* title = nullptr;
    std::vector<int> ids;
};

/**
 * Fill an int-valued QComboBox with section headers (non-selectable) and
 * separators. Item labels are indented under their section header.
 *
 * Item roles:
 * - Qt::DisplayRole: indented label (popup)
 * - Qt::UserRole: int id
 * - Qt::UserRole+1: clean label (line-edit / search)
 */
void fillGroupedIntCombo(QComboBox* combo,
                         const std::vector<ComboIntSection>& sections,
                         const std::function<QString(int)>& label_for);

/** Select first item that has valid userData (skips section headers). */
void selectFirstValidComboData(QComboBox* combo);

/**
 * Make a grouped combo searchable: editable line + filter that hides
 * non-matching rows in the popup (headers stay if a child matches).
 * Call after fillGroupedIntCombo.
 */
void enableGroupedComboSearch(QComboBox* combo, const QString& placeholder = QString());

/** Sync editable line text to the clean (non-indented) label of the current item. */
void syncGroupedComboEditText(QComboBox* combo);

} // namespace nebbie::qt
