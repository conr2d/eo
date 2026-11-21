// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once
#include <eo/go.h>
#include <eo/panic.h>
#include <boost/asio/any_completion_handler.hpp>
#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/async_result.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/this_coro.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <list>
#include <memory>
#include <mutex>
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

  // The resumption of one blocked goroutine: a select, or a single send or receive. Every
  // operation that the goroutine waits on refers to the same token, and an operation completes only
  // after it claims the token. So exactly one of a select's cases completes, as with Go's
  // selectDone, and the others stay in their queues until the select removes them.
  struct wait_token {
    std::atomic<bool> done{false};
    int index = -1;
    boost::asio::any_io_executor executor;
    boost::asio::any_completion_handler<void()> handler;

    auto claim() noexcept -> bool {
      auto expected = false;
      return done.compare_exchange_strong(expected, true, std::memory_order_acq_rel);
    }

    // Resumes the goroutine with case `index`. Called once, by the operation that claimed it.
    void complete(int i) noexcept {
      index = i;
      boost::asio::post(std::move(executor), [h = std::move(handler)]() mutable { std::move(h)(); });
    }
  };

  // Suspends the running coroutine on `token`. `arm` runs once the handler is stored: it checks the
  // operations again under their locks and either completes the token or queues the waiters.
  template<typename Arm>
  auto suspend(wait_token& token, Arm arm) -> boost::asio::awaitable<void> {
    token.executor = co_await boost::asio::this_coro::executor;
    co_await boost::asio::async_initiate<const boost::asio::use_awaitable_t<>&, void()>(
      [&token, &arm](auto handler) {
        token.handler = std::move(handler);
        arm();
      },
      boost::asio::use_awaitable);
  }

  // A blocked receive. A sender or close fills it after claiming its token.
  template<typename T>
  struct recv_waiter {
    wait_token* token = nullptr;
    int index = 0;
    std::optional<T> value;
    bool queued = false;
    typename std::list<recv_waiter*>::iterator pos;
  };

  // A blocked send of `*value`. A receiver takes the value, or close sets `closed`, after claiming
  // its token.
  template<typename T>
  struct send_waiter {
    wait_token* token = nullptr;
    int index = 0;
    T* value = nullptr;
    bool closed = false;
    bool queued = false;
    typename std::list<send_waiter*>::iterator pos;
  };

  enum class send_status {
    done,
    closed,
    blocked
  };

  // The state that the copies of one Go channel share, as Go's hchan: a buffer and the queues of
  // blocked receivers and senders, all guarded by `mu`. The *_locked functions require `mu`.
  //
  // They are noexcept. Between claiming a waiter's token and completing it, a throw would leave that
  // goroutine blocked forever and its select's channels locked. A throw there can only come from
  // running out of memory, or from a throwing move of T, which no translated type has. Go's runtime
  // treats running out of memory as a fatal error, and so does Eo: the throw terminates.
  template<typename T>
  class chan_state {
  public:
    explicit chan_state(std::size_t capacity): capacity_(capacity) {}

    std::mutex mu;

    // Hands `value` to a blocked receiver, or buffers it. A queued receiver whose token another
    // operation already claimed belongs to a select that chose another case, and is skipped.
    auto send_locked(T& value) noexcept -> send_status {
      if (closed_) {
        return send_status::closed;
      }
      while (!recvq_.empty()) {
        auto* w = pop(recvq_);
        if (w->token->claim()) {
          w->value.emplace(std::move(value));
          w->token->complete(w->index);
          return send_status::done;
        }
      }
      if (buffer_.size() < capacity_) {
        buffer_.push_back(std::move(value));
        return send_status::done;
      }
      return send_status::blocked;
    }

    // A value and true, the zero value and false once the channel is closed and drained, or nullopt
    // when a receive would block. A value from a full buffer makes room for a blocked sender.
    auto recv_locked() noexcept -> std::optional<std::pair<T, bool>> {
      if (!buffer_.empty()) {
        auto value = std::move(buffer_.front());
        buffer_.pop_front();
        while (!sendq_.empty()) {
          auto* w = pop(sendq_);
          if (w->token->claim()) {
            buffer_.push_back(std::move(*w->value));
            w->token->complete(w->index);
            break;
          }
        }
        return std::pair<T, bool>{std::move(value), true};
      }
      while (!sendq_.empty()) {
        auto* w = pop(sendq_);
        if (w->token->claim()) {
          auto value = std::move(*w->value);
          w->token->complete(w->index);
          return std::pair<T, bool>{std::move(value), true};
        }
      }
      if (closed_) {
        return std::pair<T, bool>{T{}, false};
      }
      return std::nullopt;
    }

    // Closes the channel and wakes every blocked receiver, which gets the zero value, and every
    // blocked sender, which panics. Returns false if the channel was already closed.
    auto close_locked() noexcept -> bool {
      if (closed_) {
        return false;
      }
      closed_ = true;
      while (!recvq_.empty()) {
        auto* w = pop(recvq_);
        if (w->token->claim()) {
          w->token->complete(w->index);
        }
      }
      while (!sendq_.empty()) {
        auto* w = pop(sendq_);
        if (w->token->claim()) {
          w->closed = true;
          w->token->complete(w->index);
        }
      }
      return true;
    }

    void enqueue_locked(recv_waiter<T>& w) noexcept {
      w.pos = recvq_.insert(recvq_.end(), &w);
      w.queued = true;
    }

    void enqueue_locked(send_waiter<T>& w) noexcept {
      w.pos = sendq_.insert(sendq_.end(), &w);
      w.queued = true;
    }

    // Removes a waiter that is still queued, as a select does for the cases it did not choose.
    template<typename W>
    void dequeue(W& w) {
      std::lock_guard lock(mu);
      if (w.queued) {
        if constexpr (std::is_same_v<W, recv_waiter<T>>) {
          recvq_.erase(w.pos);
        } else {
          sendq_.erase(w.pos);
        }
        w.queued = false;
      }
    }

    auto closed_locked() const -> bool {
      return closed_;
    }

    auto buffered_locked() const -> std::size_t {
      return buffer_.size();
    }

  private:
    template<typename W>
    static auto pop(std::list<W*>& queue) -> W* {
      auto* w = queue.front();
      queue.pop_front();
      w->queued = false;
      return w;
    }

    std::size_t capacity_;
    bool closed_ = false;
    std::deque<T> buffer_;
    std::list<recv_waiter<T>*> recvq_;
    std::list<send_waiter<T>*> sendq_;
  };

  // v, ok := <-c on a channel that is not nil.
  template<typename T>
  auto receive_from(std::shared_ptr<chan_state<T>> c) -> boost::asio::awaitable<std::pair<T, bool>> {
    {
      std::unique_lock lock(c->mu);
      if (auto result = c->recv_locked()) {
        lock.unlock();
        co_return std::move(*result);
      }
    }
    auto token = wait_token{};
    auto waiter = recv_waiter<T>{&token, 0};
    co_await suspend(token, [&] {
      // The token is completed only after the lock is released: the coroutine can then resume at
      // once on another thread and end the channel's lifetime.
      std::unique_lock lock(c->mu);
      if (auto result = c->recv_locked()) {
        if (result->second) {
          waiter.value.emplace(std::move(result->first));
        }
        lock.unlock();
        token.claim();
        token.complete(0);
      } else {
        c->enqueue_locked(waiter);
      }
    });
    if (waiter.value) {
      co_return std::pair<T, bool>{std::move(*waiter.value), true};
    }
    co_return std::pair<T, bool>{T{}, false};
  }

} // namespace detail

