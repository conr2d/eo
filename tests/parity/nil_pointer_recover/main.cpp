#include <eo/fmt.h>

#include <cstdint>

using namespace eo;

struct T;

struct T {
  int64_t n{};

  auto operator==(const T&) const -> bool = default;
};

auto read(ptr<T> p) -> int64_t {
  int64_t n{};
  eo_defer_scope;
  try {
    eo_defer([&] {
      if (auto r = recover(); r != nullptr) {
        fmt::Println("recovered:"s, r);
        n = -1;
      }
    });
    n = p->n;
    eo_defer_run;
    return n;
  } catch (...) {
    eo_defer_panic;
  }
  return n;
}

func<> eo_main() {
  fmt::Println(read(New<T>(T{.n = 3})));
  fmt::Println(read(nullptr));
  co_return;
}
