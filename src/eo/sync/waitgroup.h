#pragma once
#include <atomic>
#include <condition_variable>
#include <memory>
#include <shared_mutex>

namespace eo::sync {

struct WaitGroup {
  std::shared_mutex mtx;
  std::atomic<int> state;
  std::condition_variable_any cv;

  void add(int delta) {
    state.fetch_add(delta);
    if (state < 0) {
      throw std::runtime_error("sync: negative WaitGroup counter");
    }
  }

  void done() {
    state.fetch_sub(1);
    cv.notify_all();
  }

  void wait() {
    std::shared_lock lock(mtx);
    cv.wait(lock, [&]() { return state == 0; });
  }
};

using WaitGroupPtr = std::shared_ptr<WaitGroup>;

} // namespace eo::sync
