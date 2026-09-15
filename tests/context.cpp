// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#include <eo/context.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

void check(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

struct CustomContext : eo::context::Context {
  CustomContext(): done_(eo::make_chan()) {}

  auto Done() -> std::optional<eo::chan<>> override {
    return done_;
  }

  auto Err() -> eo::Error override {
    std::lock_guard _{mutex_};
    return err_;
  }

  auto Value(Type key) -> std::any override {
    if (key == Custom) {
      return 42;
    }
    return {};
  }

  void cancel() {
    {
      std::lock_guard _{mutex_};
      err_ = eo::context::canceled;
    }
    done_.close();
  }

private:
  eo::chan<> done_;
  std::mutex mutex_;
  eo::Error err_;
};

void test_background_context_is_never_canceled() {
  auto* ctx = eo::context::Background();

  check(!ctx->Done(), "background context should not have a done channel");
  check(!ctx->Err(), "background context should not have an error");
}

void test_cancel_closes_done_and_sets_error() {
  auto [ctx, cancel] = eo::context::WithCancel(eo::context::Background());
  auto done = ctx->Done();

  check(done.has_value(), "cancelable context should have a done channel");
  check(done->raw().is_open(), "done channel should start open");
  check(!ctx->Err(), "cancelable context should start without an error");

  cancel();

  check(!done->raw().is_open(), "cancel should close the done channel");
  check(ctx->Err() == eo::context::canceled, "cancel should set context canceled error");
}

void test_cancel_is_idempotent() {
  auto [ctx, cancel] = eo::context::WithCancel(eo::context::Background());

  cancel();
  cancel();

  check(ctx->Err() == eo::context::canceled, "repeated cancel should preserve the first error");
}

void test_parent_cancel_propagates_to_child() {
  auto [parent, cancel_parent] = eo::context::WithCancel(eo::context::Background());
  auto [child, cancel_child] = eo::context::WithCancel(parent.get());
  auto child_done = child->Done();

  cancel_parent();

  check(child->Err() == eo::context::canceled, "parent cancel should propagate the cancellation error");
  check(!child_done->raw().is_open(), "parent cancel should close the child done channel");

  cancel_child();
}

void test_parent_retains_uncanceled_child() {
  auto [parent, cancel_parent] = eo::context::WithCancel(eo::context::Background());
  std::weak_ptr<eo::context::Context> child_ref;

  {
    auto [child, cancel_child] = eo::context::WithCancel(parent.get());
    child_ref = child;
  }

  check(!child_ref.expired(), "parent should retain an uncanceled child");

  cancel_parent();

  check(child_ref.expired(), "parent cancel should release retained children");
}

void test_child_cancel_releases_parent_reference() {
  auto [parent, cancel_parent] = eo::context::WithCancel(eo::context::Background());
  auto [child, cancel_child] = eo::context::WithCancel(parent.get());
  std::weak_ptr<eo::context::Context> child_ref = child;

  cancel_child();
  child.reset();

  check(child_ref.expired(), "child cancel should detach it from the parent");
  cancel_parent();
}

void test_child_survives_released_parent() {
  auto [parent, cancel_parent] = eo::context::WithCancel(eo::context::Background());
  auto [child, cancel_child] = eo::context::WithCancel(parent.get());
  auto child_done = child->Done();
  std::weak_ptr<eo::context::Context> parent_ref = parent;

  parent.reset();

  check(parent_ref.expired(), "child should not retain its parent through an ownership cycle");
  check(!child->Value(eo::context::Context::Custom).has_value(),
    "child value lookup should tolerate a released parent");

  cancel_child();

  check(child->Err() == eo::context::canceled, "child cancel should survive a released parent");
  check(!child_done->raw().is_open(), "child cancel should still close done after parent release");
}

void test_shared_custom_parent_survives_external_release() {
  auto parent = std::make_shared<CustomContext>();
  std::weak_ptr<CustomContext> parent_ref = parent;
  auto [child, cancel_child] = eo::context::WithCancel(std::shared_ptr<eo::context::Context>{parent});

  parent.reset();

  check(!parent_ref.expired(), "child should retain a shared custom parent");
  check(std::any_cast<int>(child->Value(eo::context::Context::Custom)) == 42,
    "child should preserve value lookup through a shared custom parent");

  parent_ref.lock()->cancel();
  for (int i = 0; i < 100 && !child->Err(); ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  check(child->Err() == eo::context::canceled, "shared custom parent cancellation should propagate to child");
  cancel_child();
  child.reset();

  for (int i = 0; i < 100 && !parent_ref.expired(); ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  check(parent_ref.expired(), "released child should release its shared custom parent");
}

void test_child_of_canceled_parent_is_canceled_immediately() {
  auto [parent, cancel_parent] = eo::context::WithCancel(eo::context::Background());
  cancel_parent();

  auto [child, cancel_child] = eo::context::WithCancel(parent.get());
  auto done = child->Done();

  check(child->Err() == eo::context::canceled, "child should inherit an already canceled parent error");
  check(!done->raw().is_open(), "child of canceled parent should start with closed done channel");

  cancel_child();
}

void test_concurrent_state_access_during_cancel() {
  auto [ctx, cancel] = eo::context::WithCancel(eo::context::Background());
  std::atomic_bool valid{true};
  std::vector<std::thread> readers;

  for (int i = 0; i < 8; ++i) {
    readers.emplace_back([&] {
      for (int j = 0; j < 1000; ++j) {
        if (!ctx->Done().has_value()) {
          valid = false;
        }
        auto err = ctx->Err();
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
  check(ctx->Err() == eo::context::canceled, "concurrent cancel should preserve cancellation error");
  check(!ctx->Done()->raw().is_open(), "concurrent cancel should close the done channel");
}

void test_nil_parent_is_rejected() {
  try {
    eo::context::WithCancel(nullptr);
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
  test_shared_custom_parent_survives_external_release();
  test_child_of_canceled_parent_is_canceled_immediately();
  test_concurrent_state_access_during_cancel();
  test_nil_parent_is_rejected();
}
