// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once

#include <eo/unicode/case.h>

#include <cstdint>

// Go's unicode, in part: the case mappings, SimpleFold, and IsSpace, from the tables that
// tools/unicode generates from Go's own (see <eo/unicode/tables.h>).

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

} // namespace eo::unicode
