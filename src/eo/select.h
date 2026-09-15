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

struct CaseDefault {
  auto ready() {
    return true;
  }
  auto wait() -> boost::asio::awaitable<bool> {
    co_return true;
  }
  void get() {}
};

template<typename T, typename... Ts>
struct OneOf {
  static constexpr bool value = (std::is_same_v<T, Ts> || ...);
};

template<typename T, typename... Ts>
constexpr bool one_of = OneOf<T, Ts...>::value;

template<typename T, typename... Ts>
struct Select {
  Select(T&& t, Ts&&... ts): cases(std::make_tuple(std::forward<T>(t), std::forward<Ts>(ts)...)) {}

  std::tuple<T, Ts...> cases;

private:
  static constexpr bool has_default = one_of<CaseDefault, std::remove_cvref_t<T>, std::remove_cvref_t<Ts>...>;

  std::optional<size_t> selected_index;

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

  template<size_t I>
  void append_communication_index(std::vector<size_t>& indices) {
    using Case = std::remove_cvref_t<std::tuple_element_t<I, decltype(cases)>>;
    if constexpr (!std::is_same_v<Case, CaseDefault>) {
      indices.push_back(I);
    }
  }

  template<size_t... I>
  auto communication_indices(std::index_sequence<I...>) -> std::vector<size_t> {
    std::vector<size_t> indices;
    indices.reserve(sizeof...(Ts));
    (append_communication_index<I>(indices), ...);
    return indices;
  }

  auto randomized_indices() -> std::vector<size_t> {
    auto indices = communication_indices(std::make_index_sequence<sizeof...(Ts) + 1>());
    for (size_t i = indices.size(); i > 1; --i) {
      auto j = static_cast<size_t>(math::rand::Int63n(i));
      std::swap(indices[i - 1], indices[j]);
    }
    return indices;
  }

public:
  auto try_index() -> int {
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

  // https://go.dev/ref/spec#Select_statements
  // If one or more of the communications can proceed, a single one that can proceed is chosen via a uniform
  // pseudo-random selection. Otherwise, if there is a default case, that case is chosen. If there is no default case,
  // the "select" statement blocks until at least one of the communications can proceed.
  auto index() -> boost::asio::awaitable<int>
    requires(has_default)
  {
    for (auto index : randomized_indices()) {
      if (ready(index)) {
        selected_index = index;
        co_return index;
      }
    }
    selected_index.reset();
    co_return -1;
  }

  auto index() -> boost::asio::awaitable<int>
    requires(!has_default)
  {
    for (auto index : randomized_indices()) {
      if (ready(index)) {
        commit(index);
        selected_index = index;
        co_return index;
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
      co_return index;
    }
  }

  template<size_t I>
  auto recv() {
    static_assert(I < sizeof...(Ts) + 1, "Select receive index out of range");
    if (!selected_index || *selected_index != I) {
      throw std::runtime_error("Select receive accessor does not match selected case");
    }
    if constexpr (requires { std::get<I>(cases).get(); }) {
      return std::get<I>(cases).get();
    } else {
      static_assert(I != I, "Select receive accessor requires a receive case");
    }
  }

  template<size_t I>
  auto process() {
    return std::get<I>(cases).process();
  }
};

} // namespace eo
