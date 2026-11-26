// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once
#include <eo/interface.h>
#include <eo/panic.h>
#include <eo/ptr.h>

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <type_traits>
#include <utility>

// Go's sync/atomic: the typed values Bool, Int32, Int64, Uint32, Uint64, Uintptr, Pointer[T], and
// Value, and the functions on *int32, *int64, *uint32, *uint64, and *uintptr. Every operation is
// sequentially consistent, as Go's are. The functions on unsafe.Pointer are not covered.
namespace eo::atomic {

namespace detail {

  // The integer that addr points to, as an atomic object. A nil addr panics as Go's dereference does.
  template<typename T>
  auto ref(const ptr<T>& addr) -> std::atomic_ref<T> {
    if (addr == nullptr) {
      eo::detail::panic_nil_dereference();
    }
    return std::atomic_ref<T>(*addr);
  }

  // The typed integers: Int32, Int64, Uint32, Uint64, and Uintptr. The zero value is 0. Like Go's,
  // they are not copied after first use, so they are neither copyable nor movable.
  template<typename T>
  struct integer {
    integer() = default;
    integer(const integer&) = delete;
    auto operator=(const integer&) -> integer& = delete;

    auto Load() const -> T {
      return v_.load();
    }

    void Store(T val) {
      v_.store(val);
    }

    auto Swap(T new_) -> T {
      return v_.exchange(new_);
    }

    auto CompareAndSwap(T old, T new_) -> bool {
      return v_.compare_exchange_strong(old, new_);
    }

    // The new value. Signed addition wraps, as in Go.
    auto Add(T delta) -> T {
      return static_cast<T>(v_.fetch_add(delta) + static_cast<std::make_unsigned_t<T>>(delta));
    }

    // The old value.
    auto And(T mask) -> T {
      return v_.fetch_and(mask);
    }

    // The old value.
    auto Or(T mask) -> T {
      return v_.fetch_or(mask);
    }

  private:
    std::atomic<T> v_{};
  };

} // namespace detail

// Go's atomic.Bool.
struct Bool {
  Bool() = default;
  Bool(const Bool&) = delete;
  auto operator=(const Bool&) -> Bool& = delete;

  auto Load() const -> bool {
    return v_.load();
  }

  void Store(bool val) {
    v_.store(val);
  }

  auto Swap(bool new_) -> bool {
    return v_.exchange(new_);
  }

  auto CompareAndSwap(bool old, bool new_) -> bool {
    return v_.compare_exchange_strong(old, new_);
  }

private:
  std::atomic<bool> v_{};
};

// Go's atomic.Int32, atomic.Int64, atomic.Uint32, atomic.Uint64, and atomic.Uintptr.
struct Int32 : detail::integer<int32_t> {};
struct Int64 : detail::integer<int64_t> {};
struct Uint32 : detail::integer<uint32_t> {};
struct Uint64 : detail::integer<uint64_t> {};
struct Uintptr : detail::integer<uintptr_t> {};

// Go's atomic.Pointer[T]: a *T that is loaded and stored atomically. CompareAndSwap compares the
// pointers, not their targets. A ptr owns its target, so the value is guarded by a lock rather than
// held in one atomic word; every operation is still atomic with respect to the others.
template<typename T>
struct Pointer {
  Pointer() = default;
  Pointer(const Pointer&) = delete;
  auto operator=(const Pointer&) -> Pointer& = delete;

  auto Load() const -> ptr<T> {
    std::lock_guard lock(m_);
    return v_;
  }

  void Store(ptr<T> val) {
    std::lock_guard lock(m_);
    v_ = std::move(val);
  }

  auto Swap(ptr<T> new_) -> ptr<T> {
    std::lock_guard lock(m_);
    return std::exchange(v_, std::move(new_));
  }

