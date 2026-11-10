// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#include <eo/core.h>
#include <eo/fmt.h>
#include <eo/sync.h>
#include <eo/time.h>

#include <any>
#include <stdexcept>
#include <string>
#include <vector>

using namespace eo;

void check(bool condition, const char* message) {
  if (!condition) {
    throw std::logic_error(message);
  }
}

void panics_after_defers(std::vector<int>& order) {
  eo_defer_scope;
  try {
    eo_defer([&] { order.push_back(1); });
    eo_defer([&] { order.push_back(2); });
    panic("boom");
    eo_defer_run;
  } catch (...) {
    eo_defer_panic;
  }
}

void test_unrecovered_panic_runs_defers_and_propagates() {
  auto order = std::vector<int>{};
  try {
    panics_after_defers(order);
  } catch (const Panic& p) {
    check(std::string{p.what()} == "panic: boom", "panic report is wrong");
    check(std::any_cast<std::string>(p.value()) == "boom", "panic value is not the Go string");
    check((order == std::vector<int>{2, 1}), "deferred calls did not run in LIFO order");
    return;
  }
  throw std::logic_error("panic did not propagate");
}

auto recovers_value(std::any& value) -> int {
  int n{};
  eo_defer_scope;
  try {
    eo_defer([&] {
      if (auto r = recover(); r != nullptr) {
        value = r.value();
        n = 7;
      }
    });
    panic(42);
    eo_defer_run;
    return n;
  } catch (...) {
    eo_defer_panic;
  }
  return n;
}

void test_recover_returns_the_panic_value() {
  auto value = std::any{};
  check(recovers_value(value) == 7, "recovering function did not return its named result");
  check(std::any_cast<int>(value) == 42, "recover returned the wrong value");
  check(fmt::format("{}", recover()) == "<nil>", "recover outside a deferred call is not nil");
}

void normal_return_with_recover(bool& saw_panic) {
  eo_defer_scope;
  try {
    eo_defer([&] { saw_panic = recover() != nullptr; });
    eo_defer_run;
  } catch (...) {
    eo_defer_panic;
  }
}

// A deferred call of a function that returns normally cannot recover the panic that is running.
void inner_defers_do_not_recover(bool& inner_saw_panic, bool& outer_saw_panic) {
  eo_defer_scope;
  try {
    eo_defer([&] { outer_saw_panic = recover() != nullptr; });
    eo_defer([&] { normal_return_with_recover(inner_saw_panic); });
    panic("boom");
    eo_defer_run;
  } catch (...) {
    eo_defer_panic;
  }
}

void test_recover_only_in_the_panicking_frame() {
  auto inner = true;
  auto outer = false;
  inner_defers_do_not_recover(inner, outer);
  check(!inner, "a normal-return deferred call recovered the running panic");
  check(outer, "the panicking frame's deferred call did not recover");
}

auto repanics_recovered() -> std::string {
  std::string message{};
  eo_defer_scope;
  try {
    eo_defer([&] {
      auto r = recover();
      message = r.message();
    });
    eo_defer([&] { panic(recover()); });
    panic("original");
    eo_defer_run;
    return message;
  } catch (...) {
    eo_defer_panic;
  }
  return message;
}

void test_panic_of_recovered_raises_it_again() {
  check(repanics_recovered() == "original", "panic(recover()) did not raise the same panic");
}

auto panics_in_normal_drain(std::vector<std::string>& seen) -> int {
  int n{};
  eo_defer_scope;
  try {
    eo_defer([&] { seen.push_back(recover().message()); });
    eo_defer([&] { panic("from defer"); });
    eo_defer([&] { seen.push_back("before"); });
    n = 3;
    eo_defer_run;
    return n;
  } catch (...) {
    eo_defer_panic;
  }
  return n;
}

void test_panic_in_normal_drain_runs_the_rest() {
  auto seen = std::vector<std::string>{};
  check(panics_in_normal_drain(seen) == 3, "result set by return was lost");
  check((seen == std::vector<std::string>{"before", "from defer"}), "remaining deferred calls did not recover");
}

