// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once
#include <eo/panic.h>
#include <eo/utf8.h>

#include <fmt/format.h>

#include <concepts>
#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>

namespace eo::detail {

// Index and slice checks with Go's runtime messages. The checks compare the operands as integers of
// any type, so a negative index is reported as negative and a huge unsigned one is not wrapped. Go
// leaves out the length or the other bound when the reported operand is negative.

// Each check compares inline, so that a check whose operands are constants or already known folds
// away, and formats its message out of line, in a cold function that the caller only calls to panic.
// An error function requires that at least one bound fails: it tests the bounds again in Go's order
// and reports the last one without testing it. Its tests and the condition of its check list the
// same bounds, so a bound added to one must be added to the other.

template<go_integer I, go_integer N>
[[noreturn, gnu::cold, gnu::noinline]] void index_error(I i, N length) {
  if (std::cmp_less(i, 0)) {
    panic_bounds_error(fmt::format("index out of range [{}]", i));
  }
  panic_bounds_error(fmt::format("index out of range [{}] with length {}", i, length));
}

template<go_integer I, go_integer N>
[[gnu::always_inline]] inline void check_index(I i, N length) {
  if (std::cmp_less(i, 0) || std::cmp_greater_equal(i, length)) [[unlikely]] {
    index_error(i, length);
  }
}

template<go_integer L, go_integer H, go_integer N>
[[noreturn, gnu::cold, gnu::noinline]] void slice_error(L low, H high, N limit, const char* limit_name) {
  if (std::cmp_less(high, 0)) {
    panic_bounds_error(fmt::format("slice bounds out of range [:{}]", high));
  }
  if (std::cmp_greater(high, limit)) {
    panic_bounds_error(fmt::format("slice bounds out of range [:{}] with {} {}", high, limit_name, limit));
  }
  if (std::cmp_less(low, 0)) {
    panic_bounds_error(fmt::format("slice bounds out of range [{}:]", low));
  }
  panic_bounds_error(fmt::format("slice bounds out of range [{}:{}]", low, high));
}

// s[low:high] of a value whose upper limit is `limit`: the length of a string or an array, or the
// capacity of a slice, named by `limit_name`. Go checks the high bound first.
template<go_integer L, go_integer H, go_integer N>
[[gnu::always_inline]] inline void check_slice(L low, H high, N limit, const char* limit_name = "length") {
  if (std::cmp_less(high, 0) || std::cmp_greater(high, limit) || std::cmp_less(low, 0) || std::cmp_greater(low, high))
    [[unlikely]] {
    slice_error(low, high, limit, limit_name);
  }
}

template<go_integer L, go_integer H, go_integer M, go_integer N>
[[noreturn, gnu::cold, gnu::noinline]] void slice3_error(L low, H high, M max, N limit, const char* limit_name) {
  if (std::cmp_less(max, 0)) {
    panic_bounds_error(fmt::format("slice bounds out of range [::{}]", max));
  }
  if (std::cmp_greater(max, limit)) {
    panic_bounds_error(fmt::format("slice bounds out of range [::{}] with {} {}", max, limit_name, limit));
  }
  if (std::cmp_less(high, 0)) {
    panic_bounds_error(fmt::format("slice bounds out of range [:{}:]", high));
  }
  if (std::cmp_greater(high, max)) {
    panic_bounds_error(fmt::format("slice bounds out of range [:{}:{}]", high, max));
  }
  if (std::cmp_less(low, 0)) {
    panic_bounds_error(fmt::format("slice bounds out of range [{}::]", low));
  }
  panic_bounds_error(fmt::format("slice bounds out of range [{}:{}:]", low, high));
}

// s[low:high:max] of a value whose upper limit is `limit`: the capacity of a slice, or the length of
// an array, named by `limit_name`. Go checks max first, then high, then low.
template<go_integer L, go_integer H, go_integer M, go_integer N>
[[gnu::always_inline]] inline void check_slice3(L low, H high, M max, N limit, const char* limit_name = "capacity") {
  if (std::cmp_less(max, 0) || std::cmp_greater(max, limit) || std::cmp_less(high, 0) || std::cmp_greater(high, max) ||
    std::cmp_less(low, 0) || std::cmp_greater(low, high)) [[unlikely]] {
    slice3_error(low, high, max, limit, limit_name);
  }
}

} // namespace eo::detail
