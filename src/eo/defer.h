// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once

#include <utility>

namespace eo::detail {
template <typename F> class scope_exit {
public:
  explicit scope_exit(F function) : function_(std::move(function)) {}

  scope_exit(const scope_exit &) = delete;
  scope_exit &operator=(const scope_exit &) = delete;

  ~scope_exit() noexcept { function_(); }

private:
  F function_;
};

template <typename F> scope_exit(F) -> scope_exit<F>;
} // namespace eo::detail

#define EO_CONCAT_IMPL(a, b) a##b
#define EO_CONCAT(a, b) EO_CONCAT_IMPL(a, b)
#define eo_defer(...) auto EO_CONCAT(_eo_defer_, __COUNTER__) = eo::detail::scope_exit(__VA_ARGS__)
