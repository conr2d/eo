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
  if (p->children.size()) {
    if (auto it = p->children.find(child); it != p->children.end()) {
      p->children.erase(it);
    }
  }
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
  if (done_) {
    return *done_;
  }
  std::unique_lock _{mtx};
  if (!done_) {
    done_ = make_chan();
  }
  return *done_;
}

auto CancelContext::err() -> Error {
  return err_;
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
    for (auto& c : children) {
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

void propagate_cancel(Context* parent, Canceler* child) {
  auto done = parent->done();
  if (!done) {
    return; // parent is never canceled
  }
  invoke([&]() -> func<> {
    auto select = Select{**done, CaseDefault()};
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
    std::unique_lock _{p->mtx};
    if (p->err()) {
      // parent has already been canceled
      child->cancel(false, p->err());
    } else {
      p->children.insert(child);
    }
  } else {
    go([=]() mutable -> func<> {
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
  propagate_cancel(parent, c.get());
  return {c, [cw{std::weak_ptr<CancelContext>(c)}]() {
            if (cw.expired())
              return;
            auto c = cw.lock();
            c->cancel(true, canceled);
          }};
}

} // namespace eo::context
