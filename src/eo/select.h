// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once
#include <eo/chan.h>
#include <eo/go.h>

#include <algorithm>
#include <functional>
#include <mutex>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <vector>

namespace eo {

namespace math::rand {
  extern auto Int63n(int64_t n) -> int64_t;
} // namespace math::rand

template<typename T, typename... Ts>
struct Select {
  Select(T&& t, Ts&&... ts): cases(std::make_tuple(std::forward<T>(t), std::forward<Ts>(ts)...)) {}

  std::tuple<T, Ts...> cases;

private:
  bool executed = false;
  bool receive_consumed = false;
  std::optional<size_t> selected_index;

  void begin_selection() {
    if (executed) {
      throw std::runtime_error("Select can only be executed once");
    }
    executed = true;
  }

  template<size_t I = 0>
  auto ready(size_t index) -> bool {
    if constexpr (I < sizeof...(Ts) + 1) {
      if (I == index) {
        return std::get<I>(cases).ready();
      }
      return ready<I + 1>(index);
    }
    return false;
  }

  template<size_t I = 0>
  void commit(size_t index) {
    if constexpr (I < sizeof...(Ts) + 1) {
      if (I == index) {
        if constexpr (requires { std::get<I>(cases).commit(); }) {
          std::get<I>(cases).commit();
        }
        return;
      }
      commit<I + 1>(index);
    }
  }

  auto randomized_indices() -> std::vector<size_t> {
    std::vector<size_t> indices(sizeof...(Ts) + 1);
    std::iota(indices.begin(), indices.end(), 0);
    for (size_t i = indices.size(); i > 1; --i) {
      auto j = static_cast<size_t>(math::rand::Int63n(i));
      std::swap(indices[i - 1], indices[j]);
    }
    return indices;
  }

  template<size_t I>
  void validate_receive() {
    static_assert(I < sizeof...(Ts) + 1, "Select receive index out of range");
    if (!selected_index || *selected_index != I) {
      throw std::runtime_error("Select receive accessor does not match selected case");
    }
    if (receive_consumed) {
      throw std::runtime_error("Select receive result already consumed");
    }
    receive_consumed = true;
  }

public:
  Select(const Select&) = delete;
  Select(Select&&) = delete;
  auto operator=(const Select&) -> Select& = delete;
  auto operator=(Select&&) -> Select& = delete;

  auto try_index() -> int {
    begin_selection();
    for (auto index : randomized_indices()) {
      if (ready(index)) {
        commit(index);
        selected_index = index;
        return static_cast<int>(index);
      }
    }
    selected_index.reset();
    return -1;
  }

  auto index() -> boost::asio::awaitable<int> {
    begin_selection();
    for (auto index : randomized_indices()) {
      if (ready(index)) {
        commit(index);
        selected_index = index;
        co_return static_cast<int>(index);
      }
    }

    if constexpr ((detail::queued_case<T> && ... && detail::queued_case<Ts>)) {
      co_return co_await wait_queued();
    } else if constexpr ((detail::queued_case<T> || ... || detail::queued_case<Ts>)) {
      static_assert(sizeof(T) == 0, "a blocking Select cannot mix channel cases with other case types");
      co_return -1;
    } else {
      // Go's select has only channel cases. One case of another type, which tests use, waits alone.
      static_assert(!sizeof...(Ts), "a blocking Select of several cases needs channel cases");
      co_await std::get<0>(cases).wait();
      selected_index = 0;
      co_return 0;
    }
  }

private:
  template<typename F>
  void for_each_case(F&& f) {
    std::apply([&](auto&... c) { (f(c), ...); }, cases);
  }

  // Waits until exactly one case completes, as Go's selectgo does. Under the locks of every
  // channel, taken in address order, the select checks its cases again in a random order, and
  // otherwise queues one waiter per case under a shared token. The operation that claims the token
  // completes its case, and the others can no longer complete theirs, so the select removes them
  // from their queues when it resumes.
  auto wait_queued() -> boost::asio::awaitable<int> {
    auto mutexes = std::vector<std::mutex*>{};
    for_each_case([&](auto& c) {
      if (auto* m = c.state_mutex()) {
        mutexes.push_back(m);
      }
    });
    if (mutexes.empty()) {
      // Every case is a nil channel: the select blocks forever.
      co_await detail::wait_forever();
    }
    std::sort(mutexes.begin(), mutexes.end(), std::less<>{});
    mutexes.erase(std::unique(mutexes.begin(), mutexes.end()), mutexes.end());

    auto token = detail::wait_token{};
    auto chosen_now = false;
    co_await detail::suspend(token, [&] {
      // Once a waiter is queued and its channel unlocked, another goroutine can complete the token
      // and resume this coroutine on another thread. So the arm unlocks from its own copy of the
      // list, and completes the token itself only as its last action.
      auto locked = mutexes;
      auto order = randomized_indices();
      for (auto* m : locked) {
        m->lock();
      }
      // Nothing from here to the unlocks throws: the channel core is noexcept (see chan_state).
      auto ready = std::optional<size_t>{};
      for (auto index : order) {
        if (try_locked(index)) {
          ready = index;
          break;
        }
      }
      if (ready) {
        chosen_now = true;
      } else {
        auto index = 0;
        for_each_case([&](auto& c) { c.enqueue_locked(&token, index++); });
      }
      for (auto it = locked.rbegin(); it != locked.rend(); ++it) {
        (*it)->unlock();
      }
      if (ready) {
        token.claim();
        token.complete(static_cast<int>(*ready));
      }
    });

    auto index = static_cast<size_t>(token.index);
    if (!chosen_now) {
      for_each_case([](auto& c) { c.dequeue(); });
      finish(index);
    }
    commit(index);
    selected_index = index;
    co_return static_cast<int>(index);
  }

  template<size_t I = 0>
  auto try_locked(size_t index) -> bool {
    if constexpr (I < sizeof...(Ts) + 1) {
      if (I == index) {
        return std::get<I>(cases).try_locked();
      }
      return try_locked<I + 1>(index);
    }
    return false;
  }

  template<size_t I = 0>
  void finish(size_t index) {
    if constexpr (I < sizeof...(Ts) + 1) {
      if (I == index) {
        std::get<I>(cases).finish();
        return;
      }
      finish<I + 1>(index);
    }
  }

public:
  template<size_t I>
  auto recv() {
    if constexpr (requires { std::get<I>(cases).get(); }) {
      validate_receive<I>();
      return std::get<I>(cases).get();
    } else {
      static_assert(I != I, "Select receive accessor requires a receive case");
    }
  }

  template<size_t I>
  auto recv2() {
    if constexpr (requires { std::get<I>(cases).get2(); }) {
      validate_receive<I>();
      return std::get<I>(cases).get2();
    } else {
      static_assert(I != I, "Select receive accessor requires a receive case");
    }
  }
};

} // namespace eo
