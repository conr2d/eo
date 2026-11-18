// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once
#include <eo/core.h>
#include <eo/float.h>
#include <eo/interface.h>

#include <boost/pfr.hpp>
#include <fmt/format.h>
#include <array>
#include <concepts>
#include <cstdio>
#include <iterator>
#include <map>
#include <string>
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

// A value as Go's %v prints it, including inside slices, maps, and structs: an interface by its
// dynamic value, and a value whose method set has Error or String by calling it, as Go's
// handleMethods does. `top` is true for an operand of Println.
template<typename T>
struct printed {
  const T& value;
  bool top;
};

template<typename T>
auto go_print(const T& value, bool top = false) -> printed<T> {
  return {value, top};
}

// The method named `name` with signature `string()` in T's method set. It must exist.
template<typename T>
auto string_method(std::size_t position) -> const entry<std::string()>* {
  return static_cast<const entry<std::string()>*>(method_set<T>::entries[position]);
}

// Calls an Error or String method for fmt. A panic in it prints as Go's fmt prints it, and as
// <nil> for a nil pointer receiver.
template<typename T>
auto call_string_method(const T& value, const entry<std::string()>* method) -> std::string {
  try {
    return method->fn(&value);
  } catch (const Panic& p) {
    if constexpr (is_ptr<T>) {
      if (value == nullptr) {
        return "<nil>";
      }
    }
    return "%!v(PANIC=" + std::string{method->name} + " method: " + p.message() + ")";
  }
}

template<typename T>
auto print_value(const T& value, bool top) -> std::string {
  return fmt::format("{}", go_print(value, top));
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
      fmt::format_to(std::back_inserter(buf), "{}", eo::detail::go_print(arg, true));
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

// Go's fmt.Stringer.
struct Stringer : eo::interface<Stringer> {
  using interface::interface;
  static constexpr std::string_view _eo_name = "fmt.Stringer";
  auto String() const -> std::string;
  using _eo_methods = eo::methods<eo::method<&Stringer::String, "String">>;
};

inline auto Stringer::String() const -> std::string {
  return _eo_call<&Stringer::String>();
}

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
    return fmt::format_to(ctx.out(), "{}:{}", eo::detail::go_print(p.first), eo::detail::go_print(p.second));
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
      out = fmt::format_to(out, "{}", eo::detail::go_print(element));
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
      out = fmt::format_to(out, "{}", eo::detail::go_print(field));
      first = false;
    });
    return fmt::format_to(out, "}}");
  }
};

// Go's %v of a pointer inside another value: <nil>, or its address. A pointer operand of Println to
// a struct or slice prints its target instead (see formatter<printed<T>>).
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

template<typename T>
struct formatter<eo::detail::printed<T>> {
  constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    return ctx.end();
  }
  template<typename FormatContext>
  auto format(const eo::detail::printed<T>& p, FormatContext& ctx) const {
    const auto& value = p.value;
    if constexpr (eo::detail::interface_type<T>) {
      if (value == nullptr) {
        return fmt::format_to(ctx.out(), "<nil>");
      }
      return fmt::format_to(ctx.out(), "{}", value._eo_type->print(value._eo_value.get(), p.top));
    } else {
      // Go prefers Error to String.
      constexpr auto error = eo::detail::method_position<T, std::string()>("Error");
      constexpr auto string = eo::detail::method_position<T, std::string()>("String");
      if constexpr (error != eo::detail::no_method || string != eo::detail::no_method) {
        constexpr auto position = error != eo::detail::no_method ? error : string;
        return fmt::format_to(ctx.out(),
          "{}",
          eo::detail::call_string_method(value, eo::detail::string_method<T>(position)));
      } else if constexpr (eo::detail::is_pointer_to_composite<T>) {
        // Go's fmt prints an operand of Println that points to a struct, slice, or map as & followed
        // by its target, and any other pointer as <nil> or its address.
        if (p.top && value != nullptr) {
          return fmt::format_to(ctx.out(), "&{}", eo::detail::go_print(*value));
        }
        return fmt::format_to(ctx.out(), "{}", value);
      } else {
        return fmt::format_to(ctx.out(), "{}", eo::detail::go_value(value));
      }
    }
  }
};

// An interface value prints its dynamic value.
template<eo::detail::interface_type T>
struct formatter<T> : formatter<eo::detail::printed<T>> {
  template<typename FormatContext>
  auto format(const T& value, FormatContext& ctx) const {
    return formatter<eo::detail::printed<T>>::format(eo::detail::go_print(value), ctx);
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
