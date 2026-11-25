// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once

#include <eo/slice.h>
#include <eo/string.h>
#include <eo/utf8.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <tuple>

// Go's unicode/utf8, ported from Go 1.26's utf8.go. A rune is an int32_t, an int is an int64_t, and
// a string is a std::string of bytes, as elsewhere in Eo. Valid and ValidString check one byte at a
// time; Go's word-at-a-time scan over ASCII is a speed-up that gives the same results.

namespace eo::utf8 {

inline constexpr int32_t RuneError = 0xFFFD;
inline constexpr int32_t RuneSelf = 0x80;
inline constexpr int32_t MaxRune = 0x10FFFF;
inline constexpr int64_t UTFMax = 4;

namespace detail {

  inline constexpr int32_t surrogateMin = 0xD800;
  inline constexpr int32_t surrogateMax = 0xDFFF;

  inline constexpr uint8_t tx = 0b10000000;
  inline constexpr uint8_t t2 = 0b11000000;
  inline constexpr uint8_t t3 = 0b11100000;
  inline constexpr uint8_t t4 = 0b11110000;

  inline constexpr uint8_t maskx = 0b00111111;
  inline constexpr uint8_t mask2 = 0b00011111;
  inline constexpr uint8_t mask3 = 0b00001111;
  inline constexpr uint8_t mask4 = 0b00000111;

  inline constexpr int32_t rune1Max = (1 << 7) - 1;
  inline constexpr int32_t rune2Max = (1 << 11) - 1;
  inline constexpr int32_t rune3Max = (1 << 16) - 1;

  inline constexpr uint8_t locb = 0b10000000;
  inline constexpr uint8_t hicb = 0b10111111;

  inline constexpr uint8_t xx = 0xF1;
  inline constexpr uint8_t as = 0xF0;
  inline constexpr uint8_t s1 = 0x02;
  inline constexpr uint8_t s2 = 0x13;
  inline constexpr uint8_t s3 = 0x03;
  inline constexpr uint8_t s4 = 0x23;
  inline constexpr uint8_t s5 = 0x34;
  inline constexpr uint8_t s6 = 0x04;
  inline constexpr uint8_t s7 = 0x44;

  inline constexpr uint8_t runeErrorByte0 = t3 | (RuneError >> 12);
  inline constexpr uint8_t runeErrorByte1 = tx | ((RuneError >> 6) & maskx);
  inline constexpr uint8_t runeErrorByte2 = tx | (RuneError & maskx);

  // clang-format off
  inline constexpr std::array<uint8_t, 256> first = {
    as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, // 0x00-0x0F
    as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, // 0x10-0x1F
    as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, // 0x20-0x2F
    as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, // 0x30-0x3F
    as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, // 0x40-0x4F
    as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, // 0x50-0x5F
    as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, // 0x60-0x6F
    as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, // 0x70-0x7F
    xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, // 0x80-0x8F
    xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, // 0x90-0x9F
    xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, // 0xA0-0xAF
    xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, // 0xB0-0xBF
    xx, xx, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, // 0xC0-0xCF
    s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, // 0xD0-0xDF
    s2, s3, s3, s3, s3, s3, s3, s3, s3, s3, s3, s3, s3, s4, s3, s3, // 0xE0-0xEF
    s5, s6, s6, s6, s7, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, // 0xF0-0xFF
  };
  // clang-format on

  struct acceptRange {
    uint8_t lo;
    uint8_t hi;
  };

  inline constexpr std::array<acceptRange, 16> acceptRanges = {{
    {locb, hicb},
    {0xA0, hicb},
    {locb, 0x9F},
    {0x90, hicb},
    {locb, 0x8F},
  }};

  // The bytes of a []byte or a string, read by the same code: Go writes each function twice.
  template<typename S>
  auto byte_at(const S& s, int64_t i) -> uint8_t {
    return eo::at(s, i);
  }

  template<typename S>
  auto full_rune(const S& p) -> bool {
    auto n = eo::len(p);
    if (n == 0) {
      return false;
    }
    auto x = first[byte_at(p, 0)];
    if (n >= static_cast<int64_t>(x & 7)) {
      return true;
    }
    auto accept = acceptRanges[x >> 4];
    if (n > 1 && (byte_at(p, 1) < accept.lo || accept.hi < byte_at(p, 1))) {
      return true;
    } else if (n > 2 && (byte_at(p, 2) < locb || hicb < byte_at(p, 2))) {
      return true;
    }
    return false;
  }

