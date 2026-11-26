// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once
#include <eo/defined.h>
#include <eo/panic.h>

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace eo {

template<typename T>
class slice;

namespace detail {

  struct ref_fn;
  struct ptr_access;

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
  friend struct detail::ref_fn;
  friend struct detail::ptr_access;

  explicit ptr(std::shared_ptr<T> p) noexcept: p_(std::move(p)) {}

  auto checked() const -> T* {
    auto* p = p_.get();
    if (p == nullptr) [[unlikely]] {
      detail::panic_nil_dereference();
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

namespace detail {

  struct ref_fn {
    template<typename T>
    auto operator()(T& target) const noexcept -> ptr<T> {
      return ptr<T>{std::shared_ptr<T>{std::shared_ptr<void>{}, std::addressof(target)}};
    }
  };

} // namespace detail

// &x for storage that is not moved to heap: a borrowed pointer that does not own x. It is valid only
// while x is alive, which the translation rules guarantee by boxing every variable whose address
// could outlive it. A function object, like the string helpers (see <eo/string.h>), so that ref(x) of
// a std::string or a std::array does not also find std::ref by argument-dependent lookup.
inline constexpr detail::ref_fn ref{};

namespace detail {

  // The type that Go's underlying type of T translates to: U for a defined type over U, and T itself
  // otherwise. For a struct T, that is T, as C++ has no name for Go's struct type literal.
  template<typename T>
  struct go_underlying {
    using type = T;
  };

  template<defined_type T>
  struct go_underlying<T> {
    using type = typename go_underlying<typename T::_eo_underlying>::type;
  };

  template<typename T>
  using go_underlying_t = typename go_underlying<T>::type;

} // namespace detail

// Go's conversion (*T)(p) of a pointer whose base type S has the same underlying type as T: a pointer
// to the same variable, as a T, which keeps the target of p alive. A defined type adds no data to its
// underlying type, so T and S have the same layout, and Eo builds with -fno-strict-aliasing, so that
// accesses through both pointers see each other. The C++ object model does not allow either cast:
// the target is not a T, so this relies on GCC and Clang treating a cast between layout-identical
// types with no virtual functions as the same address. A nil p converts to nil.
template<typename T, typename S>
  requires std::same_as<detail::go_underlying_t<std::remove_cv_t<T>>, detail::go_underlying_t<S>>
auto ptr_cast(const ptr<S>& p) noexcept -> ptr<T> {
  static_assert(sizeof(T) == sizeof(S) && alignof(T) == alignof(S));
  if (p == nullptr) {
    return nullptr;
  }
  if constexpr (std::is_base_of_v<S, T> || std::is_base_of_v<T, S>) {
    return ptr<T>(p, static_cast<T*>(p.get()));
  } else {
    return ptr<T>(p, reinterpret_cast<T*>(p.get()));
  }
}

namespace detail {

  // The shared pointer that a ptr holds, for reflect, which handles pointers with their types erased.
  struct ptr_access {
    template<typename T>
    static auto shared(const ptr<T>& p) noexcept -> const std::shared_ptr<T>& {
      return p.p_;
    }

    template<typename T>
    static auto make(std::shared_ptr<T> p) noexcept -> ptr<T> {
      return ptr<T>{std::move(p)};
    }
  };

} // namespace detail

namespace runtime {

  // *runtime.TypeAssertionError: a failed type assertion or interface conversion.
  struct TypeAssertionError {
    static constexpr std::string_view _eo_name = "runtime.TypeAssertionError";

    std::string message;

    auto operator==(const TypeAssertionError&) const -> bool = default;

    static auto Error(ptr<TypeAssertionError> e) -> std::string {
      return e->message;
    }
    static void RuntimeError(ptr<TypeAssertionError>) {}

    using _eo_methods =
      methods<method<&TypeAssertionError::Error, "Error">, method<&TypeAssertionError::RuntimeError, "RuntimeError">>;
  };

  // *runtime.PanicNilError: the value of panic(nil).
  struct PanicNilError {
    static constexpr std::string_view _eo_name = "runtime.PanicNilError";

    auto operator==(const PanicNilError&) const -> bool = default;

    static auto Error(ptr<PanicNilError>) -> std::string {
      return "panic called with nil argument";
    }
    static void RuntimeError(ptr<PanicNilError>) {}

    using _eo_methods =
      methods<method<&PanicNilError::Error, "Error">, method<&PanicNilError::RuntimeError, "RuntimeError">>;
  };

} // namespace runtime

namespace detail {

  [[noreturn]] inline void panic_nil_dereference() {
    panic_runtime_error("invalid memory address or nil pointer dereference");
  }

  [[noreturn]] inline void panic_type_assertion(std::string message) {
    throw Panic{any{New<runtime::TypeAssertionError>(runtime::TypeAssertionError{std::move(message)})}};
  }

  [[noreturn]] inline void panic_nil() {
    throw Panic{any{New<runtime::PanicNilError>()}};
  }

} // namespace detail

} // namespace eo

template<typename T>
struct std::hash<eo::ptr<T>> {
  auto operator()(const eo::ptr<T>& p) const noexcept -> std::size_t {
    return std::hash<T*>{}(p.get());
  }
};
