// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once

#include <eo/core.h>

#include <string_view>
#include <tuple>
#include <utility>

// Go's encoding: the interfaces that types implement to convert themselves to and from bytes and
// text, from Go 1.26's encoding.go. A translated type implements one by declaring the method, as for
// any interface.

namespace eo::encoding {

// Go's encoding.BinaryMarshaler.
struct BinaryMarshaler : eo::interface<BinaryMarshaler> {
  using interface::interface;
  static constexpr std::string_view _eo_name = "encoding.BinaryMarshaler";

  auto MarshalBinary() const -> std::tuple<slice<uint8_t>, error>;

  using _eo_methods = eo::methods<eo::method<&BinaryMarshaler::MarshalBinary, "MarshalBinary">>;
};

// Go's encoding.BinaryUnmarshaler.
struct BinaryUnmarshaler : eo::interface<BinaryUnmarshaler> {
  using interface::interface;
  static constexpr std::string_view _eo_name = "encoding.BinaryUnmarshaler";

  auto UnmarshalBinary(slice<uint8_t> data) const -> error;

  using _eo_methods = eo::methods<eo::method<&BinaryUnmarshaler::UnmarshalBinary, "UnmarshalBinary">>;
};

// Go's encoding.BinaryAppender.
struct BinaryAppender : eo::interface<BinaryAppender> {
  using interface::interface;
  static constexpr std::string_view _eo_name = "encoding.BinaryAppender";

  auto AppendBinary(slice<uint8_t> b) const -> std::tuple<slice<uint8_t>, error>;

  using _eo_methods = eo::methods<eo::method<&BinaryAppender::AppendBinary, "AppendBinary">>;
};

// Go's encoding.TextMarshaler.
struct TextMarshaler : eo::interface<TextMarshaler> {
  using interface::interface;
  static constexpr std::string_view _eo_name = "encoding.TextMarshaler";

  auto MarshalText() const -> std::tuple<slice<uint8_t>, error>;

  using _eo_methods = eo::methods<eo::method<&TextMarshaler::MarshalText, "MarshalText">>;
};

// Go's encoding.TextUnmarshaler.
struct TextUnmarshaler : eo::interface<TextUnmarshaler> {
  using interface::interface;
  static constexpr std::string_view _eo_name = "encoding.TextUnmarshaler";

  auto UnmarshalText(slice<uint8_t> text) const -> error;

  using _eo_methods = eo::methods<eo::method<&TextUnmarshaler::UnmarshalText, "UnmarshalText">>;
};

// Go's encoding.TextAppender.
struct TextAppender : eo::interface<TextAppender> {
  using interface::interface;
  static constexpr std::string_view _eo_name = "encoding.TextAppender";

  auto AppendText(slice<uint8_t> b) const -> std::tuple<slice<uint8_t>, error>;

  using _eo_methods = eo::methods<eo::method<&TextAppender::AppendText, "AppendText">>;
};

inline auto BinaryMarshaler::MarshalBinary() const -> std::tuple<slice<uint8_t>, error> {
  return _eo_call<&BinaryMarshaler::MarshalBinary>();
}
inline auto BinaryUnmarshaler::UnmarshalBinary(slice<uint8_t> data) const -> error {
  return _eo_call<&BinaryUnmarshaler::UnmarshalBinary>(std::move(data));
}
inline auto BinaryAppender::AppendBinary(slice<uint8_t> b) const -> std::tuple<slice<uint8_t>, error> {
  return _eo_call<&BinaryAppender::AppendBinary>(std::move(b));
}
inline auto TextMarshaler::MarshalText() const -> std::tuple<slice<uint8_t>, error> {
  return _eo_call<&TextMarshaler::MarshalText>();
}
inline auto TextUnmarshaler::UnmarshalText(slice<uint8_t> text) const -> error {
  return _eo_call<&TextUnmarshaler::UnmarshalText>(std::move(text));
}
inline auto TextAppender::AppendText(slice<uint8_t> b) const -> std::tuple<slice<uint8_t>, error> {
  return _eo_call<&TextAppender::AppendText>(std::move(b));
}

} // namespace eo::encoding
