// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#include <eo/core.h>

#include <fmt/format.h>

#include <any>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>

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

int main() {
  auto s = "héllo"s;
  check(len(s) == 6, "len counts bytes");
  check(at(s, 1) == 0xc3 && at(s, int8_t{2}) == 0xa9, "at yields bytes");
  check(fmt::format("{}", at(s, 1)) == "195", "a byte prints as a number");
  check(substr(s, 1, 3) == "é", "substr");
  check(substr(s, 3) == "llo" && substr(s, 6).empty() && substr(s, 0, 0).empty(), "substr bounds");

  // Go's runtime messages, checked against the Go toolchain.
  check(panic_message([&] { at(s, 9); }) == "runtime error: index out of range [9] with length 6", "index high");
  check(panic_message([&] { at(s, -1); }) == "runtime error: index out of range [-1]", "index negative");
  check(panic_message([&] { at(s, std::numeric_limits<uint64_t>::max()); }) ==
      "runtime error: index out of range [18446744073709551615] with length 6",
    "index unsigned");
  check(panic_message([&] { substr(s, 0, 9); }) == "runtime error: slice bounds out of range [:9] with length 6",
    "slice high");
  check(panic_message([&] { substr(s, 9); }) == "runtime error: slice bounds out of range [9:6]", "slice low");
  check(panic_message([&] { substr(s, 2, 1); }) == "runtime error: slice bounds out of range [2:1]", "slice inverted");
  check(panic_message([&] { substr(s, -1); }) == "runtime error: slice bounds out of range [-1:]",
    "slice low negative");
  check(panic_message([&] { substr(s, 0, -1); }) == "runtime error: slice bounds out of range [:-1]",
    "slice high negative");
  check(panic_message([&] { substr(s, 9, 10); }) == "runtime error: slice bounds out of range [:10] with length 6",
    "slice high is checked first");

  // A recovered runtime error prints its message.
  try {
    at(s, 9);
  } catch (const Panic& p) {
    check(fmt::format("{}", std::any_cast<const eo::detail::runtime_error&>(p.value())) ==
        "runtime error: index out of range [9] with length 6",
      "runtime error value");
  }

  check(string_from_rune(int32_t{0x41}) == "A", "ascii rune");
  check(string_from_rune(int32_t{0xe9}) == "é", "two-byte rune");
  check(string_from_rune(0x20ac) == "€", "three-byte rune");
  check(string_from_rune(0x1f600) == "\U0001f600", "four-byte rune");
  check(string_from_rune(0x10ffff) == "\U0010ffff", "maximum rune");
  check(string_from_rune(0x110000) == "�", "rune too large");
  check(string_from_rune(-1) == "�", "negative rune");
  check(string_from_rune(0xd800) == "�" && string_from_rune(0xdfff) == "�", "surrogate");
  check(string_from_rune(uint8_t{0xff}) == "ÿ", "byte converts as a code point");
  check(string_from_rune(std::numeric_limits<uint64_t>::max()) == "�", "huge unsigned rune");
  check(string_from_rune(int64_t{0x1'0000'0041}) == "�", "rune beyond int32 is not truncated");

  // Literal forms: "..."s keeps a NUL byte, and \x{hh} ends where Go's \xhh does.
  auto nul = "a\x{00}b\x{ff}c"s;
  check(len(nul) == 5 && at(nul, 1) == 0 && at(nul, 3) == 0xff && at(nul, 4) == 'c', "escaped literal");
  check(R"(a\n)"s == "a\\n", "raw literal");
  check("\u0041\u0000\U0001F600"s == "A\x{00}\x{f0}\x{9f}\x{98}\x{80}"s, "universal character names");

  // Comparison is bytewise and unsigned, as in Go.
  check("\x{ff}"s > "a"s && "a"s < "b"s && "ab"s < "b"s && "a"s + "b"s == "ab"s, "comparison and concatenation");
  return 0;
}
