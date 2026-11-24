// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once

#include <eo/core.h>

#include <bit>
#include <cstdint>
#include <tuple>

// Go's math/bits, ported from Go 1.26's bits.go. Go's uint is uint64_t, and its int results are
// int64_t.

namespace eo::bits {

inline constexpr int64_t UintSize = 64;

inline auto LeadingZeros(uint64_t x) -> int64_t {
  return std::countl_zero(x);
}
inline auto LeadingZeros8(uint8_t x) -> int64_t {
  return std::countl_zero(x);
}
inline auto LeadingZeros16(uint16_t x) -> int64_t {
  return std::countl_zero(x);
}
inline auto LeadingZeros32(uint32_t x) -> int64_t {
  return std::countl_zero(x);
}
inline auto LeadingZeros64(uint64_t x) -> int64_t {
  return std::countl_zero(x);
}

inline auto TrailingZeros(uint64_t x) -> int64_t {
  return std::countr_zero(x);
}
inline auto TrailingZeros8(uint8_t x) -> int64_t {
  return std::countr_zero(x);
}
inline auto TrailingZeros16(uint16_t x) -> int64_t {
  return std::countr_zero(x);
}
inline auto TrailingZeros32(uint32_t x) -> int64_t {
  return std::countr_zero(x);
}
inline auto TrailingZeros64(uint64_t x) -> int64_t {
  return std::countr_zero(x);
}

inline auto OnesCount(uint64_t x) -> int64_t {
  return std::popcount(x);
}
inline auto OnesCount8(uint8_t x) -> int64_t {
  return std::popcount(x);
}
inline auto OnesCount16(uint16_t x) -> int64_t {
  return std::popcount(x);
}
inline auto OnesCount32(uint32_t x) -> int64_t {
  return std::popcount(x);
}
inline auto OnesCount64(uint64_t x) -> int64_t {
  return std::popcount(x);
}

// The number of bits needed to represent x, 0 for 0.
inline auto Len(uint64_t x) -> int64_t {
  return std::bit_width(x);
}
inline auto Len8(uint8_t x) -> int64_t {
  return std::bit_width(x);
}
inline auto Len16(uint16_t x) -> int64_t {
  return std::bit_width(x);
}
inline auto Len32(uint32_t x) -> int64_t {
  return std::bit_width(x);
}
inline auto Len64(uint64_t x) -> int64_t {
  return std::bit_width(x);
}

// A negative k rotates right, as in Go.
inline auto RotateLeft(uint64_t x, int64_t k) -> uint64_t {
  return std::rotl(x, static_cast<int>(k % 64));
}
inline auto RotateLeft8(uint8_t x, int64_t k) -> uint8_t {
  return std::rotl(x, static_cast<int>(k % 8));
}
inline auto RotateLeft16(uint16_t x, int64_t k) -> uint16_t {
  return std::rotl(x, static_cast<int>(k % 16));
}
inline auto RotateLeft32(uint32_t x, int64_t k) -> uint32_t {
  return std::rotl(x, static_cast<int>(k % 32));
}
inline auto RotateLeft64(uint64_t x, int64_t k) -> uint64_t {
  return std::rotl(x, static_cast<int>(k % 64));
}

namespace detail {

  template<typename U>
  auto reverse(U x) -> U {
    auto r = U{0};
    for (int i = 0; i < static_cast<int>(sizeof(U) * 8); ++i) {
      r = static_cast<U>((r << 1) | ((x >> i) & 1));
    }
    return r;
  }

