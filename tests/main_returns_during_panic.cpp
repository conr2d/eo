// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
// When eo_main returns while another goroutine is panicking, the panic ends the process with status
// 2, as in Go. The panic's what() is slow, which keeps the panic in progress while eo_main returns.
// The test expects status 2; a 400 ms margin makes it reliable, not deterministic.
#include <eo/fmt.h>
#include <eo/time.h>

#include <chrono>
#include <exception>
#include <thread>

using namespace eo;

struct slow_panic : std::exception {
  auto what() const noexcept -> const char* override {
    std::this_thread::sleep_for(std::chrono::milliseconds{500});
    return "panic: slow";
  }
};

func<> eo_main() {
  go([]() -> func<> {
    throw slow_panic{};
    co_return;
  });
  co_await time::Sleep(std::chrono::milliseconds{100});
  fmt::Println("main returns");
}
