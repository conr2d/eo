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

template<go_integer I, go_integer N>
void check_index(I i, N length) {
  if (std::cmp_less(i, 0)) {
    panic_bounds_error(fmt::format("index out of range [{}]", i));
  }
  if (std::cmp_greater_equal(i, length)) {
    panic_bounds_error(fmt::format("index out of range [{}] with length {}", i, length));
  }
}

// s[low:high] of a value whose upper limit is `limit`: the length of a string, or the capacity of a
// slice, named by `limit_name`. Go checks the high bound first.
template<go_integer L, go_integer H, go_integer N>
void check_slice(L low, H high, N limit, const char* limit_name = "length") {
  if (std::cmp_less(high, 0)) {
    panic_bounds_error(fmt::format("slice bounds out of range [:{}]", high));
  }
  if (std::cmp_greater(high, limit)) {
    panic_bounds_error(fmt::format("slice bounds out of range [:{}] with {} {}", high, limit_name, limit));
  }
  if (std::cmp_less(low, 0)) {
    panic_bounds_error(fmt::format("slice bounds out of range [{}:]", low));
  }
  if (std::cmp_greater(low, high)) {
    panic_bounds_error(fmt::format("slice bounds out of range [{}:{}]", low, high));
  }
}

// s[low:high:max] of a slice whose capacity is `cap`. Go checks max first, then high, then low.
template<go_integer L, go_integer H, go_integer M, go_integer N>
void check_slice3(L low, H high, M max, N cap) {
  if (std::cmp_less(max, 0)) {
    panic_bounds_error(fmt::format("slice bounds out of range [::{}]", max));
  }
  if (std::cmp_greater(max, cap)) {
    panic_bounds_error(fmt::format("slice bounds out of range [::{}] with capacity {}", max, cap));
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
  if (std::cmp_greater(low, high)) {
    panic_bounds_error(fmt::format("slice bounds out of range [{}:{}:]", low, high));
  }
}

} // namespace eo::detail
