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

// Runs f, which must panic, and returns the panic message.
template<typename F>
auto panic_message(F f) -> std::string {
  try {
    f();
  } catch (const Panic& p) {
    return p.message();
  }
  throw std::logic_error("expected a panic");
}

struct Shape : interface<Shape> {
  using interface::interface;
  static constexpr std::string_view _eo_name = "main.Shape";
  auto Area() const -> int64_t {
    return _eo_call<&Shape::Area>();
  }
  auto Scale(int64_t k) const -> void {
    return _eo_call<&Shape::Scale>(k);
  }
  using _eo_methods = eo::methods<eo::method<&Shape::Area, "Area">, eo::method<&Shape::Scale, "Scale">>;
};

struct Named : interface<Named> {
  using interface::interface;
  static constexpr std::string_view _eo_name = "main.Named";
  auto Name() const -> std::string {
    return _eo_call<&Named::Name>();
  }
  using _eo_methods = eo::methods<eo::method<&Named::Name, "Name">>;
};

struct Areaer : interface<Areaer> {
  using interface::interface;
  static constexpr std::string_view _eo_name = "main.Areaer";
  auto Area() const -> int64_t {
    return _eo_call<&Areaer::Area>();
  }
  using _eo_methods = eo::methods<eo::method<&Areaer::Area, "Area">>;
};

struct Rect {
  static constexpr std::string_view _eo_name = "main.Rect";
  int64_t W{};
  int64_t H{};
  auto operator==(const Rect&) const -> bool = default;
  static auto Area(Rect r) -> int64_t;
  static void Scale(ptr<Rect> r, int64_t k);
  static auto Name(Rect r) -> std::string;
  static auto String(Rect r) -> std::string;
  using _eo_methods = eo::methods<eo::method<&Rect::Area, "Area">,
    eo::method<&Rect::Scale, "Scale">,
    eo::method<&Rect::Name, "Name">,
    eo::method<&Rect::String, "String">>;
};

auto Rect::Area(Rect r) -> int64_t {
  return r.W * r.H;
}

void Rect::Scale(ptr<Rect> r, int64_t k) {
  r->W *= k;
  r->H *= k;
}

auto Rect::Name(Rect) -> std::string {
  return "rect";
}

auto Rect::String(Rect r) -> std::string {
  return fmt::format("R{}x{}", r.W, r.H);
}

struct Bag {
  static constexpr std::string_view _eo_name = "main.Bag";
  slice<int64_t> items{};
};

int main() {
  // A value of *Rect has every method; a Rect lacks the pointer-receiver Scale.
  auto r = New<Rect>(Rect{.W = 2, .H = 3});
  Shape s = r;
  check(s.Area() == 6, "value method through a pointer");
  s.Scale(2);
  check(r->W == 4 && s.Area() == 24, "pointer method shares the target");
  Areaer a = *r;
  *r = Rect{.W = 1, .H = 1};
  check(a.Area() == 24, "the interface holds a copy of a value");

  // nil and a typed nil.
  Shape nil_shape;
  check(nil_shape == nullptr && Shape{} == nullptr, "zero value is nil");
  Shape typed_nil = ptr<Rect>{};
  check(typed_nil != nullptr, "an interface holding a nil pointer is not nil");
  check(panic_message([&] { nil_shape.Area(); }) == "runtime error: invalid memory address or nil pointer dereference",
    "method of a nil interface");
  check(panic_message([&] { typed_nil.Area(); }) == "runtime error: invalid memory address or nil pointer dereference",
    "value method through a nil pointer");

  // Equality.
  any x = Rect{.W = 1, .H = 2};
  any y = Rect{.W = 1, .H = 2};
  check(x == y && x != any(Rect{.W = 1, .H = 3}), "equal dynamic values");
  check(any(int64_t{1}) != any(int32_t{1}), "different dynamic types");
  check(any(r) == any(r) && any(r) != any(New<Rect>(*r)), "pointers compare by address");
  check(panic_message([] { (void)(any(Bag{}) == any(Bag{})); }) ==
      "runtime error: comparing uncomparable type main.Bag",
    "uncomparable");
  check(any(Bag{}) != any(int64_t{1}), "different types do not panic");

  // Assertions.
  check(eo::type_assert<Rect>(x).W == 1, "assert concrete");
  auto [p, ok] = eo::type_assert_ok<ptr<Rect>>(x);
  check(p == nullptr && !ok, "comma-ok failure gives the zero value");
  check(panic_message([&] { eo::type_assert<std::string>(any(int64_t{1})); }) ==
      "interface conversion: interface {} is int, not string",
    "assert wrong type");
  check(panic_message([&] { eo::type_assert<int64_t>(any{}); }) == "interface conversion: interface {} is nil, not int",
    "assert nil");
  check(panic_message([&] { eo::type_assert<ptr<Rect>>(Areaer(Rect{})); }) ==
      "interface conversion: main.Areaer is main.Rect, not *main.Rect",
    "assert from a named interface");
  check(panic_message([&] { eo::type_assert<Shape>(any(Rect{})); }) ==
      "interface conversion: main.Rect is not main.Shape: missing method Scale",
    "assert to an interface");
  check(panic_message([&] { eo::type_assert<Named>(Shape{}); }) ==
      "interface conversion: interface is nil, not main.Named",
    "assert nil to an interface");
  auto named = eo::type_assert<Named>(s);
  check(named.Name() == "rect", "interface to interface");
  auto [shape, is_shape] = eo::type_assert_ok<Shape>(any(r));
  check(is_shape && shape.Area() == 1, "comma-ok to an interface");
  check(eo::holds<Rect>(x) && !eo::holds<ptr<Rect>>(x) && eo::holds<Named>(x) && !eo::holds<Shape>(x), "holds");

  // A static conversion between interfaces.
  Areaer from_shape = s;
  any from_areaer = from_shape;
  check(from_shape.Area() == 1 && eo::type_assert<ptr<Rect>>(from_areaer) == r, "interface to a smaller interface");

  // Printing calls String, and prints a nil interface as <nil>.
  check(fmt::format("{}", eo::detail::go_print(x, true)) == "R1x2", "String method");
  check(fmt::format("{}", eo::detail::go_print(any{}, true)) == "<nil>", "nil interface");
  auto list = slice<any>::of({any(int64_t{1}), any{}, any(std::string{"s"}), any(Rect{.W = 3, .H = 4})});
  check(fmt::format("{}", eo::detail::go_print(list, true)) == "[1 <nil> s R3x4]", "slice of any");
  check(eo::detail::type_name<slice<ptr<Rect>>>() == "[]*main.Rect", "type names");

  // A pointer to an interface has no methods, and prints as an address.
  auto shape_pointer = New<Shape>(s);
  any holds_pointer = shape_pointer;
  check(eo::type_assert<ptr<Shape>>(holds_pointer) == shape_pointer && !eo::holds<Named>(holds_pointer),
    "pointer to an interface");
  check(eo::detail::type_name<ptr<Shape>>() == "*main.Shape", "pointer to an interface name");
  return 0;
}
