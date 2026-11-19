// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once
#include <eo/core.h>
#include <eo/errors.h>
#include <eo/print.h>

#include <fmt/format.h>

#include <array>
#include <cstddef>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

namespace fmt {

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

namespace detail {

  // The operands are taken by value, as Go passes them. A reference would refer into a slice's backing
  // array, which another operand's call can free before the text is formatted.

  struct println_fn {
    template<typename... T>
    void operator()(T... args) const {
      eo::detail::write_text(stdout, eo::detail::sprint(true, args...));
    }
  };

  struct fprintln_fn {
    template<typename... T>
    void operator()(std::FILE* f, T... args) const {
      eo::detail::write_text(f, eo::detail::sprint(true, args...));
    }
  };

  struct sprint_fn {
    template<typename... T>
    auto operator()(T... args) const -> std::string {
      return eo::detail::sprint(false, args...);
    }
  };

  struct sprintln_fn {
    template<typename... T>
    auto operator()(T... args) const -> std::string {
      return eo::detail::sprint(true, args...);
    }
  };

  struct sprintf_fn {
    template<typename... T>
    auto operator()(const std::string& format, T... args) const -> std::string {
      return eo::detail::sprintf(format, args...);
    }
  };

  struct printf_fn {
    template<typename... T>
    void operator()(const std::string& format, T... args) const {
      eo::detail::write_text(stdout, eo::detail::sprintf(format, args...));
    }
  };

  struct fprintf_fn {
    template<typename... T>
    void operator()(std::FILE* f, const std::string& format, T... args) const {
      eo::detail::write_text(f, eo::detail::sprintf(format, args...));
    }
  };

} // namespace detail

// Function objects rather than function templates, so that a call with the results of another call,
// such as Go's fmt.Println(f()), translates to std::apply(fmt::Println, f()).
inline constexpr detail::println_fn Println{};
inline constexpr detail::fprintln_fn Fprintln{};
inline constexpr detail::sprint_fn Sprint{};
inline constexpr detail::sprintln_fn Sprintln{};
inline constexpr detail::sprintf_fn Sprintf{};
inline constexpr detail::printf_fn Printf{};
inline constexpr detail::fprintf_fn Fprintf{};

// fmt's formatters for Go values print them as Go's %v does inside another value.

template<typename T>
struct formatter<eo::detail::printed<T>> {
  constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    return ctx.end();
  }
  template<typename FormatContext>
  auto format(const eo::detail::printed<T>& p, FormatContext& ctx) const {
    auto printer = eo::detail::printer{};
    eo::detail::print_arg(printer, p.value, 'v', p.top ? 0 : 1);
    return fmt::format_to(ctx.out(), "{}", printer.buf);
  }
};

template<typename K, typename V>
struct formatter<eo::map<K, V>> : formatter<eo::detail::printed<eo::map<K, V>>> {
  template<typename FormatContext>
  auto format(const eo::map<K, V>& value, FormatContext& ctx) const {
    return formatter<eo::detail::printed<eo::map<K, V>>>::format(eo::detail::go_print(value), ctx);
  }
};

template<typename T>
struct formatter<eo::slice<T>> : formatter<eo::detail::printed<eo::slice<T>>> {
  template<typename FormatContext>
  auto format(const eo::slice<T>& value, FormatContext& ctx) const {
    return formatter<eo::detail::printed<eo::slice<T>>>::format(eo::detail::go_print(value), ctx);
  }
};

template<eo::detail::go_struct T>
struct formatter<T> : formatter<eo::detail::printed<T>> {
  template<typename FormatContext>
  auto format(const T& value, FormatContext& ctx) const {
    return formatter<eo::detail::printed<T>>::format(eo::detail::go_print(value), ctx);
  }
};

template<typename T>
struct formatter<eo::ptr<T>> : formatter<eo::detail::printed<eo::ptr<T>>> {
  template<typename FormatContext>
  auto format(const eo::ptr<T>& value, FormatContext& ctx) const {
    return formatter<eo::detail::printed<eo::ptr<T>>>::format(eo::detail::go_print(value), ctx);
  }
};

template<eo::detail::interface_type T>
struct formatter<T> : formatter<eo::detail::printed<T>> {
  template<typename FormatContext>
  auto format(const T& value, FormatContext& ctx) const {
    return formatter<eo::detail::printed<T>>::format(eo::detail::go_print(value), ctx);
  }
};

// *fmt.wrapError, which Errorf returns for one %w.
struct wrapError {
  static constexpr std::string_view _eo_name = "fmt.wrapError";

  std::string msg;
  eo::error err;

  auto operator==(const wrapError&) const -> bool = default;

  static auto Error(eo::ptr<wrapError> e) -> std::string {
    return e->msg;
  }

  static auto Unwrap(eo::ptr<wrapError> e) -> eo::error {
    return e->err;
  }

  using _eo_methods = eo::methods<eo::method<&wrapError::Error, "Error">, eo::method<&wrapError::Unwrap, "Unwrap">>;
};

// *fmt.wrapErrors, which Errorf returns for several %w.
struct wrapErrors {
  static constexpr std::string_view _eo_name = "fmt.wrapErrors";

  std::string msg;
  eo::slice<eo::error> errs;

  static auto Error(eo::ptr<wrapErrors> e) -> std::string {
    return e->msg;
  }

  static auto Unwrap(eo::ptr<wrapErrors> e) -> eo::slice<eo::error> {
    return e->errs;
  }

  using _eo_methods = eo::methods<eo::method<&wrapErrors::Error, "Error">, eo::method<&wrapErrors::Unwrap, "Unwrap">>;
};

namespace detail {

  struct errorf_fn {
    template<typename... T>
    auto operator()(const std::string& format, T... args) const -> eo::error {
      auto p = eo::detail::printer{.wrap_errs = true};
      const auto operands = std::array<eo::detail::operand, sizeof...(T)>{eo::detail::make_operand(args)...};
      auto wrapped = std::vector<std::size_t>{};
      eo::detail::print_format(p, format, operands, &wrapped);
      if (wrapped.empty()) {
        return eo::errors::New(std::move(p.buf));
      }
      auto error_of = [&](std::size_t i) { return operands[i].as_error(operands[i].value); };
      if (wrapped.size() == 1) {
        return eo::New<wrapError>(wrapError{std::move(p.buf), error_of(wrapped[0])});
      }
      auto errs = std::vector<eo::error>{};
      for (auto i : wrapped) {
        if (auto e = error_of(i); e != nullptr) {
          errs.push_back(std::move(e));
        }
      }
      auto s = eo::slice<eo::error>{};
      if (!errs.empty()) {
        s = eo::slice<eo::error>::allocate(static_cast<int64_t>(errs.size()), static_cast<int64_t>(errs.size()));
        std::move(errs.begin(), errs.end(), s.begin());
      }
      return eo::New<wrapErrors>(wrapErrors{std::move(p.buf), std::move(s)});
    }
  };

} // namespace detail

// fmt.Errorf: an error whose message is formatted as Sprintf formats it. Each %w operand that is an
// error is wrapped, so errors.Is, errors.As, and errors.Unwrap see it.
inline constexpr detail::errorf_fn Errorf{};

} // namespace fmt

namespace eo {

namespace fmt = ::fmt;

} // namespace eo
