// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once

#include <eo/unicode/case.h>

#include <cstdint>
#include <span>

// Go's unicode, in part: the case mappings, SimpleFold, and the predicates of the categories, from
// the tables that tools/unicode generates from Go's own (see <eo/unicode/tables.h>).

namespace eo::unicode {

inline constexpr int32_t MaxRune = 0x10ffff;
inline constexpr int32_t ReplacementChar = 0xfffd;
inline constexpr int32_t MaxASCII = 0x7f;
inline constexpr int32_t MaxLatin1 = 0xff;

inline auto ToUpper(int32_t r) -> int32_t {
  return eo::detail::unicode::to_upper(r);
}

inline auto ToLower(int32_t r) -> int32_t {
  return eo::detail::unicode::to_lower(r);
}

inline auto ToTitle(int32_t r) -> int32_t {
  return eo::detail::unicode::to_title(r);
}

inline auto SimpleFold(int32_t r) -> int32_t {
  return eo::detail::unicode::simple_fold(r);
}

inline auto IsSpace(int32_t r) -> bool {
  return eo::detail::unicode::is_space(r);
}

namespace detail {

  // Go's Is over a RangeTable's R16 and R32 entries, which tables.h holds as one list: a linear
  // search for a short table or a Latin-1 code point, else a binary search.
  inline auto is(std::span<const eo::detail::unicode::stride_range> ranges, int32_t r) -> bool {
    // Compared as uint32, as Go does, so that a negative rune is in no range.
    auto u = static_cast<uint32_t>(r);
    auto in = [u](const eo::detail::unicode::stride_range& range) {
      return range.stride == 1 || (u - range.lo) % range.stride == 0;
    };
    if (ranges.size() <= 18 || u <= static_cast<uint32_t>(MaxLatin1)) {
      for (const auto& range : ranges) {
        if (u < range.lo) {
          return false;
        }
        if (u <= range.hi) {
          return in(range);
        }
      }
      return false;
    }
    auto lo = std::size_t{0};
    auto hi = ranges.size();
    while (lo < hi) {
      auto m = (lo + hi) / 2;
      const auto& range = ranges[m];
      if (range.lo <= u && u <= range.hi) {
        return in(range);
      }
      if (u < range.lo) {
        hi = m;
      } else {
        lo = m + 1;
      }
    }
    return false;
  }

  // A Latin-1 code point's properties, which Go's predicates look up instead of the tables.
  inline auto latin1(int32_t r, uint16_t property) -> bool {
    return (eo::detail::unicode::latin1_properties[static_cast<uint8_t>(r)] & property) != 0;
  }

} // namespace detail

inline auto IsControl(int32_t r) -> bool {
  if (static_cast<uint32_t>(r) <= MaxLatin1) {
    return detail::latin1(r, eo::detail::unicode::p_control);
  }
  // Every control character is in Latin-1.
  return false;
}

inline auto IsLetter(int32_t r) -> bool {
  if (static_cast<uint32_t>(r) <= MaxLatin1) {
    return detail::latin1(r, eo::detail::unicode::p_letter);
  }
  return detail::is(eo::detail::unicode::letter, r);
}

inline auto IsUpper(int32_t r) -> bool {
  if (static_cast<uint32_t>(r) <= MaxLatin1) {
    return detail::latin1(r, eo::detail::unicode::p_upper);
  }
  return detail::is(eo::detail::unicode::upper, r);
}

inline auto IsLower(int32_t r) -> bool {
  if (static_cast<uint32_t>(r) <= MaxLatin1) {
    return detail::latin1(r, eo::detail::unicode::p_lower);
  }
  return detail::is(eo::detail::unicode::lower, r);
}

inline auto IsTitle(int32_t r) -> bool {
  if (r <= MaxLatin1) {
    return false;
  }
  return detail::is(eo::detail::unicode::title, r);
}

inline auto IsMark(int32_t r) -> bool {
  // No Latin-1 code point is a mark.
  return detail::is(eo::detail::unicode::mark, r);
}

inline auto IsNumber(int32_t r) -> bool {
  if (static_cast<uint32_t>(r) <= MaxLatin1) {
    return detail::latin1(r, eo::detail::unicode::p_number);
  }
  return detail::is(eo::detail::unicode::number, r);
}

inline auto IsDigit(int32_t r) -> bool {
  if (r <= MaxLatin1) {
    return '0' <= r && r <= '9';
  }
  return detail::is(eo::detail::unicode::digit, r);
}

inline auto IsPunct(int32_t r) -> bool {
  if (static_cast<uint32_t>(r) <= MaxLatin1) {
    return detail::latin1(r, eo::detail::unicode::p_punct);
  }
  return detail::is(eo::detail::unicode::punct, r);
}

inline auto IsSymbol(int32_t r) -> bool {
  if (static_cast<uint32_t>(r) <= MaxLatin1) {
    return detail::latin1(r, eo::detail::unicode::p_symbol);
  }
  return detail::is(eo::detail::unicode::symbol, r);
}

// Go's IsGraphic: a letter, mark, number, punctuation, symbol, or space separator (Zs).
inline auto IsGraphic(int32_t r) -> bool {
  if (static_cast<uint32_t>(r) <= MaxLatin1) {
    return detail::latin1(r, eo::detail::unicode::p_graphic);
  }
  namespace t = eo::detail::unicode;
  return detail::is(t::letter, r) || detail::is(t::mark, r) || detail::is(t::number, r) || detail::is(t::punct, r) ||
    detail::is(t::symbol, r) || detail::is(t::zs, r);
}

// Go's IsPrint: as IsGraphic, but the only space it includes is U+0020.
inline auto IsPrint(int32_t r) -> bool {
  if (static_cast<uint32_t>(r) <= MaxLatin1) {
    return detail::latin1(r, eo::detail::unicode::p_print);
  }
  namespace t = eo::detail::unicode;
  return detail::is(t::letter, r) || detail::is(t::mark, r) || detail::is(t::number, r) || detail::is(t::punct, r) ||
    detail::is(t::symbol, r);
}

} // namespace eo::unicode
