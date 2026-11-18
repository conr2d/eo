// Provisional: the C++ type of Go int is not a frozen mapping (see ../README.md).
#include <eo/fmt.h>

#include <cstdint>
#include <string>

using namespace eo;

auto next() -> slice<int64_t> {
  fmt::Println("next called"s);
  return slice<int64_t>::of({1, 2, 3});
}

auto g() -> int64_t {
  fmt::Println("g called"s);
  return 1;
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
  for ([[maybe_unused]] auto _eo_range_key : eo::range(int64_t{4})) {
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
  for (auto _eo_range_key : eo::range(int64_t{3})) {
    j = _eo_range_key;
  }
  fmt::Println("assigned"s, j);
  for (auto _eo_range_key : eo::range(int64_t{0})) {
    j = _eo_range_key;
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
  for (auto [_eo_range_key, v] : eo::range(next())) {
    fmt::Println("once"s, v);
  }
  slice<std::string> none;
  for (auto i : eo::range(eo::len(none))) {
    fmt::Println("never"s, i);
  }
  auto words = slice<std::string>::of({"a"s, "b"s});
  for (auto [_eo_range_key, w] : eo::range(words)) {
    w += "!"s;
    fmt::Println(w);
  }
  fmt::Println(words);
  int64_t k{};
  std::string w;
  for (auto [_eo_range_key, _eo_range_value] : eo::range(words)) {
    k = _eo_range_key;
    w = _eo_range_value;
  }
  fmt::Println(k, w);
  for (auto [i, v] : eo::range(s)) {
    auto sum = [=]() mutable -> int64_t { return i + v; };
    fmt::Println("captured"s, sum());
  }
  for (auto [_eo_range_key, v] : eo::range(slice<int64_t>::of({5}))) {
    auto inc = [=, &v]() mutable { v++; };
    inc();
    fmt::Println("by ref"s, v);
  }

  // break and continue
  for (auto i : eo::range(int64_t{3})) {
    if (i == 1) {
      continue;
    }
    for (auto [_eo_range_key, v] : eo::range(s)) {
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
  for (auto [i, _eo_range_value] : eo::range(str)) {
    if (i == 0) {
      str = "x"s;
    }
    fmt::Println("index"s, i);
  }
  fmt::Println(str);
  for ([[maybe_unused]] auto _eo_range_key : eo::range("abc"s)) {
    count++;
  }
  fmt::Println("count"s, count);
  for (auto [i, b] : eo::range(eo::bytes_from_string("hi"s))) {
    fmt::Println("byte"s, i, b);
  }

  // a nested call in the range expression runs once, before the loop
  auto _eo_call_0 = next();
  auto _eo_call_1 = g();
  for (auto [_eo_range_key, v] : eo::range(eo::subslice(_eo_call_0, _eo_call_1))) {
    fmt::Println("tail"s, v);
  }

  // an untyped rune constant ranges over rune values
  for (auto r : eo::range(int32_t{2})) {
    fmt::Println("rune"s, eo::string_from_rune(r + 'a'));
  }

  // a body may redeclare a clause variable
  for (auto [_eo_range_key, v] : eo::range(s)) {
    auto _eo_shadow_v = v * 10;
    fmt::Println("shadow"s, _eo_shadow_v);
  }
  for (auto i = int64_t{0}; i < 2; i++) {
    auto _eo_shadow_i = i + 100;
    fmt::Println("shadow"s, _eo_shadow_i);
  }

  // the same variable on both sides is assigned the index, then the element
  int64_t x{};
  for (auto [_eo_range_key, _eo_range_value] : eo::range(slice<int64_t>::of({7, 8}))) {
    x = _eo_range_key;
    x = _eo_range_value;
  }
  fmt::Println("twice"s, x);
  co_return;
}
