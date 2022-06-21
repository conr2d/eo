#pragma once
#include <eo/go.h>
#include <boost/asio/experimental/concurrent_channel.hpp>

namespace eo {

template<typename T = std::monostate>
struct chan : public boost::asio::experimental::concurrent_channel<void(boost::system::error_code, T)> {
  using boost::asio::experimental::concurrent_channel<void(boost::system::error_code, T)>::concurrent_channel;
};

template<typename T, typename U>
auto operator<<(chan<T>& channel, U&& message) {
  return channel.async_send(boost::system::error_code{}, T{std::forward<U>(message)}, eoroutine);
}

template<typename T>
auto operator*(chan<T>& channel) {
  return channel.async_receive(boost::asio::use_awaitable);
}

template<typename T = std::monostate>
auto make_chan(size_t s = 0) -> chan<T> {
  return chan<T>{runtime::executor, s};
}

struct default_chan_type: public chan<std::monostate> {
  default_chan_type(): chan(runtime::executor, 0) { close(); }

  auto operator*() {
    return async_receive(boost::asio::experimental::as_result(boost::asio::use_awaitable));
  }
};

extern default_chan_type default_chan;

} // namespace eo
