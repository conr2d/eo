// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once
#include <eo/ptr.h>

#include <boost/pfr.hpp>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <list>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace eo {

namespace detail {

  // Whether a key of type K can be unequal to itself, as a float NaN is, or a struct with a NaN field.
  template<typename K>
  consteval auto has_float_key() -> bool {
    if constexpr (defined_type<K>) {
      return has_float_key<typename K::_eo_underlying>();
    } else if constexpr (std::is_floating_point_v<K>) {
      return true;
    } else if constexpr (go_struct<K> && !std::is_same_v<K, empty_struct>) {
      return []<std::size_t... I>(std::index_sequence<I...>) {
        return (has_float_key<boost::pfr::tuple_element_t<I, K>>() || ...);
      }(std::make_index_sequence<boost::pfr::tuple_size_v<K>>{});
    } else {
      return false;
    }
  }

  // Go's hash of a map key: equal keys hash alike. 0.0 and -0.0 are equal, so -0.0 hashes as 0.0,
  // which the standard library's float hash does not promise.
  template<typename K>
  auto hash_key(const K& k) -> std::size_t {
    if constexpr (defined_type<K>) {
      return hash_key(underlying(k));
    } else if constexpr (std::is_floating_point_v<K>) {
      return std::hash<K>{}(k == K{0} ? K{0} : k);
    } else if constexpr (std::is_arithmetic_v<K> || std::is_same_v<K, std::string>) {
      return std::hash<K>{}(k);
    } else if constexpr (is_ptr<K>) {
      return std::hash<const void*>{}(static_cast<const void*>(k.get()));
    } else if constexpr (std::is_same_v<K, empty_struct>) {
      return 0;
    } else if constexpr (go_struct<K>) {
      auto h = std::size_t{0};
      boost::pfr::for_each_field(k,
        [&](const auto& field) { h ^= hash_key(field) + 0x9e3779b97f4a7c15 + (h << 6) + (h >> 2); });
      return h;
    } else {
      static_assert(sizeof(K) == 0,
        "a map key must be a bool, number, string, pointer, or struct of such types; other key types are not "
        "covered yet");
    }
  }

  template<typename K>
  struct key_hash {
    auto operator()(const K& k) const -> std::size_t {
      return hash_key(k);
    }
  };

  // Whether two equal keys differ in their bits, as 0.0 and -0.0 do. Go's map assignment stores the
  // new key in that case.
  template<typename K>
  auto key_bits_differ(const K& a, const K& b) -> bool {
    if constexpr (defined_type<K>) {
      return key_bits_differ(underlying(a), underlying(b));
    } else if constexpr (std::is_floating_point_v<K>) {
      return std::signbit(a) != std::signbit(b);
    } else if constexpr (go_struct<K> && has_float_key<K>()) {
      return [&]<std::size_t... I>(std::index_sequence<I...>) {
        return (key_bits_differ(boost::pfr::get<I>(a), boost::pfr::get<I>(b)) || ...);
      }(std::make_index_sequence<boost::pfr::tuple_size_v<K>>{});
    } else {
      return false;
    }
  }

  // The storage that the copies of one Go map share. A key that is unequal to itself, such as NaN,
  // can be stored but never found, deleted, or assigned again, so each such entry is kept apart in
  // `nans`, in insertion order. `clears` counts calls of clear, for range loops.
  template<typename K, typename V>
  struct map_table {
    std::unordered_map<K, V, key_hash<K>> entries;
    std::list<std::pair<K, V>> nans;
    uint64_t clears = 0;

    auto size() const noexcept -> int64_t {
      return static_cast<int64_t>(entries.size() + nans.size());
    }

    // Calls f(key, value) for every entry.
    template<typename F>
    void for_each(F&& f) const {
      for (const auto& [k, v] : entries) {
        f(k, v);
      }
      for (const auto& [k, v] : nans) {
        f(k, v);
      }
    }
  };

  template<typename K>
  auto is_nan_key(const K& k) -> bool {
    if constexpr (has_float_key<K>()) {
      return !(k == k);
    } else {
      return false;
    }
  }

} // namespace detail

// Go's map[K]V: a handle to a hash table that every copy of the handle shares, as Go maps do.
//
// map<K, V>{} and nullptr are the nil map, which reads as empty and panics when assigned. A
// composite literal is map<K, V>::of({{k, v}, ...}), which is never nil. A translation reaches the
// entries only through the eo:: functions below.
template<typename K, typename V>
class map {
public:
  using key_type = K;
  using mapped_type = V;
  using table_type = detail::map_table<K, V>;

  map() noexcept = default;
  map(std::nullptr_t) noexcept {}

