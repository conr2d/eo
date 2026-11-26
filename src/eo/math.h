// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once

#include <bit>
#include <cmath>
#include <cstdint>
#include <tuple>

// Go's math, in part: the constants, and the functions whose results IEEE 754 defines exactly, so
// that they match Go's to the bit. The transcendental functions (Exp, Log, Pow, Sin, and the rest)
// are not covered, since a C++ library's results can differ from Go's in the last place.

namespace eo::math {

// The mathematical constants, rounded to float64 as Go rounds its untyped constants.
inline constexpr double E = 0x1.5bf0a8b145769p+01;
inline constexpr double Pi = 0x1.921fb54442d18p+01;
inline constexpr double Phi = 0x1.9e3779b97f4a8p+00;
inline constexpr double Sqrt2 = 0x1.6a09e667f3bcdp+00;
inline constexpr double SqrtE = 0x1.a61298e1e069cp+00;
inline constexpr double SqrtPi = 0x1.c5bf891b4ef6bp+00;
inline constexpr double SqrtPhi = 0x1.45a3146a88456p+00;
inline constexpr double Ln2 = 0x1.62e42fefa39efp-01;
inline constexpr double Log2E = 0x1.71547652b82fep+00;
inline constexpr double Ln10 = 0x1.26bb1bbb55516p+01;
inline constexpr double Log10E = 0x1.bcb7b1526e50ep-02;

// The limits of the floating-point types, with the default type of Go's untyped constants, float64.
inline constexpr double MaxFloat32 = 0x1.fffffep+127;
inline constexpr double SmallestNonzeroFloat32 = 0x1p-149;
inline constexpr double MaxFloat64 = 0x1.fffffffffffffp+1023;
inline constexpr double SmallestNonzeroFloat64 = 0x1p-1074;

// The limits of the integer types. Go's are untyped constants, so each is declared with its default
// type, int64_t, as a translated constant is, except MaxUint and MaxUint64, which int64_t cannot
// hold, and are uint64_t. Go's int is 64 bits in Eo.
inline constexpr int64_t MaxInt = INT64_MAX;
inline constexpr int64_t MinInt = INT64_MIN;
inline constexpr int64_t MaxInt8 = INT8_MAX;
inline constexpr int64_t MinInt8 = INT8_MIN;
inline constexpr int64_t MaxInt16 = INT16_MAX;
inline constexpr int64_t MinInt16 = INT16_MIN;
inline constexpr int64_t MaxInt32 = INT32_MAX;
inline constexpr int64_t MinInt32 = INT32_MIN;
inline constexpr int64_t MaxInt64 = INT64_MAX;
inline constexpr int64_t MinInt64 = INT64_MIN;
inline constexpr uint64_t MaxUint = UINT64_MAX;
inline constexpr int64_t MaxUint8 = UINT8_MAX;
inline constexpr int64_t MaxUint16 = UINT16_MAX;
inline constexpr int64_t MaxUint32 = UINT32_MAX;
inline constexpr uint64_t MaxUint64 = UINT64_MAX;

inline auto Float64bits(double f) -> uint64_t {
  return std::bit_cast<uint64_t>(f);
}

inline auto Float64frombits(uint64_t b) -> double {
  return std::bit_cast<double>(b);
}

inline auto Float32bits(float f) -> uint32_t {
  return std::bit_cast<uint32_t>(f);
}

inline auto Float32frombits(uint32_t b) -> float {
  return std::bit_cast<float>(b);
}

// Go's Inf: +Inf if sign >= 0, -Inf if sign < 0.
inline auto Inf(int64_t sign) -> double {
  return Float64frombits(sign >= 0 ? 0x7FF0000000000000 : 0xFFF0000000000000);
}

// Go's NaN, with Go's bits.
inline auto NaN() -> double {
  return Float64frombits(0x7FF8000000000001);
}

inline auto IsNaN(double f) -> bool {
  // Only a NaN is not equal to itself.
  return f != f;
}

inline auto Abs(double x) -> double {
  return Float64frombits(Float64bits(x) & ~(uint64_t{1} << 63));
}

// Go's IsInf: whether f is an infinity of the sign's sign, or of either sign when sign is 0.
inline auto IsInf(double f, int64_t sign) -> bool {
  if (sign == 0) {
    f = Abs(f);
  } else if (sign < 0) {
    f = -f;
  }
  return f > MaxFloat64;
}

inline auto Signbit(double x) -> bool {
  return (Float64bits(x) & (uint64_t{1} << 63)) != 0;
}

inline auto Copysign(double f, double sign) -> double {
  constexpr auto signBit = uint64_t{1} << 63;
  return Float64frombits((Float64bits(f) & ~signBit) | (Float64bits(sign) & signBit));
}

// Go's Max: +Inf if either is +Inf, else NaN if either is NaN, and +0 over -0.
inline auto Max(double x, double y) -> double {
  if (IsInf(x, 1) || IsInf(y, 1)) {
    return Inf(1);
  }
  if (IsNaN(x) || IsNaN(y)) {
    return NaN();
  }
  if (x == 0 && x == y) {
    return Signbit(x) ? y : x;
  }
  return x > y ? x : y;
}

// Go's Min: -Inf if either is -Inf, else NaN if either is NaN, and -0 over +0.
inline auto Min(double x, double y) -> double {
  if (IsInf(x, -1) || IsInf(y, -1)) {
    return Inf(-1);
  }
  if (IsNaN(x) || IsNaN(y)) {
    return NaN();
  }
  if (x == 0 && x == y) {
    return Signbit(x) ? x : y;
  }
  return x < y ? x : y;
}

inline auto Dim(double x, double y) -> double {
  auto v = x - y;
  if (v <= 0) {
    return 0;
  }
  // v is positive or NaN.
  return v;
}

inline auto Floor(double x) -> double {
  return std::floor(x);
}

inline auto Ceil(double x) -> double {
  return std::ceil(x);
}

inline auto Trunc(double x) -> double {
  return std::trunc(x);
}

// Go's Round: half away from zero.
inline auto Round(double x) -> double {
  return std::round(x);
}

// Go's RoundToEven: half to even, from the bits, so that it does not depend on the rounding mode.
inline auto RoundToEven(double x) -> double {
  constexpr auto mask = uint64_t{0x7FF};
  constexpr auto shift = 64 - 11 - 1;
  constexpr auto bias = uint64_t{1023};
  constexpr auto signMask = uint64_t{1} << 63;
  constexpr auto fracMask = (uint64_t{1} << shift) - 1;
  constexpr auto halfMinusULP = (uint64_t{1} << (shift - 1)) - 1;
  auto bits = Float64bits(x);
  auto e = (bits >> shift) & mask;
  if (e >= bias) {
    // Round abs(x) >= 1: add 0.499... or 0.5 before truncating, as the truncated number is even or
    // odd. Values with no fraction bits, and Inf and NaN, are unchanged: Go's shifts by more than
    // 63 give 0 there.
    e -= bias;
    if (e < shift) {
      bits += (halfMinusULP + ((bits >> (shift - e)) & 1)) >> e;
      bits &= ~(fracMask >> e);
    }
  } else if (e == bias - 1 && (bits & fracMask) != 0) {
    // Round 0.5 < abs(x) < 1 to ±1.
    bits = (bits & signMask) | (bias << shift);
  } else {
    // Round abs(x) <= 0.5, including denormals, to ±0.
    bits &= signMask;
  }
  return Float64frombits(bits);
}

inline auto Sqrt(double x) -> double {
  return std::sqrt(x);
}

// Go's Mod: the remainder of x/y, with x's sign. The remainder is exact, so std::fmod gives Go's,
// and the cases with no remainder give Go's NaN, whose bits std::fmod's need not have.
inline auto Mod(double x, double y) -> double {
  if (y == 0 || IsInf(x, 0) || IsNaN(x) || IsNaN(y)) {
    return NaN();
  }
  return std::fmod(x, y);
}

// Go's Frexp: a fraction in [0.5, 1) and a power of two. For ±0, ±Inf, and NaN, it returns x and 0.
inline auto Frexp(double f) -> std::tuple<double, int64_t> {
  if (f == 0 || IsInf(f, 0) || IsNaN(f)) {
    return {f, 0};
  }
  auto exp = 0;
  auto frac = std::frexp(f, &exp);
  return {frac, exp};
}

inline auto Ldexp(double frac, int64_t exp) -> double {
  if (exp > 100000) {
    exp = 100000;
  } else if (exp < -100000) {
    exp = -100000;
  }
  return std::ldexp(frac, static_cast<int>(exp));
}

// Go's Modf: the integer and fractional parts, each with f's sign. For ±Inf, the fraction is NaN.
inline auto Modf(double f) -> std::tuple<double, double> {
  auto integer = Trunc(f);
  return {integer, Copysign(f - integer, f)};
}

inline auto Nextafter(double x, double y) -> double {
  if (IsNaN(x) || IsNaN(y)) {
    return NaN();
  }
  if (x == y) {
    return x;
  }
  if (x == 0) {
    return Copysign(Float64frombits(1), y);
  }
  if ((y > x) == (x > 0)) {
    return Float64frombits(Float64bits(x) + 1);
  }
  return Float64frombits(Float64bits(x) - 1);
}

} // namespace eo::math
