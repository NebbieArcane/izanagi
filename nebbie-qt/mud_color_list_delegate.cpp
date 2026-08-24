#include "mud_color_list_delegate.hpp"

#include "mud_color_common.hpp"

#include "nebbie/mud_colors.hpp"

#include <QApplication>
#include <QPainter>
#include <QStyle>
#include <QListWidgetItem>

namespace nebbie::qt {

namespace {

QString room_list_prefix(long vnum) {
    return QString("#%1 ").arg(vnum);
}

void paint_mud_text(QPainter* painter,
                    const QRect& rect,
                    const QString& prefix,
                    const std::string& storage,
                    bool show_color_codes,
                    const QPalette& palette,
                    bool selected) {
    const QFont font = painter->font();
    QFontMetrics metrics(font);
    int x = rect.x() + 4;
    const int y = rect.y() + (rect.height() + metrics.ascent() - metrics.descent()) / 2;
    const int max_x = rect.right() - 2;

    auto draw_segment = [&](const QString& text, const QColor& color) {
        if (text.isEmpty() || x > max_x) {
            return;
        }
        painter->setPen(color);
        painter->drawText(x, y, text);
        x += metrics.horizontalAdvance(text);
    };

    const QColor default_color =
        selected ? palette.color(QPalette::HighlightedText) : palette.color(QPalette::Text);
    draw_segment(prefix, default_color);

    MudDisplayColorState state;
    const auto tokens = nebbie::tokenize_mud_colored_text(storage);
    for (const auto& token : tokens) {
        if (token.type == nebbie::MudColorToken::Type::color_code) {
            if (show_color_codes) {
                draw_segment(QString::fromUtf8(token.text.data(), static_cast<int>(token.text.size())),
                             selected ? default_color.darker(115) : QColor(120, 120, 120));
            } else {
                state.foreground = token.code.foreground;
                state.bold = token.code.bold;
                state.modifier = token.code.modifier;
            }
            continue;
        }

        const QString chunk = QString::fromUtf8(token.text.data(), static_cast<int>(token.text.size()));
        if (show_color_codes) {
            draw_segment(chunk, default_color);
        } else {
            draw_segment(chunk, mud_foreground_color(state.foreground, state.bold != 0));
        }
    }
}

} // namespace

MudColorListDelegate::MudColorListDelegate(QObject* parent) : QStyledItemDelegate(parent) {}

void MudColorListDelegate::setShowColorCodes(bool show) {
    if (show_color_codes_ == show) {
        return;
    }
    show_color_codes_ = show;
}

void MudColorListDelegate::paint(QPainter* painter,
                                 const QStyleOptionViewItem& option,
                                 const QModelIndex& index) const {
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    opt.text.clear();

    const QWidget* widget = opt.widget;
    QStyle* style = widget ? widget->style() : QApplication::style();
    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, widget);

    const long vnum = index.data(Qt::UserRole).toLongLong();
    const QString storage_q = index.data(kMudStorageTextRole).toString();
    if (vnum <= 0 || storage_q.isNull()) {
        return;
    }

    painter->save();
    painter->setFont(opt.font);
    const QRect text_rect = style->subElementRect(QStyle::SE_ItemViewItemText, &opt, widget);
    paint_mud_text(painter,
                   text_rect,
                   room_list_prefix(vnum),
                   storage_q.toStdString(),
                   show_color_codes_,
                   opt.palette,
                   (opt.state & QStyle::State_Selected) != 0);
    painter->restore();
}

QSize MudColorListDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    const long vnum = index.data(Qt::UserRole).toLongLong();
    const QString storage_q = index.data(kMudStorageTextRole).toString();
    const QString label = storage_q.isNull()
                              ? index.data(Qt::DisplayRole).toString()
                              : room_list_prefix(vnum) + build_display_text(storage_q, show_color_codes_);
    QFontMetrics metrics(opt.font);
    const int height = metrics.height() + 8;
    const int width = metrics.horizontalAdvance(label) + 16;
    return {width, height};
}

void setRoomListItemData(QListWidgetItem* item, long vnum, const std::string& storage_name) {
    if (item == nullptr) {
        return;
    }
    item->setData(Qt::UserRole, static_cast<qlonglong>(vnum));
    item->setData(kMudStorageTextRole, QString::fromStdString(storage_name));
    item->setText(mud_entity_list_label(vnum, storage_name));
}

void refreshRoomListItemData(QListWidgetItem* item, long vnum, const std::string& storage_name) {
    setRoomListItemData(item, vnum, storage_name);
}

} // namespace nebbie::qt
