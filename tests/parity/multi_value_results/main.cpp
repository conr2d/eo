#include <eo/fmt.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>

using namespace eo;

struct Point;
struct Splitter;
struct Halves;

struct Point {
  static constexpr std::string_view _eo_name = "main.Point";

  int64_t X{}, Y{};

  auto operator==(const Point&) const -> bool = default;

  static auto Move(ptr<Point> p, int64_t dx, int64_t dy) -> std::tuple<int64_t, int64_t>;

  using _eo_methods = eo::methods<eo::method<&Point::Move, "Move">>;
};

struct Splitter : interface<Splitter> {
  using interface::interface;
  static constexpr std::string_view _eo_name = "main.Splitter";

  auto Split(int64_t n) const -> std::tuple<int64_t, int64_t>;

  using _eo_methods = eo::methods<eo::method<&Splitter::Split, "Split">>;
};

struct Halves {
  static constexpr std::string_view _eo_name = "main.Halves";

  int64_t Offset{};

  auto operator==(const Halves&) const -> bool = default;

  static auto Split(Halves h, int64_t n) -> std::tuple<int64_t, int64_t>;

  using _eo_methods = eo::methods<eo::method<&Halves::Split, "Split">>;
};

auto Splitter::Split(int64_t n) const -> std::tuple<int64_t, int64_t> {
  return _eo_call<&Splitter::Split>(n);
}

auto Halves::Split(Halves h, int64_t n) -> std::tuple<int64_t, int64_t> {
  return {n / 2 + h.Offset, n - n / 2 + h.Offset};
}

auto Point::Move(ptr<Point> p, int64_t dx, int64_t dy) -> std::tuple<int64_t, int64_t> {
  p->X += dx;
  p->Y += dy;
  return {p->X, p->Y};
}

auto trace(std::string name, int64_t v) -> int64_t {
  fmt::Println("eval"s, name);
  return v;
}

auto pair(int64_t n) -> std::tuple<int64_t, std::string> {
  return {n * 2, "pair"s};
}

auto forward(int64_t n) -> std::tuple<int64_t, std::string> {
  return pair(n + 1);
}

auto three() -> std::tuple<int64_t, bool, std::string> {
  return {1, true, "three"s};
}

auto lookup(std::string k) -> std::tuple<int64_t, bool> {
  if (k == "a"s) {
    return {1, true};
  }
  return {0, false};
}

auto ordered() -> std::tuple<int64_t, int64_t> {
  return {trace("first"s, 1), trace("second"s, 2)};
}

auto divmod(int64_t a, int64_t b) -> std::tuple<int64_t, int64_t> {
  int64_t q{};
  int64_t r{};
  q = eo::div(a, b);
  r = eo::rem(a, b);
  return {q, r};
}

auto swapped(int64_t a, int64_t b) -> std::tuple<int64_t, int64_t> {
  int64_t x{};
  int64_t y{};
  x = a;
  y = b;
  return {y, x};
}

void show(int64_t n, std::string s) {
  fmt::Println("show"s, n, s);
}

func<> eo_main() {
  auto [v, s] = pair(3);
  fmt::Println(v, s);

  std::tie(v, s) = forward(4);
  fmt::Println(v, s);

  int64_t w{};
  std::tie(w, s) = pair(5);
  fmt::Println(w, s);

  auto n = std::get<0>(lookup("a"s));
  auto ok = std::get<1>(lookup("b"s));
  fmt::Println(n, ok);

  n = std::get<0>(lookup("z"s));
  fmt::Println(n);

  lookup("a"s);

  int64_t a{};
  std::string c{};
  std::tie(a, std::ignore, c) = three();
  fmt::Println(a, c);

  if (auto found = std::get<1>(lookup("a"s)); found) {
    fmt::Println("found a"s);
  }
  if (auto [k, found] = lookup("c"s); !found) {
    fmt::Println("missing c"s, k);
  }

  auto [x, y] = ordered();
  fmt::Println(x, y);

  std::apply(fmt::Println, divmod(17, 5));
  std::apply(fmt::Println, swapped(1, 2));

  std::apply(show, pair(6));

  Splitter sp = Halves{.Offset = 10};
  auto [lo, hi] = sp.Split(7);
  fmt::Println(lo, hi);

  auto pt = Point{.X = 1, .Y = 2};
  auto [px, py] = Point::Move(ref(pt), 3, 4);
  fmt::Println(px, py, pt);

  auto [count, label] = pair(1);
  auto inc = [=, &count]() mutable { count++; };
  inc();
  auto get = [=]() mutable -> std::string { return label; };
  fmt::Println(count, get());

  slice<ptr<int64_t>> ptrs;
  for (auto i = int64_t{0}; i < 2; i++) {
    auto m = New<int64_t>(std::get<0>(lookup("a"s)));
    *m += i;
    ptrs = eo::append(ptrs, {m});
  }
  fmt::Println(*eo::at(ptrs, 0), *eo::at(ptrs, 1));

  ptr<int64_t> keep;
  {
    auto q = New<int64_t>();
    int64_t r{};
    std::tie(*q, r) = divmod(9, 4);
    keep = q;
    fmt::Println(r);
  }
  fmt::Println(*keep);
  co_return;
}
