// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once
#include <eo/go.h>
#include <tuple>

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

    if constexpr (!sizeof...(Ts)) {
      co_await std::get<0>(cases).wait();
      selected_index = 0;
      co_return 0;
    } else {
      auto index = co_await [this]<size_t... I>(std::index_sequence<I...>) -> boost::asio::awaitable<int> {
        auto res = co_await (std::get<I>(cases).wait() || ...);
        co_return res.index();
      }(std::make_index_sequence<sizeof...(Ts) + 1>());
      selected_index = index;
      co_return static_cast<int>(index);
    }
  }

  template<size_t I>
  auto recv() {
    static_assert(I < sizeof...(Ts) + 1, "Select receive index out of range");
    if (!selected_index || *selected_index != I) {
      throw std::runtime_error("Select receive accessor does not match selected case");
    }
    if (receive_consumed) {
      throw std::runtime_error("Select receive result already consumed");
    }
    if constexpr (requires { std::get<I>(cases).get(); }) {
      receive_consumed = true;
      return std::get<I>(cases).get();
    } else {
      static_assert(I != I, "Select receive accessor requires a receive case");
    }
  }
};

} // namespace eo
