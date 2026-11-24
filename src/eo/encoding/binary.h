// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once

#include <eo/core.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>

// Go's encoding/binary, in part: the byte orders and the varint encoding, ported from Go 1.26's
// binary.go and varint.go. The methods of binary.BigEndian and binary.LittleEndian have value
// receivers of the unexported types bigEndian and littleEndian, and they are written as for a
// translated type: binary.BigEndian.Uint64(b) is binary::bigEndian::Uint64(binary::BigEndian, b).

namespace eo::binary {

// Go's binary.ByteOrder.
struct ByteOrder : eo::interface<ByteOrder> {
  using interface::interface;
  static constexpr std::string_view _eo_name = "binary.ByteOrder";

  auto Uint16(slice<uint8_t> b) const -> uint16_t;
  auto Uint32(slice<uint8_t> b) const -> uint32_t;
  auto Uint64(slice<uint8_t> b) const -> uint64_t;
  void PutUint16(slice<uint8_t> b, uint16_t v) const;
  void PutUint32(slice<uint8_t> b, uint32_t v) const;
  void PutUint64(slice<uint8_t> b, uint64_t v) const;
  auto String() const -> std::string;

  using _eo_methods = eo::methods<eo::method<&ByteOrder::Uint16, "Uint16">,
    eo::method<&ByteOrder::Uint32, "Uint32">,
    eo::method<&ByteOrder::Uint64, "Uint64">,
    eo::method<&ByteOrder::PutUint16, "PutUint16">,
    eo::method<&ByteOrder::PutUint32, "PutUint32">,
    eo::method<&ByteOrder::PutUint64, "PutUint64">,
    eo::method<&ByteOrder::String, "String">>;
};

inline auto ByteOrder::Uint16(slice<uint8_t> b) const -> uint16_t {
  return _eo_call<&ByteOrder::Uint16>(std::move(b));
}
inline auto ByteOrder::Uint32(slice<uint8_t> b) const -> uint32_t {
  return _eo_call<&ByteOrder::Uint32>(std::move(b));
}
inline auto ByteOrder::Uint64(slice<uint8_t> b) const -> uint64_t {
  return _eo_call<&ByteOrder::Uint64>(std::move(b));
}
inline void ByteOrder::PutUint16(slice<uint8_t> b, uint16_t v) const {
  _eo_call<&ByteOrder::PutUint16>(std::move(b), v);
}
inline void ByteOrder::PutUint32(slice<uint8_t> b, uint32_t v) const {
  _eo_call<&ByteOrder::PutUint32>(std::move(b), v);
}
inline void ByteOrder::PutUint64(slice<uint8_t> b, uint64_t v) const {
  _eo_call<&ByteOrder::PutUint64>(std::move(b), v);
}
inline auto ByteOrder::String() const -> std::string {
  return _eo_call<&ByteOrder::String>();
}

// Go's binary.AppendByteOrder.
struct AppendByteOrder : eo::interface<AppendByteOrder> {
  using interface::interface;
  static constexpr std::string_view _eo_name = "binary.AppendByteOrder";

  auto AppendUint16(slice<uint8_t> b, uint16_t v) const -> slice<uint8_t>;
  auto AppendUint32(slice<uint8_t> b, uint32_t v) const -> slice<uint8_t>;
  auto AppendUint64(slice<uint8_t> b, uint64_t v) const -> slice<uint8_t>;
  auto String() const -> std::string;

  using _eo_methods = eo::methods<eo::method<&AppendByteOrder::AppendUint16, "AppendUint16">,
    eo::method<&AppendByteOrder::AppendUint32, "AppendUint32">,
    eo::method<&AppendByteOrder::AppendUint64, "AppendUint64">,
    eo::method<&AppendByteOrder::String, "String">>;
};

inline auto AppendByteOrder::AppendUint16(slice<uint8_t> b, uint16_t v) const -> slice<uint8_t> {
  return _eo_call<&AppendByteOrder::AppendUint16>(std::move(b), v);
}
inline auto AppendByteOrder::AppendUint32(slice<uint8_t> b, uint32_t v) const -> slice<uint8_t> {
  return _eo_call<&AppendByteOrder::AppendUint32>(std::move(b), v);
}
inline auto AppendByteOrder::AppendUint64(slice<uint8_t> b, uint64_t v) const -> slice<uint8_t> {
  return _eo_call<&AppendByteOrder::AppendUint64>(std::move(b), v);
}
inline auto AppendByteOrder::String() const -> std::string {
  return _eo_call<&AppendByteOrder::String>();
}

namespace detail {

