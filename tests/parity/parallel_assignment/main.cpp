#include <eo/errors.h>
#include <eo/fmt.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>

using namespace eo;

struct pair;

struct pair {
  static constexpr std::string_view _eo_name = "main.pair";

  int64_t a{};
  int64_t b{};

  auto operator==(const pair&) const -> bool = default;
};

ptr<int64_t> kept{};

auto trace(std::string name, int64_t v) -> int64_t {
  fmt::Println("eval"s, name);
  return v;
}

auto fib(int64_t n) -> int64_t {
  auto a = int64_t{0};
  auto b = int64_t{1};
  for (auto i = int64_t{0}; i < n; i++) {
    std::tie(a, b) = std::tuple<int64_t, int64_t>{b, a + b};
  }
  return a;
}

void reverse(slice<int64_t> s) {
  for (auto i = int64_t{0}, j = eo::len(s) - 1; i < j; std::tie(i, j) = std::tuple<int64_t, int64_t>{i + 1, j - 1}) {
    auto t = eo::at(s, i);
    eo::at(s, i) = eo::at(s, j);
    eo::at(s, j) = t;
  }
}

auto lookup(map<std::string, int64_t> m, std::string k) -> std::tuple<int64_t, error> {
  int64_t v{};
  error err{};
  std::tie(v, err) = std::tuple<int64_t, error>{eo::map_get(m, k), nullptr};
  if (v == 0) {
    std::tie(v, err) = std::tuple<int64_t, error>{-1, errors::New("missing "s + k)};
  }
  return {v, err};
}

auto partial(slice<int64_t> s) -> std::tuple<int64_t, int64_t> {
  int64_t a{};
  int64_t b{};
  eo_defer_scope;
  try {
    eo_defer([&] {
      if (auto r = recover(); r != nullptr) {
        fmt::Println("recovered:"s, r, a, b);
      }
    });
    a = 1;
    b = 2;
    std::tie(a, b) = std::tuple<int64_t, int64_t>{10, eo::at(s, 5)};
    std::tie(a, b) = std::tuple<int64_t, int64_t>{a, b};
    eo_defer_run;
    return {a, b};
  } catch (...) {
    eo_defer_panic;
  }
  return {a, b};
}

func<> eo_main() {
  auto _eo_call_0 = fib(10);
  auto _eo_call_1 = fib(50);
  fmt::Println(_eo_call_0, _eo_call_1);

  auto x = int64_t{1};
  auto y = int64_t{2};
  std::tie(x, y) = std::tuple<int64_t, int64_t>{y, x};
  fmt::Println(x, y);
  std::tie(x, y) = std::tuple<int64_t, int64_t>{y + 10, x * 10};
  fmt::Println(x, y);
  std::tie(x, y) = std::tuple<int64_t, int64_t>{trace("first"s, y), trace("second"s, x)};
  fmt::Println(x, y);
  std::tie(std::ignore, y) = std::tuple<int64_t, int64_t>{trace("blank"s, 0), 7};
  fmt::Println(x, y);

  double f{};
  uint8_t u{};
  int32_t r{};
  std::tie(f, u, r) = std::tuple<double, uint8_t, int32_t>{1, 255, 122};
  fmt::Println(f, u, r);

  error e{};
  ptr<pair> p{};
  std::tie(e, p) = std::tuple<error, ptr<pair>>{nullptr, New<pair>(pair{1, 2})};
  fmt::Println(e == nullptr, *p);
  std::tie(e, p) = std::tuple<error, ptr<pair>>{errors::New("boom"s), nullptr};
  fmt::Println(e, p == nullptr);

  any v{};
  std::string s{};
  std::tie(v, s) = std::tuple<any, std::string>{int64_t{3}, "three"s};
  fmt::Println(v, s);

  auto z = New<int64_t>(5);
  kept = z;
  std::tie(*z, x) = std::tuple<int64_t, int64_t>{x, *z};
  fmt::Println(*z, x, *kept);

  auto nums = slice<int64_t>::of({1, 2, 3, 4, 5});
  reverse(nums);
  fmt::Println(nums);

  auto m = map<std::string, int64_t>::of({{"a"s, 1}});
  std::apply(fmt::Println, lookup(m, "a"s));
  std::apply(fmt::Println, lookup(m, "b"s));

  std::apply(fmt::Println, partial(slice<int64_t>::of({1, 2})));
  std::apply(fmt::Println, partial(slice<int64_t>::of({0, 0, 0, 0, 0, 6})));
  co_return;
}
