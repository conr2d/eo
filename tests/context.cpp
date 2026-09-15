// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#include <eo/context.h>

#include <atomic>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

void check(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

void test_background_context_is_never_canceled() {
  auto* ctx = eo::context::background();

  check(!ctx->done(), "background context should not have a done channel");
  check(!ctx->err(), "background context should not have an error");
}

void test_cancel_closes_done_and_sets_error() {
  auto [ctx, cancel] = eo::context::with_cancel(eo::context::background());
  auto done = ctx->done();

  check(done.has_value(), "cancelable context should have a done channel");
  check(done->raw().is_open(), "done channel should start open");
  check(!ctx->err(), "cancelable context should start without an error");

  cancel();

  check(!done->raw().is_open(), "cancel should close the done channel");
  check(ctx->err() == eo::context::canceled, "cancel should set context canceled error");
}

void test_cancel_is_idempotent() {
  auto [ctx, cancel] = eo::context::with_cancel(eo::context::background());

  cancel();
  cancel();

  check(ctx->err() == eo::context::canceled, "repeated cancel should preserve the first error");
}

void test_parent_cancel_propagates_to_child() {
  auto [parent, cancel_parent] = eo::context::with_cancel(eo::context::background());
  auto [child, cancel_child] = eo::context::with_cancel(parent.get());
  auto child_done = child->done();

  cancel_parent();

  check(child->err() == eo::context::canceled, "parent cancel should propagate the cancellation error");
  check(!child_done->raw().is_open(), "parent cancel should close the child done channel");

  cancel_child();
}

void test_parent_retains_uncanceled_child() {
  auto [parent, cancel_parent] = eo::context::with_cancel(eo::context::background());
  std::weak_ptr<eo::context::Context> child_ref;

  {
    auto [child, cancel_child] = eo::context::with_cancel(parent.get());
    child_ref = child;
  }

  check(!child_ref.expired(), "parent should retain an uncanceled child");

  cancel_parent();

  check(child_ref.expired(), "parent cancel should release retained children");
}

void test_child_cancel_releases_parent_reference() {
  auto [parent, cancel_parent] = eo::context::with_cancel(eo::context::background());
  auto [child, cancel_child] = eo::context::with_cancel(parent.get());
  std::weak_ptr<eo::context::Context> child_ref = child;

  cancel_child();
  child.reset();

  check(child_ref.expired(), "child cancel should detach it from the parent");
  cancel_parent();
}

void test_child_survives_released_parent() {
  auto [parent, cancel_parent] = eo::context::with_cancel(eo::context::background());
  auto [child, cancel_child] = eo::context::with_cancel(parent.get());
  auto child_done = child->done();
  std::weak_ptr<eo::context::Context> parent_ref = parent;

  parent.reset();

  check(parent_ref.expired(), "child should not retain its parent through an ownership cycle");
  check(!child->value(eo::context::Context::Custom).has_value(),
        "child value lookup should tolerate a released parent");

  cancel_child();

  check(child->err() == eo::context::canceled, "child cancel should survive a released parent");
  check(!child_done->raw().is_open(), "child cancel should still close done after parent release");
}

void test_child_of_canceled_parent_is_canceled_immediately() {
  auto [parent, cancel_parent] = eo::context::with_cancel(eo::context::background());
  cancel_parent();

  auto [child, cancel_child] = eo::context::with_cancel(parent.get());
  auto done = child->done();

  check(child->err() == eo::context::canceled, "child should inherit an already canceled parent error");
  check(!done->raw().is_open(), "child of canceled parent should start with closed done channel");

  cancel_child();
}

void test_concurrent_state_access_during_cancel() {
  auto [ctx, cancel] = eo::context::with_cancel(eo::context::background());
  std::atomic_bool valid{true};
  std::vector<std::thread> readers;

  for (int i = 0; i < 8; ++i) {
    readers.emplace_back([&] {
      for (int j = 0; j < 1000; ++j) {
        if (!ctx->done().has_value()) {
          valid = false;
        }
        auto err = ctx->err();
        if (err && err != eo::context::canceled) {
          valid = false;
        }
      }
    });
  }

  std::thread canceler([&] { cancel(); });

  for (auto& reader : readers) {
    reader.join();
  }
  canceler.join();

  check(valid, "concurrent context access observed invalid state");
  check(ctx->err() == eo::context::canceled, "concurrent cancel should preserve cancellation error");
  check(!ctx->done()->raw().is_open(), "concurrent cancel should close the done channel");
}

void test_nil_parent_is_rejected() {
  try {
    eo::context::with_cancel(nullptr);
  } catch (const std::runtime_error& error) {
    check(std::string{error.what()} == "cannot create context from nil parent", "nil parent returned wrong error");
    return;
  }

  throw std::runtime_error("nil parent did not panic");
}

int main() {
  test_background_context_is_never_canceled();
  test_cancel_closes_done_and_sets_error();
  test_cancel_is_idempotent();
  test_parent_cancel_propagates_to_child();
  test_parent_retains_uncanceled_child();
  test_child_cancel_releases_parent_reference();
  test_child_survives_released_parent();
  test_child_of_canceled_parent_is_canceled_immediately();
  test_concurrent_state_access_during_cancel();
  test_nil_parent_is_rejected();
}
