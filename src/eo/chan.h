// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once
#include <eo/go.h>
#include <boost/asio/experimental/concurrent_channel.hpp>

namespace eo {

template<typename T>
struct SendOp;

template<typename T>
struct ReceiveOp;

template<typename T = std::monostate>
struct chan : public boost::asio::experimental::concurrent_channel<void(boost::system::error_code, T)> {
  using boost::asio::experimental::concurrent_channel<void(boost::system::error_code, T)>::concurrent_channel;

  template<typename U>
  auto operator<<(U&& message) -> SendOp<T> {
    return {*this, T{std::forward<U>(message)}};
  }

  auto operator*() -> ReceiveOp<T> {
    return {*this};
  }
};

template<typename T = std::monostate>
auto make_chan(size_t s = 0) -> chan<T> {
  return chan<T>{runtime::executor, s};
}

template<typename T = std::monostate>
auto make_shared_chan(size_t s = 0) -> std::shared_ptr<chan<T>> {
  return std::make_shared<chan<T>>(runtime::executor, s);
}

template<typename T>
struct SendOp {
  chan<T>& ch;
  T value;

  auto ready() -> bool {
    return ch.try_send(boost::system::error_code{}, value);
  }

  auto wait() -> boost::asio::awaitable<bool> {
    co_await ch.async_send(boost::system::error_code{}, value, eoroutine);
    co_return true;
  }

  auto process() -> boost::asio::awaitable<void> {
    co_return;
  }

  auto operator*() {
    return wait();
  }
};

template<typename T>
struct ReceiveOp {
  chan<T>& ch;
  std::optional<T> processed;

  auto ready() -> bool {
    return ch.ready();
  }

  auto wait() -> boost::asio::awaitable<bool> {
    if (processed) {
      co_return true;
    }
    processed = co_await ch.async_receive(use_awaitable);
    co_return true;
  }

  auto process() -> boost::asio::awaitable<T> {
    if (processed) {
      co_return std::move(*processed);
    }
    auto res = co_await ch.async_receive(eoroutine);
    co_return !std::get<0>(res).value() ? std::get<1>(res) : T{};
  }

  auto operator*() {
    return process();
  }
};

} // namespace eo
