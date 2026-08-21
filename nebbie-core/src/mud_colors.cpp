#include "nebbie/mud_colors.hpp"

#include <cctype>
#include <sstream>

namespace nebbie {

namespace {

bool is_digit(char ch) {
    return ch >= '0' && ch <= '9';
}

bool read_digits(std::string_view text, std::size_t offset, int count, int* digits) {
    if (offset + static_cast<std::size_t>(count) > text.size()) {
        return false;
    }
    for (int i = 0; i < count; ++i) {
        const char ch = text[offset + static_cast<std::size_t>(i)];
        if (!is_digit(ch)) {
            return false;
        }
        digits[i] = ch - '0';
    }
    return true;
}

} // namespace

std::size_t mud_color_code_length(std::string_view text, std::size_t offset) {
    if (offset + 7 <= text.size() && text[offset] == '$' && text[offset + 1] == '$'
        && text[offset + 2] == 'c') {
        int digits[4] = {};
        if (read_digits(text, offset + 3, 4, digits)) {
            return 7;
        }
    }
    if (offset + 6 <= text.size() && text[offset] == '$' && text[offset + 1] == 'c') {
        int digits[4] = {};
        if (read_digits(text, offset + 2, 4, digits)) {
            return 6;
        }
    }
    return 0;
}

bool try_parse_mud_color_code(std::string_view text, std::size_t offset, MudColorCode* out) {
    const std::size_t length = mud_color_code_length(text, offset);
    if (length == 0 || out == nullptr) {
        return false;
    }

    const std::size_t digit_offset = (length == 7) ? offset + 3 : offset + 2;
    int digits[4] = {};
    if (!read_digits(text, digit_offset, 4, digits)) {
        return false;
    }

    out->modifier = digits[0];
    out->bold = digits[1];
    out->foreground = digits[2] * 10 + digits[3];
    out->background = 0;
    out->raw.assign(text.substr(offset, length));
    return true;
}

std::string strip_mud_color_codes(const std::string& text) {
    std::string stripped;
    stripped.reserve(text.size());

    for (std::size_t i = 0; i < text.size();) {
        const std::size_t code_length = mud_color_code_length(text, i);
        if (code_length > 0) {
            i += code_length;
            continue;
        }
        stripped.push_back(text[i]);
        ++i;
    }
    return stripped;
}

std::vector<MudColorToken> tokenize_mud_colored_text(std::string_view text) {
    std::vector<MudColorToken> tokens;
    std::size_t i = 0;
    while (i < text.size()) {
        MudColorCode code;
        if (try_parse_mud_color_code(text, i, &code)) {
            MudColorToken token;
            token.type = MudColorToken::Type::color_code;
            token.text = text.substr(i, code.raw.size());
            token.code = std::move(code);
            tokens.push_back(std::move(token));
            i += tokens.back().text.size();
            continue;
        }

        const std::size_t start = i;
        while (i < text.size() && mud_color_code_length(text, i) == 0) {
            ++i;
        }
        MudColorToken token;
        token.type = MudColorToken::Type::text;
        token.text = text.substr(start, i - start);
        tokens.push_back(std::move(token));
    }
    return tokens;
}

std::string mud_color_code_name(int foreground) {
    switch (foreground) {
    case 1:
        return "Rosso scuro";
    case 2:
        return "Verde scuro";
    case 3:
        return "Marrone";
    case 4:
        return "Blu";
    case 5:
        return "Magenta";
    case 6:
        return "Cyan";
    case 7:
        return "Grigio (default)";
    case 8:
        return "Grigio scuro";
    case 9:
        return "Rosso chiaro";
    case 10:
        return "Verde chiaro";
    case 11:
        return "Giallo";
    case 12:
        return "Blu chiaro";
    case 13:
        return "Violetto";
    case 14:
        return "Celeste chiaro";
    case 15:
        return "Bianco";
    default:
        return "Colore " + std::to_string(foreground);
    }
}

std::string mud_color_code_description(const MudColorCode& code) {
    std::ostringstream out;
    out << code.raw << " — " << mud_color_code_name(code.foreground);
    if (code.modifier != 0 || code.bold != 0) {
        out << " (mod=" << code.modifier << ", grassetto=" << code.bold << ")";
    }
    return out.str();
}

bool mud_color_code_is_advanced(const MudColorCode& code) {
    if (code.modifier != 0 || code.bold != 0) {
        return true;
    }
    return code.foreground < 1 || code.foreground > 15;
}

} // namespace nebbie
