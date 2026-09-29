#include "nebbie/mob_trailing_sound.hpp"

#include <ostream>

namespace nebbie {

std::string normalize_mob_trailing_sound_text(std::string value) {
    while (!value.empty() && (value.back() == '\r' || value.back() == '\n')) {
        value.pop_back();
    }
    if (!value.empty() && value.back() == '~') {
        value.pop_back();
        while (!value.empty() && (value.back() == ' ' || value.back() == '\t')) {
            value.pop_back();
        }
    }
    return value;
}

void write_mob_trailing_sound_line(std::ostream& out, const std::string& value) {
    const std::string text = normalize_mob_trailing_sound_text(value);
    if (text.empty()) {
        out << "~\n";
        return;
    }
    out << text << "\n~\n";
}

} // namespace nebbie
