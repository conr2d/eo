// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#include <eo/sync/waitgroup.h>

#include <atomic>
#include <stdexcept>
#include <string>
#include <thread>

void check(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

void test_done_panics_on_negative_counter() {
  eo::sync::WaitGroup wg;

  try {
    wg.done();
  } catch (const std::runtime_error& error) {
    check(std::string{error.what()} == "sync: negative WaitGroup counter", "negative counter returned wrong error");
    return;
  }

  throw std::runtime_error("WaitGroup done did not panic on a negative counter");
}

void test_add_negative_delta_releases_waiters_at_zero() {
  eo::sync::WaitGroup wg;
  wg.add(2);

  std::atomic_bool returned{false};
  std::thread waiter([&] {
    wg.wait();
    returned = true;
  });

  wg.add(-2);
  waiter.join();

  check(returned, "WaitGroup waiter was not released when Add reached zero");
}

int main() {
  test_done_panics_on_negative_counter();
  test_add_negative_delta_releases_waiters_at_zero();
}
