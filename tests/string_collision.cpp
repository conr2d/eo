// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
// Must not compile: a Go function named like an Eo string or slice helper, called unqualified, is ambiguous with
// the helper instead of silently resolving to either one. The eo.string.collision test builds this
// file and passes only on the ambiguity error.
#include <eo/core.h>

#include <cstdint>
#include <string>

using namespace eo;

auto at(std::string, int64_t) -> int64_t {
  return 42;
}

auto substr(std::string s, int64_t, int64_t) -> std::string {
  return s;
}

auto len(std::string) -> int64_t {
  return 0;
}

auto string_from_rune(int32_t) -> std::string {
  return {};
}

auto cap(slice<int64_t>) -> int64_t {
  return 0;
}

auto subslice(slice<int64_t> s, int64_t) -> slice<int64_t> {
  return s;
}

auto append(slice<int64_t> s, int64_t) -> slice<int64_t> {
  return s;
}

auto append_slice(slice<int64_t> s, slice<int64_t>) -> slice<int64_t> {
  return s;
}

auto copy(slice<int64_t>, slice<int64_t>) -> int64_t {
  return 0;
}

void clear(slice<int64_t>) {}

auto bytes_from_string(std::string) -> slice<uint8_t> {
  return {};
}

auto string_from_bytes(slice<uint8_t>) -> std::string {
  return {};
}

auto runes_from_string(std::string) -> slice<int32_t> {
  return {};
}

auto string_from_runes(slice<int32_t>) -> std::string {
  return {};
}

int main() {
  auto s = "abc"s;
  at(s, 1);
  substr(s, 1, 2);
  len(s);
  string_from_rune(int32_t{65});
  auto v = slice<int64_t>{};
  cap(v);
  subslice(v, 0);
  append(v, 1);
  append_slice(v, v);
  copy(v, v);
  clear(v);
  bytes_from_string(s);
  string_from_bytes(slice<uint8_t>{});
  runes_from_string(s);
  string_from_runes(slice<int32_t>{});
  return 0;
}
