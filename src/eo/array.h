// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <type_traits>
#include <utility>

// Go's [N]T is std::array<T, N>, which has Go's value semantics: assignment, arguments, and results
// copy the elements, and == compares them in order. A defined array type, such as `type Hash
// [32]byte`, derives from its std::array (see <eo/defined.h>). The helpers of the other headers take
// both through the concept go_array.

namespace eo {

namespace detail {

  template<typename T>
  inline constexpr bool is_std_array = false;

  template<typename T, std::size_t N>
  inline constexpr bool is_std_array<std::array<T, N>> = true;

  // The std::array that A is or derives from. Declared only, for its type.
  template<typename T, std::size_t N>
  auto array_base(const std::array<T, N>&) -> std::array<T, N>;

  // A Go array: a std::array, or a defined array type that derives from one.
  template<typename A>
  concept go_array = requires(const A& a) { detail::array_base(a); };

  template<go_array A>
  using array_base_t = decltype(detail::array_base(std::declval<const A&>()));

  template<go_array A>
  inline constexpr int64_t array_length = static_cast<int64_t>(std::tuple_size_v<array_base_t<A>>);

  template<typename A>
  struct keyed_array_fn;

  template<typename T, std::size_t N>
  struct keyed_array_fn<std::array<T, N>> {
    // [N]T{k1: v1, k2: v2}, with every element keyed by its index: zero values, then each value at its
    // index. The braced list evaluates the elements in source order, as Go does.
    auto operator()(std::initializer_list<std::pair<int64_t, T>> elements) const -> std::array<T, N> {
      auto a = std::array<T, N>{};
      for (const auto& [i, v] : elements) {
        a[static_cast<std::size_t>(i)] = v;
      }
      return a;
    }
  };

} // namespace detail

// eo::keyed_array<std::array<T, N>>({{i, v}, ...}): an array literal with keyed elements. Go's keys
// are constants in range, so the indexes are not checked.
template<typename A>
inline constexpr detail::keyed_array_fn<A> keyed_array{};

} // namespace eo