  template<typename U>
  auto reverse_bytes(U x) -> U {
    auto r = U{0};
    for (int i = 0; i < static_cast<int>(sizeof(U)); ++i) {
      r = static_cast<U>((r << 8) | ((x >> (8 * i)) & 0xff));
    }
    return r;
  }

} // namespace detail

inline auto Reverse(uint64_t x) -> uint64_t {
  return detail::reverse(x);
}
inline auto Reverse8(uint8_t x) -> uint8_t {
  return detail::reverse(x);
}
inline auto Reverse16(uint16_t x) -> uint16_t {
  return detail::reverse(x);
}
inline auto Reverse32(uint32_t x) -> uint32_t {
  return detail::reverse(x);
}
inline auto Reverse64(uint64_t x) -> uint64_t {
  return detail::reverse(x);
}

inline auto ReverseBytes(uint64_t x) -> uint64_t {
  return detail::reverse_bytes(x);
}
inline auto ReverseBytes16(uint16_t x) -> uint16_t {
  return detail::reverse_bytes(x);
}
inline auto ReverseBytes32(uint32_t x) -> uint32_t {
  return detail::reverse_bytes(x);
}
inline auto ReverseBytes64(uint64_t x) -> uint64_t {
  return detail::reverse_bytes(x);
}

// x + y + carry, and the carry out. carry must be 0 or 1.
inline auto Add64(uint64_t x, uint64_t y, uint64_t carry) -> std::tuple<uint64_t, uint64_t> {
  auto sum = x + y + carry;
  auto carryOut = ((x & y) | ((x | y) & ~sum)) >> 63;
  return {sum, carryOut};
}
inline auto Add(uint64_t x, uint64_t y, uint64_t carry) -> std::tuple<uint64_t, uint64_t> {
  return Add64(x, y, carry);
}
inline auto Add32(uint32_t x, uint32_t y, uint32_t carry) -> std::tuple<uint32_t, uint32_t> {
  auto sum64 = uint64_t{x} + uint64_t{y} + uint64_t{carry};
  return {static_cast<uint32_t>(sum64), static_cast<uint32_t>(sum64 >> 32)};
}

// x - y - borrow, and the borrow out. borrow must be 0 or 1.
inline auto Sub64(uint64_t x, uint64_t y, uint64_t borrow) -> std::tuple<uint64_t, uint64_t> {
  auto diff = x - y - borrow;
  auto borrowOut = ((~x & y) | (~(x ^ y) & diff)) >> 63;
  return {diff, borrowOut};
}
inline auto Sub(uint64_t x, uint64_t y, uint64_t borrow) -> std::tuple<uint64_t, uint64_t> {
  return Sub64(x, y, borrow);
}
inline auto Sub32(uint32_t x, uint32_t y, uint32_t borrow) -> std::tuple<uint32_t, uint32_t> {
  auto diff = x - y - borrow;
  auto borrowOut = ((~x & y) | (~(x ^ y) & diff)) >> 31;
  return {diff, borrowOut};
}

// The 128-bit product of x and y, as its high and low halves.
inline auto Mul64(uint64_t x, uint64_t y) -> std::tuple<uint64_t, uint64_t> {
  auto p = static_cast<unsigned __int128>(x) * y;
  return {static_cast<uint64_t>(p >> 64), static_cast<uint64_t>(p)};
}
inline auto Mul(uint64_t x, uint64_t y) -> std::tuple<uint64_t, uint64_t> {
  return Mul64(x, y);
}
inline auto Mul32(uint32_t x, uint32_t y) -> std::tuple<uint32_t, uint32_t> {
  auto p = uint64_t{x} * y;
  return {static_cast<uint32_t>(p >> 32), static_cast<uint32_t>(p)};
}

// (hi, lo) / y and its remainder. Go panics with its integer division error when y is 0, and with
// its overflow error when y <= hi, whose quotient does not fit.
inline auto Div64(uint64_t hi, uint64_t lo, uint64_t y) -> std::tuple<uint64_t, uint64_t> {
  if (y == 0) {
    eo::detail::panic_runtime_error("integer divide by zero");
  }
  if (y <= hi) {
    eo::detail::panic_runtime_error("integer overflow");
  }
  auto n = (static_cast<unsigned __int128>(hi) << 64) | lo;
  return {static_cast<uint64_t>(n / y), static_cast<uint64_t>(n % y)};
}
inline auto Div(uint64_t hi, uint64_t lo, uint64_t y) -> std::tuple<uint64_t, uint64_t> {
  return Div64(hi, lo, y);
}
inline auto Div32(uint32_t hi, uint32_t lo, uint32_t y) -> std::tuple<uint32_t, uint32_t> {
  if (y == 0) {
    eo::detail::panic_runtime_error("integer divide by zero");
  }
  if (y <= hi) {
    eo::detail::panic_runtime_error("integer overflow");
  }
  auto n = (uint64_t{hi} << 32) | lo;
  return {static_cast<uint32_t>(n / y), static_cast<uint32_t>(n % y)};
}

// The remainder of (hi, lo) / y, which does not panic when the quotient overflows.
inline auto Rem64(uint64_t hi, uint64_t lo, uint64_t y) -> uint64_t {
  if (y == 0) {
    eo::detail::panic_runtime_error("integer divide by zero");
  }
  auto n = (static_cast<unsigned __int128>(hi % y) << 64) | lo;
  return static_cast<uint64_t>(n % y);
}

inline auto Rem(uint64_t hi, uint64_t lo, uint64_t y) -> uint64_t {
  return Rem64(hi, lo, y);
}

inline auto Rem32(uint32_t hi, uint32_t lo, uint32_t y) -> uint32_t {
  if (y == 0) {
    eo::detail::panic_runtime_error("integer divide by zero");
  }
  return static_cast<uint32_t>(((uint64_t{hi} << 32) | lo) % y);
}

} // namespace eo::bits
