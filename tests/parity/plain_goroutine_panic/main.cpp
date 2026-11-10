#include <eo/fmt.h>
#include <eo/time.h>

#include <chrono>

using namespace eo;

void work() {
  panic("boom");
}

func<> eo_main() {
  fmt::Println("start");
  go([=]() mutable { return work(); });
  co_await time::Sleep(std::chrono::seconds{1});
  fmt::Println("unreachable");
}
