// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
// A blocking select completes exactly one of its cases: a case that it does not choose neither
// receives nor sends a value, even while other goroutines on other threads use the same channels.
#include <eo/chan.h>
#include <eo/select.h>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/asio/use_future.hpp>

#include <chrono>
#include <cstdint>
#include <future>
#include <stdexcept>
#include <vector>

namespace asio = boost::asio;

void check(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

constexpr int rounds = 2000;

template<typename T>
void wait_for(std::future<T>& f, const char* message) {
  check(f.wait_for(std::chrono::seconds(30)) == std::future_status::ready, message);
  f.get();
}

auto send_all(eo::chan<int> ch, int first, int count) -> eo::func<> {
  for (auto i = first; i < first + count; i++) {
    co_await (ch << i);
  }
}

// Two senders feed two unbuffered channels, and one goroutine receives with a select over both
// and a third channel that never gets a value. Every sent value must arrive exactly once. A losing
// receive that took a value would drop it, and the receiver would then wait forever.
auto receive_all(eo::chan<int> a, eo::chan<int> b, eo::chan<int> never) -> eo::func<std::vector<int>> {
  auto seen = std::vector<int>(2 * rounds, 0);
  for (auto i = 0; i < 2 * rounds; i++) {
    switch (auto select = eo::Select{*a, *b, *never}; co_await select.index()) {
    case 0:
      seen.at(select.recv<0>())++;
      break;
    case 1:
      seen.at(select.recv<1>())++;
      break;
    default:
      check(false, "the channel that gets no value should not be chosen");
    }
  }
  co_return seen;
}

void test_losing_receive_takes_nothing() {
  asio::thread_pool pool(4);
  eo::chan<int> a{pool.get_executor()};
  eo::chan<int> b{pool.get_executor()};
  eo::chan<int> never{pool.get_executor()};
  auto received = asio::co_spawn(pool, receive_all(a, b, never), asio::use_future);
  auto first = asio::co_spawn(pool, send_all(a, 0, rounds), asio::use_future);
  auto second = asio::co_spawn(pool, send_all(b, rounds, rounds), asio::use_future);
  check(received.wait_for(std::chrono::seconds(30)) == std::future_status::ready,
    "the select should receive every value");
  for (auto count : received.get()) {
    check(count == 1, "every value should be received exactly once");
  }
  wait_for(first, "the first sender should finish");
  wait_for(second, "the second sender should finish");
  pool.join();
}

// One goroutine sends 0, 1, 2, ... with a select over two channels, and two receivers count what
// they get. A losing send that also delivered its value would make the receivers see more values
// than the selects chose, or a value twice.
auto send_with_select(eo::chan<int> a, eo::chan<int> b) -> eo::func<std::vector<int>> {
  auto chosen = std::vector<int>(2, 0);
  for (auto i = 0; i < 2 * rounds; i++) {
    auto index = co_await eo::Select{a << i, b << i}.index();
    chosen.at(index)++;
  }
  a.close();
  b.close();
  co_return chosen;
}

auto count_received(eo::chan<int> ch) -> eo::func<std::vector<int>> {
  auto values = std::vector<int>{};
  for (;;) {
    auto [v, ok] = co_await eo::recv_ok(ch);
    if (!ok) {
      co_return values;
    }
    values.push_back(v);
  }
}

void test_losing_send_sends_nothing() {
  asio::thread_pool pool(4);
  eo::chan<int> a{pool.get_executor()};
  eo::chan<int> b{pool.get_executor()};
  auto from_a = asio::co_spawn(pool, count_received(a), asio::use_future);
  auto from_b = asio::co_spawn(pool, count_received(b), asio::use_future);
  auto chosen = asio::co_spawn(pool, send_with_select(a, b), asio::use_future);
  check(chosen.wait_for(std::chrono::seconds(30)) == std::future_status::ready, "the sends should finish");
  auto counts = chosen.get();
  check(from_a.wait_for(std::chrono::seconds(30)) == std::future_status::ready &&
      from_b.wait_for(std::chrono::seconds(30)) == std::future_status::ready,
    "the receivers should see both closes");
  auto got_a = from_a.get();
  auto got_b = from_b.get();
  check(static_cast<int>(got_a.size()) == counts[0] && static_cast<int>(got_b.size()) == counts[1],
    "each channel should get exactly the sends that the selects chose");
  auto seen = std::vector<int>(2 * rounds, 0);
  for (auto v : got_a) {
    seen.at(v)++;
  }
  for (auto v : got_b) {
    seen.at(v)++;
  }
  for (auto count : seen) {
    check(count == 1, "every value should be sent exactly once");
  }
  pool.join();
}

int main() {
  for (auto i = 0; i < 5; i++) {
    test_losing_receive_takes_nothing();
    test_losing_send_sends_nothing();
  }
}
