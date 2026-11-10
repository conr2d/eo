// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once
#include <eo/core.h>

#include <fmt/format.h>
#include <cstdio>
#include <iterator>
#include <map>
#include <utility>

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

template<typename... T>
void Println(T&&... args) {
  detail::print_line(stdout, std::forward<T>(args)...);
}

template<typename... T>
void Fprintln(std::FILE* f, T&&... args) {
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
