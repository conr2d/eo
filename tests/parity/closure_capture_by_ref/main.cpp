#include <eo/fmt.h>

#include <cstdint>

using namespace eo;

func<> eo_main() {
  auto x = int64_t{0};
  auto y = int64_t{5};
  auto inc = [=, &x]() mutable { x++; };
  auto get = [=, &x]() mutable -> int64_t { return x + y; };
  inc();
  inc();
  fmt::Println(x, get());
  co_return;
}
