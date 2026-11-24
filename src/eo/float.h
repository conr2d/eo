// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once

// Go's formatting of float32 and float64 values: %v, for fmt::Println and for the message of a panic
// value, and strconv.FormatFloat, which fmt's other float verbs use.

#include <fmt/format.h>

#include <algorithm>
#include <bit>
#include <charconv>
#include <cmath>
#include <concepts>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace eo::detail {

// A float32 or float64 value printed as Go's %v prints it, which differs from fmt's {}.
template<std::floating_point F>
struct go_float {
  F value;
};

// The value to format with {} for Go's %v: a float wrapped in go_float, and anything else itself.
template<typename T>
auto go_value(const T& v) -> decltype(auto) {
  if constexpr (std::floating_point<T>) {
    return go_float<T>{v};
  } else {
    return (v);
  }
}

// The decimal digits of a float's magnitude, as Go's decimalSlice: the digits d, with no trailing
// zeros, and the position dp of the decimal point, so that the magnitude is 0.d times 10 to the dp.
// Zero has no digits.
struct float_decimal {
  std::string d;
  int nd = 0;
  int dp = 0;
};

// The shortest digits that read back as the same float32 or float64 value when `digits` is -1, and
// otherwise the digits rounded to `digits` significant digits, rounding the exact value half to even
// as Go does. Neither has a trailing zero, since Go's rounding trims them.
inline auto float_digits(double v, int bit_size, int digits) -> float_decimal {
  auto r = float_decimal{};
  if (v == 0) {
    return r;
  }
  char buf[64];
  auto result = std::to_chars_result{};
  if (digits < 0) {
    result = bit_size == 32
      ? std::to_chars(buf, buf + sizeof(buf), std::fabs(static_cast<float>(v)), std::chars_format::scientific)
      : std::to_chars(buf, buf + sizeof(buf), std::fabs(v), std::chars_format::scientific);
  } else {
    result = std::to_chars(buf, buf + sizeof(buf), std::fabs(v), std::chars_format::scientific, digits - 1);
  }
  auto text = std::string_view(buf, result.ptr);
  auto e = text.find('e');
  for (auto c : text.substr(0, e)) {
    if (c != '.') {
      r.d += c;
    }
  }
  auto exponent = 0;
  std::from_chars(text.data() + e + 2, text.data() + text.size(), exponent);
  if (text[e + 1] == '-') {
    exponent = -exponent;
  }
  while (!r.d.empty() && r.d.back() == '0') {
    r.d.pop_back();
  }
  r.nd = static_cast<int>(r.d.size());
  r.dp = exponent + 1;
  return r;
}

// Go's strconv fmtE: d.ddddde±dd, with prec digits after the point.
inline void format_float_e(std::string& dst, bool neg, const float_decimal& d, int prec, char fmt) {
  if (neg) {
    dst += '-';
  }
  dst += d.nd != 0 ? d.d[0] : '0';
  if (prec > 0) {
    dst += '.';
    auto i = 1;
    auto m = std::min(d.nd, prec + 1);
    if (i < m) {
      dst.append(d.d, static_cast<std::size_t>(i), static_cast<std::size_t>(m - i));
      i = m;
    }
    for (; i <= prec; i++) {
      dst += '0';
    }
  }
  dst += fmt;
  auto exp = d.nd == 0 ? 0 : d.dp - 1;
  if (exp < 0) {
    dst += '-';
    exp = -exp;
  } else {
    dst += '+';
  }
  if (exp < 10) {
    dst += '0';
    dst += static_cast<char>('0' + exp);
  } else if (exp < 100) {
    dst += static_cast<char>('0' + exp / 10);
    dst += static_cast<char>('0' + exp % 10);
  } else {
    dst += static_cast<char>('0' + exp / 100);
    dst += static_cast<char>('0' + exp / 10 % 10);
    dst += static_cast<char>('0' + exp % 10);
  }
}

