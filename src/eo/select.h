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
  static constexpr bool has_default = one_of<CaseDefault, std::remove_cvref_t<T>, std::remove_cvref_t<Ts>...>;

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
      auto j = static_cast<size_t>(math::rand::int63_n(i));
      std::swap(indices[i - 1], indices[j]);
    }
    return indices;
  }

public:
  // https://go.dev/ref/spec#Select_statements
  // If one or more of the communications can proceed, a single one that can proceed is chosen via a uniform
  // pseudo-random selection. Otherwise, if there is a default case, that case is chosen. If there is no default case,
  // the "select" statement blocks until at least one of the communications can proceed.
  auto index() -> boost::asio::awaitable<int>
    requires(has_default)
  {
    for (auto index : randomized_indices()) {
      if (ready(index)) {
        co_return index;
      }
    }
    co_return -1;
  }

  auto index() -> boost::asio::awaitable<int>
    requires(!has_default)
  {
    for (auto index : randomized_indices()) {
      if (ready(index)) {
        co_return index;
      }
    }

    if constexpr (!sizeof...(Ts)) {
      co_await std::get<0>(cases).wait();
      co_return 0;
    } else {
      auto res = co_await [this]<size_t... I>(std::index_sequence<I...>) -> boost::asio::awaitable<decltype(
                            co_await (std::get<I>(cases).wait() || ...))> {
        co_return co_await (std::get<I>(cases).wait() || ...);
      }(std::make_index_sequence<sizeof...(Ts) + 1>());
      co_return res.index();
    }
  }

  template<size_t I>
  auto process() {
    return std::get<I>(cases).process();
  }
};

} // namespace eo
