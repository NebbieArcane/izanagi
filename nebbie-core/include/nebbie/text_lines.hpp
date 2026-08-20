#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace nebbie {

struct LineLengthIssue {
    int line_number = 0;
    std::size_t length = 0;
};

struct TextLineLengthReport {
    int max_length = 0;
    std::vector<LineLengthIssue> overlong;

    bool ok() const { return overlong.empty(); }
};

std::size_t utf8_char_count(const std::string& text);
std::vector<std::string> split_text_lines(const std::string& text);
TextLineLengthReport check_text_line_lengths(const std::string& text, int max_length);

} // namespace nebbie
