// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#include <eo/chan.h>
#include <eo/slice.h>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/use_future.hpp>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <future>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

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
  } catch (const eo::Panic& error) {
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
  } catch (const eo::Panic& error) {
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
  } catch (const eo::Panic& error) {
    check(std::string{error.what()} == "panic: close of closed channel", "double close returned the wrong error");
    return;
  }

  throw std::runtime_error("double close did not panic");
}

void test_concurrent_close_has_single_winner() {
  asio::io_context io;
  eo::chan<int> ch{io.get_executor()};
  std::atomic_int successes{0};
  std::atomic_int panics{0};
  std::atomic_bool wrong_error{false};
  std::vector<std::thread> closers;

  for (int i = 0; i < 8; ++i) {
    auto copy = ch;
    closers.emplace_back([copy, &successes, &panics, &wrong_error]() mutable {
      try {
        copy.close();
        ++successes;
      } catch (const eo::Panic& error) {
        if (std::string{error.what()} == "panic: close of closed channel") {
          ++panics;
        } else {
          wrong_error = true;
        }
      }
    });
  }

  for (auto& closer : closers) {
    closer.join();
  }

  check(successes == 1, "concurrent close should have exactly one successful closer");
  check(panics == 7, "all losing concurrent close calls should panic");
  check(!wrong_error, "concurrent close returned the wrong panic");
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

auto receive_ok_sequence(eo::chan<int> ch) -> eo::func<std::vector<std::pair<int, bool>>> {
  auto results = std::vector<std::pair<int, bool>>{};
  for (int i = 0; i < 3; ++i) {
    results.push_back(co_await eo::recv_ok(ch));
  }
  co_return results;
}

void test_recv_ok_reports_close_after_drain() {
  asio::io_context io;
  eo::chan<int> ch{io.get_executor(), 2};
  auto fill = [](eo::chan<int> ch) -> eo::func<> {
    co_await (ch << 7);
    co_await (ch << 0);
    ch.close();
  };
  auto filled = asio::co_spawn(io, fill(ch), asio::use_future);
  auto result = asio::co_spawn(io, receive_ok_sequence(ch), asio::use_future);

  io.run();
  filled.get();
  auto values = result.get();
  check(values[0] == std::pair{7, true}, "recv_ok should receive a buffered value with true");
  check(values[1] == std::pair{0, true}, "recv_ok should report a buffered zero value as received");
  check(values[2] == std::pair{0, false}, "recv_ok should report false once the channel is closed and drained");
}

void test_recv_ok_released_by_close() {
  asio::io_context io;
  eo::chan<int> ch{io.get_executor()};
  auto receive = [](eo::chan<int> ch) -> eo::func<std::pair<int, bool>> { co_return co_await eo::recv_ok(ch); };
  auto receiver = asio::co_spawn(io, receive(ch), asio::use_future);
  auto closer = asio::co_spawn(io, close_after_yield(ch), asio::use_future);

  io.run();
  closer.get();
  check(receiver.get() == std::pair{0, false}, "closing a channel should release a pending recv_ok with false");
}

void test_nil_channel_and_capacity() {
  eo::chan<int> nil;
  check(nil == nullptr && eo::chan<int>{nullptr} == nullptr, "the zero value should be the nil channel");
  check(eo::cap(nil) == 0, "a nil channel should have capacity 0");
  asio::io_context io;
  eo::chan<int> buffered{io.get_executor(), 3};
  check(buffered != nullptr && eo::cap(buffered) == 3, "cap should report the buffer size");

  try {
    nil.close();
  } catch (const eo::Panic& error) {
    check(std::string{error.what()} == "panic: close of nil channel", "closing nil returned the wrong error");
    return;
  }
  throw std::runtime_error("closing a nil channel did not panic");
}

void test_make_chan_rejects_bad_sizes() {
  for (auto size : {int64_t{-1}, int64_t{1} << 62}) {
    try {
      [[maybe_unused]] auto ch = eo::make_chan<int64_t>(size);
    } catch (const eo::Panic& error) {
      check(std::string{error.what()} == "panic: makechan: size out of range", "makechan returned the wrong error");
      continue;
    }
    throw std::runtime_error("make_chan did not panic on a bad size");
  }
}

// Many senders blocked on an unbuffered channel, which another thread closes while worker threads
// resume them. Every one must panic: a sender cancelled by close must not see the channel as open.
void test_close_panics_every_blocked_sender() {
  for (int round = 0; round < 20; ++round) {
    asio::io_context io;
    eo::chan<int> ch{io.get_executor()};
    constexpr int senders = 32;
    std::atomic_int panics{0};
    std::atomic_int returned{0};
    std::atomic_int started{0};
    for (int i = 0; i < senders; ++i) {
      asio::co_spawn(
        io,
        [&, ch]() -> eo::func<> {
          ++started;
          try {
            co_await (ch << 1);
            ++returned;
          } catch (const eo::Panic&) {
            ++panics;
          }
        },
        asio::detached);
    }
    auto work = asio::make_work_guard(io);
    std::vector<std::thread> workers;
    for (int i = 0; i < 4; ++i) {
      workers.emplace_back([&io] { io.run(); });
    }
    while (started < senders) {
      std::this_thread::yield();
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    auto copy = ch;
    copy.close();
    work.reset();
    for (auto& worker : workers) {
      worker.join();
    }
    check(panics == senders && returned == 0, "every sender blocked at close should panic");
  }
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
  test_concurrent_close_has_single_winner();
  test_channel_copy_shares_identity();
  test_channel_const_copy_shares_identity();
  test_channel_copy_assignment_shares_identity();
  test_recv_ok_reports_close_after_drain();
  test_recv_ok_released_by_close();
  test_nil_channel_and_capacity();
  test_make_chan_rejects_bad_sizes();
  test_close_panics_every_blocked_sender();
}
