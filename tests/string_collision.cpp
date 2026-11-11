// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
// Must not compile: a Go function named like an Eo string helper, called unqualified, is ambiguous with
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

int main() {
  auto s = "abc"s;
  at(s, 1);
  substr(s, 1, 2);
  return 0;
}
