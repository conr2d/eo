// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once
#include <eo/fields.h>
#include <eo/float.h>
#include <eo/interface.h>
#include <eo/panic.h>
#include <eo/utf8.h>

#include <boost/pfr.hpp>
#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <functional>
#include <iterator>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace fmt {

// A time point, which the printing below reaches through its fallback to fmt's formatters. It is
// declared here, before the printing, so that every translation unit sees the same formatter.
template<typename Clock>
struct formatter<std::chrono::time_point<Clock>> {
  constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    return ctx.end();
  }
  template<typename FormatContext>
  auto format(const std::chrono::time_point<Clock>& p, FormatContext& ctx) const {
    auto buffer = std::vector<char>(256);
    auto t = Clock::to_time_t(p);
    auto size = std::strftime(buffer.data(), buffer.size(), "%F %T %z %Z", std::localtime(&t));
    return fmt::format_to(ctx.out(), "{}", std::string_view(buffer.data(), size));
  }
};

} // namespace fmt

// Go's fmt printing. One engine, after Go's pp, prints every operand of Println, Sprint, Sprintf,
// Printf, Fprintf, and Errorf: print_arg follows Go's printArg, handle_methods its handleMethods,
// print_value its printValue, and print_format its doPrintf. The fmt_ functions and pad follow the
// methods of Go's fmt in format.go, with the flags, width, and precision in the printer.

namespace fmt {

struct State;
struct pp;

} // namespace fmt

namespace eo::detail {

template<typename T>
consteval auto is_composite_type() -> bool {
  if constexpr (defined_type<T>) {
    using U = typename T::_eo_underlying;
    return go_struct<U> || struct_underlying<U> || is_slice<U> || is_map<U> || is_std_array<U>;
  } else {
    return go_struct<T> || is_slice<T> || is_map<T> || is_std_array<T>;
  }
}

// Whether a pointer to T prints as & followed by its target: a struct, an array, a slice, or a map,
// or a defined type whose underlying type is one of them.
template<typename T>
inline constexpr bool is_composite = is_composite_type<T>();

// Go's internal/fmtsort order of map keys, which fmt prints a map in: numbers by value, with NaN
// before every other float, strings by bytes, false before true, pointers and channels by address,
// arrays element by element, and structs field by field. An interface puts nil first, then orders
// by dynamic type, and then by value. Go orders the types by the addresses of their descriptors, and
// so does Eo, so keys of different dynamic types can print in another order than Go's. Returns a
// negative number, zero, or a positive number.
template<typename K>
auto compare_keys(const K& a, const K& b) -> int {
  if constexpr (defined_type<K>) {
    return compare_keys(underlying(a), underlying(b));
  } else if constexpr (std::is_floating_point_v<K>) {
    if (std::isnan(a) || std::isnan(b)) {
      return std::isnan(a) ? (std::isnan(b) ? 0 : -1) : 1;
    }
    return a < b ? -1 : (b < a ? 1 : 0);
  } else if constexpr (std::is_arithmetic_v<K>) {
    return a < b ? -1 : (b < a ? 1 : 0);
  } else if constexpr (std::is_same_v<K, std::string>) {
    auto c = a.compare(b);
    return c < 0 ? -1 : (c > 0 ? 1 : 0);
  } else if constexpr (is_ptr<K> || is_chan_type<K> || requires { a._eo_key(); }) {
    auto address = [](const K& k) -> const void* {
      if constexpr (is_ptr<K>) {
        return k.get();
      } else if constexpr (is_chan_type<K>) {
        return k.shared_impl().get();
      } else {
        return k._eo_key();
      }
    };
    auto x = address(a);
    auto y = address(b);
    return std::less<const void*>{}(x, y) ? -1 : (x == y ? 0 : 1);
  } else if constexpr (std::is_base_of_v<interface_base, K>) {
    if (a._eo_type == nullptr || b._eo_type == nullptr) {
      return a._eo_type == b._eo_type ? 0 : (a._eo_type == nullptr ? -1 : 1);
    }
    if (a._eo_type != b._eo_type) {
      return std::less<const void*>{}(a._eo_type, b._eo_type) ? -1 : 1;
    }
    return a._eo_type->key_compare(a._eo_value.get(), b._eo_value.get());
  } else if constexpr (std::is_same_v<K, empty_struct>) {
    return 0;
  } else if constexpr (is_std_array<K>) {
    for (std::size_t i = 0; i < a.size(); ++i) {
      if (auto c = compare_keys(a[i], b[i]); c != 0) {
        return c;
      }
    }
    return 0;
  } else {
    auto c = 0;
    [&]<std::size_t... I>(std::index_sequence<I...>) {
      ((c = c != 0 ? c : compare_keys(boost::pfr::get<I>(a), boost::pfr::get<I>(b))), ...);
    }(std::make_index_sequence<boost::pfr::tuple_size_v<K>>{});
    return c;
  }
}

// The flags of one verb, as Go's fmtFlags. plus_v and sharp_v are the + and # flags of %v, which Go
// keeps apart from plus and sharp.
struct fmt_flags {
  bool wid_present = false;
  bool prec_present = false;
  bool minus = false;
  bool plus = false;
  bool sharp = false;
  bool space = false;
  bool zero = false;
  bool plus_v = false;
  bool sharp_v = false;
};

// The state of one call that prints, as Go's pp, with the flags, width, and precision of its fmt.
struct printer {
  std::string buf;
  // Errorf, where %w prints an error operand with %v.
  bool wrap_errs = false;
  // Printing the operand of a bad verb, which skips Error and String methods, as Go does.
  bool erroring = false;
  // Printing the value of a panic in an Error or String method. Another panic then propagates, as Go's
  // catchPanic does.
  bool panicking = false;
  // Printing an unexported struct field, or a value inside one, which Go cannot pass to a method
  // through an interface. Its methods are not called.
  bool unexported = false;
  fmt_flags flags{};
  int64_t wid = 0;
  int64_t prec = 0;

