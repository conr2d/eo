// Provisional: channel operations and the C++ type of Go int are not frozen mappings (see ../README.md).
#include <eo/fmt.h>

using namespace eo;

func<> eo_main() {
  auto start = make_chan<bool>();
  auto results = make_chan<int>();
  for (auto i = New<int>(0); *i < 4; i = New<int>(*i), (*i)++) {
    go([=]() mutable -> func<> {
      co_await *start;
      *i *= 10;
      co_await (results << *i);
    });
  }
  start.close();
  auto sum = 0;
  for (auto k = 0; k < 4; k++) {
    sum += co_await *results;
  }
  fmt::Println("sum"s, sum);
}
