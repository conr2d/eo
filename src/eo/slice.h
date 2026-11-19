// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once
#include <eo/bounds.h>
#include <eo/ptr.h>
#include <eo/string.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <memory>
#include <string>
#include <utility>

namespace eo {

// Defined in <eo/chan.h>. cap takes both slices and channels.
template<typename T>
class chan;

namespace detail {

  // Go's runtime limit on one allocation on 64-bit Linux, which makeslice checks against.
  inline constexpr uint64_t max_alloc = uint64_t{1} << 48;

  // Go's runtime.nextslicecap: the capacity that append picks when it must allocate. Go does not
  // specify it, and the gc compiler also rounds it to an allocation size class and may start a
  // non-escaping slice in a stack buffer, so a program cannot rely on it. Eo uses the formula
  // without the rounding.
  inline auto next_slice_cap(int64_t new_len, int64_t old_cap) -> int64_t {
    // Unsigned arithmetic wraps where Go's int does, instead of overflowing.
    auto new_cap = static_cast<uint64_t>(old_cap);
    auto double_cap = new_cap + new_cap;
    if (new_len > static_cast<int64_t>(double_cap)) {
      return new_len;
    }
    constexpr uint64_t threshold = 256;
    if (static_cast<uint64_t>(old_cap) < threshold) {
      return static_cast<int64_t>(double_cap);
    }
    // Go compares as unsigned, so a capacity that overflows ends the loop and falls back to new_len.
    while (true) {
      new_cap += (new_cap + 3 * threshold) >> 2;
      if (new_cap >= static_cast<uint64_t>(new_len)) {
        break;
      }
    }
    if (static_cast<int64_t>(new_cap) <= 0) {
      return new_len;
    }
    return static_cast<int64_t>(new_cap);
  }

  // Copies n elements from src to dst, which may overlap, as Go's memmove does.
  template<typename T>
  void move_elements(T* dst, const T* src, int64_t n) {
    if (n <= 0 || dst == src) {
      return;
    }
    if (std::less<const T*>{}(src, dst) && std::less<const T*>{}(dst, src + n)) {
      std::copy_backward(src, src + n, dst + n);
    } else {
      std::copy(src, src + n, dst);
    }
  }

} // namespace detail

// Go's []T: a view of len elements of a shared backing array that has room for cap elements. Copies
// of a slice share the array, as Go slices do, and the array stays alive for as long as any slice or
// element pointer refers to it.
//
// slice<T>{} and nullptr are the nil slice. slice<T>::of({...}) is a composite literal, which is never
// nil, even when it is empty. A translation reaches the elements and the other Go operations only
// through the eo:: functions below, which check bounds as Go does.
template<typename T>
class slice {
public:
  using value_type = T;

  slice() noexcept = default;
  slice(std::nullptr_t) noexcept {}

  // []T{a, b, c}: a new backing array holding copies of the elements, whose capacity is the length.
  // The braced list evaluates the elements left to right, as Go does.
  static auto of(std::initializer_list<T> elements) -> slice {
    auto n = static_cast<int64_t>(elements.size());
    auto s = allocate(n, n);
    std::copy(elements.begin(), elements.end(), s.data());
    return s;
  }

  // The first element, or nullptr for nil. These members are not translation forms: they serve Eo,
  // std algorithms, and tests.
  auto data() const noexcept -> T* {
    return data_.get();
  }

  auto size() const noexcept -> int64_t {
    return len_;
  }

  auto capacity() const noexcept -> int64_t {
    return cap_;
  }

  auto begin() const noexcept -> T* {
    return data();
  }

  auto end() const noexcept -> T* {
    return data() + len_;
  }

  // s == nil. Go compares a slice only with nil.
  friend auto operator==(const slice& s, std::nullptr_t) noexcept -> bool {
    return s.data() == nullptr;
  }