// Go's strconv fmtF: ddd.ddd, with prec digits after the point.
inline void format_float_f(std::string& dst, bool neg, const float_decimal& d, int prec) {
  if (neg) {
    dst += '-';
  }
  if (d.dp > 0) {
    auto m = std::min(d.nd, d.dp);
    dst.append(d.d, 0, static_cast<std::size_t>(m));
    for (; m < d.dp; m++) {
      dst += '0';
    }
  } else {
    dst += '0';
  }
  if (prec > 0) {
    dst += '.';
    for (auto i = 1; i <= prec; i++) {
      auto j = d.dp + i - 1;
      dst += 0 <= j && j < d.nd ? d.d[static_cast<std::size_t>(j)] : '0';
    }
  }
}

// The fields of a float32 or float64, after Go's genericFtoa: the sign, the mantissa with its
// implicit bit, and the binary exponent of its lowest bit plus the mantissa width.
struct float_bits {
  bool neg = false;
  uint64_t mant = 0;
  int exp = 0;
  int mantbits = 52;
};

inline auto decompose_float(double v, int bit_size) -> float_bits {
  auto mantbits = bit_size == 32 ? 23 : 52;
  auto expbits = bit_size == 32 ? 8 : 11;
  auto bias = bit_size == 32 ? -127 : -1023;
  auto bits =
    bit_size == 32 ? static_cast<uint64_t>(std::bit_cast<uint32_t>(static_cast<float>(v))) : std::bit_cast<uint64_t>(v);
  auto r = float_bits{};
  r.neg = (bits >> (expbits + mantbits)) != 0;
  r.exp = static_cast<int>(bits >> mantbits) & ((1 << expbits) - 1);
  r.mant = bits & ((uint64_t{1} << mantbits) - 1);
  r.mantbits = mantbits;
  if (r.exp == 0) {
    r.exp++;
  } else {
    r.mant |= uint64_t{1} << mantbits;
  }
  r.exp += bias;
  return r;
}

// Go's strconv fmtB: the mantissa and the binary exponent, as -ddddp±ddd.
inline void format_float_b(std::string& dst, const float_bits& f) {
  auto exp = f.exp - f.mantbits;
  if (f.neg) {
    dst += '-';
  }
  dst += std::to_string(f.mant);
  dst += 'p';
  if (exp >= 0) {
    dst += '+';
  }
  dst += std::to_string(exp);
}

// Go's strconv fmtX: -0x1.yyyyyyp±dd, with prec hexadecimal digits after the point, or the fewest
// that hold the mantissa when prec is -1.
inline void format_float_x(std::string& dst, int prec, char fmt, const float_bits& f) {
  auto mant = f.mant;
  auto exp = f.exp;
  if (mant == 0) {
    exp = 0;
  }
  // Shift the mantissa so that its leading 1 is bit 60.
  mant <<= 60 - f.mantbits;
  while (mant != 0 && (mant & (uint64_t{1} << 60)) == 0) {
    mant <<= 1;
    exp--;
  }
  // Round to prec digits, half to even.
  if (prec >= 0 && prec < 15) {
    auto shift = static_cast<unsigned>(prec * 4);
    auto extra = (mant << shift) & ((uint64_t{1} << 60) - 1);
    mant >>= 60 - shift;
    if ((extra | (mant & 1)) > (uint64_t{1} << 59)) {
      mant++;
    }
    mant <<= 60 - shift;
    if ((mant & (uint64_t{1} << 61)) != 0) {
      mant >>= 1;
      exp++;
    }
  }
  const char* hex = fmt == 'X' ? "0123456789ABCDEF" : "0123456789abcdef";
  if (f.neg) {
    dst += '-';
  }
  dst += '0';
  dst += fmt;
  dst += static_cast<char>('0' + ((mant >> 60) & 1));
  mant <<= 4;
  if (prec < 0 && mant != 0) {
    dst += '.';
    while (mant != 0) {
      dst += hex[(mant >> 60) & 15];
      mant <<= 4;
    }
  } else if (prec > 0) {
    dst += '.';
    for (auto i = 0; i < prec; i++) {
      dst += hex[(mant >> 60) & 15];
      mant <<= 4;
    }
  }
  dst += fmt == 'X' ? 'P' : 'p';
  if (exp < 0) {
    dst += '-';
    exp = -exp;
  } else {
    dst += '+';
  }
  if (exp < 100) {
    dst += static_cast<char>('0' + exp / 10);
    dst += static_cast<char>('0' + exp % 10);
  } else if (exp < 1000) {
    dst += static_cast<char>('0' + exp / 100);
    dst += static_cast<char>('0' + exp / 10 % 10);
    dst += static_cast<char>('0' + exp % 10);
  } else {
    dst += static_cast<char>('0' + exp / 1000);
    dst += static_cast<char>('0' + exp / 100 % 10);
    dst += static_cast<char>('0' + exp / 10 % 10);
    dst += static_cast<char>('0' + exp % 10);
  }
}

