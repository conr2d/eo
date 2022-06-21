#pragma once
#include <boost/asio/thread_pool.hpp>

namespace eo::runtime {

extern boost::asio::thread_pool executor;

} // namespace eo::runtime
