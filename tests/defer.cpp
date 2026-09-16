// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#include <eo/core.h>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/use_future.hpp>

#include <stdexcept>
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

void register_nested(std::vector<int>& values) {
  eo_defer_scope;
  {
    eo_defer(append, &values, 1);
    check(values.empty(), "defer ran at block exit instead of function exit");
  }
  check(values.empty(), "nested defer ran before function exit");
}

void register_loop(std::vector<int>& values) {
  eo_defer_scope;
  for (int i = 0; i < 3; ++i) {
    eo_defer(append, &values, i);
  }
}

void direct_arguments_are_evaluated_at_registration(std::vector<int>& values) {
  eo_defer_scope;
  int value = 1;
  eo_defer(append, &values, value++);
  value = 9;
  check(value == 9, "test setup failed");
}

void closure_observes_later_variable_value(std::vector<int>& values) {
  eo_defer_scope;
  int value = 1;
  eo_defer([&] { values.push_back(value); });
  value = 7;
}

auto coroutine_defer(std::vector<int>& values) -> eo::func<> {
  eo_defer_scope;
  eo_defer(append, &values, 3);
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
  direct_arguments_are_evaluated_at_registration(values);
  check(values == std::vector<int>{1}, "defer arguments were not captured at registration");

  values.clear();
  closure_observes_later_variable_value(values);
  check(values == std::vector<int>{7}, "deferred closure did not preserve reference capture semantics");

  values.clear();
  asio::io_context io;
  auto result = asio::co_spawn(io, coroutine_defer(values), asio::use_future);
  io.run();
  result.get();
  check(values == std::vector<int>{3}, "coroutine defer did not run before completion");
}
