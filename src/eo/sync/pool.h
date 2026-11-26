// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once
#include <eo/core.h>
#include <eo/fn.h>

#include <mutex>
#include <utility>
#include <vector>

namespace eo::sync {

namespace detail {

  // The free values of a Pool under a mutex. Copying it copies the values with a new mutex, so that
  // a Pool stays an aggregate that a translated `sync.Pool{New: f}` initializes and assigns.
  struct pool_items {
    pool_items() = default;

    pool_items(const pool_items& other): items(other.snapshot()) {}

    auto operator=(const pool_items& other) -> pool_items& {
      if (this != &other) {
        auto copy = other.snapshot();
        std::lock_guard lock(m);
        items = std::move(copy);
      }
      return *this;
    }

    auto snapshot() const -> std::vector<any> {
      std::lock_guard lock(m);
      return items;
    }

    mutable std::mutex m;
    std::vector<any> items;
  };

} // namespace detail

// Go's sync.Pool: a set of values that Get may hand out again after Put. New, if not nil, makes a
// value when the pool has none. Go may drop pooled values at any garbage collection, so a program
// cannot depend on which value Get returns. Eo keeps every value until Get takes it, and Get returns
// the one put last.
struct Pool {
  fn<any()> New{};
  detail::pool_items _eo_items{};

  auto Get() -> any {
    {
      std::lock_guard lock(_eo_items.m);
      if (!_eo_items.items.empty()) {
        auto x = std::move(_eo_items.items.back());
        _eo_items.items.pop_back();
        return x;
      }
    }
    if (New != nullptr) {
      return New();
    }
    return nullptr;
  }

  // Adds x to the pool. A nil x is ignored, as in Go.
  void Put(any x) {
    if (x == nullptr) {
      return;
    }
    std::lock_guard lock(_eo_items.m);
    _eo_items.items.push_back(std::move(x));
  }
};

} // namespace eo::sync
