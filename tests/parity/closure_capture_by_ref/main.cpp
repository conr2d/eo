// Provisional: the C++ type of Go int is not a frozen mapping (see ../README.md).
#include <eo/fmt.h>

using namespace eo;

func<> eo_main() {
  auto x = 0;
  auto y = 5;
  auto inc = [=, &x]() mutable { x++; };
  auto get = [=, &x]() mutable -> int { return x + y; };
  inc();
  inc();
  fmt::Println(x, get());
  co_return;
}
