// Provisional: struct types, composite literals, channels, and Go int are not frozen mappings (see ../README.md).
#include <eo/fmt.h>

using namespace eo;

struct Pair {
  int a{};
  int b{};
};

func<> eo_main() {
  auto p = New<Pair>(Pair{.a = 1, .b = 2});
  auto pa = ptr(p, &p->a);
  auto done = make_chan<bool>();
  go([=]() mutable -> func<> {
    *pa = 10;
    p->b = 20;
    co_await (done << true);
  });
  co_await *done;
  fmt::Println(p->a, p->b, *pa);
}
