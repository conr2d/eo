// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#include <eo/fmt.h>

#include <cstdio>
#include <map>
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
  fmt::Fprintln(file.get(), "hello", "world", 42, true);

  check(read_file(file.get()) == "hello world 42 true\n", "Fprintln should separate all operands with spaces");
}

void test_does_not_interpret_format_strings() {
  auto file = make_file();
  fmt::Fprintln(file.get(), "value {}", 42);

  check(read_file(file.get()) == "value {} 42\n", "Fprintln should treat braces as ordinary text");
}

void test_empty_call_writes_newline() {
  auto file = make_file();
  fmt::Fprintln(file.get());

  check(read_file(file.get()) == "\n", "Fprintln with no operands should write only a newline");
}

void test_prints_map_in_key_order() {
  auto file = make_file();
  fmt::Fprintln(file.get(), std::map<std::string, int>{{"c", 3}, {"a", 1}, {"b", 2}});

  check(read_file(file.get()) == "map[a:1 b:2 c:3]\n", "Fprintln should print a map like Go, in key order");
}

void test_prints_nested_and_empty_maps() {
  auto file = make_file();
  auto nested = std::map<std::string, std::map<std::string, int>>{{"x", {}}, {"y", {{"q", 2}, {"p", 1}}}};
  fmt::Fprintln(file.get(), nested, std::map<int, int>{});

  check(read_file(file.get()) == "map[x:map[] y:map[p:1 q:2]] map[]\n", "Fprintln should print nested and empty maps");
}

void test_prints_map_of_strings() {
  auto file = make_file();
  fmt::Fprintln(file.get(), std::map<std::string, std::string>{{"k1", ""}, {"k2", "hello world"}});

  check(read_file(file.get()) == "map[k1: k2:hello world]\n", "Fprintln should print map strings without quotes");
}

void test_prints_error_message() {
  auto file = make_file();
  fmt::Fprintln(file.get(), eo::Error{"x"}, eo::Error::format("bad {}", 42));

  check(read_file(file.get()) == "x bad 42\n", "Fprintln should print an error's message");
}

int main() {
  test_adds_spaces_between_operands();
  test_does_not_interpret_format_strings();
  test_empty_call_writes_newline();
  test_prints_map_in_key_order();
  test_prints_nested_and_empty_maps();
  test_prints_map_of_strings();
  test_prints_error_message();
}
