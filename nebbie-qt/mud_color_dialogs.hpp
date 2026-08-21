#pragma once

#include "nebbie/mud_colors.hpp"

#include <QDialog>

namespace nebbie::qt {

class MudColorLegendDialog : public QDialog {
    Q_OBJECT

public:
    explicit MudColorLegendDialog(QWidget* parent = nullptr);
};

class MudColorInsertDialog : public QDialog {
    Q_OBJECT

public:
    explicit MudColorInsertDialog(QWidget* parent = nullptr);

    QString selectedCode() const { return selected_code_; }

private:
    QString selected_code_;
};

} // namespace nebbie::qt
