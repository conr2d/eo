#include <eo/context.h>
#include <eo/invoke.h>
#include <eo/select.h>

namespace eo::context {

const Error canceled = user_error_registry().register_error("context canceled");

auto closed_chan() -> chan<> {
  static auto c = []() {
    auto c = make_chan();
    c.close();
    return c;
  }();
  return c;
}

auto background() -> Context* {
  static EmptyContext ctx{};
  return &ctx;
}

auto EmptyContext::done() -> std::optional<chan<>> {
  return {};
}

auto EmptyContext::err() -> Error {
  return {};
}

auto EmptyContext::value(Type key) -> std::any {
  return {};
}

auto EmptyContext::type() -> Type {
  return Empty;
}

auto parent_cancel_ctx(Context* parent) -> std::tuple<CancelContext*, bool> {
  auto done = parent->done();
  if (done == closed_chan()) {
    return {nullptr, false};
  }
  CancelContext* p;
  try {
    auto pv = parent->value(Context::Type::Cancel);
    p = std::any_cast<CancelContext*>(pv);
  } catch (const std::bad_any_cast& e) {
    return {nullptr, false};
  }
  auto pdone = p->done();
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
      return c->value(key);
    }
  }
}

CancelContext::CancelContext(Context* parent): context(parent) {}

auto CancelContext::done() -> std::optional<chan<>> {
  std::unique_lock _{mtx};
  if (!done_) {
    done_ = make_chan();
  }
  return *done_;
}

auto CancelContext::err() -> Error {
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
    remove_child(context, this);
  }
}

auto CancelContext::value(Type key) -> std::any {
  if (key == Cancel) {
    return this;
  }
  return context::value(context, key);
}

auto CancelContext::type() -> Type {
  return Cancel;
}

void propagate_cancel(Context* parent, std::shared_ptr<Canceler> child) {
  auto done = parent->done();
  if (!done) {
    return; // parent is never canceled
  }
  invoke([parent, child]() -> func<> {
    auto select = Select{**parent->done(), CaseDefault()};
    switch (co_await select.index()) {
    case 0:
      co_await select.process<0>();
      child->cancel(false, parent->err());
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
      auto select = Select{**parent->done(), **child->done()};
      switch (co_await select.index()) {
      case 0:
        co_await select.process<0>();
        child->cancel(false, parent->err());
        break;
      case 1:
        co_await select.process<1>();
        break;
      }
    });
  }
}

auto with_cancel(Context* parent) -> std::tuple<std::shared_ptr<Context>, CancelFunc> {
  if (!parent) {
    throw std::runtime_error("cannot create context from nil parent");
  }
  auto c = std::make_shared<CancelContext>(parent);
  propagate_cancel(parent, c);
  return {c, [cw{std::weak_ptr<CancelContext>(c)}]() {
            if (cw.expired())
              return;
            auto c = cw.lock();
            c->cancel(true, canceled);
          }};
}

} // namespace eo::context
