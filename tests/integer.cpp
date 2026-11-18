// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#include <eo/core.h>

#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>

using namespace eo;

void check(bool condition, const char* message) {
  if (!condition) {
    throw std::logic_error(message);
  }
}

// Runs f, which must panic, and returns the panic message.
template<typename F>
auto panic_message(F f) -> std::string {
  try {
    f();
  } catch (const Panic& p) {
    return p.message();
  }
  throw std::logic_error("expected a panic");
}

// A Go function whose name matches an Eo helper, as a Go program may declare it.
auto shl(int64_t a, int64_t b) -> int64_t {
  return a + b;
}

// Values are passed through a volatile, so the compiler cannot fold the operations and the checks
// see what -fwrapv makes of them at run time.
template<typename T>
auto opaque(T v) -> T {
  volatile T x = v;
  return x;
}

int main() {
  constexpr auto min64 = std::numeric_limits<int64_t>::min();
  constexpr auto max64 = std::numeric_limits<int64_t>::max();

  // Signed +, -, * and unary - wrap under -fwrapv, which Eo passes to every target that links it.
  check(opaque(max64) + 1 == min64, "signed + wraps");
  check(opaque(min64) - 1 == max64, "signed - wraps");
  check(opaque(max64) * 2 == -2, "signed * wraps");
  check(-opaque(min64) == min64, "unary - wraps");
  check(opaque(std::numeric_limits<int32_t>::max()) + 1 == std::numeric_limits<int32_t>::min(), "int32 + wraps");

  // A narrow result converts back to its type, which wraps modulo 2^N.
  int8_t a = opaque<int8_t>(100);
  int8_t b = opaque<int8_t>(100);
  check(int8_t(a + b) == -56 && int8_t(int8_t(a + b) / 2) == -28, "int8 arithmetic");
  check(uint8_t(opaque<uint8_t>(3) - 5) == 254 && uint16_t(opaque<uint16_t>(65535) * 65535) == 1, "unsigned narrow");

  // Division truncates toward zero, and the remainder has the sign of the dividend.
  check(eo::div(opaque<int64_t>(-7), 2) == -3 && eo::rem(opaque<int64_t>(-7), 2) == -1, "div and rem truncate");
  check(eo::div(opaque<uint64_t>(7), 2u) == 3 && eo::rem(opaque<uint64_t>(7), 2u) == 1, "unsigned div and rem");
  check(eo::div(opaque(min64), -1) == min64 && eo::rem(opaque(min64), -1) == 0, "MinInt64 / -1");
  check(eo::div(opaque<int8_t>(-128), -1) == -128 && eo::rem(opaque<int8_t>(-128), -1) == 0, "MinInt8 / -1");
  static_assert(std::is_same_v<decltype(eo::div(int8_t{1}, 1)), int8_t>, "div keeps a narrow type");
  check(panic_message([] { eo::div(opaque<int64_t>(1), 0); }) == "runtime error: integer divide by zero", "div by 0");
  check(panic_message([] { eo::rem(opaque<uint8_t>(1), 0); }) == "runtime error: integer divide by zero", "rem by 0");

  // Shifts take a count of any integer type. A count of at least the width is defined.
  check(eo::shl(int64_t{1}, opaque(62)) == int64_t{1} << 62 && eo::shl(int64_t{1}, opaque(63)) == min64, "shl");
  check(eo::shl(opaque<int64_t>(-5), uint64_t{64}) == 0 && eo::shl(opaque<uint8_t>(0xff), 4) == 0xf0, "shl width");
  check(eo::shr(opaque<int64_t>(-5), 70u) == -1 && eo::shr(opaque<int64_t>(5), 70u) == 0, "signed shr past width");
  check(eo::shr(opaque<int64_t>(-5), 1) == -3 && eo::shr(opaque<uint64_t>(5), 64) == 0, "shr");
  check(eo::shr(opaque<int8_t>(-128), 7) == -1 && eo::shl(opaque<int8_t>(1), 7) == -128, "narrow shifts");
  static_assert(std::is_same_v<decltype(eo::shl(uint16_t{1}, 3)), uint16_t>, "shl keeps a narrow type");
  check(panic_message([] { eo::shl(int64_t{1}, opaque(-1)); }) == "runtime error: negative shift amount", "shl < 0");
  check(panic_message([] { eo::shr(int64_t{1}, opaque<int8_t>(-1)); }) == "runtime error: negative shift amount",
    "shr < 0");

  // x op= y evaluates x once.
  auto values = slice<int64_t>::of({100, 7});
  auto index = 0;
  eo::div_assign(eo::at(values, index++), 3);
  check(eo::at(values, 0) == 33 && index == 1, "div_assign");
  eo::rem_assign(eo::at(values, index++), 4);
  check(eo::at(values, 1) == 3 && index == 2, "rem_assign");
  uint8_t u = 0x81;
  eo::shl_assign(u, 1);
  check(u == 2, "shl_assign");
  eo::shr_assign(u, 9);
  check(u == 0, "shr_assign");
  check(panic_message([&] { eo::div_assign(u, 0); }) == "runtime error: integer divide by zero", "div_assign by 0");

  // A Go function named shl keeps its spelling, and the qualified helper still reaches Eo.
  check(::shl(7, 2) == 9 && eo::shl(int64_t{7}, 2) == 28, "a Go shl does not collide");
}
