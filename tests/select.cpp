// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#include <eo/chan.h>
#include <eo/select.h>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/use_future.hpp>

#include <stdexcept>
#include <string>

namespace asio = boost::asio;

void check(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

auto select_ready_over_default(eo::chan<int> ready, eo::chan<int> idle) -> eo::func<> {
  auto sent = co_await (ready << 7);
  check(sent, "buffered send should succeed");

  auto select = eo::Select{*ready, *idle, eo::CaseDefault{}};
  auto index = co_await select.index();
  check(index == 0, "ready communication should win over default");

  auto value = co_await select.process<0>();
  check(value == 7, "selected receive should preserve its value");
}

auto select_default_when_idle(eo::chan<int> first, eo::chan<int> second) -> eo::func<> {
  auto select = eo::Select{*first, *second, eo::CaseDefault{}};
  auto index = co_await select.index();
  check(index == -1, "default should be selected when no communication is ready");
}

auto select_closed_receive(eo::chan<int> closed, eo::chan<int> idle) -> eo::func<> {
  auto select = eo::Select{*closed, *idle, eo::CaseDefault{}};
  auto index = co_await select.index();
  check(index == 0, "closed receive should be ready");

  auto value = co_await select.process<0>();
  check(value == 0, "closed receive should yield the zero value");
}

auto select_blocking_winner_and_reuse_loser(eo::chan<int> first, eo::chan<int> second) -> eo::func<> {
  auto select = eo::Select{*first, *second};
  auto index = co_await select.index();
  check(index == 1, "later second send should wake blocking select");

  auto selected = co_await select.process<1>();
  check(selected == 22, "selected receive should preserve the winning value");

  auto sent = co_await (first << 11);
  check(sent, "losing channel should remain usable after cancellation");

  auto remaining = co_await *first;
  check(remaining == 11, "losing receive cancellation should not consume a later value");
}

auto send_after_select_suspends(eo::chan<int> ch) -> eo::func<> {
  co_await asio::post(asio::use_awaitable);
  auto sent = co_await (ch << 22);
  check(sent, "wake-up send should succeed");
}

void test_ready_case_beats_default() {
  asio::io_context io;
  eo::chan<int> ready{io.get_executor(), 1};
  eo::chan<int> idle{io.get_executor(), 1};

  auto result = asio::co_spawn(io, select_ready_over_default(ready, idle), asio::use_future);
  io.run();
  result.get();
}

void test_default_when_no_case_ready() {
  asio::io_context io;
  eo::chan<int> first{io.get_executor(), 1};
  eo::chan<int> second{io.get_executor(), 1};

  auto result = asio::co_spawn(io, select_default_when_idle(first, second), asio::use_future);
  io.run();
  result.get();
}

void test_closed_receive_is_ready() {
  asio::io_context io;
  eo::chan<int> closed{io.get_executor(), 1};
  eo::chan<int> idle{io.get_executor(), 1};
  closed.close();

  auto result = asio::co_spawn(io, select_closed_receive(closed, idle), asio::use_future);
  io.run();
  result.get();
}

void test_blocking_select_cancels_loser_cleanly() {
  asio::io_context io;
  eo::chan<int> first{io.get_executor(), 1};
  eo::chan<int> second{io.get_executor(), 1};

  auto result = asio::co_spawn(io, select_blocking_winner_and_reuse_loser(first, second), asio::use_future);
  auto sender = asio::co_spawn(io, send_after_select_suspends(second), asio::use_future);

  io.run();
  sender.get();
  result.get();
}

int main() {
  test_ready_case_beats_default();
  test_default_when_no_case_ready();
  test_closed_receive_is_ready();
  test_blocking_select_cancels_loser_cleanly();
}