  // A new backing array of cap zero values, viewed as its first len elements. A zero capacity still
  // gives a non-nil slice, as make([]T, 0) and []T{} do in Go.
  static auto allocate(int64_t len, int64_t cap) -> slice {
    if (cap == 0) {
      // Never dereferenced: every index of a zero-capacity slice is out of range.
      alignas(T) static std::byte zero_base[sizeof(T)];
      auto empty = std::shared_ptr<T>{std::shared_ptr<void>{}, reinterpret_cast<T*>(zero_base)};
      return slice{ptr<T>{std::move(empty)}, 0, 0};
    }
    auto array = std::make_shared<T[]>(static_cast<std::size_t>(cap));
    auto* first = array.get();
    return slice{ptr<T>{std::shared_ptr<T>{std::move(array), first}}, len, cap};
  }

  // s[low:high:max], with the bounds already checked.
  auto view(int64_t low, int64_t high, int64_t max) const -> slice {
    return slice{ptr<T>{data_, data() + low}, high - low, max - low};
  }

private:
  template<typename U>
  friend class ptr;

  slice(ptr<T> data, int64_t len, int64_t cap) noexcept: data_(std::move(data)), len_(len), cap_(cap) {}

  ptr<T> data_;
  int64_t len_ = 0;
  int64_t cap_ = 0;
};

namespace detail {

  template<typename T>
  struct make_slice_fn {
    // make([]T, len, cap)
    template<go_integer N, go_integer C>
    auto operator()(N len, C cap) const -> slice<T> {
      auto limit = max_alloc / sizeof(T);
      if (std::cmp_less(len, 0) || std::cmp_greater(len, cap) || std::cmp_less(cap, 0) ||
        std::cmp_greater(cap, limit)) {
        if (std::cmp_less(len, 0) || std::cmp_greater(len, limit)) {
          panic_runtime_error("makeslice: len out of range");
        }
        panic_runtime_error("makeslice: cap out of range");
      }
      return slice<T>::allocate(static_cast<int64_t>(len), static_cast<int64_t>(cap));
    }

    // make([]T, len)
    template<go_integer N>
    auto operator()(N len) const -> slice<T> {
      return (*this)(len, len);
    }
  };

  struct cap_fn {
    template<typename T>
    auto operator()(const slice<T>& s) const -> int64_t {
      return s.capacity();
    }

    // cap(ch): the buffer size of ch, 0 for an unbuffered or nil channel.
    template<typename T>
    auto operator()(const chan<T>& ch) const -> int64_t {
      return ch.capacity();
    }
  };

  struct subslice_fn {
    // s[low:high]
    template<typename T, go_integer L, go_integer H>
    auto operator()(const slice<T>& s, L low, H high) const -> slice<T> {
      check_slice(low, high, s.capacity(), "capacity");
      return s.view(static_cast<int64_t>(low), static_cast<int64_t>(high), s.capacity());
    }

    // s[low:]
    template<typename T, go_integer L>
    auto operator()(const slice<T>& s, L low) const -> slice<T> {
      return (*this)(s, low, s.size());
    }

    // s[low:high:max]
    template<typename T, go_integer L, go_integer H, go_integer M>
    auto operator()(const slice<T>& s, L low, H high, M max) const -> slice<T> {
      check_slice3(low, high, max, s.capacity());
      return s.view(static_cast<int64_t>(low), static_cast<int64_t>(high), static_cast<int64_t>(max));
    }
  };

  // Appends n elements, read through src, to s: in place when they fit in s's capacity, so other
  // slices of the same array see them, and otherwise into a new array, as Go's append does.
  template<typename T, typename Copy>
  auto append_n(slice<T> s, int64_t n, Copy copy) -> slice<T> {
    if (n == 0) {
      return s;
    }
    auto len = s.size();
    auto new_len = len + n;
    if (new_len <= s.capacity()) {
      auto out = s.view(0, new_len, s.capacity());
      copy(out.data() + len);
      return out;
    }
    auto out = slice<T>::allocate(new_len, next_slice_cap(new_len, s.capacity()));
    std::copy(s.begin(), s.end(), out.data());
    copy(out.data() + len);
    return out;
  }

  struct append_fn {
    // append(s, a, b, c), as eo::append(s, {a, b, c}). The braced list evaluates the elements left to
    // right, as Go does.
    template<typename T>
    auto operator()(slice<T> s, std::initializer_list<std::type_identity_t<T>> elements) const -> slice<T> {
      return append_n(std::move(s), static_cast<int64_t>(elements.size()), [&](T* dst) {
        std::copy(elements.begin(), elements.end(), dst);
      });
    }
  };

