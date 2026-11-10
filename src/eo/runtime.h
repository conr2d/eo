// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once
#include <boost/asio/thread_pool.hpp>

#include <exception>

namespace eo::runtime {

extern boost::asio::thread_pool execution_context;

// Ends the process at once with the given status, from any thread, as Go does when main returns or
// os.Exit is called. Pending C++ stream and stdio output is flushed; nothing else runs: no deferred
// calls, no other goroutines, no static destructors.
[[noreturn]] void exit(int code);

// Reports an uncaught panic on stderr and ends the process with status 2, as Go does for a panic
// that no goroutine recovers. If several goroutines panic at once, only the first one is reported.
[[noreturn]] void fatal_panic(std::exception_ptr eptr);

namespace detail {

  // Completion handler for goroutines started by go(): a panic ends the program.
  struct goroutine_completion {
    template<typename... R>
    void operator()(std::exception_ptr eptr, R&&...) const {
      if (eptr) {
        fatal_panic(eptr);
      }
    }
  };

} // namespace detail

} // namespace eo::runtime
