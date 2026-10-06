#include "nebbie/mob_trailing_sound.hpp"
#include "nebbie/nebbie_string_field.hpp"

#include <ostream>

namespace nebbie {

void write_mob_trailing_sound_line(std::ostream& out, const std::string& value) {
    write_nebbie_string_field(out, value, NebbieTildeStyle::OnOwnLine);
}

} // namespace nebbie
