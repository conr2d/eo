// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once
#include <eo/core.h>

#include <boost/pfr.hpp>
#include <fmt/format.h>
#include <array>
#include <charconv>
#include <cmath>
#include <concepts>
#include <cstdio>
#include <iterator>
#include <map>
#include <string_view>
#include <type_traits>
#include <utility>

namespace eo::detail {

template<typename T>
inline constexpr bool is_std_array = false;

template<typename T, std::size_t N>
inline constexpr bool is_std_array<std::array<T, N>> = true;

// A translated Go struct: a class aggregate, which Boost.PFR can visit field by field. This also
// matches other aggregates, so it is ambiguous with fmt's own formatters for aggregate types, such as
// std::tm in <fmt/chrono.h>, std::monostate in <fmt/std.h>, and fmt::streamed. Eo uses none of them
// for Go values.
template<typename T>
concept go_struct = std::is_class_v<T> && std::is_aggregate_v<T> && !is_std_array<T>;

template<typename T>
inline constexpr bool is_composite = go_struct<T>;

template<typename T>
inline constexpr bool is_composite<eo::slice<T>> = true;

template<typename K, typename V>
inline constexpr bool is_composite<std::map<K, V>> = true;

template<typename T>
inline constexpr bool is_pointer_to_composite = false;

// Go's fmt prints an operand that points to a struct, slice, or map as & followed by its target.
template<typename T>
inline constexpr bool is_pointer_to_composite<eo::ptr<T>> = is_composite<T>;

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
namespace detail {

  // Go's fmt.Println writes each call to the unbuffered os.Stdout with a single write. Format the
  // whole line first, then write and flush it, so output is not held in a stdio buffer and lines
  // from concurrent goroutines do not interleave.
  template<typename... T>
  void print_line(std::FILE* f, T&&... args) {
    auto buf = fmt::memory_buffer{};
    bool first = true;
    auto print_arg = [&](auto&& arg) {
      if (!first) {
        buf.push_back(' ');
      }
      if constexpr (eo::detail::is_pointer_to_composite<std::remove_cvref_t<decltype(arg)>>) {
        if (arg != nullptr) {
          fmt::format_to(std::back_inserter(buf), "&{}", *arg);
          first = false;
          return;
        }
      }
      fmt::format_to(std::back_inserter(buf), "{}", eo::detail::go_value(arg));
      first = false;
    };
    (print_arg(std::forward<T>(args)), ...);
    buf.push_back('\n');
    std::fwrite(buf.data(), 1, buf.size(), f);
    // runtime::exit skips stdout while another thread holds its lock, so this flush is what keeps a
    // goroutine's last line before os::Exit.
    std::fflush(f);
  }

} // namespace detail

// The operands are taken by value, as Go passes them. A reference would refer into a slice's backing
// array, which another operand's call can free before the line is formatted.
template<typename... T>
void Println(T... args) {
  detail::print_line(stdout, std::move(args)...);
}

template<typename... T>
void Fprintln(std::FILE* f, T... args) {
  detail::print_line(f, std::move(args)...);
}

template<typename K, typename V>
struct formatter<std::pair<K, V>> {
  constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    return ctx.end();
  }
  template<typename FormatContext>
  auto format(const std::pair<K, V>& p, FormatContext& ctx) const {
    return fmt::format_to(ctx.out(), "{}:{}", eo::detail::go_value(p.first), eo::detail::go_value(p.second));
  }
};

template<typename K, typename V>
struct formatter<std::map<K, V>> {
  constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    return ctx.end();
  }
  template<typename FormatContext>
  auto format(const std::map<K, V>& p, FormatContext& ctx) const {
    // fmt::join needs <fmt/ranges.h> since fmt 11, whose tuple formatter conflicts with formatter<std::pair> above.
    auto out = fmt::format_to(ctx.out(), "map[");
    bool first = true;
    for (const auto& entry : p) {
      if (!first) {
        out = fmt::format_to(out, " ");
      }
      out = fmt::format_to(out, "{}", entry);
      first = false;
    }
    return fmt::format_to(out, "]");
  }
};

// Go's %v of a slice: the elements in brackets, separated by spaces, and [] for nil.
template<typename T>
struct formatter<eo::slice<T>> {
  constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    return ctx.end();
  }
  template<typename FormatContext>
  auto format(const eo::slice<T>& s, FormatContext& ctx) const {
    auto out = fmt::format_to(ctx.out(), "[");
    bool first = true;
    for (const auto& element : s) {
      if (!first) {
        out = fmt::format_to(out, " ");
      }
      out = fmt::format_to(out, "{}", eo::detail::go_value(element));
      first = false;
    }
    return fmt::format_to(out, "]");
  }
};

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

// Go's %v of a struct: its fields in braces, separated by spaces, each printed as %v prints it.
template<eo::detail::go_struct T>
struct formatter<T> {
  constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    return ctx.end();
  }
  template<typename FormatContext>
  auto format(const T& s, FormatContext& ctx) const {
    auto out = fmt::format_to(ctx.out(), "{{");
    bool first = true;
    boost::pfr::for_each_field(s, [&](const auto& field) {
      if (!first) {
        out = fmt::format_to(out, " ");
      }
      out = fmt::format_to(out, "{}", eo::detail::go_value(field));
      first = false;
    });
    return fmt::format_to(out, "}}");
  }
};

// Go's %v of a pointer inside another value: <nil>, or its address. A pointer operand of Println to
// a struct or slice prints its target instead (see print_line).
template<typename T>
struct formatter<eo::ptr<T>> {
  constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    return ctx.end();
  }
  template<typename FormatContext>
  auto format(const eo::ptr<T>& p, FormatContext& ctx) const {
    if (p == nullptr) {
      return fmt::format_to(ctx.out(), "<nil>");
    }
    return fmt::format_to(ctx.out(), "{}", fmt::ptr(p.get()));
  }
};

template<typename Clock>
struct formatter<std::chrono::time_point<Clock>> {
  constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    return ctx.end();
  }
  template<typename FormatContext>
  auto format(const std::chrono::time_point<Clock>& p, FormatContext& ctx) const {
    auto buffer = std::vector<char>(256);
    auto t = Clock::to_time_t(p);
    auto size = std::strftime(buffer.data(), buffer.size(), "%F %T %z %Z", std::localtime(&t));
    return fmt::format_to(ctx.out(), "{}", std::string_view(buffer.data(), size));
  }
};

} // namespace fmt

namespace eo {

namespace fmt = ::fmt;

} // namespace eo