  template<typename S>
  auto decode_rune_slow(const S& p) -> std::tuple<int32_t, int64_t> {
    auto n = eo::len(p);
    if (n < 1) {
      return {RuneError, 0};
    }
    auto p0 = byte_at(p, 0);
    auto x = first[p0];
    if (x >= as) {
      // Create 0x0000 or 0xFFFF.
      auto mask = static_cast<int32_t>(static_cast<uint32_t>(x) << 31) >> 31;
      return {(static_cast<int32_t>(p0) & ~mask) | (RuneError & mask), 1};
    }
    auto sz = static_cast<int64_t>(x & 7);
    auto accept = acceptRanges[x >> 4];
    if (n < sz) {
      return {RuneError, 1};
    }
    auto b1 = byte_at(p, 1);
    if (b1 < accept.lo || accept.hi < b1) {
      return {RuneError, 1};
    }
    if (sz <= 2) {
      return {(static_cast<int32_t>(p0 & mask2) << 6) | static_cast<int32_t>(b1 & maskx), 2};
    }
    auto b2 = byte_at(p, 2);
    if (b2 < locb || hicb < b2) {
      return {RuneError, 1};
    }
    if (sz <= 3) {
      return {(static_cast<int32_t>(p0 & mask3) << 12) | (static_cast<int32_t>(b1 & maskx) << 6) |
          static_cast<int32_t>(b2 & maskx),
        3};
    }
    auto b3 = byte_at(p, 3);
    if (b3 < locb || hicb < b3) {
      return {RuneError, 1};
    }
    return {(static_cast<int32_t>(p0 & mask4) << 18) | (static_cast<int32_t>(b1 & maskx) << 12) |
        (static_cast<int32_t>(b2 & maskx) << 6) | static_cast<int32_t>(b3 & maskx),
      4};
  }

