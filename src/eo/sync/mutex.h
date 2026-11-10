// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once
#include <eo/runtime.h>
#include <eo/sync/waiter.h>

#include <deque>
#include <mutex>
#include <optional>
#include <utility>

namespace eo::sync {

// Go's sync.Mutex. Lock suspends only the calling goroutine: co_await mu.Lock() never blocks a
// runtime worker thread, so the goroutine holding the lock can run and unlock it with any
// EOMAXPROCS, even while it is suspended itself.
//
// Unlock hands the lock directly to the longest-waiting goroutine, like Go's starvation mode. Any
// goroutine may unlock it, as in Go. Unlocking an unlocked Mutex is a fatal error.
struct Mutex {
  Mutex() = default;
  Mutex(const Mutex&) = delete;
  auto operator=(const Mutex&) -> Mutex& = delete;

  [[nodiscard]] auto Lock() -> func<> {
    if (TryLock()) {
      co_return;
    }
    co_await detail::suspend([this](detail::waiter&& w) -> std::optional<detail::waiter> {
      auto lock = std::lock_guard{mutex};
      if (!locked) {
        locked = true;
        return std::move(w);
      }
      waiters.push_back(std::move(w));
      return std::nullopt;
    });
  }

  [[nodiscard]] auto TryLock() -> bool {
    auto lock = std::lock_guard{mutex};
    if (locked) {
      return false;
    }
    locked = true;
    return true;
  }

  void Unlock() {
    auto next = std::optional<detail::waiter>{};
    {
      auto lock = std::lock_guard{mutex};
      if (!locked) {
        runtime::fatal_error("sync: unlock of unlocked mutex");
      }
      if (waiters.empty()) {
        locked = false;
      } else {
        // Stays locked: ownership passes to the waiter.
        next = std::move(waiters.front());
        waiters.pop_front();
      }
    }
    if (next) {
      detail::resume(std::move(*next));
    }
  }

private:
  std::mutex mutex;
  bool locked = false;
  std::deque<detail::waiter> waiters;
};

} // namespace eo::sync
