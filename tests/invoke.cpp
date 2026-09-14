// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#include <eo/invoke.h>

#include <stdexcept>
#include <string>

void check(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

auto return_value() -> eo::func<int> {
  co_return 42;
}

auto throw_void() -> eo::func<> {
  throw std::runtime_error("void failure");
  co_return;
}

auto throw_value() -> eo::func<int> {
  throw std::runtime_error("value failure");
  co_return 0;
}

void test_invoke_returns_value() {
  check(eo::invoke(return_value()) == 42, "invoke should return coroutine value");
}

void test_invoke_propagates_void_exception() {
  try {
    eo::invoke(throw_void());
  } catch (const std::runtime_error& error) {
    check(std::string{error.what()} == "void failure", "invoke returned wrong void exception");
    return;
  }

  throw std::runtime_error("invoke swallowed void coroutine exception");
}

void test_invoke_propagates_value_exception() {
  try {
    eo::invoke(throw_value());
  } catch (const std::runtime_error& error) {
    check(std::string{error.what()} == "value failure", "invoke returned wrong value exception");
    return;
  }

  throw std::runtime_error("invoke swallowed value coroutine exception");
}

int main() {
  test_invoke_returns_value();
  test_invoke_propagates_void_exception();
  test_invoke_propagates_value_exception();
}
