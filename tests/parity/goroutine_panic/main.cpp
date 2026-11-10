// Provisional: the goroutine launch shape and the panic form are not frozen mappings (see ../README.md).
#include <eo/fmt.h>
#include <eo/time.h>

#include <chrono>
#include <stdexcept>

using namespace eo;

func<> eo_main() {
  fmt::Println("start");
  go([]() -> func<> {
    throw std::runtime_error("panic: boom");
    co_return;
  });
  co_await time::Sleep(std::chrono::seconds{1});
  fmt::Println("unreachable");
}
