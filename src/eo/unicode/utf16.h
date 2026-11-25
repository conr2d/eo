// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once

#include <eo/slice.h>

#include <cstdint>
#include <tuple>

// Go's unicode/utf16, ported from Go 1.26's utf16.go. A rune is an int32_t and an int is an int64_t,
// as elsewhere in Eo.

namespace eo::utf16 {

namespace detail {

  inline constexpr int32_t replacementChar = 0xFFFD;
  inline constexpr int32_t maxRune = 0x10FFFF;

  inline constexpr int32_t surr1 = 0xd800;
  inline constexpr int32_t surr2 = 0xdc00;
  inline constexpr int32_t surr3 = 0xe000;

  inline constexpr int32_t surrSelf = 0x10000;

} // namespace detail

inline auto IsSurrogate(int32_t r) -> bool {
  return detail::surr1 <= r && r < detail::surr3;
}

inline auto DecodeRune(int32_t r1, int32_t r2) -> int32_t {
  using namespace detail;
  if (surr1 <= r1 && r1 < surr2 && surr2 <= r2 && r2 < surr3) {
    return ((r1 - surr1) << 10 | (r2 - surr2)) + surrSelf;
  }
  return replacementChar;
}

inline auto EncodeRune(int32_t r) -> std::tuple<int32_t, int32_t> {
  using namespace detail;
  if (r < surrSelf || r > maxRune) {
    return {replacementChar, replacementChar};
  }
  r -= surrSelf;
  return {surr1 + ((r >> 10) & 0x3ff), surr2 + (r & 0x3ff)};
}

inline auto RuneLen(int32_t r) -> int64_t {
  using namespace detail;
  if ((0 <= r && r < surr1) || (surr3 <= r && r < surrSelf)) {
    return 1;
  }
  if (surrSelf <= r && r <= maxRune) {
    return 2;
  }
  return -1;
}

inline auto Encode(slice<int32_t> s) -> slice<uint16_t> {
  using namespace detail;
  auto n = eo::len(s);
  for (auto v : s) {
    if (v >= surrSelf) {
      n++;
    }
  }

  auto a = eo::make_slice<uint16_t>(n);
  n = 0;
  for (auto v : s) {
    switch (RuneLen(v)) {
    case 1:
      eo::at(a, n) = static_cast<uint16_t>(v);
      n++;
      break;
    case 2: {
      auto [r1, r2] = EncodeRune(v);
      eo::at(a, n) = static_cast<uint16_t>(r1);
      eo::at(a, n + 1) = static_cast<uint16_t>(r2);
      n += 2;
      break;
    }
    default:
      eo::at(a, n) = static_cast<uint16_t>(replacementChar);
      n++;
    }
  }
  return eo::subslice(a, 0, n);
}

inline auto AppendRune(slice<uint16_t> a, int32_t r) -> slice<uint16_t> {
  using namespace detail;
  if ((0 <= r && r < surr1) || (surr3 <= r && r < surrSelf)) {
    return eo::append(a, {static_cast<uint16_t>(r)});
  }
  if (surrSelf <= r && r <= maxRune) {
    auto [r1, r2] = EncodeRune(r);
    return eo::append(a, {static_cast<uint16_t>(r1), static_cast<uint16_t>(r2)});
  }
  return eo::append(a, {static_cast<uint16_t>(replacementChar)});
}

namespace detail {

  inline auto decode(slice<uint16_t> s, slice<int32_t> buf) -> slice<int32_t> {
    for (int64_t i = 0; i < eo::len(s); i++) {
      int32_t ar{};
      auto r = static_cast<int32_t>(eo::at(s, i));
      if (r < surr1 || surr3 <= r) {
        ar = r;
      } else if (surr1 <= r && r < surr2 && i + 1 < eo::len(s) && surr2 <= eo::at(s, i + 1) &&
        eo::at(s, i + 1) < surr3) {
        ar = DecodeRune(r, static_cast<int32_t>(eo::at(s, i + 1)));
        i++;
      } else {
        ar = replacementChar;
      }
      buf = eo::append(buf, {ar});
    }
    return buf;
  }

} // namespace detail

inline auto Decode(slice<uint16_t> s) -> slice<int32_t> {
  auto buf = eo::make_slice<int32_t>(0, 64);
  return detail::decode(s, buf);
}

} // namespace eo::utf16
