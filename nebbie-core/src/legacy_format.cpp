#include "nebbie/legacy_format.hpp"

#include "nebbie/fread.hpp"

#include <sstream>
#include <string>

namespace nebbie {

namespace {

long parsed_mask_value(const std::string& token) {
    const auto nums = parse_numbers(token);
    return nums.empty() ? 0L : nums.front();
}

} // namespace

std::string format_nebbie_bit_mask(const long value) {
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

std::string format_nebbie_bit_mask_for_file(const long value,
                                           const std::optional<std::string>& prior_token) {
    // Unchanged token: keep exact file spelling (order/spacing of existing 1|2|4).
    // On any rewrite: always pipe-separated powers of two — never a summed decimal.
    if (prior_token && !prior_token->empty() && parsed_mask_value(*prior_token) == value) {
        return *prior_token;
    }
    return format_nebbie_bit_mask(value);
}

} // namespace nebbie
