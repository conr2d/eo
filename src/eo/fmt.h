// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once
#include <eo/core.h>

#include <boost/pfr.hpp>
#include <fmt/format.h>
#include <array>
#include <cstdio>
#include <iterator>
#include <map>
#include <type_traits>
#include <utility>

namespace eo::detail {

template<typename T>
inline constexpr bool is_std_array = false;

template<typename T, std::size_t N>
inline constexpr bool is_std_array<std::array<T, N>> = true;

// A translated Go struct: a class aggregate, which Boost.PFR can visit field by field.
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
      fmt::format_to(std::back_inserter(buf), "{}", std::forward<decltype(arg)>(arg));
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
    return fmt::format_to(ctx.out(), "{}:{}", p.first, p.second);
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
      out = fmt::format_to(out, "{}", element);
      first = false;
    }
    return fmt::format_to(out, "]");
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
      out = fmt::format_to(out, "{}", field);
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
