// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once

#include <atomic>
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

// The number of panics in flight, like Go's runningPanicDefers. When eo_main returns, the runtime
// waits briefly while it is not zero, because such a panic may still end the program with status 2.
// An eo::Panic is counted from the first frame that runs deferred calls for it until it is recovered
// or replaced by another panic; an unrecovered one ends the program. Any other exception is counted
// only while one frame runs its deferred calls.
inline std::atomic<int> running_panic_defers{0};

// Base of eo::Panic that remembers whether the panic is counted in running_panic_defers.
// A panic that leaves through a C++ handler outside any drain, such as eo::invoke, is uncounted when
// the exception object is destroyed. A copy starts uncounted.
struct counted_panic {
  counted_panic() = default;
  counted_panic(const counted_panic&) noexcept {}
  auto operator=(const counted_panic&) noexcept -> counted_panic& {
    return *this;
  }
  ~counted_panic() {
    if (counted) {
      running_panic_defers.fetch_sub(1);
    }
  }

  mutable bool counted = false;
};

inline auto find_counted_panic(const std::exception_ptr& panic) -> const counted_panic* {
  try {
    std::rethrow_exception(panic);
  } catch (const counted_panic& p) {
    return &p;
  } catch (...) {
    return nullptr;
  }
}

// Counts a panic when a frame starts running deferred calls for it.
inline void begin_panic(const std::exception_ptr& panic) {
  if (auto* p = find_counted_panic(panic)) {
    if (p->counted) {
      return;
    }
    p->counted = true;
  }
  running_panic_defers.fetch_add(1);
}

// Uncounts a panic that was recovered or replaced, or any non-Panic exception when its frame is done.
inline void end_panic(const std::exception_ptr& panic) {
  if (auto* p = find_counted_panic(panic)) {
    if (!p->counted) {
      return;
    }
    p->counted = false;
  }
  running_panic_defers.fetch_sub(1);
}

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
    begin_panic(state.panic);
    {
      auto scope = panic_scope{&state};
      while (!calls.empty()) {
        auto deferred = std::move(calls.back());
        calls.pop_back();
        try {
          deferred->invoke();
        } catch (...) {
          end_panic(state.panic);
          state.panic = std::current_exception();
          state.recovered = false;
          begin_panic(state.panic);
        }
      }
    }
    if (state.recovered || !find_counted_panic(state.panic)) {
      end_panic(state.panic);
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
