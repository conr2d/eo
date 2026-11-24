// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once

#include <eo/core.h>
#include <eo/errors.h>
#include <eo/print.h>
#include <eo/utf8.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>

// Go's strconv, in part: the integer and bool conversions, Quote, QuoteRune, Unquote, and IsPrint,
// ported from Go 1.26's strconv and internal/strconv.

namespace eo::strconv {

inline constexpr int64_t IntSize = 64;

inline error ErrRange = errors::New("value out of range");
inline error ErrSyntax = errors::New("invalid syntax");

// Go's strconv.NumError, which the parse functions return through error as a *NumError.
struct NumError {
  static constexpr std::string_view _eo_name = "strconv.NumError";

  std::string Func{};
  std::string Num{};
  error Err{};

  auto operator==(const NumError&) const -> bool = default;

  static auto Error(ptr<NumError> e) -> std::string {
    auto quoted = std::string{};
    eo::detail::append_quoted(quoted, e->Num);
    return "strconv." + e->Func + ": parsing " + quoted + ": " + e->Err.Error();
  }

  static auto Unwrap(ptr<NumError> e) -> error {
    return e->Err;
  }

  using _eo_methods = eo::methods<eo::method<&NumError::Error, "Error">, eo::method<&NumError::Unwrap, "Unwrap">>;
};

namespace detail {

  // The errors of internal/strconv, which the exported functions turn into a *NumError.
  enum class failure {
    none,
    range,
    syntax,
    base,
    bit_size
  };

  inline auto lower(char c) -> char {
    return static_cast<char>(c | ('x' - 'X'));
  }

  inline auto num_error(std::string fn, std::string s, int64_t base, int64_t bit_size, failure f) -> error {
    auto reason = error{};
    switch (f) {
    case failure::range:
      reason = ErrRange;
      break;
    case failure::syntax:
      reason = ErrSyntax;
      break;
    case failure::base:
      reason = errors::New("invalid base " + std::to_string(base));
      break;
    case failure::bit_size:
      reason = errors::New("invalid bit size " + std::to_string(bit_size));
      break;
    case failure::none:
      return nullptr;
    }
    return New<NumError>(NumError{std::move(fn), std::move(s), std::move(reason)});
  }

  // Go's underscoreOK: whether the underscores of a base-prefixed literal separate digits.
  inline auto underscore_ok(std::string_view s) -> bool {
    auto saw = '^';
    std::size_t i = 0;
    if (!s.empty() && (s[0] == '-' || s[0] == '+')) {
      s.remove_prefix(1);
    }
    auto hex = false;
    if (s.size() >= 2 && s[0] == '0' && (lower(s[1]) == 'b' || lower(s[1]) == 'o' || lower(s[1]) == 'x')) {
      i = 2;
      saw = '0';
      hex = lower(s[1]) == 'x';
    }
    for (; i < s.size(); ++i) {
      if (('0' <= s[i] && s[i] <= '9') || (hex && 'a' <= lower(s[i]) && lower(s[i]) <= 'f')) {
        saw = '0';
        continue;
      }
      if (s[i] == '_') {
        if (saw != '0') {
          return false;
        }
        saw = '_';
        continue;
      }
      if (saw == '_') {
        return false;
      }
      saw = '!';
    }
    return saw != '_';
  }

  struct parsed_uint {
    uint64_t value;
    failure f;
  };

