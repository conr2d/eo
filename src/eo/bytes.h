// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once

#include <eo/strings.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>

// Go's bytes, in part, ported from Go 1.26's bytes.go. The functions that Go defines as returning
// part of their argument return a slice of the same backing array, as Go's do, and they return nil
// and empty slices where Go's do.

namespace eo::bytes {

namespace detail {

  inline auto view(const slice<uint8_t>& s) -> std::string_view {
    return {reinterpret_cast<const char*>(s.data()), static_cast<std::size_t>(eo::len(s))};
  }

  // A new slice that holds the bytes of s: append([]byte(nil), s...), so nil when s is empty.
  inline auto from(std::string_view s) -> slice<uint8_t> {
    return eo::append_slice(slice<uint8_t>{}, std::string{s});
  }

  // A new non-nil slice with the bytes of s, as make([]byte, len(s)) followed by copy.
  inline auto make(std::string_view s) -> slice<uint8_t> {
    auto b = eo::make_slice<uint8_t>(static_cast<int64_t>(s.size()));
    std::copy(s.begin(), s.end(), b.data());
    return b;
  }

  inline auto index(const slice<uint8_t>& s, const slice<uint8_t>& sep) -> int64_t {
    return strings::detail::index(view(s), view(sep));
  }

  inline auto trim_left(slice<uint8_t> s, std::string_view cutset) -> slice<uint8_t> {
    auto v = view(s);
    auto rest = strings::detail::trim_left(v, cutset);
    if (rest.empty()) {
      return {};
    }
    return eo::subslice(s, static_cast<int64_t>(v.size() - rest.size()));
  }

