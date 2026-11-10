// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#include <eo/core.h>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/this_coro.hpp>
#include <boost/asio/use_awaitable.hpp>

#include <chrono>
#include <future>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace asio = boost::asio;

void check(bool condition, std::string_view message) {
  if (!condition) {
    throw std::runtime_error(std::string{message});
  }
}

using worker_fn = eo::func<> (*)(std::vector<int>*, int, int);

auto worker(std::vector<int>* order, int first, int second) -> eo::func<> {
  order->push_back(3);
  order->push_back(first);
  order->push_back(second);
  co_return;
}

auto evaluate_function(std::vector<int>& order) -> worker_fn {
  order.push_back(0);
  return worker;
}

int evaluate_argument(std::vector<int>& order, int marker, int value) {
  order.push_back(marker);
  return value;
}

// Returns eo::func<> without being a coroutine, so its own statements run when it is called.
auto eager_wrapper(std::vector<int>* order, int value) -> eo::func<> {
  order->push_back(value);
  return worker(order, value + 1, value + 2);
}

void plain_function(std::vector<int>* order, int value) {
  order->push_back(value);
}

struct account {
  int balance = 0;

  auto Report(std::vector<int>* order) -> eo::func<> {
    order->push_back(balance);
    co_return;
  }
};

struct counter {
  std::vector<int>* order;
  int calls = 0;

  void operator()() {
    order->push_back(++calls);
  }
};

// These tests pass the address of a test-local vector. That is only safe because io.run() drains
// every goroutine before the vector goes out of scope; it is not the canonical goroutine mapping.

void function_and_arguments_are_evaluated_before_launch() {
  asio::io_context io;
  std::vector<int> order;

  auto _eo_go_fn_0 = evaluate_function(order);
  std::vector<int>* _eo_go_arg_0_0 = &order;
  int _eo_go_arg_0_1 = evaluate_argument(order, 1, 10);
  int _eo_go_arg_0_2 = evaluate_argument(order, 2, 20);
  eo::go(io, [=]() mutable { return _eo_go_fn_0(_eo_go_arg_0_0, _eo_go_arg_0_1, _eo_go_arg_0_2); });

  check(order == std::vector<int>({0, 1, 2}), "goroutine expressions were not evaluated before launch");

  io.run();

  check(order == std::vector<int>({0, 1, 2, 3, 10, 20}), "goroutine body did not run after launch");
}

void non_coroutine_callee_runs_after_launch() {
  asio::io_context io;
  std::vector<int> order;

  std::vector<int>* _eo_go_arg_0_0 = &order;
  int _eo_go_arg_0_1 = 100;
  eo::go(io, [=]() mutable { return eager_wrapper(_eo_go_arg_0_0, _eo_go_arg_0_1); });

  check(order.empty(), "non-coroutine callee ran in the launching goroutine");

  io.run();

  check(order == std::vector<int>({100, 3, 101, 102}), "non-coroutine callee did not run after launch");
}

void plain_callee_runs_after_launch() {
  asio::io_context io;
  std::vector<int> order;

  std::vector<int>* _eo_go_arg_0_0 = &order;
  int _eo_go_arg_0_1 = 7;
  eo::go(io, [=]() mutable { return plain_function(_eo_go_arg_0_0, _eo_go_arg_0_1); });

  check(order.empty(), "plain callee ran in the launching goroutine");

  io.run();

  check(order == std::vector<int>({7}), "plain callee did not run after launch");
}

void function_value_is_copied_at_launch() {
  asio::io_context io;
  std::vector<int> order;

  auto f = counter{&order};
  auto _eo_go_fn_0 = f;
  eo::go(io, [=]() mutable { return _eo_go_fn_0(); });
  f();

  io.run();

  check(order == std::vector<int>({1, 1}), "launched goroutine did not call its own copy of the function value");
}

void value_receiver_is_copied_at_launch() {
  asio::io_context io;
  std::vector<int> order;

  auto a = account{1};
  auto _eo_go_receiver_0 = a;
  std::vector<int>* _eo_go_arg_0_0 = &order;
  eo::go(io, [=]() mutable { return _eo_go_receiver_0.Report(_eo_go_arg_0_0); });
  a.balance = 2;

  io.run();

  check(order == std::vector<int>({1}), "value receiver was not copied at the go statement");
}

void launch_from_running_goroutine_does_not_wait() {
  asio::io_context io;
  std::vector<int> order;

  asio::co_spawn(
    io,
    [&]() -> eo::func<> {
      std::vector<int>* _eo_go_arg_0_0 = &order;
      int _eo_go_arg_0_1 = 10;
      eo::go(io, [=]() mutable { return eager_wrapper(_eo_go_arg_0_0, _eo_go_arg_0_1); });
      order.push_back(1);

      std::vector<int>* _eo_go_arg_1_0 = &order;
      int _eo_go_arg_1_1 = 20;
      eo::go(io, [=]() mutable { return plain_function(_eo_go_arg_1_0, _eo_go_arg_1_1); });
      order.push_back(2);
      co_return;
    },
    asio::detached);

  io.run();

  check(order == std::vector<int>({1, 2, 10, 3, 11, 12, 20}), "go statement waited for the launched goroutine");
}

