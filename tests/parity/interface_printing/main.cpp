#include <eo/fmt.h>

#include <cstdint>
#include <string>
#include <string_view>

using namespace eo;

struct Point;
struct Temp;
struct Fault;
struct Boom;
struct Pair;

struct Point {
  static constexpr std::string_view _eo_name = "main.Point";

  int64_t X{};
  int64_t Y{};

  auto operator==(const Point&) const -> bool = default;

  static auto String(Point p) -> std::string;

  using _eo_methods = eo::methods<eo::method<&Point::String, "String">>;
};

struct Temp {
  static constexpr std::string_view _eo_name = "main.Temp";

  int64_t C{};

  auto operator==(const Temp&) const -> bool = default;

  static auto String(ptr<Temp> t) -> std::string;

  using _eo_methods = eo::methods<eo::method<&Temp::String, "String">>;
};

struct Fault {
  static constexpr std::string_view _eo_name = "main.Fault";

  int64_t Code{};

  auto operator==(const Fault&) const -> bool = default;

  static auto Error(Fault f) -> std::string;
  static auto String(Fault f) -> std::string;

  using _eo_methods = eo::methods<eo::method<&Fault::Error, "Error">, eo::method<&Fault::String, "String">>;
};

struct Boom {
  static constexpr std::string_view _eo_name = "main.Boom";

  auto operator==(const Boom&) const -> bool = default;

  static auto String(Boom _eo_receiver) -> std::string;

  using _eo_methods = eo::methods<eo::method<&Boom::String, "String">>;
};

struct Pair {
  static constexpr std::string_view _eo_name = "main.Pair";

  Point A{};
  ptr<Temp> B{};
  int64_t N{};

  auto operator==(const Pair&) const -> bool = default;
};

auto digit(int64_t n) -> std::string {
  return eo::string_from_rune(int32_t('0' + n));
}

auto Point::String(Point p) -> std::string {
  return "("s + digit(p.X) + ","s + digit(p.Y) + ")"s;
}

auto Temp::String(ptr<Temp> t) -> std::string {
  return digit(t->C) + "C"s;
}

auto Fault::Error(Fault f) -> std::string {
  return "fault "s + digit(f.Code);
}

auto Fault::String(Fault f) -> std::string {
  return "unused"s;
}

auto Boom::String(Boom _eo_receiver) -> std::string {
  panic("boom"s);
}

func<> eo_main() {
  auto p = Point{.X = 1, .Y = 2};
  fmt::Println(p, ref(p));

  // String has a pointer receiver, so a Temp value prints as a struct.
  auto t = Temp{.C = 2};
  fmt::Println(t, ref(t));

  // Error is preferred to String.
  fmt::Println(Fault{.Code = 7});

  // A nil receiver whose method panics prints as <nil>; any other panic is reported.
  ptr<Point> nilTemp{};
  fmt::Println(nilTemp, Boom{});

  fmt::Stringer s = p;
  any e{};
  fmt::Println(s, e, slice<any>::of({int64_t{1}, "a"s, nullptr, double{2.5}, p, ref(t)}));
  fmt::Println(Pair{.A = p, .B = ref(t), .N = 3}, slice<Point>::of({p, Point{.X = 3, .Y = 4}}));
  co_return;
}
