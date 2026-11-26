// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once
#include <eo/core.h>
#include <eo/fn.h>

#include <atomic>
#include <exception>
#include <memory>
#include <mutex>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

namespace eo::sync {

namespace detail {

  // Whether a call returns a coroutine, an awaitable that Once would drop without running.
  template<typename T>
  inline constexpr bool is_coroutine_result = false;

  template<typename T, typename E>
  inline constexpr bool is_coroutine_result<boost::asio::awaitable<T, E>> = true;

  template<typename F>
  concept plain_function = !is_coroutine_result<std::invoke_result_t<F&>>;

} // namespace detail

// Go's sync.Once. Do calls f only on the first call of any goroutine, and the other calls return
// after that call of f has returned. If f panics, Do considers it returned, and later calls do not
// call f. f is a function that is not a coroutine, and a coroutine does not compile, so a goroutine
// that waits for another's call of f blocks its worker thread only until f returns.
struct Once {
  Once() = default;
  Once(const Once&) = delete;
  auto operator=(const Once&) -> Once& = delete;

  template<typename F>
    requires detail::plain_function<F>
  void Do(F&& f) {
    if (done_.load(std::memory_order_acquire)) {
      return;
    }
    std::lock_guard lock(m_);
    if (done_.load(std::memory_order_relaxed)) {
      return;
    }
    // Set even when f panics, as Go's deferred store does.
    struct mark_done {
      std::atomic<bool>& done;
      ~mark_done() {
        done.store(true, std::memory_order_release);
      }
    } mark{done_};
    std::forward<F>(f)();
  }

private:
  std::mutex m_;
  std::atomic<bool> done_{false};
};

namespace detail {

  // The state that the functions OnceFunc, OnceValue, and OnceValues return share: Go's struct of
  // f, the Once, whether f returned, its panic, and its results. A panic is kept by its value, so
  // that each later call panics with a new panic of the same value, as Go's panic(d.p) does.
  template<typename R>
  struct once_state {
    std::mutex m;
    bool done = false;
    bool valid = false;
    std::optional<any> panic_value;
    std::exception_ptr other;
    std::optional<R> result;

    template<typename F>
    auto call(F& f) -> R& {
      {
        std::lock_guard lock(m);
        if (!done) {
          done = true;
          try {
            if constexpr (std::is_void_v<decltype(f())>) {
              f();
              result.emplace();
            } else {
              result.emplace(f());
            }
            valid = true;
          } catch (const Panic& p) {
            panic_value = p.value();
          } catch (...) {
            other = std::current_exception();
          }
          // Do not keep f alive after calling it.
          f = F{};
        }
      }
      if (!valid) {
        if (panic_value) {
          throw Panic(*panic_value);
        }
        std::rethrow_exception(other);
      }
      return *result;
    }
  };

} // namespace detail

// Go's sync.OnceFunc: a function that calls f only the first time it is called, and returns or
// panics as f did on every call.
template<detail::plain_function F>
auto OnceFunc(F f) -> fn<void()> {
  auto d = std::make_shared<std::pair<detail::once_state<std::monostate>, fn<void()>>>();
  d->second = std::move(f);
  return [d]() { d->first.call(d->second); };
}

// Go's sync.OnceValue: a function that calls f only the first time it is called, and returns its
// result, or panics with its panic, on every call.
template<detail::plain_function F>
auto OnceValue(F f) -> fn<std::invoke_result_t<F&>()> {
  using R = std::invoke_result_t<F&>;
  auto d = std::make_shared<std::pair<detail::once_state<R>, fn<R()>>>();
  d->second = std::move(f);
  return [d]() -> R { return d->first.call(d->second); };
}

// Go's sync.OnceValues, for a function with two results, which are a std::tuple.
template<detail::plain_function F>
auto OnceValues(F f) -> fn<std::invoke_result_t<F&>()> {
  return OnceValue(std::move(f));
}

} // namespace eo::sync
