// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once

#include <compare>
#include <concepts>
#include <string>
#include <type_traits>
#include <utility>

// Go's defined types, such as `type Celsius float64`, whose underlying type is not a struct
// literal. A translated defined type D derives from defined<D, U>, U being the translated underlying
// type, so D is a type of its own, which can declare methods and a Go type name, as a translated
// struct does. For `type B A` with a struct type A, U is A, which stands for A's struct type.

namespace eo {

template<typename D, typename U>
struct defined;

template<typename... M>
struct methods;

namespace detail {

  // A class type that a defined type derives from, and among those a struct type: a translated
  // struct or a struct of a package that Eo maps, which C++ tells from the other classes by its Go
  // name. Concepts, so that the constraints of the two specializations below subsume.
  template<typename U>
  concept class_underlying = std::is_class_v<U>;

  template<typename U>
  concept struct_underlying = class_underlying<U> && requires { U::_eo_name; };

} // namespace detail

namespace detail {

  // Whether T is a translated defined type, and its underlying type.
  template<typename T>
  concept defined_type =
    requires { typename T::_eo_underlying; } && std::derived_from<T, defined<T, typename T::_eo_underlying>>;

  template<typename T>
  concept defined_arithmetic = defined_type<T> && std::is_arithmetic_v<typename T::_eo_underlying>;

  template<typename T>
  concept defined_integer = defined_arithmetic<T> && std::integral<typename T::_eo_underlying> &&
    !std::same_as<typename T::_eo_underlying, bool>;

  // The value of a defined type as its underlying type: a copy for a number or a bool, and a
  // reference to the base otherwise.
  template<defined_type T>
  constexpr auto underlying(const T& value) -> decltype(auto) {
    if constexpr (std::is_arithmetic_v<typename T::_eo_underlying>) {
      return value._eo_value;
    } else {
      return static_cast<const typename T::_eo_underlying&>(value);
    }
  }

} // namespace detail

// A defined type whose underlying type is a number or a bool holds the value. Its operators take and
// return D, as Go's operators keep the defined type, and they convert the result back to U, so that
// a narrow type wraps as Go's does. Conversions to and from numbers are explicit, as in Go.
template<typename D, typename U>
  requires std::is_arithmetic_v<U>
struct defined<D, U> {
  using _eo_underlying = U;

  U _eo_value{};

  constexpr defined() = default;

  template<typename V>
    requires std::is_arithmetic_v<V>
  constexpr explicit defined(V v): _eo_value(static_cast<U>(v)) {}

  // A conversion from another defined type, whose underlying type is a number.
  template<detail::defined_arithmetic E>
  constexpr explicit defined(const E& e): _eo_value(static_cast<U>(e._eo_value)) {}

  template<typename V>
    requires std::is_arithmetic_v<V>
  constexpr explicit operator V() const {
    return static_cast<V>(_eo_value);
  }

  friend constexpr auto operator==(D a, D b) -> bool {
    return a._eo_value == b._eo_value;
  }

  friend constexpr auto operator<=>(D a, D b)
    requires(!std::same_as<U, bool>)
  {
    return a._eo_value <=> b._eo_value;
  }

  friend constexpr auto operator!(D a) -> D
    requires std::same_as<U, bool>
  {
    return D(!a._eo_value);
  }

  // Arithmetic. Integer division by a variable goes through eo::div and eo::rem, as for a basic
  // type; `/` and `%` here are for a constant divisor other than -1, and `/` for floats.
  friend constexpr auto operator+(D a) -> D
    requires(!std::same_as<U, bool>)
  {
    return a;
  }

  friend constexpr auto operator-(D a) -> D
    requires(!std::same_as<U, bool>)
  {
    return D(-a._eo_value);
  }

  friend constexpr auto operator+(D a, D b) -> D
    requires(!std::same_as<U, bool>)
  {
    return D(a._eo_value + b._eo_value);
  }

  friend constexpr auto operator-(D a, D b) -> D
    requires(!std::same_as<U, bool>)
  {
    return D(a._eo_value - b._eo_value);
  }

  friend constexpr auto operator*(D a, D b) -> D
    requires(!std::same_as<U, bool>)
  {
    return D(a._eo_value * b._eo_value);
  }

  friend constexpr auto operator/(D a, D b) -> D
    requires(!std::same_as<U, bool>)
  {
    return D(a._eo_value / b._eo_value);
  }

  friend constexpr auto operator%(D a, D b) -> D
    requires(std::integral<U> && !std::same_as<U, bool>)
  {
    return D(a._eo_value % b._eo_value);
  }

  friend constexpr auto operator~(D a) -> D
    requires(std::integral<U> && !std::same_as<U, bool>)
  {
    return D(~a._eo_value);
  }

