// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once
#include <eo/bounds.h>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

// Go strings are std::string. A Go string is immutable, so copying a std::string is observably the
// same as sharing Go's bytes. These functions translate the operations whose C++ form would differ:
// a byte count of the translated int type, checked indexing that yields a byte, checked slicing,
// and the conversion of an integer to the UTF-8 encoding of a rune.

// The ordinary literal encoding and the source encoding must both be UTF-8, so that "\u00e9" and a
// literal é are Go's bytes.
static_assert(sizeof("\u00e9") == 3 && static_cast<unsigned char>("\u00e9"[0]) == 0xc3 &&
    static_cast<unsigned char>("\u00e9"[1]) == 0xa9,
  "Eo requires UTF-8 as the ordinary literal encoding");
static_assert(sizeof("é") == 3 && static_cast<unsigned char>("é"[0]) == 0xc3 &&
    static_cast<unsigned char>("é"[1]) == 0xa9,
  "Eo requires UTF-8 source files");

namespace eo {

// Go string literals translate to "..."s.
using namespace std::string_literals;

inline auto len(const std::string& s) -> int64_t {
  return static_cast<int64_t>(s.size());
}

// s[i]
template<detail::go_integer I>
auto at(const std::string& s, I i) -> uint8_t {
  detail::check_index(i, s.size());
  return static_cast<uint8_t>(s[static_cast<std::size_t>(i)]);
}

// s[low:high]
template<detail::go_integer L, detail::go_integer H>
auto substr(const std::string& s, L low, H high) -> std::string {
  detail::check_slice(low, high, s.size());
  auto begin = static_cast<std::size_t>(low);
  return s.substr(begin, static_cast<std::size_t>(high) - begin);
}

// s[low:]
template<detail::go_integer L>
auto substr(const std::string& s, L low) -> std::string {
  return substr(s, low, s.size());
}

// string(r) for a value r of any integer type: the UTF-8 encoding of r, or of U+FFFD when r is not a
// valid code point, as Go converts it.
template<detail::go_integer I>
auto string_from_rune(I r) -> std::string {
  auto c = char32_t{0xfffd};
  if (std::cmp_greater_equal(r, 0) && std::cmp_less_equal(r, 0x10ffff) &&
    !(std::cmp_greater_equal(r, 0xd800) && std::cmp_less_equal(r, 0xdfff))) {
    c = static_cast<char32_t>(r);
  }
  auto out = std::string{};
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
  return out;
}

} // namespace eo
