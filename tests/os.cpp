// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#include <eo/os.h>

#include <cstdlib>
#include <stdexcept>
#include <string>

void check(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

void test_expands_named_variables() {
  setenv("EO_EXPAND_NAME", "gopher", 1);
  setenv("EO_EXPAND_HOME", "/usr/gopher", 1);

  check(eo::os::ExpandEnv("$EO_EXPAND_NAME lives in ${EO_EXPAND_HOME}.") == "gopher lives in /usr/gopher.",
    "environment variables were not expanded like Go os.ExpandEnv");
}

void test_undefined_variables_become_empty() {
  unsetenv("EO_EXPAND_MISSING");

  check(eo::os::ExpandEnv("before$EO_EXPAND_MISSING-after") == "before-after",
    "undefined environment variable should expand to an empty string");
}

void test_preserves_non_expansion_text() {
  setenv("EO_EXPAND_WORD", "two words", 1);

  check(eo::os::ExpandEnv("prefix $EO_EXPAND_WORD suffix") == "prefix two words suffix",
    "environment expansion should preserve surrounding whitespace");
  check(eo::os::ExpandEnv("$(printf injected)") == "$(printf injected)",
    "environment expansion should not perform shell command substitution");
  check(eo::os::ExpandEnv("*.cpp") == "*.cpp", "environment expansion should not perform glob expansion");
}

void test_matches_go_invalid_syntax_handling() {
  check(eo::os::ExpandEnv("${}") == "", "empty braced expansion should be consumed");
  check(eo::os::ExpandEnv("${missing") == "missing", "unterminated braced expansion should consume the prefix");
  check(eo::os::ExpandEnv("$-") == "", "undefined special variable should expand to an empty string");
  check(eo::os::ExpandEnv("$") == "$", "trailing dollar should remain unchanged");
}

int main() {
  test_expands_named_variables();
  test_undefined_variables_become_empty();
  test_preserves_non_expansion_text();
  test_matches_go_invalid_syntax_handling();
}
