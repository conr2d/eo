// Provisional: the goroutine launch shape is not a frozen mapping (see ../README.md).
#include <eo/fmt.h>
#include <eo/time.h>

#include <chrono>

using namespace eo;

func<> eo_main() {
  fmt::Println("start"s);
  go([]() -> func<> {
    panic("boom"s);
    co_return;
  });
  co_await time::Sleep(std::chrono::seconds{1});
  fmt::Println("unreachable"s);
}
