// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#include <eo/fmt.h>

#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>

using namespace eo;

void check(bool condition, const char* message) {
  if (!condition) {
    throw std::logic_error(message);
  }
}

// A user error type with a pointer receiver, as Go's *E.
struct E {
  static constexpr std::string_view _eo_name = "main.E";

  int64_t c{};

  auto operator==(const E&) const -> bool = default;

  static auto Error(ptr<E> e) -> std::string {
    return "E" + std::to_string(e->c);
  }

  using _eo_methods = eo::methods<eo::method<&E::Error, "Error">>;
};

inline const auto sentinel = errors::New("sentinel");

// An error with an Is method that matches the sentinel.
struct errMatch {
  static constexpr std::string_view _eo_name = "main.errMatch";

  auto operator==(const errMatch&) const -> bool = default;

  static auto Error(errMatch) -> std::string {
    return "match";
  }
  static auto Is(errMatch, error target) -> bool {
    return target == sentinel;
  }

  using _eo_methods = eo::methods<eo::method<&errMatch::Error, "Error">, eo::method<&errMatch::Is, "Is">>;
};

void test_new() {
  auto a = errors::New("x");
  auto b = errors::New("x");
  check(a != b && a == a, "errors.New returns a distinct pointer each call");
  check(a.Error() == "x", "Error");
  check(fmt::Sprintf("%T|%v|%s|%q", a, a, a, a) == "*errors.errorString|x|x|\"x\"", "errors.New prints");
}

void test_wrapping() {
  auto e1 = errors::New("x");
  auto e2 = errors::New("x");
  auto w = fmt::Errorf("wrap %w and %w", e1, e2);
  check(fmt::Sprintf("%T %v", w, w) == "*fmt.wrapErrors wrap x and x", "several %w");
  check(errors::Is(w, e1) && errors::Is(w, e2), "Is through several %w");
  check(errors::Unwrap(w) == nullptr, "Unwrap does not see Unwrap() []error");

  error inner = New<E>(E{.c = 3});
  auto w1 = fmt::Errorf("one: %w", inner);
  check(fmt::Sprintf("%T", w1) == "*fmt.wrapError" && w1.Error() == "one: E3", "one %w");
  check(errors::Unwrap(w1) == inner, "Unwrap");
  auto target = ptr<E>{};
  check(errors::As(w1, ref(target)) && target->c == 3, "As to a pointer type");
  auto as_error = error{};
  check(errors::As(w1, ref(as_error)) && as_error == w1, "As to an interface stops at the first error");
  check(!errors::As(errors::New("y"), ref(target)), "As without a match");
  check(!errors::Is(w1, errors::New("one: E3")), "Is compares pointers");
  check(!errors::Is(nullptr, e1) && errors::Is(nullptr, nullptr) && !errors::Is(e1, nullptr), "Is with nil");

  check(fmt::Sprintf("%T", fmt::Errorf("no")) == "*errors.errorString", "Errorf without %w");
  auto bad = fmt::Errorf("bad %w", int64_t{3});
  check(bad.Error() == "bad %!w(int=3)" && fmt::Sprintf("%T", bad) == "*fmt.wrapError" &&
      errors::Unwrap(bad) == nullptr,
    "%w of a non-error");
  auto nil = fmt::Errorf("nil %w", error{});
  check(nil.Error() == "nil %!w(<nil>)" && errors::Unwrap(nil) == nullptr, "%w of nil");

  auto deep = fmt::Errorf("a: %w", fmt::Errorf("b: %w", errMatch{}));
  check(errors::Is(deep, errMatch{}), "Is through two levels, comparable target");
  check(errors::Is(deep, sentinel) && !errors::Is(deep, errors::New("sentinel")), "Is method");
}

