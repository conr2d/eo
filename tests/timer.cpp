// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#include <eo/time/timer.h>

#include <boost/asio/io_context.hpp>

#include <chrono>
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

int main() {
  test_reset_ignores_stale_cancel_completion();
}
