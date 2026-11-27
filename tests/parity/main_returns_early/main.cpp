// Provisional: the goroutine launch shape is not a frozen mapping (see ../README.md).
#include <eo/fmt.h>
#include <eo/time.h>

#include <chrono>

using namespace eo;

func<> eo_main() {
  go([]() -> func<> {
    for (;;) {
      co_await time::Sleep(time::Millisecond);
    }
  });
  co_await time::Sleep(time::Duration{10000000});
  fmt::Println("main returns"s);
}