  // internal/strconv's ParseUint.
  inline auto parse_uint(std::string_view s, int64_t base, int64_t bit_size) -> parsed_uint {
    if (s.empty()) {
      return {0, failure::syntax};
    }
    auto base0 = base == 0;
    auto s0 = s;
    if (2 <= base && base <= 36) {
    } else if (base == 0) {
      base = 10;
      if (s[0] == '0') {
        if (s.size() >= 3 && lower(s[1]) == 'b') {
          base = 2;
          s.remove_prefix(2);
        } else if (s.size() >= 3 && lower(s[1]) == 'o') {
          base = 8;
          s.remove_prefix(2);
        } else if (s.size() >= 3 && lower(s[1]) == 'x') {
          base = 16;
          s.remove_prefix(2);
        } else {
          base = 8;
          s.remove_prefix(1);
        }
      }
    } else {
      return {0, failure::base};
    }
    if (bit_size == 0) {
      bit_size = IntSize;
    } else if (bit_size < 0 || bit_size > 64) {
      return {0, failure::bit_size};
    }
    auto cutoff = UINT64_MAX / static_cast<uint64_t>(base) + 1;
    auto max_val = bit_size == 64 ? UINT64_MAX : (uint64_t{1} << bit_size) - 1;
    auto underscores = false;
    auto n = uint64_t{0};
    for (auto c : s) {
      auto d = 0;
      if (c == '_' && base0) {
        underscores = true;
        continue;
      }
      if ('0' <= c && c <= '9') {
        d = c - '0';
      } else if ('a' <= lower(c) && lower(c) <= 'z') {
        d = lower(c) - 'a' + 10;
      } else {
        return {0, failure::syntax};
      }
      if (d >= base) {
        return {0, failure::syntax};
      }
      if (n >= cutoff) {
        return {max_val, failure::range};
      }
      n *= static_cast<uint64_t>(base);
      auto n1 = n + static_cast<uint64_t>(d);
      if (n1 < n || n1 > max_val) {
        return {max_val, failure::range};
      }
      n = n1;
    }
    if (underscores && !underscore_ok(s0)) {
      return {0, failure::syntax};
    }
    return {n, failure::none};
  }

  struct parsed_int {
    int64_t value;
    failure f;
  };

  // internal/strconv's ParseInt.
  inline auto parse_int(std::string_view s, int64_t base, int64_t bit_size) -> parsed_int {
    if (s.empty()) {
      return {0, failure::syntax};
    }
    auto neg = false;
    if (s[0] == '+') {
      s.remove_prefix(1);
    } else if (s[0] == '-') {
      s.remove_prefix(1);
      neg = true;
    }
    auto [un, f] = parse_uint(s, base, bit_size);
    if (f != failure::none && f != failure::range) {
      return {0, f};
    }
    if (bit_size == 0) {
      bit_size = IntSize;
    }
    auto cutoff = uint64_t{1} << (bit_size - 1);
    if (!neg && un >= cutoff) {
      return {static_cast<int64_t>(cutoff - 1), failure::range};
    }
    if (neg && un > cutoff) {
      return {static_cast<int64_t>(0 - cutoff), failure::range};
    }
    auto n = static_cast<int64_t>(un);
    return {neg ? static_cast<int64_t>(0 - un) : n, failure::none};
  }

  // internal/strconv's formatBits.
  inline auto format_bits(uint64_t u, int64_t base, bool neg) -> std::string {
    if (base < 2 || base > 36) {
      panic("strconv: illegal AppendInt/FormatInt base"s);
    }
    constexpr std::string_view digits = "0123456789abcdefghijklmnopqrstuvwxyz";
    auto s = std::string{};
    do {
      s += digits[u % static_cast<uint64_t>(base)];
      u /= static_cast<uint64_t>(base);
    } while (u != 0);
    if (neg) {
      s += '-';
    }
    return {s.rbegin(), s.rend()};
  }

  inline auto append_string(slice<uint8_t> dst, const std::string& s) -> slice<uint8_t> {
    return eo::append_slice(std::move(dst), s);
  }

  inline auto unhex(char b) -> std::pair<int32_t, bool> {
    if ('0' <= b && b <= '9') {
      return {b - '0', true};
    }
    if ('a' <= b && b <= 'f') {
      return {b - 'a' + 10, true};
    }
    if ('A' <= b && b <= 'F') {
      return {b - 'A' + 10, true};
    }
    return {0, false};
  }

