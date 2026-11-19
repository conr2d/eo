#include <eo/fmt.h>

#include <cstdint>
#include <string>

using namespace eo;

func<> eo_main() {
  auto m = map<int64_t, int64_t>::of({{1, 10}, {2, 20}, {3, 30}, {4, 40}});
  auto iterations = int64_t{0};
  for (auto [k, _eo_range_value] : eo::range(m)) {
    iterations++;
    for (auto [_eo_range_key, other] : eo::range(slice<int64_t>::of({1, 2, 3, 4}))) {
      if (other != k) {
        eo::delete_(m, other);
      }
    }
  }
  fmt::Println(iterations, eo::len(m));

  auto u = map<std::string, int64_t>::of({{"a"s, 0}, {"b"s, 0}});
  auto total = int64_t{0};
  for (auto [k, v] : eo::range(u)) {
    total += v;
    if (k == "a"s) {
      eo::map_entry(u, "b"s) = 5;
    } else {
      eo::map_entry(u, "a"s) = 5;
    }
  }
  fmt::Println(total);

  auto c = map<int64_t, bool>::of({{1, true}, {2, true}, {3, true}});
  iterations = 0;
  for ([[maybe_unused]] auto _eo_range_key : eo::range(c)) {
    iterations++;
    eo::clear(c);
  }
  fmt::Println(iterations, eo::len(c));

  auto g = map<int64_t, int64_t>::of({{0, 0}});
  for (auto [k, _eo_range_value] : eo::range(g)) {
    if (k < 50) {
      eo::map_entry(g, k + 100) = k;
    }
  }
  fmt::Println(eo::len(g), eo::map_get(g, 100));

  auto r = map<std::string, int64_t>::of({{"x"s, 1}});
  for (auto [k, v] : eo::range(r)) {
    r = map<std::string, int64_t>::of({});
    eo::map_entry(r, k) = v + 1;
  }
  fmt::Println(r);
  co_return;
}
