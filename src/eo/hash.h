// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once

#include <eo/core.h>
#include <eo/errors.h>
#include <eo/io.h>

#include <cstdint>
#include <string_view>
#include <tuple>
#include <utility>

// Go's hash: the interfaces of Go 1.26's hash.go. An embedded interface is spelled out as its methods,
// as io's interfaces are, so a Hash32 is also a Hash and an io::Writer.

namespace eo::hash {

struct Hash;
struct Hash32;
struct Hash64;
struct Cloner;
struct XOF;

// Go's hash.Hash.
struct Hash : eo::interface<Hash> {
  using interface::interface;
  static constexpr std::string_view _eo_name = "hash.Hash";

  auto Write(slice<uint8_t> p) const -> std::tuple<int64_t, error>;
  auto Sum(slice<uint8_t> b) const -> slice<uint8_t>;
  void Reset() const;
  auto Size() const -> int64_t;
  auto BlockSize() const -> int64_t;

  using _eo_methods = eo::methods<eo::method<&Hash::Write, "Write">,
    eo::method<&Hash::Sum, "Sum">,
    eo::method<&Hash::Reset, "Reset">,
    eo::method<&Hash::Size, "Size">,
    eo::method<&Hash::BlockSize, "BlockSize">>;
};

// Go's hash.Hash32.
struct Hash32 : eo::interface<Hash32> {
  using interface::interface;
  static constexpr std::string_view _eo_name = "hash.Hash32";

  auto Write(slice<uint8_t> p) const -> std::tuple<int64_t, error>;
  auto Sum(slice<uint8_t> b) const -> slice<uint8_t>;
  void Reset() const;
  auto Size() const -> int64_t;
  auto BlockSize() const -> int64_t;
  auto Sum32() const -> uint32_t;

  using _eo_methods = eo::methods<eo::method<&Hash32::Write, "Write">,
    eo::method<&Hash32::Sum, "Sum">,
    eo::method<&Hash32::Reset, "Reset">,
    eo::method<&Hash32::Size, "Size">,
    eo::method<&Hash32::BlockSize, "BlockSize">,
    eo::method<&Hash32::Sum32, "Sum32">>;
};

// Go's hash.Hash64.
struct Hash64 : eo::interface<Hash64> {
  using interface::interface;
  static constexpr std::string_view _eo_name = "hash.Hash64";

  auto Write(slice<uint8_t> p) const -> std::tuple<int64_t, error>;
  auto Sum(slice<uint8_t> b) const -> slice<uint8_t>;
  void Reset() const;
  auto Size() const -> int64_t;
  auto BlockSize() const -> int64_t;
  auto Sum64() const -> uint64_t;

  using _eo_methods = eo::methods<eo::method<&Hash64::Write, "Write">,
    eo::method<&Hash64::Sum, "Sum">,
    eo::method<&Hash64::Reset, "Reset">,
    eo::method<&Hash64::Size, "Size">,
    eo::method<&Hash64::BlockSize, "BlockSize">,
    eo::method<&Hash64::Sum64, "Sum64">>;
};

// Go's hash.Cloner.
struct Cloner : eo::interface<Cloner> {
  using interface::interface;
  static constexpr std::string_view _eo_name = "hash.Cloner";

  auto Write(slice<uint8_t> p) const -> std::tuple<int64_t, error>;
  auto Sum(slice<uint8_t> b) const -> slice<uint8_t>;
  void Reset() const;
  auto Size() const -> int64_t;
  auto BlockSize() const -> int64_t;
  auto Clone() const -> std::tuple<Cloner, error>;

  using _eo_methods = eo::methods<eo::method<&Cloner::Write, "Write">,
    eo::method<&Cloner::Sum, "Sum">,
    eo::method<&Cloner::Reset, "Reset">,
    eo::method<&Cloner::Size, "Size">,
    eo::method<&Cloner::BlockSize, "BlockSize">,
    eo::method<&Cloner::Clone, "Clone">>;
};

// Go's hash.XOF.
struct XOF : eo::interface<XOF> {
  using interface::interface;
  static constexpr std::string_view _eo_name = "hash.XOF";

