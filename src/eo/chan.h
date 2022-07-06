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
class chan {
public:
  using channel_type = boost::asio::experimental::concurrent_channel<void(boost::system::error_code, T)>;

  template<typename Executor>
  chan(Executor&& ex, size_t capacity): impl(new channel_type(ex, capacity)) {}

  template<typename U>
  auto operator<<(U&& message) const -> SendOp<T> {
    return {impl, T{std::forward<U>(message)}};
  }

  auto operator*() const -> ReceiveOp<T> {
    return {impl};
  }

  void close() {
    if (!impl->is_open()) {
      throw std::runtime_error("panic: close of closed channel");
    }
    impl->close();
  }

  template<typename... Args>
  auto try_send(Args&&... args) {
    return impl->try_send(std::forward<Args>(args)...);
  }

private:
  std::shared_ptr<channel_type> impl;
};

template<typename T = std::monostate>
auto make_chan(size_t s = 0) -> chan<T> {
  return chan<T>{runtime::execution_context, s};
}

template<typename T>
struct SendOp {
  std::shared_ptr<typename chan<T>::channel_type> ch;
  T value;
  bool sent{false}; // TODO: need atomic?

  // FIXME: there is no way to check whether a message can be sent via channel
  // try_send() in ready stage, but turn off sent flag during process().
  auto ready() -> bool {
    if (!sent && !ch->is_open()) {
      return true;
    }
    return sent ? sent : (sent = ch->try_send(boost::system::error_code{}, value));
  }

  auto wait() -> boost::asio::awaitable<bool> {
    if (!ch->is_open()) {
      throw std::runtime_error("panic: send on closed channel");
    }
    auto res = co_await ch->async_send(boost::system::error_code{}, value, eoroutine);
    co_return (sent = !std::get<0>(res).value());
  }

  auto process() -> boost::asio::awaitable<bool> {
    if (!sent && !ch->is_open()) {
      throw std::runtime_error("panic: send on closed channel");
    }
    co_return std::exchange(sent, false);
  }

  auto operator*() {
    return wait();
  }
};

template<typename T>
struct ReceiveOp {
  std::shared_ptr<typename chan<T>::channel_type> ch;
  std::optional<T> processed;

  auto ready() -> bool {
    return ch->ready() || !ch->is_open();
  }

  auto wait() -> boost::asio::awaitable<bool> {
    if (processed || !ch->is_open()) {
      co_return true;
    }
    // XXX: caching received value not to lose by coroutine cancellation
    processed = co_await ch->async_receive(use_awaitable);
    co_return true;
  }

  auto process() -> boost::asio::awaitable<T> {
    if (processed) {
      std::optional<T> ret{};
      std::swap(ret, processed);
      co_return std::move(*ret);
    }
    auto res = co_await ch->async_receive(eoroutine);
    co_return !std::get<0>(res).value() ? std::get<1>(res) : T{};
  }

  auto operator*() {
    return process();
  }
};

} // namespace eo
