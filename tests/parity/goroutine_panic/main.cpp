#include <eo/fmt.h>

#include <stdexcept>

using namespace eo;

func<> eo_main() {
  fmt::Println("start");
  auto done = make_chan<bool>();
  go([]() -> func<> {
    throw std::runtime_error("panic: boom");
    co_return;
  });
  co_await *done;
  fmt::Println("unreachable");
}
