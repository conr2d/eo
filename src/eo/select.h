#pragma once
#include <eo/go.h>
#include <tuple>

namespace eo {

struct CaseDefault {
  auto ready() { return true; }
  auto wait() -> boost::asio::awaitable<bool> { co_return true; }
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
  Select(T&& t, Ts&&... ts): cases(t, ts...) {}

  std::tuple<T, Ts...> cases;

  template<size_t I = 0>
  auto index() -> int requires (one_of<CaseDefault, Ts...>) {
    if constexpr (!sizeof...(Ts)) {
      return 0;
    }
    if constexpr (I < sizeof...(Ts) + 1) {
      if (std::get<I>(cases).ready()) {
        return I;
      }
      return index<I + 1>();
    }
    return -1;
  }

  auto index() requires (!one_of<CaseDefault, Ts...>) {
    return [this]<size_t... I>(std::index_sequence<I...>) -> boost::asio::awaitable<int> {
      auto res = co_await (std::get<I>(cases).wait() || ...);
      co_return res.index();
    }(std::make_index_sequence<sizeof...(Ts) + 1>());
  }

  template<size_t I>
  auto process() {
    return std::get<I>(cases).process();
  }
};

} // namespace eo
