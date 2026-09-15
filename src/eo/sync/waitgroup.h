// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once
#include <atomic>
#include <condition_variable>
#include <memory>
#include <shared_mutex>

namespace eo::sync {

struct WaitGroup {
  std::shared_mutex mtx;
  std::atomic<int> state{0};
  std::condition_variable_any cv;

  void add(int delta) {
    auto current = state.fetch_add(delta) + delta;
    if (current < 0) {
      throw std::runtime_error("sync: negative WaitGroup counter");
    }
    if (current == 0) {
      cv.notify_all();
    }
  }

  void done() {
    add(-1);
  }

  void wait() {
    std::shared_lock lock(mtx);
    cv.wait(lock, [&]() { return state == 0; });
  }
};

using WaitGroupPtr = std::shared_ptr<WaitGroup>;

} // namespace eo::sync
