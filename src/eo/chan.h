// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once
#include <eo/go.h>
#include <eo/panic.h>
#include <boost/asio/experimental/concurrent_channel.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/this_coro.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>
#include <variant>

namespace eo {

namespace detail {

  // Go's runtime panics on channels, whose values are runtime.plainError.
  [[noreturn]] inline void panic_close_of_nil_channel() {
    panic_plain_error("close of nil channel");
  }

  [[noreturn]] inline void panic_close_of_closed_channel() {
    panic_plain_error("close of closed channel");
  }

  [[noreturn]] inline void panic_send_on_closed_channel() {
    panic_plain_error("send on closed channel");
  }

  // An operation on a nil channel blocks forever, as in Go.
  inline auto wait_forever() -> boost::asio::awaitable<void> {
    auto executor = co_await boost::asio::this_coro::executor;
    boost::asio::steady_timer timer{executor};
    timer.expires_at((boost::asio::steady_timer::time_point::max)());
    co_await timer.async_wait(use_awaitable);
  }

  // Whether close has been called. close sets the flag before it cancels the blocked senders, so a
  // sender that wakes up from the cancellation always sees it, while asio's own is_open() can still
  // be true until close returns.
  using closed_flag = std::shared_ptr<std::atomic_bool>;

  inline auto is_closed(const closed_flag& closed) -> bool {
    return closed != nullptr && closed->load();
  }

} // namespace detail

template<typename T>
struct send_t;

template<typename T>
struct recv_t;

template<typename T = std::monostate>
class chan {
public:
  using channel_type = boost::asio::experimental::concurrent_channel<void(boost::system::error_code, T)>;

  // chan<T>{} and nullptr are the nil channel.
  chan() = default;
  chan(std::nullptr_t) noexcept {}

  template<typename Executor>
    requires(!std::is_same_v<std::remove_cvref_t<Executor>, chan> &&
              !std::is_same_v<std::remove_cvref_t<Executor>, std::nullptr_t>)
  chan(Executor&& ex, size_t capacity = 0)
    : impl(new channel_type(ex, capacity)),
      closed(std::make_shared<std::atomic_bool>(false)),
      capacity_(static_cast<int64_t>(capacity)) {}

  chan(const chan&) = default;
  chan(chan&&) = default;
  auto operator=(const chan&) -> chan& = default;
  auto operator=(chan&&) -> chan& = default;

  // ch <- v, as co_await (ch << v), and a send case of a select. The awaitable does not refer to the
  // operation object, so it stays valid when the object is moved.
  template<typename U>
  auto operator<<(U&& message) const -> send_t<T> {
    send_t<T> op{};
    op.c = impl;
    op.closed = closed;
    op.value = T{std::forward<U>(message)};
    op = send_t<T>::send(impl, closed, op.value);
    return op;
  }

  // <-ch, as co_await *ch, and a receive case of a select.
  auto operator*() const -> recv_t<T> {
    recv_t<T> op{};
    op.c = impl;
    op.closed = closed;
    op = recv_t<T>::receive(impl);
    return op;
  }

  // cap(ch). Not a translation form: eo::cap(ch) is.
  auto capacity() const noexcept -> int64_t {
    return capacity_;
  }

