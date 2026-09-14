// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#include <eo/chan.h>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/use_future.hpp>

#include <cassert>
#include <future>
#include <stdexcept>
#include <string>

namespace asio = boost::asio;

void test_buffered_send_receive() {
  asio::io_context io;
  eo::chan<int> ch{io.get_executor(), 1};

  auto result = asio::co_spawn(
    io,
    [&]() -> eo::func<int> {
      assert(co_await (ch << 42));
      co_return co_await *ch;
    }(),
    asio::use_future);

  io.run();
  assert(result.get() == 42);
}

void test_unbuffered_send_receive() {
  asio::io_context io;
  eo::chan<std::string> ch{io.get_executor()};

  asio::co_spawn(
    io,
    [&]() -> eo::func<> {
      assert(co_await (ch << "ping"));
      co_return;
    }(),
    asio::detached);

  auto result = asio::co_spawn(
    io,
    [&]() -> eo::func<std::string> { co_return co_await *ch; }(),
    asio::use_future);

  io.run();
  assert(result.get() == "ping");
}

void test_close_then_receive_zero_value() {
  asio::io_context io;
  eo::chan<int> ch{io.get_executor()};
  ch.close();

  auto result = asio::co_spawn(
    io,
    [&]() -> eo::func<int> { co_return co_await *ch; }(),
    asio::use_future);

  io.run();
  assert(result.get() == 0);
}

void test_send_on_closed_channel_panics() {
  asio::io_context io;
  eo::chan<int> ch{io.get_executor()};
  ch.close();

  auto result = asio::co_spawn(
    io,
    [&]() -> eo::func<> {
      co_await (ch << 1);
      co_return;
    }(),
    asio::use_future);

  io.run();

  try {
    result.get();
    assert(false);
  } catch (const std::runtime_error& error) {
    assert(std::string{error.what()} == "panic: send on closed channel");
  }
}

void test_double_close_panics() {
  asio::io_context io;
  eo::chan<int> ch{io.get_executor()};
  ch.close();

  try {
    ch.close();
    assert(false);
  } catch (const std::runtime_error& error) {
    assert(std::string{error.what()} == "panic: close of closed channel");
  }
}

void test_channel_copy_shares_identity() {
  asio::io_context io;
  eo::chan<int> original{io.get_executor(), 1};
  auto copy = original;

  assert(original == copy);
}

int main() {
  test_buffered_send_receive();
  test_unbuffered_send_receive();
  test_close_then_receive_zero_value();
  test_send_on_closed_channel_panics();
  test_double_close_panics();
  test_channel_copy_shares_identity();
}