  inline auto valid_rune(int32_t r) -> bool {
    return (0 <= r && r < 0xd800) || (0xdfff < r && r <= 0x10ffff);
  }

} // namespace detail

inline auto ParseUint(std::string s, int64_t base, int64_t bitSize) -> std::tuple<uint64_t, error> {
  auto [x, f] = detail::parse_uint(s, base, bitSize);
  return {x, detail::num_error("ParseUint", s, base, bitSize, f)};
}

inline auto ParseInt(std::string s, int64_t base, int64_t bitSize) -> std::tuple<int64_t, error> {
  auto [x, f] = detail::parse_int(s, base, bitSize);
  return {x, detail::num_error("ParseInt", s, base, bitSize, f)};
}

inline auto Atoi(std::string s) -> std::tuple<int64_t, error> {
  auto [x, f] = detail::parse_int(s, 10, 0);
  return {x, detail::num_error("Atoi", s, 0, 0, f)};
}

inline auto ParseBool(std::string str) -> std::tuple<bool, error> {
  if (str == "1" || str == "t" || str == "T" || str == "true" || str == "TRUE" || str == "True") {
    return {true, nullptr};
  }
  if (str == "0" || str == "f" || str == "F" || str == "false" || str == "FALSE" || str == "False") {
    return {false, nullptr};
  }
  return {false, detail::num_error("ParseBool", str, 0, 0, detail::failure::syntax)};
}

inline auto FormatBool(bool b) -> std::string {
  return b ? "true" : "false";
}

inline auto FormatUint(uint64_t i, int64_t base) -> std::string {
  return detail::format_bits(i, base, false);
}

inline auto FormatInt(int64_t i, int64_t base) -> std::string {
  auto neg = i < 0;
  return detail::format_bits(neg ? 0 - static_cast<uint64_t>(i) : static_cast<uint64_t>(i), base, neg);
}

inline auto Itoa(int64_t i) -> std::string {
  return FormatInt(i, 10);
}

inline auto AppendBool(slice<uint8_t> dst, bool b) -> slice<uint8_t> {
  return detail::append_string(std::move(dst), FormatBool(b));
}

inline auto AppendInt(slice<uint8_t> dst, int64_t i, int64_t base) -> slice<uint8_t> {
  return detail::append_string(std::move(dst), FormatInt(i, base));
}

inline auto AppendUint(slice<uint8_t> dst, uint64_t i, int64_t base) -> slice<uint8_t> {
  return detail::append_string(std::move(dst), FormatUint(i, base));
}

// Go's strconv.IsPrint, with the same exception as %q: an unassigned code point is printable.
inline auto IsPrint(int32_t r) -> bool {
  if (r < 0 || r > 0x10ffff || (r >= 0xd800 && r <= 0xdfff)) {
    return false;
  }
  if (r < 0x80) {
    return r >= 0x20 && r < 0x7f;
  }
  return eo::detail::is_print(static_cast<char32_t>(r));
}

inline auto Quote(std::string s) -> std::string {
  auto out = std::string{};
  eo::detail::append_quoted(out, s);
  return out;
}

inline auto AppendQuote(slice<uint8_t> dst, std::string s) -> slice<uint8_t> {
  return detail::append_string(std::move(dst), Quote(std::move(s)));
}

inline auto QuoteRune(int32_t r) -> std::string {
  if (!detail::valid_rune(r)) {
    r = 0xfffd;
  }
  auto out = std::string{"'"};
  eo::detail::append_escaped_rune(out, static_cast<char32_t>(r), '\'');
  out += '\'';
  return out;
}

// Go's UnquoteChar.
inline auto UnquoteChar(std::string s, uint8_t quote) -> std::tuple<int32_t, bool, std::string, error> {
  auto syntax = std::tuple<int32_t, bool, std::string, error>{0, false, "", ErrSyntax};
  if (s.empty()) {
    return syntax;
  }
  auto c = static_cast<unsigned char>(s[0]);
  if (c == quote && (quote == '\'' || quote == '"')) {
    return syntax;
  }
  if (c >= 0x80) {
    auto [r, size] = eo::detail::decode_rune(s, 0);
    return {static_cast<int32_t>(r), true, s.substr(size), nullptr};
  }
  if (c != '\\') {
    return {static_cast<int32_t>(c), false, s.substr(1), nullptr};
  }
  if (s.size() <= 1) {
    return syntax;
  }
  auto e = s[1];
  auto rest = std::string_view{s}.substr(2);
  auto value = int32_t{0};
  auto multibyte = false;
  switch (e) {
  case 'a':
    value = '\a';
    break;
  case 'b':
    value = '\b';
    break;
  case 'f':
    value = '\f';
    break;
  case 'n':
    value = '\n';
    break;
  case 'r':
    value = '\r';
    break;
  case 't':
    value = '\t';
    break;
  case 'v':
    value = '\v';
    break;
  case 'x':
  case 'u':
  case 'U': {
    auto n = e == 'x' ? std::size_t{2} : e == 'u' ? std::size_t{4} : std::size_t{8};
    if (rest.size() < n) {
      return syntax;
    }
    auto v = int32_t{0};
    for (std::size_t j = 0; j < n; ++j) {
      auto [x, ok] = detail::unhex(rest[j]);
      if (!ok) {
        return syntax;
      }
      v = static_cast<int32_t>(static_cast<uint32_t>(v) << 4 | static_cast<uint32_t>(x));
    }
    rest.remove_prefix(n);
    if (e == 'x') {
      value = v;
      break;
    }
    if (!detail::valid_rune(v)) {
      return syntax;
    }
    value = v;
    multibyte = true;
    break;
  }
  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7': {
    auto v = int32_t{e - '0'};
    if (rest.size() < 2) {
      return syntax;
    }
    for (std::size_t j = 0; j < 2; ++j) {
      auto x = rest[j] - '0';
      if (x < 0 || x > 7) {
        return syntax;
      }
      v = (v << 3) | x;
    }
    rest.remove_prefix(2);
    if (v > 255) {
      return syntax;
    }
    value = v;
    break;
  }
  case '\\':
    value = '\\';
    break;
  case '\'':
  case '"':
    if (static_cast<unsigned char>(e) != quote) {
      return syntax;
    }
    value = e;
    break;
  default:
    return syntax;
  }
  return {value, multibyte, std::string{rest}, nullptr};
}

