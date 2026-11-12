// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#include <eo/core.h>

#include <fmt/format.h>

#include <any>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>

using namespace eo;

template<typename T>
concept indexes_with = requires(std::string s, T i) { eo::at(s, i); };

template<typename T>
concept converts_to_rune = requires(T r) { eo::string_from_rune(r); };

template<typename T>
concept slices_with = requires(std::string s, T i) { eo::substr(s, i, i); };

// Go functions whose names match Eo helpers, as a Go program may declare them.
auto substr(std::string s, int64_t start, int64_t n) -> std::string {
  return s.substr(static_cast<std::size_t>(start), static_cast<std::size_t>(n));
}

auto at(std::string, int64_t) -> int64_t {
  return 42;
}

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
  check(eo::len(s) == 6, "len counts bytes");
  check(eo::at(s, 1) == 0xc3 && eo::at(s, int8_t{2}) == 0xa9, "at yields bytes");
  check(fmt::format("{}", eo::at(s, 1)) == "195", "a byte prints as a number");
  check(eo::substr(s, 1, 3) == "é", "substr");
  check(eo::substr(s, 3) == "llo" && eo::substr(s, 6).empty() && eo::substr(s, 0, 0).empty(), "substr bounds");

  // Go's runtime messages, checked against the Go toolchain.
  check(panic_message([&] { eo::at(s, 9); }) == "runtime error: index out of range [9] with length 6", "index high");
  check(panic_message([&] { eo::at(s, -1); }) == "runtime error: index out of range [-1]", "index negative");
  check(panic_message([&] { eo::at(s, std::numeric_limits<uint64_t>::max()); }) ==
      "runtime error: index out of range [18446744073709551615] with length 6",
    "index unsigned");
  check(panic_message([&] { eo::substr(s, 0, 9); }) == "runtime error: slice bounds out of range [:9] with length 6",
    "slice high");
  check(panic_message([&] { eo::substr(s, 9); }) == "runtime error: slice bounds out of range [9:6]", "slice low");
  check(panic_message([&] { eo::substr(s, 2, 1); }) == "runtime error: slice bounds out of range [2:1]",
    "slice inverted");
  check(panic_message([&] { eo::substr(s, -1); }) == "runtime error: slice bounds out of range [-1:]",
    "slice low negative");
  check(panic_message([&] { eo::substr(s, 0, -1); }) == "runtime error: slice bounds out of range [:-1]",
    "slice high negative");
  check(panic_message([&] { eo::substr(s, 9, 10); }) == "runtime error: slice bounds out of range [:10] with length 6",
    "slice high is checked first");

  // A recovered runtime error prints its message.
  auto value_checked = false;
  try {
    eo::at(s, 9);
  } catch (const Panic& p) {
    check(fmt::format("{}", std::any_cast<const eo::detail::runtime_error&>(p.value())) ==
        "runtime error: index out of range [9] with length 6",
      "runtime error value");
    value_checked = true;
  }
  check(value_checked, "at out of range panics");

  // A Go function named like an Eo helper keeps its name, and the qualified call still reaches Eo.
  check(::substr("abcdef"s, 2, 4) == "cdef" && eo::substr("abcdef"s, 2, 4) == "cd", "qualified substr");
  check(::at("abc"s, 2) == 42 && eo::at("abc"s, 2) == 'c', "qualified at");

  static_assert(indexes_with<int8_t> && indexes_with<uint64_t> && !indexes_with<bool> && !indexes_with<char> &&
      converts_to_rune<int32_t> && !converts_to_rune<char32_t> && slices_with<int16_t> && !slices_with<bool>,
    "bool and character types are not Go integers");

  check(eo::string_from_rune(int32_t{0x41}) == "A", "ascii rune");
  check(eo::string_from_rune(int32_t{0xe9}) == "é", "two-byte rune");
  check(eo::string_from_rune(0x20ac) == "€", "three-byte rune");
  check(eo::string_from_rune(0x1f600) == "\U0001f600", "four-byte rune");
  check(eo::string_from_rune(0x10ffff) == "\U0010ffff", "maximum rune");
  check(eo::string_from_rune(0x110000) == "�", "rune too large");
  check(eo::string_from_rune(-1) == "�", "negative rune");
  check(eo::string_from_rune(0xd800) == "�" && eo::string_from_rune(0xdfff) == "�", "surrogate");
  check(eo::string_from_rune(uint8_t{0xff}) == "ÿ", "byte converts as a code point");
  check(eo::string_from_rune(std::numeric_limits<uint64_t>::max()) == "�", "huge unsigned rune");
  check(eo::string_from_rune(int64_t{0x1'0000'0041}) == "�", "rune beyond int32 is not truncated");

  // Literal forms: "..."s keeps a NUL byte, and \x{hh} ends where Go's \xhh does.
  auto nul = "a\x{00}b\x{ff}c"s;
  check(eo::len(nul) == 5 && eo::at(nul, 1) == 0 && eo::at(nul, 3) == 0xff && eo::at(nul, 4) == 'c', "escaped literal");
  check(R"(a\n)"s == "a\\n", "raw literal");
  check("\u0041\u0000\U0001F600"s == "A\x{00}\x{f0}\x{9f}\x{98}\x{80}"s, "universal character names");

  // Comparison is bytewise and unsigned, as in Go.
  check("\x{ff}"s > "a"s && "a"s < "b"s && "ab"s < "b"s && "a"s + "b"s == "ab"s, "comparison and concatenation");
  return 0;
}