template<typename T>
struct send_t;

template<typename T>
struct recv_t;

template<typename T = std::monostate>
class chan {
public:
  using state_type = detail::chan_state<T>;

  // chan<T>{} and nullptr are the nil channel.
  chan() = default;
  chan(std::nullptr_t) noexcept {}

  // A channel with `capacity` buffered values. The executor is not needed by the channel, whose
  // blocked goroutines resume on their own executors; it is kept for the callers that pass one.
  template<typename Executor>
    requires(!std::is_same_v<std::remove_cvref_t<Executor>, chan> &&
              !std::is_same_v<std::remove_cvref_t<Executor>, std::nullptr_t>)
  chan(Executor&&, size_t capacity = 0)
    : impl(std::make_shared<state_type>(capacity)), capacity_(static_cast<int64_t>(capacity)) {}

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
    op.value = T{std::forward<U>(message)};
    op = send_t<T>::send(impl, op.value);
    return op;
  }

  // <-ch, as co_await *ch, and a receive case of a select.
  auto operator*() const -> recv_t<T> {
    recv_t<T> op{};
    op.c = impl;
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
    auto closed = false;
    {
      std::lock_guard lock(impl->mu);
      closed = impl->close_locked();
    }
    if (!closed) {
      detail::panic_close_of_closed_channel();
    }
  }

  // These members are not translation forms: they serve Eo's timers and tests.

  // Sends v if that does not block: to a waiting receiver, or into the buffer. False on a nil or
  // closed channel.
  auto try_send(T v) const -> bool {
    if (!impl) {
      return false;
    }
    std::lock_guard lock(impl->mu);
    return impl->send_locked(v) == detail::send_status::done;
  }

  // Receives a value if that does not block. nullopt on a nil channel, or a closed and drained one.
  auto try_receive() const -> std::optional<T> {
    if (!impl) {
      return std::nullopt;
    }
    std::lock_guard lock(impl->mu);
    if (auto result = impl->recv_locked(); result && result->second) {
      return std::move(result->first);
    }
    return std::nullopt;
  }

  // Whether close has not been called.
  auto is_open() const -> bool {
    if (!impl) {
      return true;
    }
    std::lock_guard lock(impl->mu);
    return !impl->closed_locked();
  }

  // The number of buffered values.
  auto buffered() const -> int64_t {
    if (!impl) {
      return 0;
    }
    std::lock_guard lock(impl->mu);
    return static_cast<int64_t>(impl->buffered_locked());
  }

  auto shared_impl() const noexcept -> const std::shared_ptr<state_type>& {
    return impl;
  }

  friend auto operator==(const chan<T>& lhs, const chan<T>& rhs) {
    return lhs.impl.get() == rhs.impl.get();
  }

  // ch == nil
  friend auto operator==(const chan<T>& ch, std::nullptr_t) noexcept -> bool {
    return ch.impl == nullptr;
  }