void launch_through_local_strand(asio::io_context& io, std::vector<int>* order) {
  auto ex = asio::make_strand(io);
  std::vector<int>* _eo_go_arg_0_0 = order;
  int _eo_go_arg_0_1 = 30;
  eo::go(ex, [=]() mutable { return eager_wrapper(_eo_go_arg_0_0, _eo_go_arg_0_1); });
}

void launch_executor_may_go_out_of_scope() {
  asio::io_context io;
  std::vector<int> order;

  launch_through_local_strand(io, &order);
  io.run();

  check(order == std::vector<int>({30, 3, 31, 32}), "goroutine launched through a local strand did not run");
}

void direct_literal_runs_after_launch() {
  asio::io_context io;
  std::vector<int> order;
  auto* o = &order;
  auto label = std::string{"literal"};

  eo::go(io, [=]() mutable -> eo::func<> {
    o->push_back(static_cast<int>(label.size()));
    co_await asio::post(co_await asio::this_coro::executor, asio::use_awaitable);
    o->push_back(1);
  });

  check(order.empty(), "function literal body ran in the launching goroutine");

  io.run();

  check(order == std::vector<int>({7, 1}), "function literal body did not run after launch");
}

// Each iteration's capture is copied into the launched lambda, which must stay valid across a
// suspension after the launching function has returned.
void launch_per_iteration(asio::io_context& io, std::vector<std::string>* out) {
  for (int i = 0; i < 3; i++) {
    auto name = std::string(32, static_cast<char>('a' + i));
    eo::go(io, [=]() mutable -> eo::func<> {
      co_await asio::post(co_await asio::this_coro::executor, asio::use_awaitable);
      out->push_back(name + std::to_string(i));
    });
  }
}

void per_iteration_capture_outlives_launcher() {
  asio::io_context io;
  std::vector<std::string> out;

  launch_per_iteration(io, &out);
  io.run();

  check(out ==
      std::vector<std::string>({std::string(32, 'a') + "0", std::string(32, 'b') + "1", std::string(32, 'c') + "2"}),
    "per-iteration captures did not survive the launching function");
}

auto send_value(eo::chan<int> ch, int value) -> eo::func<> {
  co_await (ch << value);
}

void fulfill(std::promise<int>* done, int value) {
  done->set_value(value);
}

// Translated code calls the default go(F&&), which launches on runtime::execution_context. The
// outer launches come from the main thread. The nested launch inside _eo_go_fn_1 runs on a runtime
// worker, which with EOMAXPROCS=1 it shares with the goroutine it launches.
void default_runtime_launch_and_wait() {
  std::promise<int> plain_done;
  auto plain_result = plain_done.get_future();
  std::promise<int> waited_done;
  auto waited_result = waited_done.get_future();

  std::promise<int>* _eo_go_arg_0_0 = &plain_done;
  int _eo_go_arg_0_1 = 7;
  eo::go([=]() mutable { return fulfill(_eo_go_arg_0_0, _eo_go_arg_0_1); });

  auto _eo_go_fn_1 = [=](std::promise<int>* done) mutable -> eo::func<> {
    auto ch = eo::make_chan<int>();
    eo::chan<int> _eo_go_arg_2_0 = ch;
    int _eo_go_arg_2_1 = 42;
    eo::go([=]() mutable { return send_value(_eo_go_arg_2_0, _eo_go_arg_2_1); });
    auto value = co_await *ch;
    done->set_value(value);
  };
  std::promise<int>* _eo_go_arg_1_0 = &waited_done;
  eo::go([=]() mutable { return _eo_go_fn_1(_eo_go_arg_1_0); });

  check(plain_result.wait_for(std::chrono::seconds(10)) == std::future_status::ready,
    "plain goroutine on the runtime did not run");
  check(plain_result.get() == 7, "plain goroutine on the runtime received the wrong argument");
  check(waited_result.wait_for(std::chrono::seconds(10)) == std::future_status::ready,
    "goroutine waiting on a goroutine it launched on the runtime did not finish");
  check(waited_result.get() == 42, "goroutine launched on the runtime sent the wrong value");
}

int main() {
  function_and_arguments_are_evaluated_before_launch();
  non_coroutine_callee_runs_after_launch();
  plain_callee_runs_after_launch();
  function_value_is_copied_at_launch();
  value_receiver_is_copied_at_launch();
  launch_from_running_goroutine_does_not_wait();
  launch_executor_may_go_out_of_scope();
  direct_literal_runs_after_launch();
  per_iteration_capture_outlives_launcher();
  default_runtime_launch_and_wait();
}