void test_join() {
  auto e1 = errors::New("x");
  error w1 = fmt::Errorf("one: %w", error{New<E>(E{.c = 3})});
  check(errors::Join(slice<error>::of({error{}, error{}})) == nullptr, "Join of nils");
  check(errors::Join(nullptr) == nullptr, "Join of nothing");
  auto j = errors::Join(slice<error>::of({e1, error{}, w1}));
  check(fmt::Sprintf("%T|%q", j, j.Error()) == "*errors.joinError|\"x\\none: E3\"", "Join prints");
  auto target = ptr<E>{};
  check(errors::Is(j, e1) && errors::As(j, ref(target)) && target->c == 3, "Is and As through Join");
}

void test_as_nil_target() {
  auto message = std::string{};
  try {
    errors::As(errors::New("x"), ptr<ptr<E>>{});
  } catch (const Panic& p) {
    message = p.message();
  }
  check(message == "errors: target must be a non-nil pointer", "As with a nil target panics");
}

struct T {
  static constexpr std::string_view _eo_name = "main.T";

  int64_t A{};
  std::string S;

  auto operator==(const T&) const -> bool = default;
};

struct St {
  static constexpr std::string_view _eo_name = "main.St";

  int64_t n{};

  auto operator==(const St&) const -> bool = default;

  static auto String(St) -> std::string {
    return "St!";
  }

  using _eo_methods = eo::methods<eo::method<&St::String, "String">>;
};

void test_verbs() {
  // The expected strings are Go 1.26's output for the same calls.
  check(fmt::Sprintf("%d %s %v %x %X %q %t %T %%",
          int64_t{42},
          std::string{"s"},
          T{1, "a"},
          int64_t{255},
          std::string{"hi"},
          std::string{"q\"\n\x01\u00e9"},
          true,
          T{}) == "42 s {1 a} ff 6869 \"q\\\"\\n\\x01\u00e9\" true main.T %",
    "basic verbs");
  auto ints = slice<int64_t>::of({1, 2});
  auto strings = slice<std::string>::of({"a", "b"});
  auto bytes = slice<uint8_t>::of({'h', 'i'});
  check(fmt::Sprintf("%d|%s|%x|%q|%v", ints, strings, bytes, bytes, bytes) == "[1 2]|[a b]|6869|\"hi\"|[104 105]",
    "slices");
  check(fmt::Sprintf("%d|%s|%x|%v|%s|%d", T{1, "a"}, T{1, "a"}, T{255, "a"}, New<T>(T{1, "a"}), St{}, St{}) ==
      "{1 %!d(string=a)}|{%!s(int=1) a}|{ff 61}|&{1 a}|St!|{0}",
    "structs and methods");
  check(fmt::Sprintf("%d|%s|%z|%d", std::string{"s"}, int64_t{3}, int64_t{1}) ==
      "%!d(string=s)|%!s(int=3)|%!z(int=1)|%!d(MISSING)",
    "bad verbs");
  check(fmt::Sprintf("%d", int64_t{1}, std::string{"x"}, 2.5) == "1%!(EXTRA string=x, float64=2.5)", "extra operands");
  check(fmt::Sprintf("%v|%d|%s|%T|%w", any{}, any{}, error{}, any{}, errors::New("x")) ==
      "<nil>|%!d(<nil>)|%!s(<nil>)|<nil>|%!w(*errors.errorString=&{x})",
    "nil operands and %w outside Errorf");
  check(fmt::Sprintf("%x|%X|%x|%q|%q|%c",
          int64_t{-255},
          std::string{"hi"},
          std::string{},
          int32_t{'a'},
          int32_t{0x1F600},
          int32_t{0xe9}) == "-ff|6869||'a'|'\U0001F600'|\u00e9",
    "integers as runes and hex");
  check(fmt::Sprintf("%s|%v|%d", errors::New("x"), New<E>(E{.c = 2}), New<E>(E{.c = 2})) == "x|E2|&{2}",
    "errors with verbs");
  check(fmt::Sprintf("%t|%v|%s", int64_t{1}, 1.5, 2.5) == "%!t(int=1)|1.5|%!s(float64=2.5)", "bool and float verbs");
  check(fmt::Sprintf("trail%") == "trail%!(NOVERB)", "no verb");
  check(fmt::Sprintf("%v %v", int8_t{-3}, uint8_t{200}) == "-3 200", "small integers");
  check(fmt::Sprintf("%x %d", uint64_t{1} << 63, INT64_MIN) == "8000000000000000 -9223372036854775808", "64-bit");
  check(fmt::Sprintf("%s %v %q",
          std::string{"\xff"},
          std::string{"\xff"},
          std::string{"\xff"
                      "a\u00ad\t"}) == "\xff \xff \"\\xffa\\u00ad\\t\"",
    "invalid UTF-8 and unprintable runes");
  check(fmt::Sprintf("%U|%U|%x", int8_t{-3}, uint8_t{253}, int8_t{-3}) == "U+FFFFFFFFFFFFFFFD|U+00FD|-3",
    "%U sign-extends");
  check(fmt::Sprint("a", "b", int64_t{1}, int64_t{2}, "c") == "ab1 2c", "Sprint spaces");
  check(fmt::Sprintln("a", int64_t{1}) == "a 1\n", "Sprintln");
}

