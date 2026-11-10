// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#include <eo/go.h>

using namespace eo;

extern func<> eo_main();

int main(int argc, char** argv) {
  // When main returns, a Go program exits at once without waiting for other goroutines. An uncaught
  // panic exits with status 2.
  boost::asio::co_spawn(runtime::execution_context, eo_main(), [](std::exception_ptr eptr) {
    if (eptr) {
      runtime::fatal_panic(eptr);
    }
    runtime::exit(0);
  });
  runtime::execution_context.join();
  runtime::exit(0);
}
