// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once
#include <eo/go.h>
#include <eo/ptr.h>

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace eo {

// A Go function value: fn<R(A...)> for a function that does not block, and fn<func<R>(A...)> for one
// translated as a coroutine. The zero value is nil, and calling a nil fn panics as Go does. Copies
// share the target, as copies of a Go function value share its closure, and a call keeps the target
// alive until it returns, so assigning the variable that held it during the call is safe.
template<typename Sig>
class fn;

namespace detail {

  template<typename T>
  struct awaitable_traits {
    static constexpr bool value = false;
  };

  template<typename T, typename Executor>
  struct awaitable_traits<boost::asio::awaitable<T, Executor>> {
    static constexpr bool value = true;
    using result = T;
  };

  template<typename T>
  inline constexpr bool is_awaitable_v = awaitable_traits<std::remove_cvref_t<T>>::value;

  template<typename T>
  struct is_fn : std::false_type {};

  template<typename Sig>
  struct is_fn<fn<Sig>> : std::true_type {};

  // Whether a callable F can be the target of fn<R(A...)>. A plain target fits a plain signature
  // whose result it converts to. A coroutine target fits only a coroutine signature, so a coroutine
  // never becomes a plain call whose awaitable is discarded unrun. A plain target also fits a
  // coroutine signature, and is adapted to one.
  template<typename F, typename R, typename... A>
  consteval auto fn_target_fits() -> bool {
    if constexpr (!std::is_invocable_v<F&, A...>) {
      return false;
    } else {
      using Result = std::invoke_result_t<F&, A...>;
      if constexpr (is_awaitable_v<R>) {
        if constexpr (is_awaitable_v<Result>) {
          return std::is_convertible_v<Result, R>;
        } else {
          return std::is_invocable_r_v<typename awaitable_traits<R>::result, F&, A...>;
        }
      } else {
        return !is_awaitable_v<Result> && std::is_invocable_r_v<R, F&, A...>;
      }
    }
  }

  // The result of a plain target where a coroutine signature is expected. The target has already
  // run, in the same full expression as the co_await, so no Go program can tell it from a call that
  // runs when the awaitable is awaited.
  template<typename R>
  auto fn_ready() -> R {
    co_return;
  }

  template<typename R, typename T>
  auto fn_ready(T value) -> R {
    co_return value;
  }

  // Holds the target of a coroutine call until the coroutine finishes.
  template<typename R, typename C, typename... A>
  auto fn_keep(std::shared_ptr<C> self, A... a) -> R {
    if constexpr (std::is_void_v<typename awaitable_traits<R>::result>) {
      co_await self->call(std::move(a)...);
    } else {
      co_return co_await self->call(std::move(a)...);
    }
  }

} // namespace detail

template<typename R, typename... A>
class fn<R(A...)> {
  struct callable {
    callable() = default;
    callable(const callable&) = delete;
    auto operator=(const callable&) -> callable& = delete;
    virtual ~callable() = default;
    virtual auto call(A... a) -> R = 0;
  };

  template<typename F>
  struct target final : callable {
    explicit target(F f): f(std::move(f)) {}

    auto call(A... a) -> R override {
      if constexpr (detail::is_awaitable_v<R> && !detail::is_awaitable_v<std::invoke_result_t<F&, A...>>) {
        using T = typename detail::awaitable_traits<R>::result;
        if constexpr (std::is_void_v<T>) {
          std::invoke(f, std::move(a)...);
          return detail::fn_ready<R>();
        } else {
          return detail::fn_ready<R, T>(std::invoke(f, std::move(a)...));
        }
      } else {
        return std::invoke(f, std::move(a)...);
      }
    }

    F f;
  };

  std::shared_ptr<callable> target_;

  template<typename Sig>
  friend class fn;

public:
  fn() = default;
  fn(std::nullptr_t) {}

  template<typename F>
    requires(!detail::is_fn<std::remove_cvref_t<F>>::value && !std::is_same_v<std::remove_cvref_t<F>, std::nullptr_t> &&
      detail::fn_target_fits<std::decay_t<F>, R, A...>())
  fn(F&& f): target_(std::make_shared<target<std::decay_t<F>>>(std::forward<F>(f))) {}

  // A plain function value where a coroutine one of the same signature is expected, as Go assigns
  // only identical function types. A nil value stays nil.
  template<typename T>
    requires(detail::is_awaitable_v<R> && std::is_same_v<T, typename detail::awaitable_traits<R>::result>)
  fn(const fn<T(A...)>& f) {
    if (f.target_) {
      target_ = std::make_shared<target<fn<T(A...)>>>(f);
    }
  }

  auto operator()(A... a) const -> R
    requires(!detail::is_awaitable_v<R>)
  {
    if (!target_) {
      detail::panic_nil_dereference();
    }
    auto self = target_;
    return self->call(std::move(a)...);
  }

  // A coroutine call does nothing until it is awaited, so dropping its awaitable is diagnosed.
  [[nodiscard]] auto operator()(A... a) const -> R
    requires(detail::is_awaitable_v<R>)
  {
    if (!target_) {
      detail::panic_nil_dereference();
    }
    return detail::fn_keep<R>(target_, std::move(a)...);
  }

  // Go compares a function value only with nil.
  friend auto operator==(const fn& f, std::nullptr_t) -> bool {
    return !f.target_;
  }
};

// The receiver of a value method called through a method expression on a pointer type, as in
// (*T).M: the target of p, or Go's panic when p is nil.
template<typename T>
auto value_receiver(ptr<T> p, std::string_view method) -> T& {
  if (p == nullptr) {
    auto name = std::string_view{T::_eo_name};
    auto type = name.substr(name.rfind('.') + 1);
    detail::panic_plain_error("value method " + std::string{name} + "." + std::string{method} + " called using nil *" +
      std::string{type} + " pointer");
  }
  return *p;
}

} // namespace eo
