// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once
#include <eo/core.h>

#include <fmt/format.h>
#include <cstdio>
#include <map>
#include <utility>

namespace fmt {
namespace detail {

  template<typename... T>
  void print_line(std::FILE* f, T&&... args) {
    bool first = true;
    auto print_arg = [&](auto&& arg) {
      if (!first) {
        fmt::print(f, " ");
      }
      fmt::print(f, "{}", std::forward<decltype(arg)>(arg));
      first = false;
    };
    (print_arg(std::forward<T>(args)), ...);
    fmt::print(f, "\n");
  }

} // namespace detail

template<typename... T>
void println(T&&... args) {
  detail::print_line(stdout, std::forward<T>(args)...);
}

template<typename... T>
void fprintln(std::FILE* f, T&&... args) {
  detail::print_line(f, std::forward<T>(args)...);
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
    return fmt::format_to(ctx.out(), "map[{}]", fmt::join(p, " "));
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
