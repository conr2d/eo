// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once
#include <eo/panic.h>
#include <eo/sync/waiter.h>

#include <memory>
#include <mutex>
#include <optional>
#include <utility>
#include <vector>

namespace eo::sync {

// Go's sync.WaitGroup. Wait suspends only the calling goroutine: co_await wg.Wait() never blocks a
// runtime worker thread, so the goroutines it waits for can run with any EOMAXPROCS.
struct WaitGroup {
  WaitGroup() = default;
  WaitGroup(const WaitGroup&) = delete;
  auto operator=(const WaitGroup&) -> WaitGroup& = delete;

  void Add(int delta) {
    auto released = std::vector<detail::waiter>{};
    {
      auto lock = std::lock_guard{mutex};
      counter += delta;
      if (counter < 0) {
        panic("sync: negative WaitGroup counter");
      }
      if (counter == 0) {
        released.swap(waiters);
      }
    }
    // Resume the waiters on their own executors, not inline in the goroutine that called Add or Done.
    for (auto& w : released) {
      detail::resume(std::move(w));
    }
  }

  void Done() {
    Add(-1);
  }

  [[nodiscard]] auto Wait() -> func<> {
    return detail::suspend([this](detail::waiter&& w) -> std::optional<detail::waiter> {
      auto lock = std::lock_guard{mutex};
      if (counter == 0) {
        return std::move(w);
      }
      waiters.push_back(std::move(w));
      return std::nullopt;
    });
  }

private:
  std::mutex mutex;
  int counter = 0;
  std::vector<detail::waiter> waiters;
};

using WaitGroupPtr = std::shared_ptr<WaitGroup>;

} // namespace eo::sync
