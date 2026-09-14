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

auto select_ready_send_over_default(eo::chan<int> ch) -> eo::func<> {
  auto select = eo::Select{ch << 7, eo::CaseDefault{}};
  auto index = co_await select.index();
  check(index == 0, "ready send should win over default");

  auto sent = co_await select.process<0>();
  check(sent, "selected ready send should report success");
  check(co_await *ch == 7, "selected ready send should enqueue its value");
}

auto select_default_over_blocked_send(eo::chan<int> ch) -> eo::func<> {
  auto sent = co_await (ch << 1);
  check(sent, "initial buffered send should succeed");

  auto select = eo::Select{ch << 2, eo::CaseDefault{}};
  auto index = co_await select.index();
  check(index == -1, "default should win while send is blocked");

  check(co_await *ch == 1, "blocked send should not replace the buffered value");

  sent = co_await (ch << 3);
  check(sent, "send after default selection should succeed");
  check(co_await *ch == 3, "blocked send should not leak a value after default selection");
}

auto select_closed_send(eo::chan<int> ch) -> eo::func<> {
  auto select = eo::Select{ch << 1, eo::CaseDefault{}};
  auto index = co_await select.index();
  check(index == 0, "closed send should be immediately selectable");

  try {
    co_await select.process<0>();
  } catch (const std::runtime_error& error) {
    check(
      std::string{error.what()} == "panic: send on closed channel", "closed selected send returned the wrong error");
    co_return;
  }

  throw std::runtime_error("selected send on closed channel did not panic");
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

auto select_receive_wins_over_blocked_send(eo::chan<int> send_channel, eo::chan<int> receive_channel) -> eo::func<> {
  auto select = eo::Select{send_channel << 11, *receive_channel};
  auto index = co_await select.index();
  check(index == 1, "receive should win while send remains blocked");

  auto selected = co_await select.process<1>();
  check(selected == 22, "selected receive should preserve its value");
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

void test_ready_send_beats_default() {
  asio::io_context io;
  eo::chan<int> ch{io.get_executor(), 1};

  auto result = asio::co_spawn(io, select_ready_send_over_default(ch), asio::use_future);
  io.run();
  result.get();
}

void test_default_when_send_is_blocked() {
  asio::io_context io;
  eo::chan<int> ch{io.get_executor(), 1};

  auto result = asio::co_spawn(io, select_default_over_blocked_send(ch), asio::use_future);
  io.run();
  result.get();
}

void test_closed_send_is_selected_then_panics() {
  asio::io_context io;
  eo::chan<int> ch{io.get_executor(), 1};
  ch.close();

  auto result = asio::co_spawn(io, select_closed_send(ch), asio::use_future);
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

void test_blocking_select_cancels_losing_send_cleanly() {
  asio::io_context io;
  eo::chan<int> blocked_send{io.get_executor()};
  eo::chan<int> receive_winner{io.get_executor()};

  auto result =
    asio::co_spawn(io, select_receive_wins_over_blocked_send(blocked_send, receive_winner), asio::use_future);
  auto sender = asio::co_spawn(io, send_after_select_suspends(receive_winner), asio::use_future);

  io.run();
  sender.get();
  result.get();
}

int main() {
  test_ready_case_beats_default();
  test_default_when_no_case_ready();
  test_closed_receive_is_ready();
  test_ready_send_beats_default();
  test_default_when_send_is_blocked();
  test_closed_send_is_selected_then_panics();
  test_blocking_select_cancels_loser_cleanly();
  test_blocking_select_cancels_losing_send_cleanly();
}
