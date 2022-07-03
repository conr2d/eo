// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once
#include <boost/asio/thread_pool.hpp>

namespace eo::runtime {

extern boost::asio::thread_pool execution_context;

} // namespace eo::runtime
