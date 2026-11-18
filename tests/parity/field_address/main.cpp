// Provisional: channels are not a frozen mapping (see ../README.md).
#include <eo/fmt.h>

#include <cstdint>

using namespace eo;

struct Pair;

struct Pair {
  int64_t a{};
  int64_t b{};

  auto operator==(const Pair&) const -> bool = default;
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
