#pragma once

#include <optional>
#include <string>

namespace nebbie {

/** Nebbie file format: bitmask as pipe-separated powers of two (e.g. 2|64|1048576). */
std::string format_nebbie_bit_mask(long value);

/** Write a mask using the same style as prior_token when value matches or when rewriting (scalar vs pipe). */
std::string format_nebbie_bit_mask_for_file(long value, const std::optional<std::string>& prior_token);

} // namespace nebbie
