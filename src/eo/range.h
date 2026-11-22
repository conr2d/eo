// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once
#include <eo/array.h>
#include <eo/bounds.h>
#include <eo/map.h>
#include <eo/slice.h>
#include <eo/string.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

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
      template<typename U, std::size_t N>
      friend class array_range;

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

  // range a, for an array a: the index and a copy of each element. Go evaluates a once, so the range
  // holds a copy of the array and the body's writes to a do not change the values.
  template<typename T, std::size_t N>
  class array_range {
  public:
    using iterator = typename slice_range<T>::iterator;

    explicit array_range(const std::array<T, N>& a): a_(a) {}

    auto begin() const noexcept -> iterator {
      return iterator{a_.data(), static_cast<int64_t>(N)};
    }

    auto end() const noexcept -> std::default_sentinel_t {
      return {};
    }

  private:
    std::array<T, N> a_;
  };

  // range p, for a pointer p to an array: the index and a copy of each element, read through p when
  // its iteration starts, so the body sees writes to later elements. A nil p panics when the first
  // element is read, and an empty array reads none, as in Go.
  template<typename A>
  class array_ptr_range {
  public:
    using T = typename array_base_t<A>::value_type;

    class iterator {
    public:
      auto operator*() const -> std::pair<int64_t, T> {
        return {i_, (**p_)[static_cast<std::size_t>(i_)]};
      }

      auto operator++() noexcept -> iterator& {
        ++i_;
        return *this;
      }

      auto operator==(std::default_sentinel_t) const noexcept -> bool {
        return i_ >= array_length<A>;
      }

    private:
      friend class array_ptr_range;

      explicit iterator(const ptr<A>* p) noexcept: p_(p) {}

      const ptr<A>* p_;
      int64_t i_ = 0;
    };

    explicit array_ptr_range(ptr<A> p) noexcept: p_(std::move(p)) {}

    auto begin() const noexcept -> iterator {
      return iterator{&p_};
    }

    auto end() const noexcept -> std::default_sentinel_t {
      return {};
    }

  private:
    ptr<A> p_;
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

  // range m, for a map m: each key and a copy of its value, in an unspecified order. The range holds
  // a copy of m and the keys it has when the loop starts. Each iteration looks its key up again, so
  // an entry that the body deletes before the loop reaches it is not produced, and the value is the
  // one the entry has when its iteration starts. An entry that the body adds is not produced, which
  // Go allows. An entry whose key is NaN cannot be deleted or assigned, so its copy is produced unless
  // the body clears the map.
  template<typename K, typename V>
  class map_range {
  public:
    class iterator {
    public:
      auto operator*() const -> std::pair<K, V> {
        return {*key_, *value_};
      }

      auto operator++() noexcept -> iterator& {
        ++i_;
        return *this;
      }

      // Finds the next entry that is still in the map, so that operator* reads it right after.
      auto operator==(std::default_sentinel_t) -> bool {
        auto* t = range_->m_.table();
        for (; i_ < range_->keys_.size(); ++i_) {
          auto it = t->entries.find(range_->keys_[i_]);
          if (it != t->entries.end()) {
            key_ = &it->first;
            value_ = &it->second;
            return false;
          }
        }
        auto j = i_ - range_->keys_.size();
        if (t != nullptr && t->clears == range_->clears_ && j < range_->nans_.size()) {
          key_ = &range_->nans_[j].first;
          value_ = &range_->nans_[j].second;
          return false;
        }
        return true;
      }

    private:
      friend class map_range;

      explicit iterator(const map_range* range) noexcept: range_(range) {}

      const map_range* range_;
      std::size_t i_ = 0;
      const K* key_ = nullptr;
      const V* value_ = nullptr;
    };

    explicit map_range(map<K, V> m): m_(std::move(m)) {
      if (auto* t = m_.table(); t != nullptr) {
        keys_.reserve(t->entries.size());
        for (const auto& entry : t->entries) {
          keys_.push_back(entry.first);
        }
        nans_.assign(t->nans.begin(), t->nans.end());
        clears_ = t->clears;
      }
    }

    auto begin() const noexcept -> iterator {
      return iterator{this};
    }

    auto end() const noexcept -> std::default_sentinel_t {
      return {};
    }

  private:
    map<K, V> m_;
    std::vector<K> keys_;
    std::vector<std::pair<K, V>> nans_;
    uint64_t clears_ = 0;
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

    // range a, for an array a
    template<typename T, std::size_t N>
    auto operator()(const std::array<T, N>& a) const -> array_range<T, N> {
      return array_range<T, N>{a};
    }

    // range p, for a pointer p to an array
    template<go_array A>
    auto operator()(ptr<A> p) const noexcept -> array_ptr_range<A> {
      return array_ptr_range<A>{std::move(p)};
    }

    // range m, for a map m
    template<typename K, typename V>
    auto operator()(map<K, V> m) const -> map_range<K, V> {
      return map_range<K, V>{std::move(m)};
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