  // Go's `_ = b[n-1]`, which panics with the index error before any element is read or written.
  inline void check(const slice<uint8_t>& b, int64_t n) {
    (void)eo::at(b, n - 1);
  }

  // Byte k of a big-endian value of n bytes.
  template<typename U>
  inline auto get(const slice<uint8_t>& b, int64_t n, bool big) -> U {
    check(b, n);
    auto v = U{0};
    for (int64_t k = 0; k < n; ++k) {
      auto byte = static_cast<U>(b.data()[big ? k : n - 1 - k]);
      v = static_cast<U>(v | static_cast<U>(byte << (8 * (n - 1 - k))));
    }
    return v;
  }

  template<typename U>
  inline void put(const slice<uint8_t>& b, U v, int64_t n, bool big) {
    check(b, n);
    for (int64_t k = 0; k < n; ++k) {
      b.data()[big ? n - 1 - k : k] = static_cast<uint8_t>(v >> (8 * k));
    }
  }

  template<typename U>
  inline auto append(slice<uint8_t> b, U v, int64_t n, bool big) -> slice<uint8_t> {
    for (int64_t k = 0; k < n; ++k) {
      auto shift = big ? 8 * (n - 1 - k) : 8 * k;
      b = eo::append(std::move(b), {static_cast<uint8_t>(v >> shift)});
    }
    return b;
  }

} // namespace detail

struct littleEndian {
  static constexpr std::string_view _eo_name = "binary.littleEndian";

  auto operator==(const littleEndian&) const -> bool = default;

  static auto Uint16(littleEndian, slice<uint8_t> b) -> uint16_t {
    return detail::get<uint16_t>(b, 2, false);
  }
  static auto Uint32(littleEndian, slice<uint8_t> b) -> uint32_t {
    return detail::get<uint32_t>(b, 4, false);
  }
  static auto Uint64(littleEndian, slice<uint8_t> b) -> uint64_t {
    return detail::get<uint64_t>(b, 8, false);
  }
  static void PutUint16(littleEndian, slice<uint8_t> b, uint16_t v) {
    detail::put(b, v, 2, false);
  }
  static void PutUint32(littleEndian, slice<uint8_t> b, uint32_t v) {
    detail::put(b, v, 4, false);
  }
  static void PutUint64(littleEndian, slice<uint8_t> b, uint64_t v) {
    detail::put(b, v, 8, false);
  }
  static auto AppendUint16(littleEndian, slice<uint8_t> b, uint16_t v) -> slice<uint8_t> {
    return detail::append(std::move(b), v, 2, false);
  }
  static auto AppendUint32(littleEndian, slice<uint8_t> b, uint32_t v) -> slice<uint8_t> {
    return detail::append(std::move(b), v, 4, false);
  }
  static auto AppendUint64(littleEndian, slice<uint8_t> b, uint64_t v) -> slice<uint8_t> {
    return detail::append(std::move(b), v, 8, false);
  }
  static auto String(littleEndian) -> std::string {
    return "LittleEndian";
  }
  static auto GoString(littleEndian) -> std::string {
    return "binary.LittleEndian";
  }

  using _eo_methods = eo::methods<eo::method<&littleEndian::Uint16, "Uint16">,
    eo::method<&littleEndian::Uint32, "Uint32">,
    eo::method<&littleEndian::Uint64, "Uint64">,
    eo::method<&littleEndian::PutUint16, "PutUint16">,
    eo::method<&littleEndian::PutUint32, "PutUint32">,
    eo::method<&littleEndian::PutUint64, "PutUint64">,
    eo::method<&littleEndian::AppendUint16, "AppendUint16">,
    eo::method<&littleEndian::AppendUint32, "AppendUint32">,
    eo::method<&littleEndian::AppendUint64, "AppendUint64">,
    eo::method<&littleEndian::String, "String">,
    eo::method<&littleEndian::GoString, "GoString">>;
};

struct bigEndian {
  static constexpr std::string_view _eo_name = "binary.bigEndian";

  auto operator==(const bigEndian&) const -> bool = default;

