// Provisional: the panic form is not a frozen mapping (see ../README.md).
#include <eo/fmt.h>
#include <eo/time.h>

#include <chrono>
#include <stdexcept>

using namespace eo;

void work() {
  throw std::runtime_error("panic: boom");
}

func<> eo_main() {
  fmt::Println("start");
  go([=]() mutable { return work(); });
  co_await time::Sleep(std::chrono::seconds{1});
  fmt::Println("unreachable");
}
