// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#include <eo/go.h>

#include <cstdlib>

using namespace eo;

extern func<> eo_main();

// A translated program defines eo_init when its package has variable initializers or init
// functions. This weak definition stands in for it otherwise.
[[gnu::weak]] func<> eo_init() {
  co_return;
}

namespace {

// Initializes the package, then runs main, both in the main goroutine, as Go does.
func<> run_main() {
  co_await eo_init();
  co_await eo_main();
}

} // namespace

int main(int argc, char** argv) {
  // When main returns, a Go program exits at once without waiting for other goroutines. An uncaught
  // panic exits with status 2.
  boost::asio::co_spawn(runtime::execution_context, run_main(), [](std::exception_ptr eptr) {
    if (eptr) {
      runtime::fatal_panic(eptr);
    }
    runtime::main_returned();
  });
  // Not reached: the pending run_main keeps the pool's work count above zero, so join() returns
  // only after the handler above has ended the process. If every goroutine blocks forever, the
  // program hangs instead of reporting Go's "all goroutines are asleep" deadlock.
  runtime::execution_context.join();
  std::abort();
}
