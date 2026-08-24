#pragma once

#include <QStyledItemDelegate>

class QListWidgetItem;

namespace nebbie::qt {

constexpr int kMudStorageTextRole = Qt::UserRole + 1;

class MudColorListDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
    explicit MudColorListDelegate(QObject* parent = nullptr);

    void setShowColorCodes(bool show);
    bool showColorCodes() const { return show_color_codes_; }

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    bool show_color_codes_ = false;
};

void setRoomListItemData(QListWidgetItem* item, long vnum, const std::string& storage_name);
void refreshRoomListItemData(QListWidgetItem* item, long vnum, const std::string& storage_name);

} // namespace nebbie::qt
