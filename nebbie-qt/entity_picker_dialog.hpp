#pragma once

#include "nebbie/world.hpp"

#include <QDialog>

#include <optional>

class QLineEdit;
class QListWidget;

enum class EntityPickerKind {
    Mobile,
    Object,
    Room,
};

class EntityPickerDialog : public QDialog {
    Q_OBJECT

public:
    EntityPickerDialog(EntityPickerKind kind, const nebbie::World& world, QWidget* parent = nullptr);

    std::optional<long> selectedVnum() const;

private:
    void rebuildList(const QString& filter);

    EntityPickerKind kind_;
    const nebbie::World& world_;
    QLineEdit* filter_ = nullptr;
    QListWidget* list_ = nullptr;
};
