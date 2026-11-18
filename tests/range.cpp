// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#include <eo/fmt.h>
#include <eo/invoke.h>
#include <eo/time.h>

#include <fmt/format.h>

#include <chrono>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

using namespace eo;

void check(bool condition, const char* message) {
  if (!condition) {
    throw std::logic_error(message);
  }
}

int main() {
  // range n yields n's type, from 0 through n-1, and stops before the type's maximum overflows
  auto int8_count = 0;
  for (auto i : eo::range(int8_t{127})) {
    static_assert(std::is_same_v<decltype(i), int8_t>);
    check(i == int8_count, "int8 value");
    ++int8_count;
  }
  check(int8_count == 127, "range of int8 maximum");
  auto uint8_count = 0;
  for ([[maybe_unused]] auto _eo_range_blank : eo::range(uint8_t{255})) {
    ++uint8_count;
  }
  check(uint8_count == 255, "range of uint8 maximum");
  for ([[maybe_unused]] auto i : eo::range(int64_t{-1})) {
    check(false, "negative range");
  }
  for ([[maybe_unused]] auto i : eo::range(uint64_t{0})) {
    check(false, "zero range");
  }

  // range s copies the slice once and reads each element when its iteration starts
  auto s = slice<std::string>::of({"a"s, "b"s, "c"s});
  auto seen = std::vector<std::string>{};
  for (auto [i, v] : eo::range(s)) {
    static_assert(std::is_same_v<decltype(i), int64_t>);
    if (i == 0) {
      eo::at(s, 1) = "B"s;
      s = nullptr;
    }
    v += "!";
    seen.push_back(fmt::format("{}{}", i, v));
  }
  check(seen == std::vector<std::string>{"0a!", "1B!", "2c!"}, "slice range");
  for ([[maybe_unused]] auto entry : eo::range(slice<int64_t>{})) {
    check(false, "nil slice range");
  }
  auto a = slice<int64_t>::of({1, 2});
  for (auto [i, v] : eo::range(a)) {
    a = eo::append(a, {v});
  }
  check(fmt::format("{}", a) == "[1 2 1 2]", "appending in the body does not extend the loop");

  // range str decodes UTF-8 from a copy of str
  auto str = "a\x{e2}\x{82}\x{ac}\x{ff}\x{e2}\x{82}"s;
  auto runes = std::vector<std::string>{};
  for (auto [i, r] : eo::range(str)) {
    static_assert(std::is_same_v<decltype(i), int64_t> && std::is_same_v<decltype(r), int32_t>);
    str.clear();
    runes.push_back(fmt::format("{}:{}", i, r));
  }
  check(runes == std::vector<std::string>{"0:97", "1:8364", "4:65533", "5:65533", "6:65533"}, "string range");
  for ([[maybe_unused]] auto entry : eo::range(""s)) {
    check(false, "empty string range");
  }

  // a range loop in a coroutine keeps its range across suspension
  auto sum = eo::invoke([]() -> func<int64_t> {
    auto total = int64_t{0};
    for (auto [i, v] : eo::range(slice<int64_t>::of({1, 2, 3}))) {
      co_await eo::time::Sleep(std::chrono::milliseconds{1});
      total += i * v;
    }
    co_return total;
  });
  check(sum == 8, "range across co_await");

  return 0;
}