  void clear_flags() {
    flags = {};
    wid = 0;
    prec = 0;
  }
};

template<typename T>
inline constexpr bool is_string_like = std::is_convertible_v<const T&, std::string_view>;

// The Go type name of an operand, as %T and bad verbs print it: an interface value's dynamic type.
template<typename T>
auto type_of(const T& value) -> std::string {
  if constexpr (interface_type<T>) {
    return value._eo_type == nullptr ? std::string{"<nil>"} : value._eo_type->name();
  } else if constexpr (std::is_same_v<T, std::nullptr_t>) {
    return "<nil>";
  } else if constexpr (go_type<T>) {
    return type_name<T>();
  } else if constexpr (is_string_like<T>) {
    return "string";
  } else {
    return "?";
  }
}

// %!verb(type=value), with the value printed as %v without its methods, or %!verb(<nil>).
template<typename T>
void bad_verb(printer& p, int32_t verb, const T& value) {
  auto erroring = std::exchange(p.erroring, true);
  p.buf += "%!";
  append_rune(p.buf, verb);
  p.buf += '(';
  if constexpr (interface_type<T>) {
    if (value == nullptr) {
      p.buf += "<nil>";
    } else {
      p.buf += type_of(value);
      p.buf += '=';
      print_arg(p, value, 'v', 0);
    }
  } else {
    p.buf += type_of(value);
    p.buf += '=';
    print_arg(p, value, 'v', 0);
  }
  p.buf += ')';
  p.erroring = erroring;
}

inline constexpr const char* ldigits = "0123456789abcdefx";
inline constexpr const char* udigits = "0123456789ABCDEFX";

inline void append_hex(std::string& out, unsigned char byte, bool upper) {
  const char* digits = upper ? udigits : ldigits;
  out += digits[byte >> 4];
  out += digits[byte & 0xf];
}

// Go's unicode.IsPrint for a code point other than ASCII, except that it treats unassigned code
// points as printable. It rejects C1 controls, spaces other than U+0020, the common format
// characters, private use, and noncharacters.
inline auto is_print(char32_t r) -> bool {
  if (r < 0xa1) {
    return false;
  }
  return !(r == 0xad || r == 0x1680 || (r >= 0x2000 && r <= 0x200f) || (r >= 0x2028 && r <= 0x202f) ||
    (r >= 0x205f && r <= 0x2064) || r == 0x3000 || (r >= 0xe000 && r <= 0xf8ff) || r == 0xfeff ||
    (r >= 0xfff9 && r <= 0xfffb) || r == 0xfffe || r == 0xffff || (r >= 0xf0000));
}

// Go's strconv.appendEscapedRune. With `ascii`, as for QuoteToASCII, every code point outside
// printable ASCII is escaped.
inline void append_escaped_rune(std::string& out, char32_t r, char quote, bool ascii = false) {
  if (r == static_cast<char32_t>(quote) || r == '\\') {
    out += '\\';
    out += static_cast<char>(r);
    return;
  }
  if ((r >= 0x20 && r < 0x7f) ||
    (!ascii && r >= 0x80 && r <= 0x10ffff && !(r >= 0xd800 && r <= 0xdfff) && is_print(r))) {
    append_rune(out, static_cast<int32_t>(r));
    return;
  }
  switch (r) {
  case '\a':
    out += "\\a";
    return;
  case '\b':
    out += "\\b";
    return;
  case '\f':
    out += "\\f";
    return;
  case '\n':
    out += "\\n";
    return;
  case '\r':
    out += "\\r";
    return;
  case '\t':
    out += "\\t";
    return;
  case '\v':
    out += "\\v";
    return;
  default:
    break;
  }
  if (r < 0x20 || r == 0x7f) {
    out += "\\x";
    append_hex(out, static_cast<unsigned char>(r), false);
    return;
  }
  if (r > 0x10ffff || (r >= 0xd800 && r <= 0xdfff)) {
    r = 0xfffd;
  }
  if (r < 0x10000) {
    fmt::format_to(std::back_inserter(out), "\\u{:04x}", static_cast<uint32_t>(r));
  } else {
    fmt::format_to(std::back_inserter(out), "\\U{:08x}", static_cast<uint32_t>(r));
  }
}

// Go's strconv.Quote, or QuoteToASCII with `ascii`: a byte that does not start a valid UTF-8
// sequence is \x followed by its hex.
inline void append_quoted(std::string& out, std::string_view s, bool ascii = false) {
  out += '"';
  for (std::size_t i = 0; i < s.size();) {
    auto [r, n] = decode_rune(s, i);
    if (n == 1 && r == 0xfffd) {
      out += "\\x";
      append_hex(out, static_cast<unsigned char>(s[i]), false);
    } else {
      append_escaped_rune(out, r, '"', ascii);
    }
    i += n;
  }
  out += '"';
}

// Go's strconv.QuoteRune, or QuoteRuneToASCII with `ascii`, of a code point already made valid.
inline void append_quoted_rune(std::string& out, char32_t r, bool ascii = false) {
  out += '\'';
  append_escaped_rune(out, r, '\'', ascii);
  out += '\'';
}

// Go's strconv.CanBackquote.
inline auto can_backquote(std::string_view s) -> bool {
  for (std::size_t i = 0; i < s.size();) {
    auto [r, n] = decode_rune(s, i);
    i += n;
    if (n > 1) {
      if (r == 0xfeff) {
        return false;
      }
      continue;
    }
    if (r == 0xfffd) {
      return false;
    }
    if ((r < ' ' && r != '\t') || r == '`' || r == 0x7f) {
      return false;
    }
  }
  return true;
}

inline auto rune_count(std::string_view s) -> int64_t {
  auto count = int64_t{0};
  for (std::size_t i = 0; i < s.size(); ++count) {
    i += decode_rune(s, i).second;
  }
  return count;
}

// Go's fmt, the formatting of one value with the flags, width, and precision, as in Go's format.go.

inline void write_padding(printer& p, int64_t n) {
  if (n <= 0) {
    return;
  }
  p.buf.append(static_cast<std::size_t>(n), p.flags.zero && !p.flags.minus ? '0' : ' ');
}

// Go's pad and padString: the text, padded to the width with spaces or zeros, on the left unless the
// minus flag is set. The width counts runes, not bytes.
inline void pad(printer& p, std::string_view s) {
  if (!p.flags.wid_present || p.wid == 0) {
    p.buf += s;
    return;
  }
  auto width = p.wid - rune_count(s);
  if (!p.flags.minus) {
    write_padding(p, width);
    p.buf += s;
  } else {
    p.buf += s;
    write_padding(p, width);
  }
}

inline void fmt_boolean(printer& p, bool v) {
  pad(p, v ? "true" : "false");
}

// Go's strconv.IsPrint, for fmt.
inline auto is_print_rune(uint64_t r) -> bool {
  if (r < 0x80) {
    return r >= 0x20 && r < 0x7f;
  }
  return r <= 0x10ffff && !(r >= 0xd800 && r <= 0xdfff) && is_print(static_cast<char32_t>(r));
}

// Go's fmtUnicode: U+0078, with at least the precision in hex digits, and U+0078 'x' with the sharp
// flag for a printable code point.
inline void fmt_unicode(printer& p, uint64_t u) {
  auto prec = int64_t{4};
  if (p.flags.prec_present && p.prec > 4) {
    prec = p.prec;
  }
  // The text is built from its end, as Go builds it.
  auto out = std::string{};
  if (p.flags.sharp && is_print_rune(u)) {
    out += '\'';
    auto r = std::string{};
    append_rune(r, static_cast<int32_t>(u));
    out.append(r.rbegin(), r.rend());
    out += "' ";
  }
  for (; u >= 16; u >>= 4) {
    out += udigits[u & 0xf];
    --prec;
  }
  out += udigits[u];
  --prec;
  for (; prec > 0; --prec) {
    out += '0';
  }
  out += "+U";
  std::reverse(out.begin(), out.end());
  auto zero = std::exchange(p.flags.zero, false);
  pad(p, out);
  p.flags.zero = zero;
}

// Go's fmtInteger: u in the base, with the sign, the precision or the zero padding as the minimum
// number of digits, and the prefix of the sharp flag or of %O.
inline void fmt_integer(printer& p, uint64_t u, int base, bool is_signed, int32_t verb, const char* digits) {
  auto negative = is_signed && static_cast<int64_t>(u) < 0;
  if (negative) {
    u = 0 - u;
  }
  auto size = int64_t{68};
  if (p.flags.wid_present || p.flags.prec_present) {
    // Account 3 extra bytes for a possible sign and 0x.
    size = std::max(size, 3 + p.wid + p.prec);
  }
  auto buf = std::string(static_cast<std::size_t>(size), '\0');
  auto prec = int64_t{0};
  if (p.flags.prec_present) {
    prec = p.prec;
    // A precision of 0 and the value 0 print nothing but the padding.
    if (prec == 0 && u == 0) {
      auto zero = std::exchange(p.flags.zero, false);
      write_padding(p, p.wid);
      p.flags.zero = zero;
      return;
    }
  } else if (p.flags.zero && !p.flags.minus && p.flags.wid_present) {
    // Zero padding is allowed only to the left.
    prec = p.wid;
    if (negative || p.flags.plus || p.flags.space) {
      --prec;
    }
  }
  auto i = size;
  switch (base) {
  case 10:
    for (; u >= 10; u /= 10) {
      buf[static_cast<std::size_t>(--i)] = static_cast<char>('0' + u % 10);
    }
    break;
  case 16:
    for (; u >= 16; u >>= 4) {
      buf[static_cast<std::size_t>(--i)] = digits[u & 0xf];
    }
    break;
  case 8:
    for (; u >= 8; u >>= 3) {
      buf[static_cast<std::size_t>(--i)] = static_cast<char>('0' + (u & 7));
    }
    break;
  default:
    for (; u >= 2; u >>= 1) {
      buf[static_cast<std::size_t>(--i)] = static_cast<char>('0' + (u & 1));
    }
    break;
  }
  buf[static_cast<std::size_t>(--i)] = digits[u];
  while (i > 0 && prec > size - i) {
    buf[static_cast<std::size_t>(--i)] = '0';
  }
  if (p.flags.sharp) {
    switch (base) {
    case 2:
      buf[static_cast<std::size_t>(--i)] = 'b';
      buf[static_cast<std::size_t>(--i)] = '0';
      break;
    case 8:
      if (buf[static_cast<std::size_t>(i)] != '0') {
        buf[static_cast<std::size_t>(--i)] = '0';
      }
      break;
    case 16:
      buf[static_cast<std::size_t>(--i)] = digits[16];
      buf[static_cast<std::size_t>(--i)] = '0';
      break;
    default:
      break;
    }
  }
  if (verb == 'O') {
    buf[static_cast<std::size_t>(--i)] = 'o';
    buf[static_cast<std::size_t>(--i)] = '0';
  }
  if (negative) {
    buf[static_cast<std::size_t>(--i)] = '-';
  } else if (p.flags.plus) {
    buf[static_cast<std::size_t>(--i)] = '+';
  } else if (p.flags.space) {
    buf[static_cast<std::size_t>(--i)] = ' ';
  }
  // The zero flag was used for the precision above, so pad with spaces.
  auto zero = std::exchange(p.flags.zero, false);
  pad(p, std::string_view(buf).substr(static_cast<std::size_t>(i)));
  p.flags.zero = zero;
}

// Go's truncateString: s cut to the precision in runes.
inline auto truncate(const printer& p, std::string_view s) -> std::string_view {
  if (p.flags.prec_present) {
    auto n = p.prec;
    for (std::size_t i = 0; i < s.size(); i += decode_rune(s, i).second) {
      if (--n < 0) {
        return s.substr(0, i);
      }
    }
  }
  return s;
}

inline void fmt_s(printer& p, std::string_view s) {
  pad(p, truncate(p, s));
}

// Go's fmtSbx: the bytes in hex, cut to the precision in bytes, with spaces between them for the
// space flag and 0x before each, or before all, for the sharp flag.
inline void fmt_sbx(printer& p, std::string_view s, const char* digits) {
  auto length = static_cast<int64_t>(s.size());
  if (p.flags.prec_present && p.prec < length) {
    length = p.prec;
  }
  auto width = 2 * length;
  if (width > 0) {
    if (p.flags.space) {
      if (p.flags.sharp) {
        width *= 2;
      }
      width += length - 1;
    } else if (p.flags.sharp) {
      width += 2;
    }
  } else {
    // Nothing to encode: only the padding.
    if (p.flags.wid_present) {
      write_padding(p, p.wid);
    }
    return;
  }
  if (p.flags.wid_present && p.wid > width && !p.flags.minus) {
    write_padding(p, p.wid - width);
  }
  if (p.flags.sharp) {
    p.buf += '0';
    p.buf += digits[16];
  }
  for (int64_t i = 0; i < length; ++i) {
    if (p.flags.space && i > 0) {
      p.buf += ' ';
      if (p.flags.sharp) {
        p.buf += '0';
        p.buf += digits[16];
      }
    }
    auto c = static_cast<unsigned char>(s[static_cast<std::size_t>(i)]);
    p.buf += digits[c >> 4];
    p.buf += digits[c & 0xf];
  }
  if (p.flags.wid_present && p.wid > width && p.flags.minus) {
    write_padding(p, p.wid - width);
  }
}

// Go's fmtQ: a quoted string, in backquotes for the sharp flag if it can be, and escaped to ASCII for
// the plus flag.
inline void fmt_q(printer& p, std::string_view s) {
  s = truncate(p, s);
  if (p.flags.sharp && can_backquote(s)) {
    pad(p, "`" + std::string(s) + "`");
    return;
  }
  auto quoted = std::string{};
  append_quoted(quoted, s, p.flags.plus);
  pad(p, quoted);
}

inline auto valid_code_point(uint64_t c) -> char32_t {
  return c > 0x10ffff ? char32_t{0xfffd} : static_cast<char32_t>(c);
}

// Go's fmtC: the character, with U+FFFD for a value above the largest code point.
inline void fmt_c(printer& p, uint64_t c) {
  auto out = std::string{};
  append_rune(out, static_cast<int32_t>(valid_code_point(c)));
  pad(p, out);
}

// Go's fmtQc: the quoted character, escaped to ASCII for the plus flag.
inline void fmt_qc(printer& p, uint64_t c) {
  auto r = valid_code_point(c);
  if (r >= 0xd800 && r <= 0xdfff) {
    r = 0xfffd;
  }
  auto out = std::string{};
  append_quoted_rune(out, r, p.flags.plus);
  pad(p, out);
}

// Go's fmtFloat: strconv's formatting of v with the precision, then the sign, the sharp flag's
// decimal point and trailing zeros, and the padding.
inline void fmt_float(printer& p, double v, int size, int32_t verb, int64_t prec) {
  if (p.flags.prec_present) {
    prec = p.prec;
  }
  // The number, after a + that the code below replaces or drops.
  auto num = "+" + format_float(v, static_cast<char>(verb), static_cast<int>(prec), size);
  if (num[1] == '-' || num[1] == '+') {
    num.erase(0, 1);
  } else {
    num[0] = '+';
  }
  // The space flag puts a space where the sign of a positive number would be.
  if (p.flags.space && num[0] == '+' && !p.flags.plus) {
    num[0] = ' ';
  }
  // Infinities and NaN are not padded with zeros, and NaN has no sign unless asked for.
  if (num[1] == 'I' || num[1] == 'N') {
    auto zero = std::exchange(p.flags.zero, false);
    if (num[1] == 'N' && !p.flags.space && !p.flags.plus) {
      num.erase(0, 1);
    }
    pad(p, num);
    p.flags.zero = zero;
    return;
  }
  // The sharp flag forces a decimal point, and fills %v, %g, %G, and %x, though not %X, with
  // trailing zeros to the precision, or to 6 digits without one, as Go does.
  if (p.flags.sharp && verb != 'b') {
    auto digits = int64_t{0};
    switch (verb) {
    case 'v':
    case 'g':
    case 'G':
    case 'x':
      digits = prec;
      // Without a precision, use a precision of 6.
      if (digits == -1) {
        digits = 6;
      }
      break;
    default:
      break;
    }
    // The exponent, which is moved aside so that zeros can be added before it.
    auto tail = std::string{};
    auto has_decimal_point = false;
    auto saw_nonzero_digit = false;
    for (std::size_t i = 1; i < num.size(); ++i) {
      switch (num[i]) {
      case '.':
        has_decimal_point = true;
        break;
      case 'p':
      case 'P':
        tail = num.substr(i);
        num.resize(i);
        break;
      case 'e':
      case 'E':
        if (verb != 'x' && verb != 'X') {
          tail = num.substr(i);
          num.resize(i);
          break;
        }
        [[fallthrough]];
      default:
        if (num[i] != '0') {
          saw_nonzero_digit = true;
        }
        // Count the significant digits after the first nonzero digit.
        if (saw_nonzero_digit) {
          --digits;
        }
        break;
      }
    }
    if (!has_decimal_point) {
      // The leading digit 0 counts as a significant digit of a number without a decimal point.
      if (num.size() == 2 && num[1] == '0') {
        --digits;
      }
      num += '.';
    }
    for (; digits > 0; --digits) {
      num += '0';
    }
    num += tail;
  }
  // A sign of its own, from the plus flag or a negative number, goes before the zero padding.
  if (p.flags.plus || num[0] != '+') {
    if (p.flags.zero && !p.flags.minus && p.flags.wid_present && p.wid > static_cast<int64_t>(num.size())) {
      p.buf += num[0];
      write_padding(p, p.wid - static_cast<int64_t>(num.size()));
      p.buf.append(num, 1);
      return;
    }
    pad(p, num);
    return;
  }
  // No sign to show: drop the +.
  pad(p, std::string_view(num).substr(1));
}

// Go's pp.fmt0x64: v in hex, with 0x if leading0x.
inline void fmt0x64(printer& p, uint64_t v, bool leading0x) {
  auto sharp = std::exchange(p.flags.sharp, leading0x);
  fmt_integer(p, v, 16, false, 'v', ldigits);
  p.flags.sharp = sharp;
}

// Go's fmtString for the verbs of a string: false for another verb.
inline auto print_string(printer& p, std::string_view s, int32_t verb) -> bool {
  switch (verb) {
  case 'v':
    if (p.flags.sharp_v) {
      fmt_q(p, s);
    } else {
      fmt_s(p, s);
    }
    return true;
  case 's':
    fmt_s(p, s);
    return true;
  case 'x':
    fmt_sbx(p, s, ldigits);
    return true;
  case 'X':
    fmt_sbx(p, s, udigits);
    return true;
  case 'q':
    fmt_q(p, s);
    return true;
  default:
    return false;
  }
}

// Go's pp.fmtInteger: the verb picks the base, and %#v prints an unsigned integer in hex.
template<go_integer I>
void print_integer(printer& p, I v, int32_t verb) {
  constexpr auto is_signed = std::is_signed_v<I>;
  // Go converts the value to uint64, so a negative value is sign-extended.
  auto u = is_signed ? static_cast<uint64_t>(static_cast<int64_t>(v)) : static_cast<uint64_t>(v);
  switch (verb) {
  case 'v':
    if (p.flags.sharp_v && !is_signed) {
      fmt0x64(p, u, true);
    } else {
      fmt_integer(p, u, 10, is_signed, verb, ldigits);
    }
    return;
  case 'd':
    fmt_integer(p, u, 10, is_signed, verb, ldigits);
    return;
  case 'b':
    fmt_integer(p, u, 2, is_signed, verb, ldigits);
    return;
  case 'o':
  case 'O':
    fmt_integer(p, u, 8, is_signed, verb, ldigits);
    return;
  case 'x':
    fmt_integer(p, u, 16, is_signed, verb, ldigits);
    return;
  case 'X':
    fmt_integer(p, u, 16, is_signed, verb, udigits);
    return;
  case 'c':
    fmt_c(p, u);
    return;
  case 'q':
    fmt_qc(p, u);
    return;
  case 'U':
    fmt_unicode(p, u);
    return;
  default:
    bad_verb(p, verb, v);
  }
}

// Go's pp.fmtFloat: %v is the shortest %g, and %e, %f, and %F have a precision of 6.
template<std::floating_point F>
void print_float(printer& p, F v, int32_t verb) {
  constexpr auto size = sizeof(F) == 4 ? 32 : 64;
  switch (verb) {
  case 'v':
    fmt_float(p, v, size, 'g', -1);
    return;
  case 'b':
  case 'g':
  case 'G':
  case 'x':
  case 'X':
    fmt_float(p, v, size, verb, -1);
    return;
  case 'f':
  case 'e':
  case 'E':
    fmt_float(p, v, size, verb, 6);
    return;
  case 'F':
    fmt_float(p, v, size, 'f', 6);
    return;
  default:
    bad_verb(p, verb, v);
  }
}

// The method named `name` with signature `string()` in T's method set. It must exist.
template<typename T>
auto string_method(std::size_t position) -> const entry<std::string()>* {
  return static_cast<const entry<std::string()>*>(method_set<T>::entries[position]);
}

// Go's catchPanic, for a panic in an Error, String, GoString, or Format method that fmt called. It
// prints <nil> for a nil pointer, without padding, and otherwise the verb, the method, and the panic's value, without
// the flags. A panic while printing that value propagates.
template<typename T>
void catch_panic(printer& p, const T& value, int32_t verb, std::string_view method) {
  if constexpr (is_ptr<T>) {
    if (value == nullptr) {
      p.buf += "<nil>";
      return;
    }
  }
  if (p.panicking) {
    throw;
  }
  // Any exception is a panic, whose value prints as recover() would return it.
  auto recovered = panic_value(std::current_exception());
  auto flags = p.flags;
  auto wid = p.wid;
  auto prec = p.prec;
  p.clear_flags();
  p.buf += "%!";
  append_rune(p.buf, verb);
  p.buf += "(PANIC=";
  p.buf += method;
  p.buf += " method: ";
  p.panicking = true;
  print_arg(p, recovered, 'v', 0);
  p.panicking = false;
  p.buf += ')';
  p.flags = flags;
  p.wid = wid;
  p.prec = prec;
}

// Calls an Error, String, or GoString method for fmt and prints its result: with the verb, or with
// %s for GoString.
template<typename T>
void print_method(printer& p, const T& value, const entry<std::string()>* method, int32_t verb, bool go_string) {
  auto s = std::string{};
  try {
    s = method->fn(&value);
  } catch (...) {
    catch_panic(p, value, verb, method->name);
    return;
  }
  if (go_string) {
    fmt_s(p, s);
  } else {
    print_string(p, s, verb);
  }
}

// A Format method takes a fmt::State, which <eo/fmt.h> defines. A type with one has its Format
// declared with that header, so print_formatter, a template on it, sees the complete types.

// Calls a Format method with the printer as its State.
template<typename T, typename State = ::fmt::State, typename Pp = ::fmt::pp>
void print_formatter(printer& p, const T& value, const entry<void(State, int32_t)>* method, int32_t verb) {
  auto state = Pp{&p};
  try {
    method->fn(&value, Pp::_eo_state(state), verb);
  } catch (...) {
    catch_panic(p, value, verb, method->name);
  }
}

// Go's handleMethods: %w of an error in Errorf, a Formatter, and GoString for %#v, or Error or String,
// which Go prefers in that order, for %v, %s, %x, %X, and %q. Returns whether it printed the operand.
template<typename T>
auto handle_methods(printer& p, const T& value, int32_t verb) -> bool {
  if (p.erroring || p.unexported) {
    return false;
  }
  constexpr auto error = method_position<T, std::string()>("Error");
  constexpr auto string = method_position<T, std::string()>("String");
  constexpr auto go_string = method_position<T, std::string()>("GoString");
  if (verb == 'w') {
    if (error == no_method || !p.wrap_errs) {
      bad_verb(p, verb, value);
      return true;
    }
    verb = 'v';
  }
  // A Formatter prints itself for every verb.
  constexpr auto format = method_position<T, void(::fmt::State, int32_t)>("Format");
  if constexpr (format != no_method) {
    print_formatter(p,
      value,
      static_cast<const entry<void(::fmt::State, int32_t)>*>(method_set<T>::entries[format]),
      verb);
    return true;
  }
  if (p.flags.sharp_v) {
    if constexpr (go_string != no_method) {
      print_method(p, value, string_method<T>(go_string), verb, true);
      return true;
    }
    return false;
  }
  if constexpr (error != no_method || string != no_method) {
    switch (verb) {
    case 'v':
    case 's':
    case 'x':
    case 'X':
    case 'q': {
      constexpr auto position = error != no_method ? error : string;
      print_method(p, value, string_method<T>(position), verb, false);
      return true;
    }
    default:
      break;
    }
  }
  return false;
}

// Go's fmtPointer, for an address: <nil> or 0x followed by the address for %v, and %p and the integer
// verbs as for an unsigned integer. %#v prints (type)(address), with nil for a nil address.
template<typename T>
void print_address(printer& p, const T& value, std::uintptr_t address, int32_t verb) {
  switch (verb) {
  case 'v':
    if (p.flags.sharp_v) {
      p.buf += '(';
      p.buf += type_of(value);
      p.buf += ")(";
      if (address == 0) {
        p.buf += "nil";
      } else {
        fmt0x64(p, address, true);
      }
      p.buf += ')';
    } else if (address == 0) {
      pad(p, "<nil>");
    } else {
      fmt0x64(p, address, !p.flags.sharp);
    }
    return;
  case 'p':
    fmt0x64(p, address, !p.flags.sharp);
    return;
  case 'b':
  case 'o':
  case 'd':
  case 'x':
  case 'X':
    print_integer(p, static_cast<uint64_t>(address), verb);
    return;
  default:
    bad_verb(p, verb, value);
  }
}

// The address that Go's %p prints for a pointer, a slice, or a map, or nullopt for another type.
template<typename T>
auto address_of(const T& value) -> std::optional<std::uintptr_t> {
  if constexpr (defined_type<T>) {
    return address_of(underlying(value));
  } else if constexpr (is_ptr<T>) {
    return reinterpret_cast<std::uintptr_t>(value.get());
  } else if constexpr (is_slice<T>) {
    return reinterpret_cast<std::uintptr_t>(value.data());
  } else if constexpr (is_map<T>) {
    return reinterpret_cast<std::uintptr_t>(value.table());
  } else {
    return std::nullopt;
  }
}

// The separator between the elements of a slice, an array, a map, or a struct: ", " for %#v.
inline void print_separator(printer& p) {
  if (p.flags.sharp_v) {
    p.buf += ", ";
  } else {
    p.buf += ' ';
  }
}

// Go's printValue, after the methods: the operand's kind decides how it prints. `type` names the
// operand's Go type for %#v, which differs from the type of `value` when that is the underlying value
// of a defined type.
template<typename T>
void print_value(printer& p, const T& value, int32_t verb, int depth, const std::string& type) {
  if constexpr (is_slice<T> || is_std_array<T>) {
    if constexpr (std::is_same_v<typename T::value_type, uint8_t>) {
      if (verb == 's' || verb == 'q' || verb == 'x' || verb == 'X') {
        print_string(p, std::string_view(reinterpret_cast<const char*>(value.data()), value.size()), verb);
        return;
      }
    }
    if (p.flags.sharp_v) {
      p.buf += type;
      if constexpr (is_slice<T>) {
        if (value == nullptr) {
          p.buf += "(nil)";
          return;
        }
      }
      p.buf += '{';
    } else {
      p.buf += '[';
    }
    auto first = true;
    for (const auto& element : value) {
      if (!first) {
        print_separator(p);
      }
      print_arg(p, element, verb, depth + 1);
      first = false;
    }
    p.buf += p.flags.sharp_v ? '}' : ']';
  } else if constexpr (is_map<T>) {
    using K = typename T::key_type;
    using V = typename T::mapped_type;
    if (p.flags.sharp_v) {
      p.buf += type;
      if (value == nullptr) {
        p.buf += "(nil)";
        return;
      }
      p.buf += '{';
    } else {
      p.buf += "map[";
    }
    auto entries = std::vector<std::pair<const K*, const V*>>{};
    if (auto* t = value.table(); t != nullptr) {
      entries.reserve(static_cast<std::size_t>(t->size()));
      t->for_each([&](const K& k, const V& v) { entries.emplace_back(&k, &v); });
    }
    std::stable_sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) {
      return compare_keys(*a.first, *b.first) < 0;
    });
    auto first = true;
    for (const auto& [k, v] : entries) {
      if (!first) {
        print_separator(p);
      }
      print_arg(p, *k, verb, depth + 1);
      p.buf += ':';
      print_arg(p, *v, verb, depth + 1);
      first = false;
    }
    p.buf += p.flags.sharp_v ? '}' : ']';
  } else if constexpr (is_ptr<T>) {
    // A pointer operand to a struct, an array, a slice, or a map prints as & followed by its target,
    // and any other pointer as its address.
    if constexpr (is_composite<typename pointee<T>::type>) {
      if (depth == 0 && value != nullptr) {
        p.buf += '&';
        print_arg(p, *value, verb, depth + 1);
        return;
      }
    }
    print_address(p, value, reinterpret_cast<std::uintptr_t>(value.get()), verb);
  } else if constexpr (go_struct<T>) {
    if (p.flags.sharp_v) {
      p.buf += type;
    }
    p.buf += '{';
    [&]<std::size_t... I>(std::index_sequence<I...>) {
      auto field = [&]<std::size_t J>(std::integral_constant<std::size_t, J>) {
        if (J > 0) {
          print_separator(p);
        }
        const auto name = field_name<T, J>();
        if (p.flags.plus_v || p.flags.sharp_v) {
          p.buf += name;
          p.buf += ':';
        }
        // Go exports a field whose name starts with an upper-case letter. Eo checks only ASCII ones.
        const auto unexported = p.unexported;
        p.unexported = unexported || name.empty() || name[0] < 'A' || name[0] > 'Z';
        print_arg(p, boost::pfr::get<J>(value), verb, depth + 1);
        p.unexported = unexported;
      };
      (field(std::integral_constant<std::size_t, I>{}), ...);
    }(std::make_index_sequence<boost::pfr::tuple_size_v<T>>{});
    p.buf += '}';
  } else if constexpr (fmt::is_formattable<T>::value) {
    fmt::format_to(std::back_inserter(p.buf), "{}", value);
  } else {
    p.buf += "?";
  }
}

