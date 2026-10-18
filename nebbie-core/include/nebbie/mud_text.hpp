#pragma once

#include <cstddef>
#include <string>

namespace nebbie {

bool is_mud_ascii_text(const std::string& text);
std::size_t count_non_mud_ascii_chars(const std::string& text);
/** Accents → letter+' ; curly apostrophes → ' ; NBSP → space. Unmapped non-ASCII kept. */
std::string transliterate_italian_accents_to_apostrophe(const std::string& utf8);
inline std::string normalize_mud_ascii_text(const std::string& utf8) {
    return transliterate_italian_accents_to_apostrophe(utf8);
}

} // namespace nebbie
