#pragma once

#include "nebbie/nebbie_string_field.hpp"

#include <iosfwd>
#include <string>

namespace nebbie {

/** NebbieArcane L-mob trailing sounds: content line, then a line containing only ~. */
void write_mob_trailing_sound_line(std::ostream& out, const std::string& value);

/** @see normalize_nebbie_field_text */
inline std::string normalize_mob_trailing_sound_text(std::string value) {
    return normalize_nebbie_field_text(std::move(value));
}

} // namespace nebbie