  auto CompareAndSwap(ptr<T> old, ptr<T> new_) -> bool {
    std::lock_guard lock(m_);
    if (v_ != old) {
      return false;
    }
    v_ = std::move(new_);
    return true;
  }

private:
  mutable std::mutex m_;
  ptr<T> v_;
};

// Go's atomic.Value: an any that is loaded and stored atomically. Load returns nil until the first
// Store. Every value stored must have the dynamic type of the first, and nil cannot be stored; both
// panic with Go's messages. CompareAndSwap compares with ==, which panics for an uncomparable type,
// as Go's does.
struct Value {
  Value() = default;
  Value(const Value&) = delete;
  auto operator=(const Value&) -> Value& = delete;

  auto Load() const -> any {
    std::lock_guard lock(m_);
    return v_;
  }

  void Store(any val) {
    if (val == nullptr) {
      panic(std::string("sync/atomic: store of nil value into Value"));
    }
    std::lock_guard lock(m_);
    if (v_ != nullptr && v_._eo_type != val._eo_type) {
      panic(std::string("sync/atomic: store of inconsistently typed value into Value"));
    }
    v_ = std::move(val);
  }

  auto Swap(any new_) -> any {
    if (new_ == nullptr) {
      panic(std::string("sync/atomic: swap of nil value into Value"));
    }
    std::lock_guard lock(m_);
    if (v_ != nullptr && v_._eo_type != new_._eo_type) {
      panic(std::string("sync/atomic: swap of inconsistently typed value into Value"));
    }
    return std::exchange(v_, std::move(new_));
  }

  auto CompareAndSwap(any old, any new_) -> bool {
    if (new_ == nullptr) {
      panic(std::string("sync/atomic: compare and swap of nil value into Value"));
    }
    if (old != nullptr && old._eo_type != new_._eo_type) {
      panic(std::string("sync/atomic: compare and swap of inconsistently typed values"));
    }
    std::lock_guard lock(m_);
    if (v_ == nullptr) {
      if (old != nullptr) {
        return false;
      }
      v_ = std::move(new_);
      return true;
    }
    if (v_._eo_type != new_._eo_type) {
      panic(std::string("sync/atomic: compare and swap of inconsistently typed value into Value"));
    }
    if (v_ != old) {
      return false;
    }
    v_ = std::move(new_);
    return true;
  }

private:
  mutable std::mutex m_;
  any v_;
};

// The functions on integers that addr points to. Add returns the new value, Swap, And, and Or the
// old one. A nil addr panics with Go's nil dereference.
#define EO_ATOMIC_FUNCTIONS(Name, T) \
  inline auto Add##Name(ptr<T> addr, T delta)->T { \
    return static_cast<T>(detail::ref(addr).fetch_add(delta) + static_cast<std::make_unsigned_t<T>>(delta)); \
  } \
  inline auto And##Name(ptr<T> addr, T mask)->T { \
    return detail::ref(addr).fetch_and(mask); \
  } \
  inline auto Or##Name(ptr<T> addr, T mask)->T { \
    return detail::ref(addr).fetch_or(mask); \
  } \
  inline auto CompareAndSwap##Name(ptr<T> addr, T old, T new_)->bool { \
    return detail::ref(addr).compare_exchange_strong(old, new_); \
  } \
  inline auto Load##Name(ptr<T> addr)->T { \
    return detail::ref(addr).load(); \
  } \
  inline void Store##Name(ptr<T> addr, T val) { \
    detail::ref(addr).store(val); \
  } \
  inline auto Swap##Name(ptr<T> addr, T new_)->T { \
    return detail::ref(addr).exchange(new_); \
  }

EO_ATOMIC_FUNCTIONS(Int32, int32_t)
EO_ATOMIC_FUNCTIONS(Int64, int64_t)
EO_ATOMIC_FUNCTIONS(Uint32, uint32_t)
EO_ATOMIC_FUNCTIONS(Uint64, uint64_t)
EO_ATOMIC_FUNCTIONS(Uintptr, uintptr_t)

#undef EO_ATOMIC_FUNCTIONS

} // namespace eo::atomic
