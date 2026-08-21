#pragma once

#include "nebbie/mud_colors.hpp"

#include <QColor>
#include <QString>

namespace nebbie::qt {

struct MudDisplayColorState {
    int foreground = 7;
    int bold = 0;
    int modifier = 0;
};

inline QColor mud_foreground_color(int color, bool bold) {
    static const QColor palette[] = {
        QColor(),
        QColor(170, 0, 0),
        QColor(0, 140, 0),
        QColor(160, 90, 0),
        QColor(0, 0, 170),
        QColor(170, 0, 170),
        QColor(0, 140, 140),
        QColor(60, 60, 60),
        QColor(110, 110, 110),
        QColor(210, 40, 40),
        QColor(40, 170, 40),
        QColor(200, 160, 0),
        QColor(70, 70, 220),
        QColor(180, 70, 180),
        QColor(60, 170, 200),
        QColor(20, 20, 20),
    };
    const int index = (color >= 1 && color <= 15) ? color : 7;
    QColor result = palette[index];
    if (bold) {
        result = result.darker(80);
    }
    return result;
}

inline int display_position_to_storage(const QString& storage_text, int display_pos, bool show_codes) {
    int display = 0;
    int storage_index = 0;
    const std::string bytes = storage_text.toStdString();
    while (storage_index < storage_text.size() && display < display_pos) {
        const std::size_t code_len = nebbie::mud_color_code_length(bytes, static_cast<std::size_t>(storage_index));
        if (code_len > 0) {
            storage_index += static_cast<int>(code_len);
            if (show_codes) {
                display += static_cast<int>(code_len);
            }
            continue;
        }
        ++storage_index;
        ++display;
    }
    return storage_index;
}

inline int storage_position_to_display(const QString& storage_text, int storage_pos, bool show_codes) {
    int display = 0;
    int index = 0;
    const std::string bytes = storage_text.toStdString();
    while (index < storage_pos && index < storage_text.size()) {
        const std::size_t code_len = nebbie::mud_color_code_length(bytes, static_cast<std::size_t>(index));
        if (code_len > 0) {
            index += static_cast<int>(code_len);
            if (show_codes) {
                display += static_cast<int>(code_len);
            }
            continue;
        }
        ++index;
        ++display;
    }
    return display;
}

inline QString build_display_text(const QString& storage_text, bool show_codes) {
    if (show_codes) {
        return storage_text;
    }
    return QString::fromStdString(nebbie::strip_mud_color_codes(storage_text.toStdString()));
}

} // namespace nebbie::qt
