// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once

#include <eo/core.h>
#include <eo/io.h>
#include <eo/unicode/case.h>

#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

// Go's strings, in part, ported from Go 1.26's strings.go, builder.go, replace.go, and reader.go. A
// function that Go gives a string parameter takes a std::string, and the code points of a string are
// decoded as `range` decodes them: a byte that does not start a valid UTF-8 sequence is U+FFFD.
// Reader is written as a translated type, with static methods, so that a ptr<Reader> is an io::Reader.

namespace eo::strings {

namespace detail {

  inline constexpr int32_t rune_error = 0xfffd;

  // utf8.DecodeRuneInString(s[i:]).
  inline auto decode(std::string_view s, std::size_t i = 0) -> std::pair<int32_t, std::size_t> {
    if (i >= s.size()) {
      return {rune_error, 0};
    }
    auto [r, n] = eo::detail::decode_rune(s, i);
    return {static_cast<int32_t>(r), n};
  }

  // utf8.DecodeLastRuneInString(s).
  inline auto decode_last(std::string_view s) -> std::pair<int32_t, std::size_t> {
    if (s.empty()) {
      return {rune_error, 0};
    }
    auto end = s.size();
    auto start = end - 1;
    if (static_cast<unsigned char>(s[start]) < 0x80) {
      return {static_cast<unsigned char>(s[start]), 1};
    }
    auto lim = end >= 4 ? end - 4 : 0;
    // Back up to the start of a sequence, at most three continuation bytes.
    while (start > lim) {
      --start;
      if ((static_cast<unsigned char>(s[start]) & 0xc0) != 0x80) {
        break;
      }
    }
    auto [r, size] = decode(s.substr(start, end - start));
    if (start + size != end) {
      return {rune_error, 1};
    }
    return {r, size};
  }

  inline auto rune_count(std::string_view s) -> int64_t {
    auto n = int64_t{0};
    for (std::size_t i = 0; i < s.size(); ++n) {
      i += decode(s, i).second;
    }
    return n;
  }

  inline auto index(std::string_view s, std::string_view sub) -> int64_t {
    auto i = s.find(sub);
    return i == std::string_view::npos ? -1 : static_cast<int64_t>(i);
  }

  inline auto last_index(std::string_view s, std::string_view sub) -> int64_t {
    auto i = s.rfind(sub);
    return i == std::string_view::npos ? -1 : static_cast<int64_t>(i);
  }

  inline auto count(std::string_view s, std::string_view sub) -> int64_t {
    if (sub.empty()) {
      return rune_count(s) + 1;
    }
    auto n = int64_t{0};
    for (auto i = s.find(sub); i != std::string_view::npos; i = s.find(sub, i + sub.size())) {
      ++n;
    }
    return n;
  }

  inline auto valid_rune(int32_t r) -> bool {
    return (0 <= r && r < 0xd800) || (0xdfff < r && r <= 0x10ffff);
  }

  // strings.IndexRune: an invalid r other than U+FFFD is never found, and U+FFFD matches an invalid
  // byte or the encoding of U+FFFD.
  inline auto index_rune(std::string_view s, int32_t r) -> int64_t {
    if (0 <= r && r < 0x80) {
      auto i = s.find(static_cast<char>(r));
      return i == std::string_view::npos ? -1 : static_cast<int64_t>(i);
    }
    if (r == rune_error) {
      for (std::size_t i = 0; i < s.size();) {
        auto [c, n] = decode(s, i);
        if (c == rune_error) {
          return static_cast<int64_t>(i);
        }
        i += n;
      }
      return -1;
    }
    if (!valid_rune(r)) {
      return -1;
    }
    auto e = std::string{};
    eo::detail::append_rune(e, r);
    return index(s, e);
  }

  inline auto contains_rune(std::string_view s, int32_t r) -> bool {
    return index_rune(s, r) >= 0;
  }

  // strings.IndexAny: the first code point of s that chars contains.
  inline auto index_any(std::string_view s, std::string_view chars) -> int64_t {
    if (chars.empty()) {
      return -1;
    }
    for (std::size_t i = 0; i < s.size();) {
      auto [r, n] = decode(s, i);
      if (index_rune(chars, r) >= 0) {
        return static_cast<int64_t>(i);
      }
      i += n;
    }
    return -1;
  }

