#include <eo/fmt.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>

using namespace eo;

struct Point;

struct Point {
  static constexpr std::string_view _eo_name = "main.Point";

  int64_t X{}, Y{};

  auto operator==(const Point&) const -> bool = default;
};

void add(map<std::string, int64_t> m, std::string k) {
  eo::map_entry(m, k)++;
}

func<> eo_main() {
  auto ages = map<std::string, int64_t>::of({{"bob"s, 31}, {"alice"s, 29}});
  auto alias = ages;
  eo::map_entry(alias, "carol"s) = 40;
  add(ages, "bob"s);
  fmt::Println(ages, eo::len(ages), eo::len(alias));

  fmt::Println(eo::map_get(ages, "dave"s), eo::len(ages));
  auto [v, ok] = eo::map_get_ok(ages, "alice"s);
  fmt::Println(v, ok);
  std::tie(v, ok) = eo::map_get_ok(ages, "dave"s);
  fmt::Println(v, ok);
  if (auto ok = eo::map_get_ok(ages, "carol"s).second; ok) {
    fmt::Println("carol present"s);
  }

  eo::delete_(ages, "bob"s);
  eo::delete_(ages, "nobody"s);
  fmt::Println(ages, eo::len(ages));

  auto counts = eo::make_map<int32_t, int64_t>();
  for (auto [_eo_range_key, r] : eo::range("hello"s)) {
    eo::map_entry(counts, r)++;
  }
  eo::map_entry(counts, 122) += 2;
  fmt::Println(counts);

  auto sum = int64_t{0};
  for (auto [k, v] : eo::range(counts)) {
    sum += int64_t(k) * v;
  }
  fmt::Println(sum);

  map<std::string, int64_t> nilMap;
  fmt::Println(nilMap == nullptr, eo::len(nilMap), eo::map_get(nilMap, "x"s), nilMap);
  for ([[maybe_unused]] auto _eo_range_key : eo::range(nilMap)) {
    fmt::Println("unreachable"s);
  }
  eo::delete_(nilMap, "x"s);

  auto grid = map<Point, std::string>::of({{Point{1, 2}, "a"s}, {Point{0, 5}, "b"s}});
  eo::map_entry(grid, Point{1, 2}) = "c"s;
  fmt::Println(grid, eo::map_get(grid, Point{0, 5}), eo::len(grid));

  auto groups = map<std::string, slice<int64_t>>::of({});
  eo::map_entry(groups, "even"s) = eo::append(eo::map_get(groups, "even"s), {2, 4});
  eo::map_entry(groups, "odd"s) = eo::append(eo::map_get(groups, "odd"s), {1});
  fmt::Println(groups);

  auto nested = map<std::string, map<std::string, int64_t>>::of({{"x"s, map<std::string, int64_t>::of({{"y"s, 1}})}});
  eo::map_entry(eo::map_get(nested, "x"s), "z"s) = 2;
  fmt::Println(nested, eo::map_get(eo::map_get(nested, "w"s), "y"s), eo::len(nested));

  auto m = map<int64_t, bool>::of({{3, true}, {-1, false}, {2, true}});
  eo::map_entry(m, eo::len(m)) = false;
  fmt::Println(m);
  fmt::Printf("%v|%d|%T|%T\n"s, m, map<std::string, int64_t>::of({{"k"s, 7}}), m, groups);

  eo::clear(ages);
  fmt::Println(ages, eo::len(ages), ages == nullptr, eo::len(alias));

  auto q = New<map<std::string, int64_t>>(map<std::string, int64_t>::of({{"x"s, 1}}));
  auto p = q;
  eo::map_entry(*p, "y"s) = 2;
  fmt::Println(p, eo::len(*q));
  co_return;
}
