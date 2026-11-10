// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once
#include <eo/defer.h>

#include <fmt/format.h>

#include <any>
#include <cstddef>
#include <exception>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace eo {

// The exception that carries a Go panic. what() is the report Go prints, "panic: <message>". It
// derives from std::runtime_error so existing handlers of runtime panics keep working.
class Panic : public std::runtime_error, public detail::counted_panic {
public:
  Panic(std::any value, std::string message)
    : std::runtime_error("panic: " + message), value_(std::move(value)), message_(std::move(message)) {}

  auto value() const -> const std::any& {
    return value_;
  }

  // The panic value as Go prints it, without the "panic: " prefix.
  auto message() const -> const std::string& {
    return message_;
  }

private:
  std::any value_;
  std::string message_;
};

// The result of Go's recover(): nil, or the value of the panic it stopped.
class Recovered {
public:
  Recovered() = default;

  explicit Recovered(std::exception_ptr panic): panic_(std::move(panic)) {
    try {
      std::rethrow_exception(panic_);
    } catch (const Panic& p) {
      value_ = p.value();
      message_ = p.message();
    } catch (const std::exception& e) {
      message_ = e.what();
    } catch (...) {
      message_ = "unknown panic";
    }
  }

  friend auto operator==(const Recovered& r, std::nullptr_t) -> bool {
    return !r.panic_;
  }

  explicit operator bool() const {
    return static_cast<bool>(panic_);
  }

  // The Go panic value. Empty for a panic raised by a C++ exception other than Panic.
  auto value() const -> const std::any& {
    return value_;
  }

  auto message() const -> const std::string& {
    return message_;
  }

  auto exception() const -> std::exception_ptr {
    return panic_;
  }

private:
  std::exception_ptr panic_;
  std::any value_;
  std::string message_;
};

namespace detail {

  template<typename T>
  auto panic_message(const T& value) -> std::string {
    if constexpr (std::is_convertible_v<const T&, std::string_view>) {
      return std::string{std::string_view{value}};
    } else if constexpr (fmt::is_formattable<T>::value) {
      return fmt::format("{}", value);
    } else {
      return "(unprintable panic value)";
    }
  }

  // Go 1.21 and later: panic(nil) raises a *runtime.PanicNilError.
  [[noreturn]] inline void panic_nil() {
    throw Panic{std::any{}, "panic called with nil argument"};
  }

} // namespace detail

// Go's panic(v). A string literal is stored as std::string, the C++ form of a Go string.
template<typename T>
[[noreturn]] void panic(T&& value) {
  using value_type = std::decay_t<T>;
  if constexpr (std::is_same_v<value_type, std::nullptr_t>) {
    detail::panic_nil();
  } else if constexpr (std::is_same_v<value_type, Recovered>) {
    // panic(r) starts a new panic with the recovered value, as in Go; panic of a nil recover() is
    // panic(nil). It never rethrows the recovered exception object, which other goroutines may also
    // be raising again.
    if (value == nullptr) {
      detail::panic_nil();
    }
    throw Panic{value.value(), value.message()};
  } else if constexpr (std::is_convertible_v<value_type, std::string_view> &&
    !std::is_same_v<value_type, std::string>) {
    auto s = std::string{std::string_view{value}};
    auto message = s;
    throw Panic{std::any{std::move(s)}, std::move(message)};
  } else {
    auto message = detail::panic_message(value);
    throw Panic{std::any{value_type(std::forward<T>(value))}, std::move(message)};
  }
}

// Go's recover(). Returns the current panic and stops it, but only in a deferred call that runs
// because of that panic. Anywhere else, and after the panic was recovered, returns nil.
inline auto recover() -> Recovered {
  auto* state = detail::current_panic;
  if (!state || state->recovered || !state->panic) {
    return {};
  }
  state->recovered = true;
  return Recovered{state->panic};
}

} // namespace eo

template<>
struct fmt::formatter<eo::Recovered> : fmt::formatter<std::string> {
  auto format(const eo::Recovered& r, format_context& ctx) const -> decltype(ctx.out()) {
    return fmt::formatter<std::string>::format(r == nullptr ? std::string{"<nil>"} : r.message(), ctx);
  }
};
