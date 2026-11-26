// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once
#include <eo/array.h>

#include <boost/pfr.hpp>

#include <concepts>
#include <cstddef>
#include <string_view>
#include <type_traits>

// The fields of a translated Go struct, which fmt prints and reflect reports. This header depends only
// on <eo/array.h>, so <eo/rtype.h> can use it wherever the other headers are in their include cycle.

namespace eo::detail {

// A translated Go struct: a class aggregate, which Boost.PFR can visit field by field. This also
// matches other aggregates, so it is ambiguous with fmt's own formatters for aggregate types, such as
// std::tm in <fmt/chrono.h>, std::monostate in <fmt/std.h>, and fmt::streamed. Eo uses none of them
// for Go values.
template<typename T>
concept go_struct = std::is_class_v<T> && std::is_aggregate_v<T> && !is_std_array<T>;

// A struct of a package that Eo maps onto a C++ class whose fields are not Go's, such as big.Int,
// which Go declares as struct { neg bool; abs nat }. Its kind is Struct, as in Go, but reflect sees
// none of its fields, all of which are unexported in Go. The class declares _eo_opaque_struct, and
// _eo_is_zero(v), which reports whether v is the zero value, as Go's reflect.Value.IsZero does.
template<typename T>
concept opaque_struct = std::is_class_v<T> && requires(const T& v) {
  { T::_eo_opaque_struct } -> std::convertible_to<bool>;
  { T::_eo_is_zero(v) } -> std::same_as<bool>;
};

// The name of field I of the translated struct T, as %+v and %#v print it. Boost.PFR gives field
// names only from Boost 1.84, so Eo reads them as PFR does: from the name of a function whose template
// argument points to the field of an object that is declared and never defined. GCC and Clang print
// that pointer as an expression that ends with the field's name, such as
// `(& field_object<T>.field_holder<T>::value.T::Name))}` and `&field_object.value.Name}`.
template<typename T>
struct field_holder {
  T value;
};

#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wundefined-var-template"
#endif

template<typename T>
extern const field_holder<T> field_object;

template<typename T>
struct field_pointer {
  const T* pointer;
};

template<auto P>
auto field_signature() -> const char* {
  return __PRETTY_FUNCTION__;
}

template<typename T, std::size_t I>
auto field_name() -> std::string_view {
  using F = std::remove_cvref_t<decltype(boost::pfr::get<I>(field_object<T>.value))>;
  auto s = std::string_view{field_signature<field_pointer<F>{&boost::pfr::get<I>(field_object<T>.value)}>()};
  auto end = s.size();
  while (end > 0 && (s[end - 1] == ']' || s[end - 1] == '}' || s[end - 1] == ')')) {
    --end;
  }
  auto start = end;
  while (start > 0 && s[start - 1] != '.' && s[start - 1] != ':') {
    --start;
  }
  return s.substr(start, end - start);
}

#if defined(__clang__)
#  pragma clang diagnostic pop
#endif

} // namespace eo::detail
