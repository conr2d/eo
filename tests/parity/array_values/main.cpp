#include <eo/fmt.h>

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

using namespace eo;

struct Point {
  static constexpr std::string_view _eo_name = "main.Point";

  int64_t X{}, Y{};

  auto operator==(const Point&) const -> bool = default;
};

struct Grid {
  static constexpr std::string_view _eo_name = "main.Grid";

  std::string Name{};
  std::array<int64_t, 3> Cells{};

  auto operator==(const Grid&) const -> bool = default;
};

auto bump(std::array<int64_t, 3> a) -> std::array<int64_t, 3> {
  eo::at(a, 0)++;
  return a;
}

auto get(std::array<int64_t, 4> a, int64_t i) -> int64_t {
  int64_t v{};
  eo_defer_scope;
  try {
    eo_defer([&] {
      if (auto r = recover(); r != nullptr) {
        fmt::Println("recovered:"s, r);
        v = -1;
      }
    });
    v = eo::at(a, i);
    eo_defer_run;
    return v;
  } catch (...) {
    eo_defer_panic;
  }
  return v;
}

auto sum(ptr<std::array<int64_t, 2>> p) -> int64_t {
  int64_t n{};
  eo_defer_scope;
  try {
    eo_defer([&] {
      if (auto r = recover(); r != nullptr) {
        fmt::Println("recovered:"s, r);
      }
    });
    for (auto [_eo_range_key, v] : eo::range(p)) {
      n += v;
    }
    eo_defer_run;
    return n;
  } catch (...) {
    eo_defer_panic;
  }
  return n;
}

func<> eo_main() {
  // Assignment, arguments, and results copy an array.
  auto a = std::array<int64_t, 3>{1, 2, 3};
  auto b = a;
  eo::at(b, 1) = 20;
  auto c = bump(a);
  fmt::Println(a, b, c, eo::len(a), eo::cap(a));
  fmt::Println(a == std::array<int64_t, 3>{1, 2, 3}, a != b, a == c);

  // Missing elements are zero, [...] counts the elements, and keyed elements leave zero gaps.
  std::array<int64_t, 4> z{};
  auto few = std::array<std::string, 5>{"x"s, "y"s};
  auto counted = std::array<double, 2>{1.5, 2.5};
  auto keyed = eo::keyed_array<std::array<int64_t, 6>>({{0, 1}, {4, 40}, {5, 50}, {2, 20}});
  auto tail = eo::keyed_array<std::array<std::string, 4>>({{3, "d"s}, {1, "b"s}});
  auto ks = slice<int64_t>::keyed(4, {{2, 7}, {3, 9}});
  fmt::Println(z, eo::len(counted), counted, keyed, eo::len(tail), ks, eo::len(ks), eo::cap(ks));
  fmt::Printf("%q %q\n"s, few, tail);

  // An index out of range panics.
  auto _eo_call_0 = get(std::array<int64_t, 4>{5, 6, 7, 8}, 3);
  auto _eo_call_1 = get(std::array<int64_t, 4>{}, 4);
  auto _eo_call_2 = get(std::array<int64_t, 4>{}, -1);
  fmt::Println(_eo_call_0, _eo_call_1, _eo_call_2);

  // Arrays of arrays and of structs, and a struct with an array field.
  auto m = New<std::array<std::array<int64_t, 3>, 2>>();
  eo::at(eo::at(*m, 1), 2) = 9;
  auto row = eo::at(*m, 1);
  eo::at(row, 0) = 1;
  auto ps = New<std::array<Point, 2>>(std::array<Point, 2>{Point{1, 2}, Point{3, 4}});
  eo::at(*ps, 1).X = 30;
  auto g = Grid{.Name = "g"s, .Cells = std::array<int64_t, 3>{7, 8, 9}};
  auto h = g;
  eo::at(h.Cells, 0) = 70;
  fmt::Println(*m, row, *ps, g, h, g == h, g == Grid{"g"s, std::array<int64_t, 3>{7, 8, 9}});
  fmt::Printf("%v %v %d\n"s, *ps, g, *m);

  // The address of an element keeps referring to the array.
  auto p = ref(eo::at(a, 2));
  *p = 30;
  auto q = ptr(m, &eo::at(*m, 0));
  eo::at(*q, 1) = 5;
  fmt::Println(a, *m);

  // range over an array iterates over a copy of it, and range over a pointer to an array does not.
  auto arr = New<std::array<int64_t, 3>>(std::array<int64_t, 3>{1, 2, 3});
  for (auto [i, v] : eo::range(*arr)) {
    eo::at(*arr, 2) = 100;
    fmt::Printf("%d:%d "s, i, v);
  }
  fmt::Println(*arr);
  *arr = std::array<int64_t, 3>{1, 2, 3};
  for (auto [i, v] : eo::range(arr)) {
    eo::at(*arr, 2) = 100;
    fmt::Printf("%d:%d "s, i, v);
  }
  fmt::Println();
  ptr<std::array<int64_t, 2>> nilArr;
  for (auto i : eo::range(eo::len(nilArr))) {
    fmt::Printf("%d "s, i);
  }
  for (auto i : eo::range(eo::len(nilArr))) {
    fmt::Printf("%d "s, i);
  }
  auto _eo_call_3 = sum(nilArr);
  auto _eo_call_4 = sum(New<std::array<int64_t, 2>>(std::array<int64_t, 2>{3, 4}));
  fmt::Println(eo::len(nilArr), eo::len(nilArr), eo::cap(nilArr), _eo_call_3, _eo_call_4);

  // Printing.
  auto bs = New<std::array<uint8_t, 4>>(std::array<uint8_t, 4>{0xde, 0xad, 104, 105});
  fmt::Println(*bs, arr, std::array<int64_t, 0>{});
  fmt::Printf("%v %x %X %s %q %T %T\n"s, *bs, *bs, *bs, eo::subslice(bs, 2), eo::subslice(bs, 2), *bs, *m);
  fmt::Printf("%v %d %T\n"s, ps, m, m);

  // Arrays as map keys and in interfaces.
  auto seen = map<std::array<int64_t, 2>, std::string>::of(
    {{std::array<int64_t, 2>{1, 2}, "a"s}, {std::array<int64_t, 2>{0, 5}, "b"s}});
  eo::map_entry(seen, std::array<int64_t, 2>{1, 2}) = "c"s;
  auto fk = map<std::array<double, 2>, int64_t>::of({{std::array<double, 2>{0, 1}, 1}});
  eo::map_entry(fk, std::array<double, 2>{0, 1})++;
  auto zero = 0.0;
  eo::map_entry(fk, std::array<double, 2>{-zero, 1})++;
  fmt::Println(seen, eo::len(seen), eo::map_get(seen, std::array<int64_t, 2>{0, 5}), fk);
  any x = std::array<int64_t, 2>{1, 2};
  any y = std::array<int64_t, 2>{1, 2};
  fmt::Println(x == y, x == any(std::array<int64_t, 2>{2, 1}), x);
  if (auto [v, ok] = eo::type_assert_ok<std::array<int64_t, 2>>(x); ok) {
    fmt::Println(eo::at(v, 1));
  }
  auto ok = eo::type_assert_ok<std::array<int64_t, 3>>(x).second;
  fmt::Println(ok);
  co_return;
}
