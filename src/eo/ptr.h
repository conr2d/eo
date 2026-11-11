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

namespace eo {

namespace detail {

  inline auto nil_pointer_dereference() -> const Error& {
    static const auto err = Error{"runtime error: invalid memory address or nil pointer dereference"};
    return err;
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

  // &owner.f: a pointer to storage inside the target of owner, keeping that whole target alive.
  template<typename U>
  ptr(const ptr<U>& owner, T* target) noexcept: p_(owner.p_, target) {}

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
