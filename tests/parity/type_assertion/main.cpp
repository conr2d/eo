#include <eo/fmt.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>

using namespace eo;

struct Shape;
struct Namer;
struct Rect;
struct Circle;

struct Shape : interface<Shape> {
  using interface::interface;
  static constexpr std::string_view _eo_name = "main.Shape";

  auto Area() const -> int64_t;

  using _eo_methods = eo::methods<eo::method<&Shape::Area, "Area">>;
};

struct Namer : interface<Namer> {
  using interface::interface;
  static constexpr std::string_view _eo_name = "main.Namer";

  auto Name() const -> std::string;

  using _eo_methods = eo::methods<eo::method<&Namer::Name, "Name">>;
};

struct Rect {
  static constexpr std::string_view _eo_name = "main.Rect";

  int64_t W{}, H{};

  auto operator==(const Rect&) const -> bool = default;

  static auto Area(Rect r) -> int64_t;

  using _eo_methods = eo::methods<eo::method<&Rect::Area, "Area">>;
};

struct Circle {
  static constexpr std::string_view _eo_name = "main.Circle";

  int64_t R{};

  auto operator==(const Circle&) const -> bool = default;

  static auto Area(ptr<Circle> c) -> int64_t;
  static auto Name(ptr<Circle> c) -> std::string;

  using _eo_methods = eo::methods<eo::method<&Circle::Area, "Area">, eo::method<&Circle::Name, "Name">>;
};

auto Shape::Area() const -> int64_t {
  return _eo_call<&Shape::Area>();
}

auto Namer::Name() const -> std::string {
  return _eo_call<&Namer::Name>();
}

auto Rect::Area(Rect r) -> int64_t {
  return r.W * r.H;
}

auto Circle::Area(ptr<Circle> c) -> int64_t {
  return 3 * c->R * c->R;
}

auto Circle::Name(ptr<Circle> c) -> std::string {
  return "circle"s;
}

void describe(any v) {
  if (auto _eo_type_switch = v; _eo_type_switch == nullptr) {
    fmt::Println("nil"s);
  } else if (eo::holds<int64_t>(_eo_type_switch) || eo::holds<std::string>(_eo_type_switch)) {
    auto x = _eo_type_switch;
    fmt::Println("int or string"s, x);
  } else if (eo::holds<Rect>(_eo_type_switch)) {
    auto x = eo::type_assert<Rect>(_eo_type_switch);
    fmt::Println("rect"s, x.W, x.H);
  } else if (eo::holds<Namer>(_eo_type_switch)) {
    auto x = eo::type_assert<Namer>(_eo_type_switch);
    fmt::Println("namer"s, x.Name());
  } else if (eo::holds<Shape>(_eo_type_switch)) {
    auto x = eo::type_assert<Shape>(_eo_type_switch);
    fmt::Println("shape"s, x.Area());
  } else {
    fmt::Println("other"s);
  }
}

void attempt(std::string name, fn<void()> f) {
  eo_defer_scope;
  try {
    eo_defer([&] { fmt::Println(name, "recovered:"s, recover()); });
    f();
    eo_defer_run;
  } catch (...) {
    eo_defer_panic;
  }
}

func<> eo_main() {
  Shape s = Rect{.W = 2, .H = 3};
  auto r = eo::type_assert<Rect>(s);
  fmt::Println(r.W, r.H);

  auto [c, ok] = eo::type_assert_ok<ptr<Circle>>(s);
  fmt::Println(c == nullptr, ok);

  s = New<Circle>(Circle{.R = 1});
  std::tie(c, ok) = eo::type_assert_ok<ptr<Circle>>(s);
  fmt::Println(c->R, ok);

  Namer n{};
  std::tie(n, ok) = eo::type_assert_ok<Namer>(s);
  fmt::Println(n.Name(), ok);

  ok = eo::type_assert_ok<Namer>(any(Rect{})).second;
  fmt::Println(ok);

  any a = int64_t{7};
  auto i = eo::type_assert<int64_t>(a);
  fmt::Println(i + 1);

  attempt("wrong type"s, [=]() mutable { fmt::Println(eo::type_assert<std::string>(a)); });
  attempt("nil"s, [=]() mutable {
    any e{};
    fmt::Println(eo::type_assert<int64_t>(e));
  });
  attempt("named interface"s, [=]() mutable {
    Shape e = Rect{};
    fmt::Println(eo::type_assert<ptr<Circle>>(e));
  });
  attempt("missing method"s, [=]() mutable { fmt::Println(eo::type_assert<Shape>(a)); });
  attempt("nil to interface"s, [=]() mutable {
    Shape e{};
    fmt::Println(eo::type_assert<Namer>(e));
  });

  describe(nullptr);
  describe(int64_t{1});
  describe("s"s);
  describe(Rect{.W = 1, .H = 1});
  describe(New<Circle>(Circle{.R = 2}));
  describe(double{2.5});
  co_return;
}
