#include <eo/fmt.h>

#include <cstdint>

using namespace eo;

auto newCounter() -> ptr<int64_t> {
  auto n = New<int64_t>(0);
  return n;
}

void incr(ptr<int64_t> p) {
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