void test_recovered_values() {
  auto recovered = [](auto f) {
    auto r = any{};
    [&] {
      eo_defer_scope;
      try {
        eo_defer([&] { r = recover(); });
        f();
        eo_defer_run;
      } catch (...) {
        eo_defer_panic;
      }
    }();
    auto [e, is_error] = eo::type_assert_ok<error>(r);
    auto [re, is_runtime] = eo::type_assert_ok<runtime::Error_>(r);
    return fmt::Sprintf("%T|%v|err=%t|rt=%t", r, r, is_error, is_runtime);
  };
  auto p = ptr<T>{};
  auto s = slice<int64_t>{};
  any i = std::string{"x"};
  check(recovered([&] { (void)p->A; }) ==
      "runtime.errorString|runtime error: invalid memory address or nil pointer dereference|err=true|rt=true",
    "nil dereference");
  check(recovered([&] { (void)eo::at(s, 3); }) ==
      "runtime.boundsError|runtime error: index out of range [3] with length 0|err=true|rt=true",
    "index");
  check(recovered([&] { (void)eo::type_assert<int64_t>(i); }) ==
      "*runtime.TypeAssertionError|interface conversion: interface {} is string, not int|err=true|rt=true",
    "type assertion");
  check(recovered([&] { chan<int64_t>{}.close(); }) == "runtime.plainError|close of nil channel|err=true|rt=true",
    "close of nil channel");
  check(recovered([&] { (void)eo::div(int64_t{1}, int64_t{0}); }) ==
      "runtime.errorString|runtime error: integer divide by zero|err=true|rt=true",
    "divide by zero");
  check(recovered([&] { panic(nullptr); }) == "*runtime.PanicNilError|panic called with nil argument|err=true|rt=true",
    "panic(nil)");
  check(recovered([&] { panic("str"); }) == "string|str|err=false|rt=false", "string");
  check(recovered([&] { panic(errors::New("boom")); }) == "*errors.errorString|boom|err=true|rt=false", "error");
  check(recovered([&] { panic(T{1, "a"}); }) == "main.T|{1 a}|err=false|rt=false", "struct");
  check(recovered([&] { panic(error{}); }) == "*runtime.PanicNilError|panic called with nil argument|err=true|rt=true",
    "panic of a nil error");
  check(recovered([&] { throw std::runtime_error("cpp"); }) == "eo.cppException|cpp|err=true|rt=false",
    "a C++ exception");
}

auto main() -> int {
  test_new();
  test_wrapping();
  test_join();
  test_as_nil_target();
  test_verbs();
  test_recovered_values();
  return 0;
}
