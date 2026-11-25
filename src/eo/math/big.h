// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once

#include <eo/fmt.h>

#include <boost/multiprecision/cpp_int.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

// Go's math/big, in part: big.Int and big.Word. An Int holds its value in a Boost.Multiprecision
// cpp_int, whose sign and magnitude match Go's representation, and its methods follow Go's
// int.go, including the identities that give the bit operations two's complement semantics on
// negative values.
//
// The methods are static members whose first parameter is the receiver, as for a translated type,
// because Go's methods return their receiver, and the result must keep it alive: z.Add(x, y) is
// big::Int::Add(z, x, y).

namespace eo::big {

// Go's big.Word, the unsigned integer of one digit of an Int's magnitude: uint on a 64-bit target.
struct Word : eo::defined<Word, uint64_t> {
  using defined::defined;
  static constexpr std::string_view _eo_name = "big.Word";
};

struct Int;

namespace detail {

  // Without expression templates, so that every operation gives a cpp_int, which auto and ?: need.
  using cpp_int =
    boost::multiprecision::number<boost::multiprecision::cpp_int_backend<>, boost::multiprecision::et_off>;

  inline constexpr std::string_view digits = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
  inline constexpr int64_t max_base = 62;
  inline constexpr int64_t max_base_small = 36;

  [[noreturn]] inline void panic_division_by_zero() {
    panic("division by zero"s);
  }

  // The low 64 bits of the magnitude, as Go's low64.
  inline auto low64(const cpp_int& magnitude) -> uint64_t {
    return static_cast<uint64_t>(magnitude & cpp_int{UINT64_MAX});
  }

  // The magnitude in base `base`, without a sign: Go's nat.utoa.
  inline auto utoa(cpp_int magnitude, int64_t base) -> std::string {
    if (magnitude == 0) {
      return "0";
    }
    if (base == 10) {
      return magnitude.str();
    }
    auto s = std::string{};
    auto b = cpp_int{base};
    while (magnitude != 0) {
      cpp_int q;
      cpp_int r;
      boost::multiprecision::divide_qr(magnitude, b, q, r);
      s += digits[static_cast<std::size_t>(r)];
      magnitude = std::move(q);
    }
    return {s.rbegin(), s.rend()};
  }

  // Go's nat.scan without fractions, on s from position i: the magnitude, the position after it,
  // and whether Go accepts the digits and separators it read.
  struct scanned {
    cpp_int magnitude;
    std::size_t next;
    bool ok;
  };

  inline auto scan(std::string_view s, std::size_t i, int64_t base) -> scanned {
    if (!(base == 0 || (2 <= base && base <= max_base))) {
      panic(fmt::Sprintf("invalid number base %d"s, base));
    }
    auto at_end = [&] { return i >= s.size(); };
    // prev is '_', '0' for a digit, or '.' for anything else. A separator '_' may only follow a
    // digit, and only when base is 0.
    auto prev = '.';
    auto invalid_separator = false;
    auto b = base;
    auto prefix = '\0';
    auto count = 0;
    if (base == 0) {
      b = 10;
      if (!at_end() && s[i] == '0') {
        prev = '0';
        count = 1;
        ++i;
        if (!at_end()) {
          switch (s[i]) {
          case 'b':
          case 'B':
            b = 2;
            prefix = 'b';
            break;
          case 'o':
          case 'O':
            b = 8;
            prefix = 'o';
            break;
          case 'x':
          case 'X':
            b = 16;
            prefix = 'x';
            break;
          default:
            b = 8;
            prefix = '0';
          }
          count = 0;
          if (prefix != '0') {
            ++i;
          }
        }
      }
    }
    auto magnitude = cpp_int{};
    for (; !at_end(); ++i) {
      auto ch = s[i];
      if (ch == '_' && base == 0) {
        if (prev != '0') {
          invalid_separator = true;
        }
        prev = '_';
        continue;
      }
      auto d = max_base + 1;
      if ('0' <= ch && ch <= '9') {
        d = ch - '0';
      } else if ('a' <= ch && ch <= 'z') {
        d = ch - 'a' + 10;
      } else if ('A' <= ch && ch <= 'Z') {
        d = b <= max_base_small ? ch - 'A' + 10 : ch - 'A' + max_base_small;
      }
      if (d >= b) {
        break;
      }
      prev = '0';
      ++count;
      magnitude = magnitude * b + d;
    }
    auto ok = !(invalid_separator || prev == '_');
    if (count == 0) {
      if (prefix == '0') {
        // Only the octal prefix 0, possibly followed by separators and digits above 7: decimal 0.
        return {cpp_int{}, i, ok};
      }
      ok = false;
    }
    return {std::move(magnitude), i, ok};
  }

} // namespace detail

// Go's big.Int. The zero value is 0, as in Go. Copying an Int copies its value, where Go copies the
// header of the slice that holds its magnitude, and go vet does not report it: a program that changes
// one copy and reads the other is outside this mapping.
struct Int {
  static constexpr std::string_view _eo_name = "big.Int";

