// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once
#include <eo/runtime.h>
#include <eo/sync/waiter.h>

#include <condition_variable>
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

  // mu.Lock() for a Mutex none of whose critical sections suspends, at every site that locks it:
  // it waits for the lock on a C++ condition variable, blocking the worker thread, so the calling
  // function stays a plain function. The holder never suspends while it holds the lock, so it runs
  // on another worker until it unlocks, and with one worker no other goroutine runs while it holds
  // the lock. A Mutex that is also locked with Lock is outside this use: Unlock can hand the lock to
  // a goroutine that is not running yet, while threads in BlockingLock hold every worker. The
  // waiters here get the lock in no particular order, and TryLock or a new BlockingLock may take it
  // first, which Go allows.
  void BlockingLock() {
    auto lock = std::unique_lock{mutex};
    ++blocked;
    released.wait(lock, [this] { return !locked; });
    --blocked;
    locked = true;
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
    auto wake = false;
    {
      auto lock = std::lock_guard{mutex};
      if (!locked) {
        runtime::fatal_error("sync: unlock of unlocked mutex");
      }
      if (waiters.empty()) {
        locked = false;
        wake = blocked > 0;
      } else {
        // Stays locked: ownership passes to the waiter.
        next = std::move(waiters.front());
        waiters.pop_front();
      }
    }
    if (wake) {
      released.notify_one();
    }
    if (next) {
      detail::resume(std::move(*next));
    }
  }

private:
  friend struct detail::probe;

  std::mutex mutex;
  bool locked = false;
  std::deque<detail::waiter> waiters;
  std::condition_variable released;
  int blocked = 0;
};

} // namespace eo::sync
