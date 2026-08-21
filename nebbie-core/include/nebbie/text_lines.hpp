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

struct LongestVisibleLine {
    int line_number = 0;
    std::size_t visible_length = 0;
};

std::size_t utf8_char_count(const std::string& text);
std::size_t visible_utf8_char_count(const std::string& text);
std::vector<std::string> split_text_lines(const std::string& text);
LongestVisibleLine find_longest_visible_line(const std::string& text);
TextLineLengthReport check_text_line_lengths(const std::string& text, int max_length);
TextLineLengthReport check_visible_text_line_lengths(const std::string& text, int max_length);

} // namespace nebbie
