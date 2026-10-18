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
        QColor(235, 235, 235),
        QColor(210, 40, 40),
        QColor(40, 170, 40),
        QColor(200, 160, 0),
        QColor(70, 70, 220),
        QColor(180, 70, 180),
        QColor(60, 170, 200),
        QColor(235, 235, 235),
        QColor(160, 160, 160),
        QColor(255, 80, 80),
        QColor(80, 220, 80),
        QColor(255, 220, 80),
        QColor(120, 160, 255),
        QColor(220, 120, 220),
        QColor(120, 220, 255),
        QColor(255, 255, 255),
    };
    const int index = (color >= 0 && color <= 15) ? color : 7;
    QColor result = palette[index];
    if (bold) {
        result = result.lighter(115);
    }
    return result;
}

inline bool is_mud_ascii_qchar(const QChar ch) {
    const uint unicode = ch.unicode();
    return unicode == '\t' || unicode == '\n' || unicode == '\r'
           || (unicode >= 0x20u && unicode <= 0x7Eu);
}

// Color codes are ASCII; probe from a QString index so UTF-8 multi-byte letters do not
// desync storage indexing used by the editor widgets.
inline int mud_color_code_length_at(const QString& storage_text, int storage_index) {
    if (storage_index < 0 || storage_index >= storage_text.size()
        || storage_text.at(storage_index) != QLatin1Char('$')) {
        return 0;
    }
    constexpr int kMaxCodeChars = 8;
    const std::string prefix =
        storage_text.mid(storage_index, kMaxCodeChars).toStdString();
    return static_cast<int>(nebbie::mud_color_code_length(prefix, 0));
}

inline int display_position_to_storage(const QString& storage_text, int display_pos, bool show_codes) {
    int display = 0;
    int storage_index = 0;
    while (storage_index < storage_text.size() && display < display_pos) {
        const int code_len = mud_color_code_length_at(storage_text, storage_index);
        if (code_len > 0) {
            storage_index += code_len;
            if (show_codes) {
                display += code_len;
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
    while (index < storage_pos && index < storage_text.size()) {
        const int code_len = mud_color_code_length_at(storage_text, index);
        if (code_len > 0) {
            index += code_len;
            if (show_codes) {
                display += code_len;
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

inline QString mud_entity_list_label(long vnum, const std::string& storage_text) {
    return QString("#%1 %2")
        .arg(vnum)
        .arg(QString::fromStdString(nebbie::strip_mud_color_codes(storage_text)));
}

} // namespace nebbie::qt
