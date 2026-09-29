#pragma once

#include "nebbie/mob_catalog.hpp"

#include <QWidget>

class QCheckBox;
class QGridLayout;

class FlagGroupWidget : public QWidget {
    Q_OBJECT

public:
    explicit FlagGroupWidget(const std::vector<nebbie::MobFlagDef>& defs, QWidget* parent = nullptr);

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
