#pragma once

#include <string>
#include <vector>

namespace nebbie {

/**
 * Find unique lowercase tokens from {@code text} that match a curated English
 * MUD/stock lexicon (heuristic census for Controlla area/lib warnings).
 */
std::vector<std::string> find_english_tokens(const std::string& text);

/** Join tokens as "a, b, c" for validation messages. */
std::string join_english_tokens(const std::vector<std::string>& tokens);

} // namespace nebbie
