#pragma once

#include "mud_color_widgets.hpp"

#include <QWidget>

#include <vector>

namespace nebbie::qt {

using MudFieldList = std::vector<MudColorTextEdit*>;

void applyMudFieldSettings(const MudFieldList& fields, int max_line_length, bool show_color_codes);
MudColorTextEdit* focusedMudField(const MudFieldList& fields);
void pastePlainTextIntoWidget(QWidget* widget);

} // namespace nebbie::qt
