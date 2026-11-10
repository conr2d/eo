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
#include <vector>

namespace eo::sync {

// Go's sync.RWMutex. Lock and RLock suspend only the calling goroutine, like Mutex::Lock.
//
// As in Go, a goroutine waiting in Lock keeps new readers out: once a writer is waiting, RLock
// suspends until that writer has unlocked, so writers are not starved. When a writer unlocks, every
// waiting reader gets the lock before the next writer. Unlocking an RWMutex that is not locked in
// that mode is a fatal error.
struct RWMutex {
  RWMutex() = default;
  RWMutex(const RWMutex&) = delete;
  auto operator=(const RWMutex&) -> RWMutex& = delete;

  [[nodiscard]] auto Lock() -> func<> {
    if (TryLock()) {
      co_return;
    }
    co_await detail::suspend([this](detail::waiter&& w) -> std::optional<detail::waiter> {
      auto lock = std::lock_guard{mutex};
      if (can_lock()) {
        writer = true;
        return std::move(w);
      }
      writers.push_back(std::move(w));
      return std::nullopt;
    });
  }

  [[nodiscard]] auto TryLock() -> bool {
    auto lock = std::lock_guard{mutex};
    if (!can_lock()) {
      return false;
    }
    writer = true;
    return true;
  }

  void Unlock() {
    auto released = std::vector<detail::waiter>{};
    {
      auto lock = std::lock_guard{mutex};
      if (!writer) {
        runtime::fatal_error("sync: Unlock of unlocked RWMutex");
      }
      writer = false;
      if (!waiting_readers.empty()) {
        readers += static_cast<int>(waiting_readers.size());
        released.swap(waiting_readers);
      } else if (!writers.empty()) {
        writer = true;
        released.push_back(std::move(writers.front()));
        writers.pop_front();
      }
    }
    for (auto& w : released) {
      detail::resume(std::move(w));
    }
  }

  [[nodiscard]] auto RLock() -> func<> {
    if (TryRLock()) {
      co_return;
    }
    co_await detail::suspend([this](detail::waiter&& w) -> std::optional<detail::waiter> {
      auto lock = std::lock_guard{mutex};
      if (can_rlock()) {
        ++readers;
        return std::move(w);
      }
      waiting_readers.push_back(std::move(w));
      return std::nullopt;
    });
  }

  [[nodiscard]] auto TryRLock() -> bool {
    auto lock = std::lock_guard{mutex};
    if (!can_rlock()) {
      return false;
    }
    ++readers;
    return true;
  }

  void RUnlock() {
    auto next = std::optional<detail::waiter>{};
    {
      auto lock = std::lock_guard{mutex};
      if (readers == 0) {
        runtime::fatal_error("sync: RUnlock of unlocked RWMutex");
      }
      --readers;
      if (readers == 0 && !writers.empty()) {
        writer = true;
        next = std::move(writers.front());
        writers.pop_front();
      }
    }
    if (next) {
      detail::resume(std::move(*next));
    }
  }

private:
  friend struct detail::probe;

  // Called with mutex held.
  auto can_lock() const -> bool {
    return !writer && readers == 0 && writers.empty();
  }

  auto can_rlock() const -> bool {
    return !writer && writers.empty();
  }

  std::mutex mutex;
  bool writer = false;
  int readers = 0;
  std::deque<detail::waiter> writers;
  std::vector<detail::waiter> waiting_readers;
};

} // namespace eo::sync
