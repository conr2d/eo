// Provisional: channel operations and the C++ type of Go int are not frozen mappings (see ../README.md).
#include <eo/fmt.h>

using namespace eo;

func<> eo_main() {
  auto x = New<int>(0);
  auto done = make_chan<bool>();
  go([=]() mutable -> func<> {
    *x = 42;
    co_await (done << true);
  });
  co_await *done;
  fmt::Println("x =", *x);
}