void test_runtime_panic_is_recoverable() {
  auto ch = make_chan<int>();
  ch.close();
  auto message = std::string{};
  [&] {
    eo_defer_scope;
    try {
      eo_defer([&] { message = fmt::format("{}", recover()); });
      ch.close();
      eo_defer_run;
    } catch (...) {
      eo_defer_panic;
    }
  }();
  check(message == "close of closed channel", "runtime panic message is not Go's");
}

auto coroutine_panics() -> func<int> {
  co_await time::Sleep(std::chrono::milliseconds{1});
  panic("in coroutine");
  co_return 0;
}

auto coroutine_recovers() -> func<int> {
  int n{};
  eo_defer_scope;
  try {
    eo_defer([&] {
      if (recover() != nullptr) {
        n = -1;
      }
    });
    n = co_await coroutine_panics();
    eo_defer_run;
    co_return n;
  } catch (...) {
    eo_defer_panic;
  }
  co_return n;
}

void test_panic_across_coroutines() {
  check(invoke(coroutine_recovers()) == -1, "coroutine did not recover a panic from its callee");
}

void test_other_exceptions_are_panics() {
  auto message = std::string{};
  [&] {
    eo_defer_scope;
    try {
      eo_defer([&] { message = recover().message(); });
      throw std::out_of_range("index out of range");
      eo_defer_run;
    } catch (...) {
      eo_defer_panic;
    }
  }();
  check(message == "index out of range", "a C++ exception was not recovered as a panic");
}

auto recovered_message(bool nil_from_recover) -> std::string {
  std::string message{};
  eo_defer_scope;
  try {
    eo_defer([&] { message = recover().message(); });
    if (nil_from_recover) {
      panic(recover());
    }
    panic(nullptr);
    eo_defer_run;
    return message;
  } catch (...) {
    eo_defer_panic;
  }
  return message;
}

void test_panic_nil() {
  check(recovered_message(false) == "panic called with nil argument", "panic(nil) is not Go's PanicNilError");
  check(recovered_message(true) == "panic called with nil argument", "panic of a nil recover() is not panic(nil)");
}

void panics_and_recovers_inside() {
  eo_defer_scope;
  try {
    eo_defer([&] { recover(); });
    panic("inner");
    eo_defer_run;
  } catch (...) {
    eo_defer_panic;
  }
}

// A deferred call whose callee panics and recovers on its own leaves the outer panic recoverable.
auto outer_panic_survives_inner_recover() -> std::string {
  std::string message{};
  eo_defer_scope;
  try {
    eo_defer([&] { message = recover().message(); });
    eo_defer([&] { panics_and_recovers_inside(); });
    panic("outer");
    eo_defer_run;
    return message;
  } catch (...) {
    eo_defer_panic;
  }
  return message;
}

void test_nested_recover_restores_the_outer_panic() {
  check(outer_panic_survives_inner_recover() == "outer", "the outer panic was lost after a nested recover");
}

void test_runtime_panics_are_go_panics() {
  auto wg = sync::WaitGroup{};
  try {
    wg.Done();
  } catch (const Panic& p) {
    check(p.message() == "sync: negative WaitGroup counter", "WaitGroup panic message is wrong");
    return;
  }
  throw std::logic_error("WaitGroup misuse did not raise eo::Panic");
}

int main() {
  test_unrecovered_panic_runs_defers_and_propagates();
  test_recover_returns_the_panic_value();
  test_recover_only_in_the_panicking_frame();
  test_panic_of_recovered_raises_it_again();
  test_panic_in_normal_drain_runs_the_rest();
  test_runtime_panic_is_recoverable();
  test_panic_across_coroutines();
  test_other_exceptions_are_panics();
  test_panic_nil();
  test_nested_recover_restores_the_outer_panic();
  test_runtime_panics_are_go_panics();
}