template<typename T>
void print_value(printer& p, const T& value, int32_t verb, int depth) {
  if (p.flags.sharp_v) {
    print_value(p, value, verb, depth, type_of(value));
  } else {
    print_value(p, value, verb, depth, std::string{});
  }
}

// Whether a verb prints a value of the basic type T, rather than the bad-verb form.
template<typename T>
constexpr auto accepts_verb(int32_t verb) -> bool {
  if constexpr (std::is_same_v<T, bool>) {
    return verb == 't' || verb == 'v';
  } else if constexpr (go_integer<T>) {
    switch (verb) {
    case 'v':
    case 'd':
    case 'b':
    case 'o':
    case 'O':
    case 'x':
    case 'X':
    case 'c':
    case 'q':
    case 'U':
      return true;
    default:
      return false;
    }
  } else if constexpr (std::floating_point<T>) {
    switch (verb) {
    case 'v':
    case 'b':
    case 'g':
    case 'G':
    case 'x':
    case 'X':
    case 'e':
    case 'E':
    case 'f':
    case 'F':
      return true;
    default:
      return false;
    }
  } else if constexpr (std::is_same_v<T, std::string>) {
    return verb == 'v' || verb == 's' || verb == 'x' || verb == 'X' || verb == 'q';
  } else {
    return true;
  }
}

