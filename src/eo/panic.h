// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once
#include <eo/defer.h>
#include <eo/float.h>
#include <eo/interface.h>

#include <cstddef>
#include <exception>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace eo {

namespace detail {

  // %v of a panic value, as Go's fmt prints it, from <eo/fmt.h>.
  inline auto print_panic_value(const any& value) -> std::string;

} // namespace detail

// The exception that carries a Go panic, whose value is an eo::any, as Go's panic takes. what() is
// the report Go prints, "panic: <message>".
class Panic : public std::exception, public detail::counted_panic {
public:
  explicit Panic(any value): state_(std::make_shared<state>(std::move(value))) {}

  auto value() const -> const any& {
    return state_->value;
  }

  // The panic value as Go's %v prints it, calling its Error or String method, without the "panic: "
  // prefix. It is formatted when first asked for, as Go formats a panic value only to print it.
  auto message() const -> const std::string& {
    format();
    return state_->message;
  }

  auto what() const noexcept -> const char* override {
    format();
    return state_->report.c_str();
  }

private:
  struct state {
    explicit state(any v): value(std::move(v)) {}

    any value;
    std::once_flag once;
    std::string message;
    std::string report;
  };

  void format() const noexcept {
    std::call_once(state_->once, [this] {
      try {
        state_->message = detail::print_panic_value(state_->value);
      } catch (...) {
        state_->message = "(unprintable panic value)";
      }
      state_->report = "panic: " + state_->message;
    });
  }

  // Shared, because an exception object may be copied.
  std::shared_ptr<state> state_;
};

namespace runtime {

  // Go's runtime.Error. C++ cannot name a member like its class, so the type takes a trailing
  // underscore, and its Go name stays runtime.Error.
  struct Error_ : interface<Error_> {
    using interface::interface;
    static constexpr std::string_view _eo_name = "runtime.Error";

    auto Error() const -> std::string;
    void RuntimeError() const;

    using _eo_methods = methods<method<&Error_::Error, "Error">, method<&Error_::RuntimeError, "RuntimeError">>;
  };

  inline auto Error_::Error() const -> std::string {
    return _eo_call<&Error_::Error>();
  }

  inline void Error_::RuntimeError() const {
    _eo_call<&Error_::RuntimeError>();
  }

  // The values of Go's runtime panics. Each keeps the text that Go prints, rather than Go's fields.

  // runtime.errorString: a runtime error with a fixed message, such as a nil dereference.
  struct errorString {
    static constexpr std::string_view _eo_name = "runtime.errorString";

    std::string s;

    auto operator==(const errorString&) const -> bool = default;

    static auto Error(errorString e) -> std::string {
      return "runtime error: " + e.s;
    }
    static void RuntimeError(errorString) {}

    using _eo_methods =
      methods<method<&errorString::Error, "Error">, method<&errorString::RuntimeError, "RuntimeError">>;
  };

  // runtime.boundsError: an index or a slice bound out of range.
  struct boundsError {
    static constexpr std::string_view _eo_name = "runtime.boundsError";

    std::string s;

    auto operator==(const boundsError&) const -> bool = default;

    static auto Error(boundsError e) -> std::string {
      return "runtime error: " + e.s;
    }
    static void RuntimeError(boundsError) {}

    using _eo_methods =
      methods<method<&boundsError::Error, "Error">, method<&boundsError::RuntimeError, "RuntimeError">>;
  };

  // runtime.plainError: a runtime error without the "runtime error: " prefix, such as a close of a
  // closed channel.
  struct plainError {
    static constexpr std::string_view _eo_name = "runtime.plainError";

    std::string s;

    auto operator==(const plainError&) const -> bool = default;

    static auto Error(plainError e) -> std::string {
      return e.s;
    }
    static void RuntimeError(plainError) {}

    using _eo_methods = methods<method<&plainError::Error, "Error">, method<&plainError::RuntimeError, "RuntimeError">>;
  };

  // *runtime.TypeAssertionError and *runtime.PanicNilError are in <eo/ptr.h>.
  struct TypeAssertionError;
  struct PanicNilError;

} // namespace runtime

namespace detail {

  // The recovered value of a C++ exception other than eo::Panic: an error whose message is what().
  struct cpp_exception {
    static constexpr std::string_view _eo_name = "eo.cppException";

    std::string what;

    auto operator==(const cpp_exception&) const -> bool = default;

    static auto Error(cpp_exception e) -> std::string {
      return e.what;
    }

    using _eo_methods = methods<method<&cpp_exception::Error, "Error">>;
  };

  [[noreturn]] inline void panic_runtime_error(std::string message) {
    throw Panic{any{runtime::errorString{std::move(message)}}};
  }

  [[noreturn]] inline void panic_bounds_error(std::string message) {
    throw Panic{any{runtime::boundsError{std::move(message)}}};
  }

  [[noreturn]] inline void panic_plain_error(std::string message) {
    throw Panic{any{runtime::plainError{std::move(message)}}};
  }

  // Go 1.21 and later: panic(nil) raises a *runtime.PanicNilError. In <eo/ptr.h>.
  [[noreturn]] inline void panic_nil();

  // The value that recover() returns for a panic.
  inline auto panic_value(const std::exception_ptr& panic) -> any {
    try {
      std::rethrow_exception(panic);
    } catch (const Panic& p) {
      return p.value();
    } catch (const std::exception& e) {
      return cpp_exception{e.what()};
    } catch (...) {
      return cpp_exception{"unknown exception"};
    }
  }

} // namespace detail

// Go's panic(v). The value is converted to any, as in Go: a string literal becomes a Go string, an
// interface value passes its dynamic value, and a nil interface value is panic(nil).
template<typename T>
[[noreturn]] void panic(T&& value) {
  using value_type = std::decay_t<T>;
  if constexpr (std::is_same_v<value_type, std::nullptr_t>) {
    detail::panic_nil();
  } else if constexpr (detail::interface_type<value_type>) {
    // panic(r) with a recovered r starts a new panic with the same value, as in Go. It never rethrows
    // the recovered exception object, which other goroutines may also be raising again.
    if (value == nullptr) {
      detail::panic_nil();
    }
    throw Panic{any{value}};
  } else if constexpr (std::is_convertible_v<value_type, std::string_view> &&
    !std::is_same_v<value_type, std::string>) {
    throw Panic{any{std::string{std::string_view{value}}}};
  } else {
    throw Panic{any{value_type(std::forward<T>(value))}};
  }
}

// Go's recover(). Returns the current panic's value and stops the panic, but only in a deferred call
// that runs because of that panic. Anywhere else, and after the panic was recovered, returns nil.
inline auto recover() -> any {
  auto* state = detail::current_panic;
  if (!state || state->recovered || !state->panic) {
    return {};
  }
  state->recovered = true;
  return detail::panic_value(state->panic);
}

} // namespace eo

// The definitions of panic_nil and of the printing that Panic uses.
#include <eo/print.h>
#include <eo/ptr.h>
