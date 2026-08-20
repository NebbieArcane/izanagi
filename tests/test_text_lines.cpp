#include "nebbie/mud_text.hpp"
#include "nebbie/mud_text.hpp"
#include "nebbie/text_lines.hpp"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void expect(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

} // namespace

int main() {
    try {
        expect(nebbie::utf8_char_count("") == 0, "empty string");
        expect(nebbie::utf8_char_count("abc") == 3, "ascii count");
        expect(nebbie::utf8_char_count("àè") == 2, "utf8 accented count");

        const std::vector<std::string> lines = nebbie::split_text_lines("one\ntwo\r\nthree");
        expect(lines.size() == 3, "split line count");
        expect(lines[0] == "one", "first line");
        expect(lines[1] == "two", "second line");
        expect(lines[2] == "three", "third line");

        const nebbie::TextLineLengthReport ok =
            nebbie::check_text_line_lengths("short\nline", 10);
        expect(ok.ok(), "within limit");

        const nebbie::TextLineLengthReport bad =
            nebbie::check_text_line_lengths("12345678901", 10);
        expect(!bad.ok(), "over limit");
        expect(bad.overlong.size() == 1, "one overlong line");
        expect(bad.overlong[0].line_number == 1, "line number");
        expect(bad.overlong[0].length == 11, "line length");

        const nebbie::TextLineLengthReport disabled =
            nebbie::check_text_line_lengths("12345678901", 0);
        expect(disabled.ok(), "disabled limit");

        expect(nebbie::is_mud_ascii_text("c'e' una porta"), "mud ascii apostrophe text");
        expect(!nebbie::is_mud_ascii_text("c'è una porta"), "utf8 accent is not mud ascii");
        expect(nebbie::transliterate_italian_accents_to_apostrophe("c'è") == "c'e'",
               "accent transliteration");

        std::cout << "OK\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "FAILED: " << ex.what() << '\n';
        return 1;
    }
}
