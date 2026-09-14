// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#include <eo/chan.h>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/use_future.hpp>

#include <future>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

namespace asio = boost::asio;

static_assert(std::is_copy_constructible_v<eo::chan<int>>);
static_assert(std::is_copy_assignable_v<eo::chan<int>>);

void check(bool condition, std::string_view message) {
  if (!condition) {
    throw std::runtime_error(std::string{message});
  }
}

auto buffered_roundtrip(eo::chan<int> ch) -> eo::func<int> {
  auto sent = co_await (ch << 42);
  check(sent, "buffered send failed");
  co_return co_await *ch;
}

auto send_string(eo::chan<std::string> ch) -> eo::func<> {
  auto sent = co_await (ch << "ping");
  check(sent, "unbuffered send failed");
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

auto close_after_yield(eo::chan<int> ch) -> eo::func<> {
  co_await asio::post(asio::use_awaitable);
  ch.close();
}

auto drain_buffer_after_close(eo::chan<int> ch) -> eo::func<> {
  auto sent = co_await (ch << 10);
  check(sent, "first buffered send failed");
  sent = co_await (ch << 20);
  check(sent, "second buffered send failed");

  ch.close();

  check(co_await *ch == 10, "close discarded the first buffered value");
  check(co_await *ch == 20, "close discarded the second buffered value");
  check(co_await *ch == 0, "drained closed channel did not return the zero value");
}

void test_buffered_send_receive() {
  asio::io_context io;
  eo::chan<int> ch{io.get_executor(), 1};

  auto result = asio::co_spawn(io, buffered_roundtrip(ch), asio::use_future);

  io.run();
  check(result.get() == 42, "buffered receive returned the wrong value");
}

void test_unbuffered_send_receive() {
  asio::io_context io;
  eo::chan<std::string> ch{io.get_executor()};

  auto sender = asio::co_spawn(io, send_string(ch), asio::use_future);
  auto receiver = asio::co_spawn(io, receive_string(ch), asio::use_future);

  io.run();
  sender.get();
  check(receiver.get() == "ping", "unbuffered receive returned the wrong value");
}

void test_close_then_receive_zero_value() {
  asio::io_context io;
  eo::chan<int> ch{io.get_executor()};
  ch.close();

  auto result = asio::co_spawn(io, receive_int(ch), asio::use_future);

  io.run();
  check(result.get() == 0, "closed channel receive did not return the zero value");
}

void test_close_drains_buffered_values() {
  asio::io_context io;
  eo::chan<int> ch{io.get_executor(), 2};

  auto result = asio::co_spawn(io, drain_buffer_after_close(ch), asio::use_future);

  io.run();
  result.get();
}

void test_close_releases_pending_receive() {
  asio::io_context io;
  eo::chan<int> ch{io.get_executor()};

  auto receiver = asio::co_spawn(io, receive_int(ch), asio::use_future);
  auto closer = asio::co_spawn(io, close_after_yield(ch), asio::use_future);

  io.run();
  closer.get();
  check(receiver.get() == 0, "closing a channel did not release a pending receive with the zero value");
}

void test_close_releases_pending_send_with_panic() {
  asio::io_context io;
  eo::chan<int> ch{io.get_executor()};

  auto sender = asio::co_spawn(io, send_int(ch, 1), asio::use_future);
  auto closer = asio::co_spawn(io, close_after_yield(ch), asio::use_future);

  io.run();
  closer.get();

  try {
    sender.get();
  } catch (const std::runtime_error& error) {
    check(std::string{error.what()} == "panic: send on closed channel", "pending send returned the wrong close error");
    return;
  }

  throw std::runtime_error("pending send did not panic when the channel closed");
}

void test_send_on_closed_channel_panics() {
  asio::io_context io;
  eo::chan<int> ch{io.get_executor()};
  ch.close();

  auto result = asio::co_spawn(io, send_int(ch, 1), asio::use_future);

  io.run();

  try {
    result.get();
  } catch (const std::runtime_error& error) {
    check(std::string{error.what()} == "panic: send on closed channel", "closed channel send returned the wrong error");
    return;
  }

  throw std::runtime_error("send on closed channel did not panic");
}

void test_double_close_panics() {
  asio::io_context io;
  eo::chan<int> ch{io.get_executor()};
  ch.close();

  try {
    ch.close();
  } catch (const std::runtime_error& error) {
    check(std::string{error.what()} == "panic: close of closed channel", "double close returned the wrong error");
    return;
  }

  throw std::runtime_error("double close did not panic");
}

void test_channel_copy_shares_identity() {
  asio::io_context io;
  eo::chan<int> original{io.get_executor(), 1};
  auto copy = original;

  check(original == copy, "copied channel did not preserve identity");
}

void test_channel_const_copy_shares_identity() {
  asio::io_context io;
  eo::chan<int> original{io.get_executor(), 1};
  const auto& view = original;
  auto copy = view;

  check(original == copy, "copying a const channel did not preserve identity");
}

void test_channel_copy_assignment_shares_identity() {
  asio::io_context io;
  eo::chan<int> source{io.get_executor(), 1};
  eo::chan<int> target{io.get_executor(), 1};

  target = source;

  check(source == target, "copy assignment did not preserve channel identity");
}

int main() {
  test_buffered_send_receive();
  test_unbuffered_send_receive();
  test_close_then_receive_zero_value();
  test_close_drains_buffered_values();
  test_close_releases_pending_receive();
  test_close_releases_pending_send_with_panic();
  test_send_on_closed_channel_panics();
  test_double_close_panics();
  test_channel_copy_shares_identity();
  test_channel_const_copy_shares_identity();
  test_channel_copy_assignment_shares_identity();
}
