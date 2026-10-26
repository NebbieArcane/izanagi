#pragma once

#include "nebbie/mob_catalog.hpp"

#include <QWidget>

#include <vector>

class QCheckBox;
class QListWidget;
class QPushButton;

class FlagGroupWidget : public QWidget {
    Q_OBJECT

public:
    enum class Mode {
        /** Checkbox grid/row (default). */
        Checkboxes,
        /** SMAUG-style Available | Set dual list. */
        DualList,
    };

    /**
     * @param columns Grid columns for checkboxes. Use 0 (or negative) to put
     *        every flag on a single horizontal row (compact UX for small sets).
     *        Default 2 matches the historical two-column layout.
     *        Ignored when mode is DualList.
     */
    explicit FlagGroupWidget(const std::vector<nebbie::MobFlagDef>& defs,
                             QWidget* parent = nullptr,
                             int columns = 2,
                             Mode mode = Mode::Checkboxes);

    void setValue(long flags);
    long value() const;

signals:
    void valueChanged();

private:
    void rebuildDualLists();
    void moveSelected(QListWidget* from, bool to_set);

    std::vector<nebbie::MobFlagDef> defs_;
    Mode mode_ = Mode::Checkboxes;
    std::vector<QCheckBox*> boxes_;
    QListWidget* available_list_ = nullptr;
    QListWidget* set_list_ = nullptr;
    QPushButton* add_button_ = nullptr;
    QPushButton* remove_button_ = nullptr;
    std::vector<bool> dual_selected_;
    /** Bits set in file but not represented in {@link defs_} (must survive editor round-trip). */
    long preserved_bits_ = 0;
};
