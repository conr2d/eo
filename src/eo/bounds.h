// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once
#include <eo/panic.h>

#include <fmt/format.h>

#include <any>
#include <concepts>
#include <cstdint>
#include <string>
#include <utility>

namespace eo::detail {

// The value of a runtime panic whose message depends on its operands, such as an index out of range.
// eo::Error registers every distinct message for good, so it is kept for fixed messages.
struct runtime_error {
  std::string message;
};

[[noreturn]] inline void panic_runtime_error(std::string message) {
  auto text = "runtime error: " + std::move(message);
  auto value = runtime_error{text};
  throw Panic{std::any{std::move(value)}, std::move(text)};
}

// Index and slice checks with Go's runtime messages. The checks compare the operands as integers of
// any type, so a negative index is reported as negative and a huge unsigned one is not wrapped. Go
// leaves out the length or the other bound when the reported operand is negative.

template<std::integral I, std::integral N>
void check_index(I i, N length) {
  if (std::cmp_less(i, 0)) {
    panic_runtime_error(fmt::format("index out of range [{}]", i));
  }
  if (std::cmp_greater_equal(i, length)) {
    panic_runtime_error(fmt::format("index out of range [{}] with length {}", i, length));
  }
}

// s[low:high] of a value whose upper limit is `limit`: the length of a string, or the capacity of a
// slice, named by `limit_name`. Go checks the high bound first.
template<std::integral L, std::integral H, std::integral N>
void check_slice(L low, H high, N limit, const char* limit_name = "length") {
  if (std::cmp_less(high, 0)) {
    panic_runtime_error(fmt::format("slice bounds out of range [:{}]", high));
  }
  if (std::cmp_greater(high, limit)) {
    panic_runtime_error(fmt::format("slice bounds out of range [:{}] with {} {}", high, limit_name, limit));
  }
  if (std::cmp_less(low, 0)) {
    panic_runtime_error(fmt::format("slice bounds out of range [{}:]", low));
  }
  if (std::cmp_greater(low, high)) {
    panic_runtime_error(fmt::format("slice bounds out of range [{}:{}]", low, high));
  }
}

} // namespace eo::detail

template<>
struct fmt::formatter<eo::detail::runtime_error> : fmt::formatter<std::string> {
  auto format(const eo::detail::runtime_error& e, format_context& ctx) const -> decltype(ctx.out()) {
    return fmt::formatter<std::string>::format(e.message, ctx);
  }
};
