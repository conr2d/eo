// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once
#include <eo/bounds.h>

#include <climits>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <utility>

// Go's integer operators whose plain C++ form differs from Go's: division and remainder, which
// panic on a zero divisor and wrap for the most negative value divided by -1, and shifts, which
// panic on a negative count and are defined for a count of at least the width. Signed +, -, * and
// unary - wrap in Go; Eo builds with -fwrapv so that the plain C++ operators wrap too.

// Go's int and uint are 64 bits on the platforms Eo targets, and its float32 and float64 are IEEE 754
// binary32 and binary64.
static_assert(sizeof(void*) == 8, "Eo requires a 64-bit platform");
static_assert(std::numeric_limits<float>::is_iec559 && std::numeric_limits<double>::is_iec559,
  "Eo requires IEEE 754 float and double");

namespace eo {

namespace detail {

  // The width of T in bits.
  template<go_integer T>
  inline constexpr auto width = static_cast<int>(sizeof(T) * CHAR_BIT);

  struct div_fn {
    // x / y. The result has the type of x, without C++'s promotion of narrow types to int.
    template<go_integer T>
    constexpr auto operator()(T x, std::type_identity_t<T> y) const -> T {
      if (y == 0) {
        panic_runtime_error("integer divide by zero");
      }
      if constexpr (std::is_signed_v<T>) {
        if (y == -1) {
          // Go's -x, which wraps for the most negative value instead of trapping.
          return static_cast<T>(-static_cast<std::make_unsigned_t<T>>(x));
        }
      }
      return static_cast<T>(x / y);
    }
  };

  struct rem_fn {
    // x % y, with the sign of x, as in Go and C++.
    template<go_integer T>
    constexpr auto operator()(T x, std::type_identity_t<T> y) const -> T {
      if (y == 0) {
        panic_runtime_error("integer divide by zero");
      }
      if constexpr (std::is_signed_v<T>) {
        if (y == -1) {
          return 0;
        }
      }
      return static_cast<T>(x % y);
    }
  };

  // A shift count may have any integer type. A negative count panics.
  template<go_integer N>
  constexpr void check_shift(N n) {
    if (std::cmp_less(n, 0)) {
      panic_runtime_error("negative shift amount");
    }
  }

  struct shl_fn {
    // x << n. A count of at least the width shifts every bit out.
    template<go_integer T, go_integer N>
    constexpr auto operator()(T x, N n) const -> T {
      check_shift(n);
      if (std::cmp_greater_equal(n, width<T>)) {
        return 0;
      }
      return static_cast<T>(static_cast<std::make_unsigned_t<T>>(x) << n);
    }
  };

  struct shr_fn {
    // x >> n, arithmetic for a signed x. A count of at least the width leaves only the sign: -1 for
    // a negative x, and 0 otherwise.
    template<go_integer T, go_integer N>
    constexpr auto operator()(T x, N n) const -> T {
      check_shift(n);
      if (std::cmp_greater_equal(n, width<T>)) {
        return x < 0 ? static_cast<T>(-1) : static_cast<T>(0);
      }
      return static_cast<T>(x >> n);
    }
  };

  // x op= y: evaluates x once, as Go does, and assigns the result of op.
  template<typename Op>
  struct assign_fn {
    template<go_integer T, go_integer U>
    constexpr void operator()(T& x, U y) const {
      x = Op{}(x, y);
    }
  };

} // namespace detail

// Function objects, called qualified, like the string helpers (see <eo/string.h>).
inline constexpr detail::div_fn div{};
inline constexpr detail::rem_fn rem{};
inline constexpr detail::shl_fn shl{};
inline constexpr detail::shr_fn shr{};
inline constexpr detail::assign_fn<detail::div_fn> div_assign{};
inline constexpr detail::assign_fn<detail::rem_fn> rem_assign{};
inline constexpr detail::assign_fn<detail::shl_fn> shl_assign{};
inline constexpr detail::assign_fn<detail::shr_fn> shr_assign{};

} // namespace eo
