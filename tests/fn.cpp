// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#include <eo/core.h>
#include <eo/fn.h>

#include <concepts>
#include <cstdint>
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

auto twice(int64_t n) -> int64_t {
  return n * 2;
}

auto twice_later(int64_t n) -> func<int64_t> {
  co_return n * 2;
}

struct Holder {
  fn<void()> f{};

  auto operator==(const Holder&) const -> bool = default;
};

// A coroutine target cannot be a plain function value, whose call would discard the awaitable unrun.
static_assert(std::is_constructible_v<fn<int64_t(int64_t)>, decltype(&twice)>);
static_assert(!std::is_constructible_v<fn<int64_t(int64_t)>, decltype(&twice_later)>);
static_assert(!std::is_constructible_v<fn<void(int64_t)>, decltype(&twice_later)>);
static_assert(std::is_constructible_v<fn<func<int64_t>(int64_t)>, decltype(&twice_later)>);
static_assert(std::is_constructible_v<fn<func<int64_t>(int64_t)>, decltype(&twice)>);
static_assert(std::is_constructible_v<fn<func<int64_t>(int64_t)>, fn<int64_t(int64_t)>>);
static_assert(!std::is_constructible_v<fn<int64_t(int64_t)>, fn<func<int64_t>(int64_t)>>);
static_assert(!std::is_constructible_v<fn<int64_t(int64_t)>, fn<int64_t()>>);
// Go compares function values only with nil, and a struct with a function field is uncomparable.
static_assert(!std::equality_comparable<fn<void()>>);
static_assert(!std::equality_comparable<Holder>);

void nil_values() {
  fn<int64_t(int64_t)> f;
  check(f == nullptr, "the zero value is nil");
  check(panic_message([&] { f(1); }) == "runtime error: invalid memory address or nil pointer dereference",
    "calling a nil function value panics");
  f = twice;
  check(f != nullptr, "a named function is not nil");
  check(f(3) == 6, "a named function is called");
  f = nullptr;
  check(f == nullptr, "nil is assigned");

  fn<func<int64_t>(int64_t)> g;
  check(panic_message([&] { [[maybe_unused]] auto a = g(1); }).ends_with("nil pointer dereference"),
    "calling a nil coroutine function value panics at the call");
  fn<int64_t(int64_t)> none;
  g = none;
  check(g == nullptr, "a nil plain value stays nil as a coroutine value");
}

void shared_closures() {
  auto n = int64_t{0};
  fn<int64_t()> inc = [=, &n]() mutable -> int64_t { return ++n; };
  auto copy = inc;
  inc();
  copy();
  check(n == 2, "copies call the same closure");

  auto count = int64_t{0};
  fn<int64_t()> counter = [=]() mutable -> int64_t {
    count++;
    return count;
  };
  auto other = counter;
  counter();
  check(other() == 2, "copies share the closure's state");

  // The closure stays alive while it runs, even when the variable that held it is assigned.
  fn<std::string()> self;
  auto text = std::string("alive");
  self = [=, &self]() mutable -> std::string {
    self = nullptr;
    return text;
  };
  check(self() == "alive", "assigning the variable during the call keeps the closure");
  check(self == nullptr, "the assignment is kept");
}

auto coroutines() -> func<> {
  fn<func<int64_t>(int64_t)> f = twice_later;
  check(co_await f(4) == 8, "a coroutine target is awaited");
  f = twice;
  check(co_await f(5) == 10, "a plain target is adapted");
  f = fn<int64_t(int64_t)>(twice);
  check(co_await f(6) == 12, "a plain function value is adapted");

  auto calls = int64_t{0};
  fn<func<>()> g = [=, &calls]() mutable { calls++; };
  co_await g();
  g = [=, &calls]() mutable -> func<> {
    calls += 10;
    co_return;
  };
  co_await g();
  check(calls == 11, "void targets are called");
}

auto main() -> int {
  nil_values();
  shared_closures();
  invoke(coroutines());
}
