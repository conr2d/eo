#include <eo/fmt.h>

#include <cstdint>
#include <string>

using namespace eo;

struct Point;
struct Labeled;

struct Point {
  int64_t X{}, Y{};

  auto operator==(const Point&) const -> bool = default;
};

struct Labeled {
  std::string Name{};
  Point At{};
  slice<std::string> Tags{};
  ptr<Labeled> Next{};
  bool Valid{};

  auto operator==(const Labeled&) const -> bool = default;
};

auto trace(std::string name, int64_t v) -> int64_t {
  fmt::Println("eval"s, name);
  return v;
}

void show(int64_t n, Point p) {
  fmt::Println("show"s, n, p);
}

func<> eo_main() {
  Labeled zero;
  fmt::Println(zero);
  fmt::Println(zero.At == Point{}, zero.Name == ""s);

  auto p = Point{1, 2};
  auto q = Point{.Y = 5};
  fmt::Println(p, q, Point{});

  auto l = Labeled{.Name = "a"s, .At = Point{3, 4}, .Tags = slice<std::string>::of({"x"s, "y"s})};
  fmt::Println(l);
  fmt::Println(l.At.X, eo::len(l.Tags), l.Next == nullptr, l.Valid);

  auto ps = slice<Point>::of({Point{1, 2}, Point{.Y = 3}, Point{}});
  fmt::Println(ps, eo::len(ps));

  auto pp = slice<ptr<Point>>::of({New<Point>(Point{5, 6}), nullptr});
  fmt::Println(*eo::at(pp, 0), eo::at(pp, 1) == nullptr);

  auto head = New<Labeled>(Labeled{.Name = "head"s, .Next = New<Labeled>(Labeled{.Name = "tail"s})});
  fmt::Println(head->Name, head->Next->Name, head->Next->Next == nullptr);
  fmt::Println(New<Point>(Point{7, 8}));

  auto _eo_call_0 = trace("y"s, 10);
  auto _eo_call_1 = trace("x"s, 20);
  auto r = Point{.X = _eo_call_1, .Y = _eo_call_0};
  fmt::Println(r);

  auto _eo_call_2 = trace("n"s, 1);
  auto _eo_call_3 = trace("y"s, 2);
  auto _eo_call_4 = trace("x"s, 3);
  show(_eo_call_2, Point{.X = _eo_call_4, .Y = _eo_call_3});

  auto in = Point{.X = trace("x"s, 4), .Y = trace("y"s, 5)};
  fmt::Println(in);

  auto e = eo::empty_struct{};
  fmt::Println(e, e == eo::empty_struct{});
  co_return;
}
