#include "nebbie/legacy_format.hpp"

#include <sstream>
#include <string>

namespace nebbie {

std::string format_nebbie_bit_mask(long value) {
    if (value == 0) {
        return "0";
    }
    if (value < 0) {
        return std::to_string(value);
    }

    std::ostringstream oss;
    unsigned long bits = static_cast<unsigned long>(value);
    long bit = 1;
    bool first = true;
    while (bits != 0) {
        if ((bits & 1U) != 0U) {
            if (!first) {
                oss << '|';
            }
            oss << bit;
            first = false;
        }
        bit <<= 1;
        bits >>= 1;
    }
    return oss.str();
}

} // namespace nebbie
