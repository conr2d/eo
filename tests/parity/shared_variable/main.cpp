#include <eo/fmt.h>

#include <cstdint>

using namespace eo;

func<> eo_main() {
  auto x = New<int64_t>(0);
  auto done = eo::make_chan<bool>();
  go([=]() mutable -> func<> {
    *x = 42;
    co_await (done << true);
  });
  co_await *done;
  fmt::Println("x ="s, *x);
}
