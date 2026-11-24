// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once

#include <eo/io.h>
#include <eo/strings.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>

// Go's bytes, in part, ported from Go 1.26's bytes.go, buffer.go, and reader.go. The functions that
// Go defines as returning part of their argument return a slice of the same backing array, as Go's
// do, and they return nil and empty slices where Go's do. Buffer and Reader are written as translated
// types, with static methods, so that a ptr<Buffer> is an io::Reader and an io::Writer.

namespace eo::bytes {

namespace detail {

  inline auto view(const slice<uint8_t>& s) -> std::string_view {
    if (s.data() == nullptr) {
      return {};
    }
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

// Go's bytes.MinRead.
inline constexpr int64_t MinRead = 512;

// Go's bytes.ErrTooLarge.
inline error ErrTooLarge = errors::New("bytes.Buffer: too large");

namespace detail {

  inline error errNegativeRead = errors::New("bytes.Buffer: reader returned negative count from Read");
  inline error errUnreadByte = errors::New("bytes.Buffer: UnreadByte: previous operation was not a successful read");

  inline constexpr int64_t smallBufferSize = 64;
  inline constexpr int64_t maxInt = INT64_MAX;

  // Go's readOp.
  inline constexpr int8_t opRead = -1;
  inline constexpr int8_t opInvalid = 0;
  inline constexpr int8_t opReadRune1 = 1;

  // Go's growSlice: a new slice with the bytes of b and room for n more, of capacity at least twice
  // cap(b). Go rounds the capacity up to an allocation size class, and Eo does not. An allocation
  // that Go's make would reject panics with ErrTooLarge, as Go's recover there does.
  inline auto growSlice(const slice<uint8_t>& b, int64_t n) -> slice<uint8_t> {
    auto c = eo::len(b) + n;
    if (c < 2 * eo::cap(b)) {
      c = 2 * eo::cap(b);
    }
    if (c < 0 || static_cast<uint64_t>(c) > eo::detail::max_alloc) {
      panic(ErrTooLarge);
    }
    auto b2 = eo::make_slice<uint8_t>(c);
    auto i = eo::copy(b2, b);
    return eo::subslice(b2, 0, i);
  }

  // The bytes of the UTF-8 encoding of r, as Go's utf8.AppendRune writes them.
  inline auto encode_rune(int32_t r) -> std::string {
    auto e = std::string{};
    eo::detail::append_rune(e, r);
    return e;
  }

} // namespace detail

// Go's bytes.Buffer. The zero value is an empty buffer, as in Go. Copying a Buffer copies its
// fields, so the copy shares the backing array, as in Go, where go vet reports the copy.
struct Buffer {
  static constexpr std::string_view _eo_name = "bytes.Buffer";

  slice<uint8_t> buf{};
  int64_t off{};
  int8_t lastRead{};

  auto operator==(const Buffer&) const -> bool = default;

  static auto Bytes(ptr<Buffer> b) -> slice<uint8_t> {
    return eo::subslice(b->buf, b->off);
  }

  static auto AvailableBuffer(ptr<Buffer> b) -> slice<uint8_t> {
    return eo::subslice(b->buf, eo::len(b->buf));
  }

  static auto String(ptr<Buffer> b) -> std::string {
    if (b == nullptr) {
      return "<nil>";
    }
    return eo::string_from_bytes(eo::subslice(b->buf, b->off));
  }

  static auto Peek(ptr<Buffer> b, int64_t n) -> std::tuple<slice<uint8_t>, error> {
    if (Buffer::Len(b) < n) {
      return {eo::subslice(b->buf, b->off), io::EOF_};
    }
    return {eo::subslice(b->buf, b->off, b->off + n), nullptr};
  }

  static auto empty(ptr<Buffer> b) -> bool {
    return eo::len(b->buf) <= b->off;
  }

  static auto Len(ptr<Buffer> b) -> int64_t {
    return eo::len(b->buf) - b->off;
  }

  static auto Cap(ptr<Buffer> b) -> int64_t {
    return eo::cap(b->buf);
  }

  static auto Available(ptr<Buffer> b) -> int64_t {
    return eo::cap(b->buf) - eo::len(b->buf);
  }

  static void Truncate(ptr<Buffer> b, int64_t n) {
    if (n == 0) {
      Buffer::Reset(b);
      return;
    }
    b->lastRead = detail::opInvalid;
    if (n < 0 || n > Buffer::Len(b)) {
      panic(std::string{"bytes.Buffer: truncation out of range"});
    }
    b->buf = eo::subslice(b->buf, 0, b->off + n);
  }

  static void Reset(ptr<Buffer> b) {
    b->buf = eo::subslice(b->buf, 0, 0);
    b->off = 0;
    b->lastRead = detail::opInvalid;
  }

  static auto tryGrowByReslice(ptr<Buffer> b, int64_t n) -> std::tuple<int64_t, bool> {
    if (auto l = eo::len(b->buf); n <= eo::cap(b->buf) - l) {
      b->buf = eo::subslice(b->buf, 0, l + n);
      return {l, true};
    }
    return {0, false};
  }

  static auto grow(ptr<Buffer> b, int64_t n) -> int64_t {
    auto m = Buffer::Len(b);
    if (m == 0 && b->off != 0) {
      Buffer::Reset(b);
    }
    if (auto [i, ok] = Buffer::tryGrowByReslice(b, n); ok) {
      return i;
    }
    if (b->buf == nullptr && n <= detail::smallBufferSize) {
      b->buf = eo::make_slice<uint8_t>(n, detail::smallBufferSize);
      return 0;
    }
    auto c = eo::cap(b->buf);
    if (n <= c / 2 - m) {
      eo::copy(b->buf, eo::subslice(b->buf, b->off));
    } else if (c > detail::maxInt - c - n) {
      panic(ErrTooLarge);
    } else {
      b->buf = detail::growSlice(eo::subslice(b->buf, b->off), b->off + n);
    }
    b->off = 0;
    b->buf = eo::subslice(b->buf, 0, m + n);
    return m;
  }

  static void Grow(ptr<Buffer> b, int64_t n) {
    if (n < 0) {
      panic(std::string{"bytes.Buffer.Grow: negative count"});
    }
    auto m = Buffer::grow(b, n);
    b->buf = eo::subslice(b->buf, 0, m);
  }

  static auto Write(ptr<Buffer> b, slice<uint8_t> p) -> std::tuple<int64_t, error> {
    b->lastRead = detail::opInvalid;
    auto [m, ok] = Buffer::tryGrowByReslice(b, eo::len(p));
    if (!ok) {
      m = Buffer::grow(b, eo::len(p));
    }
    return {eo::copy(eo::subslice(b->buf, m), p), nullptr};
  }

  static auto WriteString(ptr<Buffer> b, std::string s) -> std::tuple<int64_t, error> {
    b->lastRead = detail::opInvalid;
    auto [m, ok] = Buffer::tryGrowByReslice(b, eo::len(s));
    if (!ok) {
      m = Buffer::grow(b, eo::len(s));
    }
    return {eo::copy(eo::subslice(b->buf, m), s), nullptr};
  }

  static auto ReadFrom(ptr<Buffer> b, io::Reader r) -> std::tuple<int64_t, error> {
    b->lastRead = detail::opInvalid;
    auto n = int64_t{0};
    for (;;) {
      auto i = Buffer::grow(b, MinRead);
      b->buf = eo::subslice(b->buf, 0, i);
      auto [m, e] = r.Read(eo::subslice(b->buf, i, eo::cap(b->buf)));
      if (m < 0) {
        panic(detail::errNegativeRead);
      }
      b->buf = eo::subslice(b->buf, 0, i + m);
      n += m;
      if (e == io::EOF_) {
        return {n, nullptr};
      }
      if (e != nullptr) {
        return {n, e};
      }
    }
  }

  static auto WriteTo(ptr<Buffer> b, io::Writer w) -> std::tuple<int64_t, error> {
    b->lastRead = detail::opInvalid;
    auto n = int64_t{0};
    if (auto n_bytes = Buffer::Len(b); n_bytes > 0) {
      auto [m, e] = w.Write(eo::subslice(b->buf, b->off));
      if (m > n_bytes) {
        panic(std::string{"bytes.Buffer.WriteTo: invalid Write count"});
      }
      b->off += m;
      n = m;
      if (e != nullptr) {
        return {n, e};
      }
      if (m != n_bytes) {
        return {n, io::ErrShortWrite};
      }
    }
    Buffer::Reset(b);
    return {n, nullptr};
  }

  static auto WriteByte(ptr<Buffer> b, uint8_t c) -> error {
    b->lastRead = detail::opInvalid;
    auto [m, ok] = Buffer::tryGrowByReslice(b, 1);
    if (!ok) {
      m = Buffer::grow(b, 1);
    }
    eo::at(b->buf, m) = c;
    return nullptr;
  }

  static auto WriteRune(ptr<Buffer> b, int32_t r) -> std::tuple<int64_t, error> {
    if (static_cast<uint32_t>(r) < 0x80) {
      Buffer::WriteByte(b, static_cast<uint8_t>(r));
      return {1, nullptr};
    }
    b->lastRead = detail::opInvalid;
    auto [m, ok] = Buffer::tryGrowByReslice(b, 4);
    if (!ok) {
      m = Buffer::grow(b, 4);
    }
    b->buf = eo::append_slice(eo::subslice(b->buf, 0, m), detail::encode_rune(r));
    return {eo::len(b->buf) - m, nullptr};
  }

  static auto Read(ptr<Buffer> b, slice<uint8_t> p) -> std::tuple<int64_t, error> {
    b->lastRead = detail::opInvalid;
    if (Buffer::empty(b)) {
      Buffer::Reset(b);
      if (eo::len(p) == 0) {
        return {0, nullptr};
      }
      return {0, io::EOF_};
    }
    auto n = eo::copy(p, eo::subslice(b->buf, b->off));
    b->off += n;
    if (n > 0) {
      b->lastRead = detail::opRead;
    }
    return {n, nullptr};
  }

  static auto Next(ptr<Buffer> b, int64_t n) -> slice<uint8_t> {
    b->lastRead = detail::opInvalid;
    auto m = Buffer::Len(b);
    if (n > m) {
      n = m;
    }
    auto data = eo::subslice(b->buf, b->off, b->off + n);
    b->off += n;
    if (n > 0) {
      b->lastRead = detail::opRead;
    }
    return data;
  }

  static auto ReadByte(ptr<Buffer> b) -> std::tuple<uint8_t, error> {
    if (Buffer::empty(b)) {
      Buffer::Reset(b);
      return {0, io::EOF_};
    }
    auto c = eo::at(b->buf, b->off);
    b->off++;
    b->lastRead = detail::opRead;
    return {c, nullptr};
  }

  static auto ReadRune(ptr<Buffer> b) -> std::tuple<int32_t, int64_t, error> {
    if (Buffer::empty(b)) {
      Buffer::Reset(b);
      return {0, 0, io::EOF_};
    }
    auto c = eo::at(b->buf, b->off);
    if (c < 0x80) {
      b->off++;
      b->lastRead = detail::opReadRune1;
      return {static_cast<int32_t>(c), 1, nullptr};
    }
    auto [r, n] = eo::detail::decode_rune(detail::view(b->buf), static_cast<std::size_t>(b->off));
    b->off += static_cast<int64_t>(n);
    b->lastRead = static_cast<int8_t>(n);
    return {static_cast<int32_t>(r), static_cast<int64_t>(n), nullptr};
  }

  static auto UnreadRune(ptr<Buffer> b) -> error {
    if (b->lastRead <= detail::opInvalid) {
      return errors::New("bytes.Buffer: UnreadRune: previous operation was not a successful ReadRune");
    }
    if (b->off >= b->lastRead) {
      b->off -= b->lastRead;
    }
    b->lastRead = detail::opInvalid;
    return nullptr;
  }

  static auto UnreadByte(ptr<Buffer> b) -> error {
    if (b->lastRead == detail::opInvalid) {
      return detail::errUnreadByte;
    }
    b->lastRead = detail::opInvalid;
    if (b->off > 0) {
      b->off--;
    }
    return nullptr;
  }

  static auto ReadBytes(ptr<Buffer> b, uint8_t delim) -> std::tuple<slice<uint8_t>, error> {
    auto [s, err] = Buffer::readSlice(b, delim);
    auto line = eo::append_slice(slice<uint8_t>{}, s);
    return {line, err};
  }

  static auto readSlice(ptr<Buffer> b, uint8_t delim) -> std::tuple<slice<uint8_t>, error> {
    auto err = error{};
    auto i = IndexByte(eo::subslice(b->buf, b->off), delim);
    auto end = b->off + i + 1;
    if (i < 0) {
      end = eo::len(b->buf);
      err = io::EOF_;
    }
    auto line = eo::subslice(b->buf, b->off, end);
    b->off = end;
    b->lastRead = detail::opRead;
    return {line, err};
  }

  static auto ReadString(ptr<Buffer> b, uint8_t delim) -> std::tuple<std::string, error> {
    auto [s, err] = Buffer::readSlice(b, delim);
    return {eo::string_from_bytes(s), err};
  }

  using _eo_methods = eo::methods<eo::method<&Buffer::Bytes, "Bytes">,
    eo::method<&Buffer::AvailableBuffer, "AvailableBuffer">,
    eo::method<&Buffer::String, "String">,
    eo::method<&Buffer::Peek, "Peek">,
    eo::method<&Buffer::empty, "empty">,
    eo::method<&Buffer::Len, "Len">,
    eo::method<&Buffer::Cap, "Cap">,
    eo::method<&Buffer::Available, "Available">,
    eo::method<&Buffer::Truncate, "Truncate">,
    eo::method<&Buffer::Reset, "Reset">,
    eo::method<&Buffer::tryGrowByReslice, "tryGrowByReslice">,
    eo::method<&Buffer::grow, "grow">,
    eo::method<&Buffer::Grow, "Grow">,
    eo::method<&Buffer::Write, "Write">,
    eo::method<&Buffer::WriteString, "WriteString">,
    eo::method<&Buffer::ReadFrom, "ReadFrom">,
    eo::method<&Buffer::WriteTo, "WriteTo">,
    eo::method<&Buffer::WriteByte, "WriteByte">,
    eo::method<&Buffer::WriteRune, "WriteRune">,
    eo::method<&Buffer::Read, "Read">,
    eo::method<&Buffer::Next, "Next">,
    eo::method<&Buffer::ReadByte, "ReadByte">,
    eo::method<&Buffer::ReadRune, "ReadRune">,
    eo::method<&Buffer::UnreadRune, "UnreadRune">,
    eo::method<&Buffer::UnreadByte, "UnreadByte">,
    eo::method<&Buffer::ReadBytes, "ReadBytes">,
    eo::method<&Buffer::readSlice, "readSlice">,
    eo::method<&Buffer::ReadString, "ReadString">>;
};

// Go's bytes.NewBuffer.
inline auto NewBuffer(slice<uint8_t> buf) -> ptr<Buffer> {
  return eo::New<Buffer>(Buffer{std::move(buf), 0, 0});
}

// Go's bytes.NewBufferString.
inline auto NewBufferString(std::string s) -> ptr<Buffer> {
  return eo::New<Buffer>(Buffer{eo::bytes_from_string(s), 0, 0});
}

// Go's bytes.Reader.
struct Reader {
  static constexpr std::string_view _eo_name = "bytes.Reader";

  slice<uint8_t> s{};
  int64_t i{};
  int64_t prevRune{};

  auto operator==(const Reader&) const -> bool = default;

  static auto Len(ptr<Reader> r) -> int64_t {
    if (r->i >= eo::len(r->s)) {
      return 0;
    }
    return eo::len(r->s) - r->i;
  }

  static auto Size(ptr<Reader> r) -> int64_t {
    return eo::len(r->s);
  }

  static auto Read(ptr<Reader> r, slice<uint8_t> b) -> std::tuple<int64_t, error> {
    if (r->i >= eo::len(r->s)) {
      return {0, io::EOF_};
    }
    r->prevRune = -1;
    auto n = eo::copy(b, eo::subslice(r->s, r->i));
    r->i += n;
    return {n, nullptr};
  }

  static auto ReadAt(ptr<Reader> r, slice<uint8_t> b, int64_t off) -> std::tuple<int64_t, error> {
    if (off < 0) {
      return {0, errors::New("bytes.Reader.ReadAt: negative offset")};
    }
    if (off >= eo::len(r->s)) {
      return {0, io::EOF_};
    }
    auto n = eo::copy(b, eo::subslice(r->s, off));
    auto err = error{};
    if (n < eo::len(b)) {
      err = io::EOF_;
    }
    return {n, err};
  }

  static auto ReadByte(ptr<Reader> r) -> std::tuple<uint8_t, error> {
    r->prevRune = -1;
    if (r->i >= eo::len(r->s)) {
      return {0, io::EOF_};
    }
    auto b = eo::at(r->s, r->i);
    r->i++;
    return {b, nullptr};
  }

  static auto UnreadByte(ptr<Reader> r) -> error {
    if (r->i <= 0) {
      return errors::New("bytes.Reader.UnreadByte: at beginning of slice");
    }
    r->prevRune = -1;
    r->i--;
    return nullptr;
  }

  static auto ReadRune(ptr<Reader> r) -> std::tuple<int32_t, int64_t, error> {
    if (r->i >= eo::len(r->s)) {
      r->prevRune = -1;
      return {0, 0, io::EOF_};
    }
    r->prevRune = r->i;
    if (auto c = eo::at(r->s, r->i); c < 0x80) {
      r->i++;
      return {static_cast<int32_t>(c), 1, nullptr};
    }
    auto [ch, size] = eo::detail::decode_rune(detail::view(r->s), static_cast<std::size_t>(r->i));
    r->i += static_cast<int64_t>(size);
    return {static_cast<int32_t>(ch), static_cast<int64_t>(size), nullptr};
  }

  static auto UnreadRune(ptr<Reader> r) -> error {
    if (r->i <= 0) {
      return errors::New("bytes.Reader.UnreadRune: at beginning of slice");
    }
    if (r->prevRune < 0) {
      return errors::New("bytes.Reader.UnreadRune: previous operation was not ReadRune");
    }
    r->i = r->prevRune;
    r->prevRune = -1;
    return nullptr;
  }

  static auto Seek(ptr<Reader> r, int64_t offset, int64_t whence) -> std::tuple<int64_t, error> {
    r->prevRune = -1;
    auto abs = int64_t{0};
    if (whence == io::SeekStart) {
      abs = offset;
    } else if (whence == io::SeekCurrent) {
      abs = r->i + offset;
    } else if (whence == io::SeekEnd) {
      abs = eo::len(r->s) + offset;
    } else {
      return {0, errors::New("bytes.Reader.Seek: invalid whence")};
    }
    if (abs < 0) {
      return {0, errors::New("bytes.Reader.Seek: negative position")};
    }
    r->i = abs;
    return {abs, nullptr};
  }

  static auto WriteTo(ptr<Reader> r, io::Writer w) -> std::tuple<int64_t, error> {
    r->prevRune = -1;
    if (r->i >= eo::len(r->s)) {
      return {0, nullptr};
    }
    auto b = eo::subslice(r->s, r->i);
    auto [m, err] = w.Write(b);
    if (m > eo::len(b)) {
      panic(std::string{"bytes.Reader.WriteTo: invalid Write count"});
    }
    r->i += m;
    if (m != eo::len(b) && err == nullptr) {
      err = io::ErrShortWrite;
    }
    return {m, err};
  }

  static void Reset(ptr<Reader> r, slice<uint8_t> b) {
    *r = Reader{std::move(b), 0, -1};
  }

  using _eo_methods = eo::methods<eo::method<&Reader::Len, "Len">,
    eo::method<&Reader::Size, "Size">,
    eo::method<&Reader::Read, "Read">,
    eo::method<&Reader::ReadAt, "ReadAt">,
    eo::method<&Reader::ReadByte, "ReadByte">,
    eo::method<&Reader::UnreadByte, "UnreadByte">,
    eo::method<&Reader::ReadRune, "ReadRune">,
    eo::method<&Reader::UnreadRune, "UnreadRune">,
    eo::method<&Reader::Seek, "Seek">,
    eo::method<&Reader::WriteTo, "WriteTo">,
    eo::method<&Reader::Reset, "Reset">>;
};

// Go's bytes.NewReader.
inline auto NewReader(slice<uint8_t> b) -> ptr<Reader> {
  return eo::New<Reader>(Reader{std::move(b), 0, -1});
}

} // namespace eo::bytes