// Go's strconv.FormatFloat, after its genericFtoa, for a bit size of 32 or 64. A precision of -1
// asks for the fewest digits that read back as the same value. A format other than 'b', 'e', 'E',
// 'f', 'g', 'G', 'x', and 'X' gives % followed by the format, as in Go.
inline auto format_float(double v, char fmt, int prec, int bit_size) -> std::string {
  auto dst = std::string{};
  if (bit_size == 32) {
    v = static_cast<float>(v);
  }
  if (std::isnan(v)) {
    return "NaN";
  }
  if (std::isinf(v)) {
    return v > 0 ? "+Inf" : "-Inf";
  }
  auto neg = std::signbit(v);
  switch (fmt) {
  case 'b':
    format_float_b(dst, decompose_float(v, bit_size));
    return dst;
  case 'x':
  case 'X':
    format_float_x(dst, prec, fmt, decompose_float(v, bit_size));
    return dst;
  case 'e':
  case 'E': {
    auto d = float_digits(v, bit_size, prec < 0 ? -1 : prec + 1);
    format_float_e(dst, neg, d, prec < 0 ? std::max(d.nd - 1, 0) : prec, fmt);
    return dst;
  }
  case 'f': {
    if (prec < 0) {
      auto d = float_digits(v, bit_size, -1);
      format_float_f(dst, neg, d, std::max(d.nd - d.dp, 0));
      return dst;
    }
    // The exact value rounded half to even at prec digits after the point, as Go's ftoa rounds it.
    // A float64 has at most 309 digits before the point.
    auto buf = std::vector<char>(static_cast<std::size_t>(prec) + 330);
    auto result = std::to_chars(buf.data(), buf.data() + buf.size(), v, std::chars_format::fixed, prec);
    return std::string(buf.data(), result.ptr);
  }
  case 'g':
  case 'G': {
    auto shortest = prec < 0;
    if (prec == 0) {
      prec = 1;
    }
    auto d = float_digits(v, bit_size, shortest ? -1 : prec);
    if (shortest) {
      prec = d.nd;
    }
    auto eprec = prec;
    if (eprec > d.nd && d.nd >= d.dp) {
      eprec = d.nd;
    }
    // %e is used if the exponent from the conversion is less than -4 or at least the precision, and
    // the shortest form decides with a precision of 6.
    if (shortest) {
      eprec = 6;
    }
    auto exp = d.dp - 1;
    if (exp < -4 || exp >= eprec) {
      if (prec > d.nd) {
        prec = d.nd;
      }
      format_float_e(dst, neg, d, prec - 1, static_cast<char>(fmt + 'e' - 'g'));
      return dst;
    }
    if (prec > d.dp) {
      prec = d.nd;
    }
    format_float_f(dst, neg, d, std::max(prec - d.dp, 0));
    return dst;
  }
  default:
    dst += '%';
    dst += fmt;
    return dst;
  }
}

} // namespace eo::detail

namespace fmt {

// Go's %v of a float, which is strconv.FormatFloat(v, 'g', -1, bits): the shortest decimal that reads
// back as the same value, in exponent form when the exponent is below -4 or at least 6, as in 1e+06,
// and +Inf, -Inf, and NaN.
template<std::floating_point F>
struct formatter<eo::detail::go_float<F>> {
  constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    return ctx.end();
  }
  template<typename FormatContext>
  auto format(const eo::detail::go_float<F>& f, FormatContext& ctx) const {
    return fmt::format_to(ctx.out(), "{}", eo::detail::format_float(f.value, 'g', -1, sizeof(F) == 4 ? 32 : 64));
  }
};

} // namespace fmt