  Int() = default;

  static auto Set(ptr<Int> z, ptr<Int> x) -> ptr<Int> {
    if (z != x) {
      z->value_ = x->value_;
    }
    return z;
  }

  static auto SetInt64(ptr<Int> z, int64_t x) -> ptr<Int> {
    z->value_ = x;
    return z;
  }

  static auto SetUint64(ptr<Int> z, uint64_t x) -> ptr<Int> {
    z->value_ = x;
    return z;
  }

  // Interprets buf as a big-endian unsigned integer.
  static auto SetBytes(ptr<Int> z, slice<uint8_t> buf) -> ptr<Int> {
    auto v = detail::cpp_int{};
    if (eo::len(buf) > 0) {
      boost::multiprecision::import_bits(v, buf.data(), buf.data() + eo::len(buf), 8, true);
    }
    z->value_ = std::move(v);
    return z;
  }

  // Sets z to the little-endian words of abs, as a non-negative value.
  static auto SetBits(ptr<Int> z, slice<Word> abs) -> ptr<Int> {
    auto v = detail::cpp_int{};
    for (auto i = eo::len(abs) - 1; i >= 0; --i) {
      v <<= 64;
      v += abs.data()[i]._eo_value;
    }
    z->value_ = std::move(v);
    return z;
  }

  static auto SetString(ptr<Int> z, std::string s, int64_t base) -> std::tuple<ptr<Int>, bool> {
    auto i = std::size_t{0};
    if (s.empty()) {
      return {nullptr, false};
    }
    auto negative = false;
    if (s[0] == '-' || s[0] == '+') {
      negative = s[0] == '-';
      i = 1;
    }
    auto r = detail::scan(s, i, base);
    if (!r.ok || r.next != s.size()) {
      // Go leaves z's value undefined; it is the magnitude scanned so far.
      z->value_ = negative ? -r.magnitude : r.magnitude;
      return {nullptr, false};
    }
    z->value_ = negative ? -r.magnitude : r.magnitude;
    return {z, true};
  }

  // Sets z to x with bit i set to b, which must be 0 or 1, in two's complement for a negative x.
  static auto SetBit(ptr<Int> z, ptr<Int> x, int64_t i, uint64_t b) -> ptr<Int> {
    if (i < 0) {
      panic("negative bit index"s);
    }
    const auto& xv = x->value_;
    auto set_bit = [&](detail::cpp_int t, uint64_t bit) {
      if (bit == 0) {
        boost::multiprecision::bit_unset(t, static_cast<std::size_t>(i));
      } else if (bit == 1) {
        boost::multiprecision::bit_set(t, static_cast<std::size_t>(i));
      } else {
        panic("set bit is not 0 or 1"s);
      }
      return t;
    };
    if (xv < 0) {
      auto t = set_bit(-xv - 1, b ^ 1);
      z->value_ = -(t + 1);
    } else {
      z->value_ = set_bit(xv, b);
    }
    return z;
  }

