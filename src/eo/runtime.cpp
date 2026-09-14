// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#include <eo/core.h>

#include <algorithm>
#include <charconv>
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

} // namespace eo::runtime
