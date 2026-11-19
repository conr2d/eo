// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#include <eo/chan.h>
#include <eo/select.h>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/use_future.hpp>

#include <chrono>
#include <future>
#include <stdexcept>
#include <string>
#include <utility>

namespace asio = boost::asio;
using namespace std::chrono_literals;

void check(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

eo::func<> await_nil_send(eo::chan<int>& nil) {
  co_await (nil << 1);
}

eo::func<> await_nil_receive(eo::chan<int>& nil) {
  (void)co_await *nil;
}

void test_default_constructed_channel_is_nil() {
  eo::chan<int> first;
  eo::chan<int> second;

  check(first == second, "default-constructed channels should compare as nil");
  check(eo::Select{*first}.try_index() == -1, "nil receive should not be selectable");
  check(eo::Select{first << 1}.try_index() == -1, "nil send should not be selectable");
}

void test_close_nil_channel_panics() {
  eo::chan<int> ch;

  try {
    ch.close();
  } catch (const eo::Panic& error) {
    check(std::string{error.what()} == "panic: close of nil channel", "close(nil) returned the wrong panic");
    return;
  }

  throw std::runtime_error("close(nil) did not panic");
}

void test_awaited_nil_operations_stay_suspended() {
  asio::io_context io;
  eo::chan<int> nil;
  bool progressed = false;

  auto sender = asio::co_spawn(io, await_nil_send(nil), asio::use_future);
  auto receiver = asio::co_spawn(io, await_nil_receive(nil), asio::use_future);

  asio::post(io, [&] { progressed = true; });
  io.poll();

  check(progressed, "nil channel operations blocked unrelated scheduled work");
  check(sender.wait_for(0s) == std::future_status::timeout, "nil send should remain suspended");
  check(receiver.wait_for(0s) == std::future_status::timeout, "nil receive should remain suspended");
}

void test_ready_channel_beats_nil_case() {
  asio::io_context io;
  eo::chan<int> nil;
  eo::chan<int> ready{io.get_executor(), 1};
  check(ready.raw().try_send(boost::system::error_code{}, 7), "test setup send should succeed");

  auto select = eo::Select{*nil, *ready};
  check(select.try_index() == 1, "ready receive should win over disabled nil receive");
  check(select.recv<1>() == 7, "selected receive should preserve its value");
}

void test_recv2_reports_successful_receive() {
  asio::io_context io;
  eo::chan<int> ch{io.get_executor(), 1};
  check(ch.raw().try_send(boost::system::error_code{}, 11), "test setup send should succeed");

  auto select = eo::Select{*ch};
  check(select.try_index() == 0, "ready receive should be selected");

  auto [value, ok] = select.recv2<0>();
  check(value == 11, "recv2 should preserve the received value");
  check(ok, "recv2 should report true for a received value");
}

void test_recv2_reports_closed_receive() {
  asio::io_context io;
  eo::chan<int> ch{io.get_executor(), 1};
  ch.close();

  auto select = eo::Select{*ch};
  check(select.try_index() == 0, "closed receive should be selectable");

  auto [value, ok] = select.recv2<0>();
  check(value == 0, "closed recv2 should return the zero value");
  check(!ok, "closed recv2 should report false");
}

// Go delivers values sent before close before it reports the close, also to a select.
void test_select_drains_closed_buffered_channel() {
  asio::io_context io;
  eo::chan<int> ch{io.get_executor(), 2};
  check(ch.raw().try_send(boost::system::error_code{}, 1), "test setup send should succeed");
  check(ch.raw().try_send(boost::system::error_code{}, 2), "test setup send should succeed");
  ch.close();

  for (auto [want, want_ok] : {std::pair{1, true}, std::pair{2, true}, std::pair{0, false}}) {
    auto select = eo::Select{*ch};
    check(select.try_index() == 0, "a closed channel should be selectable");
    auto [value, ok] = select.recv2<0>();
    check(value == want && ok == want_ok, "select should receive buffered values before reporting the close");
  }
}

auto select_receive(eo::chan<int> ch) -> eo::func<std::pair<int, bool>> {
  auto select = eo::Select{*ch};
  co_await select.index();
  co_return select.recv2<0>();
}

void test_select_wait_released_by_close() {
  asio::io_context io;
  eo::chan<int> ch{io.get_executor()};
  auto receiver = asio::co_spawn(io, select_receive(ch), asio::use_future);
  auto closer = asio::co_spawn(
    io,
    [ch]() mutable -> eo::func<> {
      co_await asio::post(asio::use_awaitable);
      ch.close();
    },
    asio::use_future);

  io.run();
  closer.get();
  check(receiver.get() == std::pair{0, false}, "a select waiting to receive should report the close");
}

int main() {
  test_default_constructed_channel_is_nil();
  test_close_nil_channel_panics();
  test_awaited_nil_operations_stay_suspended();
  test_ready_channel_beats_nil_case();
  test_recv2_reports_successful_receive();
  test_recv2_reports_closed_receive();
  test_select_drains_closed_buffered_channel();
  test_select_wait_released_by_close();
}
