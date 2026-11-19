#include <eo/fmt.h>

#include <cstdint>
#include <string>

using namespace eo;

auto grade(int64_t n) -> void {
  if (n >= 90) {
    fmt::Println(n, "A"s);
    goto _eo_fallthrough_0;
  } else if (n >= 80) {
_eo_fallthrough_0:;
    fmt::Println(n, "at least B"s);
    goto _eo_fallthrough_1;
  } else if (n >= 70) {
_eo_fallthrough_1:;
    fmt::Println(n, "at least C"s);
  } else {
    fmt::Println(n, "below C"s);
  }
}

auto middle(int64_t n) -> void {
  if (auto _eo_switch = n; _eo_switch == 1) {
    fmt::Println(n, "one"s);
    goto _eo_fallthrough_0;
  } else if (_eo_switch == 2) {
_eo_fallthrough_1:;
    fmt::Println(n, "two"s);
  } else if (_eo_switch == 3) {
    fmt::Println(n, "three"s);
  } else {
_eo_fallthrough_0:;
    fmt::Println(n, "default"s);
    goto _eo_fallthrough_1;
  }
}

auto last(std::string s) -> void {
  if (auto _eo_switch = s; _eo_switch == "heap"s) {
    fmt::Println("collect"s);
    goto _eo_fallthrough_0;
  } else {
_eo_fallthrough_0:;
    fmt::Println("profile"s, s);
  }
}

auto find(slice<int64_t> xs, int64_t want) -> void {
  for (auto [i, x] : eo::range(xs)) {
    if (x == want) {
      fmt::Println("found"s, want, "at"s, i);
      goto _eo_break_0;
    } else if (x < 0) {
      fmt::Println("negative at"s, i);
      if (x < -5) {
        goto _eo_break_0;
      }
      fmt::Println("small negative"s);
    } else {
      continue;
    }
_eo_break_0:;
    fmt::Println("after switch"s, i);
  }
}

auto kinds(slice<any> vs) -> void {
  for (auto [_eo_range_key, v] : eo::range(vs)) {
    if (auto _eo_type_switch = v; eo::holds<int64_t>(_eo_type_switch)) {
      auto x = eo::type_assert<int64_t>(_eo_type_switch);
      if (x == 0) {
        goto _eo_break_0;
      }
      fmt::Println("int"s, x);
    } else if (eo::holds<std::string>(_eo_type_switch)) {
      auto x = eo::type_assert<std::string>(_eo_type_switch);
      fmt::Println("string"s, x);
    }
_eo_break_0:;
    fmt::Println("next"s);
  }
}

auto inner() -> void {
  {
    auto n = int64_t{3};
    if (auto _eo_switch = n; _eo_switch == 3) {
      for (auto i = int64_t{0}; i < 5; i++) {
        if (i == 2) {
          break;
        }
        fmt::Println("loop"s, i);
      }
      fmt::Println("after loop"s);
      goto _eo_break_0;
    }
_eo_break_0:;
  }
  fmt::Println("after switch"s);
}

auto eo_main() -> func<> {
  for (auto [_eo_range_key, n] : eo::range(slice<int64_t>::of({95, 85, 75, 65}))) {
    grade(n);
  }
  for (auto [_eo_range_key, n] : eo::range(slice<int64_t>::of({1, 2, 3, 4}))) {
    middle(n);
  }
  last("heap"s);
  last("cpu"s);
  find(slice<int64_t>::of({4, -1, -9, 7, 3}), 7);
  kinds(slice<any>::of({int64_t{1}, int64_t{0}, "s"s, double{2.5}}));
  inner();
  co_return;
}
