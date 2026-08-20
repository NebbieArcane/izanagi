#include "nebbie/text_lines.hpp"

namespace nebbie {

std::size_t utf8_char_count(const std::string& text) {
    std::size_t count = 0;
    for (std::size_t i = 0; i < text.size();) {
        const unsigned char byte = static_cast<unsigned char>(text[i]);
        if (byte < 0x80) {
            i += 1;
        } else if ((byte & 0xE0) == 0xC0) {
            i += 2;
        } else if ((byte & 0xF0) == 0xE0) {
            i += 3;
        } else if ((byte & 0xF8) == 0xF0) {
            i += 4;
        } else {
            i += 1;
        }
        ++count;
    }
    return count;
}

std::vector<std::string> split_text_lines(const std::string& text) {
    std::vector<std::string> lines;
    if (text.empty()) {
        lines.emplace_back();
        return lines;
    }

    std::size_t start = 0;
    for (std::size_t i = 0; i <= text.size(); ++i) {
        if (i == text.size() || text[i] == '\n') {
            std::string line = text.substr(start, i - start);
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            lines.push_back(std::move(line));
            start = i + 1;
        }
    }
    return lines;
}

TextLineLengthReport check_text_line_lengths(const std::string& text, int max_length) {
    TextLineLengthReport report;
    report.max_length = max_length;
    if (max_length <= 0) {
        return report;
    }

    const std::vector<std::string> lines = split_text_lines(text);
    for (std::size_t i = 0; i < lines.size(); ++i) {
        const std::size_t length = utf8_char_count(lines[i]);
        if (length > static_cast<std::size_t>(max_length)) {
            report.overlong.push_back({static_cast<int>(i + 1), length});
        }
    }
    return report;
}

} // namespace nebbie
