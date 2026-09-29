#pragma once

#include <string>

namespace nebbie {

/** Nebbie file format: bitmask as pipe-separated powers of two (e.g. 2|64|1048576). */
std::string format_nebbie_bit_mask(long value);

} // namespace nebbie