// Go's Unquote: the string that a Go string, rune, or raw string literal denotes.
inline auto Unquote(std::string in) -> std::tuple<std::string, error> {
  auto syntax = std::tuple<std::string, error>{"", ErrSyntax};
  if (in.size() < 2) {
    return syntax;
  }
  auto quote = in[0];
  auto end = in.find(quote, 1);
  if (end == std::string::npos) {
    return syntax;
  }
  end += 1;
  auto rem = std::string_view{};
  auto out = std::string{};
  switch (quote) {
  case '`': {
    for (std::size_t i = 1; i < end - 1; ++i) {
      if (in[i] != '\r') {
        out += in[i];
      }
    }
    rem = std::string_view{in}.substr(end);
    break;
  }
  case '"':
  case '\'': {
    auto body = std::string_view{in}.substr(1, end - 2);
    if (body.find('\\') == std::string_view::npos && body.find('\n') == std::string_view::npos) {
      auto valid = false;
      auto text = std::string{body};
      if (quote == '"') {
        valid = true;
        for (std::size_t i = 0; i < text.size();) {
          auto [r, n] = eo::detail::decode_rune(text, i);
          if (r == 0xfffd && n == 1) {
            valid = false;
            break;
          }
          i += n;
        }
      } else if (!text.empty()) {
        auto [r, n] = eo::detail::decode_rune(text, 0);
        valid = n == text.size() && (r != 0xfffd || n != 1);
      }
      if (valid) {
        out = std::move(text);
        rem = std::string_view{in}.substr(end);
        break;
      }
    }
    auto rest = in.substr(1);
    while (!rest.empty() && rest[0] != quote) {
      auto [r, multibyte, tail, err] = UnquoteChar(rest, static_cast<uint8_t>(quote));
      if (rest[0] == '\n' || err != nullptr) {
        return syntax;
      }
      rest = std::move(tail);
      if (r < 0x80 || !multibyte) {
        out += static_cast<char>(r);
      } else {
        eo::detail::append_rune(out, r);
      }
      if (quote == '\'') {
        break;
      }
    }
    if (rest.empty() || rest[0] != quote) {
      return syntax;
    }
    if (rest.size() > 1) {
      return syntax;
    }
    return {out, nullptr};
  }
  default:
    return syntax;
  }
  if (!rem.empty()) {
    return syntax;
  }
  return {out, nullptr};
}

} // namespace eo::strconv
