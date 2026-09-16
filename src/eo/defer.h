// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once

#include <functional>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace eo::detail {
class defer_stack {
  struct call_base {
    virtual ~call_base() = default;
    virtual void invoke() = 0;
  };

  template<typename F>
  struct call final : call_base {
    explicit call(F function): function(std::move(function)) {}

    void invoke() override {
      std::invoke(function);
    }

    F function;
  };

public:
  defer_stack() = default;
  defer_stack(const defer_stack&) = delete;
  defer_stack(defer_stack&&) = delete;
  auto operator=(const defer_stack&) -> defer_stack& = delete;
  auto operator=(defer_stack&&) -> defer_stack& = delete;

  ~defer_stack() noexcept(false) {
    while (!calls.empty()) {
      auto deferred = std::move(calls.back());
      calls.pop_back();
      deferred->invoke();
    }
  }

  template<typename F, typename... Args>
  void push(F&& function, Args&&... args) {
    using function_type = std::decay_t<F>;
    using args_type = std::tuple<std::decay_t<Args>...>;

    auto deferred = [function = function_type(std::forward<F>(function)),
                     args = args_type(std::forward<Args>(args)...)]() mutable {
      std::apply(std::move(function), std::move(args));
    };
    calls.emplace_back(std::make_unique<call<decltype(deferred)>>(std::move(deferred)));
  }

private:
  std::vector<std::unique_ptr<call_base>> calls;
};
} // namespace eo::detail

#define eo_defer_scope ::eo::detail::defer_stack _eo_defer_stack
#define eo_defer(...) _eo_defer_stack.push(__VA_ARGS__)
