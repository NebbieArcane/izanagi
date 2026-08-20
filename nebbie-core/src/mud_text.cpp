#include "nebbie/mud_text.hpp"

#include <cstdint>

namespace nebbie {

namespace {

bool is_mud_ascii_byte(unsigned char byte) {
    return byte == '\t' || byte == '\n' || byte == '\r' || (byte >= 0x20 && byte <= 0x7E);
}

bool decode_utf8_codepoint(const std::string& text, std::size_t& index, char32_t& codepoint) {
    if (index >= text.size()) {
        return false;
    }

    const unsigned char first = static_cast<unsigned char>(text[index]);
    if (first < 0x80) {
        codepoint = first;
        ++index;
        return true;
    }

    std::size_t extra = 0;
    if ((first & 0xE0) == 0xC0) {
        extra = 1;
        codepoint = first & 0x1F;
    } else if ((first & 0xF0) == 0xE0) {
        extra = 2;
        codepoint = first & 0x0F;
    } else if ((first & 0xF8) == 0xF0) {
        extra = 3;
        codepoint = first & 0x07;
    } else {
        codepoint = first;
        ++index;
        return true;
    }

    if (index + extra >= text.size()) {
        codepoint = first;
        ++index;
        return true;
    }

    for (std::size_t i = 1; i <= extra; ++i) {
        const unsigned char next = static_cast<unsigned char>(text[index + i]);
        if ((next & 0xC0) != 0x80) {
            codepoint = first;
            ++index;
            return true;
        }
        codepoint = (codepoint << 6) | (next & 0x3F);
    }

    index += extra + 1;
    return true;
}

void append_ascii(std::string& out, char byte) {
    out.push_back(byte);
}

const char* transliterate_codepoint(char32_t codepoint) {
    switch (codepoint) {
    case U'à':
    case U'á':
    case U'â':
    case U'ä':
    case U'ã':
        return "a'";
    case U'À':
    case U'Á':
    case U'Â':
    case U'Ä':
    case U'Ã':
        return "A'";
    case U'è':
    case U'é':
    case U'ê':
    case U'ë':
        return "e'";
    case U'È':
    case U'É':
    case U'Ê':
    case U'Ë':
        return "E'";
    case U'ì':
    case U'í':
    case U'î':
    case U'ï':
        return "i'";
    case U'Ì':
    case U'Í':
    case U'Î':
    case U'Ï':
        return "I'";
    case U'ò':
    case U'ó':
    case U'ô':
    case U'ö':
    case U'õ':
        return "o'";
    case U'Ò':
    case U'Ó':
    case U'Ô':
    case U'Ö':
    case U'Õ':
        return "O'";
    case U'ù':
    case U'ú':
    case U'û':
    case U'ü':
        return "u'";
    case U'Ù':
    case U'Ú':
    case U'Û':
    case U'Ü':
        return "U'";
    default:
        return nullptr;
    }
}

} // namespace

bool is_mud_ascii_text(const std::string& text) {
    for (unsigned char byte : text) {
        if (!is_mud_ascii_byte(byte)) {
            return false;
        }
    }
    return true;
}

std::size_t count_non_mud_ascii_chars(const std::string& text) {
    std::size_t count = 0;
    for (std::size_t index = 0; index < text.size();) {
        char32_t codepoint = 0;
        const std::size_t before = index;
        if (!decode_utf8_codepoint(text, index, codepoint)) {
            break;
        }
        if (codepoint <= 0x7F) {
            if (!is_mud_ascii_byte(static_cast<unsigned char>(codepoint))) {
                ++count;
            }
            continue;
        }
        ++count;
        if (index <= before) {
            ++index;
        }
    }
    return count;
}

std::string transliterate_italian_accents_to_apostrophe(const std::string& utf8) {
    std::string out;
    out.reserve(utf8.size());

    for (std::size_t index = 0; index < utf8.size();) {
        const std::size_t before = index;
        char32_t codepoint = 0;
        if (!decode_utf8_codepoint(utf8, index, codepoint)) {
            break;
        }

        if (const char* replacement = transliterate_codepoint(codepoint)) {
            out.append(replacement);
            continue;
        }

        if (codepoint <= 0x7F) {
            append_ascii(out, static_cast<char>(codepoint));
            continue;
        }

        out.append(utf8.substr(before, index - before));
    }

    return out;
}

} // namespace nebbie
