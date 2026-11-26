// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once
#include <eo/core.h>

#include <mutex>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace eo::sync {

// Go's sync.Map: a map[any]any that goroutines may use at once. It is one Eo map under a mutex,
// where Go's is a concurrent hash trie, so it gives Go's results but not its scaling. The keys are
// hashed and compared as interface map keys are: a key whose dynamic type cannot be a key panics
// with `runtime error: hash of unhashable type T`. The methods do not call user code under the
// mutex: Range calls f on a snapshot of the entries, so f may call any method of the Map.
struct Map {
  Map() = default;
  Map(const Map&) = delete;
  auto operator=(const Map&) -> Map& = delete;

  auto Load(any key) -> std::tuple<any, bool> {
    check(key);
    std::lock_guard lock(m_);
    if (auto* v = entries().find(key)) {
      return {*v, true};
    }
    return {nullptr, false};
  }

  void Store(any key, any value) {
    check(key);
    std::lock_guard lock(m_);
    eo::map_entry(entries(), key) = std::move(value);
  }

  auto LoadOrStore(any key, any value) -> std::tuple<any, bool> {
    check(key);
    std::lock_guard lock(m_);
    if (auto* v = entries().find(key)) {
      return {*v, true};
    }
    eo::map_entry(entries(), key) = value;
    return {value, false};
  }

  auto LoadAndDelete(any key) -> std::tuple<any, bool> {
    check(key);
    std::lock_guard lock(m_);
    auto* v = entries().find(key);
    if (v == nullptr) {
      return {nullptr, false};
    }
    auto value = *v;
    eo::delete_(entries(), key);
    return {value, true};
  }

  void Delete(any key) {
    check(key);
    std::lock_guard lock(m_);
    eo::delete_(entries(), key);
  }

  auto Swap(any key, any value) -> std::tuple<any, bool> {
    check(key);
    std::lock_guard lock(m_);
    if (auto* v = entries().find(key)) {
      return {std::exchange(*v, std::move(value)), true};
    }
    eo::map_entry(entries(), key) = std::move(value);
    return {nullptr, false};
  }

  // Swaps in new when the value for key equals old. Comparing values of an uncomparable dynamic type
  // panics, as == of two interfaces does.
  auto CompareAndSwap(any key, any old, any new_) -> bool {
    check(key);
    std::lock_guard lock(m_);
    auto* v = entries().find(key);
    if (v == nullptr || !(*v == old)) {
      return false;
    }
    *v = std::move(new_);
    return true;
  }

  // Deletes the entry for key when its value equals old. An absent key returns false, even for a
  // nil old.
  auto CompareAndDelete(any key, any old) -> bool {
    check(key);
    std::lock_guard lock(m_);
    auto* v = entries().find(key);
    if (v == nullptr || !(*v == old)) {
      return false;
    }
    eo::delete_(entries(), key);
    return true;
  }

  // Calls f(key, value) for the entries, in no particular order, until f returns false. Go does not
  // promise a consistent snapshot either: an entry stored or deleted during Range may or may not be
  // visited. Here f sees the entries as they were when Range started.
  template<typename F>
  void Range(F&& f) {
    auto snapshot = std::vector<std::pair<any, any>>{};
    {
      std::lock_guard lock(m_);
      if (auto* t = map_.table(); t != nullptr) {
        snapshot.reserve(static_cast<std::size_t>(t->size()));
        t->for_each([&](const any& k, const any& v) { snapshot.emplace_back(k, v); });
      }
    }
    for (auto& [k, v] : snapshot) {
      if (!f(k, v)) {
        return;
      }
    }
  }

  void Clear() {
    std::lock_guard lock(m_);
    map_ = nullptr;
  }

private:
  // Go's Map hashes every key, even in an empty map, so a key that cannot be hashed panics with
  // `runtime error: hash of unhashable type T`, where a lookup in an empty Go map reports
  // `hash of unhashable type: T`.
  static void check(const any& key) {
    static_cast<void>(eo::detail::is_nan_key(key));
  }

  auto entries() -> map<any, any>& {
    if (map_ == nullptr) {
      map_ = map<any, any>::make();
    }
    return map_;
  }

  std::mutex m_;
  map<any, any> map_{};
};

} // namespace eo::sync
