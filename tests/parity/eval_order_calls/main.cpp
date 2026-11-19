#include <eo/fmt.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>

using namespace eo;

struct Box;

struct Box {
  static constexpr std::string_view _eo_name = "main.Box";

  int64_t v{};

  static auto Add(ptr<Box> b, int64_t x, int64_t y) -> int64_t;
  static auto Next(ptr<Box> b, int64_t x) -> ptr<Box>;

  auto operator==(const Box&) const -> bool = default;
};

auto trace(std::string name, int64_t v) -> int64_t {
  fmt::Println("eval"s, name);
  return v;
}

auto cond(std::string name, bool b) -> bool {
  fmt::Println("eval"s, name);
  return b;
}

auto add(int64_t a, int64_t b) -> int64_t {
  return a + b;
}

auto box(std::string name, ptr<Box> b) -> ptr<Box> {
  fmt::Println("eval"s, name);
  return b;
}

auto Box::Add(ptr<Box> b, int64_t x, int64_t y) -> int64_t {
  b->v += x + y;
  return b->v;
}

auto Box::Next(ptr<Box> b, int64_t x) -> ptr<Box> {
  b->v += x;
  return b;
}

auto pair() -> std::tuple<int64_t, int64_t> {
  return {trace("p0"s, 1), trace("p1"s, 2)};
}

func<> eo_main() {
  // Arguments and operands.
  auto _eo_call_0 = trace("a"s, 1);
  auto _eo_call_1 = trace("b"s, 2);
  fmt::Println(_eo_call_0, _eo_call_1);
  auto _eo_call_2 = trace("c"s, 3);
  auto _eo_call_3 = trace("d"s, 4);
  auto _eo_call_4 = trace("e"s, 5);
  auto x = _eo_call_2 + _eo_call_3 * _eo_call_4;
  fmt::Println(x);
  auto _eo_call_5 = trace("f"s, 6);
  auto _eo_call_6 = trace("g"s, 7);
  auto _eo_call_7 = add(_eo_call_5, _eo_call_6);
  fmt::Println(_eo_call_7);
  fmt::Println(add(trace("h"s, 8), 1));
  auto [u, w] = pair();
  fmt::Println(u, w);

  // Receivers and selectors.
  auto b = New<Box>(Box{});
  auto _eo_call_8 = box("recv"s, b);
  auto _eo_call_9 = trace("i"s, 1);
  auto _eo_call_10 = trace("j"s, 2);
  auto _eo_call_11 = Box::Add(_eo_call_8, _eo_call_9, _eo_call_10);
  fmt::Println(_eo_call_11);
  auto _eo_call_12 = box("deref"s, b);
  auto _eo_call_13 = trace("k"s, 10);
  *_eo_call_12 = Box{_eo_call_13};
  auto _eo_call_14 = box("field"s, b);
  auto _eo_call_15 = trace("l"s, 20);
  _eo_call_14->v = _eo_call_15 + 1;
  auto _eo_call_16 = box("op"s, b);
  _eo_call_16->v += trace("m"s, 5);
  fmt::Println(b->v);
  auto _eo_call_17 = box("chain"s, b);
  auto _eo_call_18 = trace("q"s, 1);
  auto _eo_call_19 = Box::Next(_eo_call_17, _eo_call_18);
  auto _eo_call_20 = trace("r"s, 2);
  auto _eo_call_21 = Box::Next(_eo_call_19, _eo_call_20);
  fmt::Println(_eo_call_21->v);

  // Short-circuit operands.
  auto _eo_call_22 = cond("l1"s, true);
  if (_eo_call_22) {
    auto _eo_call_23 = trace("s"s, 1);
    auto _eo_call_24 = trace("t"s, 2);
    auto _eo_call_25 = add(_eo_call_23, _eo_call_24);
    _eo_call_22 = _eo_call_25 == 3;
  }
  if (_eo_call_22) {
    fmt::Println("and"s);
  }
  auto _eo_call_26 = cond("l2"s, false);
  if (_eo_call_26) {
    auto _eo_call_27 = trace("never"s, 1);
    auto _eo_call_28 = trace("never"s, 2);
    auto _eo_call_29 = add(_eo_call_27, _eo_call_28);
    _eo_call_26 = _eo_call_29 == 3;
  }
  if (_eo_call_26) {
    fmt::Println("never"s);
  } else {
    fmt::Println("short"s);
  }
  auto _eo_call_30 = cond("l3"s, false);
  if (!_eo_call_30) {
    auto _eo_call_31 = trace("y"s, 1);
    auto _eo_call_32 = trace("z"s, 2);
    auto _eo_call_33 = add(_eo_call_31, _eo_call_32);
    _eo_call_30 = _eo_call_33 == 3;
  }
  auto ok = _eo_call_30;
  fmt::Println(ok);
  auto _eo_call_34 = cond("l4"s, true) || cond("never"s, true);
  auto _eo_call_35 = trace("after"s, 0);
  fmt::Println(_eo_call_34, _eo_call_35);

  // Positions that no statement can precede.
  if (trace("if"s, 1) == 0) {
    fmt::Println("never"s);
  } else {
    auto _eo_call_36 = trace("elif1"s, 2);
    auto _eo_call_37 = trace("elif2"s, 3);
    if (_eo_call_36 + _eo_call_37 == 5) {
      fmt::Println("else if"s);
    }
  }
  {
    auto n = trace("init"s, 4);
    auto _eo_call_38 = trace("c1"s, n);
    auto _eo_call_39 = trace("c2"s, 1);
    if (_eo_call_38 + _eo_call_39 == 5) {
      fmt::Println("init"s, n);
    } else {
      fmt::Println("never"s, n);
    }
  }
  for (auto i = int64_t{0};; i++) {
    auto _eo_call_40 = trace("lim"s, 3);
    auto _eo_call_41 = trace("step"s, 0);
    if (!(i < _eo_call_40 + _eo_call_41 * i)) {
      break;
    }
    if (i == 1) {
      continue;
    }
    fmt::Println("loop"s, i);
  }
  co_return;
}