  inline auto trim_right(slice<uint8_t> s, std::string_view cutset) -> slice<uint8_t> {
    auto rest = strings::detail::trim_right(view(s), cutset);
    return eo::subslice(s, 0, static_cast<int64_t>(rest.size()));
  }

} // namespace detail

inline auto Equal(slice<uint8_t> a, slice<uint8_t> b) -> bool {
  return detail::view(a) == detail::view(b);
}

inline auto Compare(slice<uint8_t> a, slice<uint8_t> b) -> int64_t {
  auto c = detail::view(a).compare(detail::view(b));
  return c < 0 ? -1 : c > 0 ? 1 : 0;
}

inline auto HasPrefix(slice<uint8_t> s, slice<uint8_t> prefix) -> bool {
  return detail::view(s).starts_with(detail::view(prefix));
}

inline auto HasSuffix(slice<uint8_t> s, slice<uint8_t> suffix) -> bool {
  return detail::view(s).ends_with(detail::view(suffix));
}

inline auto Contains(slice<uint8_t> b, slice<uint8_t> subslice) -> bool {
  return detail::index(b, subslice) >= 0;
}

inline auto Index(slice<uint8_t> s, slice<uint8_t> sep) -> int64_t {
  return detail::index(s, sep);
}

inline auto IndexByte(slice<uint8_t> b, uint8_t c) -> int64_t {
  auto i = detail::view(b).find(static_cast<char>(c));
  return i == std::string_view::npos ? -1 : static_cast<int64_t>(i);
}

inline auto LastIndex(slice<uint8_t> s, slice<uint8_t> sep) -> int64_t {
  return strings::detail::last_index(detail::view(s), detail::view(sep));
}

inline auto Count(slice<uint8_t> s, slice<uint8_t> sep) -> int64_t {
  return strings::detail::count(detail::view(s), detail::view(sep));
}

// bytes.IndexAny: the first code point of s that chars contains.
inline auto IndexAny(slice<uint8_t> s, std::string chars) -> int64_t {
  return strings::detail::index_any(detail::view(s), chars);
}

// bytes.Clone: nil for nil, and a new slice otherwise, empty and not nil for an empty one.
inline auto Clone(slice<uint8_t> b) -> slice<uint8_t> {
  if (b == nullptr) {
    return {};
  }
  return eo::append_slice(eo::make_slice<uint8_t>(0), b);
}

inline auto Repeat(slice<uint8_t> b, int64_t count) -> slice<uint8_t> {
  if (count == 0) {
    return eo::make_slice<uint8_t>(0);
  }
  auto n = strings::detail::repeat_length(static_cast<std::size_t>(eo::len(b)), count, "bytes");
  auto out = eo::make_slice<uint8_t>(static_cast<int64_t>(n));
  for (int64_t i = 0; i < static_cast<int64_t>(n); ++i) {
    out.data()[i] = b.data()[i % eo::len(b)];
  }
  return out;
}

// bytes.Join: empty and not nil for no elements, and a copy of the one element, nil when it is
// empty, for one.
inline auto Join(slice<slice<uint8_t>> s, slice<uint8_t> sep) -> slice<uint8_t> {
  if (eo::len(s) == 0) {
    return eo::make_slice<uint8_t>(0);
  }
  if (eo::len(s) == 1) {
    return detail::from(detail::view(s.data()[0]));
  }
  auto out = std::string{};
  for (int64_t i = 0; i < eo::len(s); ++i) {
    if (i > 0) {
      out += detail::view(sep);
    }
    out += detail::view(s.data()[i]);
  }
  return detail::make(out);
}

// bytes.SplitN: subslices of s, each with its capacity cut at its end except the last.
inline auto SplitN(slice<uint8_t> s, slice<uint8_t> sep, int64_t n) -> slice<slice<uint8_t>> {
  if (n == 0) {
    return {};
  }
  if (eo::len(sep) == 0) {
    if (n <= 0 || n > eo::len(s)) {
      n = eo::len(s);
    }
    auto a = eo::make_slice<slice<uint8_t>>(n);
    auto na = int64_t{0};
    while (eo::len(s) > 0) {
      if (na + 1 >= n) {
        a.data()[na] = s;
        na++;
        break;
      }
      auto size = static_cast<int64_t>(strings::detail::decode(detail::view(s)).second);
      a.data()[na] = eo::subslice(s, 0, size, size);
      s = eo::subslice(s, size);
      na++;
    }
    return eo::subslice(a, 0, na);
  }
  if (n < 0) {
    n = Count(s, sep) + 1;
  }
  if (n > eo::len(s) + 1) {
    n = eo::len(s) + 1;
  }
  auto a = eo::make_slice<slice<uint8_t>>(n);
  n--;
  int64_t i = 0;
  while (i < n) {
    auto m = detail::index(s, sep);
    if (m < 0) {
      break;
    }
    a.data()[i] = eo::subslice(s, 0, m, m);
    s = eo::subslice(s, m + eo::len(sep));
    i++;
  }
  a.data()[i] = s;
  return eo::subslice(a, 0, i + 1);
}

inline auto Split(slice<uint8_t> s, slice<uint8_t> sep) -> slice<slice<uint8_t>> {
  return SplitN(std::move(s), std::move(sep), -1);
}

inline auto Cut(slice<uint8_t> s, slice<uint8_t> sep) -> std::tuple<slice<uint8_t>, slice<uint8_t>, bool> {
  if (auto i = detail::index(s, sep); i >= 0) {
    return {eo::subslice(s, 0, i), eo::subslice(s, i + eo::len(sep)), true};
  }
  return {s, nullptr, false};
}

// bytes.Replace: a new slice, nil when the result is empty and nothing was replaced.
inline auto Replace(slice<uint8_t> s, slice<uint8_t> old, slice<uint8_t> new_, int64_t n) -> slice<uint8_t> {
  auto m = int64_t{0};
  if (n != 0) {
    m = Count(s, old);
  }
  if (m == 0) {
    return detail::from(detail::view(s));
  }
  auto replaced =
    strings::Replace(std::string{detail::view(s)}, std::string{detail::view(old)}, std::string{detail::view(new_)}, n);
  return detail::make(replaced);
}

inline auto ReplaceAll(slice<uint8_t> s, slice<uint8_t> old, slice<uint8_t> new_) -> slice<uint8_t> {
  return Replace(std::move(s), std::move(old), std::move(new_), -1);
}

template<typename F>
auto Map(F mapping, slice<uint8_t> s) -> slice<uint8_t> {
  auto mapped = strings::detail::map(mapping, detail::view(s));
  auto out = eo::make_slice<uint8_t>(0, eo::len(s));
  return eo::append_slice(std::move(out), mapped);
}

// bytes.ToUpper and bytes.ToLower: a new slice, empty and not nil for an empty s.
inline auto ToUpper(slice<uint8_t> s) -> slice<uint8_t> {
  auto v = detail::view(s);
  for (auto c : v) {
    if (static_cast<unsigned char>(c) >= 0x80) {
      auto f = [](int32_t r) { return eo::detail::unicode::to_upper(r); };
      return Map(f, std::move(s));
    }
  }
  return detail::make(strings::detail::to_upper(v));
}

inline auto ToLower(slice<uint8_t> s) -> slice<uint8_t> {
  auto v = detail::view(s);
  for (auto c : v) {
    if (static_cast<unsigned char>(c) >= 0x80) {
      auto f = [](int32_t r) { return eo::detail::unicode::to_lower(r); };
      return Map(f, std::move(s));
    }
  }
  return detail::make(strings::detail::to_lower(v));
}

inline auto EqualFold(slice<uint8_t> s, slice<uint8_t> t) -> bool {
  return strings::detail::equal_fold(detail::view(s), detail::view(t));
}

inline auto TrimPrefix(slice<uint8_t> s, slice<uint8_t> prefix) -> slice<uint8_t> {
  if (HasPrefix(s, prefix)) {
    return eo::subslice(s, eo::len(prefix));
  }
  return s;
}

inline auto TrimSuffix(slice<uint8_t> s, slice<uint8_t> suffix) -> slice<uint8_t> {
  if (HasSuffix(s, suffix)) {
    return eo::subslice(s, 0, eo::len(s) - eo::len(suffix));
  }
  return s;
}

// bytes.TrimLeftFunc, TrimRightFunc, and TrimFunc: TrimLeftFunc gives nil when it cuts everything.
template<typename F>
auto TrimLeftFunc(slice<uint8_t> s, F f) -> slice<uint8_t> {
  auto i = strings::detail::index_func(detail::view(s), f, false);
  if (i == -1) {
    return {};
  }
  return eo::subslice(s, i);
}

template<typename F>
auto TrimRightFunc(slice<uint8_t> s, F f) -> slice<uint8_t> {
  auto v = detail::view(s);
  auto i = strings::detail::last_index_func(v, f, false);
  if (i >= 0 && static_cast<unsigned char>(v[static_cast<std::size_t>(i)]) >= 0x80) {
    i += static_cast<int64_t>(strings::detail::decode(v, static_cast<std::size_t>(i)).second);
  } else {
    i++;
  }
  return eo::subslice(s, 0, i);
}

template<typename F>
auto TrimFunc(slice<uint8_t> s, F f) -> slice<uint8_t> {
  return TrimRightFunc(TrimLeftFunc(std::move(s), f), f);
}

// bytes.TrimSpace: nil when s is empty or all white space.
inline auto TrimSpace(slice<uint8_t> s) -> slice<uint8_t> {
  auto out = TrimFunc(std::move(s), strings::detail::is_space);
  if (eo::len(out) == 0) {
    return {};
  }
  return out;
}

// bytes.Trim and TrimLeft give nil when they cut everything. TrimRight keeps an empty slice of s.
inline auto Trim(slice<uint8_t> s, std::string cutset) -> slice<uint8_t> {
  if (eo::len(s) == 0) {
    return {};
  }
  if (cutset.empty()) {
    return s;
  }
  return detail::trim_left(detail::trim_right(std::move(s), cutset), cutset);
}

inline auto TrimLeft(slice<uint8_t> s, std::string cutset) -> slice<uint8_t> {
  if (eo::len(s) == 0) {
    return {};
  }
  if (cutset.empty()) {
    return s;
  }
  return detail::trim_left(std::move(s), cutset);
}

inline auto TrimRight(slice<uint8_t> s, std::string cutset) -> slice<uint8_t> {
  if (eo::len(s) == 0 || cutset.empty()) {
    return s;
  }
  return detail::trim_right(std::move(s), cutset);
}

} // namespace eo::bytes
