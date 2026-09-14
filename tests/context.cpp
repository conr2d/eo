// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#include <eo/context.h>

#include <stdexcept>
#include <string>

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
  test_nil_parent_is_rejected();
}
