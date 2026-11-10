// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#include <eo/invoke.h>
#include <eo/sync/waitgroup.h>
#include <eo/time.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <stdexcept>
#include <string>

void check(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

void test_done_panics_on_negative_counter() {
  eo::sync::WaitGroup wg;

  try {
    wg.Done();
  } catch (const std::runtime_error& error) {
    check(std::string{error.what()} == "sync: negative WaitGroup counter", "negative counter returned wrong error");
    return;
  }

  throw std::runtime_error("WaitGroup Done did not panic on a negative counter");
}

void test_add_negative_delta_releases_waiters_at_zero() {
  auto returned = eo::invoke([]() -> eo::func<bool> {
    auto wg = std::make_shared<eo::sync::WaitGroup>();
    wg->Add(2);
    eo::go([wg]() -> eo::func<> {
      wg->Add(-2);
      co_return;
    });
    co_await wg->Wait();
    co_return true;
  });

  check(returned, "WaitGroup waiter was not released when Add reached zero");
}

void test_wait_at_zero_returns() {
  eo::invoke([]() -> eo::func<> {
    eo::sync::WaitGroup wg;
    co_await wg.Wait();
  });
}

// Wait suspends the goroutine instead of blocking its worker thread, so the goroutines it waits for
// also run when the runtime has a single worker (the eo.waitgroup_one_worker test).
void test_wait_does_not_block_the_worker() {
  auto sum = eo::invoke([]() -> eo::func<int> {
    auto wg = std::make_shared<eo::sync::WaitGroup>();
    auto total = std::make_shared<std::atomic<int>>(0);
    for (auto i = 1; i <= 5; i++) {
      wg->Add(1);
      eo::go([wg, total, i]() -> eo::func<> {
        co_await eo::time::Sleep(std::chrono::milliseconds{1});
        *total += i;
        wg->Done();
      });
    }
    co_await wg->Wait();
    co_return total->load();
  });

  check(sum == 15, "WaitGroup Wait returned before every goroutine called Done");
}

void test_all_waiters_are_released() {
  auto released = eo::invoke([]() -> eo::func<int> {
    auto wg = std::make_shared<eo::sync::WaitGroup>();
    auto done = std::make_shared<eo::sync::WaitGroup>();
    auto count = std::make_shared<std::atomic<int>>(0);
    wg->Add(1);
    for (auto i = 0; i < 3; i++) {
      done->Add(1);
      eo::go([wg, done, count]() -> eo::func<> {
        co_await wg->Wait();
        ++*count;
        done->Done();
      });
    }
    co_await eo::time::Sleep(std::chrono::milliseconds{10});
    wg->Done();
    co_await done->Wait();
    co_return count->load();
  });

  check(released == 3, "WaitGroup did not release every waiter");
}

int main() {
  test_done_panics_on_negative_counter();
  test_add_negative_delta_releases_waiters_at_zero();
  test_wait_at_zero_returns();
  test_wait_does_not_block_the_worker();
  test_all_waiters_are_released();
}
