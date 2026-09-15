// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#include <eo/math/rand.h>

#include <stdexcept>

void check(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

template<typename F>
void check_throws(F&& function, const char* message) {
  try {
    function();
  } catch (const std::runtime_error&) {
    return;
  }
  throw std::runtime_error(message);
}

void test_Intn_range() {
  for (int i = 0; i < 1000; ++i) {
    auto value = eo::math::rand::Intn(7);
    check(0 <= value && value < 7, "Intn result must remain within [0, n)");
  }
}

void test_Int63n_range() {
  for (int i = 0; i < 1000; ++i) {
    auto value = eo::math::rand::Int63n(7);
    check(0 <= value && value < 7, "Int63n result must remain within [0, n)");
  }
}

void test_non_positive_bounds_throw() {
  check_throws([]() { eo::math::rand::Intn(0); }, "Intn must reject zero bounds");
  check_throws([]() { eo::math::rand::Intn(-1); }, "Intn must reject negative bounds");
  check_throws([]() { eo::math::rand::Int63n(0); }, "Int63n must reject zero bounds");
  check_throws([]() { eo::math::rand::Int63n(-1); }, "Int63n must reject negative bounds");
}

int main() {
  test_Intn_range();
  test_Int63n_range();
  test_non_positive_bounds_throw();
}
