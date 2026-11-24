// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once

#include <eo/strconv.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>

// Go's encoding/hex, in part: Encode, Decode, their String forms, and their errors, ported from Go
// 1.26's hex.go.

namespace eo::hex {

inline error ErrLength = errors::New("encoding/hex: odd length hex string");

// Go's hex.InvalidByteError, a defined type over byte.
struct InvalidByteError : eo::defined<InvalidByteError, uint8_t> {
  using defined::defined;
  static constexpr std::string_view _eo_name = "hex.InvalidByteError";

  // "encoding/hex: invalid byte: %#U".
  static auto Error(InvalidByteError e) -> std::string {
    auto r = static_cast<int32_t>(e._eo_value);
    auto s = fmt::format("encoding/hex: invalid byte: U+{:04X}", static_cast<uint32_t>(r));
    if (strconv::IsPrint(r)) {
      s += " '";
      eo::detail::append_rune(s, r);
      s += '\'';
    }
    return s;
  }

  using _eo_methods = eo::methods<eo::method<&InvalidByteError::Error, "Error">>;
};

namespace detail {

  inline constexpr std::string_view hextable = "0123456789abcdef";

  inline auto reverse(uint8_t c) -> uint8_t {
    if ('0' <= c && c <= '9') {
      return static_cast<uint8_t>(c - '0');
    }
    if ('a' <= c && c <= 'f') {
      return static_cast<uint8_t>(c - 'a' + 10);
    }
    if ('A' <= c && c <= 'F') {
      return static_cast<uint8_t>(c - 'A' + 10);
    }
    return 0xff;
  }

} // namespace detail

inline auto EncodedLen(int64_t n) -> int64_t {
  return n * 2;
}

inline auto DecodedLen(int64_t x) -> int64_t {
  return x / 2;
}

// Writes the hex of src to dst, which panics with Go's index error when dst is too short.
inline auto Encode(slice<uint8_t> dst, slice<uint8_t> src) -> int64_t {
  auto j = int64_t{0};
  for (int64_t i = 0; i < eo::len(src); ++i) {
    auto v = src.data()[i];
    eo::at(dst, j) = static_cast<uint8_t>(detail::hextable[v >> 4]);
    eo::at(dst, j + 1) = static_cast<uint8_t>(detail::hextable[v & 0x0f]);
    j += 2;
  }
  return eo::len(src) * 2;
}

inline auto Decode(slice<uint8_t> dst, slice<uint8_t> src) -> std::tuple<int64_t, error> {
  auto i = int64_t{0};
  auto j = int64_t{1};
  auto n = eo::len(src);
  for (; j < n; j += 2) {
    auto p = src.data()[j - 1];
    auto q = src.data()[j];
    auto a = detail::reverse(p);
    auto b = detail::reverse(q);
    if (a > 0x0f) {
      return {i, InvalidByteError(p)};
    }
    if (b > 0x0f) {
      return {i, InvalidByteError(q)};
    }
    eo::at(dst, i) = static_cast<uint8_t>((a << 4) | b);
    i++;
  }
  if (n % 2 == 1) {
    if (detail::reverse(src.data()[j - 1]) > 0x0f) {
      return {i, InvalidByteError(src.data()[j - 1])};
    }
    return {i, ErrLength};
  }
  return {i, nullptr};
}

inline auto EncodeToString(slice<uint8_t> src) -> std::string {
  auto dst = eo::make_slice<uint8_t>(EncodedLen(eo::len(src)));
  Encode(dst, src);
  return eo::string_from_bytes(dst);
}

inline auto DecodeString(std::string s) -> std::tuple<slice<uint8_t>, error> {
  auto dst = eo::make_slice<uint8_t>(DecodedLen(static_cast<int64_t>(s.size())));
  auto [n, err] = Decode(dst, eo::bytes_from_string(s));
  return {eo::subslice(dst, 0, n), err};
}

} // namespace eo::hex
