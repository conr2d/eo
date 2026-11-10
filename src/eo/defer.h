// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once

#include <exception>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace eo::detail {

// The panic that the deferred calls now running may recover. It is set only while a defer stack runs
// its calls because of a panic, and cleared while one runs them at a normal return, so recover() in
// any other deferred call returns nil. Deferred calls run synchronously on one thread, so a
// thread-local is enough.
struct panic_state {
  std::exception_ptr panic;
  bool recovered = false;
};

inline thread_local panic_state* current_panic = nullptr;

class panic_scope {
public:
  explicit panic_scope(panic_state* state): saved(current_panic) {
    current_panic = state;
  }
  panic_scope(const panic_scope&) = delete;
  auto operator=(const panic_scope&) -> panic_scope& = delete;
  ~panic_scope() {
    current_panic = saved;
  }

private:
  panic_state* saved;
};

class defer_stack {
  struct call_base {
    virtual ~call_base() = default;
    virtual void invoke() = 0;
  };

  template<typename F>
  struct call final : call_base {
    explicit call(F function): function(std::move(function)) {}

    void invoke() override {
      std::invoke(function);
    }

    F function;
  };

public:
  defer_stack() = default;
  defer_stack(const defer_stack&) = delete;
  defer_stack(defer_stack&&) = delete;
  auto operator=(const defer_stack&) -> defer_stack& = delete;
  auto operator=(defer_stack&&) -> defer_stack& = delete;

  template<typename F>
  void push(F&& function) {
    using function_type = std::decay_t<F>;
    calls.emplace_back(std::make_unique<call<function_type>>(function_type(std::forward<F>(function))));
  }

  // A normal return. If a deferred call panics, the exception leaves run(), and the function's catch
  // handler runs the remaining calls with run_panicking().
  void run() {
    auto scope = panic_scope{nullptr};
    while (!calls.empty()) {
      auto deferred = std::move(calls.back());
      calls.pop_back();
      deferred->invoke();
    }
  }

  // A panic. Runs every remaining call, in LIFO order, with the panic recoverable. A deferred call
  // that panics replaces the current panic, and the remaining calls still run. Returns normally if
  // the last panic was recovered; otherwise rethrows it.
  void run_panicking(std::exception_ptr panic) {
    auto state = panic_state{std::move(panic)};
    {
      auto scope = panic_scope{&state};
      while (!calls.empty()) {
        auto deferred = std::move(calls.back());
        calls.pop_back();
        try {
          deferred->invoke();
        } catch (...) {
          state.panic = std::current_exception();
          state.recovered = false;
        }
      }
    }
    if (!state.recovered) {
      std::rethrow_exception(state.panic);
    }
  }

private:
  std::vector<std::unique_ptr<call_base>> calls;
};
} // namespace eo::detail

#define eo_defer_scope ::eo::detail::defer_stack _eo_defer_stack
#define eo_defer(...) _eo_defer_stack.push(__VA_ARGS__)
#define eo_defer_run _eo_defer_stack.run()
#define eo_defer_panic _eo_defer_stack.run_panicking(std::current_exception())
