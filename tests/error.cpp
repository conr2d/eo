// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#include <eo/error.h>

#include <set>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

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

void test_registered_errors_are_thread_safe() {
  constexpr int thread_count = 8;
  constexpr int errors_per_thread = 64;
  std::vector<std::vector<eo::Error>> errors(thread_count);
  std::vector<std::thread> threads;

  for (int thread = 0; thread < thread_count; ++thread) {
    threads.emplace_back([thread, &errors] {
      auto& registered = errors[thread];
      registered.reserve(errors_per_thread);
      for (int index = 0; index < errors_per_thread; ++index) {
        registered.push_back(eo::user_error_registry().register_error(
          "concurrent error " + std::to_string(thread) + ":" + std::to_string(index)));
      }
    });
  }

  for (auto& thread : threads) {
    thread.join();
  }

  std::set<int> conditions;
  for (int thread = 0; thread < thread_count; ++thread) {
    for (int index = 0; index < errors_per_thread; ++index) {
      const auto& error = errors[thread][index];
      check(conditions.insert(error.value()).second, "concurrent registrations should have unique conditions");
      check(error.message() == "concurrent error " + std::to_string(thread) + ":" + std::to_string(index),
        "concurrent registration should preserve its message");
    }
  }
}

int main() {
  test_registered_error_owns_message();
  test_registered_error_accepts_temporary_message();
  test_registered_errors_are_thread_safe();
}
