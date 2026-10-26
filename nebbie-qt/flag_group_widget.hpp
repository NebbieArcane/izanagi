#pragma once

#include "nebbie/mob_catalog.hpp"

#include <QWidget>

class QCheckBox;
class QGridLayout;

class FlagGroupWidget : public QWidget {
    Q_OBJECT

public:
    /**
     * @param columns Grid columns for checkboxes. Use 0 (or negative) to put
     *        every flag on a single horizontal row (compact UX for small sets).
     *        Default 2 matches the historical two-column layout.
     */
    explicit FlagGroupWidget(const std::vector<nebbie::MobFlagDef>& defs,
                             QWidget* parent = nullptr,
                             int columns = 2);

    void setValue(long flags);
    long value() const;

signals:
    void valueChanged();

private:
    std::vector<nebbie::MobFlagDef> defs_;
    std::vector<QCheckBox*> boxes_;
    /** Bits set in file but not represented in {@link defs_} (must survive editor round-trip). */
    long preserved_bits_ = 0;
};
