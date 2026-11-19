#include <eo/fmt.h>

#include <cstdint>
#include <string>

using namespace eo;

auto produce(int64_t n) -> chan<int64_t> {
  auto ch = eo::make_chan<int64_t>();
  go([=]() mutable -> func<> {
    for (auto i : eo::range(n)) {
      co_await (ch << i * i);
    }
    ch.close();
  });
  return ch;
}

func<> eo_main() {
  auto sum = int64_t{0};
  for (auto _eo_range_chan = produce(5);;) {
    auto [v, _eo_range_ok] = co_await eo::recv_ok(_eo_range_chan);
    if (!_eo_range_ok) {
      break;
    }
    if (v == 4) {
      continue;
    }
    sum += v;
  }
  fmt::Println("sum"s, sum);

  auto count = int64_t{0};
  for (auto _eo_range_chan = produce(3);;) {
    auto [_eo_range_key, _eo_range_ok] = co_await eo::recv_ok(_eo_range_chan);
    if (!_eo_range_ok) {
      break;
    }
    count++;
  }
  fmt::Println("count"s, count);

  auto words = eo::make_chan<std::string>(4);
  co_await (words << "a"s);
  co_await (words << "b"s);
  co_await (words << "stop"s);
  co_await (words << "c"s);
  words.close();
  for (auto _eo_range_chan = words;;) {
    auto [w, _eo_range_ok] = co_await eo::recv_ok(_eo_range_chan);
    if (!_eo_range_ok) {
      break;
    }
    if (w == "stop"s) {
      break;
    }
    fmt::Println("word"s, w);
  }
  fmt::Println("left"s, co_await *words);

  std::string last;
  for (auto _eo_range_chan = words;;) {
    auto [_eo_range_key, _eo_range_ok] = co_await eo::recv_ok(_eo_range_chan);
    if (!_eo_range_ok) {
      break;
    }
    last = _eo_range_key;
  }
  fmt::Println("last"s, last == ""s, eo::len(last));
}
