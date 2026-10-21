#pragma once

#include <optional>
#include <string>

namespace nebbie {

/**
 * Nebbie / Aree bitmask spelling: pipe-separated powers of two (e.g. 2|64|1048576).
 * fread_number accepts this form; do not rewrite as a summed decimal.
 */
std::string format_nebbie_bit_mask(long value);

/**
 * Prefer the exact prior_token spelling when it still encodes {@code value}.
 * Otherwise emit pipe-separated powers of two (same as {@link format_nebbie_bit_mask}).
 */
std::string format_nebbie_bit_mask_for_file(long value, const std::optional<std::string>& prior_token);

} // namespace nebbie
