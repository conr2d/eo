// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#include <eo/core.h>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>

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
  auto _eo_go_arg_0_0 = &order;
  auto _eo_go_arg_0_1 = evaluate_argument(order, 1, 10);
  auto _eo_go_arg_0_2 = evaluate_argument(order, 2, 20);
  eo::go(io, [=]() mutable { return _eo_go_fn_0(_eo_go_arg_0_0, _eo_go_arg_0_1, _eo_go_arg_0_2); });

  check(order == std::vector<int>({0, 1, 2}), "goroutine expressions were not evaluated before launch");

  io.run();

  check(order == std::vector<int>({0, 1, 2, 3, 10, 20}), "goroutine body did not run after launch");
}

void non_coroutine_callee_runs_after_launch() {
  asio::io_context io;
  std::vector<int> order;

  auto _eo_go_arg_0_0 = &order;
  auto _eo_go_arg_0_1 = 100;
  eo::go(io, [=]() mutable { return eager_wrapper(_eo_go_arg_0_0, _eo_go_arg_0_1); });

  check(order.empty(), "non-coroutine callee ran in the launching goroutine");

  io.run();

  check(order == std::vector<int>({100, 3, 101, 102}), "non-coroutine callee did not run after launch");
}

void plain_callee_runs_after_launch() {
  asio::io_context io;
  std::vector<int> order;

  auto _eo_go_arg_0_0 = &order;
  auto _eo_go_arg_0_1 = 7;
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
  auto _eo_go_arg_0_0 = &order;
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
      auto _eo_go_arg_0_0 = &order;
      auto _eo_go_arg_0_1 = 10;
      eo::go(io, [=]() mutable { return eager_wrapper(_eo_go_arg_0_0, _eo_go_arg_0_1); });
      order.push_back(1);

      auto _eo_go_arg_1_0 = &order;
      auto _eo_go_arg_1_1 = 20;
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
  auto _eo_go_arg_0_0 = order;
  auto _eo_go_arg_0_1 = 30;
  eo::go(ex, [=]() mutable { return eager_wrapper(_eo_go_arg_0_0, _eo_go_arg_0_1); });
}

void launch_executor_may_go_out_of_scope() {
  asio::io_context io;
  std::vector<int> order;

  launch_through_local_strand(io, &order);
  io.run();

  check(order == std::vector<int>({30, 3, 31, 32}), "goroutine launched through a local strand did not run");
}

int main() {
  function_and_arguments_are_evaluated_before_launch();
  non_coroutine_callee_runs_after_launch();
  plain_callee_runs_after_launch();
  function_value_is_copied_at_launch();
  value_receiver_is_copied_at_launch();
  launch_from_running_goroutine_does_not_wait();
  launch_executor_may_go_out_of_scope();
}
