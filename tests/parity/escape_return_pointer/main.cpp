// Provisional: the C++ type of Go int is not a frozen mapping (see ../README.md).
#include <eo/fmt.h>

using namespace eo;

auto newCounter() -> ptr<int> {
  auto n = New<int>(0);
  return n;
}

void incr(ptr<int> p) {
  (*p)++;
}

func<> eo_main() {
  auto a = newCounter();
  auto b = newCounter();
  incr(a);
  incr(a);
  incr(b);
  fmt::Println(*a, *b, a == b);
  auto c = a;
  incr(c);
  fmt::Println(*a, a == c);
  co_return;
}
