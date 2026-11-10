// Provisional: goroutine launch and closure capture of the boxed x are not frozen mappings (see ../README.md).
#include <eo/fmt.h>

#include <memory>

using namespace eo;

func<> eo_main() {
  auto x = std::make_shared<int>(0);
  auto done = make_chan<bool>();
  go([x, done]() -> func<> {
    *x = 42;
    co_await (done << true);
  });
  co_await *done;
  fmt::Println("x =", *x);
}