  template<typename F>
  auto index_func(std::string_view s, F& f, bool truth) -> int64_t {
    for (std::size_t i = 0; i < s.size();) {
      auto [r, n] = decode(s, i);
      if (static_cast<bool>(f(r)) == truth) {
        return static_cast<int64_t>(i);
      }
      i += n;
    }
    return -1;
  }

  template<typename F>
  auto last_index_func(std::string_view s, F& f, bool truth) -> int64_t {
    for (auto i = s.size(); i > 0;) {
      auto [r, size] = decode_last(s.substr(0, i));
      i -= size;
      if (static_cast<bool>(f(r)) == truth) {
        return static_cast<int64_t>(i);
      }
    }
    return -1;
  }

  template<typename F>
  auto trim_left_func(std::string_view s, F& f) -> std::string_view {
    auto i = index_func(s, f, false);
    if (i == -1) {
      return {};
    }
    return s.substr(static_cast<std::size_t>(i));
  }

  template<typename F>
  auto trim_right_func(std::string_view s, F& f) -> std::string_view {
    auto i = last_index_func(s, f, false);
    if (i >= 0 && static_cast<unsigned char>(s[static_cast<std::size_t>(i)]) >= 0x80) {
      i += static_cast<int64_t>(decode(s, static_cast<std::size_t>(i)).second);
    } else {
      i++;
    }
    return s.substr(0, static_cast<std::size_t>(i));
  }

  // The cutset of Trim, TrimLeft, and TrimRight: a code point is cut when cutset contains it.
  inline auto trim_left(std::string_view s, std::string_view cutset) -> std::string_view {
    while (!s.empty()) {
      auto [r, n] = decode(s);
      if (!contains_rune(cutset, r)) {
        break;
      }
      s.remove_prefix(n);
    }
    return s;
  }

  inline auto trim_right(std::string_view s, std::string_view cutset) -> std::string_view {
    while (!s.empty()) {
      auto r = static_cast<int32_t>(static_cast<unsigned char>(s.back()));
      auto n = std::size_t{1};
      if (r >= 0x80) {
        std::tie(r, n) = decode_last(s);
      }
      if (!contains_rune(cutset, r)) {
        break;
      }
      s.remove_suffix(n);
    }
    return s;
  }

  inline auto ascii_space(unsigned char c) -> bool {
    return c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r' || c == ' ';
  }

  // strings.Map and bytes.Map: each code point of s mapped, dropping those that map to a negative
  // value. An invalid byte is U+FFFD, and a mapped value that is not a code point encodes as U+FFFD.
  template<typename F>
  auto map(F& mapping, std::string_view s) -> std::string {
    auto out = std::string{};
    out.reserve(s.size());
    for (std::size_t i = 0; i < s.size();) {
      auto [c, n] = decode(s, i);
      auto r = static_cast<int32_t>(mapping(c));
      if (r >= 0) {
        eo::detail::append_rune(out, r);
      }
      i += n;
    }
    return out;
  }

  inline auto is_space(int32_t r) -> bool {
    return eo::detail::unicode::is_space(r);
  }

  // strings.EqualFold and bytes.EqualFold: equal under simple Unicode case folding.
  inline auto equal_fold(std::string_view s, std::string_view t) -> bool {
    std::size_t i = 0;
    for (auto n = std::min(s.size(), t.size()); i < n; ++i) {
      auto sr = static_cast<unsigned char>(s[i]);
      auto tr = static_cast<unsigned char>(t[i]);
      if ((sr | tr) >= 0x80) {
        break;
      }
      if (tr == sr) {
        continue;
      }
      if (tr < sr) {
        std::swap(tr, sr);
      }
      if ('A' <= sr && sr <= 'Z' && tr == sr + 'a' - 'A') {
        continue;
      }
      return false;
    }
    if (i == std::min(s.size(), t.size())) {
      return s.size() == t.size();
    }
    s.remove_prefix(i);
    t.remove_prefix(i);
    while (!s.empty() && !t.empty()) {
      auto [sr, sn] = decode(s);
      s.remove_prefix(sn);
      auto [tr, tn] = decode(t);
      t.remove_prefix(tn);
      if (tr == sr) {
        continue;
      }
      if (tr < sr) {
        std::swap(tr, sr);
      }
      if (tr < 0x80) {
        if ('A' <= sr && sr <= 'Z' && tr == sr + 'a' - 'A') {
          continue;
        }
        return false;
      }
      auto r = eo::detail::unicode::simple_fold(sr);
      while (r != sr && r < tr) {
        r = eo::detail::unicode::simple_fold(r);
      }
      if (r == tr) {
        continue;
      }
      return false;
    }
    return s.size() == t.size();
  }

