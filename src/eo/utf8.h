// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once
#include <concepts>
#include <cstddef>
#include <string>
#include <type_traits>
#include <utility>

// Go's integer types, and UTF-8 encoding and decoding, as Go's unicode/utf8 does them. This header
// includes no other Eo header, so every header can use it.

namespace eo::detail {

// A Go integer type: an integer type other than bool and the character types, which the std::cmp_*
// functions reject.
template<typename T>
concept go_integer =
  std::integral<T> && !std::same_as<std::remove_cv_t<T>, bool> && !std::same_as<std::remove_cv_t<T>, char> &&
  !std::same_as<std::remove_cv_t<T>, wchar_t> && !std::same_as<std::remove_cv_t<T>, char8_t> &&
  !std::same_as<std::remove_cv_t<T>, char16_t> && !std::same_as<std::remove_cv_t<T>, char32_t>;

// Appends the UTF-8 encoding of r, or of U+FFFD when r is not a valid code point, as Go encodes it.
template<go_integer I>
void append_rune(std::string& out, I r) {
  auto c = char32_t{0xfffd};
  if (std::cmp_greater_equal(r, 0) && std::cmp_less_equal(r, 0x10ffff) &&
    !(std::cmp_greater_equal(r, 0xd800) && std::cmp_less_equal(r, 0xdfff))) {
    c = static_cast<char32_t>(r);
  }
  if (c < 0x80) {
    out += static_cast<char>(c);
  } else if (c < 0x800) {
    out += static_cast<char>(0xc0 | (c >> 6));
    out += static_cast<char>(0x80 | (c & 0x3f));
  } else if (c < 0x10000) {
    out += static_cast<char>(0xe0 | (c >> 12));
    out += static_cast<char>(0x80 | ((c >> 6) & 0x3f));
    out += static_cast<char>(0x80 | (c & 0x3f));
  } else {
    out += static_cast<char>(0xf0 | (c >> 18));
    out += static_cast<char>(0x80 | ((c >> 12) & 0x3f));
    out += static_cast<char>(0x80 | ((c >> 6) & 0x3f));
    out += static_cast<char>(0x80 | (c & 0x3f));
  }
}

// Decodes the code point that starts at byte i of s, as Go's utf8.DecodeRune does: it returns the
// code point and its length in bytes, or U+FFFD and 1 when the bytes there are not a valid UTF-8
// encoding, which rejects overlong forms, surrogates, and values above U+10FFFF.
inline auto decode_rune(const std::string& s, std::size_t i) -> std::pair<char32_t, std::size_t> {
  constexpr auto invalid = std::pair<char32_t, std::size_t>{0xfffd, 1};
  auto byte = [&](std::size_t k) { return static_cast<char32_t>(static_cast<unsigned char>(s[i + k])); };
  auto n = s.size() - i;
  auto b0 = byte(0);
  if (b0 < 0x80) {
    return {b0, 1};
  }
  if (b0 < 0xc2 || b0 > 0xf4) {
    return invalid;
  }
  // The second byte's range depends on the first, which excludes the invalid encodings.
  auto lo = char32_t{0x80};
  auto hi = char32_t{0xbf};
  auto size = std::size_t{4};
  if (b0 < 0xe0) {
    size = 2;
  } else if (b0 < 0xf0) {
    size = 3;
    lo = b0 == 0xe0 ? 0xa0 : lo;
    hi = b0 == 0xed ? 0x9f : hi;
  } else {
    lo = b0 == 0xf0 ? 0x90 : lo;
    hi = b0 == 0xf4 ? 0x8f : hi;
  }
  if (n < 2 || byte(1) < lo || byte(1) > hi) {
    return invalid;
  }
  if (size == 2) {
    return {(b0 & 0x1f) << 6 | (byte(1) & 0x3f), 2};
  }
  if (n < 3 || byte(2) < 0x80 || byte(2) > 0xbf) {
    return invalid;
  }
  if (size == 3) {
    return {(b0 & 0x0f) << 12 | (byte(1) & 0x3f) << 6 | (byte(2) & 0x3f), 3};
  }
  if (n < 4 || byte(3) < 0x80 || byte(3) > 0xbf) {
    return invalid;
  }
  return {(b0 & 0x07) << 18 | (byte(1) & 0x3f) << 12 | (byte(2) & 0x3f) << 6 | (byte(3) & 0x3f), 4};
}

} // namespace eo::detail