  static auto Neg(ptr<Int> z, ptr<Int> x) -> ptr<Int> {
    auto v = -x->value_;
    z->value_ = std::move(v);
    return z;
  }

  static auto Abs(ptr<Int> z, ptr<Int> x) -> ptr<Int> {
    auto v = abs(x->value_);
    z->value_ = std::move(v);
    return z;
  }

  static auto Add(ptr<Int> z, ptr<Int> x, ptr<Int> y) -> ptr<Int> {
    auto v = x->value_ + y->value_;
    z->value_ = std::move(v);
    return z;
  }

  static auto Sub(ptr<Int> z, ptr<Int> x, ptr<Int> y) -> ptr<Int> {
    auto v = x->value_ - y->value_;
    z->value_ = std::move(v);
    return z;
  }

  static auto Mul(ptr<Int> z, ptr<Int> x, ptr<Int> y) -> ptr<Int> {
    auto v = x->value_ * y->value_;
    z->value_ = std::move(v);
    return z;
  }

  // Truncated division: the quotient rounds toward zero, and the remainder has the sign of x.
  static auto Quo(ptr<Int> z, ptr<Int> x, ptr<Int> y) -> ptr<Int> {
    auto [q, r] = quo_rem(x->value_, y->value_);
    z->value_ = std::move(q);
    return z;
  }

  static auto Rem(ptr<Int> z, ptr<Int> x, ptr<Int> y) -> ptr<Int> {
    auto [q, r] = quo_rem(x->value_, y->value_);
    z->value_ = std::move(r);
    return z;
  }

  static auto QuoRem(ptr<Int> z, ptr<Int> x, ptr<Int> y, ptr<Int> r) -> std::tuple<ptr<Int>, ptr<Int>> {
    auto [q, rv] = quo_rem(x->value_, y->value_);
    z->value_ = std::move(q);
    r->value_ = std::move(rv);
    return {z, r};
  }

  // Euclidean division: the modulus m has 0 <= m < |y|, and x = q*y + m.
  static auto Div(ptr<Int> z, ptr<Int> x, ptr<Int> y) -> ptr<Int> {
    auto [q, m] = div_mod(x->value_, y->value_);
    z->value_ = std::move(q);
    return z;
  }

  static auto Mod(ptr<Int> z, ptr<Int> x, ptr<Int> y) -> ptr<Int> {
    auto [q, m] = div_mod(x->value_, y->value_);
    z->value_ = std::move(m);
    return z;
  }

  static auto DivMod(ptr<Int> z, ptr<Int> x, ptr<Int> y, ptr<Int> m) -> std::tuple<ptr<Int>, ptr<Int>> {
    auto [q, mv] = div_mod(x->value_, y->value_);
    z->value_ = std::move(q);
    m->value_ = std::move(mv);
    return {z, m};
  }

  // z = x**y mod |m|, as Go's Int.exp: m nil or 0 gives x**y, and 1 when y <= 0. A negative y with
  // a non-zero m uses the inverse of x modulo m, and gives nil, leaving z unchanged, when there is
  // none.
  static auto Exp(ptr<Int> z, ptr<Int> x, ptr<Int> y, ptr<Int> m) -> ptr<Int> {
    using detail::cpp_int;
    auto x_negative = x->value_ < 0;
    auto x_abs = abs(x->value_);
    if (y->value_ < 0) {
      if (m == nullptr || m->value_ == 0) {
        return SetInt64(z, 1);
      }
      auto inverse = ModInverse(New<Int>(), x, m);
      if (inverse == nullptr) {
        return nullptr;
      }
      x_abs = inverse->value_;
    }
    auto y_abs = abs(y->value_);
    auto m_abs = m == nullptr ? cpp_int{} : abs(m->value_);
    auto v = cpp_int{};
    if (m_abs == 1) {
      v = 0;
    } else if (y_abs == 0) {
      v = 1;
    } else if (x_abs == 0) {
      v = 0;
    } else if (m_abs != 0) {
      v = boost::multiprecision::powm(x_abs, y_abs, m_abs);
    } else {
      v = pow_unbounded(x_abs, y_abs);
    }
    auto negative = v != 0 && x_negative && boost::multiprecision::bit_test(y_abs, 0);
    if (negative && m_abs != 0) {
      v = m_abs - v;
      negative = false;
    }
    z->value_ = negative ? -v : v;
    return z;
  }

