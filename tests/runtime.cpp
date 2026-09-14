// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#include <eo/invoke.h>

#include <stdexcept>

void check(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

auto return_value() -> eo::func<int> {
  co_return 7;
}

int main() {
  check(eo::invoke(return_value()) == 7, "runtime worker pool should execute coroutines");
}
