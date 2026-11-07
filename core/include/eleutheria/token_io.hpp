#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>

namespace eleutheria {

class IoError : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

class TokenReader {
 public:
  explicit TokenReader(std::string text);

  bool eof() const;
  void skip_ws();
  void skip_line();
  char peek();
  char get();
  void unget(char c);

  // Nebbie fread_number: digits with optional +/-, OR via '|'
  std::int64_t read_number();
  // Like fread_if_number: returns 0 if next token is not a number
  std::int64_t read_optional_number();
  // Optional number on same line (not past newline)
  bool try_read_number_same_line(std::int64_t* out);

  // Bitmask token (es. 1|16|4096): valore OR + spelling grezzo opzionale.
  std::int64_t read_flag(std::string* raw_out = nullptr);

  std::string read_word();
  // Diku tilde-string (fread_string)
  std::string read_tilde_string();

  // Dice NdS+B
  void read_dice(std::int64_t* n, std::int64_t* s, std::int64_t* b);

  std::size_t pos() const { return pos_; }
  const std::string& text() const { return text_; }

 private:
  std::string text_;
  std::size_t pos_ = 0;
};

std::int64_t parse_flag_token(std::string_view token);
std::string format_flag_bits(std::uint64_t flags);
/** Se prior_token rappresenta ancora value, lo riscrive invariato; altrimenti 1|2|4... */
std::string format_flag_bits_for_file(std::uint64_t value, const std::string& prior_token);
std::string format_signed(std::int64_t value);
void write_tilde_string(std::ostream& out, std::string_view value);

}  // namespace eleutheria