// Go's printArg: prints one operand with a verb. `depth` is 0 for an operand of the call.
template<typename T>
void print_arg(printer& p, const T& value, int32_t verb, int depth) {
  if constexpr (std::is_same_v<T, std::nullptr_t>) {
    // Go's nil as an operand, which the parameter of type any holds as a nil interface value.
    print_arg(p, any{}, verb, depth);
  } else if constexpr (interface_type<T>) {
    if (value == nullptr) {
      // A nil interface inside another value prints with its static type for %#v, as error(nil).
      if (depth > 0 && p.flags.sharp_v) {
        p.buf += type_name<T>();
        p.buf += "(nil)";
      } else if (depth > 0) {
        p.buf += "<nil>";
      } else if (verb == 'v' || verb == 'T') {
        pad(p, "<nil>");
      } else {
        bad_verb(p, verb, value);
      }
      return;
    }
    value._eo_type->print(p, value._eo_value.get(), verb, depth == 0 ? 0 : depth + 1);
  } else {
    if (depth == 0 && verb == 'T') {
      fmt_s(p, type_of(value));
      return;
    }
    if (depth == 0 && verb == 'p') {
      if (auto address = address_of(value)) {
        print_address(p, value, *address, 'p');
      } else {
        bad_verb(p, verb, value);
      }
      return;
    }
    if constexpr (defined_type<T>) {
      // A defined type prints by its methods, and otherwise as its underlying value, except that a bad
      // verb and %#v of a composite value name the defined type, as Go's do.
      if (handle_methods(p, value, verb)) {
        return;
      }
      using U = typename T::_eo_underlying;
      if (!accepts_verb<U>(verb)) {
        bad_verb(p, verb, value);
        return;
      }
      if constexpr (is_composite<U>) {
        if (p.flags.sharp_v) {
          print_value(p, underlying(value), verb, depth, type_name<T>());
          return;
        }
      }
      // A struct prints by its fields, without the methods of U, which T does not have: a translated
      // struct by print_value, and a struct of a package that Eo maps by its _eo_print.
      if constexpr (go_struct<U>) {
        print_value(p, underlying(value), verb, depth, type_name<T>());
        return;
      } else if constexpr (struct_underlying<U> && requires { _eo_print(p, underlying(value), verb, depth); }) {
        _eo_print(p, underlying(value), verb, depth);
        return;
      }
      print_arg(p, underlying(value), verb, depth);
    } else if constexpr (std::is_same_v<T, bool>) {
      if (verb == 't' || verb == 'v') {
        fmt_boolean(p, value);
      } else {
        bad_verb(p, verb, value);
      }
    } else if constexpr (go_integer<T>) {
      print_integer(p, value, verb);
    } else if constexpr (std::floating_point<T>) {
      print_float(p, value, verb);
    } else if constexpr (std::is_same_v<T, std::string>) {
      if (!print_string(p, value, verb)) {
        bad_verb(p, verb, value);
      }
    } else if constexpr (is_string_like<T>) {
      print_arg(p, std::string{std::string_view{value}}, verb, depth);
    } else {
      if (handle_methods(p, value, verb)) {
        return;
      }
      // A library type whose Go type is a struct with unexported fields, such as a big.Int value,
      // prints them by _eo_print, found by argument-dependent lookup.
      if constexpr (requires { _eo_print(p, value, verb, depth); }) {
        _eo_print(p, value, verb, depth);
        return;
      }
      // Go's printArg names a []byte operand []byte for %#v, where printValue names it []uint8.
      if constexpr (std::is_same_v<T, slice<uint8_t>>) {
        if (depth == 0 && p.flags.sharp_v) {
          print_value(p, value, verb, depth, "[]byte");
          return;
        }
      }
      print_value(p, value, verb, depth);
    }
  }
}

