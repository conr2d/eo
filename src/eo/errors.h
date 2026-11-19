// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once
#include <eo/core.h>

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

// Go's errors package.

namespace eo::errors {

// *errors.errorString, which errors.New returns. Each call returns a new pointer, so two errors with
// the same text are not equal, as in Go.
struct errorString {
  static constexpr std::string_view _eo_name = "errors.errorString";

  std::string s;

  auto operator==(const errorString&) const -> bool = default;

  static auto Error(ptr<errorString> e) -> std::string {
    return e->s;
  }

  using _eo_methods = methods<method<&errorString::Error, "Error">>;
};

inline auto New(std::string text) -> error {
  return eo::New<errorString>(errorString{std::move(text)});
}

// *errors.joinError, which errors.Join returns.
struct joinError {
  static constexpr std::string_view _eo_name = "errors.joinError";

  slice<error> errs;

  static auto Error(ptr<joinError> e) -> std::string {
    // Join returns nil when every error is nil, so errs is never empty.
    auto text = std::string{};
    auto first = true;
    for (const auto& err : e->errs) {
      if (!first) {
        text += '\n';
      }
      text += err.Error();
      first = false;
    }
    return text;
  }

  static auto Unwrap(ptr<joinError> e) -> slice<error> {
    return e->errs;
  }

  using _eo_methods = methods<method<&joinError::Error, "Error">, method<&joinError::Unwrap, "Unwrap">>;
};

namespace detail {

  // The anonymous interfaces that Go's errors package asserts.

  struct wrapper : interface<wrapper> {
    using interface::interface;
    static constexpr std::string_view _eo_name = "interface { Unwrap() error }";
    auto Unwrap() const -> error;
    using _eo_methods = methods<method<&wrapper::Unwrap, "Unwrap">>;
  };

  inline auto wrapper::Unwrap() const -> error {
    return _eo_call<&wrapper::Unwrap>();
  }

  struct multi_wrapper : interface<multi_wrapper> {
    using interface::interface;
    static constexpr std::string_view _eo_name = "interface { Unwrap() []error }";
    auto Unwrap() const -> slice<error>;
    using _eo_methods = methods<method<&multi_wrapper::Unwrap, "Unwrap">>;
  };

  inline auto multi_wrapper::Unwrap() const -> slice<error> {
    return _eo_call<&multi_wrapper::Unwrap>();
  }

  struct is_matcher : interface<is_matcher> {
    using interface::interface;
    static constexpr std::string_view _eo_name = "interface { Is(error) bool }";
    auto Is(error target) const -> bool;
    using _eo_methods = methods<method<&is_matcher::Is, "Is">>;
  };

  inline auto is_matcher::Is(error target) const -> bool {
    return _eo_call<&is_matcher::Is>(std::move(target));
  }

  struct as_matcher : interface<as_matcher> {
    using interface::interface;
    static constexpr std::string_view _eo_name = "interface { As(interface {}) bool }";
    auto As(any target) const -> bool;
    using _eo_methods = methods<method<&as_matcher::As, "As">>;
  };

  inline auto as_matcher::As(any target) const -> bool {
    return _eo_call<&as_matcher::As>(std::move(target));
  }

  inline auto is(error err, const error& target, bool comparable) -> bool {
    for (;;) {
      if (comparable && err == target) {
        return true;
      }
      if (auto [x, ok] = type_assert_ok<is_matcher>(err); ok && x.Is(target)) {
        return true;
      }
      if (auto [x, ok] = type_assert_ok<wrapper>(err); ok) {
        err = x.Unwrap();
        if (err == nullptr) {
          return false;
        }
        continue;
      }
      if (auto [x, ok] = type_assert_ok<multi_wrapper>(err); ok) {
        for (const auto& e : x.Unwrap()) {
          if (is(e, target, comparable)) {
            return true;
          }
        }
      }
      return false;
    }
  }

  template<typename T>
  auto as(error err, const ptr<T>& target) -> bool {
    for (;;) {
      if (auto [v, ok] = type_assert_ok<T>(err); ok) {
        *target = std::move(v);
        return true;
      }
      if (auto [x, ok] = type_assert_ok<as_matcher>(err); ok && x.As(any{target})) {
        return true;
      }
      if (auto [x, ok] = type_assert_ok<wrapper>(err); ok) {
        err = x.Unwrap();
        if (err == nullptr) {
          return false;
        }
        continue;
      }
      if (auto [x, ok] = type_assert_ok<multi_wrapper>(err); ok) {
        for (const auto& e : x.Unwrap()) {
          if (e != nullptr && as(e, target)) {
            return true;
          }
        }
      }
      return false;
    }
  }

} // namespace detail

// errors.Unwrap: the result of err's Unwrap() error method, or nil. An Unwrap() []error method does
// not count, as in Go.
inline auto Unwrap(error err) -> error {
  auto [x, ok] = type_assert_ok<detail::wrapper>(err);
  if (!ok) {
    return nullptr;
  }
  return x.Unwrap();
}

// errors.Is: whether err or an error in its tree, visited depth first, equals target or has an
// Is(error) bool method that reports it.
inline auto Is(error err, error target) -> bool {
  if (err == nullptr || target == nullptr) {
    return err == target;
  }
  auto comparable = target._eo_type->equal != nullptr;
  return detail::is(std::move(err), target, comparable);
}

// errors.As(err, &target): stores the first error in err's tree that a T can hold into *target.
template<typename T>
auto As(error err, const ptr<T>& target) -> bool {
  static_assert(eo::detail::interface_type<T> ||
      eo::detail::method_position<T, std::string()>("Error") != eo::detail::no_method,
    "errors: *target must be interface or implement error");
  if (err == nullptr) {
    return false;
  }
  if (target == nullptr) {
    panic("errors: target must be a non-nil pointer");
  }
  return detail::as(std::move(err), target);
}

// errors.Join(errs...): an error that wraps the non-nil errors, or nil if there are none. The
// variadic parameter is a slice<error>, as for any Go variadic function.
inline auto Join(slice<error> errs) -> error {
  auto kept = std::vector<error>{};
  for (const auto& err : errs) {
    if (err != nullptr) {
      kept.push_back(err);
    }
  }
  if (kept.empty()) {
    return nullptr;
  }
  auto e = slice<error>::allocate(static_cast<int64_t>(kept.size()), static_cast<int64_t>(kept.size()));
  std::move(kept.begin(), kept.end(), e.begin());
  return eo::New<joinError>(joinError{std::move(e)});
}

} // namespace eo::errors
