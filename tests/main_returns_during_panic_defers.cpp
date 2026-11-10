// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
// When eo_main returns while another goroutine is still running deferred calls for a panic, the
// runtime waits for them, as Go's runtime.main does, and the panic then ends the process with status
// 2. eo_main returns only once the deferred call has started; that call then takes 20 ms, and the
// runtime waits up to about 100 ms, so the test is reliable rather than deterministic.
#include <eo/fmt.h>
#include <eo/time.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>

using namespace eo;

func<> eo_main() {
  auto started = std::make_shared<std::atomic<bool>>(false);
  go([started]() -> func<> {
    eo_defer_scope;
    try {
      eo_defer([&] {
        *started = true;
        std::this_thread::sleep_for(std::chrono::milliseconds{20});
      });
      panic("in goroutine");
      eo_defer_run;
    } catch (...) {
      eo_defer_panic;
    }
    co_return;
  });
  while (!*started) {
    co_await time::Sleep(std::chrono::milliseconds{1});
  }
  fmt::Println("main returns");
}
