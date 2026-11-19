#pragma once
#include <eo/core.h>

#include <any>
#include <concepts>
#include <map>
#include <memory>

namespace eo::context {

extern const error Canceled;

struct Context {
  enum class Type {
    Empty,
    Cancel,
    Timer,
    Value,
    Custom,
  };

  virtual auto Done() -> std::optional<chan<>> = 0;
  virtual auto Err() -> error = 0;
  virtual auto Value(Type key) -> std::any = 0;
  virtual auto type() -> Type {
    return Type::Custom;
  }
};

struct EmptyContext : public Context {
  auto Done() -> std::optional<chan<>> override;
  auto Err() -> error override;
  auto Value(Type key) -> std::any override;
  auto type() -> Type override;
};

struct Canceler {
  virtual void cancel(bool remove_from_parent, error err) = 0;
  virtual auto Done() -> std::optional<chan<>> = 0;
};

struct CancelContext : public Context, public Canceler, public std::enable_shared_from_this<CancelContext> {
  CancelContext(Context*);
  CancelContext(std::shared_ptr<Context>);

  auto Done() -> std::optional<chan<>> override;
  auto Err() -> error override;
  auto Value(Type key) -> std::any override;
  auto type() -> Type override;

  void cancel(bool remove_from_parent, error err) override;
  auto add_child(std::shared_ptr<Canceler> child) -> error;

  Context* context;
  std::mutex mtx;
  std::map<Canceler*, std::shared_ptr<Canceler>> children;

private:
  std::shared_ptr<Context> owned_parent_;
  std::weak_ptr<CancelContext> parent_;
  bool has_managed_parent_{};
  std::optional<chan<>> done_;
  error err_;
};

using CancelFunc = std::function<void()>;

auto Background() -> Context*;
auto WithCancel(Context* parent) -> std::tuple<std::shared_ptr<Context>, CancelFunc>;
auto WithCancel(std::shared_ptr<Context> parent) -> std::tuple<std::shared_ptr<Context>, CancelFunc>;

} // namespace eo::context