  inline auto to_upper(std::string_view s) -> std::string {
    auto ascii = true;
    for (auto c : s) {
      if (static_cast<unsigned char>(c) >= 0x80) {
        ascii = false;
        break;
      }
    }
    if (ascii) {
      auto out = std::string{s};
      for (auto& c : out) {
        if ('a' <= c && c <= 'z') {
          c = static_cast<char>(c - ('a' - 'A'));
        }
      }
      return out;
    }
    auto f = [](int32_t r) { return eo::detail::unicode::to_upper(r); };
    return map(f, s);
  }

  inline auto to_lower(std::string_view s) -> std::string {
    auto ascii = true;
    for (auto c : s) {
      if (static_cast<unsigned char>(c) >= 0x80) {
        ascii = false;
        break;
      }
    }
    if (ascii) {
      auto out = std::string{s};
      for (auto& c : out) {
        if ('A' <= c && c <= 'Z') {
          c = static_cast<char>(c + ('a' - 'A'));
        }
      }
      return out;
    }
    auto f = [](int32_t r) { return eo::detail::unicode::to_lower(r); };
    return map(f, s);
  }

  // The new length of a Repeat, with Go's panics.
  inline auto repeat_length(std::size_t len, int64_t count, const char* package) -> std::size_t {
    if (count < 0) {
      panic(std::string{package} + ": negative Repeat count");
    }
    auto c = static_cast<uint64_t>(count);
    if (len != 0 && c > static_cast<uint64_t>(INT64_MAX) / len) {
      panic(std::string{package} + ": Repeat output length overflow");
    }
    return len * static_cast<std::size_t>(c);
  }

} // namespace detail

inline auto Compare(std::string a, std::string b) -> int64_t {
  auto c = a.compare(b);
  return c < 0 ? -1 : c > 0 ? 1 : 0;
}

inline auto Contains(std::string s, std::string substr) -> bool {
  return detail::index(s, substr) >= 0;
}

inline auto ContainsAny(std::string s, std::string chars) -> bool {
  return detail::index_any(s, chars) >= 0;
}

inline auto ContainsRune(std::string s, int32_t r) -> bool {
  return detail::index_rune(s, r) >= 0;
}

inline auto Count(std::string s, std::string substr) -> int64_t {
  return detail::count(s, substr);
}

inline auto HasPrefix(std::string s, std::string prefix) -> bool {
  return s.starts_with(prefix);
}

inline auto HasSuffix(std::string s, std::string suffix) -> bool {
  return s.ends_with(suffix);
}

inline auto Index(std::string s, std::string substr) -> int64_t {
  return detail::index(s, substr);
}

inline auto IndexAny(std::string s, std::string chars) -> int64_t {
  return detail::index_any(s, chars);
}

inline auto IndexByte(std::string s, uint8_t c) -> int64_t {
  auto i = s.find(static_cast<char>(c));
  return i == std::string::npos ? -1 : static_cast<int64_t>(i);
}

inline auto IndexRune(std::string s, int32_t r) -> int64_t {
  return detail::index_rune(s, r);
}

template<typename F>
auto IndexFunc(std::string s, F f) -> int64_t {
  return detail::index_func(s, f, true);
}

inline auto LastIndex(std::string s, std::string substr) -> int64_t {
  return detail::last_index(s, substr);
}

inline auto LastIndexByte(std::string s, uint8_t c) -> int64_t {
  auto i = s.rfind(static_cast<char>(c));
  return i == std::string::npos ? -1 : static_cast<int64_t>(i);
}

template<typename F>
auto LastIndexFunc(std::string s, F f) -> int64_t {
  return detail::last_index_func(s, f, true);
}

inline auto Cut(std::string s, std::string sep) -> std::tuple<std::string, std::string, bool> {
  auto i = s.find(sep);
  if (i == std::string::npos) {
    return {s, "", false};
  }
  return {s.substr(0, i), s.substr(i + sep.size()), true};
}

inline auto CutPrefix(std::string s, std::string prefix) -> std::tuple<std::string, bool> {
  if (!s.starts_with(prefix)) {
    return {s, false};
  }
  return {s.substr(prefix.size()), true};
}

inline auto CutSuffix(std::string s, std::string suffix) -> std::tuple<std::string, bool> {
  if (!s.ends_with(suffix)) {
    return {s, false};
  }
  return {s.substr(0, s.size() - suffix.size()), true};
}

// strings.SplitN: at most n substrings, all of them for n < 0, and nil for n == 0. An empty sep
// splits after each UTF-8 sequence.
inline auto SplitN(std::string s, std::string sep, int64_t n) -> slice<std::string> {
  if (n == 0) {
    return {};
  }
  std::string_view rest = s;
  if (sep.empty()) {
    auto l = detail::rune_count(rest);
    if (n < 0 || n > l) {
      n = l;
    }
    auto a = eo::make_slice<std::string>(n);
    for (int64_t i = 0; i < n - 1; ++i) {
      auto size = detail::decode(rest).second;
      a.data()[i] = std::string{rest.substr(0, size)};
      rest.remove_prefix(size);
    }
    if (n > 0) {
      a.data()[n - 1] = std::string{rest};
    }
    return a;
  }
  if (n < 0) {
    n = detail::count(rest, sep) + 1;
  }
  if (n > static_cast<int64_t>(rest.size()) + 1) {
    n = static_cast<int64_t>(rest.size()) + 1;
  }
  auto a = eo::make_slice<std::string>(n);
  n--;
  int64_t i = 0;
  while (i < n) {
    auto m = rest.find(sep);
    if (m == std::string_view::npos) {
      break;
    }
    a.data()[i] = std::string{rest.substr(0, m)};
    rest.remove_prefix(m + sep.size());
    i++;
  }
  a.data()[i] = std::string{rest};
  return eo::subslice(a, 0, i + 1);
}

inline auto Split(std::string s, std::string sep) -> slice<std::string> {
  return SplitN(std::move(s), std::move(sep), -1);
}

inline auto Join(slice<std::string> elems, std::string sep) -> std::string {
  auto out = std::string{};
  for (int64_t i = 0; i < eo::len(elems); ++i) {
    if (i > 0) {
      out += sep;
    }
    out += elems.data()[i];
  }
  return out;
}

template<typename F>
auto FieldsFunc(std::string s, F f) -> slice<std::string> {
  auto fields = eo::make_slice<std::string>(0);
  auto start = std::size_t{0};
  auto in = false;
  for (std::size_t i = 0; i < s.size();) {
    auto [r, n] = detail::decode(s, i);
    if (static_cast<bool>(f(r))) {
      if (in) {
        fields = eo::append(std::move(fields), {s.substr(start, i - start)});
        in = false;
      }
    } else if (!in) {
      start = i;
      in = true;
    }
    i += n;
  }
  if (in) {
    fields = eo::append(std::move(fields), {s.substr(start)});
  }
  return fields;
}

inline auto Fields(std::string s) -> slice<std::string> {
  return FieldsFunc(std::move(s), detail::is_space);
}

inline auto Repeat(std::string s, int64_t count) -> std::string {
  auto n = detail::repeat_length(s.size(), count, "strings");
  auto out = std::string{};
  out.reserve(n);
  for (int64_t i = 0; i < count; ++i) {
    out += s;
  }
  return out;
}

// strings.Replace: the first n non-overlapping instances of old replaced, all of them for n < 0.
// An empty old matches at the start and after each UTF-8 sequence.
inline auto Replace(std::string s, std::string old, std::string new_, int64_t n) -> std::string {
  if (old == new_ || n == 0) {
    return s;
  }
  auto m = detail::count(s, old);
  if (m == 0) {
    return s;
  }
  if (n < 0 || m < n) {
    n = m;
  }
  auto out = std::string{};
  auto start = std::size_t{0};
  if (!old.empty()) {
    for (int64_t k = 0; k < n; ++k) {
      auto j = s.find(old, start);
      out.append(s, start, j - start);
      out += new_;
      start = j + old.size();
    }
  } else {
    out += new_;
    for (int64_t k = 0; k < n - 1; ++k) {
      auto j = start + detail::decode(s, start).second;
      out.append(s, start, j - start);
      out += new_;
      start = j;
    }
  }
  out.append(s, start);
  return out;
}

inline auto ReplaceAll(std::string s, std::string old, std::string new_) -> std::string {
  return Replace(std::move(s), std::move(old), std::move(new_), -1);
}

template<typename F>
auto Map(F mapping, std::string s) -> std::string {
  return detail::map(mapping, s);
}

inline auto ToUpper(std::string s) -> std::string {
  return detail::to_upper(s);
}

inline auto ToLower(std::string s) -> std::string {
  return detail::to_lower(s);
}

inline auto EqualFold(std::string s, std::string t) -> bool {
  return detail::equal_fold(s, t);
}

template<typename F>
auto TrimFunc(std::string s, F f) -> std::string {
  return std::string{detail::trim_right_func(detail::trim_left_func(s, f), f)};
}

template<typename F>
auto TrimLeftFunc(std::string s, F f) -> std::string {
  return std::string{detail::trim_left_func(s, f)};
}

template<typename F>
auto TrimRightFunc(std::string s, F f) -> std::string {
  return std::string{detail::trim_right_func(s, f)};
}

inline auto TrimSpace(std::string s) -> std::string {
  auto f = detail::is_space;
  return std::string{detail::trim_right_func(detail::trim_left_func(s, f), f)};
}

inline auto Trim(std::string s, std::string cutset) -> std::string {
  if (s.empty() || cutset.empty()) {
    return s;
  }
  return std::string{detail::trim_left(detail::trim_right(s, cutset), cutset)};
}

inline auto TrimLeft(std::string s, std::string cutset) -> std::string {
  if (s.empty() || cutset.empty()) {
    return s;
  }
  return std::string{detail::trim_left(s, cutset)};
}

inline auto TrimRight(std::string s, std::string cutset) -> std::string {
  if (s.empty() || cutset.empty()) {
    return s;
  }
  return std::string{detail::trim_right(s, cutset)};
}

inline auto TrimPrefix(std::string s, std::string prefix) -> std::string {
  return s.starts_with(prefix) ? s.substr(prefix.size()) : s;
}

inline auto TrimSuffix(std::string s, std::string suffix) -> std::string {
  return s.ends_with(suffix) ? s.substr(0, s.size() - suffix.size()) : s;
}

// Go's strings.Builder. Its methods are member functions, as for the other Eo library types: `var
// b strings.Builder` is `strings::Builder b;`, and `b.WriteString(s)` is `b.WriteString(s)`. A
// Builder that has been written to and is then copied panics on the next write through the copy,
// as Go's copyCheck does.
class Builder {
public:
  static constexpr std::string_view _eo_name = "strings.Builder";