  // The inverse of g modulo |n|, in [0, |n|), or nil, leaving z unchanged, when g and n are not
  // relatively prime.
  static auto ModInverse(ptr<Int> z, ptr<Int> g, ptr<Int> n) -> ptr<Int> {
    using detail::cpp_int;
    auto nv = abs(n->value_);
    auto gv = g->value_;
    if (gv < 0) {
      gv = div_mod(gv, nv).second;
    }
    if (nv == 0) {
      // Go's GCD(x, nil, g, 0) gives d = |g| and x = 1.
      if (gv != 1) {
        return nullptr;
      }
      z->value_ = 1;
      return z;
    }
    // The extended Euclidean algorithm: g*x + n*y = d.
    auto old_r = gv;
    auto r = nv;
    auto old_x = cpp_int{1};
    auto xv = cpp_int{0};
    while (r != 0) {
      auto q = old_r / r;
      old_r = std::exchange(r, old_r - q * r);
      old_x = std::exchange(xv, old_x - q * xv);
    }
    if (old_r != 1) {
      return nullptr;
    }
    old_x %= nv;
    if (old_x < 0) {
      old_x += nv;
    }
    z->value_ = std::move(old_x);
    return z;
  }

  static auto Lsh(ptr<Int> z, ptr<Int> x, uint64_t n) -> ptr<Int> {
    auto v = x->value_ << n;
    z->value_ = std::move(v);
    return z;
  }

  // An arithmetic shift, which rounds toward negative infinity, as on a two's complement value.
  static auto Rsh(ptr<Int> z, ptr<Int> x, uint64_t n) -> ptr<Int> {
    const auto& xv = x->value_;
    auto v = xv < 0 ? -((-xv - 1) >> n) - 1 : xv >> n;
    z->value_ = std::move(v);
    return z;
  }

  static auto And(ptr<Int> z, ptr<Int> x, ptr<Int> y) -> ptr<Int> {
    const auto& a = x->value_;
    const auto& b = y->value_;
    auto v = detail::cpp_int{};
    if ((a < 0) == (b < 0)) {
      // (-x) & (-y) == -(((x-1) | (y-1)) + 1)
      v = a < 0 ? -(((-a - 1) | (-b - 1)) + 1) : a & b;
    } else {
      // x & (-y) == x &^ (y-1)
      const auto& p = a < 0 ? b : a;
      const auto& n = a < 0 ? a : b;
      v = and_not(p, -n - 1);
    }
    z->value_ = std::move(v);
    return z;
  }

  static auto AndNot(ptr<Int> z, ptr<Int> x, ptr<Int> y) -> ptr<Int> {
    const auto& a = x->value_;
    const auto& b = y->value_;
    auto v = detail::cpp_int{};
    if ((a < 0) == (b < 0)) {
      // (-x) &^ (-y) == (y-1) &^ (x-1)
      v = a < 0 ? and_not(-b - 1, -a - 1) : and_not(a, b);
    } else if (a < 0) {
      // (-x) &^ y == -(((x-1) | y) + 1)
      v = -(((-a - 1) | b) + 1);
    } else {
      // x &^ (-y) == x & (y-1)
      v = a & (-b - 1);
    }
    z->value_ = std::move(v);
    return z;
  }

  static auto Or(ptr<Int> z, ptr<Int> x, ptr<Int> y) -> ptr<Int> {
    const auto& a = x->value_;
    const auto& b = y->value_;
    auto v = detail::cpp_int{};
    if ((a < 0) == (b < 0)) {
      // (-x) | (-y) == -(((x-1) & (y-1)) + 1)
      v = a < 0 ? -(((-a - 1) & (-b - 1)) + 1) : a | b;
    } else {
      // x | (-y) == -(((y-1) &^ x) + 1)
      const auto& p = a < 0 ? b : a;
      const auto& n = a < 0 ? a : b;
      v = -(and_not(-n - 1, p) + 1);
    }
    z->value_ = std::move(v);
    return z;
  }

