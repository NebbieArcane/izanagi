#include "eleutheria/token_io.hpp"

#include <cctype>
#include <sstream>

namespace eleutheria {
namespace {

bool is_ws(char c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}

}  // namespace

TokenReader::TokenReader(std::string text) : text_(std::move(text)) {}

bool TokenReader::eof() const { return pos_ >= text_.size(); }

void TokenReader::skip_ws() {
  while(!eof() && is_ws(text_[pos_])) {
    ++pos_;
  }
}

void TokenReader::skip_line() {
  while(!eof()) {
    const char c = text_[pos_++];
    if(c == '\n') {
      return;
    }
  }
}

char TokenReader::peek() {
  if(eof()) {
    return '\0';
  }
  return text_[pos_];
}

char TokenReader::get() {
  if(eof()) {
    return '\0';
  }
  return text_[pos_++];
}

void TokenReader::unget(char c) {
  if(pos_ == 0) {
    return;
  }
  --pos_;
  if(!eof() && text_[pos_] != c) {
    // Keep position consistent even if mismatch (defensive).
  }
  (void)c;
}

std::int64_t TokenReader::read_number() {
  skip_ws();
  if(eof()) {
    throw IoError("Expected number, got EOF");
  }

  bool sign = false;
  char c = get();
  if(c == '+') {
    c = get();
  } else if(c == '-') {
    sign = true;
    c = get();
  }

  if(!std::isdigit(static_cast<unsigned char>(c))) {
    unget(c);
    throw IoError(std::string("Expected digit, got '") + c + "'");
  }

  std::int64_t number = 0;
  while(std::isdigit(static_cast<unsigned char>(c))) {
    number = number * 10 + (c - '0');
    c = get();
  }

  if(sign) {
    number = -number;
  }

  // Nebbie: a|b|c sono bit OR, non somme (su potenze di 2 coincidono, ma
  // con duplicati/valori sporchi += corrompe il mask).
  if(c == '|') {
    number |= read_number();
  } else if(c == ' ' || c == '\t' || c == '\0') {
    // terminatore gia' consumato
  } else {
    // Lascia newline/altri caratteri: serve a try_read_number_same_line
    // e al parsing riga-per-riga (gold/race vs pos/sex).
    unget(c);
  }

  return number;
}

std::int64_t TokenReader::read_optional_number() {
  skip_ws();
  if(eof()) {
    return 0;
  }
  const char c = peek();
  if(c == '+' || c == '-' || std::isdigit(static_cast<unsigned char>(c))) {
    try {
      return read_number();
    } catch(const IoError&) {
      return 0;
    }
  }
  return 0;
}

bool TokenReader::try_read_number_same_line(std::int64_t* out) {
  std::size_t saved = pos_;
  while(pos_ < text_.size() && (text_[pos_] == ' ' || text_[pos_] == '\t')) {
    ++pos_;
  }
  if(eof() || text_[pos_] == '\n' || text_[pos_] == '\r') {
    pos_ = saved;
    return false;
  }
  const char c = text_[pos_];
  if(!(c == '+' || c == '-' || std::isdigit(static_cast<unsigned char>(c)))) {
    pos_ = saved;
    return false;
  }
  try {
    *out = read_number();
    return true;
  } catch(const IoError&) {
    pos_ = saved;
    return false;
  }
}

std::string TokenReader::read_word() {
  skip_ws();
  std::string word;
  while(!eof()) {
    const char c = peek();
    if(is_ws(c)) {
      break;
    }
    word.push_back(get());
  }
  return word;
}

std::string TokenReader::read_tilde_string() {
  std::string result;
  while(!eof()) {
    const char c = get();
    if(c == '~') {
      // Consume trailing whitespace/newline after tilde like classic fread_string
      while(!eof()) {
        const char n = peek();
        if(n == '\n' || n == '\r') {
          get();
          if(n == '\r' && peek() == '\n') {
            get();
          }
          break;
        }
        if(n == ' ' || n == '\t') {
          get();
          continue;
        }
        break;
      }
      // Strip one trailing newline from content if present (Diku style)
      if(!result.empty() && result.back() == '\n') {
        result.pop_back();
        if(!result.empty() && result.back() == '\r') {
          result.pop_back();
        }
      }
      // Difesa: niente CR/LF iniziale (spesso lasciato dal #vnum)
      while(!result.empty() && (result.front() == '\n' || result.front() == '\r')) {
        result.erase(result.begin());
      }
      return result;
    }
    result.push_back(c);
  }
  throw IoError("Unterminated tilde string");
}

void TokenReader::read_dice(std::int64_t* n, std::int64_t* s, std::int64_t* b) {
  skip_ws();
  *n = 0;
  *s = 0;
  *b = 0;
  // Parse NdS+B manually
  auto read_int = [this]() -> std::int64_t {
    bool neg = false;
    char c = peek();
    if(c == '+') {
      get();
      c = peek();
    } else if(c == '-') {
      neg = true;
      get();
      c = peek();
    }
    if(!std::isdigit(static_cast<unsigned char>(c))) {
      throw IoError("Bad dice number");
    }
    std::int64_t v = 0;
    while(!eof() && std::isdigit(static_cast<unsigned char>(peek()))) {
      v = v * 10 + (get() - '0');
    }
    return neg ? -v : v;
  };

  *n = read_int();
  if(peek() != 'd' && peek() != 'D') {
    throw IoError("Expected 'd' in dice");
  }
  get();
  *s = read_int();
  if(peek() == '+') {
    get();
    *b = read_int();
  } else if(peek() == '-') {
    *b = read_int();
  }
}

std::int64_t parse_flag_token(std::string_view token) {
  if(token.empty()) {
    return 0;
  }
  std::int64_t value = 0;
  std::size_t i = 0;
  while(i < token.size()) {
    bool neg = false;
    if(token[i] == '+') {
      ++i;
    } else if(token[i] == '-') {
      neg = true;
      ++i;
    }
    if(i >= token.size() || !std::isdigit(static_cast<unsigned char>(token[i]))) {
      break;
    }
    std::int64_t part = 0;
    while(i < token.size() && std::isdigit(static_cast<unsigned char>(token[i]))) {
      part = part * 10 + (token[i] - '0');
      ++i;
    }
    if(neg) {
      part = -part;
    }
    value |= part;
    if(i < token.size() && token[i] == '|') {
      ++i;
      continue;
    }
    break;
  }
  return value;
}

std::int64_t TokenReader::read_flag(std::string* raw_out) {
  skip_ws();
  const std::string token = read_word();
  if(raw_out != nullptr) {
    *raw_out = token;
  }
  if(token.empty()) {
    throw IoError("Expected flag token, got EOF/empty");
  }
  return parse_flag_token(token);
}

std::string format_flag_bits(std::uint64_t flags) {
  if(flags == 0) {
    return "0";
  }
  std::ostringstream oss;
  bool first = true;
  std::uint64_t bit = 1;
  while(flags) {
    if(flags & 1ULL) {
      if(!first) {
        oss << '|';
      }
      first = false;
      oss << bit;
    }
    bit <<= 1;
    flags >>= 1;
  }
  return oss.str();
}

std::string format_flag_bits_for_file(std::uint64_t value, const std::string& prior_token) {
  if(!prior_token.empty() &&
     static_cast<std::uint64_t>(parse_flag_token(prior_token)) == value) {
    return prior_token;
  }
  return format_flag_bits(value);
}

std::string format_signed(std::int64_t value) {
  return std::to_string(value);
}

void write_tilde_string(std::ostream& out, std::string_view value) {
  out << value;
  if(value.empty() || value.back() != '\n') {
    // Classic writers often put ~ after content; description may contain newlines.
  }
  out << "~\n";
}

}  // namespace eleutheria
