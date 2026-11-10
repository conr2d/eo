// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once
#include <eo/go.h>

#include <boost/asio/any_completion_handler.hpp>
#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/associated_executor.hpp>
#include <boost/asio/async_result.hpp>
#include <boost/asio/post.hpp>

#include <memory>
#include <mutex>
#include <stdexcept>
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
    auto released = std::vector<waiter>{};
    {
      auto lock = std::lock_guard{mutex};
      counter += delta;
      if (counter < 0) {
        throw std::runtime_error("sync: negative WaitGroup counter");
      }
      if (counter == 0) {
        released.swap(waiters);
      }
    }
    // Resume the waiters on their own executors, not inline in the goroutine that called Add or Done.
    for (auto& w : released) {
      resume(std::move(w));
    }
  }

  void Done() {
    Add(-1);
  }

  [[nodiscard]] auto Wait() -> func<> {
    return boost::asio::async_initiate<decltype(boost::asio::use_awaitable), void()>(
      [this](auto handler) {
        auto w = waiter{
          boost::asio::get_associated_executor(handler, runtime::execution_context.get_executor()),
          std::move(handler),
        };
        {
          auto lock = std::lock_guard{mutex};
          if (counter != 0) {
            waiters.push_back(std::move(w));
            return;
          }
        }
        resume(std::move(w));
      },
      boost::asio::use_awaitable);
  }

private:
  // A suspended Wait: the coroutine's completion handler and the executor it resumes on.
  struct waiter {
    boost::asio::any_io_executor executor;
    boost::asio::any_completion_handler<void()> handler;
  };

  static void resume(waiter w) {
    boost::asio::post(w.executor, [handler = std::move(w.handler)]() mutable { std::move(handler)(); });
  }

  std::mutex mutex;
  int counter = 0;
  std::vector<waiter> waiters;
};

using WaitGroupPtr = std::shared_ptr<WaitGroup>;

} // namespace eo::sync
