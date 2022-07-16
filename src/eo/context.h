#pragma once
#include <eo/core.h>

#include <any>
#include <concepts>
#include <set>

namespace eo::context {

extern const Error canceled;

struct Context {
  enum Type {
    Empty,
    Cancel,
    Timer,
    Value,
    Custom,
  };

  virtual auto done() -> std::optional<chan<>> = 0;
  virtual auto err() -> Error = 0;
  virtual auto value(Type key) -> std::any = 0;
  virtual auto type() -> Type {
    return Custom;
  }
};

struct EmptyContext : public Context {
  auto done() -> std::optional<chan<>> override;
  auto err() -> Error override;
  auto value(Type key) -> std::any override;
  auto type() -> Type override;
};

struct Canceler {
  virtual void cancel(bool remove_from_parent, Error err) = 0;
  virtual auto done() -> std::optional<chan<>> = 0;
};

struct CancelContext : public Context, public Canceler {
  CancelContext(Context*);

  auto done() -> std::optional<chan<>> override;
  auto err() -> Error override;
  auto value(Type key) -> std::any override;
  auto type() -> Type override;

  void cancel(bool remove_from_parent, Error err) override;

  Context* context;
  std::mutex mtx;
  std::set<Canceler*> children;

private:
  std::optional<chan<>> done_;
  Error err_;
};

using CancelFunc = std::function<void()>;

auto background() -> Context*;
auto with_cancel(Context* parent) -> std::tuple<std::shared_ptr<Context>, CancelFunc>;

} // namespace eo::context
