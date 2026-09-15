#include <eo/context.h>
#include <eo/invoke.h>
#include <eo/select.h>

namespace eo::context {

const Error Canceled = user_error_registry().register_error("context canceled");

auto closed_chan() -> chan<> {
  static auto c = []() {
    auto c = make_chan();
    c.close();
    return c;
  }();
  return c;
}

auto Background() -> Context* {
  static EmptyContext ctx{};
  return &ctx;
}

auto EmptyContext::Done() -> std::optional<chan<>> {
  return {};
}

auto EmptyContext::Err() -> Error {
  return {};
}

auto EmptyContext::Value(Type key) -> std::any {
  return {};
}

auto EmptyContext::type() -> Type {
  return Type::Empty;
}

auto parent_cancel_ctx(Context* parent) -> std::tuple<CancelContext*, bool> {
  auto done = parent->Done();
  if (done == closed_chan()) {
    return {nullptr, false};
  }
  CancelContext* p;
  try {
    auto pv = parent->Value(Context::Type::Cancel);
    p = std::any_cast<CancelContext*>(pv);
  } catch (const std::bad_any_cast& e) {
    return {nullptr, false};
  }
  auto pdone = p->Done();
  if (pdone != done) {
    return {nullptr, false};
  }
  return {p, true};
}

void remove_child(Context* parent, Canceler* child) {
  auto [p, ok] = parent_cancel_ctx(parent);
  if (!ok) {
    return;
  }
  std::unique_lock _{p->mtx};
  p->children.erase(child);
}

auto value(Context* c, Context::Type key) -> std::any {
  for (;;) {
    switch (c->type()) {
    //case Context::Type::Value:
    case Context::Type::Cancel:
      if (key == Context::Type::Cancel) {
        return c;
      }
      c = static_cast<CancelContext*>(c)->context;
      break;
    //case Context::Type::Timer:
    case Context::Type::Empty:
      return {};
    default:
      return c->Value(key);
    }
  }
}

CancelContext::CancelContext(Context* parent): context(parent) {
  if (auto [p, ok] = parent_cancel_ctx(parent); ok) {
    parent_ = p->weak_from_this();
    has_managed_parent_ = !parent_.expired();
  }
}

CancelContext::CancelContext(std::shared_ptr<Context> parent): CancelContext(parent.get()) {
  if (!has_managed_parent_) {
    owned_parent_ = std::move(parent);
    context = owned_parent_.get();
  }
}

auto CancelContext::Done() -> std::optional<chan<>> {
  std::unique_lock _{mtx};
  if (!done_) {
    done_ = make_chan();
  }
  return *done_;
}

auto CancelContext::Err() -> Error {
  std::unique_lock _{mtx};
  return err_;
}

auto CancelContext::add_child(std::shared_ptr<Canceler> child) -> Error {
  std::unique_lock _{mtx};
  if (err_) {
    return err_;
  }
  children.emplace(child.get(), std::move(child));
  return {};
}

void CancelContext::cancel(bool remove_from_parent, Error err) {
  if (!err) {
    throw std::runtime_error("context: internal error: missing cancel error");
  }
  {
    std::unique_lock _{mtx};
    if (err_) {
      return; // already canceled
    }
    err_ = err;
    if (!done_) {
      done_ = closed_chan();
    } else {
      done_->close();
    }
    for (auto& [_, c] : children) {
      c->cancel(false, err);
    }
    children.clear();
  }
  if (remove_from_parent) {
    if (has_managed_parent_) {
      if (auto parent = parent_.lock()) {
        std::unique_lock _{parent->mtx};
        parent->children.erase(this);
      }
    } else {
      remove_child(context, this);
    }
  }
}

auto CancelContext::Value(Type key) -> std::any {
  if (key == Type::Cancel) {
    return this;
  }
  if (has_managed_parent_) {
    auto parent = parent_.lock();
    if (!parent) {
      return {};
    }
    return parent->Value(key);
  }
  return context::value(context, key);
}

auto CancelContext::type() -> Type {
  return Type::Cancel;
}

void propagate_cancel(Context* parent, std::shared_ptr<Canceler> child) {
  auto done = parent->Done();
  if (!done) {
    return; // parent is never canceled
  }
  invoke([parent, child]() -> func<> {
    auto select = Select{**parent->Done(), CaseDefault()};
    switch (co_await select.index()) {
    case 0:
      co_await select.process<0>();
      child->cancel(false, parent->Err());
      break;
    default:
      break;
    }
  });
  if (auto [p, ok] = parent_cancel_ctx(parent); ok) {
    if (auto err = p->add_child(child); err) {
      child->cancel(false, err);
    }
  } else {
    go([parent, child = std::move(child)]() mutable -> func<> {
      auto select = Select{**parent->Done(), **child->Done()};
      switch (co_await select.index()) {
      case 0:
        co_await select.process<0>();
        child->cancel(false, parent->Err());
        break;
      case 1:
        co_await select.process<1>();
        break;
      }
    });
  }
}

auto cancel_func(const std::shared_ptr<CancelContext>& context) -> CancelFunc {
  return [cw{std::weak_ptr<CancelContext>(context)}]() {
    if (cw.expired())
      return;
    auto c = cw.lock();
    c->cancel(true, Canceled);
  };
}

auto WithCancel(Context* parent) -> std::tuple<std::shared_ptr<Context>, CancelFunc> {
  if (!parent) {
    throw std::runtime_error("cannot create context from nil parent");
  }
  auto c = std::make_shared<CancelContext>(parent);
  propagate_cancel(parent, c);
  return {c, cancel_func(c)};
}

auto WithCancel(std::shared_ptr<Context> parent) -> std::tuple<std::shared_ptr<Context>, CancelFunc> {
  if (!parent) {
    throw std::runtime_error("cannot create context from nil parent");
  }
  auto c = std::make_shared<CancelContext>(parent);
  propagate_cancel(parent.get(), c);
  return {c, cancel_func(c)};
}

} // namespace eo::context
