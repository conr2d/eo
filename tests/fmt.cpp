// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#include <eo/fmt.h>

#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>

void check(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

using File = std::unique_ptr<std::FILE, decltype(&std::fclose)>;

File make_file() {
  auto* file = std::tmpfile();
  if (file == nullptr) {
    throw std::runtime_error("failed to create temporary file");
  }
  return File(file, &std::fclose);
}

std::string read_file(std::FILE* file) {
  std::fflush(file);
  std::rewind(file);

  std::string result;
  char buffer[128];
  while (auto size = std::fread(buffer, 1, sizeof(buffer), file)) {
    result.append(buffer, size);
  }
  return result;
}

void test_adds_spaces_between_operands() {
  auto file = make_file();
  fmt::fprintln(file.get(), "hello", "world", 42, true);

  check(read_file(file.get()) == "hello world 42 true\n", "fprintln should separate all operands with spaces");
}

void test_does_not_interpret_format_strings() {
  auto file = make_file();
  fmt::fprintln(file.get(), "value {}", 42);

  check(read_file(file.get()) == "value {} 42\n", "fprintln should treat braces as ordinary text");
}

void test_empty_call_writes_newline() {
  auto file = make_file();
  fmt::fprintln(file.get());

  check(read_file(file.get()) == "\n", "fprintln with no operands should write only a newline");
}

int main() {
  test_adds_spaces_between_operands();
  test_does_not_interpret_format_strings();
  test_empty_call_writes_newline();
}
