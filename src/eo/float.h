// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once

// Go's %v of float32 and float64 values, for fmt::Println and for the message of a panic value.

#include <fmt/format.h>

#include <charconv>
#include <cmath>
#include <concepts>
#include <string_view>

namespace eo::detail {

// A float32 or float64 value printed as Go's %v prints it, which differs from fmt's {}.
template<std::floating_point F>
struct go_float {
  F value;
};

// The value to format with {} for Go's %v: a float wrapped in go_float, and anything else itself.
template<typename T>
auto go_value(const T& v) -> decltype(auto) {
  if constexpr (std::floating_point<T>) {
    return go_float<T>{v};
  } else {
    return (v);
  }
}

} // namespace eo::detail

namespace fmt {

// Go's %v of a float, which is strconv.FormatFloat(v, 'g', -1, bits): the shortest decimal that reads
// back as the same value, in exponent form when the exponent is below -4 or at least 6, as in 1e+06,
// and +Inf, -Inf, and NaN.
template<std::floating_point F>
struct formatter<eo::detail::go_float<F>> {
  constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    return ctx.end();
  }
  template<typename FormatContext>
  auto format(const eo::detail::go_float<F>& f, FormatContext& ctx) const {
    auto v = f.value;
    if (std::isnan(v)) {
      return fmt::format_to(ctx.out(), "NaN");
    }
    if (std::isinf(v)) {
      return fmt::format_to(ctx.out(), "{}", v > 0 ? "+Inf" : "-Inf");
    }
    char buf[64];
    // The shortest scientific form, such as 1.234567e+06 or 1e-05, has Go's digits and Go's
    // exponent of at least two digits.
    auto scientific = std::to_chars(buf, buf + sizeof(buf), v, std::chars_format::scientific);
    auto text = std::string_view(buf, scientific.ptr);
    auto e = text.find('e');
    auto digits = text.substr(e + 2);
    int exponent = 0;
    std::from_chars(digits.data(), digits.data() + digits.size(), exponent);
    if (text[e + 1] == '-') {
      exponent = -exponent;
    }
    if (exponent < -4 || exponent >= 6) {
      return fmt::format_to(ctx.out(), "{}", text);
    }
    auto fixed = std::to_chars(buf, buf + sizeof(buf), v, std::chars_format::fixed);
    return fmt::format_to(ctx.out(), "{}", std::string_view(buf, fixed.ptr));
  }
};

} // namespace fmt
