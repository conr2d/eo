#pragma once
#include <eo/core.h>

#include <boost/asio/steady_timer.hpp>
#include <boost/asio/this_coro.hpp>
#include <boost/asio/use_awaitable.hpp>

namespace eo::time {

template<class Rep, class Period>
func<> sleep(const std::chrono::duration<Rep, Period>& sleep_duration) {
  co_await boost::asio::steady_timer((co_await boost::asio::this_coro::executor), sleep_duration)
    .async_wait(boost::asio::use_awaitable);
}

} // namespace eo::time
