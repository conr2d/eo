// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#include <eo/core.h>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/use_future.hpp>

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

void append(std::vector<int>* values, int value) {
  values->push_back(value);
}

void append_pair(std::vector<int>* values, int first, int second) {
  values->push_back(first);
  values->push_back(second);
}

using append_pair_fn = void (*)(std::vector<int>*, int, int);

auto evaluate_function(std::vector<int>& order) -> append_pair_fn {
  order.push_back(0);
  return append_pair;
}

auto evaluate_values(std::vector<int>& order, std::vector<int>& values) -> std::vector<int>* {
  order.push_back(1);
  return &values;
}

int evaluate_int(std::vector<int>& order, int marker, int value) {
  order.push_back(marker);
  return value;
}

struct receiver {
  std::vector<int>* values;

  void Add(int value) {
    values->push_back(value);
  }
};

auto evaluate_receiver(std::vector<int>& order, std::vector<int>& values) -> receiver {
  order.push_back(0);
  return receiver{&values};
}

struct lifetime_probe {
  explicit lifetime_probe(bool& alive): alive(alive) {
    alive = true;
  }

  ~lifetime_probe() {
    alive = false;
  }

  bool& alive;
};

void register_nested(std::vector<int>& values) {
  eo_defer_scope;
  {
    eo_defer([&] { append(&values, 1); });
    check(values.empty(), "defer ran at block exit instead of function exit");
  }
  check(values.empty(), "nested defer ran before function exit");
  eo_defer_run;
}

void register_loop(std::vector<int>& values) {
  eo_defer_scope;
  for (int i = 0; i < 3; ++i) {
    auto _eo_defer_arg_0_0 = &values;
    auto _eo_defer_arg_0_1 = i;
    eo_defer([=] { append(_eo_defer_arg_0_0, _eo_defer_arg_0_1); });
  }
  eo_defer_run;
}

void direct_expressions_are_evaluated_in_source_order(std::vector<int>& order, std::vector<int>& values) {
  eo_defer_scope;
  auto _eo_defer_fn_0 = evaluate_function(order);
  auto _eo_defer_arg_0_0 = evaluate_values(order, values);
  auto _eo_defer_arg_0_1 = evaluate_int(order, 2, 10);
  auto _eo_defer_arg_0_2 = evaluate_int(order, 3, 20);
  eo_defer([=] { _eo_defer_fn_0(_eo_defer_arg_0_0, _eo_defer_arg_0_1, _eo_defer_arg_0_2); });

  check(order == std::vector<int>({0, 1, 2, 3}), "defer expressions were not evaluated in source order");
  check(values.empty(), "deferred call ran during registration");
  eo_defer_run;
}

void method_receiver_is_saved_at_registration(std::vector<int>& order, std::vector<int>& values) {
  eo_defer_scope;
  auto _eo_defer_receiver_0 = evaluate_receiver(order, values);
  auto _eo_defer_arg_0_0 = evaluate_int(order, 1, 7);
  eo_defer([=]() mutable { _eo_defer_receiver_0.Add(_eo_defer_arg_0_0); });

  check(order == std::vector<int>({0, 1}), "method receiver and argument were not evaluated in source order");
  eo_defer_run;
}

void closure_observes_later_variable_value(std::vector<int>& values) {
  eo_defer_scope;
  int value = 1;
  eo_defer([&] { values.push_back(value); });
  value = 7;
  eo_defer_run;
}

void closure_runs_before_local_destruction(bool& alive, bool& observed_alive) {
  eo_defer_scope;
  lifetime_probe local{alive};
  eo_defer([&] { observed_alive = alive; });
  eo_defer_run;
}

int evaluate_return(std::vector<int>& order) {
  order.push_back(1);
  return 42;
}

int return_expression_precedes_defer(std::vector<int>& order) {
  eo_defer_scope;
  eo_defer([&] { order.push_back(2); });

  auto _eo_return_0 = evaluate_return(order);
  eo_defer_run;
  return _eo_return_0;
}

auto coroutine_defer(std::vector<int>& values) -> eo::func<> {
  eo_defer_scope;
  auto _eo_defer_arg_0_0 = &values;
  auto _eo_defer_arg_0_1 = 3;
  eo_defer([=] { append(_eo_defer_arg_0_0, _eo_defer_arg_0_1); });
  eo_defer_run;
  co_return;
}

int main() {
  std::vector<int> values;

  register_nested(values);
  check(values == std::vector<int>{1}, "function-scoped defer did not run at function exit");

  values.clear();
  register_loop(values);
  check(values == std::vector<int>({2, 1, 0}), "loop defers did not run in LIFO order");

  values.clear();
  std::vector<int> order;
  direct_expressions_are_evaluated_in_source_order(order, values);
  check(values == std::vector<int>({10, 20}), "deferred direct call did not preserve saved arguments");

  values.clear();
  order.clear();
  method_receiver_is_saved_at_registration(order, values);
  check(values == std::vector<int>{7}, "deferred method call did not preserve its saved receiver");

  values.clear();
  closure_observes_later_variable_value(values);
  check(values == std::vector<int>{7}, "deferred closure did not preserve reference capture semantics");

  bool alive = false;
  bool observed_alive = false;
  closure_runs_before_local_destruction(alive, observed_alive);
  check(observed_alive, "defer ran after a referenced local was destroyed");
  check(!alive, "local lifetime probe did not destruct after function exit");

  order.clear();
  check(return_expression_precedes_defer(order) == 42, "translated return value changed");
  check(order == std::vector<int>({1, 2}), "defer ran before the return expression was evaluated");

  values.clear();
  asio::io_context io;
  auto result = asio::co_spawn(io, coroutine_defer(values), asio::use_future);
  io.run();
  result.get();
  check(values == std::vector<int>{3}, "coroutine defer did not run before completion");
}