  auto String() const -> std::string {
    return eo::string_from_bytes(buf_);
  }

  auto Len() const -> int64_t {
    return eo::len(buf_);
  }

  auto Cap() const -> int64_t {
    return eo::cap(buf_);
  }

  void Reset() {
    addr_ = nullptr;
    buf_ = {};
  }

  // Grows the capacity, if needed, to fit n more bytes, to 2*cap + n as Go does.
  void Grow(int64_t n) {
    copy_check();
    if (n < 0) {
      panic("strings.Builder.Grow: negative count"s);
    }
    if (eo::cap(buf_) - eo::len(buf_) < n) {
      auto grown = eo::make_slice<uint8_t>(eo::len(buf_), 2 * eo::cap(buf_) + n);
      eo::copy(grown, buf_);
      buf_ = std::move(grown);
    }
  }

  auto Write(slice<uint8_t> p) -> std::tuple<int64_t, error> {
    copy_check();
    buf_ = eo::append_slice(std::move(buf_), p);
    return {eo::len(p), nullptr};
  }

  auto WriteByte(uint8_t c) -> error {
    copy_check();
    buf_ = eo::append(std::move(buf_), {c});
    return nullptr;
  }

  auto WriteRune(int32_t r) -> std::tuple<int64_t, error> {
    copy_check();
    auto e = std::string{};
    eo::detail::append_rune(e, r);
    buf_ = eo::append_slice(std::move(buf_), e);
    return {static_cast<int64_t>(e.size()), nullptr};
  }

