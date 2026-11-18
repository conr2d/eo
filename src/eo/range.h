// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once
#include <eo/bounds.h>
#include <eo/slice.h>
#include <eo/string.h>

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <string>
#include <utility>

namespace eo {

namespace detail {

  // range n: the values 0 through n-1, of n's type. The loop keeps its own counter, so a write to the
  // iteration variable in the body does not change the next value, as in Go.
  template<go_integer N>
  class int_range {
  public:
    class iterator {
    public:
      auto operator*() const noexcept -> N {
        return i_;
      }

      auto operator++() noexcept -> iterator& {
        ++i_;
        return *this;
      }

      auto operator==(std::default_sentinel_t) const noexcept -> bool {
        return !(i_ < n_);
      }

    private:
      friend class int_range;

      iterator(N n) noexcept: n_(n) {}

      N i_{0};
      N n_;
    };

    explicit int_range(N n) noexcept: n_(n) {}

    auto begin() const noexcept -> iterator {
      return iterator{n_};
    }

    auto end() const noexcept -> std::default_sentinel_t {
      return {};
    }

  private:
    N n_;
  };

  // range s, for a slice s: the index and a copy of each element. The range holds a copy of s, so the
  // length is fixed when the loop starts and assigning s in the body does not change the loop. Each
  // element is read when its iteration starts, so the body sees writes to later elements, as in Go.
  template<typename T>
  class slice_range {
  public:
    class iterator {
    public:
      auto operator*() const -> std::pair<int64_t, T> {
        return {i_, data_[i_]};
      }

      auto operator++() noexcept -> iterator& {
        ++i_;
        return *this;
      }

      auto operator==(std::default_sentinel_t) const noexcept -> bool {
        return i_ >= n_;
      }

    private:
      friend class slice_range;

      iterator(const T* data, int64_t n) noexcept: data_(data), n_(n) {}

      const T* data_;
      int64_t i_ = 0;
      int64_t n_;
    };

    explicit slice_range(slice<T> s) noexcept: s_(std::move(s)) {}

    auto begin() const noexcept -> iterator {
      return iterator{s_.data(), s_.size()};
    }

    auto end() const noexcept -> std::default_sentinel_t {
      return {};
    }

  private:
    slice<T> s_;
  };

  // range str: the byte index and the code point of each UTF-8 sequence, with U+FFFD for a byte that
  // does not start a valid sequence, which advances one byte, as Go decodes it. The range holds a copy
  // of str, so assigning str in the body does not change the loop.
  class string_range {
  public:
    class iterator {
    public:
      auto operator*() const -> std::pair<int64_t, int32_t> {
        return {static_cast<int64_t>(i_), static_cast<int32_t>(decode_rune(*s_, i_).first)};
      }

      auto operator++() -> iterator& {
        i_ += decode_rune(*s_, i_).second;
        return *this;
      }

      auto operator==(std::default_sentinel_t) const noexcept -> bool {
        return i_ >= s_->size();
      }

    private:
      friend class string_range;

      explicit iterator(const std::string* s) noexcept: s_(s) {}

      const std::string* s_;
      std::size_t i_ = 0;
    };

    explicit string_range(std::string s) noexcept: s_(std::move(s)) {}

    auto begin() const noexcept -> iterator {
      return iterator{&s_};
    }

    auto end() const noexcept -> std::default_sentinel_t {
      return {};
    }

  private:
    std::string s_;
  };

  struct range_fn {
    // range n, for an integer n
    template<go_integer N>
    auto operator()(N n) const noexcept -> int_range<N> {
      return int_range<N>{n};
    }

    // range s, for a slice s
    template<typename T>
    auto operator()(slice<T> s) const noexcept -> slice_range<T> {
      return slice_range<T>{std::move(s)};
    }

    // range str, for a string str
    auto operator()(std::string s) const -> string_range {
      return string_range{std::move(s)};
    }
  };

} // namespace detail

// The range expression of a Go for range loop, for a C++ range-based for statement, which evaluates it
// once before the first iteration, as Go does.
inline constexpr detail::range_fn range{};

} // namespace eo