  // map[K]V{k1: v1, k2: v2}: a new map holding the entries. The braced list evaluates them left to
  // right, as Go does, and a later entry with an equal key replaces an earlier one.
  static auto of(std::initializer_list<std::pair<K, V>> entries) -> map {
    auto m = make();
    for (const auto& [k, v] : entries) {
      m.entry(k) = v;
    }
    return m;
  }

  // A new empty map, as make(map[K]V) makes.
  static auto make() -> map {
    auto m = map{};
    m.table_ = std::make_shared<table_type>();
    return m;
  }

  // The shared table, or nullptr for nil. These members are not translation forms: they serve Eo
  // and tests.
  auto table() const noexcept -> table_type* {
    return table_.get();
  }

  auto size() const noexcept -> int64_t {
    return table_ == nullptr ? 0 : table_->size();
  }

  // The value of key k, inserting the zero value when k is absent, or panicking for nil.
  auto entry(const K& k) const -> V& {
    if (table_ == nullptr) {
      detail::panic_plain_error("assignment to entry in nil map");
    }
    if (detail::is_nan_key(k)) {
      return table_->nans.emplace_back(k, V{}).second;
    }
    auto [it, inserted] = table_->entries.try_emplace(k);
    if constexpr (detail::has_float_key<K>()) {
      if (!inserted && detail::key_bits_differ(it->first, k)) {
        auto node = table_->entries.extract(it);
        node.key() = k;
        it = table_->entries.insert(std::move(node)).position;
      }
    }
    return it->second;
  }

  // The value of key k, or nullptr when k is absent.
  auto find(const K& k) const -> V* {
    if (table_ == nullptr || detail::is_nan_key(k)) {
      return nullptr;
    }
    auto it = table_->entries.find(k);
    return it == table_->entries.end() ? nullptr : &it->second;
  }

  // m == nil. Go compares a map only with nil.
  friend auto operator==(const map& m, std::nullptr_t) noexcept -> bool {
    return m.table_ == nullptr;
  }

private:
  std::shared_ptr<table_type> table_;
};

namespace detail {

  template<typename K, typename V>
  struct make_map_fn {
    // make(map[K]V)
    auto operator()() const -> map<K, V> {
      return map<K, V>::make();
    }

    // make(map[K]V, hint). The hint only sizes the table. Go allows a negative hint that is not a
    // constant, and treats it as zero.
    template<go_integer N>
    auto operator()(N hint) const -> map<K, V> {
      auto m = map<K, V>::make();
      constexpr auto max_reserve = int64_t{1} << 16;
      if (std::cmp_greater(hint, 0)) {
        m.table()->entries.reserve(
          static_cast<std::size_t>(std::cmp_less(hint, max_reserve) ? static_cast<int64_t>(hint) : max_reserve));
      }
      return m;
    }
  };

  struct map_get_fn {
    // m[k] as a value: a copy of the value of k, or the zero value when k is absent. It does not
    // insert k.
    template<typename K, typename V>
    auto operator()(const map<K, V>& m, const std::type_identity_t<K>& k) const -> V {
      if (auto* v = m.find(k)) {
        return *v;
      }
      return V{};
    }
  };

  struct map_get_ok_fn {
    // v, ok := m[k]
    template<typename K, typename V>
    auto operator()(const map<K, V>& m, const std::type_identity_t<K>& k) const -> std::pair<V, bool> {
      if (auto* v = m.find(k)) {
        return {*v, true};
      }
      return {V{}, false};
    }
  };

  struct map_entry_fn {
    // m[k] on the left of an assignment, an op-assignment, ++, or --: the value of k, which is
    // inserted with the zero value when absent. A nil map panics.
    template<typename K, typename V>
    auto operator()(const map<K, V>& m, const std::type_identity_t<K>& k) const -> V& {
      return m.entry(k);
    }
  };

  struct delete_fn {
    // delete(m, k): removes k, if present. A nil map has nothing to remove.
    template<typename K, typename V>
    void operator()(const map<K, V>& m, const std::type_identity_t<K>& k) const {
      if (auto* t = m.table(); t != nullptr && !is_nan_key(k)) {
        t->entries.erase(k);
      }
    }
  };

} // namespace detail

// Function objects, called qualified, like the string helpers (see <eo/string.h>). The map_ prefix
// keeps them apart from common Go names, such as lookup and entry, which Go code spells without an
// underscore.
template<typename K, typename V>
inline constexpr detail::make_map_fn<K, V> make_map{};
inline constexpr detail::map_get_fn map_get{};
inline constexpr detail::map_get_ok_fn map_get_ok{};
inline constexpr detail::map_entry_fn map_entry{};
inline constexpr detail::delete_fn delete_{};

} // namespace eo