  static auto Xor(ptr<Int> z, ptr<Int> x, ptr<Int> y) -> ptr<Int> {
    const auto& a = x->value_;
    const auto& b = y->value_;
    auto v = detail::cpp_int{};
    if ((a < 0) == (b < 0)) {
      // (-x) ^ (-y) == (x-1) ^ (y-1)
      v = a < 0 ? (-a - 1) ^ (-b - 1) : a ^ b;
    } else {
      // x ^ (-y) == -((x ^ (y-1)) + 1)
      const auto& p = a < 0 ? b : a;
      const auto& n = a < 0 ? a : b;
      v = -((p ^ (-n - 1)) + 1);
    }
    z->value_ = std::move(v);
    return z;
  }

  static auto Not(ptr<Int> z, ptr<Int> x) -> ptr<Int> {
    auto v = -x->value_ - 1;
    z->value_ = std::move(v);
    return z;
  }

  static auto Cmp(ptr<Int> x, ptr<Int> y) -> int64_t {
    const auto& a = x->value_;
    const auto& b = y->value_;
    return a < b ? -1 : a > b ? 1 : 0;
  }

  static auto CmpAbs(ptr<Int> x, ptr<Int> y) -> int64_t {
    auto a = abs(x->value_);
    auto b = abs(y->value_);
    return a < b ? -1 : a > b ? 1 : 0;
  }

  static auto Sign(ptr<Int> x) -> int64_t {
    return x->value_.sign();
  }

  static auto BitLen(ptr<Int> x) -> int64_t {
    const auto& v = x->value_;
    return v == 0 ? 0 : static_cast<int64_t>(boost::multiprecision::msb(abs(v))) + 1;
  }

  static auto TrailingZeroBits(ptr<Int> x) -> uint64_t {
    const auto& v = x->value_;
    return v == 0 ? 0 : static_cast<uint64_t>(boost::multiprecision::lsb(abs(v)));
  }

  // Bit i of x, in two's complement for a negative x.
  static auto Bit(ptr<Int> x, int64_t i) -> uint64_t {
    const auto& v = x->value_;
    if (i == 0) {
      return v == 0 ? 0 : (boost::multiprecision::bit_test(abs(v), 0) ? 1 : 0);
    }
    if (i < 0) {
      panic("negative bit index"s);
    }
    if (v < 0) {
      return boost::multiprecision::bit_test(-v - 1, static_cast<std::size_t>(i)) ? 0 : 1;
    }
    return boost::multiprecision::bit_test(v, static_cast<std::size_t>(i)) ? 1 : 0;
  }

  static auto Int64(ptr<Int> x) -> int64_t {
    const auto& v = x->value_;
    auto w = static_cast<int64_t>(detail::low64(abs(v)));
    return v < 0 ? static_cast<int64_t>(0 - static_cast<uint64_t>(w)) : w;
  }

  static auto Uint64(ptr<Int> x) -> uint64_t {
    return detail::low64(abs(x->value_));
  }

  static auto IsInt64(ptr<Int> x) -> bool {
    const auto& v = x->value_;
    return v >= INT64_MIN && v <= INT64_MAX;
  }

  static auto IsUint64(ptr<Int> x) -> bool {
    const auto& v = x->value_;
    return v >= 0 && v <= UINT64_MAX;
  }

  // The magnitude as big-endian bytes, without leading zeros: empty, and not nil, for 0.
  static auto Bytes(ptr<Int> x) -> slice<uint8_t> {
    auto magnitude = abs(x->value_);
    auto n = magnitude == 0 ? int64_t{0} : static_cast<int64_t>(boost::multiprecision::msb(magnitude) / 8 + 1);
    auto buf = slice<uint8_t>::allocate(n, n);
    if (n > 0) {
      boost::multiprecision::export_bits(magnitude, buf.data(), 8, true);
    }
    return buf;
  }