// One operand of a Printf-like call, with its type erased.
struct operand {
  const void* value;
  void (*print)(printer& p, const void* value, int32_t verb, int depth);
  // The operand as an error, for Errorf's %w, or nil if it is not one.
  auto (*as_error)(const void* value) -> error;
  // The operand as an int for a * width or precision, as Go's intFromArg: false unless it is an
  // integer whose value fits.
  auto (*as_int)(const void* value, int64_t& out) -> bool;
};

template<typename T>
void print_operand(printer& p, const void* value, int32_t verb, int depth) {
  print_arg(p, *static_cast<const T*>(value), verb, depth);
}

template<typename T>
auto operand_error(const void* value) -> error {
  const auto& v = *static_cast<const T*>(value);
  if constexpr (std::is_same_v<T, error>) {
    return v;
  } else if constexpr (interface_type<T>) {
    return type_assert_ok<error>(v).first;
  } else if constexpr (go_type<T>) {
    if constexpr (method_position<T, std::string()>("Error") != no_method) {
      return error{v};
    } else {
      return nullptr;
    }
  } else {
    return nullptr;
  }
}

// Go's tooLarge: a width or precision that is not reasonable.
inline auto too_large(int64_t x) -> bool {
  constexpr auto max = int64_t{1000000};
  return x > max || x < -max;
}

