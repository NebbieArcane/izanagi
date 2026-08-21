#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace nebbie {

struct MudColorCode {
    int modifier = 0;
    int bold = 0;
    int foreground = 7;
    int background = 0;
    std::string raw;
};

struct MudColorToken {
    enum class Type { text, color_code };

    Type type = Type::text;
    std::string_view text;
    MudColorCode code;
};

std::size_t mud_color_code_length(std::string_view text, std::size_t offset);
bool try_parse_mud_color_code(std::string_view text, std::size_t offset, MudColorCode* out);

std::string strip_mud_color_codes(const std::string& text);
std::vector<MudColorToken> tokenize_mud_colored_text(std::string_view text);

std::string mud_color_code_name(int foreground);
std::string mud_color_code_description(const MudColorCode& code);
bool mud_color_code_is_advanced(const MudColorCode& code);

} // namespace nebbie