  auto WriteString(std::string s) -> std::tuple<int64_t, error> {
    copy_check();
    buf_ = eo::append_slice(std::move(buf_), s);
    return {static_cast<int64_t>(s.size()), nullptr};
  }

  // The method set of *Builder, so that a ptr<Builder> is an io.Writer, an io.StringWriter, and a
  // fmt.Stringer, as a *strings.Builder is in Go. A Builder that is not a pointer has no methods.
  static auto _eo_String(ptr<Builder> b) -> std::string {
    return b->String();
  }
  static auto _eo_Len(ptr<Builder> b) -> int64_t {
    return b->Len();
  }
  static auto _eo_Cap(ptr<Builder> b) -> int64_t {
    return b->Cap();
  }
  static void _eo_Reset(ptr<Builder> b) {
    b->Reset();
  }
  static void _eo_Grow(ptr<Builder> b, int64_t n) {
    b->Grow(n);
  }
  static auto _eo_Write(ptr<Builder> b, slice<uint8_t> p) -> std::tuple<int64_t, error> {
    return b->Write(std::move(p));
  }
  static auto _eo_WriteByte(ptr<Builder> b, uint8_t c) -> error {
    return b->WriteByte(c);
  }
  static auto _eo_WriteRune(ptr<Builder> b, int32_t r) -> std::tuple<int64_t, error> {
    return b->WriteRune(r);
  }
  static auto _eo_WriteString(ptr<Builder> b, std::string s) -> std::tuple<int64_t, error> {
    return b->WriteString(std::move(s));
  }

