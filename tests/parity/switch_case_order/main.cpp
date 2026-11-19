#include <eo/fmt.h>

#include <cstdint>
#include <string>

using namespace eo;

auto trace(std::string name, int64_t v) -> int64_t {
  fmt::Println("eval"s, name);
  return v;
}

auto check(std::string name, bool b) -> bool {
  fmt::Println("eval"s, name);
  return b;
}

auto pick(int64_t n) -> void {
  {
    auto _eo_switch = trace("tag"s, n);
    auto _eo_call_0 = trace("a"s, 1);
    auto _eo_call_1 = trace("b"s, 0);
    if (_eo_switch == _eo_call_0 + _eo_call_1) {
      fmt::Println("first"s);
    } else {
      auto _eo_call_2 = _eo_switch == trace("c"s, 2);
      if (!_eo_call_2) {
        auto _eo_call_3 = trace("d"s, 1);
        auto _eo_call_4 = trace("e"s, 3);
        _eo_call_2 = _eo_switch == _eo_call_3 * _eo_call_4;
      }
      if (_eo_call_2) {
        fmt::Println("second"s);
      } else {
        fmt::Println("default"s);
      }
    }
  }
}

auto test(int64_t n) -> void {
  auto _eo_call_0 = check("x"s, n > 5);
  if (_eo_call_0) {
    auto _eo_call_1 = trace("f"s, n);
    auto _eo_call_2 = trace("g"s, 1);
    _eo_call_0 = check("y"s, _eo_call_1 + _eo_call_2 > 8);
  }
  if (_eo_call_0) {
    fmt::Println("big"s);
  } else {
    auto _eo_call_3 = trace("h"s, n);
    auto _eo_call_4 = trace("i"s, 0);
    if (_eo_call_3 + _eo_call_4 == 3) {
      fmt::Println("three"s);
    }
  }
}

auto eo_main() -> func<> {
  pick(1);
  pick(3);
  pick(5);
  test(9);
  test(6);
  test(3);
  co_return;
}
