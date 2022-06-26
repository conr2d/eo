#pragma once
#include <eo/core.h>

#include <eo/time/ticker.h>
#include <eo/time/timer.h>

#include <boost/asio/this_coro.hpp>

namespace eo::time {

template<class Rep, class Period>
func<> sleep(const std::chrono::duration<Rep, Period>& d) {
  using namespace boost::asio;
  co_await steady_timer((co_await this_coro::executor), d).async_wait(use_awaitable);
}

} // namespace eo::time