  using _eo_methods = eo::methods<eo::method<&Builder::_eo_Cap, "Cap">,
    eo::method<&Builder::_eo_Grow, "Grow">,
    eo::method<&Builder::_eo_Len, "Len">,
    eo::method<&Builder::_eo_Reset, "Reset">,
    eo::method<&Builder::_eo_String, "String">,
    eo::method<&Builder::_eo_Write, "Write">,
    eo::method<&Builder::_eo_WriteByte, "WriteByte">,
    eo::method<&Builder::_eo_WriteRune, "WriteRune">,
    eo::method<&Builder::_eo_WriteString, "WriteString">>;

private:
  void copy_check() {
    if (addr_ == nullptr) {
      addr_ = this;
    } else if (addr_ != this) {
      panic("strings: illegal use of non-zero Builder copied by value"s);
    }
  }

  const Builder* addr_ = nullptr;
  slice<uint8_t> buf_;
};

// Go's strings.Replacer, from NewReplacer. Replace performs the replacements in the order they
// appear in s, without overlapping matches. At each position the pair that comes first in the
// arguments wins, and an empty old string matches between every two bytes, but not twice in a row.
// It tries the pairs in order at each position, where Go walks a trie: the same result, in time
// proportional to the length of s times the number of pairs.
class Replacer {
public:
  static constexpr std::string_view _eo_name = "strings.Replacer";

  explicit Replacer(slice<std::string> oldnew) {
    for (int64_t i = 0; i + 1 < eo::len(oldnew); i += 2) {
      pairs_.emplace_back(oldnew.data()[i], oldnew.data()[i + 1]);
    }
  }

