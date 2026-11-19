#include <eo/fmt.h>

#include <cstdint>
#include <string>

using namespace eo;

auto push(chan<int64_t> ch, int64_t v) -> func<int64_t> {
  co_await (ch << v);
  co_return v;
}

auto pick(std::string name, chan<int64_t> ch) -> chan<int64_t> {
  fmt::Println("eval"s, name);
  return ch;
}

func<> eo_main() {
  // A send that blocked on the full channel would deadlock, so each output line
  // also shows that the receive ran before the next call.
  auto ch = eo::make_chan<int64_t>(1);
  co_await (ch << 1);
  auto _eo_call_0 = co_await *ch;
  auto _eo_call_1 = co_await push(ch, 2);
  auto _eo_call_2 = co_await *ch;
  fmt::Println(_eo_call_0, _eo_call_1, _eo_call_2);

  co_await (ch << 3);
  auto _eo_call_3 = co_await *ch;
  auto _eo_call_4 = co_await push(ch, 4);
  auto x = _eo_call_3 + _eo_call_4 * 10;
  fmt::Println(x, co_await *ch);

  // The channel operand of a send is evaluated before the value.
  auto other = eo::make_chan<int64_t>(1);
  auto _eo_call_5 = pick("chan"s, ch);
  auto _eo_call_6 = pick("value"s, other);
  auto _eo_call_7 = co_await push(_eo_call_6, 5);
  co_await (_eo_call_5 << _eo_call_7 + 1);
  auto _eo_call_8 = co_await *ch;
  auto _eo_call_9 = co_await *other;
  fmt::Println(_eo_call_8, _eo_call_9);

  // An element of a slice of channels next to a call that grows the slice.
  auto chs = slice<chan<int64_t>>::of({eo::make_chan<int64_t>(1)});
  auto grow = [=, &chs]() mutable -> int64_t {
    chs = eo::append(chs, {eo::make_chan<int64_t>(1)});
    return 7;
  };
  auto _eo_call_10 = grow();
  co_await (eo::at(chs, 0) << _eo_call_10);
  auto _eo_call_11 = co_await *eo::at(chs, 0);
  fmt::Println(_eo_call_11, eo::len(chs));

  // Receives in conditions.
  co_await (ch << 8);
  for (auto n = int64_t{0};; n++) {
    auto _eo_call_12 = co_await *ch;
    auto _eo_call_13 = co_await push(ch, n);
    if (!(_eo_call_12 > _eo_call_13)) {
      break;
    }
    fmt::Println("loop"s, n);
  }
  {
    auto _eo_call_14 = co_await *ch;
    auto _eo_call_15 = co_await push(ch, 2);
    auto v = _eo_call_14 + _eo_call_15;
    if (v != 3) {
      fmt::Println("never"s);
    } else {
      auto _eo_call_16 = pick("elif"s, ch);
      auto _eo_call_17 = co_await *_eo_call_16;
      auto _eo_call_18 = co_await push(ch, 9);
      if (_eo_call_17 == _eo_call_18) {
        fmt::Println("never"s);
      } else {
        fmt::Println("else"s, co_await *ch);
      }
    }
  }
  auto _eo_call_19 = co_await push(ch, 0) == 0;
  if (_eo_call_19) {
    auto _eo_call_20 = pick("and"s, ch);
    auto _eo_call_21 = co_await *_eo_call_20;
    auto _eo_call_22 = co_await push(ch, 6);
    _eo_call_19 = _eo_call_21 == _eo_call_22;
  }
  auto ok = _eo_call_19;
  fmt::Println(ok, co_await *ch);
}
