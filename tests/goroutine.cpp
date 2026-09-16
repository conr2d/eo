// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#include <eo/core.h>

#include <boost/asio/io_context.hpp>

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

void function_and_arguments_are_evaluated_before_launch() {
  asio::io_context io;
  std::vector<int> order;

  auto _eo_go_fn_0 = evaluate_function(order);
  auto _eo_go_arg_0_0 = &order;
  auto _eo_go_arg_0_1 = evaluate_argument(order, 1, 10);
  auto _eo_go_arg_0_2 = evaluate_argument(order, 2, 20);
  eo::go(io, _eo_go_fn_0(_eo_go_arg_0_0, _eo_go_arg_0_1, _eo_go_arg_0_2));

  check(order == std::vector<int>({0, 1, 2}), "goroutine expressions were not evaluated before launch");

  io.run();

  check(order == std::vector<int>({0, 1, 2, 3, 10, 20}), "goroutine body did not run after launch");
}

int main() {
  function_and_arguments_are_evaluated_before_launch();
}