private:
  std::shared_ptr<state_type> impl;
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
      co_return std::pair<T, bool>{T{}, false};
    }
    co_return co_await receive_from<T>(ch.shared_impl());
  }

  struct recv_ok_fn {
    template<typename T>
    auto operator()(const chan<T>& ch) const -> boost::asio::awaitable<std::pair<T, bool>> {
      return receive_ok(ch);
    }
  };

  // A case of a select whose operation can wait in a channel's queue under a shared token: a send
  // or a receive. The select locks the channels of all its cases, in a fixed order, while it checks
  // them and queues them, as Go's selectgo does.
  template<typename C>
  concept queued_case = requires(C& c, wait_token* token, int index) {
    c.state_mutex();
    { c.try_locked() } -> std::same_as<bool>;
    c.enqueue_locked(token, index);
    c.dequeue();
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

public:
  using super::super;

  auto& operator=(super&& other) {
    *static_cast<super*>(this) = std::forward<super>(other);
    return *this;
  }

  std::shared_ptr<detail::chan_state<T>> c;
  T value;
  // The send completed, or found the channel closed, which commit() reports as a panic.
  bool sent = false;
  bool closed = false;
  detail::send_waiter<T> waiter;

  // The awaitable of a send statement: it owns copies of the channel and the value.
  static auto send(std::shared_ptr<detail::chan_state<T>> c, T value) -> boost::asio::awaitable<bool> {
    if (!c) {
      co_await detail::wait_forever();
      co_return false;
    }
    {
      std::unique_lock lock(c->mu);
      auto status = c->send_locked(value);
      lock.unlock();
      if (status == detail::send_status::done) {
        co_return true;
      }
      if (status == detail::send_status::closed) {
        detail::panic_send_on_closed_channel();
      }
    }
    auto token = detail::wait_token{};
    auto waiter = detail::send_waiter<T>{&token, 0, &value};
    co_await detail::suspend(token, [&] {
      std::unique_lock lock(c->mu);
      auto status = c->send_locked(value);
      if (status == detail::send_status::blocked) {
        c->enqueue_locked(waiter);
        return;
      }
      waiter.closed = status == detail::send_status::closed;
      lock.unlock();
      token.claim();
      token.complete(0);
    });
    if (waiter.closed) {
      detail::panic_send_on_closed_channel();
    }
    co_return true;
  }

  // Sends without waiting, for a select that polls its cases. A closed channel makes the case
  // ready: choosing it panics.
  auto ready() -> bool {
    if (!c) {
      return false;
    }
    if (sent || closed) {
      return true;
    }
    std::lock_guard lock(c->mu);
    return try_locked();
  }

  void commit() {
    if (closed) {
      detail::panic_send_on_closed_channel();
    }
  }

  auto state_mutex() -> std::mutex* {
    return c ? &c->mu : nullptr;
  }

  auto try_locked() noexcept -> bool {
    if (!c) {
      return false;
    }
    switch (c->send_locked(value)) {
    case detail::send_status::done:
      sent = true;
      return true;
    case detail::send_status::closed:
      closed = true;
      return true;
    case detail::send_status::blocked:
      return false;
    }
    return false;
  }

  void enqueue_locked(detail::wait_token* token, int index) noexcept {
    if (c) {
      waiter = detail::send_waiter<T>{token, index, &value};
      c->enqueue_locked(waiter);
    }
  }

  // Removes the queued send, and records the outcome when this case was chosen.
  void dequeue() {
    if (c) {
      c->dequeue(waiter);
    }
  }

  void finish() {
    if (waiter.closed) {
      closed = true;
    } else {
      sent = true;
    }
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

  std::shared_ptr<detail::chan_state<T>> c;
  std::optional<std::pair<T, bool>> processed;
  detail::recv_waiter<T> waiter;

  // The awaitable of a receive expression: the value, or the zero value once the channel is closed
  // and drained. It owns a copy of the channel.
  static auto receive(std::shared_ptr<detail::chan_state<T>> c) -> boost::asio::awaitable<T> {
    if (!c) {
      co_await detail::wait_forever();
      co_return T{};
    }
    auto result = co_await detail::receive_from<T>(std::move(c));
    co_return std::move(result.first);
  }

  // Receives without waiting, into `processed`, for a select that polls its cases.
  auto ready() -> bool {
    if (processed) {
      return true;
    }
    if (!c) {
      return false;
    }
    std::lock_guard lock(c->mu);
    return try_locked();
  }

  auto state_mutex() -> std::mutex* {
    return c ? &c->mu : nullptr;
  }

  auto try_locked() noexcept -> bool {
    if (!c) {
      return false;
    }
    if (auto result = c->recv_locked()) {
      processed = std::move(result);
      return true;
    }
    return false;
  }

  void enqueue_locked(detail::wait_token* token, int index) noexcept {
    if (c) {
      waiter = detail::recv_waiter<T>{token, index};
      c->enqueue_locked(waiter);
    }
  }

  void dequeue() {
    if (c) {
      c->dequeue(waiter);
    }
  }

  // Records the outcome when this case was chosen: a value, or the close.
  void finish() {
    if (waiter.value) {
      processed.emplace(std::move(*waiter.value), true);
    } else {
      processed.emplace(T{}, false);
    }
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
