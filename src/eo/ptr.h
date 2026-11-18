// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once
#include <eo/error.h>
#include <eo/panic.h>

#include <cstddef>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace eo {

template<typename T>
class slice;

namespace detail {

  inline auto nil_pointer_dereference() -> const Error& {
    static const auto err = Error{"runtime error: invalid memory address or nil pointer dereference"};
    return err;
  }

  // The targets that a release has yet to destroy. The first few are held in inline slots that are
  // constructed only when used, so a release that queues only a handful, as freeing a small struct
  // does, neither allocates nor initializes the whole buffer.
  class release_queue {
  public:
    // User-provided, so a release_queue is never zero-initialized.
    release_queue() noexcept {}
    release_queue(const release_queue&) = delete;
    auto operator=(const release_queue&) -> release_queue& = delete;

    ~release_queue() {
      while (inline_size_ > 0) {
        std::destroy_at(&slots_[--inline_size_].entry);
      }
    }

    void push(std::shared_ptr<void> p) {
      if (inline_size_ < inline_capacity) {
        std::construct_at(&slots_[inline_size_].entry, std::move(p));
        ++inline_size_;
      } else {
        overflow_.push_back(std::move(p));
      }
    }

    // Moves the most recently queued target into out, or returns false when the queue is empty.
    auto pop(std::shared_ptr<void>& out) noexcept -> bool {
      if (!overflow_.empty()) {
        out = std::move(overflow_.back());
        overflow_.pop_back();
        return true;
      }
      if (inline_size_ > 0) {
        auto& last = slots_[--inline_size_].entry;
        out = std::move(last);
        std::destroy_at(&last);
        return true;
      }
      return false;
    }

  private:
    // Holds an entry only between push and pop; the union leaves it unconstructed otherwise.
    union slot {
      slot() noexcept {}
      ~slot() {}
      std::shared_ptr<void> entry;
    };

    static constexpr std::size_t inline_capacity = 16;
    slot slots_[inline_capacity];
    std::size_t inline_size_ = 0;
    std::vector<std::shared_ptr<void>> overflow_;
  };

  // Drops one owning reference without recursion. Destroying a target destroys the pointers it holds,
  // and a shared_ptr would destroy their targets from inside that destructor, so freeing a long
  // linked list would overflow the stack. Here a reference that would destroy its target while
  // another release is running on this thread is queued instead, and the outermost release destroys
  // the queued targets one at a time. The queue lives on the outermost release's stack, so no thread
  // keeps a queue after its releases finish, including during thread exit and static destruction.
  inline void release(std::shared_ptr<void> p) noexcept {
    // Trivially destructible, so it stays usable while thread-local and static objects are destroyed.
    thread_local release_queue* pending = nullptr;
    // use_count() == 1 is exact: this is the only owner, and Eo pointers have no weak references.
    if (p.use_count() != 1) {
      return; // dropping a shared reference destroys nothing, unless another thread drops the last
              // one concurrently; then this release destroys the target, which queues what it holds
    }
    if (pending != nullptr) {
      pending->push(std::move(p));
      return;
    }
    release_queue queue;
    pending = &queue;
    p.reset();
    for (auto next = std::shared_ptr<void>{}; queue.pop(next);) {
      next.reset();
    }
    pending = nullptr;
  }

} // namespace detail

// Go's *T. A pointer either owns its target, keeping it alive for as long as any copy refers to it,
// or borrows storage that Go's escape analysis proved outlives every use, such as a local variable
// that is not moved to heap. Copies share the target, as Go pointers do. There is no weak form:
// nothing reachable is ever freed early, and only a reference cycle is never freed.
//
// Dereferencing nil panics with Go's runtime error instead of being undefined behavior.
template<typename T>
class ptr {
public:
  using element_type = T;

  ptr() noexcept = default;
  ptr(std::nullptr_t) noexcept {}
  ptr(const ptr&) noexcept = default;
  ptr(ptr&&) noexcept = default;

  ~ptr() {
    detail::release(std::move(p_));
  }

  // Both read other before changing this pointer, so p = p and n->next = n->next keep the target.
  auto operator=(const ptr& other) noexcept -> ptr& {
    auto next = other.p_;
    p_.swap(next);
    detail::release(std::move(next));
    return *this;
  }

  auto operator=(ptr&& other) noexcept -> ptr& {
    auto next = std::move(other.p_);
    p_.swap(next);
    detail::release(std::move(next));
    return *this;
  }

  // &owner.f: a pointer to storage inside the target of owner, keeping that whole target alive.
  template<typename U>
  ptr(const ptr<U>& owner, T* target) noexcept: p_(owner.p_, target) {}

  // &s[i]: a pointer to an element of the backing array of s, keeping that whole array alive.
  template<typename U>
  ptr(const slice<U>& owner, T* target) noexcept: p_(owner.data_.p_, target) {}

  auto operator*() const -> T& {
    return *checked();
  }

  auto operator->() const -> T* {
    return checked();
  }

  // The address, or nullptr for nil. Not a Go operation: for the runtime and tests.
  auto get() const noexcept -> T* {
    return p_.get();
  }

  friend auto operator==(const ptr& a, const ptr& b) noexcept -> bool {
    return a.get() == b.get();
  }

  friend auto operator==(const ptr& a, std::nullptr_t) noexcept -> bool {
    return a.get() == nullptr;
  }

private:
  template<typename U>
  friend class ptr;
  template<typename U>
  friend class slice;
  template<typename U, typename... Args>
  friend auto New(Args&&... args) -> ptr<U>;
  template<typename U>
  friend auto ref(U& target) noexcept -> ptr<U>;

  explicit ptr(std::shared_ptr<T> p) noexcept: p_(std::move(p)) {}

  auto checked() const -> T* {
    auto* p = p_.get();
    if (p == nullptr) [[unlikely]] {
      panic(detail::nil_pointer_dereference());
    }
    return p;
  }

  std::shared_ptr<T> p_;
};

template<typename U, typename T>
ptr(const ptr<U>&, T*) -> ptr<T>;

template<typename U, typename T>
ptr(const slice<U>&, T*) -> ptr<T>;

// new(T), &T{...}, and a variable that Go moves to heap: a new owned T. With no arguments the
// target is value-initialized, which is Go's zero value for the types Eo maps so far.
template<typename T, typename... Args>
auto New(Args&&... args) -> ptr<T> {
  return ptr<T>{std::make_shared<T>(std::forward<Args>(args)...)};
}

// &x for storage that is not moved to heap: a borrowed pointer that does not own x. It is valid only
// while x is alive, which the translation rules guarantee by boxing every variable whose address
// could outlive it.
template<typename T>
auto ref(T& target) noexcept -> ptr<T> {
  return ptr<T>{std::shared_ptr<T>{std::shared_ptr<void>{}, std::addressof(target)}};
}

} // namespace eo

template<typename T>
struct std::hash<eo::ptr<T>> {
  auto operator()(const eo::ptr<T>& p) const noexcept -> std::size_t {
    return std::hash<T*>{}(p.get());
  }
};
