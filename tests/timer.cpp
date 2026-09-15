// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#include <eo/time/timer.h>

#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/thread_pool.hpp>

#include <chrono>
#include <latch>
#include <stdexcept>

namespace asio = boost::asio;
using namespace std::chrono_literals;

void check(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

void test_reset_ignores_stale_cancel_completion() {
  asio::io_context io;
  auto executor = io.get_executor();
  auto timer = eo::time::Timer::create_with_executor(executor, 1h);

  timer->reset(1h);
  io.poll();

  check(timer->stop(), "stale reset cancellation should not expire the replacement timer");
}

void test_reset_reports_active_timer() {
  asio::io_context io;
  auto executor = io.get_executor();
  auto timer = eo::time::Timer::create_with_executor(executor, 1h);

  check(timer->reset(1h), "reset should report an active timer");
}

void test_reset_reports_stopped_timer() {
  asio::io_context io;
  auto executor = io.get_executor();
  auto timer = eo::time::Timer::create_with_executor(executor, 1h);

  check(timer->stop(), "timer should be active before stop");
  check(!timer->reset(1h), "reset should report a stopped timer as inactive");
}

void test_reset_reports_expired_timer() {
  asio::io_context io;
  auto executor = io.get_executor();
  auto timer = eo::time::Timer::create_with_executor(executor, 0ms);

  io.run();

  check(!timer->reset(1h), "reset should report an expired timer as inactive");
}

void test_timer_state_is_safe_across_worker_threads() {
  asio::thread_pool pool{2};
  auto executor = pool.get_executor();
  auto timer = eo::time::Timer::create_with_executor(executor, 1h);
  constexpr int reset_count = 64;
  std::latch resets{reset_count};

  for (int i = 0; i < reset_count; ++i) {
    asio::post(pool, [timer, &resets] {
      timer->reset(1h);
      resets.count_down();
    });
  }

  resets.wait();
  check(timer->stop(), "concurrent resets should leave the latest timer active");
  pool.stop();
  pool.join();
}

int main() {
  test_reset_ignores_stale_cancel_completion();
  test_reset_reports_active_timer();
  test_reset_reports_stopped_timer();
  test_reset_reports_expired_timer();
  test_timer_state_is_safe_across_worker_threads();
}