  auto Replace(std::string s) const -> std::string {
    auto out = std::string{};
    auto last = std::size_t{0};
    auto prev_match_empty = false;
    for (std::size_t i = 0; i <= s.size();) {
      const std::pair<std::string, std::string>* match = nullptr;
      for (const auto& p : pairs_) {
        if (p.first.empty() && prev_match_empty) {
          continue;
        }
        if (std::string_view{s}.substr(i).starts_with(p.first)) {
          match = &p;
          break;
        }
      }
      prev_match_empty = match != nullptr && match->first.empty();
      if (match != nullptr) {
        out.append(s, last, i - last);
        out += match->second;
        i += match->first.size();
        last = i;
        continue;
      }
      i++;
    }
    out.append(s, last);
    return out;
  }

private:
  std::vector<std::pair<std::string, std::string>> pairs_;
};

// strings.NewReplacer(oldnew...), whose variadic parameter is a slice<std::string>. It panics
// with Go's message for an odd number of strings.
inline auto NewReplacer(slice<std::string> oldnew) -> ptr<Replacer> {
  if (eo::len(oldnew) % 2 == 1) {
    panic("strings.NewReplacer: odd argument count"s);
  }
  return New<Replacer>(std::move(oldnew));
}

namespace detail {

  // copy(b, s[i:]), without the copy of s[i:] that eo::substr makes.
  inline auto copy_at(const slice<uint8_t>& b, const std::string& s, int64_t i) -> int64_t {
    auto n = std::min(eo::len(b), eo::len(s) - i);
    std::copy_n(s.data() + i, n, b.data());
    return n;
  }

} // namespace detail

// Go's strings.Reader.
struct Reader {
  static constexpr std::string_view _eo_name = "strings.Reader";

  std::string s{};
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
    auto n = detail::copy_at(b, r->s, r->i);
    r->i += n;
    return {n, nullptr};
  }

  static auto ReadAt(ptr<Reader> r, slice<uint8_t> b, int64_t off) -> std::tuple<int64_t, error> {
    if (off < 0) {
      return {0, errors::New("strings.Reader.ReadAt: negative offset")};
    }
    if (off >= eo::len(r->s)) {
      return {0, io::EOF_};
    }
    auto n = detail::copy_at(b, r->s, off);
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
      return errors::New("strings.Reader.UnreadByte: at beginning of string");
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
    auto [ch, size] = eo::detail::decode_rune(r->s, static_cast<std::size_t>(r->i));
    r->i += static_cast<int64_t>(size);
    return {static_cast<int32_t>(ch), static_cast<int64_t>(size), nullptr};
  }

  static auto UnreadRune(ptr<Reader> r) -> error {
    if (r->i <= 0) {
      return errors::New("strings.Reader.UnreadRune: at beginning of string");
    }
    if (r->prevRune < 0) {
      return errors::New("strings.Reader.UnreadRune: previous operation was not ReadRune");
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
      return {0, errors::New("strings.Reader.Seek: invalid whence")};
    }
    if (abs < 0) {
      return {0, errors::New("strings.Reader.Seek: negative position")};
    }
    r->i = abs;
    return {abs, nullptr};
  }

  static auto WriteTo(ptr<Reader> r, io::Writer w) -> std::tuple<int64_t, error> {
    r->prevRune = -1;
    if (r->i >= eo::len(r->s)) {
      return {0, nullptr};
    }
    auto s = eo::substr(r->s, r->i);
    auto [m, err] = io::WriteString(std::move(w), s);
    if (m > eo::len(s)) {
      panic(std::string{"strings.Reader.WriteTo: invalid WriteString count"});
    }
    r->i += m;
    if (m != eo::len(s) && err == nullptr) {
      err = io::ErrShortWrite;
    }
    return {m, err};
  }

  static void Reset(ptr<Reader> r, std::string s) {
    *r = Reader{std::move(s), 0, -1};
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

// Go's strings.NewReader.
inline auto NewReader(std::string s) -> ptr<Reader> {
  return eo::New<Reader>(Reader{std::move(s), 0, -1});
}

} // namespace eo::strings
