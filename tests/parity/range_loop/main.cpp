// Provisional: the C++ type of Go int is not a frozen mapping (see ../README.md).
#include <eo/fmt.h>

#include <cstdint>
#include <string>

using namespace eo;

auto next() -> slice<int64_t> {
  fmt::Println("next called"s);
  return slice<int64_t>::of({1, 2, 3});
}

func<> eo_main() {
  // range over an integer
  for (auto i : eo::range(int64_t{3})) {
    fmt::Println("int"s, i);
  }
  auto n = uint8_t{2};
  for (auto i : eo::range(n)) {
    fmt::Println("uint8"s, i);
  }
  auto count = int64_t{0};
  for ([[maybe_unused]] auto _eo_range_blank : eo::range(int64_t{4})) {
    count++;
  }
  fmt::Println("count"s, count);
  for (auto i : eo::range(int64_t{3})) {
    i += 10;
    fmt::Println("write"s, i);
  }
  for (auto i : eo::range(int64_t{-2})) {
    fmt::Println("negative"s, i);
  }
  auto j = int64_t{7};
  for (auto _eo_range_j : eo::range(int64_t{3})) {
    j = _eo_range_j;
  }
  fmt::Println("assigned"s, j);
  for (auto _eo_range_j : eo::range(int64_t{0})) {
    j = _eo_range_j;
  }
  fmt::Println("unchanged"s, j);

  // range over a slice: the slice is evaluated once and each element is read when its iteration starts
  auto s = slice<int64_t>::of({10, 20, 30});
  for (auto [i, v] : eo::range(s)) {
    if (i == 0) {
      eo::at(s, 2) = 99;
      s = eo::append(s, {40});
      eo::at(s, 1) = -1;
    }
    fmt::Println("slice"s, i, v);
  }
  fmt::Println(s);
  auto u = eo::make_slice<int64_t>(2, 4);
  for (auto i : eo::range(eo::len(u))) {
    u = eo::append(u, {i});
  }
  fmt::Println(u, eo::len(u));
  for (auto [_eo_range_blank, v] : eo::range(next())) {
    fmt::Println("once"s, v);
  }
  slice<std::string> none;
  for (auto i : eo::range(eo::len(none))) {
    fmt::Println("never"s, i);
  }
  auto words = slice<std::string>::of({"a"s, "b"s});
  for (auto [_eo_range_blank, w] : eo::range(words)) {
    w += "!"s;
    fmt::Println(w);
  }
  fmt::Println(words);
  int64_t k{};
  std::string w;
  for (auto [_eo_range_k, _eo_range_w] : eo::range(words)) {
    k = _eo_range_k;
    w = _eo_range_w;
  }
  fmt::Println(k, w);
  for (auto [i, v] : eo::range(s)) {
    auto sum = [=]() mutable -> int64_t { return i + v; };
    fmt::Println("captured"s, sum());
  }
  for (auto [_eo_range_blank, v] : eo::range(slice<int64_t>::of({5}))) {
    auto inc = [=, &v]() mutable { v++; };
    inc();
    fmt::Println("by ref"s, v);
  }

  // break and continue
  for (auto i : eo::range(int64_t{3})) {
    if (i == 1) {
      continue;
    }
    for (auto [_eo_range_blank, v] : eo::range(s)) {
      if (v > 50) {
        break;
      }
      fmt::Println("nested"s, i, v);
    }
  }

  // range over a string decodes UTF-8
  for (auto [i, r] : eo::range("a\u00e9\x{ff}\u20ac"s)) {
    fmt::Println("string"s, i, r);
  }
  auto str = "h\u00e9llo"s;
  for (auto [i, _eo_range_blank] : eo::range(str)) {
    if (i == 0) {
      str = "x"s;
    }
    fmt::Println("index"s, i);
  }
  fmt::Println(str);
  for ([[maybe_unused]] auto _eo_range_blank : eo::range("abc"s)) {
    count++;
  }
  fmt::Println("count"s, count);
  for (auto [i, b] : eo::range(eo::bytes_from_string("hi"s))) {
    fmt::Println("byte"s, i, b);
  }
  co_return;
}
