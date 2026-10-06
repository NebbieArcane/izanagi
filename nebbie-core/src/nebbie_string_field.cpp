#include "nebbie/nebbie_string_field.hpp"

#include <cstdio>
#include <ostream>
#include <sstream>

namespace nebbie {

std::string normalize_nebbie_field_text(std::string value) {
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

void write_nebbie_string_field(std::ostream& out, const std::string& value, NebbieTildeStyle style) {
    const std::string text = normalize_nebbie_field_text(value);
    if (style == NebbieTildeStyle::Inline) {
        if (text.empty()) {
            out << "~\n";
            return;
        }
        out << text << "~\n";
        return;
    }

    if (text.empty()) {
        out << "~\n";
        return;
    }
    out << text << "\n~\n";
}

void fwrite_nebbie_string_field(FILE* fp, const std::string& value, NebbieTildeStyle style) {
    std::ostringstream buffer;
    write_nebbie_string_field(buffer, value, style);
    std::fputs(buffer.str().c_str(), fp);
}

} // namespace nebbie
