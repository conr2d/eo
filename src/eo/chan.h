// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once
#include <eo/go.h>
#include <boost/asio/experimental/concurrent_channel.hpp>

#include <atomic>

namespace eo {

template<typename T>
struct send_t;

template<typename T>
struct recv_t;

template<typename T = std::monostate>
class chan {
public:
  using channel_type = boost::asio::experimental::concurrent_channel<void(boost::system::error_code, T)>;

  template<typename Executor>
    requires(!std::is_same_v<std::remove_cvref_t<Executor>, chan>)
  chan(Executor&& ex, size_t capacity = 0):
    impl(new channel_type(ex, capacity)), closed(std::make_shared<std::atomic_bool>(false)) {}

  chan(const chan&) = default;
  chan(chan&&) = default;
  auto operator=(const chan&) -> chan& = default;
  auto operator=(chan&&) -> chan& = default;

  template<typename U>
  auto operator<<(U&& message) const -> send_t<T> {
    send_t<T> op{};
    op.c = impl;
    op.value = T{std::forward<U>(message)};
    op = op.wait();
    return op;
  }

  auto operator*() const -> recv_t<T> {
    recv_t<T> op{};
    op.c = impl;
    op = op.process();
    return op;
  }

  void close() {
    bool expected = false;
    if (!closed->compare_exchange_strong(expected, true)) {
      throw std::runtime_error("panic: close of closed channel");
    }
    impl->cancel();
    impl->close();
  }

  auto& raw() {
    return *impl;
  }

  friend auto operator==(const chan<T>& lhs, const chan<T>& rhs) {
    return lhs.impl.get() == rhs.impl.get();
  }

private:
  std::shared_ptr<channel_type> impl;
  std::shared_ptr<std::atomic_bool> closed;
};

template<typename T = std::monostate>
auto make_chan(size_t s = 0) -> chan<T> {
  return chan<T>{runtime::execution_context, s};
}

template<typename T>
struct send_t : public boost::asio::awaitable<bool> {
private:
  using super = boost::asio::awaitable<bool>;

public:
  using super::super;

  auto& operator=(super&& other) {
    *static_cast<super*>(this) = std::forward<super>(other);
    return *this;
  }

  std::shared_ptr<typename chan<T>::channel_type> c;
  T value;
  bool sent{false}; // TODO: need atomic?

  // FIXME: there is no way to check whether a message can be sent via channel
  // try_send() in ready stage, but turn off sent flag during process().
  auto ready() -> bool {
    if (!sent && !c->is_open()) {
      return true;
    }
    return sent ? sent : (sent = c->try_send(boost::system::error_code{}, value));
  }

  auto wait() -> boost::asio::awaitable<bool> {
    if (!c->is_open()) {
      throw std::runtime_error("panic: send on closed channel");
    }
    auto res = co_await c->async_send(boost::system::error_code{}, value, eoroutine);
    if (std::get<0>(res)) {
      if (!c->is_open()) {
        throw std::runtime_error("panic: send on closed channel");
      }
      co_return false;
    }
    co_return (sent = true);
  }

  auto process() -> boost::asio::awaitable<bool> {
    if (!sent && !c->is_open()) {
      throw std::runtime_error("panic: send on closed channel");
    }
    co_return std::exchange(sent, false);
  }
};

template<typename T>
struct recv_t : public boost::asio::awaitable<T> {
private:
  using super = boost::asio::awaitable<T>;

public:
  using super::super;

  auto& operator=(super&& other) {
    *static_cast<super*>(this) = std::forward<super>(other);
    return *this;
  }

  std::shared_ptr<typename chan<T>::channel_type> c;
  std::optional<T> processed;

  auto ready() -> bool {
    return c->ready() || !c->is_open();
  }

  auto wait() -> boost::asio::awaitable<bool> {
    if (processed || !c->is_open()) {
      co_return true;
    }
    // XXX: caching received value not to lose by coroutine cancellation
    processed = co_await c->async_receive(use_awaitable);
    co_return true;
  }

  auto process() -> boost::asio::awaitable<T> {
    if (processed) {
      std::optional<T> ret{};
      std::swap(ret, processed);
      co_return std::move(*ret);
    }
    auto res = co_await c->async_receive(eoroutine);
    co_return !std::get<0>(res).value() ? std::get<1>(res) : T{};
  }
};

} // namespace eo

namespace std {
#if __has_include(<experimental/coroutine>)
namespace experimental {
#endif
  template<typename T>
  struct coroutine_traits<eo::send_t<T>> {
    typedef coroutine_traits<boost::asio::awaitable<void>>::promise_type promise_type;
  };

  template<typename T>
  struct coroutine_traits<eo::recv_t<T>> {
    typedef typename coroutine_traits<boost::asio::awaitable<T>>::promise_type promise_type;
  };
#if __has_include(<experimental/coroutine>)
} // namespace experimental
#endif
} // namespace std
