// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once
#include <atomic>
#include <memory>

namespace eo::sync {

struct WaitGroup {
  std::atomic<int> state{0};

  void add(int delta) {
    auto current = state.fetch_add(delta) + delta;
    if (current < 0) {
      throw std::runtime_error("sync: negative WaitGroup counter");
    }
    if (current == 0) {
      state.notify_all();
    }
  }

  void done() {
    add(-1);
  }

  void wait() {
    auto current = state.load();
    while (current != 0) {
      state.wait(current);
      current = state.load();
    }
  }
};

using WaitGroupPtr = std::shared_ptr<WaitGroup>;

} // namespace eo::sync
