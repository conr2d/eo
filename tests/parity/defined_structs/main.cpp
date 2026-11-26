#include <eo/bytes.h>
#include <eo/encoding/json.h>
#include <eo/fmt.h>
#include <eo/math/big.h>

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>

using namespace eo;

struct Point;
struct Vec;
struct Pair;
struct Holder;
struct Tagged;
struct Alias;
struct Amount;
struct Celsius;
struct Hash;
struct Key;

// Point has a String method, which a defined type over it does not inherit.
struct Point {
  static constexpr std::string_view _eo_name = "main.Point";

  int64_t X{};
  int64_t Y{};

  static auto String(Point p) -> std::string;
  static void Move(ptr<Point> p, int64_t dx);

  using _eo_methods = eo::methods<eo::method<&Point::String, "String">, eo::method<&Point::Move, "Move">>;

  auto operator==(const Point&) const -> bool = default;
};

// Vec is Point's struct type without Point's methods.
struct Vec : eo::defined<Vec, Point> {
  using defined::defined;
  static constexpr std::string_view _eo_name = "main.Vec";
};

// Pair is a defined type over Vec, so over Point's struct type too, with a method of its own.
struct Pair : eo::defined<Pair, Point> {
  using defined::defined;
  static constexpr std::string_view _eo_name = "main.Pair";

  static void Swap(ptr<Pair> p);

  using _eo_methods = eo::methods<eo::method<&Pair::Swap, "Swap">>;
};

struct Holder {
  static constexpr std::string_view _eo_name = "main.Holder";

  Vec V{};
  Pair P{};

  auto operator==(const Holder&) const -> bool = default;
};

struct Tagged {
  static constexpr std::string_view _eo_name = "main.Tagged";

  std::string Name{};
  int64_t N{};

  static constexpr eo::field _eo_fields[] = {{.tag = "json:\"name\""}, {.tag = "json:\"n,omitempty\""}};

  auto operator==(const Tagged&) const -> bool = default;
};

struct Alias : eo::defined<Alias, Tagged> {
  using defined::defined;
  static constexpr std::string_view _eo_name = "main.Alias";
};

// Amount is big.Int with methods of its own, as go-ethereum's hexutil.Big is.
struct Amount : eo::defined<Amount, big::Int> {
  using defined::defined;
  static constexpr std::string_view _eo_name = "main.Amount";

  static auto String(ptr<Amount> a) -> std::string;
  static auto MarshalText(Amount a) -> std::tuple<slice<uint8_t>, error>;

  using _eo_methods =
    eo::methods<eo::method<&Amount::String, "String">, eo::method<&Amount::MarshalText, "MarshalText">>;
};

struct Celsius : eo::defined<Celsius, double> {
  using defined::defined;
  static constexpr std::string_view _eo_name = "main.Celsius";
};

struct Hash : eo::defined<Hash, std::array<uint8_t, 2>> {
  using defined::defined;
  static constexpr std::string_view _eo_name = "main.Hash";
};

struct Key : eo::defined<Key, std::array<uint8_t, 2>> {
  using defined::defined;
  static constexpr std::string_view _eo_name = "main.Key";
};

auto Point::String(Point p) -> std::string {
  return fmt::Sprintf("(%d, %d)"s, p.X, p.Y);
}

void Point::Move(ptr<Point> p, int64_t dx) {
  p->X += dx;
}

void Pair::Swap(ptr<Pair> p) {
  auto x = p->X;
  p->X = p->Y;
  p->Y = x;
}

auto Amount::String(ptr<Amount> a) -> std::string {
  return "amount "s + big::Int::String(eo::ptr_cast<big::Int>(a));
}

auto Amount::MarshalText(Amount a) -> std::tuple<slice<uint8_t>, error> {
  auto _eo_call_0 = big::Int::Text(eo::ptr_cast<big::Int>(ref(a)), 16);
  return {eo::bytes_from_string(_eo_call_0), nullptr};
}

