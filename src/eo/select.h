// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once
#include <eo/go.h>
#include <tuple>

namespace eo {

namespace math::rand {
  extern auto int63_n(int64_t n) -> int64_t;
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
  template<size_t I = 0>
  void eval_ready(std::vector<size_t>& indices) {
    if constexpr (!sizeof...(Ts)) {
      indices.push_back(0);
      return;
    }
    if constexpr (I < sizeof...(Ts)) {
      if (std::get<I>(cases).ready()) {
        indices.push_back(I);
      }
      eval_ready<I + 1>(indices);
    }
  }

public:
  // https://go.dev/ref/spec#Select_statements
  // If one or more of the communications can proceed, a single one that can proceed is chosen via a uniform
  // pseudo-random selection. Otherwise, if there is a default case, that case is chosen. If there is no default case,
  // the "select" statement blocks until at least one of the communications can proceed.
  auto index() -> boost::asio::awaitable<int>
  requires(one_of<CaseDefault, Ts...>) {
    std::vector<size_t> indices{};
    indices.reserve(sizeof...(Ts) + 1);
    eval_ready(indices);
    switch (indices.size()) {
    case 0:
      co_return -1;
    case 1:
      co_return indices[0];
    default:
      co_return indices[math::rand::int63_n(indices.size())];
    }
  }

  auto index() requires(!one_of<CaseDefault, Ts...>) {
    if constexpr (!sizeof...(Ts)) {
      return [this]() -> func<int> {
        co_await std::get<0>(cases).wait();
        co_return 0;
      }();
    } else {
      return [this]<size_t... I>(std::index_sequence<I...>)->boost::asio::awaitable<int> {
        auto res = co_await (std::get<I>(cases).wait() || ...);
        co_return res.index();
      }
      (std::make_index_sequence<sizeof...(Ts) + 1>());
    }
  }

  template<size_t I>
  auto process() {
    return std::get<I>(cases).process();
  }
};

} // namespace eo
