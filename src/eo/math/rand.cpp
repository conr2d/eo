// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#include <eo/math/rand.h>
#include <random>

namespace eo::math::rand {

thread_local std::mt19937 rng = []() {
  std::mt19937 rng(std::random_device{}());
  rng.discard(700000);
  return rng;
}();

auto Intn(int n) -> int {
  if (n <= 0) {
    throw std::runtime_error("invalid argument to Intn");
  }
  return std::uniform_int_distribution(0, n - 1)(rng);
}

auto Int63n(int64_t n) -> int64_t {
  if (n <= 0) {
    throw std::runtime_error("invalid argument to Int63n");
  }
  return std::uniform_int_distribution<int64_t>(0, n - 1)(rng);
}

} // namespace eo::math::rand
