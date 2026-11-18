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

// Defined in <eo/slice.h>. len and indexing take both strings and slices.
template<typename T>
class slice;

// Go string literals translate to "..."s.
using namespace std::string_literals;

// The helpers are function objects, not function templates. A translation calls them qualified, as
// eo::at(s, i), and a Go function of the same name keeps its spelling. If both were functions, overload
// resolution would silently pick whichever matched better, in either direction. An unqualified call
// that finds both an object and a function is ambiguous, so the collision fails to compile instead.
namespace detail {

  struct len_fn {
    auto operator()(const std::string& s) const -> int64_t {
      return static_cast<int64_t>(s.size());
    }

    template<typename T>
    auto operator()(const slice<T>& s) const -> int64_t {
      return s.size();
    }
  };

  struct at_fn {
    // s[i] of a string: a copy of the byte.
    template<go_integer I>
    auto operator()(const std::string& s, I i) const -> uint8_t {
      check_index(i, s.size());
      return static_cast<uint8_t>(s[static_cast<std::size_t>(i)]);
    }

    // s[i] of a slice: the element itself, in the shared backing array, which can be assigned.
    template<typename T, go_integer I>
    auto operator()(const slice<T>& s, I i) const -> T& {
      check_index(i, s.size());
      return s.data()[i];
    }
  };

  struct substr_fn {
    // s[low:high]
    template<go_integer L, go_integer H>
    auto operator()(const std::string& s, L low, H high) const -> std::string {
      check_slice(low, high, s.size());
      auto begin = static_cast<std::size_t>(low);
      return s.substr(begin, static_cast<std::size_t>(high) - begin);
    }

    // s[low:]
    template<go_integer L>
    auto operator()(const std::string& s, L low) const -> std::string {
      return (*this)(s, low, s.size());
    }
  };

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

  // string(r) for a value r of any integer type: the UTF-8 encoding of r, or of U+FFFD when r is not a
  // valid code point, as Go converts it.
  struct string_from_rune_fn {
    template<go_integer I>
    auto operator()(I r) const -> std::string {
      auto out = std::string{};
      append_rune(out, r);
      return out;
    }
  };

} // namespace detail

inline constexpr detail::len_fn len{};
inline constexpr detail::at_fn at{};
inline constexpr detail::substr_fn substr{};
inline constexpr detail::string_from_rune_fn string_from_rune{};

} // namespace eo
