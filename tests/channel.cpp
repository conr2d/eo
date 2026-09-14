// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#include <eo/chan.h>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/use_future.hpp>

#include <cassert>
#include <future>
#include <stdexcept>
#include <string>

namespace asio = boost::asio;

auto buffered_roundtrip(eo::chan<int> ch) -> eo::func<int> {
  assert(co_await (ch << 42));
  co_return co_await *ch;
}

auto send_string(eo::chan<std::string> ch) -> eo::func<> {
  assert(co_await (ch << "ping"));
  co_return;
}

auto receive_string(eo::chan<std::string> ch) -> eo::func<std::string> {
  co_return co_await *ch;
}

auto receive_int(eo::chan<int> ch) -> eo::func<int> {
  co_return co_await *ch;
}

auto send_int(eo::chan<int> ch, int value) -> eo::func<> {
  co_await (ch << value);
  co_return;
}

void test_buffered_send_receive() {
  asio::io_context io;
  eo::chan<int> ch{io.get_executor(), 1};

  auto result = asio::co_spawn(io, buffered_roundtrip(ch), asio::use_future);

  io.run();
  assert(result.get() == 42);
}

void test_unbuffered_send_receive() {
  asio::io_context io;
  eo::chan<std::string> ch{io.get_executor()};

  auto sender = asio::co_spawn(io, send_string(ch), asio::use_future);
  auto receiver = asio::co_spawn(io, receive_string(ch), asio::use_future);

  io.run();
  sender.get();
  assert(receiver.get() == "ping");
}

void test_close_then_receive_zero_value() {
  asio::io_context io;
  eo::chan<int> ch{io.get_executor()};
  ch.close();

  auto result = asio::co_spawn(io, receive_int(ch), asio::use_future);

  io.run();
  assert(result.get() == 0);
}

void test_send_on_closed_channel_panics() {
  asio::io_context io;
  eo::chan<int> ch{io.get_executor()};
  ch.close();

  auto result = asio::co_spawn(io, send_int(ch, 1), asio::use_future);

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
