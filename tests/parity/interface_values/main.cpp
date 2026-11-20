#include <eo/fmt.h>

#include <cstdint>
#include <string>
#include <string_view>

using namespace eo;

struct Shape;
struct Areaer;
struct Rect;
struct Labeler;
struct Tag;
struct Card;
struct Bag;
struct Resizer;

struct Shape : interface<Shape> {
  using interface::interface;
  static constexpr std::string_view _eo_name = "main.Shape";

  auto Area() const -> int64_t;
  void Scale(int64_t k) const;

  using _eo_methods = eo::methods<eo::method<&Shape::Area, "Area">, eo::method<&Shape::Scale, "Scale">>;
};

struct Areaer : interface<Areaer> {
  using interface::interface;
  static constexpr std::string_view _eo_name = "main.Areaer";

  auto Area() const -> int64_t;

  using _eo_methods = eo::methods<eo::method<&Areaer::Area, "Area">>;
};

struct Rect {
  static constexpr std::string_view _eo_name = "main.Rect";

  int64_t W{}, H{};

  auto operator==(const Rect&) const -> bool = default;

  static auto Area(Rect r) -> int64_t;
  static void Scale(ptr<Rect> r, int64_t k);

  using _eo_methods = eo::methods<eo::method<&Rect::Area, "Area">, eo::method<&Rect::Scale, "Scale">>;
};

struct Labeler : interface<Labeler> {
  using interface::interface;
  static constexpr std::string_view _eo_name = "main.Labeler";

  auto Label() const -> std::string;

  using _eo_methods = eo::methods<eo::method<&Labeler::Label, "Label">>;
};

struct Tag {
  static constexpr std::string_view _eo_name = "main.Tag";

  std::string Name{};

  auto operator==(const Tag&) const -> bool = default;

  static auto Label(Tag t) -> std::string;

  using _eo_methods = eo::methods<eo::method<&Tag::Label, "Label">>;
};

// Card gets Area and Scale from *Rect and Label from its Labeler field.
struct Card {
  static constexpr std::string_view _eo_name = "main.Card";

  ptr<::Rect> Rect{};
  ::Labeler Labeler{};

  auto operator==(const Card&) const -> bool = default;

  static auto Area(Card _eo_receiver) -> int64_t;
  static auto Label(Card _eo_receiver) -> std::string;
  static void Scale(Card _eo_receiver, int64_t k);

  using _eo_methods =
    eo::methods<eo::method<&Card::Area, "Area">, eo::method<&Card::Label, "Label">, eo::method<&Card::Scale, "Scale">>;
};

struct Bag {
  static constexpr std::string_view _eo_name = "main.Bag";

  slice<int64_t> Items{};

  auto operator==(const Bag&) const -> bool = default;
};

// Resizer embeds Areaer, so it has the same methods as Shape.
struct Resizer : interface<Resizer> {
  using interface::interface;
  static constexpr std::string_view _eo_name = "main.Resizer";

  auto Area() const -> int64_t;
  void Scale(int64_t k) const;

  using _eo_methods = eo::methods<eo::method<&Resizer::Area, "Area">, eo::method<&Resizer::Scale, "Scale">>;
};

auto Shape::Area() const -> int64_t {
  return _eo_call<&Shape::Area>();
}

void Shape::Scale(int64_t k) const {
  return _eo_call<&Shape::Scale>(k);
}

auto Areaer::Area() const -> int64_t {
  return _eo_call<&Areaer::Area>();
}

auto Labeler::Label() const -> std::string {
  return _eo_call<&Labeler::Label>();
}

auto Card::Area(Card _eo_receiver) -> int64_t {
  return ::Rect::Area(*_eo_receiver.Rect);
}

auto Card::Label(Card _eo_receiver) -> std::string {
  return _eo_receiver.Labeler.Label();
}

void Card::Scale(Card _eo_receiver, int64_t k) {
  return ::Rect::Scale(_eo_receiver.Rect, k);
}

auto Resizer::Area() const -> int64_t {
  return _eo_call<&Resizer::Area>();
}

void Resizer::Scale(int64_t k) const {
  return _eo_call<&Resizer::Scale>(k);
}

auto Rect::Area(Rect r) -> int64_t {
  return r.W * r.H;
}

void Rect::Scale(ptr<Rect> r, int64_t k) {
  r->W *= k;
  r->H *= k;
}

auto Tag::Label(Tag t) -> std::string {
  return "#"s + t.Name;
}

auto total(slice<Areaer> shapes) -> int64_t {
  auto sum = int64_t{0};
  for (auto [_eo_range_key, s] : eo::range(shapes)) {
    sum += s.Area();
  }
  return sum;
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
  auto r = New<Rect>(Rect{.W = 2, .H = 3});
  Shape s = r;
  s.Scale(2);
  fmt::Println(s.Area(), r->W, r->H);

  // An interface holds a copy of a value.
  Areaer a = *r;
  r->W = 1;
  auto _eo_call_0 = a.Area();
  auto _eo_call_1 = Rect::Area(*r);
  fmt::Println(_eo_call_0, _eo_call_1);

  // Areaer has a subset of Shape's methods.
  a = s;
  auto _eo_call_2 = a.Area();
  auto _eo_call_3 = total(slice<Areaer>::of({a, Rect{.W = 5, .H = 5}, s}));
  fmt::Println(_eo_call_2, _eo_call_3);

  Shape none{};
  ptr<Rect> nilRect{};
  Shape typed = nilRect;
  fmt::Println(none == nullptr, typed == nullptr, typed != nullptr);
  attempt("nil interface"s, [=]() mutable { none.Area(); });
  attempt("nil pointer"s, [=]() mutable { typed.Area(); });

  any x = Rect{.W = 1, .H = 2};
  any y = Rect{.W = 1, .H = 2};
  fmt::Println(x == y, x == any(Rect{.W = 2, .H = 1}), x == any(r), any(r) == any(r));
  fmt::Println(any(int64_t{1}) == any(int64_t{1}),
    any(int64_t{1}) == any(int32_t{1}),
    any("a"s) == any("a"s),
    x == nullptr);
  attempt("uncomparable"s, [=]() mutable {
    any p = Bag{};
    any q = Bag{};
    fmt::Println(p == q);
  });
  fmt::Println(any(Bag{}) == any(int64_t{1}));

  auto c = Card{.Rect = New<Rect>(Rect{.W = 4, .H = 1}), .Labeler = Tag{.Name = "go"s}};
  s = c;
  s.Scale(3);
  Labeler l = c;
  auto _eo_call_4 = s.Area();
  auto _eo_call_5 = l.Label();
  auto _eo_call_6 = c.Labeler.Label();
  fmt::Println(_eo_call_4, c.Rect->W, _eo_call_5, _eo_call_6);

  Resizer rz = r;
  rz.Scale(2);
  s = rz;
  auto _eo_call_7 = rz.Area();
  auto _eo_call_8 = s.Area();
  fmt::Println(_eo_call_7, _eo_call_8, r->W, r->H);
  co_return;
}