template<typename T>
auto int_from(const T& v, int64_t& out) -> bool {
  if constexpr (defined_type<T>) {
    return int_from(underlying(v), out);
  } else if constexpr (go_integer<T>) {
    if constexpr (std::is_signed_v<T>) {
      out = static_cast<int64_t>(v);
    } else {
      if (static_cast<uint64_t>(v) > static_cast<uint64_t>(std::numeric_limits<int64_t>::max())) {
        return false;
      }
      out = static_cast<int64_t>(v);
    }
    return true;
  } else if constexpr (interface_type<T>) {
    return v != nullptr && v._eo_type->as_int(v._eo_value.get(), out);
  } else {
    return false;
  }
}

template<typename T>
auto operand_int(const void* value, int64_t& out) -> bool {
  if (!int_from(*static_cast<const T*>(value), out)) {
    out = 0;
    return false;
  }
  if (too_large(out)) {
    out = 0;
    return false;
  }
  return true;
}

template<typename T>
auto make_operand(const T& value) -> operand {
  return {&value, &print_operand<T>, &operand_error<T>, &operand_int<T>};
}

// Go's parsenum: the decimal number at s[start:end], whether there is one, and the index after it.
inline auto parse_num(
  std::string_view s, std::size_t start, std::size_t end) -> std::tuple<int64_t, bool, std::size_t> {
  if (start >= end) {
    return {0, false, end};
  }
  auto num = int64_t{0};
  auto isnum = false;
  auto i = start;
  for (; i < end && s[i] >= '0' && s[i] <= '9'; ++i) {
    // Overflow, most likely a very long number.
    if (too_large(num)) {
      return {0, false, end};
    }
    num = num * 10 + (s[i] - '0');
    isnum = true;
  }
  return {num, isnum, i};
}

