#pragma once

#include <cstdio>
#include <iosfwd>
#include <string>

namespace nebbie {

/** How Nebbie myst.* files terminate a string field with ~. */
enum class NebbieTildeStyle {
    /** Short single-line fields: keyword~ */
    Inline,
    /** Paragraph / multiline / L-mob sounds: text then newline then ~ on its own line. */
    OnOwnLine,
};

/**
 * Tilde placement rules (NebbieArcane / Aree):
 * - Inline: mob/obj names, short strings, wld room titles, exit/extra keywords, shop/dam lines.
 * - OnOwnLine always: mob long_descr, mob L sounds, wld exit look descriptions.
 * - OnOwnLine when value contains '\\n': wld/obj room-like paragraphs, mob description, extra desc bodies.
 */

/** Use split tilde when the stored value spans multiple lines (Aree multiline text). */
inline NebbieTildeStyle nebbie_paragraph_tilde_style(const std::string& value) {
    return value.find('\n') != std::string::npos ? NebbieTildeStyle::OnOwnLine : NebbieTildeStyle::Inline;
}

std::string normalize_nebbie_field_text(std::string value);

void write_nebbie_string_field(std::ostream& out, const std::string& value, NebbieTildeStyle style);

void fwrite_nebbie_string_field(FILE* fp, const std::string& value, NebbieTildeStyle style);

} // namespace nebbie