func<> eo_main() {
  auto p = Point{1, 2};
  auto v = Vec(p);
  fmt::Println(p, v, Point(v), v.X);
  fmt::Printf("%v %+v %T %T\n"s, v, v, v, p);
  auto isStringer = eo::type_assert_ok<fmt::Stringer>(any(v)).second;
  fmt::Println(isStringer, v == Vec(Point{1, 2}), Point(v) == p);

  auto pv = eo::ptr_cast<Point>(ref(v));
  Point::Move(pv, 10);
  fmt::Println(v, *pv, Point::String(*pv));
  auto pp = ref(p);
  auto vp = eo::ptr_cast<Vec>(pp);
  vp->Y = 7;
  fmt::Println(p, *vp);

  auto pair = Pair(v);
  Pair::Swap(ref(pair));
  fmt::Println(pair, Point(pair), Vec(pair));
  Pair::Swap(eo::ptr_cast<Pair>(pp));
  fmt::Println(p);

  fmt::Printf("%#v %v %+v\n"s, v, slice<Vec>::of({v, Vec(Point{3, 4})}), Holder{v, pair});
  auto seen = map<Vec, std::string>::of({{v, "v"s}});
  fmt::Println(eo::map_get(seen, Vec(p)), eo::map_get(seen, Vec(Point{11, 2})), eo::len(seen));
  if (auto _eo_type_switch = any(pair); eo::holds<Point>(_eo_type_switch)) {
    auto x = eo::type_assert<Point>(_eo_type_switch);
    fmt::Println("point"s, x);
  } else if (eo::holds<Pair>(_eo_type_switch)) {
    auto x = eo::type_assert<Pair>(_eo_type_switch);
    fmt::Println("pair"s, x);
  }

  ptr<Point> none{};
  fmt::Println(eo::ptr_cast<Vec>(none) == nullptr, eo::ptr_cast<Pair>(eo::ptr_cast<Vec>(none)) == nullptr);

  auto a = Alias(Tagged{.Name = "eo"s});
  auto [out, err] = json::Marshal(a);
  fmt::Println(eo::string_from_bytes(out), err);
  auto back = New<Alias>();
  err = json::Unmarshal(eo::bytes_from_string(R"({"name":"go","n":3})"s), back);
  fmt::Println(*back, err, Tagged(*back));

  auto n = big::NewInt(255);
  auto amount = eo::ptr_cast<Amount>(n);
  fmt::Println(amount, Amount::String(amount));
  big::Int::SetInt64(n, 4096);
  fmt::Println(amount);
  big::Int::Neg(eo::ptr_cast<big::Int>(amount), n);
  fmt::Println(n, amount);
  fmt::Println(Amount(*n), *amount);
  auto text = std::get<0>(Amount::MarshalText(Amount(*n)));
  fmt::Println(eo::string_from_bytes(text));
  Amount zero{};
  std::tie(text, err) = Amount::MarshalText(zero);
  auto _eo_call_0 = big::Int::Sign(eo::ptr_cast<big::Int>(ref(zero)));
  fmt::Println(_eo_call_0, eo::string_from_bytes(text), err);
  slice<uint8_t> enc{};
  std::tie(enc, err) = json::Marshal(map<std::string, ptr<Amount>>::of({{"v"s, amount}}));
  fmt::Println(eo::string_from_bytes(enc), err);

  auto f = New<double>(36.6);
  auto c = eo::ptr_cast<Celsius>(f);
  *c += Celsius{1.0};
  fmt::Printf("%.1f %.1f %T\n"s, *f, *c, c);
  auto h = Hash(std::array<uint8_t, 2>{1, 2});
  auto k = eo::ptr_cast<Key>(ref(h));
  eo::at(*k, 0) = 9;
  fmt::Println(h, *k, Hash(*k) == h);
  auto hs = eo::ptr_cast<std::array<uint8_t, 2>>(k);
  eo::at(*hs, 1) = 8;
  fmt::Println(h);
  co_return;
}
