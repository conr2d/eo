// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#include <eo/core.h>
#include <thread>

namespace eo::runtime {

boost::asio::thread_pool execution_context = []() {
  auto maxprocs = std::thread::hardware_concurrency();
  if (auto env = std::getenv("EOMAXPROCS")) {
    auto _maxprocs = std::stoull(env);
    if (_maxprocs >= 1) {
      maxprocs = _maxprocs;
    }
  }
  return boost::asio::thread_pool{maxprocs};
}();

} // namespace eo::runtime
