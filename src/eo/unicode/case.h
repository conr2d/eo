// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once

#include <eo/unicode/tables.h>

#include <algorithm>
#include <cstdint>
#include <iterator>

// The parts of Go's unicode package that strings and bytes use: ToUpper, ToLower, ToTitle,
// SimpleFold, and IsSpace, as Go 1.26's letter.go and graphic.go compute them, on the tables that
// tools/unicode generates.

namespace eo::detail::unicode {

inline constexpr int32_t max_rune = 0x10ffff;
inline constexpr int upper_case = 0;
inline constexpr int lower_case = 1;
inline constexpr int title_case = 2;

// Go's unicode.To with CaseRanges.
inline auto to(int which, int32_t r) -> int32_t {
  if (r < 0) {
    return r;
  }
  auto u = static_cast<uint32_t>(r);
  auto it = std::upper_bound(std::begin(case_ranges), std::end(case_ranges), u, [](uint32_t v, const case_range& cr) {
    return v < cr.lo;
  });
  if (it == std::begin(case_ranges)) {
    return r;
  }
  const auto& cr = *std::prev(it);
  if (u > cr.hi) {
    return r;
  }
  auto delta = cr.delta[which];
  if (delta > max_rune) {
    // UpperLower: upper and lower case alternate, starting with upper at lo. Title is upper.
    auto lo = static_cast<int32_t>(cr.lo);
    return lo + (((r - lo) & ~1) | (which & 1));
  }
  return r + delta;
}

inline auto to_upper(int32_t r) -> int32_t {
  if (r <= 0x7f) {
    return 'a' <= r && r <= 'z' ? r - ('a' - 'A') : r;
  }
  return to(upper_case, r);
}

inline auto to_lower(int32_t r) -> int32_t {
  if (r <= 0x7f) {
    return 'A' <= r && r <= 'Z' ? r + ('a' - 'A') : r;
  }
  return to(lower_case, r);
}

inline auto to_title(int32_t r) -> int32_t {
  if (r <= 0x7f) {
    return 'a' <= r && r <= 'z' ? r - ('a' - 'A') : r;
  }
  return to(title_case, r);
}

// Go's unicode.SimpleFold: the next code point of r's case orbit, or r.
inline auto simple_fold(int32_t r) -> int32_t {
  if (r < 0 || r > max_rune) {
    return r;
  }
  auto u = static_cast<uint32_t>(r);
  auto it =
    std::lower_bound(std::begin(simple_fold_pairs), std::end(simple_fold_pairs), u, [](const fold_pair& p, uint32_t v) {
      return p.from < v;
    });
  if (it != std::end(simple_fold_pairs) && it->from == u) {
    return static_cast<int32_t>(it->to);
  }
  return r;
}

// Go's unicode.IsSpace.
inline auto is_space(int32_t r) -> bool {
  if (r >= 0 && r <= 0xff) {
    switch (r) {
    case '\t':
    case '\n':
    case '\v':
    case '\f':
    case '\r':
    case ' ':
    case 0x85:
    case 0xa0:
      return true;
    default:
      return false;
    }
  }
  auto u = static_cast<uint32_t>(r);
  return std::any_of(std::begin(white_space), std::end(white_space), [u](const range& w) {
    return w.lo <= u && u <= w.hi;
  });
}

} // namespace eo::detail::unicode