  friend constexpr auto operator&(D a, D b) -> D
    requires(std::integral<U> && !std::same_as<U, bool>)
  {
    return D(a._eo_value & b._eo_value);
  }

  friend constexpr auto operator|(D a, D b) -> D
    requires(std::integral<U> && !std::same_as<U, bool>)
  {
    return D(a._eo_value | b._eo_value);
  }

  friend constexpr auto operator^(D a, D b) -> D
    requires(std::integral<U> && !std::same_as<U, bool>)
  {
    return D(a._eo_value ^ b._eo_value);
  }

  // A shift by a constant count less than the width, which may have any integer type.
  template<std::integral N>
  friend constexpr auto operator<<(D a, N n) -> D
    requires(std::integral<U> && !std::same_as<U, bool>)
  {
    return D(a._eo_value << n);
  }

  template<std::integral N>
  friend constexpr auto operator>>(D a, N n) -> D
    requires(std::integral<U> && !std::same_as<U, bool>)
  {
    return D(a._eo_value >> n);
  }

  friend constexpr auto operator+=(D& a, D b) -> D& requires(!std::same_as<U, bool>) { return a = a + b; }

  friend constexpr auto operator-=(D& a, D b) -> D& requires(!std::same_as<U, bool>) { return a = a - b; }

  friend constexpr auto operator*=(D& a, D b) -> D& requires(!std::same_as<U, bool>) { return a = a * b; }

  friend constexpr auto operator/=(D& a, D b) -> D& requires(!std::same_as<U, bool>) { return a = a / b; }

  friend constexpr auto operator%=(D& a, D b) -> D& requires(
                                                std::integral<U> && !std::same_as<U, bool>) { return a = a % b; }

  friend constexpr auto operator&=(D& a, D b) -> D& requires(
                                                std::integral<U> && !std::same_as<U, bool>) { return a = a & b; }

  friend constexpr auto operator|=(D& a, D b) -> D& requires(
                                                std::integral<U> && !std::same_as<U, bool>) { return a = a | b; }

  friend constexpr auto operator^=(D& a, D b) -> D& requires(
                                                std::integral<U> && !std::same_as<U, bool>) { return a = a ^ b; }

  template<std::integral N>
  friend constexpr auto operator<<=(D& a, N n) -> D& requires(
                                                 std::integral<U> && !std::same_as<U, bool>) { return a = a << n; }

  template<std::integral N>
  friend constexpr auto operator>>=(D& a, N n) -> D& requires(
                                                 std::integral<U> && !std::same_as<U, bool>) { return a = a >> n; }

  friend constexpr auto operator++(D& a) -> D& requires(!std::same_as<U, bool>) { return a = D(a._eo_value + 1); }

  friend constexpr auto operator--(D& a) -> D& requires(!std::same_as<U, bool>) { return a = D(a._eo_value - 1); }

  friend constexpr auto operator++(D& a, int) -> D
    requires(!std::same_as<U, bool>)
  {
    auto old = a;
    ++a;
    return old;
  }

  friend constexpr auto operator--(D& a, int) -> D
    requires(!std::same_as<U, bool>)
  {
    auto old = a;
    --a;
    return old;
  }
};

// A defined type whose underlying type is a string, an array, a slice, a map, or a function type
// derives from it, so the helpers of U take it as they take U. A value of an unnamed array, slice,
// map, or function type U converts to D implicitly, as Go assigns it to a defined type with the same
// underlying type. A string, which is a defined type in Go, converts only explicitly, and so does
// another defined type with the same underlying type.
template<typename D, detail::class_underlying U>
struct defined<D, U> : U {
  using _eo_underlying = U;

  using U::U;

  defined() = default;

  template<typename V>
    requires std::same_as<std::remove_cvref_t<V>, U>
  explicit(std::same_as<U, std::string>) defined(V&& u): U(std::forward<V>(u)) {}

  template<typename V>
    requires detail::defined_type<V> && std::same_as<typename V::_eo_underlying, U> && (!std::same_as<V, D>)
  explicit defined(const V& v): U(static_cast<const U&>(v)) {}

  // Go's + of strings keeps the defined type.
  friend auto operator+(const D& a, const D& b) -> D
    requires std::same_as<U, std::string>
  {
    return D(static_cast<const U&>(a) + static_cast<const U&>(b));
  }
};

// A defined type over a struct type. D derives from the struct, so its fields are D's, and the
// struct converts to D only explicitly, as in Go. D does not have the struct's
// methods, as a Go defined type does not, so defined lists none, and D lists its own methods, if it
// has any, in its own _eo_methods.
template<typename D, detail::struct_underlying U>
struct defined<D, U> : U {
  using _eo_underlying = U;
  using _eo_methods = methods<>;

  using U::U;

  defined() = default;

  explicit defined(const U& u): U(u) {}
  explicit defined(U&& u): U(std::move(u)) {}
};

} // namespace eo
