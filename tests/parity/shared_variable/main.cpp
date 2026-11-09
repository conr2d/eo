#include <eo/fmt.h>

#include <memory>

using namespace eo;

// Provisional: closure capture of shared variables is not a frozen mapping yet.
// x is boxed so that the goroutine and main share one variable, as in Go.
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
