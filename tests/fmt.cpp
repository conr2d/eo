// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#include <eo/fmt.h>

#include <cstdint>
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

struct Inner {
  int64_t A{}, B{};

  auto operator==(const Inner&) const -> bool = default;
};

struct Outer {
  std::string Name{};
  Inner In{};
  eo::slice<int64_t> Items{};
  eo::ptr<Outer> Next{};
  bool Valid{};
};

void test_prints_struct_fields() {
  auto file = make_file();
  fmt::Fprintln(file.get(),
    Inner{1, 2},
    Outer{},
    Outer{.Name = "a", .In = Inner{3, 4}, .Items = eo::slice<int64_t>::of({5})});

  check(read_file(file.get()) == "{1 2} { {0 0} [] <nil> false} {a {3 4} [5] <nil> false}\n",
    "Fprintln should print a struct's fields in braces, like Go's %v");
}

void test_prints_slice_of_structs_and_empty_struct() {
  auto file = make_file();
  fmt::Fprintln(file.get(), eo::slice<Inner>::of({Inner{1, 2}, Inner{}}), eo::empty_struct{});

  check(read_file(file.get()) == "[{1 2} {0 0}] {}\n",
    "Fprintln should print structs inside a slice, and struct{} as {}");
}

void test_prints_pointer_operand_as_its_target() {
  auto file = make_file();
  auto p = eo::New<Inner>(Inner{7, 8});
  fmt::Fprintln(file.get(), p, eo::ptr<Inner>{}, eo::New<eo::slice<int64_t>>(eo::slice<int64_t>::of({1, 2})));

  check(read_file(file.get()) == "&{7 8} <nil> &[1 2]\n",
    "Fprintln should print a pointer operand to a struct or slice as & and its target, and nil as <nil>");
}

void test_prints_nested_pointer_as_address() {
  auto file = make_file();
  auto next = eo::New<Outer>();
  fmt::Fprintln(file.get(), Outer{.Next = next}, eo::New<int64_t>(1) == nullptr);

  auto out = read_file(file.get());
  check(out.starts_with("{ {0 0} [] 0x") && out.ends_with(" false} false\n"),
    "Fprintln should print a pointer inside a struct as an address");
}

int main() {
  test_adds_spaces_between_operands();
  test_does_not_interpret_format_strings();
  test_empty_call_writes_newline();
  test_prints_map_in_key_order();
  test_prints_nested_and_empty_maps();
  test_prints_map_of_strings();
  test_prints_error_message();
  test_prints_struct_fields();
  test_prints_slice_of_structs_and_empty_struct();
  test_prints_pointer_operand_as_its_target();
  test_prints_nested_pointer_as_address();
}