// Go's parseArgNumber: the index of the bracketed operand number at the start of s, the length of
// the brackets, and whether the number is valid.
inline auto parse_arg_number(std::string_view s) -> std::tuple<int64_t, std::size_t, bool> {
  // There must be at least 3 bytes: [n].
  if (s.size() < 3) {
    return {0, 1, false};
  }
  for (std::size_t i = 1; i < s.size(); ++i) {
    if (s[i] == ']') {
      auto [width, ok, newi] = parse_num(s, 1, i);
      if (!ok || newi != i) {
        return {0, i + 1, false};
      }
      // Operand numbers start at 1.
      return {width - 1, i + 1, true};
    }
  }
  return {0, 1, false};
}

// The state of Go's doPrintf around the operands.
struct printf_state {
  bool reordered = false;
  bool good_arg_num = true;
};

// Go's argNumber: the operand to print next, after an optional [n] at format[i].
inline auto arg_number(printf_state& st,
  std::size_t arg_num,
  std::string_view format,
  std::size_t i,
  std::size_t num_args) -> std::tuple<std::size_t, std::size_t, bool> {
  if (format.size() <= i || format[i] != '[') {
    return {arg_num, i, false};
  }
  st.reordered = true;
  auto [index, wid, ok] = parse_arg_number(format.substr(i));
  if (ok && 0 <= index && index < static_cast<int64_t>(num_args)) {
    return {static_cast<std::size_t>(index), i + wid, true};
  }
  st.good_arg_num = false;
  return {arg_num, i + wid, ok};
}

