// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#include <eo/time/ticker.h>

#include <boost/asio/thread_pool.hpp>

#include <chrono>
#include <stdexcept>
#include <thread>

using namespace std::chrono_literals;

void check(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

void drain(const std::shared_ptr<eo::time::Ticker>& ticker) {
  while (ticker->c.raw().try_receive([](boost::system::error_code, eo::time::Ticker::time_point) {})) {
  }
}

void test_stop_prevents_future_ticks() {
  boost::asio::thread_pool pool(2);
  auto executor = pool.get_executor();
  auto ticker = eo::time::Ticker::create_with_executor(executor, 1ms);

  std::this_thread::sleep_for(5ms);
  ticker->stop();
  drain(ticker);
  std::this_thread::sleep_for(5ms);

  check(!ticker->c.raw().try_receive([](boost::system::error_code, eo::time::Ticker::time_point) {}),
    "ticker produced a tick after stop");

  pool.stop();
  pool.join();
}

void test_reset_restarts_stopped_ticker() {
  boost::asio::thread_pool pool(2);
  auto executor = pool.get_executor();
  auto ticker = eo::time::Ticker::create_with_executor(executor, 1h);

  ticker->stop();
  ticker->reset(1ms);
  std::this_thread::sleep_for(5ms);

  check(ticker->c.raw().try_receive([](boost::system::error_code, eo::time::Ticker::time_point) {}),
    "reset did not restart a stopped ticker");

  ticker->stop();
  pool.stop();
  pool.join();
}

int main() {
  test_stop_prevents_future_ticks();
  test_reset_restarts_stopped_ticker();
}
