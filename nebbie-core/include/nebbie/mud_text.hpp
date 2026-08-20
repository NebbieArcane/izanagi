#pragma once

#include <cstddef>
#include <string>

namespace nebbie {

bool is_mud_ascii_text(const std::string& text);
std::size_t count_non_mud_ascii_chars(const std::string& text);
std::string transliterate_italian_accents_to_apostrophe(const std::string& utf8);

} // namespace nebbie
