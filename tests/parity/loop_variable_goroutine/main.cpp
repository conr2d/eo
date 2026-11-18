// Provisional: channel operations are not a frozen mapping (see ../README.md).
#include <eo/fmt.h>

#include <cstdint>

using namespace eo;

func<> eo_main() {
  auto start = make_chan<bool>();
  auto results = make_chan<int64_t>();
  for (auto i = New<int64_t>(0); *i < 4; i = New<int64_t>(*i), (*i)++) {
    go([=]() mutable -> func<> {
      co_await *start;
      *i *= 10;
      co_await (results << *i);
    });
  }
  start.close();
  auto sum = int64_t{0};
  for (auto k = int64_t{0}; k < 4; k++) {
    sum += co_await *results;
  }
  fmt::Println("sum"s, sum);
}
