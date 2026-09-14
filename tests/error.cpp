// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#include <eo/error.h>

#include <stdexcept>
#include <string>

void check(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

void test_registered_error_owns_message() {
  std::string message = "dynamic error";
  auto error = eo::user_error_registry().register_error(message);

  message.assign("overwritten");

  check(error.message() == "dynamic error", "registered error should own its message");
}

void test_registered_error_accepts_temporary_message() {
  auto error = eo::user_error_registry().register_error(std::string{"temporary error"});

  check(error.message() == "temporary error", "registered error should preserve a temporary message");
}

int main() {
  test_registered_error_owns_message();
  test_registered_error_accepts_temporary_message();
}
