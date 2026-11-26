// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once

#include <eo/core.h>

#include <algorithm>
#include <cstdint>
#include <functional>
#include <string>

// Go's crypto/subtle, ported from Go 1.26's constant_time.go, xor.go, and dit.go, with the bodies of
// crypto/internal/fips140/subtle and crypto/internal/constanttime that they call. Go compiles the
// comparisons of constanttime to code without branches, which C++ does not promise: the results are
// Go's, but the timing is the compiler's.

namespace eo::subtle {

namespace detail {

  // Go's boolToUint8, which the Go compiler replaces with a conditional set.
  inline auto boolToUint8(bool b) noexcept -> uint8_t {
    return static_cast<uint8_t>(b);
  }

  // Go's alias.InexactOverlap: whether x and y share memory at any offset other than their start.
  // Both are n bytes long, n > 0.
  inline auto inexactOverlap(const uint8_t* x, const uint8_t* y, int64_t n) noexcept -> bool {
    if (x == y) {
      return false;
    }
    auto less = std::less<const uint8_t*>{};
    return less(x, y + n) && less(y, x + n);
  }

} // namespace detail

// ConstantTimeCompare returns 1 if x and y have equal contents and 0 otherwise.
inline auto ConstantTimeCompare(slice<uint8_t> x, slice<uint8_t> y) -> int64_t {
  if (x.size() != y.size()) {
    return 0;
  }
  uint8_t v = 0;
  for (int64_t i = 0; i < x.size(); i++) {
    v |= x.data()[i] ^ y.data()[i];
  }
  return static_cast<int64_t>(detail::boolToUint8(v == 0));
}

// ConstantTimeSelect returns x if v == 1 and y if v == 0.
inline auto ConstantTimeSelect(int64_t v, int64_t x, int64_t y) -> int64_t {
  v = static_cast<int64_t>(detail::boolToUint8(v != 0));
  return (~(v - 1) & x) | ((v - 1) & y);
}

// ConstantTimeByteEq returns 1 if x == y and 0 otherwise.
inline auto ConstantTimeByteEq(uint8_t x, uint8_t y) -> int64_t {
  return static_cast<int64_t>(detail::boolToUint8(x == y));
}

// ConstantTimeEq returns 1 if x == y and 0 otherwise.
inline auto ConstantTimeEq(int32_t x, int32_t y) -> int64_t {
  return static_cast<int64_t>(detail::boolToUint8(x == y));
}

// ConstantTimeCopy copies the contents of y into x if v == 1, and leaves x unchanged if v == 0. It
// panics if x and y have different lengths.
inline void ConstantTimeCopy(int64_t v, slice<uint8_t> x, slice<uint8_t> y) {
  if (x.size() != y.size()) {
    panic(std::string("subtle: slices have different lengths"));
  }
  auto xmask = static_cast<uint8_t>(v - 1);
  auto ymask = static_cast<uint8_t>(~(v - 1));
  for (int64_t i = 0; i < x.size(); i++) {
    x.data()[i] = static_cast<uint8_t>((x.data()[i] & xmask) | (y.data()[i] & ymask));
  }
}

// ConstantTimeLessOrEq returns 1 if x <= y and 0 otherwise.
inline auto ConstantTimeLessOrEq(int64_t x, int64_t y) -> int64_t {
  return static_cast<int64_t>(detail::boolToUint8(x <= y));
}

// XORBytes sets dst[i] = x[i] ^ y[i] for all i < n = min(len(x), len(y)), returning n. It panics if
// dst is shorter than n, or if dst overlaps x or y other than exactly.
inline auto XORBytes(slice<uint8_t> dst, slice<uint8_t> x, slice<uint8_t> y) -> int64_t {
  auto n = std::min(x.size(), y.size());
  if (n == 0) {
    return 0;
  }
  if (n > dst.size()) {
    panic(std::string("subtle.XORBytes: dst too short"));
  }
  auto d = dst.data();
  auto a = x.data();
  auto b = y.data();
  if (detail::inexactOverlap(d, a, n) || detail::inexactOverlap(d, b, n)) {
    panic(std::string("subtle.XORBytes: invalid overlap"));
  }
  for (int64_t i = 0; i < n; i++) {
    d[i] = a[i] ^ b[i];
  }
  return n;
}

// WithDataIndependentTiming calls f. Go enables the data-independent timing mode of the CPU around
// the call on arm64, and only calls f elsewhere; Eo only calls f.
inline void WithDataIndependentTiming(fn<void()> f) {
  f();
}

} // namespace eo::subtle
