// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once
#include <eo/core.h>

#include <concepts>
#include <cstdint>
#include <string>
#include <type_traits>

// Go's cmp package.
namespace eo::cmp {

namespace detail {

  template<typename T>
  struct ordered_underlying {
    using type = T;
  };

  template<eo::detail::defined_type T>
  struct ordered_underlying<T> {
    using type = typename T::_eo_underlying;
  };

  template<typename T>
  using ordered_underlying_t = typename ordered_underlying<T>::type;

  // Only a NaN is not equal to itself.
  template<typename T>
  auto is_nan(const T& x) -> bool {
    if constexpr (std::is_floating_point_v<ordered_underlying_t<T>>) {
      return x != x;
    } else {
      return false;
    }
  }

} // namespace detail

// Go's cmp.Ordered: the integer, floating-point, and string types, and the defined types whose
// underlying type is one of them.
template<typename T>
concept Ordered =
  (std::integral<detail::ordered_underlying_t<T>> && !std::same_as<detail::ordered_underlying_t<T>, bool>) ||
  std::floating_point<detail::ordered_underlying_t<T>> || std::same_as<detail::ordered_underlying_t<T>, std::string>;

// Go's cmp.Less: whether x < y, where a NaN is less than any number that is not a NaN.
template<Ordered T>
auto Less(const T& x, const std::type_identity_t<T>& y) -> bool {
  return (detail::is_nan(x) && !detail::is_nan(y)) || x < y;
}

// Go's cmp.Compare: -1, 0, or +1 as x is less than, equal to, or greater than y. A NaN is less than
// any number that is not a NaN, and equal to a NaN, and -0.0 is equal to 0.0.
template<Ordered T>
auto Compare(const T& x, const std::type_identity_t<T>& y) -> int64_t {
  auto xNaN = detail::is_nan(x);
  auto yNaN = detail::is_nan(y);
  if (xNaN) {
    if (yNaN) {
      return 0;
    }
    return -1;
  }
  if (yNaN) {
    return +1;
  }
  if (x < y) {
    return -1;
  }
  if (x > y) {
    return +1;
  }
  return 0;
}

// Go's cmp.Or: the first of its arguments that is not the zero value, or the zero value. It is
// variadic, so its parameter is a slice.
template<typename T>
auto Or(slice<T> vals) -> T {
  auto zero = T{};
  for (auto i = int64_t{0}; i < eo::len(vals); ++i) {
    if (eo::at(vals, i) != zero) {
      return eo::at(vals, i);
    }
  }
  return zero;
}

} // namespace eo::cmp