  // Sets buf to the magnitude as a big-endian number, zero-padded on the left. A value that does not
  // fit leaves its low bytes in buf before the panic, as Go's nat.bytes does.
  static auto FillBytes(ptr<Int> x, slice<uint8_t> buf) -> slice<uint8_t> {
    auto n = eo::len(buf);
    for (int64_t i = 0; i < n; ++i) {
      buf.data()[i] = 0;
    }
    auto bytes = Bytes(x);
    auto k = eo::len(bytes);
    for (int64_t i = 0; i < k && i < n; ++i) {
      buf.data()[n - 1 - i] = bytes.data()[k - 1 - i];
    }
    if (k > n) {
      panic("math/big: buffer too small to fit value"s);
    }
    return buf;
  }

  // The little-endian words of the magnitude: nil for 0. Go returns the Int's own words; these are a
  // copy, so a write to them does not change x.
  static auto Bits(ptr<Int> x) -> slice<Word> {
    return words(x->value_);
  }

  static auto Text(ptr<Int> x, int64_t base) -> std::string {
    if (x == nullptr) {
      return "<nil>";
    }
    if (base < 2 || base > detail::max_base) {
      panic("invalid base"s);
    }
    const auto& v = x->value_;
    auto s = detail::utoa(abs(v), base);
    return v < 0 ? "-" + s : s;
  }

  static auto String(ptr<Int> x) -> std::string {
    return Text(x, 10);
  }

  // Go's Int.Format: the verbs b, o, O, d, s, v, x, and X, with the flags, width, and precision of
  // the State.
  static void Format(ptr<Int> x, fmt::State s, int32_t ch) {
    auto base = int64_t{0};
    switch (ch) {
    case 'b':
      base = 2;
      break;
    case 'o':
    case 'O':
      base = 8;
      break;
    case 'd':
    case 's':
    case 'v':
      base = 10;
      break;
    case 'x':
    case 'X':
      base = 16;
      break;
    default:
      fmt::Fprintf(s, "%%!%c(big.Int=%s)"s, ch, String(x));
      return;
    }
    if (x == nullptr) {
      fmt::Fprint(s, "<nil>"s);
      return;
    }
    const auto& v = x->value_;
    auto sign = std::string{};
    if (v < 0) {
      sign = "-";
    } else if (s.Flag('+')) {
      sign = "+";
    } else if (s.Flag(' ')) {
      sign = " ";
    }
    auto prefix = std::string{};
    if (s.Flag('#')) {
      switch (ch) {
      case 'b':
        prefix = "0b";
        break;
      case 'o':
        prefix = "0";
        break;
      case 'x':
        prefix = "0x";
        break;
      case 'X':
        prefix = "0X";
        break;
      default:
        break;
      }
    }
    if (ch == 'O') {
      prefix = "0o";
    }
    auto digits = detail::utoa(abs(v), base);
    if (ch == 'X') {
      for (auto& c : digits) {
        if ('a' <= c && c <= 'z') {
          c = static_cast<char>('A' + (c - 'a'));
        }
      }
    }
    auto n = static_cast<int64_t>(digits.size());
    // Spaces left of the number, zeros left of the digits, and spaces right of the number.
    auto left = int64_t{0};
    auto zeros = int64_t{0};
    auto right = int64_t{0};
    auto [precision, precision_set] = s.Precision();
    if (precision_set) {
      if (n < precision) {
        zeros = precision - n;
      } else if (digits == "0" && precision == 0) {
        return;
      }
    }
    auto length = static_cast<int64_t>(sign.size() + prefix.size()) + zeros + n;
    if (auto [width, width_set] = s.Width(); width_set && length < width) {
      auto d = width - length;
      if (s.Flag('-')) {
        right = d;
      } else if (s.Flag('0') && !precision_set) {
        zeros = d;
      } else {
        left = d;
      }
    }
    write_multiple(s, " ", left);
    write_multiple(s, sign, 1);
    write_multiple(s, prefix, 1);
    write_multiple(s, "0", zeros);
    s.Write(bytes_from_string(digits));
    write_multiple(s, " ", right);
  }