  auto Write(slice<uint8_t> p) const -> std::tuple<int64_t, error>;
  auto Read(slice<uint8_t> p) const -> std::tuple<int64_t, error>;
  void Reset() const;
  auto BlockSize() const -> int64_t;

  using _eo_methods = eo::methods<eo::method<&XOF::Write, "Write">,
    eo::method<&XOF::Read, "Read">,
    eo::method<&XOF::Reset, "Reset">,
    eo::method<&XOF::BlockSize, "BlockSize">>;
};

// The methods of the interfaces, defined once every interface is complete.
inline auto Hash::Write(slice<uint8_t> p) const -> std::tuple<int64_t, error> {
  return _eo_call<&Hash::Write>(std::move(p));
}
inline auto Hash::Sum(slice<uint8_t> b) const -> slice<uint8_t> {
  return _eo_call<&Hash::Sum>(std::move(b));
}
inline void Hash::Reset() const {
  _eo_call<&Hash::Reset>();
}
inline auto Hash::Size() const -> int64_t {
  return _eo_call<&Hash::Size>();
}
inline auto Hash::BlockSize() const -> int64_t {
  return _eo_call<&Hash::BlockSize>();
}
inline auto Hash32::Write(slice<uint8_t> p) const -> std::tuple<int64_t, error> {
  return _eo_call<&Hash32::Write>(std::move(p));
}
inline auto Hash32::Sum(slice<uint8_t> b) const -> slice<uint8_t> {
  return _eo_call<&Hash32::Sum>(std::move(b));
}
inline void Hash32::Reset() const {
  _eo_call<&Hash32::Reset>();
}
inline auto Hash32::Size() const -> int64_t {
  return _eo_call<&Hash32::Size>();
}
inline auto Hash32::BlockSize() const -> int64_t {
  return _eo_call<&Hash32::BlockSize>();
}
inline auto Hash32::Sum32() const -> uint32_t {
  return _eo_call<&Hash32::Sum32>();
}
inline auto Hash64::Write(slice<uint8_t> p) const -> std::tuple<int64_t, error> {
  return _eo_call<&Hash64::Write>(std::move(p));
}
inline auto Hash64::Sum(slice<uint8_t> b) const -> slice<uint8_t> {
  return _eo_call<&Hash64::Sum>(std::move(b));
}
inline void Hash64::Reset() const {
  _eo_call<&Hash64::Reset>();
}
inline auto Hash64::Size() const -> int64_t {
  return _eo_call<&Hash64::Size>();
}
inline auto Hash64::BlockSize() const -> int64_t {
  return _eo_call<&Hash64::BlockSize>();
}
inline auto Hash64::Sum64() const -> uint64_t {
  return _eo_call<&Hash64::Sum64>();
}
inline auto Cloner::Write(slice<uint8_t> p) const -> std::tuple<int64_t, error> {
  return _eo_call<&Cloner::Write>(std::move(p));
}
inline auto Cloner::Sum(slice<uint8_t> b) const -> slice<uint8_t> {
  return _eo_call<&Cloner::Sum>(std::move(b));
}
inline void Cloner::Reset() const {
  _eo_call<&Cloner::Reset>();
}
inline auto Cloner::Size() const -> int64_t {
  return _eo_call<&Cloner::Size>();
}
inline auto Cloner::BlockSize() const -> int64_t {
  return _eo_call<&Cloner::BlockSize>();
}
inline auto Cloner::Clone() const -> std::tuple<Cloner, error> {
  return _eo_call<&Cloner::Clone>();
}
inline auto XOF::Write(slice<uint8_t> p) const -> std::tuple<int64_t, error> {
  return _eo_call<&XOF::Write>(std::move(p));
}
inline auto XOF::Read(slice<uint8_t> p) const -> std::tuple<int64_t, error> {
  return _eo_call<&XOF::Read>(std::move(p));
}
inline void XOF::Reset() const {
  _eo_call<&XOF::Reset>();
}
inline auto XOF::BlockSize() const -> int64_t {
  return _eo_call<&XOF::BlockSize>();
}

} // namespace eo::hash