  struct append_slice_fn {
    // append(s, t...)
    template<typename T>
    auto operator()(slice<T> s, slice<T> t) const -> slice<T> {
      return append_n(std::move(s), t.size(), [&](T* dst) { move_elements(dst, t.data(), t.size()); });
    }

    // append(b, str...), where b is a []byte
    auto operator()(slice<uint8_t> b, const std::string& str) const -> slice<uint8_t> {
      return append_n(std::move(b), static_cast<int64_t>(str.size()), [&](uint8_t* dst) {
        std::copy(str.begin(), str.end(), dst);
      });
    }
  };

  struct copy_fn {
    // copy(dst, src)
    template<typename T>
    auto operator()(const slice<T>& dst, const slice<T>& src) const -> int64_t {
      auto n = std::min(dst.size(), src.size());
      move_elements(dst.data(), src.data(), n);
      return n;
    }

    // copy(b, str), where b is a []byte
    auto operator()(const slice<uint8_t>& dst, const std::string& src) const -> int64_t {
      auto n = std::min(dst.size(), static_cast<int64_t>(src.size()));
      std::copy(src.begin(), src.begin() + n, dst.data());
      return n;
    }
  };

  struct clear_fn {
    // clear(s): sets every element in s's length to the zero value.
    template<typename T>
    void operator()(const slice<T>& s) const {
      std::fill(s.begin(), s.end(), T{});
    }

    // clear(m): deletes every entry of m, including those whose key is NaN. A nil map has none. The
    // map is defined in <eo/map.h>.
    template<typename K, typename V>
    void operator()(const map<K, V>& m) const {
      if (auto* t = m.table(); t != nullptr) {
        t->entries.clear();
        t->nans.clear();
        ++t->clears;
      }
    }
  };

  struct bytes_from_string_fn {
    // []byte(s)
    auto operator()(const std::string& s) const -> slice<uint8_t> {
      auto out = slice<uint8_t>::allocate(static_cast<int64_t>(s.size()), static_cast<int64_t>(s.size()));
      std::copy(s.begin(), s.end(), out.data());
      return out;
    }
  };

  struct string_from_bytes_fn {
    // string(b), where b is a []byte
    auto operator()(const slice<uint8_t>& b) const -> std::string {
      return std::string(b.begin(), b.end());
    }
  };

  struct runes_from_string_fn {
    // []rune(s): the code points of s, with U+FFFD for each byte that does not start a valid UTF-8
    // sequence, as Go decodes it.
    auto operator()(const std::string& s) const -> slice<int32_t> {
      auto count = int64_t{0};
      for (std::size_t i = 0; i < s.size(); i += decode_rune(s, i).second) {
        ++count;
      }
      auto out = slice<int32_t>::allocate(count, count);
      auto* dst = out.data();
      for (std::size_t i = 0; i < s.size();) {
        auto [r, n] = decode_rune(s, i);
        *dst++ = static_cast<int32_t>(r);
        i += n;
      }
      return out;
    }
  };

  struct string_from_runes_fn {
    // string(r), where r is a []rune
    auto operator()(const slice<int32_t>& runes) const -> std::string {
      auto out = std::string{};
      for (auto r : runes) {
        append_rune(out, r);
      }
      return out;
    }
  };

} // namespace detail

template<typename T>
inline constexpr detail::make_slice_fn<T> make_slice{};
inline constexpr detail::cap_fn cap{};
inline constexpr detail::subslice_fn subslice{};
inline constexpr detail::append_fn append{};
inline constexpr detail::append_slice_fn append_slice{};
inline constexpr detail::copy_fn copy{};
inline constexpr detail::clear_fn clear{};
inline constexpr detail::bytes_from_string_fn bytes_from_string{};
inline constexpr detail::string_from_bytes_fn string_from_bytes{};
inline constexpr detail::runes_from_string_fn runes_from_string{};
inline constexpr detail::string_from_runes_fn string_from_runes{};

} // namespace eo