  void close() {
    if (!impl) {
      detail::panic_close_of_nil_channel();
    }
    bool expected = false;
    if (!closed->compare_exchange_strong(expected, true)) {
      detail::panic_close_of_closed_channel();
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

  // ch == nil
  friend auto operator==(const chan<T>& ch, std::nullptr_t) noexcept -> bool {
    return ch.impl == nullptr;
  }

private:
  std::shared_ptr<channel_type> impl;
  detail::closed_flag closed;
  int64_t capacity_ = 0;
};

namespace detail {

  template<typename T>
  struct make_chan_fn {
    // make(chan T), and make(chan T, size) for any Go integer type. A negative size, or one whose
    // buffer would exceed Go's allocation limit, panics as Go's makechan does.
    template<go_integer N = int64_t>
    auto operator()(N size = 0) const -> chan<T> {
      // Go's limit on one allocation on 64-bit Linux, less the channel header.
      constexpr uint64_t max_buffer_bytes = (uint64_t{1} << 48) - 96;
      constexpr uint64_t element_size = std::is_empty_v<T> ? 0 : sizeof(T);
      if (std::cmp_less(size, 0) || (element_size != 0 && std::cmp_greater(size, max_buffer_bytes / element_size))) {
        panic_plain_error("makechan: size out of range");
      }
      return chan<T>{runtime::execution_context, static_cast<size_t>(size)};
    }
  };

  // v, ok := <-ch: the value and true, or the zero value and false once ch is closed and drained.
  template<typename T>
  auto receive_ok(chan<T> ch) -> boost::asio::awaitable<std::pair<T, bool>> {
    if (ch == nullptr) {
      co_await wait_forever();
    }
    auto [ec, value] = co_await ch.raw().async_receive(eoroutine);
    if (!ec) {
      co_return std::pair<T, bool>{std::move(value), true};
    }
    co_return std::pair<T, bool>{T{}, false};
  }

  struct recv_ok_fn {
    template<typename T>
    auto operator()(const chan<T>& ch) const -> boost::asio::awaitable<std::pair<T, bool>> {
      return receive_ok(ch);
    }
  };

} // namespace detail

// Function objects, called qualified, like the string helpers (see <eo/string.h>).
template<typename T = std::monostate>
inline constexpr detail::make_chan_fn<T> make_chan{};
inline constexpr detail::recv_ok_fn recv_ok{};

template<typename T>
struct send_t : public boost::asio::awaitable<bool> {
private:
  using super = boost::asio::awaitable<bool>;

  static auto wait_nil() -> boost::asio::awaitable<void> {
    return detail::wait_forever();
  }

  auto closed_now() const -> bool {
    return detail::is_closed(closed);
  }

public:
  using super::super;

  auto& operator=(super&& other) {
    *static_cast<super*>(this) = std::forward<super>(other);
    return *this;
  }

  std::shared_ptr<typename chan<T>::channel_type> c;
  detail::closed_flag closed;
  T value;
  bool sent{false}; // TODO: need atomic?

  // The awaitable of a send statement: it owns copies of the channel and the value.
  static auto send(std::shared_ptr<typename chan<T>::channel_type> c,
    detail::closed_flag closed,
    T value) -> boost::asio::awaitable<bool> {
    if (!c) {
      co_await detail::wait_forever();
      co_return false;
    }
    if (detail::is_closed(closed)) {
      detail::panic_send_on_closed_channel();
    }
    auto res = co_await c->async_send(boost::system::error_code{}, std::move(value), eoroutine);
    if (std::get<0>(res)) {
      // Only close cancels a send statement.
      detail::panic_send_on_closed_channel();
    }
    co_return true;
  }

  // FIXME: there is no way to check whether a message can be sent via channel
  // try_send() in ready stage, but turn off sent flag during process().
  auto ready() -> bool {
    if (!c) {
      return false;
    }
    if (!sent && closed_now()) {
      return true;
    }
    return sent ? sent : (sent = c->try_send(boost::system::error_code{}, value));
  }

  void commit() {
    if (!c) {
      return;
    }
    if (!sent && closed_now()) {
      detail::panic_send_on_closed_channel();
    }
  }

  auto wait() -> boost::asio::awaitable<bool> {
    if (!c) {
      co_await wait_nil();
      co_return false;
    }
    if (closed_now()) {
      detail::panic_send_on_closed_channel();
    }
    auto res = co_await c->async_send(boost::system::error_code{}, value, eoroutine);
    if (std::get<0>(res)) {
      if (closed_now()) {
        detail::panic_send_on_closed_channel();
      }
      co_return false;
    }
    co_return (sent = true);
  }

  auto process() -> boost::asio::awaitable<bool> {
    if (!c) {
      co_await wait_nil();
      co_return false;
    }
    if (!sent && closed_now()) {
      detail::panic_send_on_closed_channel();
    }
    co_return std::exchange(sent, false);
  }
};

template<typename T>
struct recv_t : public boost::asio::awaitable<T> {
private:
  using super = boost::asio::awaitable<T>;

  static auto wait_nil() -> boost::asio::awaitable<void> {
    return detail::wait_forever();
  }

  auto closed_now() const -> bool {
    return detail::is_closed(closed);
  }

public:
  using super::super;

  auto& operator=(super&& other) {
    *static_cast<super*>(this) = std::forward<super>(other);
    return *this;
  }

  std::shared_ptr<typename chan<T>::channel_type> c;
  detail::closed_flag closed;
  std::optional<std::pair<T, bool>> processed;

  // The awaitable of a receive expression: the value, or the zero value once the channel is closed
  // and drained. It owns a copy of the channel.
  static auto receive(std::shared_ptr<typename chan<T>::channel_type> c) -> boost::asio::awaitable<T> {
    if (!c) {
      co_await detail::wait_forever();
      co_return T{};
    }
    auto res = co_await c->async_receive(eoroutine);
    if (!std::get<0>(res)) {
      co_return std::move(std::get<1>(res));
    }
    co_return T{};
  }

  auto ready() -> bool {
    if (processed) {
      return true;
    }
    if (!c) {
      return false;
    }
    if (c->try_receive([this](boost::system::error_code ec, T value) {
          if (!ec) {
            processed.emplace(std::move(value), true);
          }
        })) {
      return true;
    }
    if (closed_now()) {
      processed.emplace(T{}, false);
      return true;
    }
    return false;
  }

  auto wait() -> boost::asio::awaitable<bool> {
    if (processed) {
      co_return true;
    }
    if (!c) {
      co_await wait_nil();
      co_return false;
    }
    if (closed_now()) {
      processed.emplace(T{}, false);
      co_return true;
    }
    // XXX: caching received value not to lose by coroutine cancellation
    auto res = co_await c->async_receive(eoroutine);
    if (!std::get<0>(res)) {
      processed.emplace(std::move(std::get<1>(res)), true);
      co_return true;
    }
    if (closed_now()) {
      processed.emplace(T{}, false);
      co_return true;
    }
    co_return false;
  }

  auto get2() -> std::pair<T, bool> {
    if (!processed) {
      return {T{}, false};
    }
    auto result = std::move(*processed);
    processed.reset();
    return result;
  }

  auto get() -> T {
    return get2().first;
  }

  auto process() -> boost::asio::awaitable<T> {
    if (processed) {
      co_return get();
    }
    if (!c) {
      co_await wait_nil();
      co_return T{};
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
