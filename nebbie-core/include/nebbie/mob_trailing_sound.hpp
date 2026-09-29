#pragma once

#include <iosfwd>
#include <string>

namespace nebbie {

/** Strip trailing whitespace and a stray ~ from editor/load glitches. */
std::string normalize_mob_trailing_sound_text(std::string value);

/** NebbieArcane L-mob trailing sounds: content line, then a line containing only ~. */
void write_mob_trailing_sound_line(std::ostream& out, const std::string& value);

} // namespace nebbie