  template<typename S>
  auto valid(const S& p) -> bool {
    auto i = int64_t{0};
    auto n = eo::len(p);
    while (i < n) {
      auto p0 = byte_at(p, i);
      if (p0 < RuneSelf) {
        i++;
        continue;
      }
      auto x = first[p0];
      auto size = static_cast<int64_t>(x & 7);
      auto accept = acceptRanges[x >> 4];
      auto rest = n - i;
      switch (size) {
      case 2:
        if (rest < 2 || byte_at(p, i + 1) < accept.lo || accept.hi < byte_at(p, i + 1)) {
          return false;
        }
        i += 2;
        break;
      case 3:
        if (rest < 3 || byte_at(p, i + 1) < accept.lo || accept.hi < byte_at(p, i + 1) || byte_at(p, i + 2) < locb ||
          hicb < byte_at(p, i + 2)) {
          return false;
        }
        i += 3;
        break;
      case 4:
        if (rest < 4 || byte_at(p, i + 1) < accept.lo || accept.hi < byte_at(p, i + 1) || byte_at(p, i + 2) < locb ||
          hicb < byte_at(p, i + 2) || byte_at(p, i + 3) < locb || hicb < byte_at(p, i + 3)) {
          return false;
        }
        i += 4;
        break;
      default:
        return false;
      }
    }
    return true;
  }

} // namespace detail

inline auto RuneStart(uint8_t b) -> bool {
  return (b & 0xC0) != 0x80;
}

inline auto FullRune(slice<uint8_t> p) -> bool {
  return detail::full_rune(p);
}

inline auto FullRuneInString(std::string s) -> bool {
  return detail::full_rune(s);
}

inline auto DecodeRune(slice<uint8_t> p) -> std::tuple<int32_t, int64_t> {
  if (eo::len(p) > 0 && eo::at(p, 0) < RuneSelf) {
    return {static_cast<int32_t>(eo::at(p, 0)), 1};
  }
  return detail::decode_rune_slow(p);
}

inline auto DecodeRuneInString(std::string s) -> std::tuple<int32_t, int64_t> {
  if (!s.empty() && eo::at(s, 0) < RuneSelf) {
    return {static_cast<int32_t>(eo::at(s, 0)), 1};
  }
  return detail::decode_rune_slow(s);
}

inline auto DecodeLastRune(slice<uint8_t> p) -> std::tuple<int32_t, int64_t> {
  auto end = eo::len(p);
  if (end == 0) {
    return {RuneError, 0};
  }
  auto start = end - 1;
  auto r = static_cast<int32_t>(eo::at(p, start));
  if (r < RuneSelf) {
    return {r, 1};
  }
  auto lim = std::max(end - UTFMax, int64_t{0});
  for (start--; start >= lim; start--) {
    if (RuneStart(eo::at(p, start))) {
      break;
    }
  }
  if (start < 0) {
    start = 0;
  }
  auto [r2, size] = DecodeRune(eo::subslice(p, start, end));
  if (start + size != end) {
    return {RuneError, 1};
  }
  return {r2, size};
}

inline auto DecodeLastRuneInString(std::string s) -> std::tuple<int32_t, int64_t> {
  auto end = eo::len(s);
  if (end == 0) {
    return {RuneError, 0};
  }
  auto start = end - 1;
  auto r = static_cast<int32_t>(eo::at(s, start));
  if (r < RuneSelf) {
    return {r, 1};
  }
  auto lim = std::max(end - UTFMax, int64_t{0});
  for (start--; start >= lim; start--) {
    if (RuneStart(eo::at(s, start))) {
      break;
    }
  }
  if (start < 0) {
    start = 0;
  }
  auto [r2, size] = DecodeRuneInString(eo::substr(s, start, end));
  if (start + size != end) {
    return {RuneError, 1};
  }
  return {r2, size};
}

inline auto RuneLen(int32_t r) -> int64_t {
  if (r < 0) {
    return -1;
  } else if (r <= detail::rune1Max) {
    return 1;
  } else if (r <= detail::rune2Max) {
    return 2;
  } else if (detail::surrogateMin <= r && r <= detail::surrogateMax) {
    return -1;
  } else if (r <= detail::rune3Max) {
    return 3;
  } else if (r <= MaxRune) {
    return 4;
  }
  return -1;
}

// EncodeRune writes into p and panics, as Go's does, when p is too short for the encoding. Go checks
// the last index first, so a short p is left unchanged.
inline auto EncodeRune(slice<uint8_t> p, int32_t r) -> int64_t {
  using namespace detail;
  auto i = static_cast<uint32_t>(r);
  if (i <= static_cast<uint32_t>(rune1Max)) {
    eo::at(p, 0) = static_cast<uint8_t>(r);
    return 1;
  }
  if (i <= static_cast<uint32_t>(rune2Max)) {
    static_cast<void>(eo::at(p, 1));
    eo::at(p, 0) = t2 | static_cast<uint8_t>(r >> 6);
    eo::at(p, 1) = tx | (static_cast<uint8_t>(r) & maskx);
    return 2;
  }
  if (i < static_cast<uint32_t>(surrogateMin) ||
    (static_cast<uint32_t>(surrogateMax) < i && i <= static_cast<uint32_t>(rune3Max))) {
    static_cast<void>(eo::at(p, 2));
    eo::at(p, 0) = t3 | static_cast<uint8_t>(r >> 12);
    eo::at(p, 1) = tx | (static_cast<uint8_t>(r >> 6) & maskx);
    eo::at(p, 2) = tx | (static_cast<uint8_t>(r) & maskx);
    return 3;
  }
  if (i > static_cast<uint32_t>(rune3Max) && i <= static_cast<uint32_t>(MaxRune)) {
    static_cast<void>(eo::at(p, 3));
    eo::at(p, 0) = t4 | static_cast<uint8_t>(r >> 18);
    eo::at(p, 1) = tx | (static_cast<uint8_t>(r >> 12) & maskx);
    eo::at(p, 2) = tx | (static_cast<uint8_t>(r >> 6) & maskx);
    eo::at(p, 3) = tx | (static_cast<uint8_t>(r) & maskx);
    return 4;
  }
  static_cast<void>(eo::at(p, 2));
  eo::at(p, 0) = runeErrorByte0;
  eo::at(p, 1) = runeErrorByte1;
  eo::at(p, 2) = runeErrorByte2;
  return 3;
}

inline auto AppendRune(slice<uint8_t> p, int32_t r) -> slice<uint8_t> {
  using namespace detail;
  auto i = static_cast<uint32_t>(r);
  if (i <= static_cast<uint32_t>(rune1Max)) {
    return eo::append(p, {static_cast<uint8_t>(r)});
  }
  if (i <= static_cast<uint32_t>(rune2Max)) {
    return eo::append(p,
      {static_cast<uint8_t>(t2 | static_cast<uint8_t>(r >> 6)), static_cast<uint8_t>(tx | (r & maskx))});
  }
  if (i < static_cast<uint32_t>(surrogateMin) ||
    (static_cast<uint32_t>(surrogateMax) < i && i <= static_cast<uint32_t>(rune3Max))) {
    return eo::append(p,
      {static_cast<uint8_t>(t3 | static_cast<uint8_t>(r >> 12)),
        static_cast<uint8_t>(tx | ((r >> 6) & maskx)),
        static_cast<uint8_t>(tx | (r & maskx))});
  }
  if (i > static_cast<uint32_t>(rune3Max) && i <= static_cast<uint32_t>(MaxRune)) {
    return eo::append(p,
      {static_cast<uint8_t>(t4 | static_cast<uint8_t>(r >> 18)),
        static_cast<uint8_t>(tx | ((r >> 12) & maskx)),
        static_cast<uint8_t>(tx | ((r >> 6) & maskx)),
        static_cast<uint8_t>(tx | (r & maskx))});
  }
  return eo::append(p, {runeErrorByte0, runeErrorByte1, runeErrorByte2});
}

// Go counts the iterations of for range s, which decodes as DecodeRuneInString does.
inline auto RuneCountInString(std::string s) -> int64_t {
  auto n = int64_t{0};
  for (auto i = std::size_t{0}; i < s.size(); n++) {
    i += eo::detail::decode_rune(s, i).second;
  }
  return n;
}

inline auto RuneCount(slice<uint8_t> p) -> int64_t {
  auto np = eo::len(p);
  auto n = int64_t{0};
  for (; n < np; n++) {
    if (auto c = eo::at(p, n); c >= RuneSelf) {
      return n + RuneCountInString(eo::string_from_bytes(eo::subslice(p, n)));
    }
  }
  return n;
}

inline auto Valid(slice<uint8_t> p) -> bool {
  return detail::valid(p);
}

inline auto ValidString(std::string s) -> bool {
  return detail::valid(s);
}

inline auto ValidRune(int32_t r) -> bool {
  if (0 <= r && r < detail::surrogateMin) {
    return true;
  } else if (detail::surrogateMax < r && r <= MaxRune) {
    return true;
  }
  return false;
}

} // namespace eo::utf8