  using _eo_methods = eo::methods<eo::method<&Int::String, "String">, eo::method<&Int::Format, "Format">>;

  // A big.Int that is not a pointer has no Format or String method, so Go prints it as its struct,
  // {neg abs}, with the verb applied to the sign and to each word.
  friend void _eo_print(eo::detail::printer& p, const Int& x, int32_t verb, int depth) {
    const auto names = p.flags.plus_v || p.flags.sharp_v;
    if (p.flags.sharp_v) {
      p.buf += "big.Int";
    }
    p.buf += names ? "{neg:" : "{";
    eo::detail::print_arg(p, x.value_ < 0, verb, depth + 1);
    p.buf += p.flags.sharp_v ? ", " : " ";
    if (names) {
      p.buf += "abs:";
    }
    auto abs = words(x.value_);
    if (p.flags.sharp_v) {
      // Go's nat is a defined []Word, nil for a zero Int that no operation has set.
      if (abs.size() == 0) {
        p.buf += "big.nat(nil)";
      } else {
        p.buf += "big.nat{";
        for (int64_t i = 0; i < abs.size(); ++i) {
          if (i > 0) {
            p.buf += ", ";
          }
          eo::detail::print_arg(p, abs.data()[i], verb, depth + 2);
        }
        p.buf += '}';
      }
    } else {
      eo::detail::print_arg(p, abs, verb, depth + 1);
    }
    p.buf += '}';
  }

private:
  detail::cpp_int value_{};

  static void write_multiple(const fmt::State& s, const std::string& text, int64_t count) {
    if (!text.empty()) {
      auto b = bytes_from_string(text);
      for (; count > 0; --count) {
        s.Write(b);
      }
    }
  }

  static auto words(const detail::cpp_int& value) -> slice<Word> {
    auto magnitude = abs(value);
    if (magnitude == 0) {
      return {};
    }
    auto n = static_cast<int64_t>(boost::multiprecision::msb(magnitude) / 64 + 1);
    auto result = slice<Word>::allocate(n, n);
    for (int64_t i = 0; i < n; ++i) {
      result.data()[i] = Word(detail::low64(magnitude));
      magnitude >>= 64;
    }
    return result;
  }

  static auto quo_rem(const detail::cpp_int& x,
    const detail::cpp_int& y) -> std::pair<detail::cpp_int, detail::cpp_int> {
    if (y == 0) {
      detail::panic_division_by_zero();
    }
    detail::cpp_int q;
    detail::cpp_int r;
    boost::multiprecision::divide_qr(x, y, q, r);
    return {std::move(q), std::move(r)};
  }

  static auto div_mod(const detail::cpp_int& x,
    const detail::cpp_int& y) -> std::pair<detail::cpp_int, detail::cpp_int> {
    auto [q, r] = quo_rem(x, y);
    if (r < 0) {
      if (y < 0) {
        q += 1;
        r -= y;
      } else {
        q -= 1;
        r += y;
      }
    }
    return {std::move(q), std::move(r)};
  }

  // x &^ y of non-negative values.
  static auto and_not(const detail::cpp_int& x, const detail::cpp_int& y) -> detail::cpp_int {
    // x &^ y == x ^ (x & y) for non-negative values.
    return x ^ (x & y);
  }

  static auto pow_unbounded(detail::cpp_int base, detail::cpp_int exponent) -> detail::cpp_int {
    auto result = detail::cpp_int{1};
    while (exponent != 0) {
      if (boost::multiprecision::bit_test(exponent, 0)) {
        result *= base;
      }
      exponent >>= 1;
      if (exponent != 0) {
        base *= base;
      }
    }
    return result;
  }
};

// big.NewInt(x): a new Int set to x.
inline auto NewInt(int64_t x) -> ptr<Int> {
  return Int::SetInt64(New<Int>(), x);
}

} // namespace eo::big