// Go's doPrintf. The positions of the operands of %w are added to `wrapped`. Returns whether an
// operand index reordered the operands.
inline auto print_format(
  printer& p, std::string_view format, std::span<const operand> operands, std::vector<std::size_t>* wrapped) -> bool {
  auto end = format.size();
  auto arg_num = std::size_t{0};
  // Whether the previous item in the format was an index like [3].
  auto after_index = false;
  auto st = printf_state{};
  for (std::size_t i = 0; i < end;) {
    st.good_arg_num = true;
    auto lasti = i;
    while (i < end && format[i] != '%') {
      ++i;
    }
    if (i > lasti) {
      p.buf += format.substr(lasti, i - lasti);
    }
    if (i >= end) {
      break;
    }
    // Skip the %.
    ++i;
    p.clear_flags();
    for (; i < end; ++i) {
      auto c = format[i];
      if (c == '#') {
        p.flags.sharp = true;
      } else if (c == '0') {
        p.flags.zero = true;
      } else if (c == '+') {
        p.flags.plus = true;
      } else if (c == '-') {
        p.flags.minus = true;
      } else if (c == ' ') {
        p.flags.space = true;
      } else {
        break;
      }
    }
    // An operand index for the width.
    std::tie(arg_num, i, after_index) = arg_number(st, arg_num, format, i, operands.size());
    if (i < end && format[i] == '*') {
      ++i;
      auto present = false;
      if (arg_num < operands.size()) {
        present = operands[arg_num].as_int(operands[arg_num].value, p.wid);
        ++arg_num;
      } else {
        p.wid = 0;
      }
      p.flags.wid_present = present;
      if (!present) {
        p.buf += "%!(BADWIDTH)";
      }
      // A negative width is the minus flag, and zeros do not pad to the right.
      if (p.wid < 0) {
        p.wid = -p.wid;
        p.flags.minus = true;
        p.flags.zero = false;
      }
      after_index = false;
    } else {
      std::tie(p.wid, p.flags.wid_present, i) = parse_num(format, i, end);
      // An index before a width, as in %[3]2d, is not allowed.
      if (after_index && p.flags.wid_present) {
        st.good_arg_num = false;
      }
    }
    if (i + 1 < end && format[i] == '.') {
      ++i;
      // An index before a precision, as in %[3].2d, is not allowed.
      if (after_index) {
        st.good_arg_num = false;
      }
      std::tie(arg_num, i, after_index) = arg_number(st, arg_num, format, i, operands.size());
      if (i < end && format[i] == '*') {
        ++i;
        auto present = false;
        if (arg_num < operands.size()) {
          present = operands[arg_num].as_int(operands[arg_num].value, p.prec);
          ++arg_num;
        } else {
          p.prec = 0;
        }
        p.flags.prec_present = present;
        // A negative precision is no precision.
        if (p.prec < 0) {
          p.prec = 0;
          p.flags.prec_present = false;
        }
        if (!p.flags.prec_present) {
          p.buf += "%!(BADPREC)";
        }
        after_index = false;
      } else {
        std::tie(p.prec, p.flags.prec_present, i) = parse_num(format, i, end);
        // A period with no number is a precision of 0.
        if (!p.flags.prec_present) {
          p.prec = 0;
          p.flags.prec_present = true;
        }
      }
    }
    if (!after_index) {
      std::tie(arg_num, i, after_index) = arg_number(st, arg_num, format, i, operands.size());
    }
    if (i >= end) {
      p.buf += "%!(NOVERB)";
      break;
    }
    auto [r, n] = decode_rune(format, i);
    i += n;
    auto verb = static_cast<int32_t>(r);
    if (verb == '%') {
      // A percent sign takes no operand and ignores the width and precision.
      p.buf += '%';
    } else if (!st.good_arg_num) {
      p.buf += "%!";
      append_rune(p.buf, verb);
      p.buf += "(BADINDEX)";
    } else if (arg_num >= operands.size()) {
      p.buf += "%!";
      append_rune(p.buf, verb);
      p.buf += "(MISSING)";
    } else {
      if (verb == 'w' && wrapped != nullptr) {
        wrapped->push_back(arg_num);
      }
      if (verb == 'v' || verb == 'w') {
        // %+v and %#v keep their flags apart, so that a Formatter and nested values see them.
        p.flags.sharp_v = p.flags.sharp;
        p.flags.sharp = false;
        p.flags.plus_v = p.flags.plus;
        p.flags.plus = false;
      }
      const auto& o = operands[arg_num++];
      o.print(p, o.value, verb, 0);
    }
  }
  // Operands that no verb used, unless the format reorders them.
  if (!st.reordered && arg_num < operands.size()) {
    p.clear_flags();
    p.buf += "%!(EXTRA ";
    for (auto j = arg_num; j < operands.size(); ++j) {
      if (j > arg_num) {
        p.buf += ", ";
      }
      // The type and %v of the operand, which is <nil> for a nil interface value.
      auto typed = printer{};
      operands[j].print(typed, operands[j].value, 'T', 0);
      if (typed.buf != "<nil>") {
        p.buf += typed.buf;
        p.buf += '=';
      }
      operands[j].print(p, operands[j].value, 'v', 0);
    }
    p.buf += ')';
  }
  return st.reordered;
}

template<typename... T>
auto sprintf(const std::string& format, const T&... args) -> std::string {
  auto p = printer{};
  const auto operands = std::array<operand, sizeof...(T)>{make_operand(args)...};
  print_format(p, format, operands, nullptr);
  return std::move(p.buf);
}

// Whether Go's Sprint would see the operand as a string, which gets no space next to another
// operand: a value whose kind is string, such as a string or a defined string type.
template<typename T>
auto is_string_operand(const T& value) -> bool {
  if constexpr (interface_type<T>) {
    return value._eo_type != nullptr && value._eo_type->string_kind;
  } else {
    return is_string_like<T>;
  }
}

template<typename... T>
auto sprint(bool line, const T&... args) -> std::string {
  auto p = printer{};
  auto first = true;
  auto previous_string = false;
  auto print = [&](const auto& arg) {
    auto string = is_string_operand(arg);
    // Println separates every pair of operands, and Print only those where neither is a string.
    if (!first && (line || (!string && !previous_string))) {
      p.buf += ' ';
    }
    print_arg(p, arg, 'v', 0);
    first = false;
    previous_string = string;
  };
  (print(args), ...);
  if (line) {
    p.buf += '\n';
  }
  return std::move(p.buf);
}

// Go's fmt functions write each call to the unbuffered os.Stdout with a single write. Write the whole
// text, then flush it, so output is not held in a stdio buffer and lines from concurrent goroutines
// do not interleave.
inline void write_text(std::FILE* f, const std::string& text) {
  std::fwrite(text.data(), 1, text.size(), f);
  // runtime::exit skips stdout while another thread holds its lock, so this flush is what keeps a
  // goroutine's last line before os::Exit.
  std::fflush(f);
}

inline auto print_panic_value(const any& value) -> std::string {
  auto p = printer{};
  print_arg(p, value, 'v', 0);
  return std::move(p.buf);
}

// %v of a value, as Go's fmt prints it. `top` is true for an operand of Println.
template<typename T>
struct printed {
  const T& value;
  bool top;
};

template<typename T>
auto go_print(const T& value, bool top = false) -> printed<T> {
  return {value, top};
}

} // namespace eo::detail
