// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once
#include <eo/core.h>

#include <utility>
#include <vector>

// Go's maps package, except the functions that take or return an iterator (iter.Seq), which wait on
// a mapping of range over functions.
namespace eo::maps {

// maps.Clone(m): a new map with the entries of m, or nil when m is nil.
template<typename K, typename V>
auto Clone(map<K, V> m) -> map<K, V> {
  auto* t = m.table();
  if (t == nullptr) {
    return nullptr;
  }
  auto out = map<K, V>::make();
  *out.table() = *t;
  out.table()->clears = 0;
  return out;
}

// maps.Copy(dst, src): assigns every entry of src in dst, so a nil dst panics when src has an entry.
template<typename K, typename V>
void Copy(map<K, V> dst, map<K, V> src) {
  for (auto [k, v] : eo::range(src)) {
    eo::map_entry(dst, k) = v;
  }
}

// maps.DeleteFunc(m, del): deletes every entry for which del(k, v) is true. An entry whose key is NaN
// cannot be deleted, as in Go.
template<typename K, typename V, typename F>
void DeleteFunc(map<K, V> m, F del) {
  for (auto [k, v] : eo::range(m)) {
    if (del(k, v)) {
      eo::delete_(m, k);
    }
  }
}

// maps.EqualFunc(m1, m2, eq): whether both maps have the same keys and eq(v1, v2) holds for each.
template<typename K, typename V1, typename V2, typename F>
auto EqualFunc(map<K, V1> m1, map<K, V2> m2, F eq) -> bool {
  if (eo::len(m1) != eo::len(m2)) {
    return false;
  }
  for (auto [k, v1] : eo::range(m1)) {
    auto [v2, ok] = eo::map_get_ok(m2, k);
    if (!ok || !eq(v1, v2)) {
      return false;
    }
  }
  return true;
}

// maps.Equal(m1, m2): whether both maps have the same keys and equal values.
template<typename K, typename V>
auto Equal(map<K, V> m1, map<K, V> m2) -> bool {
  return EqualFunc(std::move(m1), std::move(m2), [](const V& a, const V& b) { return a == b; });
}

} // namespace eo::maps
