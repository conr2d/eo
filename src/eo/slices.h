// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once
#include <eo/core.h>

#include <algorithm>
#include <cstdint>
#include <functional>
#include <string>
#include <utility>

// Go's slices package, in part: the functions that ported code has needed so far. Each takes its
// slice as a slice<E>, which a slice of a defined type converts to. A slice<E> result is converted
// back to the defined type, as for eo::append.
namespace eo::slices {

// slices.IndexFunc(s, f): the index of the first element for which f is true, or -1.
template<typename E, typename F>
auto IndexFunc(slice<E> s, F f) -> int64_t {
  for (auto i = int64_t{0}; i < eo::len(s); ++i) {
    if (f(eo::at(s, i))) {
      return i;
    }
  }
  return -1;
}

// slices.Index(s, v): the index of the first element equal to v, or -1.
template<typename E>
auto Index(slice<E> s, const std::type_identity_t<E>& v) -> int64_t {
  return IndexFunc(std::move(s), [&](const E& e) { return v == e; });
}

// slices.ContainsFunc(s, f): whether f is true for some element.
template<typename E, typename F>
auto ContainsFunc(slice<E> s, F f) -> bool {
  return IndexFunc(std::move(s), std::move(f)) >= 0;
}

// slices.Contains(s, v): whether some element equals v.
template<typename E>
auto Contains(slice<E> s, const std::type_identity_t<E>& v) -> bool {
  return Index(std::move(s), v) >= 0;
}

// slices.Clone(s): a copy of s in a new array, or nil when s is nil. An empty s gives an empty slice
// that is not nil.
template<typename E>
auto Clone(slice<E> s) -> slice<E> {
  if (s == nullptr) {
    return nullptr;
  }
  return eo::append_slice(slice<E>::of({}), s);
}

// slices.Grow(s, n): s with room for n more elements past its length, in a new array when its
// capacity is short. A negative n panics with Go's message, "cannot be negative".
template<typename E>
auto Grow(slice<E> s, int64_t n) -> slice<E> {
  if (n < 0) {
    panic(std::string{"cannot be negative"});
  }
  if (n -= eo::cap(s) - eo::len(s); n > 0) {
    s = eo::subslice(eo::append_slice(eo::subslice(s, 0, eo::cap(s)), eo::make_slice<E>(n)), 0, eo::len(s));
  }
  return s;
}

// slices.Delete(s, i, j): removes s[i:j] in place, zeroes the elements between the new length and
// the old one, and returns s shortened. Bounds outside s panic as s[i:j:len(s)] does.
template<typename E>
auto Delete(slice<E> s, int64_t i, int64_t j) -> slice<E> {
  static_cast<void>(eo::subslice(s, i, j, eo::len(s)));
  if (i == j) {
    return s;
  }
  auto oldlen = eo::len(s);
  s = eo::append_slice(eo::subslice(s, 0, i), eo::subslice(s, j));
  eo::clear(eo::subslice(s, eo::len(s), oldlen));
  return s;
}

// slices.DeleteFunc(s, del): removes in place every element for which del is true, keeping the order
// of the others, zeroes the elements between the new length and the old one, and returns s
// shortened. del is called once per element, in order.
template<typename E, typename F>
auto DeleteFunc(slice<E> s, F del) -> slice<E> {
  // One function value serves the search and the loop, as in Go.
  auto i = IndexFunc(s, std::ref(del));
  if (i == -1) {
    return s;
  }
  for (auto j = i + 1; j < eo::len(s); ++j) {
    if (auto v = eo::at(s, j); !del(v)) {
      eo::at(s, i) = std::move(v);
      ++i;
    }
  }
  eo::clear(eo::subslice(s, i));
  return eo::subslice(s, 0, i);
}

// slices.SortFunc(s, cmp): sorts s in place by cmp, which returns a negative number, zero, or a
// positive number. Go leaves the order of equal elements unspecified, and so does Eo, whose order can
// differ from Go's. cmp must be a strict weak order, as Go requires for a meaningful result.
template<typename E, typename F>
void SortFunc(slice<E> s, F cmp) {
  std::sort(s.begin(), s.end(), [&](const E& a, const E& b) { return cmp(a, b) < 0; });
}

// slices.SortStableFunc(s, cmp): sorts s in place by cmp, keeping equal elements in their order.
template<typename E, typename F>
void SortStableFunc(slice<E> s, F cmp) {
  std::stable_sort(s.begin(), s.end(), [&](const E& a, const E& b) { return cmp(a, b) < 0; });
}

} // namespace eo::slices
