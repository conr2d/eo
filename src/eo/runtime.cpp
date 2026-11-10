// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#include <eo/core.h>

#include <boost/exception/diagnostic_information.hpp>

#include <algorithm>
#include <charconv>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <string_view>
#include <thread>

namespace eo::runtime {

boost::asio::thread_pool execution_context = []() {
  auto maxprocs = std::max<std::size_t>(1, std::thread::hardware_concurrency());
  if (auto env = std::getenv("EOMAXPROCS")) {
    std::size_t configured{};
    std::string_view value{env};
    auto [ptr, ec] = std::from_chars(value.data(), value.data() + value.size(), configured);
    if (ec == std::errc{} && ptr == value.data() + value.size() && configured >= 1) {
      maxprocs = configured;
    }
  }
  return boost::asio::thread_pool{maxprocs};
}();

void exit(int code) {
  // std::exit would run static destructors, and the thread_pool destructor joins worker threads
  // that may include the caller or be blocked in user code.
  std::cout.flush();
  std::cerr.flush();
  std::fflush(nullptr);
  std::_Exit(code);
}

void fatal_panic(std::exception_ptr eptr) {
  // Never unlocked: a later panic on another thread waits here until the first one exits.
  static std::mutex reporting;
  reporting.lock();
  try {
    std::rethrow_exception(eptr);
  } catch (const boost::exception& e) {
    std::cerr << boost::diagnostic_information(e) << std::endl;
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
  } catch (...) {
    std::cerr << "panic: unknown exception" << std::endl;
  }
  exit(2);
}

} // namespace eo::runtime