  static auto Uint16(bigEndian, slice<uint8_t> b) -> uint16_t {
    return detail::get<uint16_t>(b, 2, true);
  }
  static auto Uint32(bigEndian, slice<uint8_t> b) -> uint32_t {
    return detail::get<uint32_t>(b, 4, true);
  }
  static auto Uint64(bigEndian, slice<uint8_t> b) -> uint64_t {
    return detail::get<uint64_t>(b, 8, true);
  }
  static void PutUint16(bigEndian, slice<uint8_t> b, uint16_t v) {
    detail::put(b, v, 2, true);
  }
  static void PutUint32(bigEndian, slice<uint8_t> b, uint32_t v) {
    detail::put(b, v, 4, true);
  }
  static void PutUint64(bigEndian, slice<uint8_t> b, uint64_t v) {
    detail::put(b, v, 8, true);
  }
  static auto AppendUint16(bigEndian, slice<uint8_t> b, uint16_t v) -> slice<uint8_t> {
    return detail::append(std::move(b), v, 2, true);
  }
  static auto AppendUint32(bigEndian, slice<uint8_t> b, uint32_t v) -> slice<uint8_t> {
    return detail::append(std::move(b), v, 4, true);
  }
  static auto AppendUint64(bigEndian, slice<uint8_t> b, uint64_t v) -> slice<uint8_t> {
    return detail::append(std::move(b), v, 8, true);
  }
  static auto String(bigEndian) -> std::string {
    return "BigEndian";
  }
  static auto GoString(bigEndian) -> std::string {
    return "binary.BigEndian";
  }

  using _eo_methods = eo::methods<eo::method<&bigEndian::Uint16, "Uint16">,
    eo::method<&bigEndian::Uint32, "Uint32">,
    eo::method<&bigEndian::Uint64, "Uint64">,
    eo::method<&bigEndian::PutUint16, "PutUint16">,
    eo::method<&bigEndian::PutUint32, "PutUint32">,
    eo::method<&bigEndian::PutUint64, "PutUint64">,
    eo::method<&bigEndian::AppendUint16, "AppendUint16">,
    eo::method<&bigEndian::AppendUint32, "AppendUint32">,
    eo::method<&bigEndian::AppendUint64, "AppendUint64">,
    eo::method<&bigEndian::String, "String">,
    eo::method<&bigEndian::GoString, "GoString">>;
};

inline constexpr littleEndian LittleEndian{};
inline constexpr bigEndian BigEndian{};

inline constexpr int64_t MaxVarintLen16 = 3;
inline constexpr int64_t MaxVarintLen32 = 5;
inline constexpr int64_t MaxVarintLen64 = 10;

inline auto AppendUvarint(slice<uint8_t> buf, uint64_t x) -> slice<uint8_t> {
  while (x >= 0x80) {
    buf = eo::append(std::move(buf), {static_cast<uint8_t>(static_cast<uint8_t>(x) | 0x80)});
    x >>= 7;
  }
  return eo::append(std::move(buf), {static_cast<uint8_t>(x)});
}

// Writes x to buf, which panics with Go's index error when buf is too short.
inline auto PutUvarint(slice<uint8_t> buf, uint64_t x) -> int64_t {
  auto i = int64_t{0};
  while (x >= 0x80) {
    eo::at(buf, i) = static_cast<uint8_t>(static_cast<uint8_t>(x) | 0x80);
    x >>= 7;
    i++;
  }
  eo::at(buf, i) = static_cast<uint8_t>(x);
  return i + 1;
}

// The value and the number of bytes read: 0 bytes for a buffer that ends too early, and -(n) when
// the value overflows 64 bits at byte n.
inline auto Uvarint(slice<uint8_t> buf) -> std::tuple<uint64_t, int64_t> {
  auto x = uint64_t{0};
  auto s = 0u;
  for (int64_t i = 0; i < eo::len(buf); ++i) {
    auto b = buf.data()[i];
    if (i == MaxVarintLen64) {
      return {0, -(i + 1)};
    }
    if (b < 0x80) {
      if (i == MaxVarintLen64 - 1 && b > 1) {
        return {0, -(i + 1)};
      }
      return {x | static_cast<uint64_t>(b) << s, i + 1};
    }
    x |= static_cast<uint64_t>(b & 0x7f) << s;
    s += 7;
  }
  return {0, 0};
}

inline auto AppendVarint(slice<uint8_t> buf, int64_t x) -> slice<uint8_t> {
  auto ux = static_cast<uint64_t>(x) << 1;
  if (x < 0) {
    ux = ~ux;
  }
  return AppendUvarint(std::move(buf), ux);
}

inline auto PutVarint(slice<uint8_t> buf, int64_t x) -> int64_t {
  auto ux = static_cast<uint64_t>(x) << 1;
  if (x < 0) {
    ux = ~ux;
  }
  return PutUvarint(std::move(buf), ux);
}

inline auto Varint(slice<uint8_t> buf) -> std::tuple<int64_t, int64_t> {
  auto [ux, n] = Uvarint(std::move(buf));
  auto x = static_cast<int64_t>(ux >> 1);
  if ((ux & 1) != 0) {
    x = ~x;
  }
  return {x, n};
}

} // namespace eo::binary
