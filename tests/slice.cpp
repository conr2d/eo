// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#include <eo/fmt.h>

#include <fmt/format.h>

#include <algorithm>
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

struct Node {
  int64_t value = 0;
  slice<ptr<Node>> next;
};

auto element_pointer() -> ptr<int64_t> {
  auto s = slice<int64_t>::of({1, 2, 3});
  return ptr(s, &eo::at(s, 1));
}

int main() {
  // nil and empty slices
  auto nil = slice<int64_t>{};
  check(nil == nullptr && eo::len(nil) == 0 && eo::cap(nil) == 0, "zero value is nil");
  check(eo::subslice(nil, 0) == nullptr && eo::subslice(nil, 0, 0, 0) == nullptr, "slicing nil is nil");
  auto empty = slice<int64_t>::of({});
  check(!(empty == nullptr) && eo::len(empty) == 0, "empty literal is not nil");
  check(!(eo::subslice(empty, 0, 0) == nullptr), "slicing an empty literal is not nil");
  check(!(eo::make_slice<int64_t>(0) == nullptr), "make with zero length is not nil");
  check(eo::append(nil, {}) == nullptr && eo::append_slice(nil, nil) == nullptr, "appending nothing to nil");
  check(!(eo::append_slice(empty, nil) == nullptr), "appending nothing to empty");

  // make zero-initializes, with the requested length and capacity
  auto m = eo::make_slice<int64_t>(2, 5);
  check(eo::len(m) == 2 && eo::cap(m) == 5 && eo::at(m, 1) == 0, "make");
  check(eo::at(eo::subslice(m, 0, 5), 4) == 0, "make zeroes the capacity");
  check(eo::cap(eo::make_slice<std::string>(int8_t{3})) == 3, "make with one size");

  // copies and subslices share elements
  auto a = slice<int64_t>::of({1, 2, 3});
  auto b = a;
  eo::at(b, 0) = 9;
  check(eo::at(a, 0) == 9, "assignment shares the array");
  auto t = eo::subslice(a, 1, 2);
  check(eo::len(t) == 1 && eo::cap(t) == 2, "subslice length and capacity");
  eo::at(t, 0) += 10;
  check(eo::at(a, 1) == 12, "a subslice shares the array");

  // append within capacity writes in place; past it, it copies
  t = eo::append(t, {7});
  check(eo::at(a, 2) == 7 && eo::len(t) == 2 && eo::cap(t) == 2, "append in place");
  auto u = eo::append(t, {8});
  eo::at(u, 0) = 100;
  check(eo::at(a, 1) == 12 && eo::at(u, 1) == 7 && eo::at(u, 2) == 8, "append past capacity copies");
  auto limited = eo::subslice(a, 0, 1, 1);
  check(eo::cap(limited) == 1, "three-index capacity");
  limited = eo::append(limited, {-1});
  check(eo::at(a, 1) == 12, "a three-index slice keeps append from writing in place");

  // Go's growth formula, without the size class rounding
  check(eo::detail::next_slice_cap(1, 0) == 1 && eo::detail::next_slice_cap(4, 3) == 6 &&
      eo::detail::next_slice_cap(9, 3) == 9 && eo::detail::next_slice_cap(257, 256) == 512 &&
      eo::detail::next_slice_cap(513, 512) == 832,
    "next_slice_cap");

  // append(s, s...) and overlapping copy behave as memmove
  auto c = slice<int64_t>::of({1, 2, 3, 4, 5});
  check(eo::copy(eo::subslice(c, 1), c) == 4 && fmt::format("{}", c) == "[1 1 2 3 4]", "copy forward overlap");
  c = slice<int64_t>::of({1, 2, 3, 4, 5});
  check(eo::copy(c, eo::subslice(c, 2)) == 3 && fmt::format("{}", c) == "[3 4 5 4 5]", "copy backward overlap");
  // Elements that are not trivially copyable take the element-wise path.
  auto names = slice<std::string>::of({"a"s, "b"s, "c"s, "d"s});
  check(eo::copy(eo::subslice(names, 1), names) == 3 && fmt::format("{}", names) == "[a a b c]",
    "copy forward overlap of strings");
  names = slice<std::string>::of({"a"s, "b"s, "c"s, "d"s});
  check(eo::copy(names, eo::subslice(names, 1)) == 3 && fmt::format("{}", names) == "[b c d d]",
    "copy backward overlap of strings");
  c = eo::make_slice<int64_t>(1, 4);
  eo::at(c, 0) = 1;
  auto whole = eo::subslice(c, 0, 4);
  eo::at(whole, 1) = 2;
  eo::at(whole, 2) = 3;
  c = eo::append_slice(c, eo::subslice(whole, 0, 3));
  check(fmt::format("{}", c) == "[1 1 2 3]", "append of an overlapping slice in place");
  c = eo::append_slice(c, c);
  check(fmt::format("{}", c) == "[1 1 2 3 1 1 2 3]", "append of itself past capacity");
  eo::clear(eo::subslice(c, 1, 3));
  check(fmt::format("{}", c) == "[1 0 0 3 1 1 2 3]", "clear");

  // bounds checks with Go's runtime messages
  auto s = slice<int64_t>::of({1, 2, 3});
  check(panic_message([&] { eo::at(s, 3); }) == "runtime error: index out of range [3] with length 3", "index");
  check(panic_message([&] { eo::at(nil, 0); }) == "runtime error: index out of range [0] with length 0", "index nil");
  check(panic_message([&] { eo::subslice(s, 0, 5); }) ==
      "runtime error: slice bounds out of range [:5] with capacity 3",
    "slice high");
  check(panic_message([&] { eo::subslice(s, 5); }) == "runtime error: slice bounds out of range [5:3]", "slice low");
  check(panic_message([&] { eo::subslice(s, 0, 1, 5); }) ==
      "runtime error: slice bounds out of range [::5] with capacity 3",
    "slice max");
  check(panic_message([&] { eo::subslice(s, 0, 3, 2); }) == "runtime error: slice bounds out of range [:3:2]",
    "slice3 high");
  check(panic_message([&] { eo::subslice(s, 2, 1, 3); }) == "runtime error: slice bounds out of range [2:1:]",
    "slice3 low");
  check(panic_message([&] { eo::subslice(s, -1, 1, 2); }) == "runtime error: slice bounds out of range [-1::]",
    "slice3 negative low");
  check(panic_message([&] { eo::subslice(s, 0, -1, 2); }) == "runtime error: slice bounds out of range [:-1:]",
    "slice3 negative high");
  check(panic_message([&] { eo::subslice(s, 0, 1, -1); }) == "runtime error: slice bounds out of range [::-1]",
    "slice3 negative max");
  check(panic_message([&] { eo::make_slice<int64_t>(-1); }) == "runtime error: makeslice: len out of range",
    "make negative");
  check(panic_message([&] { eo::make_slice<int64_t>(3, 2); }) == "runtime error: makeslice: cap out of range",
    "make cap below len");
  check(panic_message([&] { eo::make_slice<int64_t>(int64_t{1} << 62); }) ==
      "runtime error: makeslice: len out of range",
    "make too large");
  check(panic_message([&] { eo::make_slice<int64_t>(1, std::numeric_limits<uint64_t>::max()); }) ==
      "runtime error: makeslice: cap out of range",
    "make cap too large");

  // &s[i] keeps the backing array alive
  auto p = element_pointer();
  check(*p == 2, "element pointer outlives the slice");
  auto q = ptr(s, &eo::at(s, 2));
  s = nullptr;
  *q = 30;
  check(*q == 30, "element pointer outlives the slice variable");

  // conversions
  auto bytes = eo::bytes_from_string("h\x{00}\x{ff}"s);
  check(eo::len(bytes) == 3 && eo::at(bytes, 2) == 0xff && fmt::format("{}", bytes) == "[104 0 255]", "[]byte(s)");
  eo::at(bytes, 0) = 'j';
  check(eo::string_from_bytes(bytes) == "j\x{00}\x{ff}"s, "string(b)");
  bytes = eo::append_slice(bytes, "!?"s);
  check(eo::string_from_bytes(bytes) == "j\x{00}\x{ff}!?"s, "append(b, s...)");
  check(eo::copy(bytes, "xyz"s) == 3 && eo::string_from_bytes(bytes) == "xyz!?", "copy(b, s)");
  auto runes = eo::runes_from_string("a\x{ff}\x{c3}\x{a9}\x{e2}\x{82}"s);
  check(fmt::format("{}", runes) == "[97 65533 233 65533 65533]", "[]rune(s) of invalid bytes");
  check(fmt::format("{}", eo::runes_from_string("\x{e0}\x{80}\x{80}\x{ed}\x{a0}\x{80}\x{f4}\x{90}\x{80}\x{80}"s)) ==
      "[65533 65533 65533 65533 65533 65533 65533 65533 65533 65533]",
    "[]rune(s) rejects overlong, surrogate, and too-large encodings");
  check(eo::string_from_runes(eo::runes_from_string("\U0001F600 \x{f0}\x{9f}"s)) ==
      "\U0001F600 \x{ef}\x{bf}\x{bd}\x{ef}\x{bf}\x{bd}"s,
    "string([]rune(s))");
  check(eo::string_from_runes(slice<int32_t>::of({65, -1, 0xd800, 233})) ==
      "A\x{ef}\x{bf}\x{bd}\x{ef}\x{bf}\x{bd}\x{c3}\x{a9}"s,
    "string(r) of invalid runes");

  // printing
  check(fmt::format("{}", nil) == "[]", "nil prints []");
  check(fmt::format("{}", slice<slice<int64_t>>::of({slice<int64_t>::of({1}), nullptr})) == "[[1] []]", "nested");
  check(fmt::format("{}", slice<std::string>::of({"a"s, "b"s})) == "[a b]", "strings");
  check(fmt::format("{}", slice<bool>::of({true, false})) == "[true false]", "bools");

  // a recovered index panic carries a runtime error value
  try {
    eo::at(nil, 0);
  } catch (const Panic& e) {
    check(fmt::format("{}", std::any_cast<const eo::detail::runtime_error&>(e.value())) ==
        "runtime error: index out of range [0] with length 0",
      "runtime error value");
  }

  // std algorithms run over the view
  auto sorted = slice<int64_t>::of({3, 1, 2});
  std::sort(sorted.begin(), sorted.end());
  check(fmt::format("{}", sorted) == "[1 2 3]", "std::sort");

  // a long chain of nodes held through slices is released without recursion
  {
    auto head = New<Node>();
    auto tail = head;
    for (int64_t i = 0; i < 1'000'000; ++i) {
      auto next = New<Node>();
      tail->next = eo::append(tail->next, {next});
      tail = next;
    }
  }

  return 0;
}
